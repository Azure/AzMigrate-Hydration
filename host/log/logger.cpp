#include <stdarg.h>
#include <stdio.h>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <iostream>

#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "json_writer.h"

#include "portable.h"
#include "synchronize.h"
#include "localconfigurator.h"
#include "inmsafecapis.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "curlwrapper.h"
#include "logger.h"
#include "LogCutter.h"
#include "Telemetry.h"


#ifdef SV_UNIX

#include <syslog.h>
#include "hostagenthelpers_ported.h"

#else

#include <windows.h>
#include <assert.h>
#include <atlbase.h>
#include "hostagenthelpers.h"
#include "globs.h"

#define DEBUG_POSTFIX_STR ".log"

#endif

#ifdef SV_UNIX
#include "setpermissions.h"
#endif

using namespace std; // Added to support file io
using namespace Logger;
using namespace AgentTelemetry;

ESERIALIZE_TYPE serializerType = None ;
std::string localhostlogfile;
#define LOGGER_URL "/cxlogger.php"
#define TAG_FAILED "FAILED"
#define TAG_WARNING "WARNING"

#define RETURN_ON_LOG_LEVEL_CHECK(LogLevel)  \
    {   \
        if (SV_LOG_DISABLE == nLogLevel || \
            (SV_LOG_ALWAYS != nLogLevel && \
             SV_LOG_ALWAYS != LogLevel && \
             LogLevel > nLogLevel && \
             isDaemon)) \
        {   \
            return; \
        }   \
    }

const int MAX_LOG_BUFFER_SIZE = 256 * 1024; // in bytes, 256KB

long linecount = 0;

char pchLogLevelMapping[][10]={"DISABLE","FATAL","SEVERE","ERROR","WARNING","INFO","DEBUG","ALWAYS"};
int nLogLevel = SV_LOG_ALWAYS;
int nLogInitSize = 10 * 1024 * 1024;
ofstream errorStream ;
Logger_Agent_Type LogAgenttype = VxAgentLogger;
int nRemoteLogLevel = SV_LOG_DISABLE; 
string AgentHostId = "";
string CxServerIpAddress = "";
int CxServerPort = 80;
//bool InDebugPrintf = false;
bool isDaemon = false;
bool isHttps = false;

// serailize writes to local log file
// currently does not protect multiple dataprotections from writting
// to the same log as this mutex is only good with in the process
typedef ACE_Mutex SyncLog_t;
typedef ACE_Guard<SyncLog_t> SyncLogGuard_t;
SyncLog_t SyncLog;

void OutputDebugString(int nOutputLoggingLevel, char* szDebugStr);
void TruncateFile();
static bool DebugPrintfToCx(const char *ipAddress, SV_INT port, const char *pchPostURL, SV_LOG_LEVEL LogLevel, const char *szDebugString,const bool& https);
static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, SV_LONG hr, const char* format, va_list args );
static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, const char* format, va_list args );
static void DebugPrintfToLocalHostLog(SV_LOG_LEVEL LogLevel, const std::string& szDebugString) ;

namespace Logger
{
    const std::string LogCutter::CurrLogTag = "_curr_";
    const std::string LogCutter::CompletedLogTag = "_completed_";
    const std::string LogCutter::LogStartIndicator = "#~> ";
}

Log gLog;

#ifdef SV_WINDOWS
//This part of code is applicable only for WINDOWS and is written specific to Scout Mail Recovery (SMR)
//Requirement: Backup debug log files if file size reaches nLogInitSize
//Number of debug log files to be backed up is configurable.
//TODO: make the function thread safe
//TODO: make the function generic to WINDOWS and LINUX
bool bBackupDebugLog = false;
int maxBackupFileCount = 1;
LONG nextFileIndex = 0;
string szLogFileDir = ".\\";
string szLogFileName = "drerror";
string szLogFileExt = ".log";
bool FileExist(string filename);
void BackupAndResetStream();

template <typename T>
std::string ToString(const T &val)
{
    std::ostringstream ss;
    ss << val;
    return ss.str();
}
bool FileExist(string filename)
{
    std::ifstream ifile;
    ifile.open(filename.c_str());
    
    if (ifile.is_open() == true)
    {
        ifile.close();
        return true;
    }
    else
        return false;
}
#endif


bool GetLogLevelName(SV_LOG_LEVEL logLevel, std::string& name)
{
    BOOST_STATIC_ASSERT(SV_LOG_LEVEL_COUNT == ARRAYSIZE(pchLogLevelMapping));

    if (logLevel >= 0 && logLevel < SV_LOG_LEVEL_COUNT)
    {
        name = pchLogLevelMapping[logLevel];
        return true;
    }

    return false;
}

bool GetLogLevel(const std::string &logLevelName, SV_LOG_LEVEL &logLevel)
{
    BOOST_STATIC_ASSERT(SV_LOG_LEVEL_COUNT == ARRAYSIZE(pchLogLevelMapping));

    for (size_t ind = 0; ind < ARRAYSIZE(pchLogLevelMapping); ind++)
    {
        if (0 == logLevelName.compare(pchLogLevelMapping[ind]))
        {
            logLevel = static_cast<SV_LOG_LEVEL>(ind);
            return true;
        }
    }

    return false;
}

bool Logger::GetLogFilePath(std::string &path)
{
    if (0 == gLog.m_logFileName.length())
        return false;

    boost::filesystem::path logFilePath(gLog.m_logFileName);
    boost::filesystem::path folderPath = logFilePath.parent_path();

    if (!boost::filesystem::exists(folderPath) || !boost::filesystem::is_directory(folderPath))
        return false;

    path = folderPath.string();

    return true;
}
//
// Set the debug log level
// 
void SetLogLevel(int level)
{
    if(level <= SV_LOG_ALWAYS && level >= SV_LOG_DISABLE) 
        nLogLevel = level;
}
//
// Set the initial size of the debug log
//
void SetLogInitSize(int size)
{
    nLogInitSize = size;
}

