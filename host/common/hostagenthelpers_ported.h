//
// @file hostagenthelpers_ported.h
//
// This is a temporary file, representing the transition from a non-portable hostagenthelpers.h file to 
// a fully portable one. This file represents the portable pieces of hostagenthelpers. Once the whole
// hostagenthelpers.h and all of its dependencies have been ported, this file will be elided.
//
#ifndef HOSTAGENTHELPERS_PORTED_H
#define HOSTAGENTHELPERS_PORTED_H

#include <string.h>
#include <string>
#include <vector>
#include <time.h>       // time_t
#ifdef WIN32
#pragma warning( disable: 4244 )	// converting time_t to long isn't 64-bit safe
#pragma warning( disable: 4312 )	// converting void* to int isn't 64-bit safe
#endif
#include <ace/Time_Value.h>
#ifdef WIN32
#pragma warning( default: 4244 )
#pragma warning( default: 4312 )
#endif
#include "svtypes.h"
#include "portable.h"
#include "defines.h"    // TODO: remove registry key defines from this header

#include "portablehelpers.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "inmsafecapis.h"

#include <new>
const char INMAGE_JOB_LOG_FILENAME[]        = "inmage_job_";
const char INMAGE_KEYFILE_PATTERN[]			= ".inmage_key_";

typedef std::vector<std::string> svector_t;

int const WORKER_REINCARNATION_TIME = 180 * 1000;            // in milliseconds
int const CX_TIMEOUT                        = 600;

//
// Utility functions
//
#if !defined(ARRAYSIZE)
#define ARRAYSIZE( a ) ( sizeof( a ) / sizeof( a[ 0 ] ) )
#endif

template <typename T> inline void SAFE_DELETE( T*& p ) { if(p != NULL){delete p; p = NULL;} }
template <typename T> inline void SAFE_ARRAY_DELETE( T*& p ) { if(p !=NULL){ delete [] p; p = NULL; } }
#define SAFE_FCLOSE( p ) do { if( p ) { fclose( p ); p = NULL; } } while( 0 )

#define ACE_TIME_VALUE_AS_USEC(aceTimeToConvert) (((long long)(aceTimeToConvert.sec()) * 1000000) + aceTimeToConvert.usec())

template <typename T>
void ZeroFill( T& t ) { memset( &t, 0, sizeof( T ) ); }

template <int n, int initialCount = 0>
class CMeteredBool
{
public:
    CMeteredBool() : m_count( initialCount ) {}
    operator bool()   { m_count %= n; return( 0 == m_count++ ); }
    bool operator! ()  { m_count %= n; return( 0 != m_count++ ); }
protected:
    int m_count;
};

template <typename T> T HIGH8(T n) { return( ( n & 0xFF000000 ) >> 24 ); }
template <typename T> T HIGH16(T n) { return( ( n & 0xFFFF0000 ) >> 16 ); }
template <typename T> T LOW16(T n) { return( n & 0xFFFF ); }
template <typename T> T LOW8(T n) { return( n & 0xFF ); }

template <typename T> T SVMin(T a, T b) { return( a < b ? a : b ); }
template <typename T> T SVMax(T a, T b) { return( a < b ? b : a ); }
template <typename T> inline bool IsEmptyString( T const* psz ) { return( 0 == psz[ 0 ] ); }

char* PathRemoveAllBackslashesA( char* pszPath );
SV_WCHAR* PathRemoveAllBackslashesW( SV_WCHAR* pszPath );

#ifdef UNICODE
#define PathRemoveAllBackslashes(p) PathRemoveAllBackslashesW(p)
#else
#define PathRemoveAllBackslashes(p) PathRemoveAllBackslashesA(p)
#endif

extern char g_szEmptyString[];

class CStringAccumulator
{
public:
    CStringAccumulator()                { m_psz = NULL; m_cChars = 0; m_Capacity = 32*1024; m_CapacityNext = m_Capacity * 2; }
    SVERROR append( const char* psz )
    { 
        if( NULL == psz ) { return( SVS_OK ); }
        size_t length = strlen( psz );
        return( append( psz, length ) );
    }
    SVERROR append( const char* psz, size_t length )
    {
        if( NULL == psz ) { return( SVS_OK ); }
		 
        if( NULL == m_psz || length + m_cChars + 1 > m_Capacity )
        {
            char* pszOld = m_psz;
            size_t twicelen;
            INM_SAFE_ARITHMETIC(twicelen = 2*(InmSafeInt<size_t>::Type(length) + m_cChars + 1), INMAGE_EX(length)(m_cChars))
			size_t old = m_CapacityNext >  twicelen ? m_CapacityNext : twicelen;
            m_psz = new(std::nothrow) char[ old ];
            if( NULL == m_psz ) { m_psz = pszOld; return( SVE_OUTOFMEMORY ); }
            m_CapacityNext = old + m_Capacity;
            m_Capacity = old;            
            inm_memcpy_s( m_psz, old, pszOld, m_cChars );
            delete [] pszOld;
        }
		inm_memcpy_s(m_psz + m_cChars, (m_Capacity - m_cChars), psz, length);
        INM_SAFE_ARITHMETIC(m_cChars += InmSafeInt<size_t>::Type(length), INMAGE_EX(m_cChars)(length));
        m_psz[ m_cChars ] = 0;
        return( SVS_OK );
    }
    char* getString()                   { return( ( NULL == m_psz ) ? g_szEmptyString : m_psz ); }
    size_t getLength()                  { return( m_cChars ); }
    ~CStringAccumulator()               { delete [] m_psz; }
protected:
    CStringAccumulator( CStringAccumulator& );
    char* m_psz;
    size_t m_cChars;
    size_t m_Capacity;
    size_t m_CapacityNext;
};

