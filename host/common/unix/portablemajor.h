#ifndef PORTABLEUNIX__H
#define PORTABLEUNIX__H

class CUnixDispatchLoop : public CDispatchLoop
{
public:
    //
    // Construction/destruction
    //
    CUnixDispatchLoop( SVERROR* pError );
    ~CUnixDispatchLoop() {}

public:
    //
    // Dispatcher interface
    //
    void initialize();
    void dispatch( SV_ULONG delay );
    bool shouldExit()                   { return( m_bQuitting ); } 

protected:
    bool m_bQuitting;
};




class CUnixProcess : public CProcess
{
public:
    //
    // constructors/destructors
    //
    CUnixProcess( char const* pszCommandLine, SVERROR* pErr,
            char const* pszStdoutLogfile = NULL, char const* pszDirName = NULL,
            SV_LOG_LEVEL logLevel = SV_LOG_DEBUG);
    ~CUnixProcess();

public:
    //
    // Public interface
    //
    bool hasExited();
    void waitForExit( SV_ULONG waitTime );
    SVERROR terminate();
    SVERROR getExitCode( SV_ULONG* pExitCode );
    SV_ULONG getId()        { return( m_childPID );  }

protected:
    //
    // Helper methods
    //
    SVERROR terminateWithForce();

    //
    // State
    //
    pid_t m_childPID;
    int m_uExitCode;
    bool m_alreadyExited;
    int     m_logLevel;

private:
    SVERROR CreateSVProcess( const char* pszCommandLine, char const* pszStdoutLogfile = NULL );	
};

#endif 