//
// Initialize the debug log file name
//
void SetLogFileName(const char* fileName)
{
    gLog.m_logFileName = fileName;
    gLog.m_logStream.clear();

#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    // PR#10815: Long Path support
    gLog.m_logStream.open(getLongPathName(fileName).c_str(), ios::out | ios::app);
#else
    gLog.m_logStream.open(fileName,ios::out | ios::app );
#if defined (SV_UNIX) || defined (SV_LINUX)
    try {
        securitylib::setPermissions(fileName);
    }
    catch(ErrorException &ec)
        {
        }
        catch(...)
        {
        }
    
#endif
#endif
}

//
// Initialize the debug log file name specific to the thread.
//
void SetThreadSpecificLogFileName(const char* fileName)
{
#ifdef SV_WINDOWS
    gLog.m_threadSpecificLogFileName.reset(new std::string(fileName));

#if defined (SV_USES_LONGPATHS)
    // PR#10815: Long Path support
    gLog.m_threadSpecificLogStream.reset(new std::ofstream(getLongPathName(fileName).c_str(), ios::out | ios::app));
#else
    gLog.m_threadSpecificLogStream.reset(new std::ofstream(fileName, ios::out | ios::app));
#endif

#endif
}

void SetLocalHostLogFile( const char* fileName )
{
    localhostlogfile = fileName ;
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    // PR#10815: Long Path support
    errorStream.open(getLongPathName(fileName).c_str(),ios::out | ios::app );
#else
    errorStream.open(fileName,ios::out | ios::app );
#endif
    return ;
}


void SetLogHttpIpAddress( const char* ipAddress )
{
    CxServerIpAddress = ipAddress;
}

void SetLogHttpPort( int port )
{
    CxServerPort = port;
}

void SetLogHostId( const char* hostId )
{
    AgentHostId = hostId;
}

void SetLogRemoteLogLevel( int remoteLogLevel )
{
    if(remoteLogLevel <= SV_LOG_ALWAYS && remoteLogLevel >= SV_LOG_DISABLE) 
        nRemoteLogLevel = remoteLogLevel;
}

void SetDaemonMode(bool isdaemon)
{
    isDaemon = isdaemon;
}

void SetDaemonMode()
{
    SetDaemonMode(true);
}
void SetLogHttpsOption(const bool& https)
{
    isHttps = https;
}

//
// Truncate and open new log file
//
void TruncateFile()
{
    gLog.m_logStream.close();
    gLog.m_logStream.open(gLog.m_logFileName.c_str(), ios::out | ios::trunc);
}
//
// Close the debug log
//
void CloseDebug()
{
    gLog.m_logStream.close();
    CloseDebugThreadSpecific();
    if( errorStream.is_open() )
    {
        errorStream.close() ;
    }
#ifdef SV_UNIX
    closelog();
#endif
}

//
// Close the debug log specific to the thread.
//
void CloseDebugThreadSpecific()
{
#ifdef SV_WINDOWS
    if (gLog.m_threadSpecificLogStream.get())
    {
        if (gLog.m_threadSpecificLogStream->is_open())
        {
            gLog.m_threadSpecificLogStream->close();
        }
        gLog.m_threadSpecificLogStream.reset();
    }

#endif
}

void Log::SetLogFileSize(uint32_t size)
{
    m_logMaxSize = size;
}

void Log::SetLogGmtTimeZone(bool shouldLogInUtc)
{
    m_shouldLogInUtc = shouldLogInUtc;
}

//
// Initialize the debug log file name
//
void Log::SetLogFileName(const std::string& fileName)
{
    m_logFileName = fileName;
    
    if (m_logStream.is_open())
    {
        m_logStream.close();
    }

    m_logStream.clear();

#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    // PR#10815: Long Path support
    m_logStream.open(getLongPathName(fileName.c_str()).c_str(), ios::out | ios::app);
#else
    m_logStream.open(fileName.c_str(), ios::out | ios::app);
#if defined (SV_UNIX) || defined (SV_LINUX)
    try {
        securitylib::setPermissions(fileName);
    }
    catch(ErrorException &ec)
        {
        }
        catch(...)
        {
        }
#endif
#endif
}

void Log::TruncateLogFile()
{
    // TODO : log the truncation by fetching the start and end lines

    m_logStream.close();
    
    ACE_stat stat = { 0 };
    int statRetVal;

#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    statRetVal = sv_stat(getLongPathName(m_logFileName.c_str()).c_str(), &stat);
    m_logStream.open(getLongPathName(m_logFileName.c_str()).c_str(), ios::out | ios::trunc);
#else
    statRetVal = ACE_OS::stat(m_logFileName.c_str(), &stat);
    m_logStream.open(m_logFileName.c_str(), ios::out | ios::trunc);
#endif

    if (0 == statRetVal)
    {
        LogTruncStatus at;
        ADD_INT_TO_MAP(at, AgentTelemetry::MESSAGETYPE, AgentTelemetry::MESSAGE_TYPE_LOG_TRUNCATE);
        ADD_STRING_TO_MAP(at, AgentTelemetry::TRUNCSTART,
            boost::posix_time::to_iso_extended_string(boost::posix_time::from_time_t(stat.st_ctime)));
        ADD_STRING_TO_MAP(at, AgentTelemetry::TRUNCEND, 
            boost::posix_time::to_iso_extended_string(boost::posix_time::from_time_t(stat.st_mtime)));

        try {
            std::string truncMsg = JSON::producer<LogTruncStatus>::convert(at);
            WriteToLog(truncMsg);
        }
        catch (const std::exception &e)
        {
            //DebugPrintf(SV_LOG_ERROR, "%s caught exception %s.\n", FUNCTION_NAME, e.what());
        }
        catch (...)
        {
            //DebugPrintf(SV_LOG_ERROR, "%s caught unknown exception.\n", FUNCTION_NAME);
        }
    }
    else
    {
        //DebugPrintf(SV_LOG_ERROR, "%s sv_stat returned %d\n", FUNCTION_NAME, statRetVal);
    }
}

void Log::CloseLogFile()
{
    if (m_logStream.is_open())
    {
        m_logStream.close();
    }
}

void SetSerializeType( ESERIALIZE_TYPE serializer  )
{
    serializerType = serializer ;
}

