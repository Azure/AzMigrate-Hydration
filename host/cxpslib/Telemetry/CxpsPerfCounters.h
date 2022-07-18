#ifndef CXPS_PERF_COUNTERS_H
#define CXPS_PERF_COUNTERS_H

#include "Stringer.h"
#include "inmsafecapis.h"

namespace CxpsTelemetry
{
    struct CxpsPerfCounters
    {
        int64_t m_totalFileWriteTime, m_totalFileReadTime;
        int64_t m_totalNwReadTime, m_totalNwWriteTime;

        int64_t m_totalFileFlushTime, m_totalFileCompressTime;

        int64_t m_totalFileWriteCnt, m_totalFileReadCnt;
        int64_t m_totalNwReadCnt, m_totalNwWriteCnt;

        int64_t m_succFileWriteCnt, m_succFileReadCnt;
        int64_t m_succNwReadCnt, m_succNwWriteCnt;

        int64_t m_failedFileWriteCnt, m_failedFileReadCnt;
        int64_t m_failedNwReadCnt, m_failedNwWriteCnt;

        int64_t m_totalNwReadBytes, m_totalNwWrittenBytes;
        int64_t m_totalFileReadBytes, m_totalFileWrittenBytes;

        int64_t m_totalPutReqInterRequestTime;

        int64_t m_nwReadBuckets[Tunables::NwReadLatencyBucketsCount];
        int64_t m_fWriteBuckets[Tunables::FWriteLatencyBucketsCount];

        CxpsPerfCounters()
        {
            Clear();
        }

        void AddToTelemetry(const CxpsPerfCounters& perfCounters)
        {
            // No need of special implementation for this, since all of them are
            // addition of time elapsed or counters.
            AddBackPrevWindow(perfCounters);
        }

        bool AddBackPrevWindow(const CxpsPerfCounters &addBackObj)
        {
            // Note: Shouldn't do a simple assign back but add, since more data
            // could've been added from the time this data was retrieved.

            m_totalFileWriteTime += addBackObj.m_totalFileWriteTime;
            m_totalFileReadTime += addBackObj.m_totalFileReadTime;
            m_totalNwReadTime += addBackObj.m_totalNwReadTime;
            m_totalNwWriteTime += addBackObj.m_totalNwWriteTime;

            m_totalFileFlushTime += addBackObj.m_totalFileFlushTime;
            m_totalFileCompressTime += addBackObj.m_totalFileCompressTime;

            m_totalFileWriteCnt += addBackObj.m_totalFileWriteCnt;
            m_totalFileReadCnt += addBackObj.m_totalFileReadCnt;
            m_totalNwReadCnt += addBackObj.m_totalNwReadCnt;
            m_totalNwWriteCnt += addBackObj.m_totalNwWriteCnt;

            m_succFileWriteCnt += addBackObj.m_succFileWriteCnt;
            m_succFileReadCnt += addBackObj.m_succFileReadCnt;
            m_succNwReadCnt += addBackObj.m_succNwReadCnt;
            m_succNwWriteCnt += addBackObj.m_succNwWriteCnt;

            m_failedFileWriteCnt += addBackObj.m_failedFileWriteCnt;
            m_failedFileReadCnt += addBackObj.m_failedFileReadCnt;
            m_failedNwReadCnt += addBackObj.m_failedNwReadCnt;
            m_failedNwWriteCnt += addBackObj.m_failedNwWriteCnt;

            m_totalNwReadBytes += addBackObj.m_totalNwReadBytes;
            m_totalNwWrittenBytes += addBackObj.m_totalNwWrittenBytes;
            m_totalFileReadBytes += addBackObj.m_totalFileReadBytes;
            m_totalFileWrittenBytes += addBackObj.m_totalFileWrittenBytes;

            m_totalPutReqInterRequestTime += addBackObj.m_totalPutReqInterRequestTime;

            for (size_t ind = 0; ind < Tunables::NwReadLatencyBucketsCount; ind++)
                m_nwReadBuckets[ind] += addBackObj.m_nwReadBuckets[ind];

            for (size_t ind = 0; ind < Tunables::FWriteLatencyBucketsCount; ind++)
                m_fWriteBuckets[ind] += addBackObj.m_fWriteBuckets[ind];

            return true;
        }

        bool GetPrevWindow_CopyPhase1(CxpsPerfCounters &prevWindowElement) const
        {
            inm_memcpy_s(&prevWindowElement, sizeof(prevWindowElement), this, sizeof(*this));
            return true;
        }

