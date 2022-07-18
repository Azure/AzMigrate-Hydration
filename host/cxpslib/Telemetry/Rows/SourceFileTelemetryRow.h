#ifndef SRC_FILE_TEL_ROW_H
#define SRC_FILE_TEL_ROW_H

#include "CxpsTelemetryRowBase.h"

namespace CxpsTelemetry
{
    void SourceFileTelemetryRow_GenerateTelDataMapping(
        MessageType msgType, boost::mutex &lock, boost::atomic<bool> &isMappingInitialized);

    template<
        MessageType MsgType,
        class FileMetadataType,
        class FileMetadataRange,
        class FileSubType,
        size_t FileSubTypeCount,
        size_t PerfCountedSubTypesCnt,
        size_t DetailedRequestTypesCnt>

        class SourceFileTelemetryRow : public CxpsTelemetryRowBase
    {
    public:
        SourceFileTelemetryRow()
            : CxpsTelemetryRowBase(MsgType), m_fileMetadataRanges(TelDataCount), m_telData(TelDataCount)
        {
            if (!s_isMappingInitialized)
                SourceFileTelemetryRow_GenerateTelDataMapping(MsgType, s_initLock, s_isMappingInitialized);

            Initialize();
        }

        SourceFileTelemetryRow(boost_pt::ptime loggerStartTime, const std::string &hostId, const std::string &device)
            : CxpsTelemetryRowBase(MsgType, loggerStartTime, hostId, device),
            m_fileMetadataRanges(TelDataCount),
            m_telData(TelDataCount)
        {
            if (!s_isMappingInitialized)
                SourceFileTelemetryRow_GenerateTelDataMapping(MsgType, s_initLock, s_isMappingInitialized);

            Initialize();
        }

        virtual ~SourceFileTelemetryRow()
        {
            BOOST_ASSERT(IsCacheObj() || IsEmpty());
        }

        static void GenerateTelDataMapping(
            boost::function1<const std::string&, FileSubType> fileSubTypeToString,
            const RequestType(&detailedRequestTypes)[DetailedRequestTypesCnt],
            const FileSubType(&perfCountedSubTypes)[PerfCountedSubTypesCnt],
            FileSubType fileSubTypeAll
        )
        {
            s_fileSubTypeToString = fileSubTypeToString;

            s_isPerfCountedSubFileTypes.clear();
            for (size_t fileSubTypeInd = 0; fileSubTypeInd < FileSubTypeCount; fileSubTypeInd++)
            {
                s_isPerfCountedSubFileTypes.push_back(
                    std::find(perfCountedSubTypes, perfCountedSubTypes + PerfCountedSubTypesCnt, fileSubTypeInd)
                    != (perfCountedSubTypes + PerfCountedSubTypesCnt));
            }

            s_requestTypes.clear();
            s_fileSubTypes.clear();
            s_requestTypes.reserve(TelDataCount);
            s_fileSubTypes.reserve(TelDataCount);

            size_t telInd = 0;

            for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
            {
                s_isDetailedRequest[reqInd] =
                    std::find(detailedRequestTypes, detailedRequestTypes + DetailedRequestTypesCnt, reqInd)
                    != (detailedRequestTypes + DetailedRequestTypesCnt);

                s_telDataIndices[reqInd] = telInd;

                if (s_isDetailedRequest[reqInd])
                {
                    s_requestTypes.insert(s_requestTypes.end(), FileSubTypeCount, static_cast<RequestType>(reqInd));
                    telInd += FileSubTypeCount;

                    for (size_t fileSubTypeInd = 0; fileSubTypeInd < FileSubTypeCount; fileSubTypeInd++)
                        s_fileSubTypes.push_back(static_cast<FileSubType>(fileSubTypeInd));
                }
                else
                {
                    s_requestTypes.push_back(static_cast<RequestType>(reqInd));
                    telInd++;
                    s_fileSubTypes.push_back(static_cast<FileSubType>(fileSubTypeAll));
                }
            }

            BOOST_ASSERT(s_requestTypes.size() == TelDataCount);
            BOOST_ASSERT(s_fileSubTypes.size() == TelDataCount);
            BOOST_ASSERT(telInd = TelDataCount);
        }

        virtual bool IsEmpty() const
        {
            RecursiveLockGuard guard(m_rowObjLock);

            for (size_t telInd = 0; telInd < TelDataCount; telInd++)
                if (!m_telData[telInd].IsEmpty())
                    return false;

            return true;
        }

