#ifndef SOURCE_FILE_PATH_PARSER_H
#define SOURCE_FILE_PATH_PARSER_H

#include "Stringer.h"

namespace CxpsTelemetry
{
    struct DiffSyncFileMetadata
    {
        DiffSyncFileType    FileType;
        bool                IsPre;
        bool                IsCompressed;
        uint64_t            PrevEndTS;
        uint64_t            PrevEndSeq;
        uint32_t            PrevSplitIoSeq;
        uint64_t            CurrTS;
        uint64_t            CurrSeq;
        uint32_t            CurrSplitIoSeq;

        DiffSyncFileMetadata()
        {
            Clear();
        }

        void Clear()
        {
            memset(this, 0, sizeof(*this));
        }
    };

    class DiffSyncFileMetadataRange
    {
    public:
        DiffSyncFileMetadataRange()
        {
            Clear();
        }

        void AddToMetadataRange(const DiffSyncFileMetadata& metadataToAdd)
        {
            if (m_isEmpty)
            {
                m_firstPrevEndTS = metadataToAdd.PrevEndTS;
                m_firstPrevEndSeq = metadataToAdd.PrevEndSeq;
                m_firstPrevSplitIoSeq = metadataToAdd.PrevSplitIoSeq;
            }

            m_lastCurrTS = metadataToAdd.CurrTS;
            m_lastCurrSeq = metadataToAdd.CurrSeq;
            m_lastCurrSplitIoSeq = metadataToAdd.CurrSplitIoSeq;

            m_isEmpty = false;
        }

        bool AddBackPrevWindow(const DiffSyncFileMetadataRange& prevMdRange)
        {
            m_firstPrevEndTS = prevMdRange.m_firstPrevEndTS;
            m_firstPrevEndSeq = prevMdRange.m_firstPrevEndSeq;
            m_firstPrevSplitIoSeq = prevMdRange.m_firstPrevSplitIoSeq;

            if (m_isEmpty)
            {
                m_lastCurrTS = prevMdRange.m_lastCurrTS;
                m_lastCurrSeq = prevMdRange.m_lastCurrSeq;
                m_lastCurrSplitIoSeq = prevMdRange.m_lastCurrSplitIoSeq;
            }

            m_isEmpty = false;

            return true;
        }

        bool GetPrevWindow_CopyPhase1(DiffSyncFileMetadataRange& prevMdRange) const
        {
            prevMdRange.m_firstPrevEndTS = m_firstPrevEndTS;
            prevMdRange.m_firstPrevEndSeq = m_firstPrevEndSeq;
            prevMdRange.m_firstPrevSplitIoSeq = m_firstPrevSplitIoSeq;

            prevMdRange.m_lastCurrTS = m_lastCurrTS;
            prevMdRange.m_lastCurrSeq = m_lastCurrSeq;
            prevMdRange.m_lastCurrSplitIoSeq = m_lastCurrSplitIoSeq;

            prevMdRange.m_isEmpty = false;

            return true;
        }

        bool GetPrevWindow_CopyPhase2(const DiffSyncFileMetadataRange& prevMdRange)
        {
            Clear();
            return true;
        }

        void Clear()
        {
            m_firstPrevEndTS = 0;
            m_firstPrevEndSeq = 0;
            m_firstPrevSplitIoSeq = 0;

            m_lastCurrTS = 0;
            m_lastCurrSeq = 0;
            m_lastCurrSplitIoSeq = 0;

            m_isEmpty = true;
        }

#define TO_MDS(string, member) JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceOpsDiffSync::FileMetadata::string, member)

        void serialize(JSON::Adapter &adapter)
        {
            JSON::Class root(adapter, "DiffSyncFileMetadataRange", false);

            BOOST_ASSERT(!m_isEmpty);

            TO_MDS(FirstPrevEndTS, m_firstPrevEndTS);
            TO_MDS(FirstPrevSeqNum, m_firstPrevEndSeq);
            TO_MDS(FirstSplitIoSeqId, m_firstPrevSplitIoSeq);
            TO_MDS(LastTS, m_lastCurrTS);
            TO_MDS(LastSeqNum, m_lastCurrSeq);
            JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceOpsDiffSync::FileMetadata::LastSplitIoSeqId, m_lastCurrSplitIoSeq);
        }

#undef TO_MDS