        // Not implementing Phase2, since it's understood that it is nothing but Clear().

        void Clear()
        {
            memset(this, 0, sizeof(*this));
        }

        bool IsEmpty() const
        {
            // Note: It might be enough to check only if {file, nw} x {R,W} counts == 0,
            // but this check would help us find errors in the telemetry code, when
            // an incorrect data shows up and could be caught by asserting against counts.
            return
                m_totalFileWriteTime == 0 &&
                m_totalFileReadTime == 0 &&
                m_totalNwReadTime == 0 &&
                m_totalNwWriteTime == 0 &&

                m_totalFileFlushTime == 0 &&
                m_totalFileCompressTime == 0 &&

                m_totalFileWriteCnt == 0 &&
                m_totalFileReadCnt == 0 &&
                m_totalNwReadCnt == 0 &&
                m_totalNwWriteCnt == 0 &&

                m_succFileWriteCnt == 0 &&
                m_succFileReadCnt == 0 &&
                m_succNwReadCnt == 0 &&
                m_succNwWriteCnt == 0 &&

                m_failedFileWriteCnt == 0 &&
                m_failedFileReadCnt == 0 &&
                m_failedNwReadCnt == 0 &&
                m_failedNwWriteCnt == 0 &&

                m_totalNwReadBytes == 0 &&
                m_totalNwWrittenBytes == 0 &&
                m_totalFileReadBytes == 0 &&
                m_totalFileWrittenBytes == 0 &&

                m_totalPutReqInterRequestTime == 0;

            // TODO-SanKumar-1711: Check
            // all m_nwReadBuckets[Tunables::NwReadLatencyBucketsCount] == 0
            // all m_fWriteBuckets[Tunables::FWriteLatencyBucketsCount] == 0
        }

#define TO_MDS(string, member) if(member != 0) { JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::PerfCounters::string, member); }

        void OnContainingObjectSerialized(JSON::Adapter &adapter)
        {
            TO_MDS(TotFWriteTime, m_totalFileWriteTime);
            TO_MDS(TotFReadTime, m_totalFileReadTime);
            TO_MDS(TotNwReadTime, m_totalNwReadTime);
            TO_MDS(TotNwWriteTime, m_totalNwWriteTime);

            TO_MDS(TotFFlushTime, m_totalFileFlushTime);
            TO_MDS(TotFComprTime, m_totalFileCompressTime);

            TO_MDS(TotFWriteCnt, m_totalFileWriteCnt);
            TO_MDS(TotFReadCnt, m_totalFileReadCnt);
            TO_MDS(TotNwReadCnt, m_totalNwReadCnt);
            TO_MDS(TotNwWriteCnt, m_totalNwWriteCnt);

            TO_MDS(SuccFWriteCnt, m_succFileWriteCnt);
            TO_MDS(SuccFReadCnt, m_succFileReadCnt);
            TO_MDS(SuccNwReadCnt, m_succNwReadCnt);
            TO_MDS(SuccNwWriteCnt, m_succNwWriteCnt);

            TO_MDS(FailedFWriteCnt, m_failedFileWriteCnt);
            TO_MDS(FailedFReadCnt, m_failedFileReadCnt);
            TO_MDS(FailedNwReadCnt, m_failedNwReadCnt);
            TO_MDS(FailedNwWriteCnt, m_failedNwWriteCnt);

            TO_MDS(TotNwReadBytes, m_totalNwReadBytes);
            TO_MDS(TotNwWriteBytes, m_totalNwWrittenBytes);
            TO_MDS(TotFReadBytes, m_totalFileReadBytes);
            TO_MDS(TotFWriteBytes, m_totalFileWrittenBytes);

            TO_MDS(TotPutInterReqTime, m_totalPutReqInterRequestTime);

            Utils::StartEndCountBuckets<int64_t, int64_t> latBuckets;

            if (latBuckets.Load(Tunables::NwReadLatencyBuckets_ms, m_nwReadBuckets))
                JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::PerfCounters::NwReadBuckets, latBuckets);

            if (latBuckets.Load(Tunables::FWriteLatencyBuckets_ms, m_fWriteBuckets))
                JSON_E_KV_PRODUCER_ONLY(adapter, Strings::DynamicJson::PerfCounters::FWriteBuckets, latBuckets);
        }

#undef TO_MDS
    };
}

#endif // !CXPS_PERF_COUNTERS_H