class CSimpleChecksum
{
public:
    CSimpleChecksum() : m_Sum( 0 ) {}
    void add( SV_ULONG num )                    { m_Sum += num; }
    void add( char const* psz )             { if( NULL != psz ) { for(; *psz; psz++){ m_Sum += *psz; } } }
    SV_ULONG sum()     { return( m_Sum ); }
protected:
    SV_ULONG m_Sum;
};

struct SSH_Configuration {
	char sshd_prod_version [BUFSIZ];
	char sshd_prot_version [BUFSIZ];
	char sshd_port[6];
	char auth_keys_file [SV_MAX_PATH];
	char authorization_file [SV_MAX_PATH];
	char identity_file [SV_MAX_PATH];
	char ssh_client_path [SV_MAX_PATH];
	char ssh_keygen_path [SV_MAX_PATH];	
	char szUserName[SV_MAX_PATH];
	char szSedPath[SV_MAX_PATH];
};

struct SSHOPTIONS {
	enum { 
		SESSION_ENCRYPT=1<<0,
		KEY_TYPE_RSA_KEY=1<<1,
		KEY_TYPE_DSA_KEY=1<<2,
		CIPHER_TYPE_3DESCBC=1<<3,
		CIPHER_TYPE_AES128CBC=1<<4,
		CIPHER_TYPE_AES192CBC=1<<5,
		CIPHER_TYPE_AES256CBC=1<<6,
		CIPHER_TYPE_AES128CTR=1<<7,
		CIPHER_TYPE_AES192CTR=1<<8,
		CIPHER_TYPE_AES256CTR=1<<9,
		CIPHER_TYPE_ARCFOUR128=1<<10,
		CIPHER_TYPE_ARCFOUR256=1<<11,
		CIPHER_TYPE_ARCFOUR=1<<12,
		CIPHER_TYPE_BLOWFISHCBC=1<<13,
		CIPHER_TYPE_CAST128CBC=1<<14,
	};
	enum KEY_GEN_STATE { KEY_GEN_NOT_STARTED, 
							KEY_GEN_STARTED, 
							KEY_GEN_IN_PROGRESS, 
							KEY_GEN_COMPLETED, 
							KEYS_POSTED, 
							KEYS_RETRIEVED, 
							AUTHKEYS_FILE_UPDATED, 
							AUTHKEYS_DELETED 
						};
	int KeyGenState;
	SV_ULONG sshFlags;
	char sshdSrcVersion[SV_MAX_PATH];
	char sshdTrgVersion[SV_MAX_PATH];
	char sshSrcUser[SV_MAX_PATH];
	char sshTrgUser[SV_MAX_PATH];
	SV_INT sshdSrcPort;
	SV_INT sshdTrgPort;
	SV_ULONG START_Port;
	SV_ULONG END_Port;
	CProcess *tProcess;
	CProcess *kProcess;
    SV_ULONG localPort;
    SV_ULONG Port;
	char pubKey_SecSH[2048];
	char pubKey_OpenSSH[2048];
};
//
// Host agent configuration
//
struct SV_HOST_AGENT_PARAMS
{
    char szSVServerName[ SV_INTERNET_MAX_HOST_NAME_LENGTH ];
    char szThrottleBootstrapURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szThrottleSentinelURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szThrottleOutpostURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szThrottleURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szSVHostAgentType[ 256 ];
    char szSVHostAgentRegKey[ 256 ];
    char szSVRegisterURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szUnregisterURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szSVGetProtectedDrivesURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szSVGetTargetDrivesURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szReportResyncRequiredURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szUpdateStatusURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szCacheDirectory[ SV_MAX_PATH ];
    char szGetFileReplicationConfigurationURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szUpdateFileReplicationStatusURL[ SV_INTERNET_MAX_URL_LENGTH ];
    char szProtectedVolumeSettingsURL[ SV_INTERNET_MAX_URL_LENGTH ];
	char szInmsyncExe[ SV_MAX_PATH ];
    char szShellCommand[ SV_MAX_PATH*4 ];
    char szSVHostID[ 64 ];
    SV_ULONG AvailableDrives;
    SV_ULONG ProtectedDrives;
    SV_ULONG TargetDrives;
    SV_ULONG cbMaxSentinelPayload;
    