void SetLoggerAgentType(Logger_Agent_Type)
{
    LogAgenttype=FxAgentLogger;
}

Logger_Agent_Type GetLoggerAgentType(void)
{
    return LogAgenttype;
}

//
// Output the debug string to the debug log
//
void OutputDebugStringToLog(int nOutputLoggingLevel, char* szDebugStr)
{  
    RETURN_ON_LOG_LEVEL_CHECK(nOutputLoggingLevel)

    SyncLogGuard_t guard(gLog.m_syncLog);

    if (gLog.m_logStream.good())
    {
        int currPos = gLog.m_logStream.tellp();
        if (currPos != -1)
        {
            // TODO-SanKumar-1612: Should we also take into consideration the old log files yet to be
            // parsed and/or cleaned up by EvtCollForw (both completed and current files)? If that's
            // the case, no file system query in here. Rather update it via the log cutter thread.

            // Each char costs one byte.
            if (currPos > nLogInitSize)
                TruncateFile();
        }
        else
        {
#ifdef SV_WINDOWS
            gLog.m_logStream.clear(0);
#else
            gLog.m_logStream.clear();
#endif
        }
    }

    struct tm today;
    time_t ltime;
    time(&ltime);

#ifdef SV_WINDOWS
    if (gLog.m_shouldLogInUtc)
        gmtime_s(&today, &ltime);
    else
        localtime_s(&today, &ltime);
#else
    if (gLog.m_shouldLogInUtc)
        gmtime_r(&ltime, &today);
    else
        localtime_r(&ltime, &today);
#endif

    char present[70];
    memset(present,0,70);
    inm_sprintf_s(present, ARRAYSIZE(present), "(%02d-%02d-20%02d %02d:%02d:%02d): %7s ",
            today.tm_mon + 1,
            today.tm_mday,
            today.tm_year - 100,
            today.tm_hour,
            today.tm_min,
            today.tm_sec,
            pchLogLevelMapping[nOutputLoggingLevel] 
            );
        
    if (gLog.m_logStream.fail())
    {
#ifdef SV_WINDOWS
        //cout << "SVSTREAM FAILED \n";
        gLog.m_logStream.clear(0);
#else
        gLog.m_logStream.clear();
#endif
    }

    gLog.m_logStream << Logger::LogCutter::LogStartIndicator //"#~> "
             << present << " "
             << ACE_OS::getpid() << " "
             << ACE_OS::thr_self() << " "
             << gLog.m_logSequenceNum++ << " "
             << szDebugStr;
    gLog.m_logStream.flush();

#ifdef SV_WINDOWS

    if (gLog.m_threadSpecificLogStream.get())
    {
        gLog.m_threadSpecificLogStream -> clear(0);
        *(gLog.m_threadSpecificLogStream.get()) << Logger::LogCutter::LogStartIndicator //"#~> "
            << present << " "
            << ACE_OS::getpid() << " "
            << ACE_OS::thr_self() << " "
            << gLog.m_logSequenceNum++ << " "
            << szDebugStr;
        gLog.m_threadSpecificLogStream -> flush();
    }
#endif

}


//
// This function sends the debug messages to Cx
//
static SVERROR PostToServer(const char* pszHost, SV_INT HttpPort, const char* pszUrl, const char* pszBuffer, SV_ULONG BufferLength ,const bool& bHttps)
{
    SVERROR rc;
    CurlOptions options(pszHost, HttpPort, pszUrl, bHttps);
    options.transferTimeout(CX_TIMEOUT);

    try {
        CurlWrapper cw;
        cw.post(options,pszBuffer);
    } catch(ErrorException& exception )
    {
        //DebugPrintf( "FAILED Couldn't post to %s(%d):%s [%d bytes]\n", pszHost, HttpPort, pszUrl, BufferLength );
        rc = SVE_FAIL;
    }

    return( rc );
}


void Log::Printf(const std::string& message)
{
    SyncLogGuard_t guard(m_syncLog);

    if (m_logStream.good())
    {
        int currPos = m_logStream.tellp();
        if (currPos != -1)
        {
            // TODO-SanKumar-1612: Should we also take into consideration the old log files yet to be
            // parsed and/or cleaned up by EvtCollForw (both completed and current files)? If that's
            // the case, no file system query in here. Rather update it via the log cutter thread.

            // Each char costs one byte.
            if (currPos > m_logMaxSize)
                TruncateLogFile();
        }
        else
        {
#ifdef SV_WINDOWS
            m_logStream.clear(0);
#else
            m_logStream.clear();
#endif
        }
    }

    WriteToLog(message);

    return;
}

void Log::WriteToLog(const std::string& message)
{
    struct tm today;
    time_t ltime;
    time(&ltime);

#ifdef SV_WINDOWS
    if (m_shouldLogInUtc)
        gmtime_s(&today, &ltime);
    else
        localtime_s(&today, &ltime);
#else
    if (m_shouldLogInUtc)
        gmtime_r(&ltime, &today);
    else
        localtime_r(&ltime, &today);
#endif

    char present[70];
    memset(present, 0, 70);
    inm_sprintf_s(present, ARRAYSIZE(present), "(%02d-%02d-20%02d %02d:%02d:%02d): %7s ",
        today.tm_mon + 1,
        today.tm_mday,
        today.tm_year - 100,
        today.tm_hour,
        today.tm_min,
        today.tm_sec,
        pchLogLevelMapping[SV_LOG_ALWAYS]
        );

    if (m_logStream.fail())
    {
#ifdef SV_WINDOWS
        //cout << "SVSTREAM FAILED \n";
        m_logStream.clear(0);
#else
        m_logStream.clear();
#endif
    }

    m_logStream << Logger::LogCutter::LogStartIndicator //"#~> "
        << present << " "
        << ACE_OS::getpid() << " "
        << ACE_OS::thr_self() << " "
        << m_logSequenceNum++ << " "
        << message
        << std::endl;
    m_logStream.flush();
}

