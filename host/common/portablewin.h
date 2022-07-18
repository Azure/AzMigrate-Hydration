//
// @file portablewin.h
//
#ifndef PORTABLEWIN__H
#define PORTABLEWIN__H

#include <windows.h>
#include "portable.h"

class CWindowsDispatchLoop : public CDispatchLoop
{
public:
    //
    // Construction/destruction
    //
    CWindowsDispatchLoop( SVERROR* pError );
    virtual ~CWindowsDispatchLoop() {}

public:
    //
    // Dispatcher interface
    //
    virtual void initialize();
    virtual void dispatch( SV_ULONG delay );
    virtual bool shouldExit()                   { return( m_bQuitting ); } 

protected:
    bool m_bQuitting;
};

class CWindowsProcess : public CProcess
{
public:
    //
    // constructors/destructors
    //
    CWindowsProcess( char const* pszCommandLine, SVERROR* pErr,
            char const* pszStdoutLogfile = NULL, char const* pszDirName = NULL, HANDLE  hToken=NULL, bool binherithandles=true,
            SV_LOG_LEVEL logLevel = SV_LOG_DEBUG);
    virtual ~CWindowsProcess();

public:
    //
    // Public interface
    //
    virtual bool hasExited();
    virtual void waitForExit( SV_ULONG waitTime );
    virtual SVERROR terminate();
    virtual SVERROR getExitCode( SV_ULONG* pExitCode );
    virtual SV_ULONG getId()        { return( m_ProcessInfo.dwProcessId ); }

protected:
    //
    // State
    //
    HANDLE m_jobControlHandle;
    PROCESS_INFORMATION m_ProcessInfo;
    DWORD m_dwExitCode;
    HANDLE m_hLogFile;
    SV_LOG_LEVEL     m_logLevel;
};

#endif // PORTABLEWIN__H
