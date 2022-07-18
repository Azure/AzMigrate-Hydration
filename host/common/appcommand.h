#ifndef __APPCOMMAND__H
#define __APPCOMMAND__H
#include "svtypes.h"
#include <boost/shared_ptr.hpp>
#include <string>

class AppCommand;
typedef boost::shared_ptr<AppCommand> AppCommandPtr;

class AppCommand
{
    // allow caller to set log level
    int m_logLevel;

    std::string m_cmd;
    SV_INT m_timeout;
    std::string m_outFile;
    bool m_bUnlink;
    bool m_bInheritHandles;
    bool m_bDonotLogCmd;
    void CollectCmdOutPut(std::string& output);
public:
    /// <summary>
    /// Starts a child process when command and arguments are passed in cmd
    ///  Deprecated: using in Windows, may fail if the binary path with spaces is not in quotes
    /// </summary>
    /// <param name="cmd"></param>
    /// <param name="timeout"></param>
    /// <param name="outfile"></param>
    /// <param name="inherithandles"></param>
    /// <param name="donotLogCmd"></param>
    AppCommand(const std::string& cmd, SV_UINT timeout, std::string outfile = "", bool inherithandles = true, bool donotLogCmd = false);

    /// <summary>
    /// Starts a child process when command is passed in cmd and arguments in args
    /// On Windows, adds quotes around cmd before creating child process
    /// </summary>
    /// <param name="cmd"></param>
    /// <param name="args"></param>
    /// <param name="timeout"></param>
    /// <param name="outfile"></param>
    /// <param name="inherithandles"></param>
    /// <param name="donotLogCmd"></param>
    AppCommand(const std::string& cmd, const std::string& args, SV_UINT timeout, std::string outfile="", bool inherithandles=true, bool donotLogCmd = false);

    SVERROR Run(SV_ULONG& exitCode, std::string& outPut, bool& bQuitRequested, const std::string& workindDir="", void* handle = NULL);
    void SetLogLevel(int logLevel);
};
#endif
