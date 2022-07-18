#ifndef PORTABLEHELPERS__MAJORPORT__H_
#define PORTABLEHELPERS__MAJORPORT__H_

#include <diskguid.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <string>
#include <map>
#include "../config/volumegroupsettings.h"
#include "portablehelpers.h"
#include <boost/algorithm/string.hpp>

//typedef struct _Bpb {
//    unsigned char  BS_jmpBoot[3];  /* Jump instruction to the boot code */
//    unsigned char  BS_OEMName[8];  /* The name of the system that formatted the volume */
//    unsigned short BPB_BytsPerSec; /* How many bytes in a sector (should be 512) */
//    unsigned char  BPB_SecPerClus; /* How many sectors are in a cluster (FAT-12 should be 1) */
//    unsigned short BPB_RsvdSecCnt; /* Number of sectors that are reserved (FAT-12 should be 1) */
//    unsigned char  BPB_NumFATs;    /* The number of FAT tables on the disk (should be 2) */
//    unsigned short BPB_RootEntCnt; /* Maximum number of directory entries in the root directory */
//    unsigned short BPB_TotSec16;   /* FAT-12 total number of sectors on the disk */
//    unsigned char  BPB_Media;      /* Code for media type {fixed, removable, etc.} */
//    unsigned short BPB_FATSz16;    /* FAT-12 number of sectors that each FAT table occupies (should be 9) */
//    unsigned short BPB_SecPerTrk;  /* Number of sectors in one cylindrical track */
//    unsigned short BPB_NumHeads;   /* Number of heads for this volume (2 heads on a 1.4Mb 3.5 inch floppy) */
//    unsigned long  BPB_HiddSec;    /* Number of preceding hidden sectors (0 for non-partitioned media) */
//    unsigned long  BPB_TotSec32;   /* FAT-32 number of sectors on the disk (0 for FAT-12) */
//    unsigned char  BS_DrvNum;      /* A drive number for the media (OS specific) */
//    unsigned char  BS_Reserved1;   /* Reserved space for Windows NT (when formatting, set to 0) */
//    unsigned char  BS_BootSig;     /* Indicates that the following three fields are present (0x29) */
//    unsigned long  BS_VolID;       /* Volume serial number (for tracking this disk) */
//    unsigned char  BS_VolLab[11];  /* Volume label (matches label in the root directory, or "NO NAME    ") */
//    unsigned char  BS_FilSysType[8];/* Deceptive FAT type Label that may or may not indicate the FAT type */
//} Bpb;

#define INM_SERVICE_FAILURE_COUNT_RESET_PERIOD_SEC			3600
#define INM_SERVICE_FIRST_FAILURE_RESTART_DELAY_MSEC		300000
#define INM_SERVICE_SECOND_FAILURE_RESTART_DELAY_MSEC		600000

#define DOS_NAME_PREFIX     "\\\\?\\"
#define DISK_NAME_PREFIX    "\\\\.\\PhysicalDrive"

inline bool IS_INVALID_HANDLE_VALUE( ACE_HANDLE h )
{
   if ( ACE_INVALID_HANDLE == h )
    return true;

   return ( NULL == h );
}

#define DETACHED_DRIVE_DATA_FILE        "detacheddrives.dat"

inline void SAFE_CLOSEHANDLE( ACE_HANDLE& h )
{
    if( !IS_INVALID_HANDLE_VALUE( h ) )
    {
        ACE_OS::close( h );
    }
    h = NULL;
}

#define PATCH_FILE	"\\patch.log"

#define INM_POSIX_FADV_NORMAL 0
#define INM_POSIX_FADV_SEQUENTIAL 0 
#define INM_POSIX_FADV_RANDOM 0
#define INM_POSIX_FADV_NOREUSE 0
#define INM_POSIX_FADV_WILLNEED 0 
#define INM_POSIX_FADV_DONTNEED 0

#define INM_VACP_SUBPATH    "vacp.exe"

void HideBeforeApplyingSyncData(char * buffer,int size);

