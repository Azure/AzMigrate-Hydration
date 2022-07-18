#ifndef FILE_TEL_DATA_H
#define FILE_TEL_DATA_H

#include "RequestTelemetryData.h"

namespace CxpsTelemetry
{
    typedef std::map<RequestFailure, int64_t> FailureMap;

    class FileTelemetryData
    {
    public:
        FileTelemetryData()
            : m_includePerfCounters(false), m_perfCounters(NULL)
        {
            Clear();
        }

        FileTelemetryData(const FileTelemetryData &toCopyObj)
            : m_includePerfCounters(false), m_perfCounters(NULL)
        {
            *this = toCopyObj;
        }

        virtual ~FileTelemetryData()
        {
            BOOST_ASSERT(m_includePerfCounters == (m_perfCounters != NULL));

            if (m_perfCounters != NULL)
            {
                delete m_perfCounters;
                m_perfCounters = NULL;
            }
        }

        void Initialize(bool includePerfCounters)
        {
            BOOST_ASSERT(m_perfCounters == NULL && !m_includePerfCounters);

            if (includePerfCounters)
                m_perfCounters = new CxpsPerfCounters();

            m_includePerfCounters = includePerfCounters;
        }

        FileTelemetryData& operator=(const FileTelemetryData &toCopyObj)
        {
            BOOST_ASSERT(m_includePerfCounters == (m_perfCounters != NULL));

            if (toCopyObj.m_includePerfCounters != m_includePerfCounters)
            {
                if (toCopyObj.m_includePerfCounters)
                {
                    Initialize(true);
                }
                else
                {
                    if (m_perfCounters != NULL)
                    {
                        delete m_perfCounters;
                        m_perfCounters = NULL;
                    }

                    m_includePerfCounters = false;
                }
            }

            // Reusing phase 1 copy, which is nothing but an assignment operation.
            if (!toCopyObj.GetPrevWindow_CopyPhase1(*this))
                throw std::runtime_error("FileTelemetry data assignment failed");

            return *this;
        }

        void Clear()
        {
            // m_includePerfCounters flag and the allocated buffer must be persisted across clears.

            m_totalRequests = 0;
            m_successfulRequests = 0;
            m_failedRequests = 0;

            m_totalNumberOfFiles = 0;
            m_successfulFiles = 0;
            m_failedFiles = 0;

            m_totalNewRequestBlockTime = 0;
            m_totalReqSpecificOpTime = 0;

            m_failureMap.clear();

            if (m_perfCounters != NULL)
                m_perfCounters->Clear();

            m_totalRequestTime = 0;
        }

