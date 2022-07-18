#include <boost/interprocess/detail/os_thread_functions.hpp>

#include "cxpstelemetrylogger.h"
#include "Rows/SourceFileTelemetryRow.h"

namespace CxpsTelemetry
{
    // Static data member definitions
    boost::atomic<bool> SourceGeneralTelemetryRow::s_isMappingInitialized(false);
    size_t SourceGeneralTelemetryRow::s_fileTypeTelDataIndices[FileType_Count];
    std::vector<RequestType> SourceGeneralTelemetryRow::s_trackedReqTypes;
    std::vector<FileType> SourceGeneralTelemetryRow::s_trackedFileTypes;

    CxpsTelemetryRowBase::CxpsTelemetryRowBase(MessageType messageType)
    {
        m_isOwned = false; // Object would be used for temp and caching purposes.
        m_messageType = messageType;

        // Don't invoke Clear(), which is a virtual function, while this is local members only.
        ClearSelfMembers();
    }

    CxpsTelemetryRowBase::CxpsTelemetryRowBase(
        MessageType messageType, boost_pt::ptime loggerStartTime, const std::string &hostId, const std::string &device)
    {
        m_isOwned = true; // Object would be used for persistent tracking of a resource.
        m_messageType = messageType;

        // Don't invoke Clear(), which is a virtual function, while this is local members only.
        ClearSelfMembers();

        m_loggerStartTime = loggerStartTime;
        m_loggerEndTime = loggerStartTime;

        m_hostId = hostId;
        m_device = device;
    }

    bool CxpsTelemetryRowBase::IsValid()
    {
        // Although the value is atomic, the lock is obtained to block interpretting
        // while an operation is in progress on the object.
        RecursiveLockGuard guard(m_rowObjLock);

        return IsObjValidSlim();
    }

    void CxpsTelemetryRowBase::Clear()
    {
        RecursiveLockGuard guard(m_rowObjLock);

        OnClear(); // Clear sub-class members

        // Keep this at last, so Valid bit as the final step.
        ClearSelfMembers(); // Clear self-members
    }

    void CxpsTelemetryRowBase::ClearSelfMembers()
    {
        // m_isOwned and m_messageType must not be reset

        m_pendingReqInfo.clear();
        m_hostId.clear();
        m_device.clear();

        m_loggerStartTime = Defaults::s_defaultPTime;
        m_loggerEndTime = Defaults::s_defaultPTime;
        m_reqsStartTime = Defaults::s_defaultPTime;
        m_reqsEndTime = Defaults::s_defaultPTime;

        m_seqNum = 0;
        MarkModCompletionForObj(); // Object is valid after resetting all the members.
    }

    bool CxpsTelemetryRowBase::IsInvalidOrEmpty()
    {
        RecursiveLockGuard guard(m_rowObjLock);

        return !IsObjValidSlim() || this->IsEmpty();
    }

    // TODO-SanKumar-1710: Mark the end time and commit with the OnComplete routine?!
    bool CxpsTelemetryRowBase::GetPrevWindowRow(CxpsTelemetryRowBase &prev, boost_pt::ptime timeOfRow)
    {
        // Always ensure this lock ordering across the class : Prev -> this
        RecursiveLockGuard guardPrev(prev.m_rowObjLock);
        RecursiveLockGuard guard(m_rowObjLock);

        BOOST_ASSERT(!this->IsCacheObj() && prev.IsCacheObj());

        // The prev obj is not cleared and/or this object is inconsistent.
        if (!prev.IsObjValidSlim() || !IsObjValidSlim())
        {
            BOOST_ASSERT(prev.IsObjValidSlim());
            return false;
        }

        // TODO-SanKumar-1710: Should we fail, if it differs?
        BOOST_ASSERT(prev.m_messageType == m_messageType);

        try
        {
            prev.MarkObjForModification();

            // Do the action in the sub class
            if (!OnGetPrevWindow_CopyPhase1(prev))
                return false;

            prev.m_pendingReqInfo = m_pendingReqInfo;
            prev.m_hostId = m_hostId;
            prev.m_device = m_device;

            prev.m_loggerStartTime = m_loggerStartTime;
            prev.m_loggerEndTime = timeOfRow;
            prev.m_reqsStartTime = m_reqsStartTime;
            prev.m_reqsEndTime = m_reqsEndTime;

            prev.MarkModCompletionForObj();

            MarkObjForModification();

            // Do the action in the sub class
            if (!OnGetPrevWindow_CopyPhase2(prev))
                return false;

            m_loggerStartTime = timeOfRow;
            m_loggerEndTime = timeOfRow;
            m_reqsStartTime = Defaults::s_defaultPTime;
            m_reqsEndTime = Defaults::s_defaultPTime;

            // m_seqNum is externally added, so not involved in these transactions.

            MarkModCompletionForObj();

            return true;
        }
        GENERIC_CATCH_LOG_ACTION("CxpsTelemetryRowBase::GetPrevWindowRow", return false);
    }