    enum FILE_TRANSPORT { FILE_TRANSPORT_FTP, FILE_TRANSPORT_CIFS };
    FILE_TRANSPORT FileTransport;

    bool ShouldResync;
    bool ThrottleSentinel;
	//Https parameter
	bool Https;

    char szSVFTPHost[ SV_INTERNET_MAX_HOST_NAME_LENGTH ];
    char szSVFTPUserName[ LENGTH_TRANSPORT_USER ];
    char szSVFTPPassword[ LENGTH_TRANSPORT_PASSWORD ];
    SV_ULONG SVFTPPort;
    char szSVGetFTPInfoURL[ SV_INTERNET_MAX_URL_LENGTH ];

    // Bugfix #23
    char szSVGetSecureModesInfoURL[ SV_INTERNET_MAX_URL_LENGTH ]; 
	char szSVSSLKeyPath[SV_INTERNET_MAX_PATH_LENGTH + 1];
	char szSVSSLCertificatePath[SV_INTERNET_MAX_PATH_LENGTH + 1];
    //char szSVSSLCertPassword[ LENGTH_TRANSPORT_PASSWORD ];
    //char szSVSSLKeyPassword[ LENGTH_TRANSPORT_PASSWORD ];
    //char szSVSSLCipherList[SV_INTERNET_MAX_HOST_NAME_LENGTH];


    //Ashish - auto fstag
    char szSVGetVisibleReadOnlyDrivesURL[ SV_INTERNET_MAX_URL_LENGTH + 1];
	char szSVGetVisibleReadWriteDrivesURL[ SV_INTERNET_MAX_URL_LENGTH + 1];
    char szSVShouldUpdateResyncDrivesURL[ SV_INTERNET_MAX_URL_LENGTH + 1];
    SV_ULONG VisibleReadOnlyDrives;
	SV_ULONG VisibleReadWriteDrives;

	SV_ULONG PendingDrivesToBeMadeReadOnly;
	SV_ULONG PendingDrivesToBeMadeReadWrite;
	SV_ULONG PendingDrivesToBeMadeVisible;
	SV_ULONG PendingDrivesToBeMadeInvisible;

    //Akshay - for configurable server HTTP port
    SV_ULONG ServerHttpPort;

    SV_ULONG EnableFrLogFileDeletion;
    SV_ULONG EnableFrDeleteOptions;
    SV_ULONG MaxFrLogPayload;
	
	SV_ULONG Fx_Upgraded;
    char szSendSignalExe[ SV_INTERNET_MAX_URL_LENGTH + 1 ];
    /* Needed for setting uid and gid values in daemon.conf file */
    char szUID[SV_INTERNET_MAX_HOST_NAME_LENGTH+1];
    char szGID[SV_INTERNET_MAX_HOST_NAME_LENGTH+1];

    char szRegisterClusterInfoURL[SV_INTERNET_MAX_URL_LENGTH];
    char szUpdateAgentLogURL[SV_INTERNET_MAX_URL_LENGTH];
    char szGetVolumeGroupSettingsUrl[ SV_INTERNET_MAX_URL_LENGTH ];
	char szUpdatePermissionStateUrl[ SV_INTERNET_MAX_URL_LENGTH ];

    SV_ULONG fastSyncMaxChunkSize;
    SV_ULONG fastSyncHashCompareDataSize;

    SV_ULONG maxRsyncThreads;

    char szResyncSourceDirectoryPath[ SV_INTERNET_MAX_URL_LENGTH ];

    unsigned long RsyncTimeOut;

	// For settings polling interval
	SV_ULONG settingsPollingInterval;
	// Agent's Install Path
	char AgentInstallPath[SV_MAX_PATH];
    SV_ULONG CxUpdateInterval;

    SV_ULONG minCacheFreeDiskSpacePercent;
    SV_ULONG minCacheFreeDiskSpace;
	struct SSH_Configuration ssh_config;
};

//
// Implementation functions
//
enum SV_HOST_AGENT_TYPE { SV_AGENT_UNKNOWN, SV_AGENT_BOOTSTRAP, SV_AGENT_SENTINEL, SV_AGENT_OUTPOST,  SV_AGENT_RESYNC, SV_AGENT_FILE_REPLICATION };
SVERROR ReadAgentConfiguration( SV_HOST_AGENT_PARAMS *pConfiguration, char const* pszConfigPathname, SV_HOST_AGENT_TYPE AgentType );