        bool AddToTelemetry(RequestType reqType, const RequestTelemetryData &reqTelData, const FileMetadataType &fileMetadata)
        {
            RecursiveLockGuard guard(m_rowObjLock);

            if (!CxpsTelemetryRowBase::AddToTelemetry(std::string(), reqTelData))
                return false;

            MarkObjForModification();

            try
            {
                size_t telInd = s_telDataIndices[reqType];
                if (s_isDetailedRequest[reqType])
                    telInd += fileMetadata.FileType;

                if (!m_telData[telInd].AddRequestDataToTelemetry(reqTelData))
                    return false;

                // TODO-SanKumar-1710: Cover this rather with try..catch
                m_fileMetadataRanges[telInd].AddToMetadataRange(fileMetadata);

                MarkModCompletionForObj();
                return true;
            }
            GENERIC_CATCH_LOG_ACTION("SourceFileTelemetryRow::AddToTelemetry", return false);
        }

        class CustomData
        {
        public:
            CustomData(const SourceFileTelemetryRow &objToSerialize)
                : m_objToSerialize(objToSerialize)
            {
            }

            void serialize(JSON::Adapter &adapter)
            {
                JSON::Class root(adapter, "CustomData", false);

                std::vector<FileType> fileTypesDummyList; // Since these objects are for sub-types of same file types.
                FileTelemetryDataCollection<FileSubType, FileMetadataRange> fileTelDataColl;
                if (fileTelDataColl.Load(
                    s_requestTypes, fileTypesDummyList, s_fileSubTypes, s_fileSubTypeToString,
                    m_objToSerialize.m_telData, m_objToSerialize.m_fileMetadataRanges))
                {
                    JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::Data, fileTelDataColl);
                }

                JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::Pid, s_psProcessId);
            }

