#include <listfile.h>

#include "cxpstelemetrylogger.h"
#include "cxps.h"

namespace CxpsTelemetry
{
    // Static data member definitions
    CxpsTelemetryLogger CxpsTelemetryLogger::s_Instance;

    std::string CxpsTelemetryRowBase::s_biosId;
    std::string CxpsTelemetryRowBase::s_fabType;
    std::string CxpsTelemetryRowBase::s_psHostId;
    std::string CxpsTelemetryRowBase::s_psAgentVer;
    uint64_t CxpsTelemetryRowBase::s_psProcessId = 0;

    boost_pt::ptime Defaults::s_defaultPTime; // == Default construction
    SteadyClock::time_point Defaults::s_defaultTimePoint; // == time_since_epoch() == 0
    chrono::nanoseconds Defaults::s_zeroTime; // == chrono::nanoseconds::zero()

    SourceDiffTelemetryRow DeviceLevelTelemetryMap::s_sourceDiffTel_CacheObj;
    SourceResyncTelemetryRow DeviceLevelTelemetryMap::s_sourceResyncTel_CacheObj;
    SourceGeneralTelemetryRow HostLevelTelemetryMap::s_sourceGenTel_CacheObj;

    // String constants
    namespace FileAttributes
    {
        const std::string CxpsTransportTableName("InMageTelemetryPSTransport");
        const std::string CompletedFilePart("_completed_");
        const std::string CurrentFileSuffix("_current");
        const std::string Extension(".json");
    }

    class MdsLogUnit
    {
    public:
        void serialize(JSON::Adapter &adapter)
        {
            JSON::Class root(adapter, "MdsLogUnit", false);

            JSON_T_KV_PRODUCER_ONLY(adapter, "Map", m_rowToConvert);
        }

        static void ConvertTelemetryRowToMARSFormat(CxpsTelemetryRowBase &rowToConvert, std::string &fileRow)
        {
            CSMode csMode = GetCSMode();
            if (csMode == CS_MODE_RCM)
            {
                fileRow = JSON::producer<CxpsTelemetryRowBase>::convert(rowToConvert);
            }
            else
            {
                MdsLogUnit tempLogUnit(rowToConvert);
                fileRow = JSON::producer<MdsLogUnit>::convert(tempLogUnit);
            }
        }

    private:
        CxpsTelemetryRowBase &m_rowToConvert;

        MdsLogUnit(CxpsTelemetryRowBase &rowToConvert)
            : m_rowToConvert(rowToConvert)
        {
        }
    };

    void CxpsTelemetryLogger::Initialize(
        const std::string &psId,
        const boost_fs::path &folderPath,
        boost::chrono::seconds writeInterval,
        int maxCompletedFilesCnt)
    {
        namespace FA = FileAttributes;

        m_psId = psId;
        m_folderPath = folderPath;
        m_writeInterval = writeInterval;
        m_maxCompletedFilesCnt = maxCompletedFilesCnt;

        if (!m_folderPath.has_root_directory())
            throw std::runtime_error("CxpsTelemetryLogger output folder path is invalid : " + m_folderPath.string());

        // InMageTelemetryPSTransport_completed_
        m_completedFileNamePrefix = FA::CxpsTransportTableName + FA::CompletedFilePart;

        // <folderPath>/InMageTelemetryPSTransport_completed_*.json
        m_completedFileSearchPattern = m_folderPath /
            (m_completedFileNamePrefix + "*" + FA::Extension);

        // InMageTelemetryPSTransport_current.json
        std::string currentFileName =
            FA::CxpsTransportTableName + FA::CurrentFileSuffix + FA::Extension;

        // <folderPath>/InMageTelemetryPSTransport_current.json
        m_currentFilePath = m_folderPath / currentFileName;

        m_isInitialized = true;
    }

    void CxpsTelemetryLogger::Start()
    {
        if (!m_isInitialized)
        {
            BOOST_ASSERT(false);
            throw std::logic_error("CxpsTelemetryLogger is started without initialization");
        }

        BOOST_ASSERT(!m_thread);
        // On thread spawn failure, the exception would stop the server. So, it's
        // guaranteed that this thread would always be running, as long as the
        // server is running.
        m_thread.reset(
            new boost::thread(boost::bind(&CxpsTelemetryLogger::Run, this)));
    }

    void CxpsTelemetryLogger::Stop()
    {
        // If the thread was created, signal completion and wait for it to finish.
        if (m_thread)
        {
            BOOST_ASSERT(m_isInitialized);

            m_thread->interrupt();
            m_thread->join();
            m_thread.reset();
        }
    }