SVERROR SettingStandByCxValuesInRegistry(const char *pszServerName,int value);
//SVERROR SetStanbyCxValuesInConfigFile(const char *pszServerName,int port);

bool WriteKeyValueInConfigFile(const char *key, std::string value);
bool GetValuesInConfigFile(std::string configFile, const char *key, std::string& value );
bool SetValuesInConfigFile(const char *Key, std::string& newValue);


SVERROR RegisterFX( const char *pszRegKey,
                      const char *pszSVServerName,
                      SV_INT HttpPort,
                      const char *pszURL,
                      const char *installPath,
                      char *pszHostID,
                      SV_ULONG *pHostIDSize,bool https);

SVERROR UnregisterHost(char const * pszServerName,
                      SV_INT HttpPort,
                      char const * pszUrl,
                      char const * pszHostId,bool );

SVERROR GetFromSVServer( const char *pszSVServerName,
                         SV_INT HttpPort,
                         const char *pszGetURL,
                         char **ppszGetBuffer );


bool SVIsFilePathExists( char const* pszPathname );
// Returns SV_INVALID_FILE_SIZE if cannot get file size
//SV_ULONG SVGetFileSize( char const* pszPathname, OPTIONAL SV_ULONG* pHighSize );
//SVERROR SVGetFileSize( char const* pszPathname, SV_LONGLONG* pFileSize );

SVERROR GetConfigFileName(std::string &strConfFileName);

template <typename T>   // T should be unsigned
inline int LowestBitIndex( T value )
{
    int result = 0;

    if( value )
    {
        while( 0 == ( value & 1 ) )
        {
            result ++;
            value >>= 1;
        }
    }
    else
    {
        result = -1;
    }

    return( result );
}

const char* GetProductVersion();
const char* GetDriverVersion();
void GetKernelVersion(std::string& kernelVersion);

void GetOperatingSystemInfo(std::string& strInfo);
std::vector<std::string> split(const std::string &, const std::string &, const int numFields = 0);
int const THREAD_SAFE_ERROR_STRING_BUFFER_LENGTH = 80;
char* GetThreadsafeErrorStringBuffer();
extern char const SEND_SIGNAL_EXE_CYGWIN_SWITCH[];

//void HideBeforeApplyingSyncData(char * buffer);

SVERROR UncompressFile( char *pszFilename );
bool DeleteLocalFile(std::string const & name); // PORTING: needs to be implemented on Unix

enum CHANGEFSTAGOP { HIDE, UNHIDE_READONLY, UNHIDE_READWRITE, UNKNOWNOP , OPERATION_FAILED};
enum VOLUMEOP { LOCK_VOLUME, UNLOCK_VOLUME };
enum SVAGENTSINFO {  SERVICES_DOWN, SERVICES_UP };
enum NOTIFY_TYPE {  UPDATE_NOTIFY, REQUEST_NOTIFY };
enum VOLUME_OP_STATUS { NORMAL = 0, PENDING = 1, SUCCESS = 2 };


SVERROR generate_key_pair_impl (SV_ULONG , 
							SV_ULONG , 
							struct SSHOPTIONS &, 
							struct SSH_Configuration& , 
							const char* );								
SVERROR fetch_keys(SV_ULONG , 
							struct SSHOPTIONS &, 
							struct SSH_Configuration& , 
							const char* );								

SVERROR update_authorized_keys_impl(const char* szAuthKeys_file, 									
									const struct SSHOPTIONS&);

SVERROR update_authorization_file_impl(SV_ULONG,
									const char*,
									const struct SSHOPTIONS&,									
									const char*);
SVERROR convert_SecSH_OpenSSH(char *, 
							char* , 
							const char*, 
							SV_ULONG,
							SV_ULONG);
SVERROR cleanup_server_keys_impl(SSH_Configuration, const char*, SV_ULONG, SV_ULONG);
SVERROR cleanup_client_keys_impl(SSH_Configuration, const char*, SV_ULONG);

SVERROR create_ssh_tunnel_impl(const char* szTunnel, CProcess **, const char*, SV_ULONG*);
SVERROR delete_encryption_keys_impl( const char*, SV_ULONG );
SV_ULONG getFreePort(SV_ULONG startPort,SV_ULONG endPort);
SVERROR waitTillTunnelEstablishes(int localPort);
void PostFailOverJobStatusToCX(char* cxip, SV_ULONG httpPort, SV_ULONG processId, SV_ULONG jobId,bool https);
SVERROR SetFXRegStringValue(const char * pszValueName, const char * pszValue);

SVERROR GetFXRegStringValue(const char* pszValueName, std::string & strValue);
#ifdef SV_WINDOWS
SVERROR SetFXRegDWordValue(const char * pszValueName, DWORD pszValue);
#endif
#endif // HOSTAGENTHELPERS_PORTED__H
