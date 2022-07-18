//
// @file portablewin.cpp: implements sv OS abstractions for Windows
//
#include <windows.h>
#include <assert.h>
#include <string.h>
#include <atlbase.h>
#include <iostream>
#include <fstream>   // Added to support file io   
using namespace std; // Added to support file io
#include "hostagenthelpers.h"
#include "portablewin.h"
#include <curl/curl.h>
#include "transport_settings.h"
#include "globs.h"
#include "portablehelpersmajor.h"
#include "inmsafecapis.h"


inline void SAFE_CLOSESERVICEHANDLE( SC_HANDLE& h ) { if( NULL != h ) { ::CloseServiceHandle( h ); } }


SVERROR CProcess::Create( char const* pszCommandLine, CProcess** ppProcess,
        OPTIONAL char const* pszStdoutLogfile, OPTIONAL char const* pszDirName, HANDLE huserToken,
        bool binherithandles, SV_LOG_LEVEL logLevel)
{
    if( NULL == pszCommandLine || NULL == ppProcess )
    {
        return( SVE_INVALIDARG );
    }

    SVERROR rc;

    *ppProcess = new CWindowsProcess( pszCommandLine, &rc, pszStdoutLogfile, pszDirName,huserToken, binherithandles, logLevel);
    if( NULL == *ppProcess )
    {
        return( SVE_OUTOFMEMORY );
    }

    if( rc.failed() )
    {
        SAFE_DELETE( *ppProcess );
    }

    return( rc );
}