    void CxpsTelemetryLogger::Run()
    {
        bool hasLoggerStopped = false;
        size_t dbgIgnoredExceptionCnt = 0;

        CXPS_LOG_ERROR_INFO("Starting CxpsTelemetryLogger thread ");

        while (!hasLoggerStopped)
        {
            try
            {
                try
                {
                    boost::this_thread::sleep_for(m_writeInterval);
                }
                catch (boost::thread_interrupted)
                {
                    CXPS_LOG_ERROR_INFO("Stopping CxpsTelemetryLogger thread "
                        "(after flushing the remaining telemetry data)");

                    hasLoggerStopped = true;
                    // Write the remainder data into the next file and quit.

                    // TODO-SanKumar-1710: Assert that at the final collection,
                    // there's no pending data logged, since all the server threads
                    // are awaited before stopping telemetry logger. We should be
                    // able to account for every data till this point.
                }

                // If any of the identifiers couldn't be retrieved, we fill in
                // empty value and continue. Try to retrieve failed ones before
                // writing each file.
                CxpsTelemetryRowBase::RefreshSystemIdentifiers();

                // TODO-SanKumar-1710: Listen for cancellation within this long running call (or)
                // is it better to dump all the data that we have and then spinning off cxps server,
                // as currently implemented.
                this->WriteDataToNextFile(hasLoggerStopped);
            }
            // TODO: Move this comment to serverctl.cpp (Actually it's not happening. Even getting the stopping log as of today)
            // TODO-SanKumar-1709: Add a log for any exception.
            // The problem is that simple log monitor thread might be closed, due
            // to the thread join ordering in the stopservers(). To add logging
            // to CxpsTelemetryLogger, split the monitoring and server threads
            // joining and stop threads in the following order: server threads,
            // telLogger and finally monitor threads.
            GENERIC_CATCH_LOG_ACTION("CxpsTelemetryLogger run loop", dbgIgnoredExceptionCnt++)
        }
    }

    void CxpsTelemetryLogger::WriteDataToNextFile(bool isFinalWrite)
    {
        // Poll for the files and see if there's possibility of adding one more file.
        int completedFileCnt = 0;
        {
            std::string fileList;
            const bool IncludeFileSize = false;
            ListFile::listFileGlob(m_completedFileSearchPattern, fileList, IncludeFileSize);

            for (size_t matchInd = fileList.find('\n'); matchInd != std::string::npos; completedFileCnt++)
                matchInd = fileList.find('\n', matchInd + 1);
        }

        if (!isFinalWrite && completedFileCnt >= m_maxCompletedFilesCnt)
        {
            // The data won't be flushed and so the interval of the data would continue to expand till
            // the next write. We actually don't lose any data.

            // TODO-SanKumar-1711: Should we add a counter for this and ship it
            // as part of the logs for better telemetry debugging?
            return;
        }

        boost::system::error_code errorCode;
        if (!boost_fs::exists(m_currentFilePath.parent_path(), errorCode))
            boost_fs::create_directories(m_currentFilePath.parent_path(), errorCode);

        if (errorCode)
            throw std::runtime_error(errorCode.message());

        ++m_printSessionId;
        m_curFileRowCount = 0;
        try
        {
            bool isRowObjUnusable;
            bool shouldContinueWritingToFile;

            std::ofstream currentFileOutStream;
            currentFileOutStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);

            currentFileOutStream.open(m_currentFilePath.string().c_str(), std::ofstream::trunc | std::ofstream::binary);

            CxpsTelemetryRowBasePtr nextTelRowToPrint;
            CxpsTelemetryRowBase *cacheObj;
            while (nextTelRowToPrint = m_hostTelemetryMap.GetNextTelemetryRowToPrint(m_printSessionId, &cacheObj))
            {
                // TODO-SanKumar-1710: For a moment in between the above and below calls, the lock
                // could be owned by another request completion and that could probably invalidate
                // the row object. So, we should overcome that by acquiring lock, validating the
                // object, checking if not empty here, then write the row and finally releasing the
                // lock. Applies for all the row objects.
                if (!nextTelRowToPrint->IsEmpty())
                {
                    shouldContinueWritingToFile = this->WriteRowToFile(
                        currentFileOutStream, *nextTelRowToPrint, *cacheObj, isRowObjUnusable);

                    // TODO-SanKumar-1711: Currently, we don't know the key to the
                    // obj in the host tel map. So, we can't regenerate a valid obj,
                    // by calling GetValidRow(key) - see GlobalTelemetry reset below.
                    // Doing that operation in this thread avoids the load on the
                    // server worker thread.
                    //if(isRowObjUnusable) { Reset here... }

                    if (!shouldContinueWritingToFile)
                        goto WritesCompleted;
                }
            }

            GlobalCxpsTelemetryRowPtr globalTel = PruneOrGetGlobalTelemetry();
            if (globalTel && !globalTel->IsEmpty())
            {
                shouldContinueWritingToFile = this->WriteRowToFile(
                    currentFileOutStream, *globalTel, m_cacheGlobalTelRow, isRowObjUnusable);

                // Refresh the telemetry object (unless already done by another thread)
                if (isRowObjUnusable)
                    GetValidGlobalTelemetry();

                if (!shouldContinueWritingToFile)
                    goto WritesCompleted;
            }

            if (m_curFileRowCount == 0)
            {
                // No data. Write an empty message.
                // We have to generate an empty telemetry row to cut from, since we can't persist
                // empty row's object across iterations. Say at itr1 - no data, itr2 - data and
                // itr3 - no data. If a persisted object is used, the empty row of itr3 would have
                // logger time from itr1 to tr3, where it must be itr2 to itr3.
                EmptyCxpsTelemetryRow currEmptyTelemetry(GetLastRowUploadTime());

                shouldContinueWritingToFile = this->WriteRowToFile(
                    currentFileOutStream, currEmptyTelemetry, m_cacheEmptyTelRow, isRowObjUnusable);

                if (!shouldContinueWritingToFile)
                    goto WritesCompleted;
            }
        }
        // TODO-SanKumar-1710: Any exception caught is forgotten after this point.
        // Should we point that somewhere? Is the message drop logged already by
        // the above block enough?
        // TODO-SanKumar-1711: If there's low disk space, we would end up eating
        // more disk space with this log, which will one line per each file.
        GENERIC_CATCH_LOG_IGNORE("Writing telemetry data to file");