///
/// Sends printf() style arguments to the debugging console. Like all variadic functions, is
/// not typesafe; you must pass the same number of arguments as there are format fields.
///
void SV_CDECL DebugPrintf( SV_LOG_LEVEL LogLevel, const char* format, ... )
{
    RETURN_ON_LOG_LEVEL_CHECK(LogLevel)

    va_list a;
    int iLastError = ACE_OS::last_error();
    
    if( 0 == strlen( format ) )
    {
        return;
    }

    va_start( a, format );

    DebugPrintfV( LogLevel, format, a );
    
    va_end( a );
    ACE_OS::last_error(iLastError);
}

///
/// Sends printf() style arguments to the debugging console. Uses default log level (LOG_DEBUG).
///
void SV_CDECL DebugPrintf( const char* format, ... )
{
    if( 0 == strlen( format ) )
    {
        return ;
    }

    int iLastError = ACE_OS::last_error();
    va_list a;    
    char *pSubStringFailed = TAG_FAILED;
    char *pSubStringWarning = TAG_WARNING;

    va_start( a, format );
    SV_LOG_LEVEL LogLevel = SV_LOG_DEBUG;

    if (strstr (format, pSubStringFailed) )
    {
        LogLevel = SV_LOG_ERROR;
    }
    else if ( strstr (format, pSubStringWarning) )
    {
        LogLevel = SV_LOG_WARNING;
    }

    if ( (SV_LOG_DISABLE == nLogLevel) || 
         (SV_LOG_ALWAYS != nLogLevel && SV_LOG_ALWAYS != LogLevel && LogLevel > nLogLevel && isDaemon) )
    {
        // do nothing
    }
    else
    {
        DebugPrintfV(LogLevel, format, a);
    }

    va_end( a ); 
    ACE_OS::last_error(iLastError);
}


static bool DebugPrintfToCx(const char *ipAddress, SV_INT port, const char *pchPostURL, SV_LOG_LEVEL LogLevel, const char *szDebugString,const bool& https)
{
    char *pszPostbuff = NULL;
    SVERROR hr = SVS_OK;
    string Agent_type;
    
    if(AgentHostId.size() <=0 || strlen(ipAddress) <=0 ) 
    {
        return (false);
    }

    time_t ltime;
    struct tm *today;

    time( &ltime );
    today= localtime( &ltime );
    char present[30];
    memset(present,0,30);

    inm_sprintf_s(present, ARRAYSIZE(present),"(%02d-%02d-%02d %02d:%02d:%02d): ",
            today->tm_year - 100,
            today->tm_mon + 1,
            today->tm_mday,        
            today->tm_hour,
            today->tm_min,
            today->tm_sec
            );

    if(GetLoggerAgentType() == FxAgentLogger)
        Agent_type="FX";
    else
        Agent_type="VX";

    std::stringstream szhttpbuff;
    szhttpbuff << "ttime=" << present << "&id=" << AgentHostId.c_str() << "&errlvl=" << pchLogLevelMapping[LogLevel] << "&agent=" << Agent_type.c_str() << "&msg=";
    size_t postbuflen;
    INM_SAFE_ARITHMETIC(postbuflen = InmSafeInt<size_t>::Type(szhttpbuff.str().length()) + strlen(szDebugString) + 1, INMAGE_EX(szhttpbuff.str().length())(strlen(szDebugString)))
    pszPostbuff = (char*)malloc(postbuflen);

    if (NULL == pszPostbuff)
    {
        return (false);
    }
    inm_strcpy_s(pszPostbuff, ( szhttpbuff.str().length() + strlen(szDebugString) + 1), szhttpbuff.str().c_str());
    inm_strcat_s(pszPostbuff, ( szhttpbuff.str().length() + strlen(szDebugString) + 1), szDebugString);
    PostToServer(ipAddress, port, pchPostURL, pszPostbuff, static_cast<SV_ULONG>( strlen(pszPostbuff) ),https );
    free (pszPostbuff);
    return (false);
}




///
/// Sends printf() style arguments to the debugging console + hr message. 
/// Uses DV_LOG_ERROR if the hr is a failed hr else use SV_LOG_INFO.
///
void SV_CDECL DebugPrintf( SV_LONG hr, const char* format, ... )
{
    int iLastError = ACE_OS::last_error();

    if( 0 == strlen( format ) )
    {
        return;
    }

    //FAILED Macro is for Windows only .. 
    SV_LOG_LEVEL logLevel ;
    if  (hr < 0 ) 
        logLevel =  SV_LOG_ERROR ;
    else
        logLevel =  SV_LOG_INFO;

    va_list a;
    va_start( a, format );
    
//    if ( InDebugPrintf == false)
    {
//		InDebugPrintf = true;
        DebugPrintfV( logLevel, hr, format, a );
//		InDebugPrintf = false;
    }
    
    va_end(a);
    ACE_OS::last_error(iLastError);
}

///
/// Similar to DebugPrintf(), but also prints a descriptive string of the error code hr.
///
void SV_CDECL DebugPrintf(SV_LOG_LEVEL LogLevel,SV_LONG hr,const char*format, ...)
{
    RETURN_ON_LOG_LEVEL_CHECK(LogLevel)

    int iLastError = ACE_OS::last_error();

    if( 0 == strlen( format ) )
    {
        return;
    }

    va_list a;
    va_start( a, format );
    
//    if ( InDebugPrintf == false)
    {
//		InDebugPrintf = true;
        DebugPrintfV( LogLevel, hr, format, a );
//		InDebugPrintf = false;
    }
    va_end( a );
    ACE_OS::last_error(iLastError);

}

void SV_CDECL DebugPrintf( int logLevel, const char* format, ... )
{
    RETURN_ON_LOG_LEVEL_CHECK(logLevel)

    int iLastError = ACE_OS::last_error();

    if( 0 == strlen( format ) )
    {
        return;
    }

    va_list a;
    va_start( a, format );
//    if ( InDebugPrintf == false)
    {
//	InDebugPrintf = true;
    DebugPrintfV((SV_LOG_LEVEL) logLevel, format, a );
//	InDebugPrintf = false;
    }
    va_end( a ); 
    ACE_OS::last_error(iLastError);
}