        private:
            const SourceFileTelemetryRow &m_objToSerialize;
        };

    protected:
        virtual void OnClear()
        {
            ClearSelfMembers();
        }

        virtual bool OnGetPrevWindow_CopyPhase1(CxpsTelemetryRowBase &prevWindowRow) const
        {
            SourceFileTelemetryRow &prevSourceFileRow = dynamic_cast<SourceFileTelemetryRow&>(prevWindowRow);

            for (size_t ind = 0; ind < TelDataCount; ind++)
            {
                if (!m_telData[ind].GetPrevWindow_CopyPhase1(prevSourceFileRow.m_telData[ind]))
                    return false;

                if (!m_fileMetadataRanges[ind].GetPrevWindow_CopyPhase1(prevSourceFileRow.m_fileMetadataRanges[ind]))
                    return false;
            }

            return true;
        }

        virtual bool OnGetPrevWindow_CopyPhase2(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const SourceFileTelemetryRow &prevSourceFileRow = dynamic_cast<const SourceFileTelemetryRow&>(prevWindowRow);

            for (size_t ind = 0; ind < TelDataCount; ind++)
            {
                if (!m_telData[ind].GetPrevWindow_CopyPhase2(prevSourceFileRow.m_telData[ind]))
                    return false;

                if (!m_fileMetadataRanges[ind].GetPrevWindow_CopyPhase2(prevSourceFileRow.m_fileMetadataRanges[ind]))
                    return false;
            }

            return true;
        }

        virtual bool OnAddBackPrevWindow(const CxpsTelemetryRowBase &prevWindowRow)
        {
            const SourceFileTelemetryRow &prevSourceFileRow = dynamic_cast<const SourceFileTelemetryRow&>(prevWindowRow);

            for (size_t ind = 0; ind < TelDataCount; ind++)
            {
                if (!m_telData[ind].AddBackPrevWindow(prevSourceFileRow.m_telData[ind]))
                    return false;

                if (!m_fileMetadataRanges[ind].AddBackPrevWindow(prevSourceFileRow.m_fileMetadataRanges[ind]))
                    return false;
            }

            return true;
        }

        virtual void RetrieveDynamicJsonData(std::string &dynamicJsonData)
        {
            CustomData customData(*this);
            dynamicJsonData = JSON::producer<CustomData>::convert(customData);
        }

    private:
        // Expanded per diff type for Put request and collective for other requests.
        static const size_t TelDataCount = DetailedRequestTypesCnt * FileSubTypeCount + RequestType_Count - DetailedRequestTypesCnt;
        static size_t s_telDataIndices[RequestType_Count];
        static bool s_isDetailedRequest[RequestType_Count];

        static boost::mutex s_initLock;
        static boost::atomic<bool> s_isMappingInitialized;

        static boost::function1<const std::string&, FileSubType> s_fileSubTypeToString;

        static std::vector<RequestType> s_requestTypes;
        static std::vector<FileSubType> s_fileSubTypes;
        static std::vector<bool> s_isPerfCountedSubFileTypes;

        std::vector<FileMetadataRange> m_fileMetadataRanges;
        std::vector<FileTelemetryData> m_telData;

        void Initialize()
        {
            size_t telInd = 0;

            for (size_t reqInd = 0; reqInd < RequestType_Count; reqInd++)
            {
                if (s_isDetailedRequest[reqInd])
                {
                    for (size_t subTypeInd = 0; subTypeInd < FileSubTypeCount; subTypeInd++)
                        m_telData[telInd++].Initialize(s_isPerfCountedSubFileTypes[subTypeInd]);
                }
                else
                {
                    m_telData[telInd++].Initialize(false);
                }
            }

            BOOST_ASSERT(telInd == TelDataCount);
        }

        void ClearSelfMembers()
        {
            for (size_t ind = 0; ind < TelDataCount; ind++)
            {
                m_telData[ind].Clear();
                m_fileMetadataRanges[ind].Clear();
            }
        }
    };

    class SourceDiffTelemetryRow
        : public SourceFileTelemetryRow<MsgType_SourceOpsDiffSync,
        DiffSyncFileMetadata, DiffSyncFileMetadataRange, DiffSyncFileType, DiffSyncFileType_Count,
        Tunables::PerfCountedDiffTypesCount, Tunables::DetailedDiffRequestTypesCount>
    {
    public:
        SourceDiffTelemetryRow() {}

        SourceDiffTelemetryRow(boost_pt::ptime loggerStartTime, const std::string &hostId, const std::string &device)
            : SourceFileTelemetryRow<MsgType_SourceOpsDiffSync,
            DiffSyncFileMetadata, DiffSyncFileMetadataRange, DiffSyncFileType, DiffSyncFileType_Count,
            Tunables::PerfCountedDiffTypesCount, Tunables::DetailedDiffRequestTypesCount>(loggerStartTime, hostId, device)
        {
        }
    };

    typedef boost::shared_ptr<SourceDiffTelemetryRow> SourceDiffTelemetryRowPtr;

    class SourceResyncTelemetryRow
        : public SourceFileTelemetryRow<MsgType_SourceOpsResync,
        ResyncFileMetadata, ResyncFileMetadataRange, ResyncFileType, ResyncFileType_Count,
        Tunables::PerfCountedResyncTypesCount, Tunables::DetailedResyncRequestTypesCount>
    {
    public:
        SourceResyncTelemetryRow() {}

        SourceResyncTelemetryRow(boost_pt::ptime loggerStartTime, const std::string &hostId, const std::string &device)
            : SourceFileTelemetryRow<MsgType_SourceOpsResync,
            ResyncFileMetadata, ResyncFileMetadataRange, ResyncFileType, ResyncFileType_Count,
            Tunables::PerfCountedResyncTypesCount, Tunables::DetailedResyncRequestTypesCount>(loggerStartTime, hostId, device)
        {
        }
    };

    typedef boost::shared_ptr<SourceResyncTelemetryRow> SourceResyncTelemetryRowPtr;

    // Static member definitions
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    boost::mutex SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_initLock;
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    boost::atomic<bool> SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_isMappingInitialized(false);
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    size_t SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_telDataIndices[RequestType_Count];
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    bool SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_isDetailedRequest[RequestType_Count];
    template <MessageType M, class T1, class T2, class FileSubType, size_t N1, size_t N2, size_t N3>
    boost::function1<const std::string&, FileSubType> SourceFileTelemetryRow<M, T1, T2, FileSubType, N1, N2, N3>::s_fileSubTypeToString;
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    std::vector<RequestType> SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_requestTypes;
    template <MessageType M, class T1, class T2, class FileSubType, size_t N1, size_t N2, size_t N3>
    std::vector<FileSubType> SourceFileTelemetryRow<M, T1, T2, FileSubType, N1, N2, N3>::s_fileSubTypes;
    template <MessageType M, class T1, class T2, class T3, size_t N1, size_t N2, size_t N3>
    std::vector<bool> SourceFileTelemetryRow<M, T1, T2, T3, N1, N2, N3>::s_isPerfCountedSubFileTypes;
}

#endif // !SRC_FILE_TEL_ROW_H
