#ifndef SRC_GEN_TEL_ROW_H
#define SRC_GEN_TEL_ROW_H

#include "CxpsTelemetryRowBase.h"

namespace CxpsTelemetry
{
    class SourceGeneralTelemetryRow : public CxpsTelemetryRowBase
    {
    public:
        SourceGeneralTelemetryRow()
            : CxpsTelemetryRowBase(MsgType_SourceGeneral),
            m_perRequestTelData(RequestType_Count),
            m_perFileTypeTelData(TrackedFileTypesCount)
        {
            if (!s_isMappingInitialized)
                GenerateTelDataMapping();

            Initialize();
        }

        SourceGeneralTelemetryRow(boost_pt::ptime loggerStartTime, const std::string &hostId)
            : CxpsTelemetryRowBase(MsgType_SourceGeneral, loggerStartTime, hostId, std::string()),
            m_perRequestTelData(RequestType_Count),
            m_perFileTypeTelData(TrackedFileTypesCount)
        {
            if (!s_isMappingInitialized)
                GenerateTelDataMapping();

            Initialize();
        }

        virtual ~SourceGeneralTelemetryRow()
        {
            BOOST_ASSERT(IsCacheObj() || IsEmpty());
        }

        bool AddToTelemetry(FileType fileType, RequestType reqType, const RequestTelemetryData &reqTelData)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            if (!CxpsTelemetryRowBase::AddToTelemetry(std::string(), reqTelData))
                return false;

            MarkObjForModification();

            if (!m_perRequestTelData[reqType].AddRequestDataToTelemetry(reqTelData))
                return false;

            size_t trackedFileTelInd = s_fileTypeTelDataIndices[fileType];
            BOOST_ASSERT(trackedFileTelInd != UntrackedFileTypeInd);
            // TODO-SanKumar-1711: Should we add an error in global telemetry for this?

            if (trackedFileTelInd != UntrackedFileTypeInd)
            {
                if (!m_perFileTypeTelData[trackedFileTelInd].AddRequestDataToTelemetry(reqTelData))
                    return false;
            }

            MarkModCompletionForObj();
            return true;
        }

        virtual bool IsEmpty() const
        {
            RecursiveLockGuard guard(m_rowObjLock);

            for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                if (!m_perRequestTelData[reqInd].IsEmpty())
                    return false;

            for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                if (!m_perFileTypeTelData[fileTypeInd].IsEmpty())
                    return false;

            return true;
        }

    protected:
        virtual void OnClear()
        {
            ClearSelfMembers();
        }

        virtual bool OnGetPrevWindow_CopyPhase1(CxpsTelemetryRowBase &prevWindowRow) const
        {
            SourceGeneralTelemetryRow& prev = dynamic_cast<SourceGeneralTelemetryRow&>(prevWindowRow);

            try
            {
                for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                    if (!m_perRequestTelData[reqInd].GetPrevWindow_CopyPhase1(prev.m_perRequestTelData[reqInd]))
                        return false;

                for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                    if (!m_perFileTypeTelData[fileTypeInd].GetPrevWindow_CopyPhase1(prev.m_perFileTypeTelData[fileTypeInd]))
                        return false;

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("SourceGeneralTelemetryRow::OnGetPrevWindow_CopyPhase1", return false);
        }

        virtual bool OnGetPrevWindow_CopyPhase2(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const SourceGeneralTelemetryRow& prev = dynamic_cast<const SourceGeneralTelemetryRow&>(prevWindowRow);

            try
            {
                for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                    if (!m_perRequestTelData[reqInd].GetPrevWindow_CopyPhase2(prev.m_perRequestTelData[reqInd]))
                        return false;

                for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                    if (!m_perFileTypeTelData[fileTypeInd].GetPrevWindow_CopyPhase2(prev.m_perFileTypeTelData[fileTypeInd]))
                        return false;

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("SourceGeneralTelemetryRow::OnGetPrevWindow_CopyPhase2", return false);
        }

        virtual bool OnAddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const SourceGeneralTelemetryRow& prev = dynamic_cast<const SourceGeneralTelemetryRow&>(prevWindowRow);

            try
            {
                for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                    if (!m_perRequestTelData[reqInd].AddBackPrevWindow(prev.m_perRequestTelData[reqInd]))
                        return false;

                for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                    if (!m_perFileTypeTelData[fileTypeInd].AddBackPrevWindow(prev.m_perFileTypeTelData[fileTypeInd]))
                        return false;

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("SourceGeneralTelemetryRow::OnGetPrevWindow_CopyPhase2", return false);
        }

        virtual void RetrieveDynamicJsonData(std::string &dynamicJsonData)
        {
            CustomData customData(*this);
            dynamicJsonData = JSON::producer<CustomData>::convert(customData);
        }

        class CustomData
        {
        public:
            CustomData(const SourceGeneralTelemetryRow &rowObj)
                : m_rowObj(rowObj)
            {
            }

            void serialize(JSON::Adapter &adapter)
            {
                JSON::Class root(adapter, "CustomData", false);

                typedef int DummyType; // Work around to use FileTelemetryDataCollection class.
                FileTelemetryDataCollection<DummyType, DummyType> fileTelDataColl;

                std::vector<RequestType> emptyReqTypes;
                std::vector<FileType> emptyFileTypes;
                std::vector<DummyType> emptyFileSubTypes, emptyFileMetdataRanges;

                if (fileTelDataColl.Load(s_trackedReqTypes, emptyFileTypes, emptyFileSubTypes, NULL, m_rowObj.m_perRequestTelData, emptyFileMetdataRanges))
                    JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceGeneral::PerReqData, fileTelDataColl);

                if (fileTelDataColl.Load(emptyReqTypes, s_trackedFileTypes, emptyFileSubTypes, NULL, m_rowObj.m_perFileTypeTelData, emptyFileMetdataRanges))
                    JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceGeneral::PerFTypeData, fileTelDataColl);

                JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::Pid, s_psProcessId);
            }