namespace Logger
{
    void LogCutter::WaitCheckCut()
    {
        for (;;)
        {
            try
            {
                try
                {
                    //DebugPrintf(SV_LOG_DEBUG, "Starting the next Wait before check and cut in LogCutter.\n");
                    boost::this_thread::sleep_for(m_cutInterval);
                }
                catch (boost::thread_interrupted)
                {
                    //DebugPrintf(SV_LOG_INFO, "WaitCheckCut thread interrupt requested.\n");
                    return;
                }

                vector<uint64_t> seqNums;
                ListCompletedLogFiles(m_logFilePath, seqNums);
                if (seqNums.size() < m_maxCompletedLogFiles)
                {
                    {
                        SyncLogGuard_t guard(m_log.m_syncLog);

                        if ((long)m_log.m_logStream.tellp() == 0)
                        {
                            // The log file is empty and it's unnecessary to cut it here, as the consumer of
                            // the cut log files would've no a action on a log file without any data. Let's
                            // continue with the same log file.

                            continue;
                        }

                        BOOST_ASSERT(0 == m_log.m_logFileName.compare(
                            GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber).string()));

                        m_log.CloseLogFile();

                        std::string nextFileName =
                            GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber + 1).string();

                        m_log.SetLogFileName(nextFileName);
                        m_currSequenceNumber++;
                    } // It's enough to scope the guard until here, since its intention of starting a new log file has succeeded.

                    CompletePreviousLogFiles();
                }
            }
            catch (...)
            {
                // TODO-SanKumar-1612: Log

                // TODO-SanKumar-1612: Quit the thread after max errors.
            }
        }
    }

    // Completes log files. Offers a provision to pass sequence number up to which user want to complete pending curr files.
    // @param completeAll : Indicates to complete pending all curr log files.
    //        For example dataprotection require to complete all curr file before shutdown so it calls CompleAllLogFiles just before shutdown.
    // *Note: Always keeps a curr file before process exits if completeAll is true.
    void LogCutter::CompletePreviousLogFiles(const bool completeAll)
    {
        std::vector<uint64_t> unmarkedCurrLogFiles;
        const uint64_t maxSeqNumToBeIncluded = completeAll ? m_currSequenceNumber : (m_currSequenceNumber - 1);
        ListNotCompletedLogFiles(m_logFilePath, maxSeqNumToBeIncluded, unmarkedCurrLogFiles);

        bool shouldCreateEmptyCurrFile = false;

        for (std::vector<uint64_t>::iterator itr = unmarkedCurrLogFiles.begin(); itr != unmarkedCurrLogFiles.end(); itr++)
        {
            boost::system::error_code errorCode;
            // Try at best effort to complete any previous log files that were missed out.
            try {
                boost::filesystem::path currLogFilePath(GetCurrLogFilePath(m_logFilePath, *itr));
                if (m_currSequenceNumber == *itr)
                {
                    // special case: keep curr file if its size is 0.
                    const uintmax_t size = boost::filesystem::file_size(currLogFilePath, errorCode);
                    if ((0 != errorCode.value()) || !size)
                    {
                        // Leave curr file as size is zero or failed to fetch size.
                        break; // last curr file
                    }
                }

                // Rename curr file to completed only if number of completed files not crossed m_maxCompletedLogFiles
                vector<uint64_t> seqNums;
                ListCompletedLogFiles(m_logFilePath, seqNums);
                if (seqNums.size() >= m_maxCompletedLogFiles)
                {
                    continue;
                }
                else
                {
                    boost::filesystem::path completedLogFilePath(GetCompletedLogFilePath(m_logFilePath, *itr));
                    boost::filesystem::rename(currLogFilePath, completedLogFilePath, errorCode);
                    if (0 != errorCode.value())
                    {
                        // Ignore error and continue
                        //DebugPrintf(SV_LOG_ERROR, "%s: failed to rename %s to %s with error %d\n", FUNCTION_NAME,
                        //currLogFilePath.string().c_str(), completedLogFilePath.string().c_str(), errorCode.value());
                        continue;
                    }

                    if (m_currSequenceNumber == *itr)
                    {
                        // This is exit path - create an empty curr file before exit.
                        // This way we will always sync up with bookmark - curr seq number should be always equal or greater than one present in bookmark.
                        shouldCreateEmptyCurrFile = true;
                    }
                }
            }
            catch (const boost::filesystem::filesystem_error &e)
            {
                //DebugPrintf(SV_LOG_ERROR, "%s: caught exception %s.\n", FUNCTION_NAME, e.what());
            }
            catch (const std::exception &e)
            {
                //DebugPrintf(SV_LOG_ERROR, "%s: caught exception %s.\n", FUNCTION_NAME, e.what());
            }
            catch (...)
            {
                //DebugPrintf(SV_LOG_ERROR, "%s: caught an unknown exception.\n", FUNCTION_NAME);
            }
        }

        if (shouldCreateEmptyCurrFile)
        {
            m_currSequenceNumber++;
            std::string currLogFileName = GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber).string();
            m_log.SetLogFileName(currLogFileName.c_str());
        }
    }

    void LogCutter::CompleAllLogFiles()
    {
        CompletePreviousLogFiles(true);
    }

    void LogCutter::ListCompletedLogFiles(const boost::filesystem::path &logFilePath, std::vector<uint64_t> &fileSeqNumbers)
    {
        boost::filesystem::path folderPath = logFilePath.parent_path();
        std::string stem = logFilePath.stem().string(), ext = logFilePath.extension().string();

        if (!boost::filesystem::exists(folderPath) || !boost::filesystem::is_directory(folderPath))
            return;

        const std::string matchPattern = stem + CompletedLogTag;

        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator itr(folderPath); itr != end; itr++)
        {
            const std::string currFileName = itr->path().filename().string();
            if (boost::algorithm::starts_with(currFileName, matchPattern)
                && boost::algorithm::ends_with(currFileName, ext))
            {
                std::string currSeqNumStr = currFileName.substr(matchPattern.size(),
                    currFileName.size() - matchPattern.size() - ext.size());

                try
                {
                    fileSeqNumbers.push_back(boost::lexical_cast<uint64_t>(currSeqNumStr));
                }
                catch (boost::bad_lexical_cast)
                {
                    // TODO-SanKumar-1612: Log.
                }
            }
        }

        std::sort(fileSeqNumbers.begin(), fileSeqNumbers.end());
    }

    void LogCutter::ListNotCompletedLogFiles(
        const boost::filesystem::path &logFilePath, uint64_t uptoInclusive, std::vector<uint64_t> &fileSeqNumbers)
    {
        boost::filesystem::path folderPath = logFilePath.parent_path();
        std::string stem = logFilePath.stem().string(), ext = logFilePath.extension().string();

        if (!boost::filesystem::exists(folderPath) || !boost::filesystem::is_directory(folderPath))
            return;

        const std::string matchPattern = stem + CurrLogTag;

        boost::filesystem::directory_iterator end;
        for (boost::filesystem::directory_iterator itr(folderPath); itr != end; itr++)
        {
            const std::string currFileName = itr->path().filename().string();
            if (boost::algorithm::starts_with(currFileName, matchPattern)
                && boost::algorithm::ends_with(currFileName, ext))
            {
                std::string currSeqNumStr = currFileName.substr(matchPattern.size(),
                    currFileName.size() - matchPattern.size() - ext.size());

                try
                {
                    uint64_t currSeqNumber = boost::lexical_cast<uint64_t>(currSeqNumStr);
                    if (currSeqNumber <= uptoInclusive)
                        fileSeqNumbers.push_back(currSeqNumber);
                }
                catch (boost::bad_lexical_cast)
                {
                    // TODO-SanKumar-1612: Log on error.
                }
            }
        }

        std::sort(fileSeqNumbers.begin(), fileSeqNumbers.end());
    }

    boost::filesystem::path LogCutter::GetCompletedLogFilePath(const boost::filesystem::path &logFilePath, uint64_t sequenceNumber)
    {
        boost::filesystem::path folderPath = logFilePath.parent_path();
        std::string stem = logFilePath.stem().string(), ext = logFilePath.extension().string();

        std::string completedFileName = stem + CompletedLogTag + boost::lexical_cast<std::string>(sequenceNumber);
        boost::filesystem::path compPath = folderPath / (completedFileName + ext);

        return compPath;
    }

    boost::filesystem::path LogCutter::GetCurrLogFilePath(const boost::filesystem::path &logFilePath, uint64_t sequenceNumber)
    {
        boost::filesystem::path folderPath = logFilePath.parent_path();
        std::string stem = logFilePath.stem().string(), ext = logFilePath.extension().string();

        std::string completedFileName = stem + CurrLogTag + boost::lexical_cast<std::string>(sequenceNumber);
        boost::filesystem::path currPath = folderPath / (completedFileName + ext);

        return currPath;
    }

    void LogCutter::Initialize(
        const boost::filesystem::path &logFilePath, int maxCompletedLogFiles, boost::chrono::seconds cutInterval, const std::set<std::string>& setCompFilesPaths)
    {
        boost::system::error_code errorCode;

        if (m_initialized)
            throw std::logic_error("LogCutter is already initialized.");

        m_logFilePath = logFilePath;
        m_maxCompletedLogFiles = maxCompletedLogFiles;
        m_cutInterval = cutInterval;

        vector<uint64_t> seqNums;
        vector<uint64_t> movedCompletedSeqNums;
        // TODO-SanKumar-1612: Since we trust on the state of the system for the sequence numbering,
        // say files 5 to 7 were deleted, this would start back from sequence number 5. But if there's a
        // log uploader, it would assumes the newly written log file is already processed file that missed
        // deletion, as it persists the state of its own. So, should we have a persisted logger state per process?
        ListNotCompletedLogFiles(m_logFilePath, ~0ULL, seqNums);

        bool completedFilesEnumerated = false;
        if (!seqNums.empty())
        {
            m_currSequenceNumber = seqNums.back();
        }
        else // If there are no curr files.
        {
            ListCompletedLogFiles(m_logFilePath, seqNums);

            if (seqNums.empty())
            {
                // List given set of completed files paths to take latest completed seq number
                for (std::set<std::string>::const_iterator itr = setCompFilesPaths.begin(); itr != setCompFilesPaths.end(); itr++)
                {
                    ListCompletedLogFiles(*itr, movedCompletedSeqNums);
                }

                if (movedCompletedSeqNums.empty())
                {
                    m_currSequenceNumber = 0;
                }
                else
                {
                    m_currSequenceNumber = movedCompletedSeqNums.back() + 1;
                }
            }
            else
            {
                m_currSequenceNumber = seqNums.back();
                if (seqNums.size() == m_maxCompletedLogFiles)
                {
                    m_currSequenceNumber++;
                }
                else if (seqNums.size() > m_maxCompletedLogFiles)
                {
                    // Instead of creating _curr file with new seq number rename last _completed to _curr.
                    // This way we never cross m_maxCompletedLogFiles
                    boost::filesystem::path currLogFilePath(GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber));
                    boost::filesystem::path completedLogFilePath(GetCompletedLogFilePath(m_logFilePath, m_currSequenceNumber));
                    boost::filesystem::rename(completedLogFilePath, currLogFilePath, errorCode);
                    if (0 != errorCode.value())
                    {
                        //DebugPrintf(SV_LOG_ERROR, "%s: failed to rename %s to %s with error %d\n", FUNCTION_NAME,
                            //currLogFilePath.string().c_str(), completedLogFilePath.string().c_str(), errorCode.value());

                        // Rere case - create _curr fle with new seq number.
                        m_currSequenceNumber++;
                    }
                }
            }

            completedFilesEnumerated = true;
        }

        if (!completedFilesEnumerated)
        {
            seqNums.clear();
            ListCompletedLogFiles(m_logFilePath, seqNums);
        }

        // At every start of the binary, create a new file, if there could be one more completed log file.
        // If curr file is of zero size continue using same file.
        if (seqNums.size() < m_maxCompletedLogFiles && movedCompletedSeqNums.empty())
        {
            boost::filesystem::path currLogFilePath(GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber));
            const uintmax_t size = boost::filesystem::file_size(currLogFilePath, errorCode);
            if (!errorCode && size)
            {
                m_currSequenceNumber++;
            }
        }

        // TODO-SanKumar-1612: Should we do this operation here? This method would be invoked at the
        // start of an agent process and we should complete as quickly as possible. Instead of this
        // cleanup operation here, we could let it be performed at the next log cut operation.
        CompletePreviousLogFiles();

        std::string currLogFileName = GetCurrLogFilePath(m_logFilePath, m_currSequenceNumber).string();
        
        m_log.SetLogFileName(currLogFileName.c_str());

        m_initialized = true;
    }

    void LogCutter::StartCutting()
    {
        if (m_startedThread)
            throw std::logic_error("LogCutter thread has already begun.");

        if (!m_initialized)
            throw std::logic_error("LogCutter is not yet initialized");

        if (m_maxCompletedLogFiles > 0)
        {
            m_cutterThread.reset(new boost::thread(boost::bind(&Logger::LogCutter::WaitCheckCut, this)));
            m_startedThread = true;
        }
    }

    void LogCutter::StopCutting()
    {
        try
        {
            SyncLogGuard_t guard(m_log.m_syncLog);
            
            m_log.CloseLogFile();
            
            CompleAllLogFiles();

            StopCutterThread();
        }
        catch (std::exception ex)
        {
            //DebugPrintf(SV_LOG_ERROR, "%s: caught exception %s.\n", FUNCTION_NAME, ex.what());
        }
        catch (...)
        {
            //DebugPrintf(SV_LOG_ERROR, "%s: caught unknown exception.\n");
        }
    }

    void LogCutter::StopCutterThread()
    {
        try
        {
            BOOST_ASSERT(!m_cutterThread == !m_startedThread);

            if (m_cutterThread)
            {
                BOOST_ASSERT(m_initialized);

                m_cutterThread->interrupt();
                m_cutterThread->join();

                m_cutterThread.reset();
            }
        }
        catch (std::exception ex)
        {
            //DebugPrintf(SV_LOG_ERROR, "%s: caught exception %s.\n", FUNCTION_NAME, ex.what());
        }
        catch (...)
        {
            // TODO-SanKumar-1612: Expand this for catching and logging all types of exceptions.
            //DebugPrintf(SV_LOG_ERROR, "%s: caught unknown exception while stopping the LoggerCutter thread.\n");
        }
    }

    LogCutter::~LogCutter()
    {
        StopCutterThread();
    }
}