    bool CxpsTelemetryRowBase::AddBackPrevWindow(const CxpsTelemetryRowBase &prev)
    {
        // Always ensure this lock ordering across the class : Prev -> this
        RecursiveLockGuard guardPrev(prev.m_rowObjLock);
        RecursiveLockGuard guard(m_rowObjLock);

        BOOST_ASSERT(!this->IsCacheObj() && prev.IsCacheObj());

        // The prev obj is not cleared and/or this object is inconsistent.
        if (!prev.IsObjValidSlim() || !IsObjValidSlim())
        {
            BOOST_ASSERT(prev.IsObjValidSlim());
            return false;
        }

        try
        {
            BOOST_ASSERT(m_messageType == prev.m_messageType);
            // m_pendingReqInfo remains as the latest window's value.
            BOOST_ASSERT(m_hostId == prev.m_hostId);
            BOOST_ASSERT(m_device == prev.m_device);

            MarkObjForModification();

            // Do the action in the sub class
            if (!OnAddBackPrevWindow(prev))
                return false;

            m_loggerStartTime = prev.m_loggerStartTime;
            // m_loggerEndTime of the current window is retained.

            BOOST_ASSERT(prev.m_reqsStartTime.is_special() == prev.m_reqsEndTime.is_special());
            BOOST_ASSERT(m_reqsStartTime.is_special() == m_reqsEndTime.is_special());

            // If there has been no request in the prev window, then curr window's start time is taken.
            if (!prev.m_reqsStartTime.is_special())
                m_reqsStartTime = prev.m_reqsStartTime;

            // If there has been no request in the curr window, then prev window's end time is taken.
            if (m_reqsEndTime.is_special())
                m_reqsEndTime = prev.m_reqsEndTime;

            // m_seqNum is externally added before serialization. It's volatile and not persisted
            // within an owned object.

            MarkModCompletionForObj();

            return true;
        }
        GENERIC_CATCH_LOG_ACTION("CxpsTelemetryRowBase::AddBackPrevWindow", return false);
    }

    bool CxpsTelemetryRowBase::AddToTelemetry(const std::string &pendingReqInfo, const RequestTelemetryData &reqTelData)
    {
        RecursiveLockGuard guard(m_rowObjLock);

        if (!IsObjValidSlim())
            return false; // The object is inconsistent

        try
        {
            BOOST_ASSERT(!this->IsCacheObj());

            // TODO-SanKumar-1710: If the session closes while waiting for new request,
            // by current collection logic, the start time would be special.
            // This would lead to incorrect data as the start will be moved to a
            // later point on another completed request. So, in this case, mark
            // start = end and then enable this assert.
            // BOOST_ASSERT(!reqTelData.m_requestStartTime.is_special());
            // BOOST_ASSERT(m_reqsStartTime.is_special() == m_reqsEndTime.is_special());
            BOOST_ASSERT(!reqTelData.m_requestEndTime.is_special());

            m_pendingReqInfo.reserve(pendingReqInfo.size());

            MarkObjForModification();

            m_pendingReqInfo = pendingReqInfo;

            // If there wasn't a previous request before this, update the start time.
            if (m_reqsStartTime.is_special())
                m_reqsStartTime = reqTelData.m_requestStartTime;

            // The curr req value is always the new end.
            if (!reqTelData.m_requestEndTime.is_special())
                m_reqsEndTime = reqTelData.m_requestEndTime;

            MarkModCompletionForObj();

            return true;
        }
        // TODO-SanKumar-1711: If bad_alloc, then process has low memory. So, avoid logging.
        // Do this for all of cxps telemetry code, as telemetry is of lower priority than data.
        GENERIC_CATCH_LOG_ACTION("CxpsTelemetryRowBase::UpdateRequestInfo", return false);
    }

    void CxpsTelemetryRowBase::SetSeqNumber(int64_t seqNum)
    {
        RecursiveLockGuard guard(m_rowObjLock);

        BOOST_ASSERT(IsCacheObj());

        // No need to check the object validity, since this is a volatile member
        // that doesn't correspond to the consistency of the object.

        m_seqNum = seqNum;
    }

