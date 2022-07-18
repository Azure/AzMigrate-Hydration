///
/// @file autoFS.h
/// Define helper functions to automate ChangeFSTag functionality
///
#ifndef AUTOFS__H
#define AUTOFS__H

#ifdef SV_WINDOWS
#include <wininet.h>
#endif

#include "svtypes.h"
#include "portablehelpers.h"

#define SV_UPDATE_SHOULD_RESYNC_URL "UpdateShouldResyncURL"
#define SV_GET_VISIBLE_READONLY_DRIVES_URL   "GetVisibleReadOnlyDrivesURL"
#define SV_GET_VISIBLE_READWRITE_DRIVES_URL   "GetVisibleReadWriteDrivesURL"
#define SV_VISIBLE_READONLY_DRIVES           TEXT("VisibleReadOnlyDrives")
#define SV_VISIBLE_READWRITE_DRIVES           TEXT("VisibleReadWriteDrives")
#define SV_HIDDEN_DRIVES					 TEXT("HiddenDrives")

#define DEFAULT_SV_UPDATE_SHOULD_RESYNC_URL TEXT("/report_resync_required_oa.php")
#define DEFAULT_SV_GET_VISIBLE_READONLY_DRIVES_URL   TEXT("/get_visible_readonly_drives.php")
#define DEFAULT_SV_GET_VISIBLE_READWRITE_DRIVES_URL   TEXT("/get_visible_readwrite_drives.php")


enum HIDE_STATE { HIDE_ADD, HIDE_REMOVE };

#ifdef SV_WINDOWS
typedef struct TAG_SV_AUTO_FS_PARAMETERS
{
    char szGetVisibleReadOnlyDrivesURL[ INTERNET_MAX_URL_LENGTH + 1 ];
    char szGetVisibleReadWriteDrivesURL[ INTERNET_MAX_URL_LENGTH + 1 ];
    char szUpdateShouldResyncDrive[ INTERNET_MAX_URL_LENGTH + 1 ];  
    DWORD dwVisibleReadOnlyDrives;
    DWORD dwVisibleReadWriteDrives;
} SV_AUTO_FS_PARAMETERS;
#endif

HRESULT ClearVisibleRegistrySettingsForNonTargets( const char *pszHostAgentRegKey, DWORD targetDrives );

HRESULT GetVisibleReadOnlyDrivesFromSVServer(char const * pszSVServerName,
                                             SV_INT HttpPort,
                                             char const * pszGetVisibleReadOnlyDrivesURL,
                                             char const * pszHostID,
                                             DWORD *pdwVisibleReadOnlyDrives );

HRESULT GetVisibleReadWriteDrivesFromSVServer(char const * pszSVServerName,
                                              SV_INT HttpPort,
                                              char const * pszGetVisibleReadWriteDrivesURL,
                                              char const * pszHostID,
                                              DWORD *pdwVisibleReadWriteDrives );

HRESULT GetVisibleReadOnlyDrivesList( const char *pszHostAgentRegKey, DWORD *dwVisibleReadOnlyDrives );
HRESULT GetVisibleReadWriteDrivesList( const char *pszHostAgentRegKey, DWORD *dwVisibleReadWriteDrives );
HRESULT UpdateVisibleReadOnlyDrivesList( const char *pszHostAgentRegKey, DWORD dwVisibleReadOnlyDrives );
HRESULT UpdateVisibleReadWriteDrivesList( const char *pszHostAgentRegKey, DWORD dwVisibleReadWriteDrives );

SVERROR HideDrives(DWORD dwHideDrives, DWORD & dwSuccessDrives, DWORD & dwFailedDrives);


HRESULT GetAutoFSInfoFromRegistry( const char *pszSVRootKey, SV_AUTO_FS_PARAMETERS *pszGetAutoFSDrivesURL );

VOLUME_STATE GetVolumeState(const char * drive);

HRESULT GetAutoFSInfoFromRegistry( const char *pszSVRootKey, SV_AUTO_FS_PARAMETERS *pszGetAutoFSDrivesURL );
void LockSemaphoreObject(const char *SemName, HANDLE & SemObject);
void UnLockSemaphoreObject(HANDLE SemObject);
HANDLE OpenVirtualVolumeDevice();
bool IsVirtualVolume(TCHAR*);


#endif