#ifdef SV_UNIX

static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, const char* format, va_list args )
{
    return DebugPrintfV(LogLevel,-1,format,args);
}

static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, SV_LONG hr, const char* format, va_list args )
{
    std::vector<char> szDebugString(MAX_LOG_BUFFER_SIZE, 0);
    std::vector<char> szhr(80, 0);

    inm_vsnprintf_s(&szDebugString[0], szDebugString.size(), format, args);

    if( 0 == strlen( &szDebugString[0] ) )
    {
        return false;
    }

    if ( hr >=  0)
    {
        std::string hrformat("- hr =%d");
        inm_sprintf_s(&szhr[0], szhr.size() - 1, hrformat.c_str(), hr);
        inm_strncat_s(&szDebugString[0], szDebugString.size(), &szhr[0], szDebugString.size());
    }
  
    if ( (nRemoteLogLevel != SV_LOG_DISABLE) && (nRemoteLogLevel >= LogLevel) )
    {
        if(!CxServerIpAddress.empty())
            DebugPrintfToCx(CxServerIpAddress.c_str(), CxServerPort, LOGGER_URL, LogLevel, &szDebugString[0],isHttps);
    }

    // Do not log to stderr or stdout if we are running as deamon process

    if(!isDaemon)
    {    
        if ( (SV_LOG_ERROR == LogLevel) || (SV_LOG_SEVERE == LogLevel) || (SV_LOG_FATAL == LogLevel) )
        {
            std::cerr << &szDebugString[0] ;
        }
        
        if( SV_LOG_INFO == LogLevel )
        {
            std::cout << &szDebugString[0] ;
        }
    }

    if (nLogLevel != SV_LOG_DISABLE)
    {
        OutputDebugStringToLog( (int) LogLevel, &szDebugString[0] );
    }

    return true;
}
#endif