/************************************************************/
SVERROR GetFileSystemTypeForVolumeUtf8(const char *szVolume, int & fstype, bool & hidden);
SVERROR GetFileSystemTypeForVolume(const char *szVolume, int & fstype, bool & hidden);
SVERROR GetFileSystemType(const char *buffer, int & fstype, bool & hidden);
SVERROR HideFileSystem(char * buffer,int size);
SVERROR UnHideFileSystem(char * buffer,int size);
/************************************************************/

bool MountPointToGuid(const char* volume, void* guid, int guidLen);
bool DriveLetterToGUID(char DriveLetter, void *VolumeGUID, SV_ULONG ulBufLen);
bool DriveLetterToGuid(char drive, void* guid, int guidLen);

bool IsVolumeNameInGuidOrGlobalGuidFormat(const std::string& sVolume);
bool IsVolumeNameInGlobalGuidFormat(const std::string& sVolume);
bool containsMountedVolumes( const std::string& volumes, std::string& mountedvolumes );
/* if a container of strings is provided,
 * also fill the page file full path name(s)
 * into it */
DWORD getpagefilevol(svector_t *pPageFileNames = 0);
SV_ULONGLONG getrawvolumesizeUtf8(const std::string& sVolume);
SV_ULONGLONG getrawvolumesize(const std::string& sVolume);
bool GetDiskSizeByName (const char* physicalDriveName, SV_ULONGLONG &diskSize);
void GetDiskAttributes(const std::string &diskname, std::string &storagetype, VolumeSummary::FormatLabel &fmt,
    VolumeSummary::Vendor &volumegroupvendor, std::string &volumegroupname, std::string &diskGuid);

// All these wrapper functions are added for PR#10815: Long Path support
BOOL SVGetVolumePathName(LPCTSTR lpszFileName, LPTSTR lpszVolumePathName, const DWORD volumePathNameLen);
BOOL SVGetVolumeInformationUtf8(
                LPCTSTR lpRootPathName,
                std::string& strVolumeName,
                LPDWORD lpVolumeSerialNumber,
                LPDWORD lpMaximumComponentLength,
                LPDWORD lpFileSystemFlags,
                std::string& strFileSystemName
);
BOOL SVGetVolumeInformation(
                LPCTSTR lpRootPathName,
                LPTSTR lpVolumeNameBuffer,
                const DWORD nVolumeNameSize,
                LPDWORD lpVolumeSerialNumber,
                LPDWORD lpMaximumComponentLength,
                LPDWORD lpFileSystemFlags,
                LPTSTR lpFileSystemNameBuffer,
                const DWORD nFileSystemNameSize
);
BOOL SVGetDiskFreeSpaceExUtf8(
                LPCTSTR lpDirectoryName,
                PULARGE_INTEGER lpFreeBytesAvailable,
                PULARGE_INTEGER lpTotalNumberOfBytes,
                PULARGE_INTEGER lpTotalNumberOfFreeBytes
);
BOOL SVGetDiskFreeSpaceEx(
                LPCTSTR lpDirectoryName,
                PULARGE_INTEGER lpFreeBytesAvailable,
                PULARGE_INTEGER lpTotalNumberOfBytes,
                PULARGE_INTEGER lpTotalNumberOfFreeBytes
);
BOOL SVGetDiskFreeSpaceUtf8(
                LPCTSTR lpRootPathName,
                LPDWORD lpSectorsPerCluster,
                LPDWORD lpBytesPerSector,
                LPDWORD lpNumberOfFreeClusters,
                LPDWORD lpTotalNumberOfClusters
);
BOOL SVGetDiskFreeSpace(
                LPCTSTR lpRootPathName,
                LPDWORD lpSectorsPerCluster,
                LPDWORD lpBytesPerSector,
                LPDWORD lpNumberOfFreeClusters,
                LPDWORD lpTotalNumberOfClusters
);
DWORD SVGetCompressedFileSize(LPCTSTR lpFileName, LPDWORD lpFileSizeHigh);
SC_HANDLE SVOpenService(SC_HANDLE hSCManager, LPCTSTR lpServiceName, DWORD dwDesiredAccess);
BOOL SVCopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists);
HANDLE SVCreateFileUtf8(
                LPCTSTR lpFileName,
                DWORD dwDesiredAccess,
                DWORD dwShareMode,
                LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                DWORD dwCreationDisposition,
                DWORD dwFlagsAndAttributes,
                HANDLE hTemplateFile
);
HANDLE SVCreateFile(
                LPCTSTR lpFileName,
                DWORD dwDesiredAccess,
                DWORD dwShareMode,
                LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                DWORD dwCreationDisposition,
                DWORD dwFlagsAndAttributes,
                HANDLE hTemplateFile
);
bool GetWMIOutput(const std::string &wminame, std::string &output, std::string &error, int &ecode);
bool EnableDisableCompresson(const std::string & filename,DWORD flags, SV_USHORT compression_format);
bool DisableCompressonOnDirectory(const std::string & filename);
bool DisableCompressonOnFile(const std::string & filename);
bool EnableCompressonOnDirectory(const std::string & filename);
bool EnableCompressonOnFile(const std::string & filename);
void mountBootableVolumesIfRequried();

