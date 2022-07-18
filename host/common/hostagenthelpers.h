///
/// @file hostagenthelpers.h
/// Functions common to agent service and sentinel.
///
#ifndef HOSTAGENTHELPERS__H
#define HOSTAGENTHELPERS__H

#ifdef SV_WINDOWS
#include <windows.h>
#include <imagehlp.h>
#include <wininet.h>
#include <atlbase.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
//#include "SV.h"
#include "defines.h"
#include "svdparse.h"
#include "hostagenthelpers_ported.h"
#include "portablehelpers.h"

#include <ace/os_include/os_fcntl.h>
#include <ace/OS.h>
#include <ace/config-lite.h>

#define SV_FTP_LOGIN          "svsystems"
#define SV_FTP_PASSWORD       "svsHillview"
//Ashish - 05/01/04 - FTP default value
#define SV_FTP_PORT           INTERNET_DEFAULT_FTP_PORT
#define SV_FTP_URL            "/get_ftp_config.php"

//Bugfix #23
#define SV_SECURE_MODES_URL   "/get_vx_secure_modes.php" 
#define SV_SSL_DEFAULT_SSLKEYPATH "transport\\newreq.pem"
#define SV_SSL_DEFAULT_SSLCERTPATH "transport\\newcert.pem"

extern char const * const NTFS_FILE_SYSTEM_TYPE;
extern char const * const FAT_FILE_SYSTEM_TYPE;



// Bugfix for bug#253
typedef struct _SV_FileList
{
    ACE_TEXT_WIN32_FIND_DATA win32_find_data;
    char                 *fileName;
    time_t                mtime;
    SV_LONGLONG           size;
    bool                  fCopied;
    bool                  fPatched;
    SV_LONGLONG           BytesWritten;  //used to send status to the SV box
    SV_LONGLONG           VolumePosition;  //used to send status to the SV box
    struct _SV_FileList   *pPrev;
    struct _SV_FileList   *pNext;
} SV_FileList;


inline void SAFE_INTERNETCLOSEHANDLE( ACE_HANDLE& h )
{
    if( NULL != h )
    {
        InternetCloseHandle( h );
    }
    h = NULL;
}

#ifndef MAX_DRIVES
int const MAX_DRIVES        = 26;
#endif

//
// Utility functions
//
void FillMemoryPattern( BYTE* pBuffer, int cbBuffer, DWORD dwPattern );

HRESULT GetHostId( char* pszValue, DWORD cch );

bool GetProtectedVolumeSettingsFromSVServer(char const *,
                                       SV_INT ,
                                       char const *,
                                       char const * ,
                                       char ** );

HRESULT GetProtectedDrivesFromSVServer( char const * pszSVServerName,
                                        SV_INT HttpPort,
                                        char const * pszGetProtectedDrivesURL,
                                        char const * pszHostID,
                                        DWORD *pdwProtectedDrives );

HRESULT UpdateProtectedDrivesList( char *pszHostAgentRegKey, DWORD dwProtectedDrives );
HRESULT UpdateTargetDrivesList( char *pszHostAgentRegKey, DWORD dwTargetDrives );

BOOL ProtectedDriveMaskChanged( char *pszHostAgentRegKey, DWORD dwCurrentProtectedDriveMask );

HRESULT OnDirtyBlockData( FILE* file,
                       HANDLE handleVolume,
                       SV_FileList *pCurSV_FileList, //Used for transmitting status to the SV box
                       SV_ULONG sv_ulongBlockCount);                       

BOOL BufferIsZero( char *pszBuffer, SV_ULONG sv_ulongLength );

HRESULT OnHeader( SVD_HEADER1 const* header, unsigned int size );

HRESULT ProcessAndApplyDeltaFile( char const* filename,
                               HANDLE handleVolume,
                               SV_FileList *pCurSV_FileList); //Used for transmitting status to the SV box
                               

HRESULT ApplyDifference( HANDLE handleVolume,
                         char *pchDeltaBuffer,
                         SV_LONGLONG sv_ulongLength,
                         SV_LONGLONG sv_large_integerByteOffset);                         

HRESULT DeleteCopyFileList( /* in */ SV_FileList *pSV_FileList );

HRESULT CopyVolumeChunkToLocalFile( HANDLE handleVolume,
                               char *pchFilename,
                               SV_LONGLONG sv_longlongLength,
                               SV_LONGLONG sv_longlongByteOffset,
                               SV_LONGLONG *psv_longlongBytesRead );

HRESULT CopyVolumeChunkToRemoteFile( HANDLE handleVolume,
                                     char *pszSVServerName,
                                     char *pszTransportUser,
                                     char *pszTransportPassword,
                                     SV_ULONG TransportPort,
                                     char *pszRemoteDirectory,
                                     char *pszRemoteFile,
                                     SV_LONGLONG sv_longlongLength,
                                     SV_LONGLONG sv_longlongByteOffset,
                                     SV_LONGLONG *psv_longlongBytesRead );

HRESULT MoveFileToRemoteDirectory( char *pchSrcFilename,
                                   char *pchDestFilename,
                                   char *pszFilename,
                                   char *pszSVServerName,
                                   SV_INT HttpPort,
                                   char *pszHostId,
                                   char *pszVolumeName,
                                   ULARGE_INTEGER cbCurrentCheckpoint,
                                   ULARGE_INTEGER *cbActualCheckpoint,
                                   SV_ULONG TransportPort,
                                   char *pszTransportUser,
                                   char *pszTransportPassword);