#ifdef SV_WINDOWS
///
/// Returns a descriptive string from an error code. Used for debugging purposes.
/// Return value must be freed with LocalFree()
///
HGLOBAL GetErrorTextFromScode( DWORD dwLastError )
{
    HMODULE hModule = NULL; // default to system source
    LPSTR MessageBuffer = NULL;
    DWORD dwBufferLength;

    DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM;

    //
    // TODO: use wininet.lib if in the right message range
    //
    //
    // Call FormatMessage() to allow for message 
    //  text to be acquired from the system 
    //  or from the supplied module handle.
    //
    dwBufferLength = FormatMessageA(
        dwFormatFlags,
        hModule, // module to get message from (NULL == system)
        dwLastError,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), // default language
        (LPSTR) &MessageBuffer,
        0,
        NULL
        );

    //
    // If we loaded a message source, unload it.
    //
    if(hModule != NULL)
    {
        FreeLibrary(hModule);
    }

    return( MessageBuffer );
}

HGLOBAL GetErrorTextFromHresult( HRESULT hr )
{
    return( GetErrorTextFromScode( SCODE_CODE( hr ) ) );
}

static void DebugPrintfToLocalHostLog(SV_LOG_LEVEL LogLevel, const std::string& szDebugString)
{
    SyncLogGuard_t guard(gLog.m_syncLog);
    string Agent_type;
    try
    {
        static SV_LONGLONG llSize = 0;
        if( llSize == 0 )
        {
            SVERROR rc;
        #ifdef SV_WINDOWS
            rc = SVGetFileSize(localhostlogfile.c_str(),&llSize);
        #elif SV_UNIX
            //INFO: The SVGetFileSize in unixagenthelpers and hostagenthelpers dont have the same signature. In *nix environments, the SVGetFileSize will cause recursion as the SVGetFileSize in svutils.cpp calls DebugPrintfs.
            struct stat file;
            if(!stat(localhostlogfile.c_str(),&file))
            {
                llSize = file.st_size;
            }
        #endif
        }
        
        if(llSize >= (SV_ULONG)nLogInitSize)
        {
            errorStream.close() ;
            errorStream.open(localhostlogfile.c_str(),ios::out | ios::trunc );
        }
        
        time_t ltime;
        struct tm *today;

        time( &ltime );
        today= localtime( &ltime );
        char present[30];
        memset(present,0,30);

        inm_sprintf_s(present, ARRAYSIZE(present), "(%02d-%02d-%02d %02d:%02d:%02d): ",
            today->tm_year - 100,
            today->tm_mon + 1,
            today->tm_mday,        
            today->tm_hour,
            today->tm_min,
            today->tm_sec
            );

        if(GetLoggerAgentType() == FxAgentLogger)
            Agent_type="FX";
        else
            Agent_type="VX";
        std::stringstream msg ;
        msg << present << " " << pchLogLevelMapping[LogLevel] << " "  << Agent_type << " " << szDebugString ;
        llSize += msg.str().length() ;     
        errorStream << msg.str() ;
        errorStream.flush() ;
    }
    catch(...) 
    {
        //DebugPrintf(SV_LOG_ERROR,"FAILED:DebugPrintfToLocalHostLog call failed.\n");
    }    
}
///
/// Helper function to take variable arguments to DebugPrintf()
///
static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, const char* format, va_list args )
{
    std::vector<char> szDebugString(MAX_LOG_BUFFER_SIZE, 0);        

    static bool LogOptionInitialized = false;

    static bool bStdError  = false;
    static bool bStdOutput = false;
    static bool bConsoleCheck = false;
    

    if (!bConsoleCheck && !isDaemon)
    {
        if ( GetStdHandle(STD_ERROR_HANDLE) )  
            bStdError   = true;
        if ( GetStdHandle(STD_OUTPUT_HANDLE) ) 
            bStdOutput  = true;
        bConsoleCheck = true;
    }           


    if( 0 == lstrlenA( format ) )
    {
        return( false );
    }

    _vsnprintf_s(&szDebugString[0], szDebugString.size(), szDebugString.size() - 1, format, args);

    if( 0 == lstrlenA( &szDebugString[0] ) )
    {
        return( false );
    }
    
    if ( (nRemoteLogLevel != SV_LOG_DISABLE) && (nRemoteLogLevel >= LogLevel) )
    {
        if( serializerType == PHPSerialize )
        {
            if(!CxServerIpAddress.empty())
            DebugPrintfToCx(CxServerIpAddress.c_str(), CxServerPort, LOGGER_URL, LogLevel, &szDebugString[0],isHttps);
        }
        else if( serializerType == Xmlize && localhostlogfile.length() > 0 )
        {
            DebugPrintfToLocalHostLog(LogLevel, &szDebugString[0]) ;
        }
    }    
    
    if (bStdError  && ( (SV_LOG_ERROR == LogLevel) || (SV_LOG_SEVERE == LogLevel) || (SV_LOG_FATAL == LogLevel) ) )         
    {
        cerr << &szDebugString[0] ;
    }
    else if( bStdOutput && (SV_LOG_INFO == LogLevel))
    {
        cout << &szDebugString[0] ;
    }

    if (nLogLevel != SV_LOG_DISABLE)
    {
        OutputDebugStringToLog((int)LogLevel, &szDebugString[0]);
    }

    return( true );
}