bool ReattachDeviceNameToGuid(std::string deviceName,std::string sGuid);

std::map<std::string,std::string> GetDetachedVolumeEntries();
bool UpdateDetachedVolumeEntries(std::map<std::string,std::string> &);

bool PersistDetachedVolumeEntry(std::string mountpt, std::string guid);
bool RemoveAttachedVolumeEntry(std::string mountpt, std::string guid);
#define WMICOMPUTERSYSTEM       "Win32_ComputerSystem"
#define DISKNAMEPREFIX "\\\\.\\PHYSICALDRIVE"
#define WMIBIOS                 "Win32_BIOS" 
#define WMBASEBOARD             "Win32_BaseBoard"
#define FOPEN_MODE_NOINHERIT "N"
#define INM_BOOT_VOLUME_MOUNTPOINT "C:\\SRV\\"
#define WMICOMPUTERSYSTEMPRODUCT	"Win32_ComputerSystemProduct"
#define WMISYSTEMENCLOSURE	"Win32_SystemEnclosure"


struct InstalledProduct
{
    std::string pkgname ;
    std::string name ;
    std::string version ;
    std::string vendor ;
    std::string installLocation ;
public:
    bool operator==(const InstalledProduct& rhs) const
    {
        return (boost::iequals(pkgname, rhs.pkgname) &&
            boost::iequals(name, rhs.name) &&
            boost::iequals(version, rhs.version) &&
            boost::iequals(vendor, rhs.vendor) &&
            boost::iequals(installLocation, rhs.installLocation));
    }
} ;

int CompareProudctVersion(const InstalledProduct& first, const InstalledProduct& second);

SVERROR GetInMageInstalledProductsFromRegistry(
    const std::string& productKeyName,
    std::list<InstalledProduct> &installedProducts);

bool GetInstalledPatches(std::list<std::string>& hotfixList) ;
bool GetInstalledProducts(std::list<InstalledProduct>& installedProds);
void DumpProductDetails(std::list<InstalledProduct>& installedProds);
bool GetMarsProductDetails(std::list<InstalledProduct>& marsProduct);
void UpdateBackupAgentHeartBeatTime() ;
long GetAgentHeartBeatTime() ;
void UpdateAgentRepositoryAccess(int error, const std::string& errmsg) ;
void GetAgentRepositoryAccess(int& accessError, SV_LONG& lastSuccessfulTs, std::string& errmsg) ;
void UpdateRepositoryAccess() ;
long GetRepositoryAccessTime() ;
void UpdateBookMarkSuccessTimeStamp() ;
long GetBookMarkSuccessTimeStamp() ;
unsigned int retrieveBusType( std::string volume );
std::string GetServiceStopReason() ;
void UpdateServiceStopReason(std::string& reason) ;
std::string ConvertDWordToString(const DWORD &dw);
std::string convertWstringToUtf8(const std::wstring &wstr);
std::wstring convertUtf8ToWstring(const std::string &str);

