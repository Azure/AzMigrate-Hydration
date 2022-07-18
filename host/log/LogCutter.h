#ifndef LOG_CUTTER_H
#define LOG_CUTTER_H

#include <ace/Mutex.h>
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <string>
#include <set>

#define LOG_DEFAULT_MAX_SIZE_IN_BYTES    1048576 // 1MB

namespace Logger
{

    bool GetLogFilePath(std::string &path);

    class Log {
    public:
        Log() : 
        m_logMaxSize(LOG_DEFAULT_MAX_SIZE_IN_BYTES), 
        m_shouldLogInUtc(false),
        m_logSequenceNum(1){}

        std::ofstream m_logStream;
        std::string m_logFileName;
        ACE_Mutex m_syncLog;
        uint32_t m_logMaxSize;
        bool m_shouldLogInUtc;
        uint64_t    m_logSequenceNum;

#ifdef SV_WINDOWS

		boost::thread_specific_ptr<std::ofstream> m_threadSpecificLogStream;
		boost::thread_specific_ptr<std::string> m_threadSpecificLogFileName;
#endif

        void SetLogFileName(const std::string& filename);
        void SetLogFileSize(uint32_t logSize);
        void SetLogGmtTimeZone(bool shouldLogInUtc);

        void TruncateLogFile();
        void CloseLogFile();
        void Printf(const std::string& message);
    private:
        void WriteToLog(const std::string& message);
    };

    class LogCutter
    {
    public:
        static const std::string CurrLogTag;
        static const std::string CompletedLogTag;
        static const std::string LogStartIndicator;

        LogCutter(Log &log)
            : m_startedThread(false),
            m_initialized(false),
            m_currSequenceNumber(~0ULL),
            m_log(log)
        {
            m_log.SetLogGmtTimeZone(true);
        }

        virtual ~LogCutter();
        
        void Initialize(
            const boost::filesystem::path &logFilePath,
            int maxCompletedLogFiles, boost::chrono::seconds cutInterval, const std::set<std::string>& setCompFilesPaths = std::set<std::string>());
        
        void StartCutting();

        void StopCutting();

        static boost::filesystem::path GetCompletedLogFilePath(
            const boost::filesystem::path &logFilePath, uint64_t sequenceNumber);

        static boost::filesystem::path GetCurrLogFilePath(
            const boost::filesystem::path &logFilePath, uint64_t sequenceNumber);

        static void ListCompletedLogFiles(
            const boost::filesystem::path &logFilePath, std::vector<uint64_t> &fileSeqNumbers);

        static void ListNotCompletedLogFiles(
            const boost::filesystem::path &logFilePath,
            uint64_t uptoInclusive, std::vector<uint64_t> &fileSeqNumbers);

    private:
        uint64_t m_currSequenceNumber;
        boost::shared_ptr<boost::thread> m_cutterThread;
        bool m_startedThread, m_initialized;
        boost::filesystem::path m_logFilePath;
        int m_maxCompletedLogFiles;
        boost::chrono::seconds m_cutInterval;

        Log &m_log;

        void WaitCheckCut();
        void CompletePreviousLogFiles(const bool completeAll = 0);
        void CompleAllLogFiles();
        void StopCutterThread();
    };
}

#endif // LOG_CUTTER_H