    WritesCompleted:
        // On both success and falilure, transport as many rows that have been written successfully.
        if (m_curFileRowCount != 0)
        {
            // Rename the file - following similar strategy as the PS telemetry
            // files generated by tmansvc.

            // InMageTelemetryPSTransport_completed_1506512354.json
            std::string genCompletedFileName =
                m_completedFileNamePrefix + Utils::GetTimeInSecSinceEpoch1970() + FileAttributes::Extension;

            boost_fs::rename(m_currentFilePath, m_folderPath / genCompletedFileName);
        }
    }

    bool CxpsTelemetryLogger::WriteRowToFile(
        std::ofstream &ofstream, CxpsTelemetryRowBase &rowObj, CxpsTelemetryRowBase &cacheObj, bool &isRowObjUnusable)
    {
        bool shouldContinueWritingToFile;
        bool willDataBeDropped;

        isRowObjUnusable = false;

        try
        {
            cacheObj.Clear();

            boost_pt::ptime currSystemTime = boost_pt::microsec_clock::universal_time();

            if (!rowObj.GetPrevWindowRow(cacheObj, currSystemTime))
            {
                // If the failed operation without invalidating the row object, this object could be
                // continued to be used and this data would be uploaded in the next iteration.
                // If the failed operation invalidated the row object, then the object is unusable.
                shouldContinueWritingToFile = true;
                isRowObjUnusable = !rowObj.IsValid();
                willDataBeDropped = isRowObjUnusable;

                goto Cleanup;
            }

            BOOST_ASSERT(rowObj.IsValid() && cacheObj.IsValid());

            // Don't update the global sequence number yet.
            cacheObj.SetSeqNumber(m_seqNum.load(boost::memory_order_acquire) + 1);

            try
            {
                MdsLogUnit::ConvertTelemetryRowToMARSFormat(cacheObj, m_cacheFileLine);

                ofstream << m_cacheFileLine << std::endl;

                // Following disconnected increment using the atomic object, in order to
                // support update of the seqNum, if an object is dropped.
                m_seqNum.fetch_add(1, boost::memory_order_release);
                ++m_curFileRowCount;
                SetLastRowUploadTime(currSystemTime);

                shouldContinueWritingToFile = true;
                isRowObjUnusable = false;
                willDataBeDropped = false;
                goto Cleanup;
            }
            GENERIC_CATCH_LOG_IGNORE("WriteRowToFile - File write");

            // Stop writing to the file anymore for this iteration, since we hit an
            // error while serializing and writing to the file.
            shouldContinueWritingToFile = false;

            // Since there has been a failure in serializing / writing to the file,
            // add the data back to the row object.
            if (rowObj.IsValid() && cacheObj.IsValid())
            {
                if (rowObj.AddBackPrevWindow(cacheObj))
                {
                    BOOST_ASSERT(rowObj.IsValid() && cacheObj.IsValid());

                    // The data has been successfully added back to the row object.
                    // That would be uploaded in the next iteration.
                    isRowObjUnusable = false;
                    willDataBeDropped = false;
                }
                else
                {
                    // Failed to add the data back to the row.

                    // On failure to addBack(), if the row obj is valid, we should continue using it,
                    // since new data could've been added to this object after getPrev() in this stack.
                    isRowObjUnusable = !rowObj.IsValid();
                    // Even if the row object is valid, we will end up missing the data in cacheObj.
                    // (unlike getPrev() failure in the beginning of this method)
                    willDataBeDropped = true;

                    // Special case: The old data couldn't be added back to the row, even if it's
                    // valid with the new data.
                    // So, the object would only have data starting from the current time.
                    if (rowObj.IsValid())
                        SetLastRowUploadTime(currSystemTime);
                }

                goto Cleanup;
            }

            // Not expected to reach this point. But since unhandled, assume the worst.
            BOOST_ASSERT(false);
            isRowObjUnusable = true;
            willDataBeDropped = true;
            goto Cleanup;
        }
        // Unexpected catch. Assume the worst.
        GENERIC_CATCH_LOG_ACTION("WriteRowToFile", BOOST_ASSERT(false); isRowObjUnusable = true; willDataBeDropped = true; shouldContinueWritingToFile = false);

    Cleanup:
        if (willDataBeDropped)
        {
            // Message type of obj wouldn't be corruped at any cost.
            this->ReportRowObjDrop(rowObj.GetMessageType());
        }

        return shouldContinueWritingToFile;
    }

    void CxpsTelemetryLogger::ReportRowObjDrop(MessageType msgType)
    {
        GlobalCxpsTelemetryRowPtr globalTel = GetValidGlobalTelemetry();

        // Increment the counter in the global telemetry
        if (globalTel)
            globalTel->ReportMessageDrop(msgType);

        LapseSequenceNumber();
    }

