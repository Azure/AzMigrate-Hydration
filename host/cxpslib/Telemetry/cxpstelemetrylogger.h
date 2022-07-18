#ifndef CXPS_TELEMETRY_LOGGER_H
#define CXPS_TELEMETRY_LOGGER_H

#include <map>

#include <boost/filesystem/path.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

#include "Rows/EmptyCxpsTelemetryRow.h"
#include "Rows/GlobalCxpsTelemetryRow.h"
#include "Stringer.h"
#include "TelemetryMaps.h"

namespace CxpsTelemetry
{
    namespace boost_fs = boost::filesystem;

    class CxpsTelemetryLogger
    {
    public:
        CxpsTelemetryLogger()
            : m_maxCompletedFilesCnt(0),
            m_isInitialized(false),
            // For the first row, this value is the thread spawn time
            m_lastRowUploadTime(boost_pt::microsec_clock::universal_time()),
            m_seqNum(0),
            m_curFileRowCount(0),
            m_printSessionId(0)
        {
        }

        virtual ~CxpsTelemetryLogger() { }

        void Initialize(
            const std::string &psId,
            const boost_fs::path &folderPath,
            chrono::seconds writeInterval,
            int maxCompletedFilesCnt);

        void Start();

        void ReportStoppingOfServers()
        {
            m_areServersStopping = true;
        }

        bool AreServersStopping()
        {
            return m_areServersStopping;
        }

        void Stop();

        void ReportRowObjDrop(MessageType msgType);

        // Singleton pattern
        static inline CxpsTelemetryLogger& GetInstance()
        {
            return s_Instance;
        }

        const std::string& GetPsId()
        {
            return m_psId;
        }

        GlobalCxpsTelemetryRowPtr GetValidGlobalTelemetry()
        {
            LockGuard guard(m_globalTelObjsLock);

            try
            {
                if (!m_telRowCxpsGlobal || !m_telRowCxpsGlobal->IsValid())
                {
                    m_telRowCxpsGlobal.reset();
                    m_telRowCxpsGlobal = boost::make_shared<GlobalCxpsTelemetryRow>(GetLastRowUploadTime());
                }
            }
            GENERIC_CATCH_LOG_ACTION("GetValidGlobalTelemetry", return GlobalCxpsTelemetryRowPtr());
            // TODO-SanKumar-1711: On failure to get valid ptr, set a static
            // counter s_globTelCreationFailures and log that. Load it on next
            // succ creation.

            return m_telRowCxpsGlobal;
        }

        GlobalCxpsTelemetryRowPtr PruneOrGetGlobalTelemetry()
        {
            LockGuard guard(m_globalTelObjsLock);

            try
            {
                //if (m_telRowCxpsGlobal && m_telRowCxpsGlobal->HasExpired() && m_telRowCxpsGlobal->IsInvalidOrEmpty())
                // Explicitly not performing the expiry related pruning here. Since,
                // this is just one object, we can afford to keep it as long as we
                // want. That way, the continuity in the logger timestamps of the
                // global telemetry is not broken.

                if (m_telRowCxpsGlobal && !m_telRowCxpsGlobal->IsValid())
                    m_telRowCxpsGlobal.reset();
            }
            GENERIC_CATCH_LOG_ACTION("PruneOrGetGlobalTelemetry", return GlobalCxpsTelemetryRowPtr());

            return m_telRowCxpsGlobal;
        }

        boost_pt::ptime GetLastRowUploadTime()
        {
            LockGuard guard(m_telLifeTimePropLock);
            return m_lastRowUploadTime;
        }

        void AddRequestDataToTelemetryLogger(const RequestTelemetryData &reqTelData);

    private:
        // Data members for the singleton pattern
        static CxpsTelemetryLogger s_Instance;
        bool m_isInitialized;
        bool m_areServersStopping;

        // Data members for the log cutting logic
        boost::shared_ptr<boost::thread> m_thread;
        std::string m_psId;
        boost_fs::path m_folderPath;
        std::string m_completedFileNamePrefix;
        boost_fs::path m_completedFileSearchPattern;
        boost_fs::path m_currentFilePath;
        chrono::seconds m_writeInterval;
        int m_maxCompletedFilesCnt;

        boost::atomic<int64_t> m_seqNum;
        size_t m_curFileRowCount;
        boost::atomic<int64_t> m_printSessionId;

        boost::mutex m_telLifeTimePropLock;
        boost_pt::ptime m_lastRowUploadTime;

        std::string m_cacheFileLine;
        EmptyCxpsTelemetryRow m_cacheEmptyTelRow;
        GlobalCxpsTelemetryRow m_cacheGlobalTelRow;

        boost::mutex m_globalTelObjsLock;
        GlobalCxpsTelemetryRowPtr m_telRowCxpsGlobal;

        HostLevelTelemetryMap m_hostTelemetryMap;

        void Run();
        void WriteDataToNextFile(bool isFinalWrite);
        bool WriteRowToFile(
            std::ofstream &ofstream, CxpsTelemetryRowBase &rowObj,
            CxpsTelemetryRowBase &cacheObj, bool &isRowObjUnusable);

        void AddDiffTelemetryData(const RequestTelemetryData& reqTelData, const std::string &hostId, const std::string &device)
        {
            SourceDiffTelemetryRowPtr diffTelObj = m_hostTelemetryMap.GetValidSourceDiffTelemetry(hostId, device);
            if (!diffTelObj)
            {
                // TODO-SanKumar-1710: Mark in session tel - with device id?

                return;
            }

            // TODO-SanKumar-1710: Add the current data to the telemetry. Shall we rather create
            // create a temp object and then add it to the golden object?
        }

        void AddResyncTelemetryData(const RequestTelemetryData& reqTelData, const std::string &hostId, const std::string &device)
        {
            SourceResyncTelemetryRowPtr resyncTelObj = m_hostTelemetryMap.GetValidSourceResyncTelemetry(hostId, device);
            if (!resyncTelObj)
            {
                // TODO-SanKumar-1710: Mark in session tel - with device id?

                return;
            }

            // TODO-SanKumar-1710: Add the current data to the telemetry. Shall we rather create
            // create a temp object and then add it to the golden object?
        }

        void SetLastRowUploadTime(boost_pt::ptime newTime)
        {
            LockGuard guard(m_telLifeTimePropLock);
            m_lastRowUploadTime = newTime;
        }

        void LapseSequenceNumber(int64_t cnt = 1)
        {
            // Increment the sequence number to indicate possibly missing row/data
            m_seqNum.fetch_add(cnt, boost::memory_order_relaxed);
        }
    };
}
#endif // !CXPS_TELEMETRY_LOGGER_H