        bool AddRequestDataToTelemetry(const RequestTelemetryData &reqTelData)
        {
            BOOST_ASSERT(reqTelData.m_dataError == ReqTelErr_None);

            RequestFailure reqFailure = reqTelData.GetRequestFailure();
            RequestType reqType = reqTelData.m_requestType;

            try
            {
                // If the telemetry reports failure, then the last request is the failed one, whereas
                // all others would've succeeded.
                int64_t failedRequests = (reqFailure == RequestFailure_Success) ? 0 : 1;
                m_totalRequests += reqTelData.m_numOfRequests;
                m_successfulRequests += reqTelData.m_numOfRequests - failedRequests;
                m_failedRequests += failedRequests;

                if (reqType == RequestType_PutFile ||
                    reqType == RequestType_ListFile ||
                    reqType == RequestType_DeleteFile ||
                    reqType == RequestType_RenameFile ||
                    reqType == RequestType_GetFile)
                {
                    // Note that list and delete can operate on > 1 file (out of scope, for now)
                    // Always assuming that a request has serviced one file.
                    m_totalNumberOfFiles += 1;
                    m_successfulFiles += 1 - failedRequests;
                    m_failedFiles += failedRequests;
                }

                m_totalNewRequestBlockTime += NANO_TO_TICKS(reqTelData.m_newRequestBlockTime);
                m_totalReqSpecificOpTime += NANO_TO_TICKS(reqTelData.m_totalReqSpecificOpTime);

                if (reqFailure != RequestFailure_Success)
                    ++m_failureMap[reqFailure]; // Creates an entry with value 0, if not present

                if (m_includePerfCounters && m_perfCounters != NULL)
                    m_perfCounters->AddToTelemetry(reqTelData.m_perfCounters);

                m_totalRequestTime += NANO_TO_TICKS(reqTelData.m_requestEndTimePoint - reqTelData.m_requestStartTimePoint);

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("FileTelemetryData::AddRequestDataToTelemetry", return false);
        }

        bool AddBackPrevWindow(const FileTelemetryData &toAddObj)
        {
            try
            {
                m_totalRequests += toAddObj.m_totalRequests;
                m_successfulRequests += toAddObj.m_successfulRequests;
                m_failedRequests += toAddObj.m_failedRequests;

                m_totalNumberOfFiles += toAddObj.m_totalNumberOfFiles;
                m_successfulFiles += toAddObj.m_successfulFiles;
                m_failedFiles += toAddObj.m_failedFiles;

                m_totalNewRequestBlockTime += toAddObj.m_totalNewRequestBlockTime;
                m_totalReqSpecificOpTime += toAddObj.m_totalReqSpecificOpTime;

                for (FailureMap::const_iterator itr = toAddObj.m_failureMap.begin(); itr != toAddObj.m_failureMap.end(); itr++)
                    m_failureMap[itr->first] += itr->second; // If not present, an entry with 0 value would be created.

                if (m_includePerfCounters != toAddObj.m_includePerfCounters ||
                    !m_perfCounters != !toAddObj.m_perfCounters ||
                    !m_perfCounters != !m_includePerfCounters)
                {
                    BOOST_ASSERT(false);
                    throw std::logic_error("FileTelemetryData::AddBack perf counter pointers mismatch");
                }

                if (m_includePerfCounters && m_perfCounters != NULL && toAddObj.m_perfCounters != NULL)
                    m_perfCounters->AddToTelemetry(*toAddObj.m_perfCounters);

                m_totalRequestTime += toAddObj.m_totalRequestTime;

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("FileTelemetryData::AddBackPrevWindow", return false);
        }

        bool GetPrevWindow_CopyPhase1(FileTelemetryData &uptoObj) const
        {
            try
            {
                uptoObj.m_totalRequests = m_totalRequests;
                uptoObj.m_successfulRequests = m_successfulRequests;
                uptoObj.m_failedRequests = m_failedRequests;

                uptoObj.m_totalNumberOfFiles = m_totalNumberOfFiles;
                uptoObj.m_successfulFiles = m_successfulFiles;
                uptoObj.m_failedFiles = m_failedFiles;

                uptoObj.m_totalNewRequestBlockTime = m_totalNewRequestBlockTime;
                uptoObj.m_totalReqSpecificOpTime = m_totalReqSpecificOpTime;

                uptoObj.m_failureMap = m_failureMap;

                if (!m_includePerfCounters != !uptoObj.m_includePerfCounters ||
                    !m_perfCounters != !uptoObj.m_perfCounters ||
                    !m_perfCounters != !m_includePerfCounters)
                {
                    BOOST_ASSERT(false);
                    throw std::logic_error("FileTelemetryData::GetPrevWindow_CopyPhase1 perf counter pointers mismatch");
                }

                if (m_includePerfCounters)
                    if (!m_perfCounters->GetPrevWindow_CopyPhase1(*uptoObj.m_perfCounters))
                        return false;

                uptoObj.m_totalRequestTime = m_totalRequestTime;

                return true;
            }
            GENERIC_CATCH_LOG_ACTION("FileTelemetryData::GetPrevWindow_CopyPhase1", return false);
        }

        bool GetPrevWindow_CopyPhase2(const FileTelemetryData &uptoObj)
        {
            Clear();
            return true;
        }

        bool IsEmpty() const
        {
            return
                m_totalRequests == 0 &&
                m_successfulRequests == 0 &&
                m_failedRequests == 0 &&

                m_totalNumberOfFiles == 0 &&
                m_successfulFiles == 0 &&
                m_failedFiles == 0 &&

                m_totalNewRequestBlockTime == 0 &&
                m_totalReqSpecificOpTime == 0 &&

                // TODO-SanKumar-1711: Should we also check, if there's any element, check if all are 0.
                // Shouldn't be needed, since the assumption is that element would be present, if != 0.
                m_failureMap.empty() &&

                (!m_includePerfCounters || m_perfCounters->IsEmpty()) &&

                m_totalRequestTime == 0;
        }

#define TO_MDS(string, member) if(member != 0) { JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::FileTelemetryData::string, member); }

        void serialize(JSON::Adapter &adapter)
        {
            JSON::Class root(adapter, "FileTelemetryData", false);

            TO_MDS(SuccReqs, m_successfulRequests);
            TO_MDS(FailedReqs, m_failedRequests);

            TO_MDS(TotFiles, m_totalNumberOfFiles);
            TO_MDS(SuccFiles, m_successfulFiles);
            TO_MDS(FailedFiles, m_failedFiles);

            TO_MDS(TotNewReqBlockTime, m_totalNewRequestBlockTime);
            TO_MDS(TotReqOpTime, m_totalReqSpecificOpTime);

            TO_MDS(TotReqTime, m_totalRequestTime);

            if (m_includePerfCounters)
                m_perfCounters->OnContainingObjectSerialized(adapter);

            Utils::NameValueCounters<int64_t> reqFailCounters;
            if (reqFailCounters.Load<RequestFailure>(m_failureMap, Stringer::RequestFailureToString))
                JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::FileTelemetryData::Failures, reqFailCounters);

            JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::FileTelemetryData::TotReqs, m_totalRequests);
        }

#undef TO_MDS