        private:
            const SourceGeneralTelemetryRow &m_rowObj;
        };

    private:
        static const size_t TrackedFileTypesCount = Tunables::SourceGenTelTrackedFileTypesCount;
        static const size_t UntrackedFileTypeInd = INT_MAX;

        static boost::atomic<bool> s_isMappingInitialized;
        static size_t s_fileTypeTelDataIndices[FileType_Count];

        // TODO-SanKumar-1711: Temp: It's not the right way to create these vectors
        // from const arrays. Load function of FileTelemetryDataCollection must
        // take the begin and end ranges instead (hint: boost range would be a candidate).
        static std::vector<RequestType> s_trackedReqTypes;
        static std::vector<FileType> s_trackedFileTypes;

        std::vector<FileTelemetryData> m_perRequestTelData, m_perFileTypeTelData;

        static void GenerateTelDataMapping()
        {
            static boost::mutex lock;
            LockGuard guard(lock);

            // Double-checked pattern
            if (!s_isMappingInitialized)
            {
                const FileType* TrackedFileTypes = Tunables::SourceGenTelemetryTrackedFileTypes;

                size_t trackedFileTypeInd = 0;
                for (size_t fileTypeInd = 0; fileTypeInd < FileType_Count; fileTypeInd++)
                {
                    if (std::find(TrackedFileTypes, TrackedFileTypes + TrackedFileTypesCount, fileTypeInd)
                        != TrackedFileTypes + TrackedFileTypesCount)
                    {
                        s_fileTypeTelDataIndices[fileTypeInd] = trackedFileTypeInd++;
                    }
                    else
                    {
                        s_fileTypeTelDataIndices[fileTypeInd] = UntrackedFileTypeInd;
                    }
                }

                BOOST_ASSERT(trackedFileTypeInd == TrackedFileTypesCount);

                s_trackedFileTypes.clear();
                s_trackedFileTypes.reserve(TrackedFileTypesCount);
                s_trackedFileTypes.assign(TrackedFileTypes, TrackedFileTypes + TrackedFileTypesCount);

                s_trackedReqTypes.clear();
                s_trackedReqTypes.reserve(RequestType_Count);
                for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                    s_trackedReqTypes.push_back(static_cast<RequestType>(reqInd));

                s_isMappingInitialized = true;
            }
        }

        void Initialize()
        {
            for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                m_perRequestTelData[reqInd].Initialize(false);

            for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                m_perFileTypeTelData[fileTypeInd].Initialize(false);
        }

        void ClearSelfMembers()
        {
            for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
                m_perRequestTelData[reqInd].Clear();

            for (size_t fileTypeInd = 0; fileTypeInd < TrackedFileTypesCount; fileTypeInd++)
                m_perFileTypeTelData[fileTypeInd].Clear();
        }
    };

    typedef boost::shared_ptr<SourceGeneralTelemetryRow> SourceGeneralTelemetryRowPtr;
}

#endif // !SRC_GEN_TEL_ROW_H