    private:
        bool m_isEmpty;

        uint64_t m_firstPrevEndTS, m_lastCurrTS;
        uint64_t m_firstPrevEndSeq, m_lastCurrSeq;
        uint32_t m_firstPrevSplitIoSeq, m_lastCurrSplitIoSeq;
    };

    struct ResyncFileMetadata
    {
        ResyncFileType  FileType;
        std::string     JobId;
        // TODO-SanKumar-1711: Is it necessary to consume this as a uint64_t?
        std::string     TimeStamp;

        ResyncFileMetadata()
        {
            Clear();
        }

        void Clear()
        {
            this->FileType = ResyncFileType_Invalid;
            this->JobId.clear();
            this->TimeStamp.clear();
        }
    };

    class ResyncFileMetadataRange
    {
    public:
        ResyncFileMetadataRange()
        {
            Clear();
        }

        void AddToMetadataRange(const ResyncFileMetadata& metadataToAdd)
        {
            if (m_isEmpty)
            {
                m_firstTimeStamp = metadataToAdd.TimeStamp;
                m_firstJobId = metadataToAdd.JobId;
            }

            m_lastTimeStamp = metadataToAdd.TimeStamp;
            m_lastJobId = metadataToAdd.JobId;

            m_isEmpty = false;
        }

        bool AddBackPrevWindow(const ResyncFileMetadataRange& prevMdRange)
        {
            m_firstTimeStamp = prevMdRange.m_firstTimeStamp;
            m_firstJobId = prevMdRange.m_firstJobId;

            if (m_isEmpty)
            {
                m_lastTimeStamp = prevMdRange.m_lastTimeStamp;
                m_lastJobId = prevMdRange.m_lastJobId;
            }

            m_isEmpty = false;

            return true;
        }

        bool GetPrevWindow_CopyPhase1(ResyncFileMetadataRange& prevMdRange) const
        {
            prevMdRange.m_firstJobId = m_firstJobId;
            prevMdRange.m_lastJobId = m_lastJobId;
            prevMdRange.m_firstTimeStamp = m_firstTimeStamp;
            prevMdRange.m_lastTimeStamp = m_lastTimeStamp;

            prevMdRange.m_isEmpty = false;

            return true;
        }

        bool GetPrevWindow_CopyPhase2(const ResyncFileMetadataRange& prevMdRange)
        {
            Clear();
            return true;
        }

        void Clear()
        {
            m_firstJobId.clear();
            m_lastJobId.clear();
            m_firstTimeStamp.clear();
            m_lastTimeStamp.clear();
            m_isEmpty = true;
        }

#define TO_MDS(string, member) JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceOpsResync::FileMetadata::string, member)

        void serialize(JSON::Adapter &adapter)
        {
            JSON::Class root(adapter, "ResyncFileMetadataRange", false);

            if (!m_firstTimeStamp.empty())
                TO_MDS(FirstTS, m_firstTimeStamp);

            TO_MDS(FirstJobId, m_firstJobId);

            if (!m_lastTimeStamp.empty())
                TO_MDS(LastTS, m_lastTimeStamp);

            JSON_T_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::SourceOpsResync::FileMetadata::LastJobId, m_lastJobId);
        }

#undef TO_MDS

    private:
        bool m_isEmpty;

        std::string m_firstTimeStamp, m_lastTimeStamp;
        std::string m_firstJobId, m_lastJobId;
    };

    class SourceFilePathParser
    {
    private:
        SourceFilePathParser() {} // Truly static class

    public:
        static FileType GetCxpsFileType(
            const std::string &filePath, std::string &hostId, std::string &device, std::string &fileName);
        static FileType GetCxpsFileTypeInRcmMode(
            const std::string &filePath, std::string &hostId, std::string &device, std::string &fileName);
        static void ParseResyncFileName(
            const std::string &lowerFileName, ResyncFileMetadata &metadata);
        static void ParseDiffSyncFileName(
            const std::string &lowerFileName, DiffSyncFileMetadata &metadata);
    };
}

#endif // !SOURCE_FILE_PATH_PARSER_H