HRESULT SVFtpCreateDirectory( HINTERNET hSession, LPCSTR pszDirectory );

HRESULT SVInternetWriteFile( HINTERNET hFile, PVOID pData, DWORD dwSize, DWORD* pcbWritten );

HRESULT GetInitialSyncVolumeCheckPoint( DWORD *pdwOffsetLow,
                                        DWORD *pdwOffsetHigh,
                                        const char* pszDriveLetter
                                        );

HRESULT SetInitialSyncVolumeCheckPoint( DWORD dwOffsetLow,
                                        DWORD dwOffsetHigh,
                                        const char* pszDriveLetter 
                                        );

HRESULT GetVolumeChunkSizeFromRegistry( DWORD *pdwVolumeChunkSize );

HRESULT GetMaxResyncThreadsFromRegistry( DWORD *pdwMaxResyncThreads );

HRESULT GetMaxOutpostThreadsFromRegistry( DWORD *pdwMaxOutpostThreads );

HRESULT SVCreateFTPSession( HINTERNET *phinternetSession,
                            HINTERNET *phinternet,
                            const char *pszFTPServerName,
                            const char *pszFTPLogin,
                            const char *pszFTPPassword,
                            SV_ULONG ipTransportPort);

HRESULT SVDeleteFTPSession( HINTERNET *phinternetSession,
                            HINTERNET *phinternet );

//
// Host agent functions
//
HRESULT GetAgentParamsFromRegistry( const char *pszSVRootKey,
                                    SV_HOST_AGENT_PARAMS *pSV_HOST_AGENT_PARAMS, SV_HOST_AGENT_TYPE AgentType = SV_AGENT_UNKNOWN );

HRESULT GetFTPParamsFromRegistry( const char *pszSVRootKey,
                                    SV_HOST_AGENT_PARAMS *pSV_HOST_AGENT_PARAMS );

// used by fast sync to tell sever how much data matched vs. how muched had to be transfered
#define SYNC_DATA_MATCHED  true
#define SYNC_DATA_NO_MATCH false

HRESULT GetVolumeCheckpointFromSVServer( char const * pszSVServerName,
                                        SV_INT HttpPort,
                                        char const * pszGetVolumeCheckpointURL,
                                        char const * pszHostID,
                                        char const * pchSrcVolume,
                                        ULARGE_INTEGER* cbTotalSize,
                                        std::string& jobId);

// Common utility functions
HRESULT SVCleanupDirectory( const char *pszDirectory, const char *pszFilePattern );
HRESULT GetBytesAppliedThreshold( DWORD *pdwBytesAppliedThreshold, char const *pszDestVolume );
HRESULT GetSyncBytesAppliedThreshold( DWORD *pdwBytesAppliedThreshold, char const *pszDestVolume );
// HACK: temp hack to simulate volume groups
HRESULT GetFilesAppliedThreshold( DWORD *pdwAppliedThreshold, char const *pszDestVolume );
HRESULT GetTimestampAppliedThreshold( DWORD *pdwAppliedThreshold, char const *pszDestVolume );

SVERROR DeleteRegistrySettings(SV_HOST_AGENT_TYPE AgentType);
SVERROR DeleteStaleRegistrySettings(SV_HOST_AGENT_TYPE AgentType);

SVERROR GetPhysicalIpAndSubnetMasksFromRegistry(std::string interface_guid,
								 std::vector<std::string> & ips,
								 std::vector<std::string> & subnetmasks);
LONG GetAllPhysicalIPsOfSystem(std::vector<std::string> & physicalIPs);

bool GetInmsyncTimeoutFromRegistry(unsigned long & timeOut);

template <typename T>
inline BOOL inClosedInterval( T value, T low, T high )  { return( ( low <= value ) && ( value <= high ) ); }

template <typename T>
inline T* AlignPointer( T* p, unsigned PageSize = PAGE_SIZE )
{
    assert( 0 != PageSize );
    assert( NULL != p );

    SV_ULONGLONG advance = (SV_ULONGLONG) p % PageSize;
    return( p + ( PageSize - advance ) % PageSize );
}

//
// Utility classes
//
class CAutoCS
{
public:
    CAutoCS( CRITICAL_SECTION& cs ) { m_pcs = &cs; EnterCriticalSection( m_pcs ); }
    ~CAutoCS() { LeaveCriticalSection( m_pcs ); }
private:
    CRITICAL_SECTION* m_pcs;
};

class AutoInitWinsock
{
public:
    AutoInitWinsock( SCODE* pStatus = NULL );
    ~AutoInitWinsock();
};

bool GetTargetReplicationJobId(char const * server,
                               SV_INT httpPort,                               
                               char const * hostId,
                               char const * vol,                                        
                               std::string& jobId);

HRESULT GetTargetRollbackVolumes( const char *pszSVServerName,
								  SV_INT HttpPort,
								  const char *pszGetTargetRollbackVolumesURL,
								  const char *pszHostID,
								  char **pszGetBuff );

#define AGENT_IDLE_TIME_IN_SECONDS 10
void AgentIdle(int waitTimeSeconds);

bool GetSslPaths(char const * rootKey, std::string& sslKeyPath, std::string& sslCertPath);

#endif // HOSTAGENTHELPERS__H
