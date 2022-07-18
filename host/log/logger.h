#ifndef LOGGER__H
#define LOGGER__H

#include "portable.h"
#include "serializationtype.h"

bool GetLogLevelName(SV_LOG_LEVEL logLevel, std::string& name);
bool GetLogLevel(const std::string &logLevelName, SV_LOG_LEVEL &logLevel);

void CloseDebug();
void CloseDebugThreadSpecific();
void SetLogLevel(int level);
void SetLogFileName(const char* fileName);
void SetThreadSpecificLogFileName(const char* fileName);
void SetLogInitSize(int size);
void DebugPrintf(int nLogLevel, const char* format, ...);
void DebugPrintf(const char* format, ...);

void SetLogHttpIpAddress(const char* ipAddress);
void SetLogHttpPort(int port);
void SetLogHostId(const char* hostId);
void SetLogRemoteLogLevel(int remoteLogLevel);
void SetLogHttpsOption(const bool& https);
void SetDaemonMode();
void SetDaemonMode(bool);
void SetLocalHostLogFile(const char* fileName);
void SetSerializeType(ESERIALIZE_TYPE serializer);

#ifdef SV_WINDOWS
//This part of code is applicable only for WINDOWS and is written specific to Scout Mail Recovery (SMR)
//Requirement: Backup debug log files if file size reaches nLogInitSize
//Number of debug log files to be backed up is configurable.
//TODO: make the function thread safe
//TODO: make the function generic to WINDOWS and LINUX
void SetLogBackup(bool bLogBackup);
void SetLogBackupCount(int logBackupCount);
#endif

#define INM_TRACE_ENTER_FUNC DebugPrintf(SV_LOG_DEBUG, "Entered %s\n", FUNCTION_NAME);
#define INM_TRACE_EXIT_FUNC   DebugPrintf(SV_LOG_DEBUG, "Leaving %s\n", FUNCTION_NAME);

#endif