#define GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(functionName, ...) {\
    GlobalCxpsTelemetryRowPtr globTelPtr = GetValidGlobalTelemetry(); \
    if (globTelPtr) \
        globTelPtr->functionName(__VA_ARGS__); \
\
    LapseSequenceNumber(); \
    }

    boost_pt::ptime GetLastRowUploadTime()
    {
        return CxpsTelemetryLogger::GetInstance().GetLastRowUploadTime();
    }

    void AddRequestDataToTelemetryLogger(const RequestTelemetryData &reqTelData)
    {
        CxpsTelemetryLogger::GetInstance().AddRequestDataToTelemetryLogger(reqTelData);
    }

    void CxpsTelemetryLogger::AddRequestDataToTelemetryLogger(const RequestTelemetryData &reqTelData)
    {
        try
        {
            if (reqTelData.m_dataError != ReqTelErr_None)
            {
                // No data in here could be trusted.

                // TODO-SanKumar-1711: We could probably trial and error by testing values and see
                // if anything could be salvaged out of this. (For ex, although we called out there's
                // error in telemetry data, host id isn't affected in the current workflow).

                // TODO-SanKumar-1711: Should we add this data to SourceGeneral telemetry, so we get
                // the type of file (or) at least the failing host for easier debugging? (The data
                // representation will get complex).

                GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportRequestTelemetryDataError, reqTelData.m_dataError);
                return;
            }

            std::string parsedHostId;
            std::string parsedDevice;
            std::string parsedFileName;
            RequestTelemetryDataLevel dataLevel = reqTelData.m_dataLevel;
            FileType retrievedFileType = FileType_Invalid;

            if (dataLevel == ReqTelLevel_File)
            {
                if (reqTelData.m_filePath.empty())
                {
                    BOOST_ASSERT(false);
                    retrievedFileType = FileType_Invalid;
                    dataLevel = ReqTelLevel_Session; // Fallback to session level logging
                }
                else
                {
                    // Trusting the parsed host id instead of the session advertised host id,
                    // while uploading source file telemetries.
                    if (CS_MODE_RCM == GetCSMode())
                    {
                        retrievedFileType = SourceFilePathParser::GetCxpsFileTypeInRcmMode(
                            reqTelData.m_filePath, parsedHostId, parsedDevice, parsedFileName);
                    }
                    else
                    {
                        retrievedFileType = SourceFilePathParser::GetCxpsFileType(
                            reqTelData.m_filePath, parsedHostId, parsedDevice, parsedFileName);
                    }

                    switch (retrievedFileType)
                    {
                    case FileType_DiffSync:
                    {
                        if (parsedHostId.empty() || parsedDevice.empty() || parsedFileName.empty())
                        {
                            BOOST_ASSERT(false);
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_DiffEmptyMeta);
                            return;
                        }

                        BOOST_ASSERT(parsedHostId == reqTelData.m_hostId);

                        DiffSyncFileMetadata diffFileMetadata;
                        SourceFilePathParser::ParseDiffSyncFileName(parsedFileName, diffFileMetadata);

                        SourceDiffTelemetryRowPtr sourceDiffRow =
                            m_hostTelemetryMap.GetValidSourceDiffTelemetry(parsedHostId, parsedDevice);
                        if (!sourceDiffRow)
                        {
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_Alloc);
                            return;
                        }

                        if (!sourceDiffRow->AddToTelemetry(reqTelData.m_requestType, reqTelData, diffFileMetadata))
                        {
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddToDiffTel);
                            return;
                        }

                        return; // Successfully added the telemetry
                    }
                    case FileType_Resync:
                    {
                        if (parsedHostId.empty() || parsedDevice.empty() || parsedFileName.empty())
                        {
                            BOOST_ASSERT(false);
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_ResyncEmptyMeta);
                            return;
                        }

                        BOOST_ASSERT(parsedHostId == reqTelData.m_hostId);

                        ResyncFileMetadata resyncFileMetadata;
                        SourceFilePathParser::ParseResyncFileName(parsedFileName, resyncFileMetadata);

                        SourceResyncTelemetryRowPtr sourceResyncRow =
                            m_hostTelemetryMap.GetValidSourceResyncTelemetry(parsedHostId, parsedDevice);
                        if (!sourceResyncRow)
                        {
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_Alloc);
                            return;
                        }

                        if (!sourceResyncRow->AddToTelemetry(reqTelData.m_requestType, reqTelData, resyncFileMetadata))
                        {
                            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddToResyncTel);
                            return;
                        }

                        return; // Successfully added the telemetry
                    }
                    case FileType_Log:
                    case FileType_Telemetry:
                    case FileType_ChurStat:
                    case FileType_ThrpStat:
                    case FileType_TstData:
                    case FileType_Unknown:
                    case FileType_Invalid:
                    case FileType_InternalErr:
                        dataLevel = ReqTelLevel_Session; // Fallback to session level logging for these types
                        break;

                    default:
                        BOOST_ASSERT(false);
                        GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddTelUnknFType);
                        return;
                    }
                }
            }

            if (dataLevel == ReqTelLevel_Session)
            {
                // Prefer parsedHostId, if empty, use the host id provided at login.
                const std::string &sessionHostId = !parsedHostId.empty() ? parsedHostId : reqTelData.m_hostId;

                if (sessionHostId.empty())
                {
                    BOOST_ASSERT(false);
                    GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_SrcGenEmptyMeta);
                    return;
                }

                SourceGeneralTelemetryRowPtr sourceGenPtr =
                    m_hostTelemetryMap.GetValidSourceGeneralTelemetry(sessionHostId);

                if (!sourceGenPtr)
                {
                    GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_Alloc);
                    return;
                }

                if (!sourceGenPtr->AddToTelemetry(retrievedFileType, reqTelData.m_requestType, reqTelData))
                {
                    GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddToSrcGenTel);
                    return;
                }

                return; // Successfully added the telemetry
            }

            if (dataLevel == ReqTelLevel_Server)
            {
                // Currently, all server level telemetry can come only on failures (like, not logged in).
                BOOST_ASSERT(!reqTelData.m_hasRespondedSuccess && reqTelData.m_requestFailure != RequestFailure_Success);

                GlobalCxpsTelemetryRowPtr globTelPtr = GetValidGlobalTelemetry();
                if (!globTelPtr)
                {
                    GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_Alloc);
                    return;
                }

                if (!globTelPtr->AddToTelemetry(reqTelData))
                {
                    GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddToGlobalTel);
                    return;
                }

                return; // Successfully added the telemetry
            }

            BOOST_ASSERT(false); // Shouldn't reach here!
            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddTelReachedEof);
            return;
        }
        GENERIC_CATCH_LOG_ACTION("AddRequestDataToTelemetryLogger",
            GLOBAL_TEL_REPORT_ERR_N_LASPSE_SEQ_NUM(ReportTelemetryFailure, TelFailure_AddToTelUnknErr));
    }
}