    MessageType CxpsTelemetryRowBase::GetMessageType()
    {
        // No need to check the object validity or lock, since this is not affected by any mod.

        return m_messageType;
    }

    void CxpsTelemetryRowBase::RefreshSystemIdentifiers()
    {
#ifdef YET_TO_LINK_HOST_AGENT_HELPERS
        if (s_biosId.empty())
            Utils::TryGetBiosId(s_biosId);

        if (s_fabType.empty())
            Utils::TryGetFabricType(s_fabType);
#endif // YET_TO_LINK_HOST_AGENT_HELPERS

        if (s_psHostId.empty())
        {
            // TODO-SanKumar-1711: Should we try to parse the config, in case it
            // failed at the service start?
            // Currently the config is only parsed at the start service method
            // and that pattern has been continued here as well.
            try
            {
                s_psHostId = CxpsTelemetryLogger::GetInstance().GetPsId();
            }
            catch (std::bad_alloc) {} //Ignore any failure.
        }

        if (s_psAgentVer.empty())
            Utils::GetProductVersion(s_psAgentVer);

        if (s_psProcessId == 0)
            s_psProcessId = boost::interprocess::ipcdetail::get_current_process_id();
    }

    void CxpsTelemetryRowBase::serialize(JSON::Adapter &adapter)
    {
        namespace ColNames = Strings::MdsColumnNames;

        JSON::Class root(adapter, "CxpsTelemetryRowBase", false);

        RecursiveLockGuard guard(m_rowObjLock);

        BOOST_ASSERT(IsCacheObj());

        if (!IsObjValidSlim())
            throw std::runtime_error("Invalid CxpsTelemetryRowBase object");

        // Message column is not used in this table, as of now. So not constructing it.

        int64_t int_messageType = m_messageType;
        std::string str_loggerStartTime = boost_pt::to_iso_extended_string(m_loggerStartTime);
        std::string str_loggerEndTime = boost_pt::to_iso_extended_string(m_loggerEndTime);

        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::MessageType, int_messageType);
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::LoggerStartTime, str_loggerStartTime);
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::LoggerEndTime, str_loggerEndTime);

        if (!m_reqsStartTime.is_special())
        {
            std::string str_reqsStartTime = boost_pt::to_iso_extended_string(m_reqsStartTime);
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::ReqsStartTime, str_reqsStartTime);
        }

        if (!m_reqsEndTime.is_special())
        {
            std::string str_reqsEndTime = boost_pt::to_iso_extended_string(m_reqsEndTime);
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::ReqsEndTime, str_reqsEndTime);
        }

        if (!m_hostId.empty())
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::HostId, m_hostId);

        if (!m_device.empty())
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::Device, m_device);

        if (!m_pendingReqInfo.empty())
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::PendingReqInfo, m_pendingReqInfo);

        // Retrieve the dynamic data from the base class.
        std::string dynamicJsonData;
        this->RetrieveDynamicJsonData(dynamicJsonData);
        if (!dynamicJsonData.empty())
            JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::CustomJson, dynamicJsonData);

        // Include system identifiers
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::BiosId, s_biosId);
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::FabType, s_fabType);
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::PSHostId, s_psHostId);
        JSON_E_KV_PRODUCER_ONLY(adapter, ColNames::PSAgentVer, s_psAgentVer);

        JSON_T_KV_PRODUCER_ONLY(adapter, ColNames::SeqNum, m_seqNum);
    }

    void SourceFileTelemetryRow_GenerateTelDataMapping(MessageType msgType, boost::mutex &lock, boost::atomic<bool> &isMappingInitialized)
    {
        LockGuard guard(lock);

        // Double-checked pattern
        if (!isMappingInitialized)
        {
            // TODO-SanKumar-1711: Little ugly: Violates OOD by bringing sub class info into base class.
            switch (msgType)
            {
            case MsgType_SourceOpsDiffSync:
                SourceDiffTelemetryRow::GenerateTelDataMapping(
                    Stringer::DiffSyncFileTypeToString, Tunables::DetailedDiffRequestTypes,
                    Tunables::PerfCountedDiffTypes, DiffSyncFileType_All);
                break;
            case MsgType_SourceOpsResync:
                SourceResyncTelemetryRow::GenerateTelDataMapping(
                    Stringer::ResyncFileTypeToString, Tunables::DetailedResyncRequestTypes,
                    Tunables::PerfCountedResyncTypes, ResyncFileType_All);
                break;
            default:
                throw std::logic_error("GenerateTelDataMapping - unimplemented message type");
            }

            isMappingInitialized = true;
        }
    }
}