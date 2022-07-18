//
// @file portable.h: defines OS portability layer for SV code
//
#ifndef PORTABLE__H
#define PORTABLE__H

#include "svtypes.h"
#include <string>
#include <cstring>

//
// Calling conventions
//
#ifndef WIN32
#define SV_CDECL
#else
#define SV_CDECL __cdecl
#endif

#ifdef SV_UNIX

#define stricmp(x,y) strcasecmp(x,y)

#define strcmpi strcasecmp
#define strnicmp strncasecmp  
#endif

#ifdef WIN32
#define NEW_LINE_CHARACTER "\r\n"
#else
#ifdef SV_UNIX
#define NEW_LINE_CHARACTER "\n"
#endif
#endif

//
// PR #5554
// permissions when obtaining a lock to be used by 
// cdplock class, service when locking the persistent store
// moved this macro from cdplock.cpp to portable.h as it is
// required in service code as well
//
#ifdef SV_WINDOWS
#define SV_LOCK_PERMISSIONS (FILE_SHARE_READ | FILE_SHARE_WRITE)
#else
#define SV_LOCK_PERMISSIONS ( S_IRWXU |S_IRWXG | S_IROTH | S_IXOTH)
#endif

struct svstrcmp
{
    bool operator()(const std::string & s1, const std::string & s2) const
        {
#ifdef SV_UNIX
            return strcmp(s1.c_str(), s2.c_str()) < 0;
#else
            return stricmp(s1.c_str(), s2.c_str()) < 0;
#endif
        }
};

//
// PR #5554
// when comparing volumes, we would compare them
//  windows:
//     based on volume guid
//  unix:
//     based on real path
// 
// volnamecmp operator is defined as comparision functor
//
struct volnamecmp
{
    bool operator()(const std::string & s1, const std::string & s2) const;
};

struct volequalitycmp
{
    bool operator()(const std::string & s1, const std::string & s2) const;
};

//
// Coding decoration
//
#ifndef OPTIONAL
#define OPTIONAL
#endif

//
// Constants
//
#if defined (SV_WINDOWS) && !defined (SV_USES_LONGPATHS)
int const SV_MAX_PATH                           = 260;
#else
// PR#10815: Long Path support
int const SV_MAX_PATH                           = 4096;
#endif

int const SV_INTERNET_MAX_HOST_NAME_LENGTH      = 256;
int const SV_INTERNET_MAX_USER_NAME_LENGTH      = 128;
int const SV_INTERNET_MAX_PASSWORD_LENGTH       = 128;
int const SV_INTERNET_MAX_PORT_NUMBER_LENGTH    = 5;
int const SV_INTERNET_MAX_PORT_NUMBER_VALUE     = 65535;
int const SV_INTERNET_MAX_PATH_LENGTH           = 2048;
int const SV_INTERNET_MAX_SCHEME_LENGTH         = 32;          // longest protocol name length
int const SV_INTERNET_MAX_URL_LENGTH            = ( SV_INTERNET_MAX_SCHEME_LENGTH 
                                                    + sizeof( "://" ) 
                                                    + SV_INTERNET_MAX_PATH_LENGTH );
int const SV_INVALID_FILE_SIZE                  = 0xFFFFFFFF;

#ifdef SV_FABRIC
int const SV_DEFAULT_TRANSPORT_CONNECTION_TIMEOUT = 45;
int const SV_DEFAULT_TRANSPORT_RESPONSE_TIMEOUT  = 30;
int const SV_DEFAULT_TRANSPORT_TOTALTRANSFER_TIMEOUT  = 900;
int const SV_DEFAULT_TRANSPORT_LOWSPEED_TIME = 10;
int const SV_DEFAULT_TRANSPORT_LOWSPEED_LIMIT = 1;
#else
int const SV_DEFAULT_TRANSPORT_CONNECTION_TIMEOUT = 0;
int const SV_DEFAULT_TRANSPORT_RESPONSE_TIMEOUT  = 300;
int const SV_DEFAULT_TRANSPORT_TOTALTRANSFER_TIMEOUT  = 900;
int const SV_DEFAULT_TRANSPORT_LOWSPEED_TIME = 120;
int const SV_DEFAULT_TRANSPORT_LOWSPEED_LIMIT = 1;
#endif

const char FST_SCTR_OFFSET_510_VALUE = (char) 0x55;
const char FST_SCTR_OFFSET_511_VALUE = (char) 0xAA;

int const BS_JMP_BOOT_VALUE_0xEB = 0xEB;
int const BS_JMP_BOOT_VALUE_0xE9 = 0xE9;

int const BS_JMP_BOOT_VALUE_0xEC = 0xEC;
int const BS_JMP_BOOT_VALUE_0xEA = 0xEA;

int const FST_SCTR_OFFSET_510 = 510;
int const FST_SCTR_OFFSET_511 = 511;

static const char *Agent_Mode_Source = "source";
static const char *Agent_Mode_Target = "target";
static const char *Agent_Mode_ReprotectTarget = "onpremtarget";
static const char *Vxlogs_Folder = "vxlogs";
static const char *Diffsync_Folder = "diffsync";
static const char *Resync_Folder = "resync";
static const char *Perf_Folder = "perf";
static const char *Application_Data_Folder = "Application Data";

