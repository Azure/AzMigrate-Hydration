/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	Trace.h

Description	:   Trace class.

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_TRACE_H
#define AZURE_RECOVERY_TRACE_H

#include <ace/Task.h>

#include <fstream>
#include <string>
#include <cstdio>
#include <cstdarg>

#include <boost/bind.hpp>
#include <boost/function.hpp>

#define FORMAT_MSG_BUFF_SIZE	256*1024

#ifdef WIN32
#define VSNPRINTF(buf, size, maxCount, format, args) _vsnprintf_s(buf, size, maxCount, format, args)
#else
#define VSNPRINTF(buf, size, maxCount, format, args) inm_vsnprintf_s(buf, size, format, args)
#endif

#define TRACE_LOG_FILE_NAME	"AzureRecovery.log"
typedef enum _logLevel_
{
    LogLevelDisable,
    LogLevelFatal,
    LogLevelSevere,
    LogLevelError,
    LogLevelWarning,
    LogLevelInfo,
    LogLevelTrace,
    LogLevelAlways
} LogLevel;

class Trace
{
public:

    static void Init( const std::string& log_file,
                      LogLevel logLevel = LogLevelTrace,
                      boost::function<void(LogLevel logLevel,const char *msg)> callback = 0);

	static void ResetLogLevel(LogLevel logLevel = LogLevelInfo);

    static void TraceMsg(const char* format, ...);

    static void Warning(const char* format, ...);

    static void Error(const char* format, ...);

    static void Info(const char* format, ...);

private:
    static std::string GetFormatedMsg( const char* MsgType, 
                                       const char* format,
                                       va_list args);

    Trace();
    ~Trace();

    bool SetLogFile(const std::string& log_file);
    void SetLogLevel(LogLevel logLevel);
    void WriteToLog(const std::string& msg);

    static Trace s_trace;

    ACE_Recursive_Thread_Mutex m_tout_mutex;
    std::ofstream TOut;
    LogLevel m_logLevel;

    /// \brief a callback if the log need to be forwarded to another module
    static boost::function<void(LogLevel loglevel, const char *msg)> s_logCallback;
};

#define TRACE_ERROR      Trace::Error

#define TRACE_WARNING    Trace::Warning

#define TRACE_INFO       Trace::Info

#define TRACE            Trace::TraceMsg

#define TRACE_FUNC_BEGIN TRACE("Entered into %s\n", __FUNCTION__)

#define TRACE_FUNC_END   TRACE("Exited from %s\n", __FUNCTION__)

#endif //~AZURE_RECOVERY_TRACE_H