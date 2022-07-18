/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	Trace.cpp

Description	:   Trace class implementation

History		:   29-5-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "Trace.h"
#include "inmsafecapis.h"

#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/OS.h>
#include <sstream>
#include <vector>
#include <string>

namespace LogLevelTxtMsg
{
    const char Error[]   = "ERROR";
    const char Warning[] = "WARNING";
    const char Info[]    = "INFO";
    const char Trace[]   = "TRACE";
}


typedef ACE_Guard<ACE_Recursive_Thread_Mutex> LogAutoGuard;

Trace Trace::s_trace;

boost::function<void(LogLevel logLevel, const char *msg)> Trace::s_logCallback;

/*
Method      : Trace::Trace

Description : Trace destructor

Parameters  : None

Return      : None

*/
Trace::Trace()
:m_logLevel(LogLevelInfo)
{
}

/*
Method      : Trace::~Trace

Description : Trace destructor

Parameters  : None

Return      : None

*/
Trace::~Trace()
{
    if (TOut)
        TOut.close();
}

/*
Method      : Trace::SetLogFile

Description : Opens file steam for trace log

Parameters  : [in] log_file: Trace log file path

Return      : true if the file created/opened successfuly, otherwise false

*/
bool Trace::SetLogFile(const std::string& log_file)
{
    if (!log_file.empty())
        TOut.open(log_file.c_str(), std::ios_base::out | std::ios_base::app);

    return TOut.good();
}

/*
Method      : Trace::SetLogLevel

Description : Sets the trace log level

Parameters  : [in] logLevel: trace log level

Return      : None

*/
void Trace::SetLogLevel(LogLevel logLevel)
{
    ACE_ASSERT(logLevel <= LogLevelAlways);

    m_logLevel = logLevel > LogLevelAlways ? LogLevelAlways : logLevel;
}

/*
Method      : Trace::WriteToLog

Description : Writes the log statement to trace file and flushes the stream.

Parameters  : [in] msg: log message

Return      : None

*/
void Trace::WriteToLog(const std::string& msg)
{
    if (msg.empty())
        return;

    LogAutoGuard autogaurd(m_tout_mutex);

    if (TOut)
    {
        TOut << msg;

        TOut.flush();
    }
}

/*
Method      : Trace::GetFormatedMsg

Description : 

Parameters  : 

Return      : returns the formated message.

*/
std::string Trace::GetFormatedMsg(const char* MsgType,
                                  const char* format,
                                  va_list args)
{
    ACE_ASSERT(NULL != MsgType);
    ACE_ASSERT(NULL != format);

    std::stringstream msg;

    if (!s_logCallback)
    {
        //
        // Add time stamp
        //
        ACE_TCHAR timeStamp[64] = { 0 };
        if (ACE::timestamp(timeStamp, ARRAYSIZE(timeStamp)))
        {
            msg << ACE_TEXT_ALWAYS_CHAR(timeStamp);
            msg << " : ";
        }

        //
        // ProcessID & ThreadId
        //
        msg << ACE_OS::getpid();
        msg << " : ";
        msg << ACE_OS::thr_self();
        msg << " : ";

        //
        // Log level msg
        //
        msg << MsgType;
        msg << " : ";
    }

    try
    {
        //
        // Log msg
        //
        std::vector<char> frmtMsgBuff(FORMAT_MSG_BUFF_SIZE, 0);
        VSNPRINTF(&frmtMsgBuff[0], frmtMsgBuff.size(), frmtMsgBuff.size() -1, format, args);

        msg << &frmtMsgBuff[0];
    }
    catch (ContextualException ce)
    {
        msg << "Exception on formating trace msg. Exception: "
            << ce.what();
    }
    catch (...)
    {
        msg << "Unknown exception on formating trace msg.";
    }

    return msg.str();
}

/*
Method      : Trace::Init

Description : Initializes the trace logging system.

Parameters  : [in] log_file : trace log file path.
              [in, optional] logLevel: trace log level.

Return      : None

*/
void Trace::Init(const std::string& log_file,
                 LogLevel logLevel,
                 boost::function<void(LogLevel logLevel, const char *msg)> callback)
{
    LogAutoGuard autogaurd(s_trace.m_tout_mutex);

    s_trace.SetLogFile(log_file);
    s_trace.SetLogLevel(logLevel);
    
    if (callback)
        s_logCallback = callback;
    
}

/*
Method      : Trace::ResetLogLevel

Description : Resets the current log level to give log level.

Parameters  : [in] logLevel: new log level

Return      : None

*/
void Trace::ResetLogLevel(LogLevel logLevel)
{
    LogAutoGuard autogaurd(s_trace.m_tout_mutex);

    s_trace.SetLogLevel(logLevel > LogLevelAlways ? LogLevelAlways : logLevel);
}

/*
Method      : Trace::TraceMsg

Description : Logs trace/debug level statements

Parameters  : Variable lenght param list

Return      : None

*/
void Trace::TraceMsg(const char* format, ...)
{
    ACE_ASSERT(NULL != format);

    if (s_trace.m_logLevel < LogLevelTrace)
        return;

    if (0 == ACE_OS::strlen(format))
        return;

    va_list args;
    va_start(args, format);

    std::string msg = GetFormatedMsg(LogLevelTxtMsg::Trace, format, args);

    if (s_logCallback)
        s_logCallback(LogLevelAlways, msg.c_str());
    else
        s_trace.WriteToLog(msg);
    
    va_end(args);
}

/*
Method      : Trace::Warning

Description : Logs warning level trace statement

Parameters  : Variable lenght param list

Return      : None

*/
void Trace::Warning(const char* format, ...)
{
    ACE_ASSERT(NULL != format);

    if (s_trace.m_logLevel < LogLevelWarning)
        return;
    
    if (0 == ACE_OS::strlen(format))
        return;

    va_list args;
    va_start(args, format);

    std::string msg = GetFormatedMsg(LogLevelTxtMsg::Warning, format, args);

    if (s_logCallback)
        s_logCallback(LogLevelWarning, msg.c_str());
    else
        s_trace.WriteToLog(msg);

    va_end(args);
}

/*
Method      : Trace::Error

Description : Logs error level trace statement

Parameters  : Variable lenght param list

Return      : None

*/
void Trace::Error(const char* format, ...)
{
    ACE_ASSERT(NULL != format);

    if (0 == ACE_OS::strlen(format))
        return;

    va_list args;
    va_start(args, format);

    std::string msg = GetFormatedMsg(LogLevelTxtMsg::Error, format, args);
    
    if (s_logCallback)
        s_logCallback(LogLevelError, msg.c_str());
    else
        s_trace.WriteToLog(msg);

    va_end(args);
}

/*
Method      : Trace::Info

Description : Logs info level trace statement

Parameters  : variable lenght param list

Return      : None

*/
void Trace::Info(const char* format, ...)
{
    ACE_ASSERT(NULL != format);

    if (s_trace.m_logLevel < LogLevelInfo)
        return;

    if (0 == ACE_OS::strlen(format))
        return;

    va_list args;
    va_start(args, format);

    std::string msg = GetFormatedMsg(LogLevelTxtMsg::Info, format, args);

    if (s_logCallback)
        s_logCallback(LogLevelInfo, msg.c_str());
    else
        s_trace.WriteToLog(msg);

    va_end(args);
}