static const char *EvtCollForw_CmdOpt_Environment = "/Environment";

static const char *EvtCollForw_Env_V2ASource = "V2A_Source";
static const char *EvtCollForw_Env_V2APS = "V2A_Process_Server";
static const char *EvtCollForw_Env_V2ARcmSource = "V2A_RCM_Source";
static const char *EvtCollForw_Env_V2ARcmSourceOnAzure = "V2A_RCM_SourceOnAzure";
static const char *EvtCollForw_Env_V2ARcmPS = "V2A_RCM_Process_Server";
static const char *EvtCollForw_Env_A2ASource = "A2A_Source";

static const SV_ULONGLONG HUNDRED_NANO_SECS_IN_SEC = 10 * 1000 * 1000;

//
// Debug utility functions
//
enum SV_LOG_LEVEL { SV_LOG_DISABLE, 
                    SV_LOG_FATAL, 
                    SV_LOG_SEVERE, 
                    SV_LOG_ERROR, 
                    SV_LOG_WARNING, 
                    SV_LOG_INFO, 
                    SV_LOG_DEBUG,                     
                    SV_LOG_ALWAYS,
                    SV_LOG_LEVEL_COUNT};


enum SV_ALERT_TYPE { SV_ALERT_CRITICAL = 1, 
                     SV_ALERT_ERROR,                     
                     SV_ALERT_SIMPLE,
                     SV_ALERT_CRITICAL_WITH_SUBJECT};

enum SV_ALERT_MODULE { SV_ALERT_MODULE_RETENTION = 1, 
                       SV_ALERT_MODULE_ROLLBACK,                     
                       SV_ALERT_MODULE_RECOVERY,
                       SV_ALERT_MODULE_SNAPSHOT,
                       SV_ALERT_MODULE_DIFFERENTIALSYNC,
                       SV_ALERT_MODULE_CLUSTER,
                       SV_ALERT_MODULE_RESYNC,
                       SV_ALERT_MODULE_HOST_REGISTRATION,
                       SV_ALERT_MODULE_DIFFS_DRAINING,
                       SV_ALERT_MODULE_CONSISTENCY,
                       SV_ALERT_MODULE_GENERAL};

enum Logger_Agent_Type
{
    VxAgentLogger,
    FxAgentLogger
};



void SetLoggerAgentType(Logger_Agent_Type);
Logger_Agent_Type GetLoggerAgentType();
void GetLoggingDetails();

void SV_CDECL DebugPrintf( const char* format, ... );
void SV_CDECL DebugPrintf( SV_LONG hr, const char* format, ... );
void SV_CDECL DebugPrintf( SV_LOG_LEVEL LogLevel, const char* format, ... );
void SV_CDECL DebugPrintf( SV_LOG_LEVEL LogLevel, SV_LONG hr, const char* format, ... );
/*Function for adding the Agent*/
void SV_CDECL DebugPrintf(char *,SV_LOG_LEVEL LogLevel, SV_LONG hr, const char* format, ...);

//
// OS objects
//

class CDispatchLoop
{
public:
    //
    // Construction/destruction
    //
    static SVERROR Create( CDispatchLoop** ppDispatcher );
    virtual ~CDispatchLoop() {}

public:
    //
    // Dispatcher interface
    //
    virtual void initialize()                   { m_pImpl->initialize(); }
    virtual void dispatch( SV_ULONG delay )     { m_pImpl->dispatch( delay ); }
    virtual bool shouldExit()                   { return( m_pImpl->shouldExit() ); }

protected:
    CDispatchLoop* m_pImpl;
};

class CProcess
{
public:
    //
    // Construction/destruction
    //
    static SVERROR Create( char const* pszCommandLine, CProcess** ppProcess,
                           char const* pszStdoutLogfile = 0, char const* pszDirName = 0, void* huserToken=NULL, 
                           bool binherithandles=true, SV_LOG_LEVEL logLevel=SV_LOG_DEBUG);
    virtual ~CProcess() { delete m_pImpl; }

public:
    //
    // Process interface
    //
    virtual bool hasExited()        { return( m_pImpl->hasExited() ); }
    virtual void waitForExit( SV_ULONG waitTime )   { return( m_pImpl->waitForExit( waitTime ) ); }
    virtual SVERROR terminate()     { return( m_pImpl->terminate() ); }
    virtual SVERROR getExitCode( SV_ULONG* pExitCode )   { return( m_pImpl->getExitCode( pExitCode ) ); }
    virtual SV_ULONG getId()        { return( m_pImpl->getId() ); }

protected:
    CProcess() : m_pImpl( 0 ) {}

protected:
    CProcess* m_pImpl;
};

#define DSKDIR "/dsk/"
#define RDSKDIR "/rdsk/"
#define DMPDIR "/dmp/"
#define RDMPDIR "/rdmp/"

#define DISKS_LAYOUT_FILE_TRANSFTER_STATUS_KEY "disks_layout_file_transfer_status"
#define STRFAILURE "failure"
#define STRSUCCESS "success"

#endif // PORTABLE__H