/*
Description: SetServiceFailureActions()

    Sets the service recovery options as per the provided input parameters.
    The service handle must be valid service handle and it should have SERVICE_CHANGE_CONFIG access.

    Refer ChangeServiceConfig2() API documentation in msdn for more details.

Return code:
    On success ERROR_SUCCESS will be returns, otherwise a win32 error code.
*/
DWORD SetServiceFailureActions(
    SC_HANDLE hService,
    DWORD resetPeriodSec,
    DWORD cActions,
    LPSC_ACTION lpActions,
    LPTSTR szRebootMsg,
    LPTSTR szCommand);

DWORD SetServiceFailureActionsRecomended(SC_HANDLE hService);

bool RebootMachine();

bool GetDiskGuid(ULONG ulDiskIndex, std::string& deviceId);

/// \brief For a given handle h, returns the disk ID
///
/// \return 
/// \li \c MBR signature / GPT GUID: success
/// \li \c empty string            : failure
std::string GetDiskGuid(const HANDLE &h); ///< handle to disk

/// \brief For a given device, returns the disk SCSI Id
///   devicename should be dosDeviceName (\\.\PhysicalDrive<n>)
/// 
/// \return 
/// \li \c scsi id                 : success
/// \li \c empty string            : failure
std::string GetDiskScsiId(const std::string &diskname); ///< handle to disk

/*
Method: 
     GetVolumeDiskExtents()

Description:
     Retrieve the list of disk extents of a given volName. Each extent will have below information:
     disk_id => Signature/Guid of the disk where the extent exist.
     offset  => Starting offset of the disk extent in the disk.
     length  => Lenght of the disk extent in bytes.

Parameters:
     [in]  volName - volume/volume-root-path/mount-point to which the extents to be retrieved.
     [out] extents - vector of disk extents which are representing as given volume(volName).

Return Code:
    ERROR_SUCCESS - On success.
    Win32 Error code - on failure.
*/
DWORD GetVolumeDiskExtents(const std::string& volName, disk_extents_t& extents);

/// \brief For a given handle h, returns size
///
/// \return
/// \li \c non zero size: success
/// \li \c 0            : failure
SV_ULONGLONG GetDiskSize(const HANDLE &h,    ///< handle to disk
                         std::string &errMsg ///< error message in case of failure
                         );

SVERROR OpenVolumeExtendedUtf8(ACE_HANDLE *pHandleVolume, const char   *pch_VolumeName, int  OpenMode);
bool IsVolumeLockedUtf8(const char *pszVolume, int fsTypeSize = 0, char * fsType = NULL);

#ifdef SV_USES_LONGPATHS
/// \brief For a given UTF8 path string, returns long path in wide char
///
/// \return
/// long path in wide char
std::wstring getLongPathNameUtf8(const char* path);
#endif /* SV_WINDOWS */

bool FormatVolumeNameToGuidUtf8(std::string&);

bool sameVolumeCheckUtf8(const std::string & vol1, const std::string & vol2);

void GetAgentVersionFromReg(std::string& prod_ver, const std::string& hiveRoot);

void GetNewHostIdFromReg(std::string& newHostId);

void SetNewHostIdInReg(const std::string& newHostId, const std::string& hiveRoot);

void GetRecoveredEnv(std::string& recoveredEnd);

void SetRecoveredEnv(const std::string& recoveredEnd, const std::string& hiveRoot);

void SetRecoveryCompleted();

void SetRecoveryInProgress(const std::string& hiveRoot);

void SetEnableRdpFlagInReg(const std::string& hiveRoot, DWORD dwValue);

void ResetEnableRdpFlagInReg();

bool GetEnableRdpFlagFromReg();

extern BOOL GetDeviceAttributes(const HANDLE& hDevice,
    const std::string & diskname,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage);

extern BOOL GetDeviceAttributes(ULONG ulDiskIndex,
    std::string& vendorId,
    std::string& productId,
    std::string& serialNumber,
    std::string& errormessage);

/// \brief For a given disk diskName
/// \returns true if able to obtain cluster disk attribute, false otherwise
///
/// \bClusterd flag             : true if disk is clustered else false
/// \errMsg                     : failure reason if any failure encountered
bool IsDiskClustered(const std::string& diskName, bool& bClustered, std::string& errMsg);

#endif /* PORTABLEHELPERS__MAJORPORT__H_ */
