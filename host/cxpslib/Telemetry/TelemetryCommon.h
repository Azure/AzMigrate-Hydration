#ifndef CXPS_TELEMETRY_COMMON_H
#define CXPS_TELEMETRY_COMMON_H

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <json_writer.h>
#include <version.h>

#include "cxpslogger.h"
#include "TelemetrySharedParams.h"

// 1 tick = 100 ns
#define NANO_TO_TICKS(durationNs) ((durationNs).count() / 100)

#define GENERIC_CATCH_LOG_ACTION(Operation, action) \
    catch (const std::exception &ex) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << ex.what()); \
        action; \
    } \
    catch (const std::string &thrownStr) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << thrownStr); \
        action; \
    } \
    catch (const char *thrownStrLiteral) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << thrownStrLiteral); \
        action; \
    } \
    catch (...) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation ". Hit an unknown exception."); \
        action; \
    }

#define GENERIC_CATCH_LOG_IGNORE(Operation) \
    catch (const std::exception &ex) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << ex.what()); \
    } \
    catch (const std::string &thrownStr) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << thrownStr); \
    } \
    catch (const char *thrownStrLiteral) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation "." << thrownStrLiteral); \
    } \
    catch (...) \
    { \
        CXPS_LOG_ERROR(CATCH_LOC << "FAILED: " Operation ". Hit an unknown exception."); \
    }

namespace CxpsTelemetry
{
    namespace Tunables
    {
        const int64_t NwReadLatencyBuckets_ms[] = { 0, 40, 70, 125, 250, 500, 1000, 3000 };
        const size_t NwReadLatencyBucketsCount = sizeof(NwReadLatencyBuckets_ms) / sizeof(NwReadLatencyBuckets_ms[0]);

        const int64_t FWriteLatencyBuckets_ms[] = { 0, 5, 10, 20, 50, 100, 500 };
        const size_t FWriteLatencyBucketsCount = sizeof(FWriteLatencyBuckets_ms) / sizeof(FWriteLatencyBuckets_ms[0]);

        const RequestType DetailedDiffRequestTypes[] = { RequestType_PutFile };
        const size_t DetailedDiffRequestTypesCount = sizeof(DetailedDiffRequestTypes) / sizeof(RequestType);

        const DiffSyncFileType PerfCountedDiffTypes[]
            = { DiffSyncFileType_WriteOrdered, DiffSyncFileType_NonWriteOrdered, DiffSyncFileType_Tag, DiffSyncFileType_Tso };
        const size_t PerfCountedDiffTypesCount = sizeof(PerfCountedDiffTypes) / sizeof(DiffSyncFileType);

        const RequestType DetailedResyncRequestTypes[] = { RequestType_PutFile, RequestType_GetFile };
        const size_t DetailedResyncRequestTypesCount = sizeof(DetailedResyncRequestTypes) / sizeof(RequestType);

        const ResyncFileType PerfCountedResyncTypes[]
            = { ResyncFileType_Hcd, ResyncFileType_Sync, ResyncFileType_Map, ResyncFileType_Cluster };
        const size_t PerfCountedResyncTypesCount = sizeof(PerfCountedResyncTypes) / sizeof(ResyncFileType);

        // TODO-SanKumar-1711: Rather list down untracked file types.
        const FileType SourceGenTelemetryTrackedFileTypes[] // All types except diff and resync
            = { FileType_Invalid, FileType_Log, FileType_Telemetry, FileType_ChurStat, FileType_ThrpStat, FileType_TstData, FileType_InternalErr, FileType_Unknown };
        const size_t SourceGenTelTrackedFileTypesCount = sizeof(SourceGenTelemetryTrackedFileTypes) / sizeof(FileType);
    }

#define STR(name) static const std::string name(#name)

    namespace Strings
    {
        namespace DynamicJson
        {
            namespace Counters
            {
                STR(Name);
                STR(Value);
            }