    private:
        int64_t m_totalRequests, m_successfulRequests, m_failedRequests;
        int64_t m_totalNumberOfFiles, m_successfulFiles, m_failedFiles;

        int64_t m_totalNewRequestBlockTime;
        int64_t m_totalReqSpecificOpTime;

        FailureMap m_failureMap;

        bool m_includePerfCounters;
        CxpsPerfCounters *m_perfCounters;

        int64_t m_totalRequestTime;
    };

    template <class FileSubType, class FileMetadataRange>
    class FileTelemetryDataCollection
    {
    public:
        bool Load(
            const std::vector<RequestType> &reqTypes,
            const std::vector<FileType> &fileTypes,
            const std::vector<FileSubType> fileSubTypes,
            boost::function1<const std::string&, FileSubType> subTypeToString,
            const std::vector<FileTelemetryData> &fileTelemetryData,
            const std::vector<FileMetadataRange> &fileMetadataRanges)
        {
            if (reqTypes.empty() && fileTypes.empty() && fileSubTypes.empty())
                throw std::logic_error("FileTelemetryDataCollection empty initialization");

            m_shouldPrintReqTypes = !reqTypes.empty();
            // When detailed split-up is presented on a particular file type, not necessary to print redundant file type - diff & resyc.
            m_shouldPrintFileTypes = !fileTypes.empty();
            // When the file type is insignificant, there wouldn't be any sub types for it - !(diff & resync)
            m_shouldPrintFileSubTypes = !fileSubTypes.empty();
            m_shouldPrintMetadataRanges = !fileMetadataRanges.empty();

            if (!m_shouldPrintFileSubTypes != !subTypeToString)
                throw std::logic_error("FileTelemetryDataCollection check subtype stringer against flag");

            if ((m_shouldPrintReqTypes && reqTypes.size() != fileTelemetryData.size()) ||
                (m_shouldPrintFileTypes && fileTypes.size() != fileTelemetryData.size()) ||
                (m_shouldPrintFileSubTypes && fileSubTypes.size() != fileTelemetryData.size()) ||
                (m_shouldPrintMetadataRanges && fileMetadataRanges.size() != fileTelemetryData.size()))
            {
                throw std::logic_error("FileTelemetryDataCollection failed at size check");
            }

            m_reqTypesStr.clear();
            m_fileTypesStr.clear();
            m_fileSubTypesStr.clear();
            m_telDataToPrint.clear();
            m_metadataRangesToPrint.clear();

            for (size_t ind = 0; ind < fileTelemetryData.size(); ind++)
            {
                if (!fileTelemetryData[ind].IsEmpty())
                {
                    if (m_shouldPrintReqTypes)
                        m_reqTypesStr.push_back(Stringer::RequestTypeToString(reqTypes[ind]));

                    if (m_shouldPrintFileTypes)
                        m_fileTypesStr.push_back(Stringer::SourceFileTypeToString(fileTypes[ind]));

                    if (m_shouldPrintFileSubTypes)
                        m_fileSubTypesStr.push_back(subTypeToString(fileSubTypes[ind]));

                    if (m_shouldPrintMetadataRanges)
                        m_metadataRangesToPrint.push_back(fileMetadataRanges[ind]);

                    m_telDataToPrint.push_back(fileTelemetryData[ind]);
                }
            }

            return !m_telDataToPrint.empty();
        }

        void serialize(JSON::Adapter &adapter)
        {
            JSON::Class root(adapter, "FileTelemetryDataCollection", false);

            namespace Ftdc = Strings::DynamicJson::FileTelemetryDataCollection;

            if (m_shouldPrintReqTypes)
                JSON_E_KV_PRODUCER_ONLY(adapter, Ftdc::ReqType, m_reqTypesStr);

            if (m_shouldPrintFileTypes)
                JSON_E_KV_PRODUCER_ONLY(adapter, Ftdc::FileType, m_fileTypesStr);

            if (m_shouldPrintFileSubTypes)
                JSON_E_KV_PRODUCER_ONLY(adapter, Ftdc::FSubType, m_fileSubTypesStr);

            if (m_shouldPrintMetadataRanges)
                JSON_E_KV_PRODUCER_ONLY(adapter, Ftdc::MdRange, m_metadataRangesToPrint);

            JSON_T_KV_PRODUCER_ONLY(adapter, Ftdc::Telemetry, m_telDataToPrint);
        }

    private:
        bool m_shouldPrintReqTypes, m_shouldPrintFileTypes, m_shouldPrintFileSubTypes, m_shouldPrintMetadataRanges;

        std::vector<std::string> m_reqTypesStr, m_fileTypesStr, m_fileSubTypesStr;
        std::vector<FileTelemetryData> m_telDataToPrint;
        std::vector<FileMetadataRange> m_metadataRangesToPrint;
    };
}

#endif // !FILE_TEL_DATA_H