///
/// Helper function to take variable arguments to DebugPrintf(hr)
///
static bool DebugPrintfV( SV_LOG_LEVEL LogLevel, SV_LONG hr, const char* format, va_list args )
{
    bool shouldPrint = DebugPrintfV( LogLevel, format, args );

    if( shouldPrint )
    {
        HGLOBAL szError = GetErrorTextFromHresult( hr );
        if( NULL != szError )
        {
            OutputDebugStringToLog( (int) SV_LOG_ALWAYS, (char *)szError );
            LocalFree( szError );
        }
    }

    return( shouldPrint );
}
#ifdef SV_WINDOWS
//This part of code is applicable only for WINDOWS and is written specific to Scout Mail Recovery (SMR)
//Requirement: Backup debug log files if file size reaches nLogInitSize
//Number of debug log files to be backed up is configurable.
//TODO: make the function thread safe
//TODO: make the function generic to WINDOWS and LINUX
//
// Set the log backup when debug log file needs to be backed up if file size >= nLogInitSize
// 
void SetLogBackup(bool bLogBackup)
{
    bBackupDebugLog = bLogBackup;
}
//
// Set number of debug log files to backup
// 
void SetLogBackupCount(int logBackupCount)
{
    if(logBackupCount <= 0)
        maxBackupFileCount = 1;
    else
        maxBackupFileCount = logBackupCount;
}
//
// Backup debug log when file size >= nLogInitSize and reset svstream
//
void BackupAndResetStream()
{
    if(bBackupDebugLog)
    {
        gLog.m_logStream.close();
        errno_t err;
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];

        err = _splitpath_s( gLog.m_logFileName.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname,
                       _MAX_FNAME, ext, _MAX_EXT );

        if (err == 0)
        {
            szLogFileDir = drive;
            szLogFileDir.append(dir);
            szLogFileName = fname;
            szLogFileExt = ext;
            if (nextFileIndex >= maxBackupFileCount)
                nextFileIndex = 0;
            string backupFile = szLogFileDir;
            backupFile.append(szLogFileName);
            backupFile.append(ToString(nextFileIndex));
            backupFile.append(szLogFileExt);
            if(FileExist(backupFile))
            {
                DeleteFile(backupFile.c_str());
            }
            rename(gLog.m_logFileName.c_str(), backupFile.c_str());
            //InterlockedIncrement(&nextFileIndex);
            ++nextFileIndex;
        }    
        SetLogFileName(gLog.m_logFileName.c_str());
    }
}
#endif
#endif