            namespace Buckets
            {
                STR(Start);
                STR(End);
                STR(Count);
            }
        }
    }

    class Utils
    {
    private:
        Utils() {} // Purely static class

    public:
        // Copy of method from portablehelper.cpp to avoid additional linking.
        static std::string GetTimeInSecSinceEpoch1970()
        {
            // time_duration is 32-bit, so in 2038 this will overflow
            uint64_t secSinceEpoch = boost::posix_time::time_duration(
                boost::posix_time::second_clock::universal_time()
                - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_seconds();

            return boost::lexical_cast<std::string>(secSinceEpoch);
        }

        // Copy of method from inmageproduct.cpp to avoid additional linking.
        static void GetProductVersion(std::string &productVersion)
        {
            try
            {
                std::stringstream   ssProdVersion;
                uint32_t vProdVersion[] = { INMAGE_PRODUCT_VERSION };
                ssProdVersion << vProdVersion[0] << "." << vProdVersion[1] << ".";
                ssProdVersion << vProdVersion[2] << "." << vProdVersion[3];

                productVersion = ssProdVersion.str();
            }
            GENERIC_CATCH_LOG_IGNORE("GetProductVersion");
        }

        template <class ValueType>
        class NameValueCounters
        {
        public:
            template <class CounterType, size_t ArrSize>
            bool Load(
                const ValueType(&countArr)[ArrSize],
                boost::function1<const std::string&, CounterType> counterNameFinder)
            {
                m_counterNames.clear();
                m_counters.clear();

                for (size_t ind = 0; ind < ArrSize; ind++)
                {
                    if (countArr[ind] != 0)
                    {
                        m_counterNames.push_back(counterNameFinder(static_cast<CounterType>(ind)));
                        m_counters.push_back(countArr[ind]);
                    }
                }

                return !m_counters.empty();
            }

            template <class CounterType>
            bool Load(
                const std::map<CounterType, ValueType> &counterMap,
                boost::function1<const std::string&, CounterType> counterNameFinder)
            {
                m_counterNames.clear();
                m_counters.clear();

                for (typename std::map<CounterType, ValueType>::const_iterator itr = counterMap.begin(); itr != counterMap.end(); itr++)
                {
                    BOOST_ASSERT(itr->second != 0);

                    if (itr->second != 0)
                    {
                        m_counterNames.push_back(counterNameFinder(itr->first));
                        m_counters.push_back(itr->second);
                    }
                }

                return !m_counters.empty();
            }

            void serialize(JSON::Adapter &adapter)
            {
                namespace StrKeys = Strings::DynamicJson::Counters;

                JSON::Class root(adapter, "NameValueCounters", false);

                if (m_counters.size() != m_counterNames.size())
                    throw std::runtime_error("NameValueCounters: internal vectors have mismatching sizes");

                if (m_counters.empty())
                    throw std::runtime_error("NameValueCounters: Counters are empty");

                JSON_E_KV_PRODUCER_ONLY(adapter, StrKeys::Name, m_counterNames);
                JSON_T_KV_PRODUCER_ONLY(adapter, StrKeys::Value, m_counters);
            }

        private:
            std::vector<std::string> m_counterNames;
            std::vector<ValueType> m_counters;
        };

        template <class RangeType, class ValueType>
        class StartEndCountBuckets
        {
        public:
            template <size_t ArrSize>
            bool Load(const RangeType(&rangeArr)[ArrSize], const ValueType(&countArr)[ArrSize])
            {
                BOOST_STATIC_ASSERT(ArrSize >= 2);

                m_starts.clear();
                m_ends.clear();
                m_counts.clear();

                for (size_t ind = 0; ind < ArrSize - 1; ind++)
                {
                    if (countArr[ind] != 0)
                    {
                        m_starts.push_back(rangeArr[ind]);
                        m_ends.push_back(rangeArr[ind + 1]);
                        m_counts.push_back(countArr[ind]);
                    }
                }

                if (countArr[ArrSize - 1] != 0)
                {
                    m_starts.push_back(rangeArr[ArrSize - 1]);
                    m_ends.push_back(std::numeric_limits<RangeType>::max());
                    m_counts.push_back(countArr[ArrSize - 1]);
                }

                return !m_counts.empty();
            }

            void serialize(JSON::Adapter &adapter)
            {
                namespace StrKeys = Strings::DynamicJson::Buckets;

                JSON::Class root(adapter, "StartEndCountBuckets", false);

                if (m_counts.size() != m_ends.size() || m_counts.size() != m_starts.size())
                    throw std::runtime_error("StartEndCountBuckets: internal vectors have mismatching sizes");

                if (m_counts.empty())
                    throw std::runtime_error("StartEndCountBuckets: Counters are empty");

                JSON_E_KV_PRODUCER_ONLY(adapter, StrKeys::Start, m_starts);
                JSON_E_KV_PRODUCER_ONLY(adapter, StrKeys::End, m_ends);
                JSON_T_KV_PRODUCER_ONLY(adapter, StrKeys::Count, m_counts);
            }

        private:
            std::vector<RangeType> m_starts, m_ends;
            std::vector<ValueType> m_counts;
        };

#ifdef YET_TO_LINK_HOST_AGENT_HELPERS
        // TODO-SanKumar-1711: Copied these members from EvtCollForwCommon.cpp. Should be moved to a
        // common lib instead of copy.

        // TODO-SanKumar-1711: Should we ship the exception instead of the value of the column,
        // to get bugs in production? Same for the next function.

        bool TryGetBiosId(std::string& biosId)
        {
            try
            {
                biosId = GetSystemUUID();
            }
            // TODO-SanKumar-1710: Should we throw on low mem, so any operation stops?
            GENERIC_CATCH_LOG_ACTION("Getting bios ID", return false);

            return true;
        }

        bool TryGetFabricType(std::string& fabricType)
        {
            try
            {
                std::string hypervisorName, hypervisorVer;
                IsVirtual(hypervisorName, hypervisorVer);

                if (!hypervisorVer.empty())
                    fabricType = hypervisorName + '_' + hypervisorVer;
                else
                    fabricType = hypervisorName;
            }
            GENERIC_CATCH_LOG_ACTION("Getting fabric type", return false);

            return true;
        }
#endif // YET_TO_LINK_HOST_AGENT_HELPERS

        static void IncrementBucket(const int64_t bucketRanges[], double value, int64_t buckets[], size_t bucketCount)
        {
            // TODO-SanKumar-1710: Should we mark an internal error, if that's the case?
            BOOST_ASSERT(value > 0); // If an activity occurred, the time can never be 0 and obviously not -ve.

            // Compare value against upper bound of all the buckets.
            for (size_t ind = 0; ind < bucketCount - 1; ind++)
            {
                if (value < bucketRanges[ind + 1])
                {
                    ++buckets[ind];
                    return;
                }
            }

            // If didn't fit in any of the ranges, then it's the last one (unbounded range).
            ++buckets[bucketCount - 1];
        }

        static void DecrementBucket(const int64_t bucketRanges[], double value, int64_t buckets[], size_t bucketCount)
        {
            // TODO-SanKumar-1710: Should we mark an internal error, if that's the case?
            BOOST_ASSERT(value > 0); // If an activity occurred, the time can never be 0 and obviously not -ve.

            // Compare value against upper bound of all the buckets.
            for (size_t ind = 0; ind < bucketCount - 1; ind++)
            {
                if (value < bucketRanges[ind + 1])
                {
                    // TODO-SanKumar-1710: Should we mark an internal error, if the count goes < 0.
                    --buckets[ind];
                    return;
                }
            }

            // If didn't fit in any of the ranges, then it's the last one (unbounded range).
            --buckets[bucketCount - 1];
        }
    };
}

#endif // !CXPS_TELEMETRY_COMMON_H