CWindowsProcess::CWindowsProcess( char const* pszCommandLine, SVERROR* pErr,
        OPTIONAL char const* pszStdoutLogfile,  OPTIONAL char const* pszDirName, HANDLE  hToken, bool binherithandles,
    SV_LOG_LEVEL logLevel):m_logLevel(logLevel)
{
    static BOOL bConsoleAllocated = FALSE;

    assert( NULL != pszCommandLine && NULL != pErr );

    m_pImpl = NULL;
    m_hLogFile = NULL;
    ZeroFill( m_ProcessInfo );
    m_dwExitCode = 0;
    m_jobControlHandle = NULL ;
    STARTUPINFO StartupInfo;
    ZeroFill( StartupInfo );
    StartupInfo.cb = sizeof( StartupInfo );
    m_jobControlHandle = CreateJobObject(NULL, NULL);
    if (m_jobControlHandle == NULL)
    {
        DWORD dwerr = GetLastError();
        DebugPrintf(SV_LOG_ERROR, "Failed to create WIN32 Job Object.. Error %d\n", dwerr);
    }
    else
    {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobObjExtendedLimitInfo = { 0 };
        jobObjExtendedLimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (0 == SetInformationJobObject(m_jobControlHandle, JobObjectExtendedLimitInformation, &jobObjExtendedLimitInfo, sizeof(jobObjExtendedLimitInfo)))
        {
            DebugPrintf(SV_LOG_ERROR, "SetInformationJobObject failed with error code %lu.\n", GetLastError());
        }
    }
    *pErr = SVS_OK;

    /*if( !bConsoleAllocated && !AllocConsole() )
    {
        *pErr = GetLastError();
        DebugPrintf( "FAILED Couldn't allocate console (thus job control won't work), err %d\n", pErr->error.ntstatus );
    }
    else
    {*/
        bConsoleAllocated = TRUE;

        //
        // Create a handle for logging stderr and stdout
        //
        if( NULL != pszStdoutLogfile )
        {
            StartupInfo.dwFlags = STARTF_USESTDHANDLES;
            SECURITY_ATTRIBUTES SecurityAttributes = { 0 };
            SecurityAttributes.bInheritHandle = TRUE;

            // PR#10815: Long Path support
            HANDLE hLogFile = SVCreateFile( pszStdoutLogfile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &SecurityAttributes, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
            if( IS_INVALID_HANDLE_VALUE( hLogFile ) )
            {
                *pErr = GetLastError();
            }
            else
            {
                if( INVALID_SET_FILE_POINTER == SetFilePointer( hLogFile, 0, NULL, FILE_END ) )
                {
                    *pErr = GetLastError();
                }
                else
                {
                    StartupInfo.hStdInput = INVALID_HANDLE_VALUE;
                    StartupInfo.hStdOutput = hLogFile;
                    StartupInfo.hStdError = hLogFile;
                    StartupInfo.wShowWindow = SW_HIDE;
                    m_hLogFile = hLogFile;
                }
            }
        }

        if( pErr->succeeded() )
        {
            std::string strCommandLine(pszCommandLine);
            bool bProcessCreated = false;
            if( hToken == NULL )
            {
                bProcessCreated = CreateProcess(NULL, &strCommandLine[0], NULL, NULL, (binherithandles ? TRUE : FALSE),
                    CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, pszDirName, &StartupInfo, &m_ProcessInfo);
                if (!bProcessCreated)
                {
                    *pErr = GetLastError();
                    DebugPrintf(SV_LOG_ERROR, "%s: CreateProcess Failed for %s. Error %ld\n", FUNCTION_NAME, strCommandLine.c_str(), pErr->error.ntstatus);
                }
            }
            else
            {
                bProcessCreated = CreateProcessAsUser(hToken, NULL, &strCommandLine[0], NULL, NULL, (binherithandles ? TRUE : FALSE),
                    CREATE_NEW_PROCESS_GROUP | CREATE_SUSPENDED | CREATE_NO_WINDOW, NULL, pszDirName, &StartupInfo, &m_ProcessInfo);
                if (!bProcessCreated)
                {
                    *pErr = GetLastError();
                    DebugPrintf(SV_LOG_ERROR, "%s: CreateProcessAsUser Failed for %s. Error %ld\n", FUNCTION_NAME, strCommandLine.c_str(), pErr->error.ntstatus);
                }
            }
            if (bProcessCreated && m_jobControlHandle != NULL)
            {
                if (AssignProcessToJobObject(m_jobControlHandle, m_ProcessInfo.hProcess) == 0)
                {
                    *pErr = GetLastError();
                    DebugPrintf(SV_LOG_ERROR, "%s: Failed to Assign process to job object.. Error %ld\n", FUNCTION_NAME, pErr->error.ntstatus);
                }
                ResumeThread(m_ProcessInfo.hThread);
                SAFE_CLOSEHANDLE(m_ProcessInfo.hThread);
            }
        }
}

CWindowsProcess::~CWindowsProcess()
{
    SAFE_CLOSEHANDLE( m_ProcessInfo.hProcess );	
    SAFE_CLOSEHANDLE( m_hLogFile );
    SAFE_CLOSEHANDLE(m_jobControlHandle);
}

bool CWindowsProcess::hasExited()
{
    if( NULL == m_ProcessInfo.hProcess )
    {
        return( false );
    }

    DWORD dwWait = WaitForSingleObject( m_ProcessInfo.hProcess, 0 );
    if( WAIT_OBJECT_0 == dwWait )
    {
        return( true );
    }
    return( false );
}

void CWindowsProcess::waitForExit( SV_ULONG waitTime )
{
    if( NULL == m_ProcessInfo.hProcess )
    {
        return;
    }

    DWORD dwWait = WaitForSingleObject( m_ProcessInfo.hProcess, waitTime );
}

SVERROR CWindowsProcess::terminate()
{
    SV_UINT exitCode = -1 ;
    if( hasExited() )
    {
        return( SVS_FALSE );        
    }

    SVERROR rc;
    /*
    if( !GenerateConsoleCtrlEvent( CTRL_BREAK_EVENT, m_ProcessInfo.dwProcessId ) )
    {
        DebugPrintf( "WARNING Calling TerminateProcess() on process %d (Ctrl-Break failed)\n", m_ProcessInfo.dwProcessId );

        if( !TerminateProcess( m_ProcessInfo.hProcess, 0xFFFFFFFF ) )
        {
            rc = GetLastError();
        }
    }
    */
    if (TerminateJobObject(m_jobControlHandle, exitCode) == 0)
    {
        rc = GetLastError();
    }
    return( rc );
}

SVERROR CWindowsProcess::getExitCode( SV_ULONG* pExitCode )
{
    SVERROR rc;
    if( NULL == pExitCode )
    {
        return( SVE_INVALIDARG );
    }

    if( !GetExitCodeProcess( m_ProcessInfo.hProcess, &m_dwExitCode ) )
    {
        rc = GetLastError();
    }
    else
    {
        *pExitCode = m_dwExitCode;
    }
    return( rc );
}
SVERROR CDispatchLoop::Create( CDispatchLoop** ppDispatcher )
{
    SVERROR rc;
    CDispatchLoop* pDispatcher = new CWindowsDispatchLoop( &rc );
    if( NULL == pDispatcher )
    {
        return( SVE_OUTOFMEMORY );
    }
    if( rc.failed() )
    {
        SAFE_DELETE( pDispatcher );
    }
    else
    {
        *ppDispatcher = pDispatcher;
    }

    return( rc );
}

CWindowsDispatchLoop::CWindowsDispatchLoop( SVERROR* pError )
{
    assert( NULL != pError );
    m_bQuitting = false;
    *pError = SVS_OK;
}

void CWindowsDispatchLoop::initialize()
{
    MSG msg;
    PeekMessage( &msg, NULL, WM_USER, WM_USER, PM_NOREMOVE );
}

void CWindowsDispatchLoop::dispatch( SV_ULONG delay )
{
    while( TRUE )
    {
        MSG msg;
        while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            if( WM_QUIT == msg.message )
            {
                DebugPrintf( "WM_QUIT received. Exiting...\n" );
                m_bQuitting = true;
                break;
            }
            else
            {
                DispatchMessage( &msg );
            }
        }

        if( m_bQuitting )
        {
            break;
        }

        DWORD dwWait = MsgWaitForMultipleObjects( 0, NULL, FALSE, delay, QS_ALLINPUT );

        if( WAIT_TIMEOUT == dwWait )
        {
            break;
        }
    }
}