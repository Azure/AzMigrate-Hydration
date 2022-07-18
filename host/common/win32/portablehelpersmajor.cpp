#include "portablehelpers.h"
#include "portablehelpersmajor.h"


#include "extendedlengthpath.h"
#include "portablehelperscommonheaders.h"
#include "configwrapper.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "devicestream.h"
#include "scopeguard.h"
#include "createpaths.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <wchar.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <iostream>
#include <windows.h>
#include <VersionHelpers.h>
#include <winioctl.h>
#include <sddl.h>
#include <atlbase.h>
#include <psapi.h>
#include "hostagenthelpers.h"
#include "autoFS.h"
#include "vvdevcontrol.h"
#include "defines.h"
#include "Tlhelp32.h"	
#include "genericwmi.h"
#include "wmiusers.h"
#include <ace/OS_NS_errno.h>
#include <ace/os_include/os_limits.h>
#include <ace/OS_NS_sys_utsname.h>
#include <iostream>
#include <fstream>
#include <map>
#include <boost/shared_array.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/chrono.hpp>

#include "sharedalignedbuffer.h"
#include <vss.h>
#include "dlvdshelper.h"
#include "deviceidinformer.h"
#include "registry.h"

using namespace boost::chrono;

ACE_Recursive_Thread_Mutex dirCreateMutex;

ACE_Recursive_Thread_Mutex g_PersistDetachedVolumeEntries;

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL(WINAPI *LPFN_WOW64DISABLEWOW64FSREDIRECTION) (PVOID*);
typedef BOOL(WINAPI *LPFN_WOW64REVERTWOW64FSREDIRECTION) (PVOID);
LPFN_ISWOW64PROCESS fnIsWow64Process;
LPFN_WOW64DISABLEWOW64FSREDIRECTION fnWow64DisableWow64FsRedirection;
LPFN_WOW64REVERTWOW64FSREDIRECTION fnWow64RevertWow64FsRedirection;

void PrintDriveLayoutInfoEx(const DRIVE_LAYOUT_INFORMATION_EX *pdli, const std::string &diskname);
void PrintDriveLayoutInfoMbr(const DRIVE_LAYOUT_INFORMATION_MBR &mbr);
void PrintDriveLayoutInfoGpt(const DRIVE_LAYOUT_INFORMATION_GPT &gpt);
void PrintPartitionInfoEx(const PARTITION_INFORMATION_EX &p);
void PrintGUID(const GUID &guid);
void PrintPartitionInfoMbr(const PARTITION_INFORMATION_MBR &mbr);
void PrintPartitionInfoGpt(const PARTITION_INFORMATION_GPT &gpt);

#include "..\common\win32\DiskHelpers.cpp"

/*
* FUNCTION NAME :  sameVolumecheck
*
* DESCRIPTION : checks if the specified volumes are same
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
bool sameVolumeCheckUtf8(const std::string & vol1, const std::string & vol2)
{
    std::string s1 = vol1;
    std::string s2 = vol2;

    // Convert the volume name to a standard format
    FormatVolumeName(s1);
    FormatVolumeNameToGuidUtf8(s1);
    ExtractGuidFromVolumeName(s1);

    // convert to standard device name if symbolic link
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s1);
    }

    // Convert the volume name to a standard format
    FormatVolumeName(s2);
    FormatVolumeNameToGuidUtf8(s2);
    ExtractGuidFromVolumeName(s2);

    // convert to standard device name if symbolic link
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s2);
    }
    return (s1 == s2);
}

/* if a container of strings is provided,
* also fill the page file full path name(s)
* into it */
DWORD getpagefilevol(svector_t *pPageFileNames)
{
    HKEY hKey;
    DWORD pageDrives = 0;
    char * pageInfo = NULL;
    DWORD pageInfoLength = 0;
    const int PAGEFILE_START_IN_EXISTING_KEY = 4;

    do
    {
        long status = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management", 0, KEY_QUERY_VALUE, &hKey);

        if (ERROR_SUCCESS != status) {
            DebugPrintf(SV_LOG_ERROR, "Failed to get the pagefile info from registry %d\n", status);
            break;
        }

        // #15838 - Check if "ExistingPageFiles" entry is present in the registry. If so get the drive letter from this entry.  
        // The ExistingPageFiles entry will be present in the registry for version Windows 2007 and greater.
        if (RegQueryValueEx(hKey, "ExistingPageFiles", NULL, NULL, NULL, &pageInfoLength) == ERROR_SUCCESS)
        {
            pageInfo = (char *)calloc(pageInfoLength, 1);
            status = RegQueryValueEx(hKey, "ExistingPageFiles", NULL, NULL,
                reinterpret_cast<BYTE*>(pageInfo), &pageInfoLength);

            if (ERROR_SUCCESS != status) {
                DebugPrintf(SV_LOG_ERROR, "Failed to get the pagefile info from registry %d\n", status);
                break;
            }

            std::string pagefileDrive;
            unsigned long size = 0;
            while (size < pageInfoLength)
            {
                pagefileDrive.clear();
                pagefileDrive = (pageInfo + size);
                size += (pagefileDrive.size() + 1);
                if (pagefileDrive.empty())
                    continue;
                std::stringstream sspg(pagefileDrive.c_str() + PAGEFILE_START_IN_EXISTING_KEY);
                if (pPageFileNames)
                {
                    std::string path;
                    sspg >> path;
                    pPageFileNames->push_back(path);
                }
                DWORD dwSystemDrives = 0;
                dwSystemDrives = 1 << (toupper(pagefileDrive[PAGEFILE_START_IN_EXISTING_KEY]) - 'A');
                pageDrives |= dwSystemDrives;
            }

        }
        else
        {

            status = RegQueryValueEx(hKey, "PagingFiles", NULL, NULL, NULL, &pageInfoLength);

            if (ERROR_SUCCESS != status) {
                DebugPrintf(SV_LOG_ERROR, "Failed to get the pagefile length from registry %d\n", status);
                break;
            }

            pageInfo = (char *)calloc(pageInfoLength, 1);
            status = RegQueryValueEx(hKey, "PagingFiles", NULL, NULL,
                reinterpret_cast<BYTE*>(pageInfo), &pageInfoLength);

            if (ERROR_SUCCESS != status) {
                DebugPrintf(SV_LOG_ERROR, "Failed to get the pagefile info from registry %d\n", status);
                break;
            }

            std::string pagefileDrive;
            unsigned long size = 0;
            while (size < pageInfoLength)
            {
                pagefileDrive.clear();
                pagefileDrive = (pageInfo + size);
                size += (pagefileDrive.size() + 1);
                if (pagefileDrive.empty())
                    continue;
                std::stringstream sspg(pagefileDrive);
                if (pPageFileNames)
                {
                    std::string path;
                    sspg >> path;
                    pPageFileNames->push_back(path);
                }
                DWORD dwSystemDrives = 0;
                dwSystemDrives = 1 << (toupper(pagefileDrive[0]) - 'A');
                pageDrives |= dwSystemDrives;
            }
        }
    } while (false);
    RegCloseKey(hKey);
    if (pageInfo)
    {
        free(pageInfo);
    }

    return pageDrives;
}


/// \brief Get disk size of given physical drive
/// Opens disk handle and issues ioctl to get disk size
/// @param[in]     physicalDriveName name of Physical drive; Expects in format: \\.\PHYSICALDRIVE0
/// @param[out]    diskSize disk size returned by ioctl
/// @return True if ioctl succeeded, false otherwise

bool GetDiskSizeByName(const char* physicalDriveName, SV_ULONGLONG &diskSize)
{
    bool bFlag = true;
    GET_LENGTH_INFORMATION dInfo;
    DWORD dInfoSize = sizeof(GET_LENGTH_INFORMATION);
    DWORD dwValue = 0;
    do
    {
        // Try to get a handle to PhysicalDrive IOCTL, report failure and exit if can't.
        // PR#10815: Long Path support
        HANDLE hPhysicalDriveIOCTL = SVCreateFile(physicalDriveName,
            GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
        {
            DebugPrintf(SV_LOG_ERROR,
                "GetDiskSizeByName::Failed to get the disk handle for disk %s. ErrorCode: %d\n",
                physicalDriveName, GetLastError());
            bFlag = false;
            break;
        }

        bFlag = DeviceIoControl(hPhysicalDriveIOCTL, IOCTL_DISK_GET_LENGTH_INFO,
            NULL, 0, &dInfo, dInfoSize, &dwValue, NULL);
        CloseHandle(hPhysicalDriveIOCTL);
        if (bFlag)
        {
            diskSize = dInfo.Length.QuadPart;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "IOCTL_DISK_GET_LENGTH_INFO failed...(%lu) - (%d)\n",
                GetLastError(), dwValue);
            bFlag = false;
            break;
        }
    } while (0);
    return bFlag;
}

SV_ULONGLONG getrawvolumesizeUtf8(const std::string& sVolume)
{
    std::string formattedVolume = sVolume;
    SV_ULONGLONG rawsize = 0;


    FormatVolumeName(formattedVolume);
    FormatVolumeNameToGuidUtf8(formattedVolume);
    HANDLE hHandle = SVCreateFileUtf8(formattedVolume.c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hHandle)
    {
        DebugPrintf(SV_LOG_ERROR,
            "FAILED: An error occurred while opening volume"
            " %s. %s. @LINE %d in FILE %s\n",
            sVolume.c_str(),
            Error::Msg().c_str(),
            LINE_NO,
            FILE_NAME);
        return 0;
    }

    GetVolumeSize(hHandle, &rawsize);
    CloseHandle(hHandle);
    return rawsize;
}

/* Get raw capacity of volume */
SV_ULONGLONG getrawvolumesize(const std::string& sVolume)
{
    std::string formattedVolume = sVolume;
    SV_ULONGLONG rawsize = 0;


    FormatVolumeName(formattedVolume);
    FormatVolumeNameToGuid(formattedVolume);
    HANDLE hHandle = SVCreateFile(formattedVolume.c_str(),
        GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (INVALID_HANDLE_VALUE == hHandle)
    {
        DebugPrintf(SV_LOG_ERROR,
            "FAILED: An error occurred while opening volume"
            " %s. %s. @LINE %d in FILE %s\n",
            sVolume.c_str(),
            Error::Msg().c_str(),
            LINE_NO,
            FILE_NAME);
        return 0;
    }

    GetVolumeSize(hHandle, &rawsize);
    CloseHandle(hHandle);
    return rawsize;
}

bool ParseSparseFileNameAndVolPackName(const std::string lineinconffile, std::string &sparsefilename, std::string &volpackname)
{
    bool bretval = false;
    const std::string DELIM_STR = "--";
    if (lineinconffile.empty())
        return bretval;
    size_t found = lineinconffile.find(DELIM_STR);

    while (std::string::npos != found)
    {
        sparsefilename = lineinconffile.substr(0, found);
        size_t volindex = found + DELIM_STR.length();
        volpackname = lineinconffile.substr(volindex);
        if (IsDrive(volpackname))
        {
            bretval = true;
            break;
        }
        else if (isalpha(volpackname[0]) && (volpackname[1] == ':') && volpackname[2] == '\\')
        {
            bretval = true;
            break;
        }
        else
        {
            sparsefilename.clear();
            volpackname.clear();
            found = lineinconffile.find(DELIM_STR, found + 1);
        }
    }

    return bretval;
}

//For windows we are not checking for mountpoint
//this function is a dummy function which always returns true
bool IsValidMountPoint(const std::string & volume, std::string & errmsg)
{
    bool rv = true;
    return rv;
}


/** PARTED */
bool SVSetVolumeMountPoint(const std::string mtpt, const std::string guid)
{
    int count = 8;
    bool rv = false;
    SVERROR sve = SVS_OK;

    while (count--)
    {
        if (SetVolumeMountPoint(mtpt.c_str(), guid.c_str()))
        {
            rv = true;
            break;
        }

        sve = GetLastError();
        DebugPrintf(SV_LOG_ERROR, "Attempt %d :SetVolumeMountPoint failed while mounting: %s at %s err = %s\n", count, mtpt.c_str(), guid.c_str(), sve.toString());
    }

    return rv;
}


/** PARTED */
SVERROR GetFsVolSize(const char *volName, unsigned long long *pfsSize)
{

    if (pfsSize == NULL) {
        DebugPrintf(SV_LOG_ERROR, "GetFsVolSize() failed, err = EINVAL\n");
        return SVE_FAIL;
    }

    SVERROR res = SVS_OK;
    (*pfsSize) = GetVolumeCapacity(volName);
    if (0 == *pfsSize)
    {
        res = SVE_FAIL;
    }

    return res;
}

/** PARTED */
SVERROR GetFsSectorSize(const char *volName, unsigned int *psectorSize)
{
    if (psectorSize == NULL) {
        DebugPrintf(SV_LOG_ERROR, "GetFsVolSize() failed, err = EINVAL\n");
        return SVE_FAIL;
    }
    DWORD bytesPerSector;
    if (!GetDiskFreeSpace(volName, NULL, &bytesPerSector, NULL, NULL)) {
        DebugPrintf(SV_LOG_WARNING, "WARN: GetFileSystemVolumeSize volume[%s] GetDiskFreeSpace (OK if volume is a hidden target or cluster volume offline): %d\n", volName, GetLastError());
        (*psectorSize) = 512; // good enough in most cases.
    }
    else {
        (*psectorSize) = bytesPerSector;
    }

    return SVS_OK;
}


/** PARTED */

///
/// Opens the volume given its name and a pointer to a handle.
/// @bug Not const correct.
///
SVERROR OpenVolumeExtendedUtf8( /* out */ ACE_HANDLE *pHandleVolume,
    /* in */  const char   *pch_VolumeName,
    /* in */  int  openMode
    )
{
    SVERROR sve = SVS_OK;
    char *pch_DestVolume = NULL;

    do
    {
        DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

        if ((NULL == pHandleVolume) || (NULL == pch_VolumeName))
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED OpenVolumeExtended()... sve = EINVAL\n");
            break;
        }

        if (ACE_BIT_ENABLED(openMode, O_WRONLY))
        {
            TCHAR rgtszSystemDirectory[MAX_PATH + 1];
            if (0 == GetSystemDirectory(rgtszSystemDirectory, MAX_PATH + 1))
            {
                sve = HRESULT_FROM_WIN32(GetLastError());
                DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED GetSystemDirectory()... err = %s\n", sve.toString());
                break;
            }
            if (IsDrive(pch_VolumeName))
            {
                if (toupper(pch_VolumeName[0]) == toupper(rgtszSystemDirectory[0]))
                {
                    sve = EINVAL;
                    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                    DebugPrintf(SV_LOG_ERROR, "FAILED: Attempted to Write to System Directory... sve = EINVAL\n");
                    break;
                }
            }
        }

        std::string DestVolName = pch_VolumeName;
        if (!FormatVolumeNameToGuidUtf8(DestVolName))
        {
            sve = SVS_FALSE;
            break;
        }

        ACE_OS::last_error(0);
        // PR#10815: Long Path support
        *pHandleVolume = ACE_OS::open(getLongPathNameUtf8(DestVolName.c_str()).c_str(), openMode, ACE_DEFAULT_OPEN_PERMS);

        if (ACE_INVALID_HANDLE == *pHandleVolume)
        {
            sve = ACE_OS::last_error();
            std::ostringstream errmsg;
            std::string errstr;
            errstr = strerror(ACE_OS::last_error());
            errmsg.clear();
            errmsg << "Open failed for volume " << pch_VolumeName << " [" << DestVolName.c_str() << "]" << " with  " << sve.toString() << " : " << errstr << std::endl;
            DebugPrintf(SV_LOG_ERROR, "%s\n", errmsg.str().c_str());
            break;
        }

    } while (FALSE);

    return(sve);
}



/** PARTED */

///
/// Opens the volume given its name and a pointer to a handle.
/// @bug Not const correct.
///
SVERROR OpenVolumeExtended( /* out */ ACE_HANDLE *pHandleVolume,
    /* in */  const char   *pch_VolumeName,
    /* in */  int  openMode
    )
{
    SVERROR sve = SVS_OK;
    char *pch_DestVolume = NULL;

    do
    {
        DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_DEBUG, "ENTERED OpenVolumeExtended()...\n");
        DebugPrintf(SV_LOG_DEBUG, "OpenVolumeExtended::Volume is %s.\n", pch_VolumeName);

        if ((NULL == pHandleVolume) ||
            (NULL == pch_VolumeName))
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED OpenVolumeExtended()... sve = EINVAL\n");
            break;
        }

        if (ACE_BIT_ENABLED(openMode, O_WRONLY))
        {
            TCHAR rgtszSystemDirectory[MAX_PATH + 1];
            if (0 == GetSystemDirectory(rgtszSystemDirectory, MAX_PATH + 1))
            {
                sve = HRESULT_FROM_WIN32(GetLastError());
                DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED GetSystemDirectory()... err = %s\n", sve.toString());
                break;
            }
            if (IsDrive(pch_VolumeName))
            {
                if (toupper(pch_VolumeName[0]) == toupper(rgtszSystemDirectory[0]))
                {
                    sve = EINVAL;
                    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                    DebugPrintf(SV_LOG_ERROR, "FAILED: Attempted to Write to System Directory... sve = EINVAL\n");
                    break;
                }
            }
        }

        std::string DestVolName = pch_VolumeName;
        FormatVolumeNameToGuid(DestVolName);

        DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf("CALLING CreateFile()...\n");

        ACE_OS::last_error(0);
        // PR#10815: Long Path support
        *pHandleVolume = ACE_OS::open(getLongPathName(DestVolName.c_str()).c_str(), openMode, ACE_DEFAULT_OPEN_PERMS);

        std::ostringstream errmsg;
        std::string errstr;
        if (ACE_INVALID_HANDLE == *pHandleVolume)
        {
            sve = ACE_OS::last_error();
            errstr = strerror(ACE_OS::last_error());
            errmsg.clear();
            errmsg << "Open failed for volume " << DestVolName << " with  " << sve.toString() << " : " << errstr << std::endl;
            DebugPrintf(SV_LOG_ERROR, "%s\n", errmsg.str().c_str());
            break;
        }

    } while (FALSE);

    return(sve);
}


/** PARTED */

// DON'T Use this function directly, instead, use OpenVolumeExclusive.
// Algorithm:
//        1. Get Volume Guid.
//        2. Issue FSCTL_DISMOUNT_VOLUME ioctl. Invalidate the handles
//        3. Delete Volume Mount Point. This will result in Volume to be unmounted, performing Filesystem meta data flushes.
//        4. Call CreateFile() to get exclusive access.
//        5. If unmount option specified, return the result of CreateFile() call.
//        6. Else Remount the volume back to its drive letter.
// Note: In this approach, Unmounting/Remounting happens or Just Unmount happens to ensure
// clean state(flushing file system buffers if any) of the volume.

SVERROR OpenExclusive(/* out */ ACE_HANDLE *pHandleVolume,
    /* out */ std::string &outs_VolGuidName,
    /* in */  char const *pch_VolumeName,
    /* in */  char const* mountPoint,
    /* in */  bool bUnmount,
    /* in */  bool bUnbufferIO
    )
{
    std::string sDestVolume;
    std::string sVol;
    SVERROR sve = SVS_OK;
    int flags = FILE_ATTRIBUTE_NORMAL; // FILE_ATTRIBUTE_NORMAL is required for CreateFile calls
    //__asm int 3;
    do
    {
        HRESULT hr = S_OK;
        HANDLE hDevice = INVALID_HANDLE_VALUE;
        DWORD BytesReturned = 0;

        std::string  sVolumeName = "";
        if (NULL != pch_VolumeName)
        {
            sVolumeName = pch_VolumeName;
            FormatVolumeNameToGuid(sVolumeName);
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "OpenExclusive() did not succeed, Volume passed is null string.\n");
            sve = ACE_OS::last_error();
            return sve;
        }
        // Mount point volume names don't need to be translated
        sDestVolume = sVolumeName;

        // PR#10815: Long Path support
        hDevice = SVCreateFile(sDestVolume.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, flags, NULL);

        if (INVALID_HANDLE_VALUE == hDevice)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "FAILED OpenExclusive(), CreateFile() For FSCTL_DISMOUNT_VOLUME %s hr = %08X\n", sDestVolume.c_str(), hr);
        }
        //Dismount the volume. Here, issue FSCTL_DISMOUNT_VOLUME to invalidate all open handles
        else if (!DeviceIoControl((HANDLE)hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, (LPDWORD)&BytesReturned,
            (LPOVERLAPPED)NULL))
        {
            // NOTE: on fail we don't set hr because we don't want to return fail from this call to DeviceIoControlstill 
            // want to try the rest of the code and only return an error if any of the remaining code fails, otherwise 
            // we want to return success.
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "FSCTL_DISMOUNT_VOLUME did not succeed %d\n", GetLastError());
        }
        else {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "OpenExclusive FSCTL_DISMOUNT_VOLUME succeeded...\n");
        }

        if (INVALID_HANDLE_VALUE != hDevice)
        {
            CloseHandle(hDevice);
        }

        if (bUnmount) {

            outs_VolGuidName = sVolumeName;

            if (!IsVolumeNameInGuidFormat(mountPoint))
            {
                DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_DEBUG, "CALLING DeleteVolumeMountPoint for volume %s\n", mountPoint);

                string tempMountPoint = mountPoint;

                FormatVolumeNameForCxReporting(tempMountPoint);

                //if( 1 == strlen( mountPoint ) )
                if (1 == strlen(tempMountPoint.c_str()))
                {
                    tempMountPoint += ":\\";
                }
                else
                {
                    tempMountPoint += "\\";
                }

                //16211 - Add the guid and drive information to a persistent file.

                std::string guid = outs_VolGuidName;
                ExtractGuidFromVolumeName(guid);
                PersistDetachedVolumeEntry(tempMountPoint, guid);

                if (!DeleteVolumeMountPoint(tempMountPoint.c_str()))
                {
                    sve = GetLastError();
                    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                    DebugPrintf(SV_LOG_ERROR, "FAILED OpenExclusive(), DeleteVolumeMountPoint %s; err = %s\n", sDestVolume.c_str(), sve.toString());
                    //16211 - Remove the guid and drive information from a persistent file as DeleteVolumeMountPoint is not successful.
                    RemoveAttachedVolumeEntry(tempMountPoint, guid);
                    break;
                }
            }
            sVol = outs_VolGuidName;

        }
        else
        {
            sVol = sDestVolume;
        }

        if (bUnbufferIO){
            flags |= (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH);
        }

        // PR#10815: Long Path support
        *pHandleVolume = SVCreateFile(sVol.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (INVALID_HANDLE_VALUE == *pHandleVolume)
        {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "CreateFile() did not succeed %d\n", GetLastError());
            sve = GetLastError();

            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            if (bUnmount) {
                DebugPrintf(SV_LOG_ERROR, "OpenVolumeEx() did not succeed, Volume: %s; VolGuid: %s err = %s\n", pch_VolumeName, outs_VolGuidName.c_str(), sve.toString());
            }
            else {
                DebugPrintf(SV_LOG_ERROR, "OpenVolumeEx() did not succeed, Volume: %s err= %s\n", pch_VolumeName, sve.toString());
            }

            if (bUnmount) {


                if (!IsVolumeNameInGuidFormat(mountPoint))
                {
                    // Try to restore the volume back at it's previous drive letter
                    DebugPrintf(SV_LOG_DEBUG, "Restoring the volume %s to its original drive letter.\n", pch_VolumeName);

                    std::string fmtVolName = mountPoint;
                    ToStandardFileName(fmtVolName);
                    std::string::size_type len = fmtVolName.length();
                    if ('\\' != fmtVolName[len - 1]) {
                        fmtVolName += '\\';
                    }

                    std::string volGuid = outs_VolGuidName;
                    len = volGuid.length();
                    if ('\\' != volGuid[len - 1]) {
                        volGuid += '\\';
                    }

                    if (!SVSetVolumeMountPoint(fmtVolName, volGuid))
                    {
                        sve = GetLastError();
                        DebugPrintf(SV_LOG_ERROR, "OpenVolumeEx::SVSetVolumeMountPoint failed while mounting: %s at %s err = %s\n", volGuid.c_str(), fmtVolName.c_str(), sve.toString());
                        break;
                    }
                    //16211 - Remove the volume entry from the persisted file as the volume is reattached.
                    std::string guid = outs_VolGuidName;
                    ExtractGuidFromVolumeName(guid);
                    RemoveAttachedVolumeEntry(mountPoint, guid);
                }
            }
            break;
        }
        if (!DeviceIoControl((HANDLE)(*pHandleVolume), FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, (LPDWORD)&BytesReturned,
            (LPOVERLAPPED)NULL))
        {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "FSCTL_LOCK_VOLUME did not succeed %d\n", GetLastError());
            if (bUnmount)
            {
                if (!DeviceIoControl((HANDLE)(*pHandleVolume), FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, (LPDWORD)&BytesReturned,
                    (LPOVERLAPPED)NULL))
                {
                    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                    DebugPrintf(SV_LOG_DEBUG, "FSCTL_DISMOUNT_VOLUME did not succeed  for volume %s Error %d\n", mountPoint, GetLastError());
                }
                else {
                    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                    DebugPrintf(SV_LOG_DEBUG, "OpenExclusive FSCTL_DISMOUNT_VOLUME succeeded for volume %s\n", mountPoint);
                }
            }
        }

    } while (FALSE);

    return sve;
}


/** PARTED */

// Current Implementation: Earlier implementation of this function assumed the caller is the only 
// possible reader/writer, since caller has altered the filesystem tag. We need a more generalised
// mechanism wherein the Volume can be still opened by other processes and writing to it ex: Snapshot
// and Recovery volume. To achieve exclusive access functionality, we forcibly close any handles pending 
// on that volume so that a request for excluvise will never fail.
//
// Old Implementation: Opens the volume exclusively given its name and a pointer to a handle. Unlike OpenVolume, 
// we are the only possible readers/writes to the volume, because the file system tag is
// altered. The caller can also specify whether the volume has to be dismounted.
//
SVERROR OpenVolumeExclusive( /* out */ ACE_HANDLE    *pHandleVolume,
    /* out */ std::string &outs_VolGuidName,
    /* in */  char const *pch_VolumeName, /* E.G: E: */
    /* in */  char const *mountPoint,     /* either "C" or "x:\mnt\my_volume"
                                          /* in */  bool        bUnmount,               /* Whether the volume should be unmounted */
                                          /* in */  bool        bUnbufferIO
                                          )
{
    SVERROR sve = SVS_OK;

    do
    {
        DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_DEBUG, "ENTERED OpenVolumeEx()...\n");

        if ((NULL == pHandleVolume) ||
            (NULL == pch_VolumeName)
            )
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_ERROR, "Invalid argument passed to OpenVolumeExClusive\n");
            return sve;
        }

        DebugPrintf(SV_LOG_DEBUG, "Attempting Volume Open on %s...\n", pch_VolumeName);


        TCHAR rgtszSystemDirectory[MAX_PATH + 1];
        if (0 == GetSystemDirectory(rgtszSystemDirectory, MAX_PATH + 1))
        {
            sve = HRESULT_FROM_WIN32(GetLastError());
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "Error occured while reading system directory information from registry.\n");
            return sve;
        }

        if (IsDrive(pch_VolumeName))
        {
            if (toupper(pch_VolumeName[0]) == toupper(rgtszSystemDirectory[0]))
            {
                sve = EINVAL;
                DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED: Attempted to Write to System Directory... err = EINVAL\n");
                return sve;
            }
        }

        sve = OpenExclusive(pHandleVolume, outs_VolGuidName, pch_VolumeName, mountPoint, bUnmount, bUnbufferIO);
        if (sve.failed())
        {
            // FIXME: In Win 2k, after a issuing FSCTL_DISMOUNT_VOLUME ioctl, the first call to CreateFile() to get 
            // exclusive access always fails eventhough there are not any open handles , but the second call succeeds.
            // As it seemed like a windows 2k bug, we are doing a retry for one more time to get exclusive access. When
            // actual reason for CreateFile() failure is known, this code can be removed by making necessary changes.
            // For more info on this Bug, refer to Bug id #635 in bugzilla.
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_INFO, "First attempt to get exclusive access did not succeed, retrying ...\n");
            sve = OpenExclusive(pHandleVolume, outs_VolGuidName, pch_VolumeName, mountPoint, bUnmount, bUnbufferIO);
        }
    } while (FALSE);

    if (sve.failed())
    {
        DebugPrintf(SV_LOG_ERROR, "Error while trying to obtain exclusive access on volume : %s\n", pch_VolumeName);
    }

    return(sve);
}




/** PARTED */

/// Closes a volume that we opened for exclusive access using OpenVolumeExclusive.
/// See also CloseVolume
SVERROR CloseVolumeExclusive(
    /* in */ ACE_HANDLE handleVolume,
    /* in */ const char *pch_VolGuidName,// Volume in GUID format
    /* in */ const char *pch_VolumeName,  // Drive letter volume was prev mounted on
    /* in */ const char *pszMountPoint     // Optional: pathname to directory where volume mounted
    )
{
    SVERROR sve = SVS_OK;

    do
    {
        DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf("ENTERED CloseVolumeEx()...\n");

        if ((NULL == pch_VolumeName) ||
            (NULL == pch_VolGuidName)
            )
        {
            sve = EINVAL;
            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED CloseVolumeEx(). VolumeName or Guid are invalid. Err = EINVAL\n");
            break;
        }

        DebugPrintf("Attempting Volume Close on %s...\n", pch_VolumeName);

        if ((ACE_INVALID_HANDLE == handleVolume))
        {
            sve = ACE_OS::last_error();
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED CloseVolume()... err = %s\n", sve.toString());
            DebugPrintf(SV_LOG_ERROR, "handleVolume == ACE_INVALID_HANDLE... \n");
            break;
        }

        DWORD BytesReturned = 0;

        if (!DeviceIoControl((HANDLE)handleVolume, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, (LPDWORD)&BytesReturned,
            (LPOVERLAPPED)NULL))
        {
            // cout << "fsctl_unlock_volume failed" << GetLastError() << "\n";
            //goto cleanup;
        }
        if ((ACE_INVALID_HANDLE != handleVolume) &&
            ACE_OS::close(handleVolume))
        {
            sve = ACE_OS::last_error();
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED CloseVolumeExclusive(). close call failed on volume %s with err = %s\n", pch_VolumeName, sve.toString());
        }

        HRESULT hr = S_OK;

        // Bug #9090
        // mountPoint was being assigned pszMounPoint even when pszMountPoint is empty

        string mountPoint = (pszMountPoint && pszMountPoint[0]) ? pszMountPoint : pch_VolumeName;
        if (!IsVolumeNameInGuidFormat(mountPoint))
        {

            FormatVolumeNameForCxReporting(mountPoint);

            if (1 == mountPoint.size())
            {
                mountPoint += ":";
            }
            mountPoint += '\\';

            // Mount the volume back at it's previous drive letter
            std::string sGuid = pch_VolGuidName;
            if (sGuid[sGuid.length() - 1] != '\\')
            {
                sGuid += '\\';
            }

            if (!SVSetVolumeMountPoint(mountPoint, sGuid))
            {
                sve = hr = HRESULT_FROM_WIN32(GetLastError());
                DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "CloseVolumeExclusive::SVSetVolumeMountPoint failed while mounting: %s at %s err = %s\n", sGuid.c_str(), mountPoint.c_str(), sve.toString());
                break;
            }
            else
            {
                std::string guid = sGuid;

                ExtractGuidFromVolumeName(guid);

                RemoveAttachedVolumeEntry(mountPoint, guid);
                DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf("SUCCESS CloseVolumeEx() for volume %s hr = %08X\n", mountPoint.c_str(), hr);
                DebugPrintf("       in SetVolumeMountPoint");
                break;
            }
        }
    } while (FALSE);

    return(sve);
}


/** PARTED */

bool MountPointToGuid(const char* mount, void* guid, int guidLen)
{
    std::string sVolume = mount;
    if (!FormatVolumeNameToGuid(sVolume))
        return false;
    ExtractGuidFromVolumeName(sVolume);
    USES_CONVERSION;
    inm_memcpy_s(guid, guidLen, A2W(sVolume.c_str()), (GUID_SIZE_IN_CHARS * sizeof(wchar_t)));
    return true;
}


/** PARTED */
bool DriveLetterToGuid(char drive, void* guid, int guidLen)
{
    char drive_name[4] = { 0 };
    drive_name[0] = drive;
    std::string sparsefile;
    bool new_sparsefile_format = false;
    bool is_volpack = IsVolPackDevice(drive_name, sparsefile, new_sparsefile_format);
    if (!IsVolpackDriverAvailable() && (is_volpack))
    {
        USES_CONVERSION;
        inm_memcpy_s(guid, guidLen, A2W(sparsefile.c_str()), (GUID_SIZE_IN_CHARS * sizeof(WCHAR)));
        return true;
    }

    if (guidLen  < (GUID_SIZE_IN_CHARS * (sizeof(WCHAR)))) {
        return false;
    }

    std::wstringstream SSVol;
    WCHAR name[60];

    SSVol << drive << ":\\";

    if (0 == GetVolumeNameForVolumeMountPointW((LPWSTR)SSVol.str().c_str(), name, NELEMS(name))) {
        DebugPrintf(SV_LOG_ERROR, "DriveLetterToGUID failed for drive %s with errno : %d \n", SSVol.str().c_str(), GetLastError());
        return false;
    }

    inm_memcpy_s(guid, guidLen, &name[11], (GUID_SIZE_IN_CHARS * sizeof(WCHAR)));
    // PR # 2396
    // in debug builds, assert to trace the bug
    // in release builds, return false
    bool allZeros = BufferIsZero((char*)guid, guidLen);
    assert(false == allZeros);
    if (true == allZeros)
    {
        DebugPrintf(SV_LOG_ERROR, "DriveLetterToGUID returned all zeroes for %c: \n",
            drive);
        return false;
    }

    return true;
}





/** PARTED */

bool GetDriverVolumeSize(char *vol, SV_ULONGLONG* size)
{
    (*size) = 0;
    bool ok = false;


    // PR#10815: Long Path support
    HANDLE h = SVCreateFile(INMAGE_FILTER_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);
    if (INVALID_HANDLE_VALUE == h) {
        DebugPrintf(SV_LOG_ERROR, "GetDriverVolumeSize open device failed: %d\n", GetLastError());
        return false;
    }


    DWORD result = 0;
    const DWORD count = 1;
    DWORD statSize = 0;
    INM_SAFE_ARITHMETIC(statSize = InmSafeInt<size_t>::Type(sizeof(VOLUME_STATS_DATA)) + (count * InmSafeInt<size_t>::Type(sizeof(VOLUME_STATS))), INMAGE_EX(sizeof(VOLUME_STATS_DATA))(sizeof(VOLUME_STATS)))

    VOLUME_STATS_DATA* stats = 0; // we currently use the volume stats to get the volume size

    stats = (VOLUME_STATS_DATA*)new char[statSize];
    if (0 == stats) {
        DebugPrintf(SV_LOG_ERROR, "GetDriverVolumeSize failed: out of memory\n");
    }
    else {
        WCHAR guid[GUID_SIZE_IN_CHARS];
        if (DriveLetterToGuid(vol[0], guid, sizeof(guid))) {
            if (!DeviceIoControl(h, (DWORD)IOCTL_INMAGE_GET_VOLUME_STATS, guid, sizeof(guid), stats, statSize, &result, NULL)) {
                DebugPrintf(SV_LOG_ERROR, "GetDriverVolumeSize DeviceIoControl failed: %d", GetLastError());
            }
            else if (1 == stats->ulVolumesReturned) {
                PVOLUME_STATS	VolumeStats = (PVOLUME_STATS)((PUCHAR)stats + sizeof(VOLUME_STATS_DATA));
                ok = true;
                (*size) = VolumeStats->ulVolumeSize.QuadPart;
            }
        }
    }

    delete[] stats;

    CloseHandle(h);

    // OK now see if we should use the driver of file system size
    return ok;
}


/** PARTED */

bool CanReadDriverSize(char *vol, SV_ULONGLONG offset, unsigned int sectorSize)
{
    bool canRead = false;

    if (vol == NULL)
        return canRead;

    /*
    char volName[8];

    volName[0] = '\\';
    volName[1] = '\\';
    volName[2] = '.';
    volName[3] = '\\';
    volName[4] = vol[0];
    volName[5] = ':';
    volName[6] = '\0';
    */
    std::string sVolGuid = vol;
    FormatVolumeNameToGuid(sVolGuid);
    // PR#10815: Long Path support
    HANDLE h = SVCreateFile(sVolGuid.c_str(), GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == h) {
        DebugPrintf(SV_LOG_ERROR, "CanReadDriverSize opening volume %s failed: %d\n", vol, GetLastError());
        return false;
    }
    LARGE_INTEGER seekOffset;
    seekOffset.QuadPart = offset - sectorSize; // 0 indexed so need to subtract 1 sector and then try reading

    LARGE_INTEGER newOffset;
    newOffset.QuadPart = 0;

    if (!SetFilePointerEx(h, seekOffset, &newOffset, FILE_BEGIN)) {
        DebugPrintf(SV_LOG_WARNING, "WARN: CanReadDriverSize SetFilePointerEx did not succeed (OK if EOF): %d\n", GetLastError());
    }
    else {

        SV_ULONG bytesRead = 0;
        size_t buflen;
        INM_SAFE_ARITHMETIC(buflen = 2 * InmSafeInt<unsigned int>::Type(sectorSize), INMAGE_EX(sectorSize))
            char* buffer = (char*)malloc(buflen);
        char* align = (char*)((unsigned long long)(buffer + sectorSize) & ~(sectorSize - 1));
        if (0 != buffer) {
            if (!ReadFile(h, align, sectorSize, &bytesRead, NULL)) {
                DebugPrintf(SV_LOG_WARNING, "WARN, CanReadDriverSize ReadFile did not succeed (OK if EOF): %d\n", GetLastError());
            }
            else if (bytesRead == sectorSize) {
                canRead = true;
            }
            free(buffer);
        }
    }

    CloseHandle(h);

    return canRead;
}

/** PARTED */

SVERROR GetHardwareSectorSize(ACE_HANDLE handleVolume, unsigned int *pSectSize)
{
    SVERROR sve = SVS_OK;
    unsigned int sector_size = 0;


    do
    {
        if (pSectSize == NULL || handleVolume == ACE_INVALID_HANDLE)
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED GetHardwareSectorSize, err = EINVAL\n");
            break;
        }


        DWORD cbReturned = 0;
        DISK_GEOMETRY disk_geometry = { 0 };

        if (0 == DeviceIoControl(handleVolume,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            &disk_geometry,
            sizeof(DISK_GEOMETRY),
            &cbReturned,
            NULL))
        {
            //
            // PR# 11616:
            // Tracking bug: Customer: Spansion. Replication progressed without hiding the target volume
            // if the ioctl fails, send the sector size as 512
            //
            //sve = ACE_OS::last_error();
            HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "FAILED DeviceIoControl()... hr = %08X\n", hr);
            sector_size = 512;
            break;
        }

        sector_size = (unsigned int)disk_geometry.BytesPerSector;

    } while (FALSE);

    if (!sve.failed())
        *pSectSize = sector_size;

    return sve;
}


// check if volume locked by us. 
// as a convenience if the fsType buffer is supplied also return what type of file system it is
bool IsVolumeLockedUtf8(const char *pszVolume, int fsTypeSize, char * fsType)
{
    int fs_type;
    bool hidden = false;
    if (GetFileSystemTypeForVolumeUtf8(pszVolume, fs_type, hidden).succeeded())
    {
        if (hidden && NULL != fsType)
        {
            std::string strFsType;
            switch (fs_type)
            {
            case TYPE_FAT:
                strFsType = "FAT";
                break;
            case TYPE_NTFS:
                strFsType = "NTFS";
                break;
            case TYPE_ReFS:
                strFsType = "ReFS";
                break;
            case TYPE_EXFAT:
                strFsType = "EXFAT";
                break;
            default:
                DebugPrintf(SV_LOG_ERROR, "%s: Unknown hidden filesystem (%d) reported for volume %s.\n", FUNCTION_NAME, fs_type, pszVolume);
                break;
                //Note: We still return true as the GetFileSystemTypeForVolume returned SUCCESS. It could be a new filesytem support
                //and an entry to this switch/case is miising.
            }
            strncpy_s(fsType, fsTypeSize, strFsType.c_str(), strFsType.length());
        }
        return hidden;
    }
    return hidden;
}

/** PARTED */

// check if volume locked by us. 
// as a convenience if the fsType buffer is supplied also return what type of file system it is
bool IsVolumeLocked(const char *pszVolume, const int fsTypeSize, char * fsType)
{
    int fs_type;
    bool hidden = false;
    if (GetFileSystemTypeForVolume(pszVolume, fs_type, hidden).succeeded())
    {
        if (hidden && NULL != fsType)
        {
            switch (fs_type)
            {
            case TYPE_FAT:
                inm_strncpy_s(fsType, fsTypeSize, "FAT", _TRUNCATE);
                break;
            case TYPE_NTFS:
                inm_strncpy_s(fsType, fsTypeSize, "NTFS", _TRUNCATE);
                break;
            case TYPE_ReFS:
                inm_strncpy_s(fsType, fsTypeSize, "ReFS", _TRUNCATE);
                break;
            case TYPE_EXFAT:
                inm_strncpy_s(fsType, fsTypeSize, "EXFAT", _TRUNCATE);
                break;
            default:
                DebugPrintf(SV_LOG_ERROR, "%s: Unknown hidden filesystem (%d) reported for volume %s.\n", FUNCTION_NAME, fs_type, pszVolume);
                break;
                //Note: We still return true as the GetFileSystemTypeForVolume returned SUCCESS. It could be a new filesytem support
                //and an entry to this switch/case is miising.
            }
        }
        return hidden;
    }
    return hidden;
}

/** PARTED */

bool
DriveLetterToGUID(char DriveLetter, void *VolumeGUID, SV_ULONG ulBufLen)
{
    WCHAR   MountPoint[4];
    WCHAR   UniqueVolumeName[60];

    MountPoint[0] = (WCHAR)DriveLetter;
    MountPoint[1] = L':';
    MountPoint[2] = L'\\';
    MountPoint[3] = L'\0';

    if (ulBufLen  < (GUID_SIZE_IN_CHARS * sizeof(WCHAR)))
        return false;

    if (0 == GetVolumeNameForVolumeMountPointW((WCHAR *)MountPoint, (WCHAR *)UniqueVolumeName, NELEMS(UniqueVolumeName))) {
        DebugPrintf(SV_LOG_ERROR, "DriveLetterToGUID failed for %c: \n", DriveLetter);
        return false;
    }

    inm_memcpy_s(VolumeGUID, ulBufLen, &UniqueVolumeName[11], (GUID_SIZE_IN_CHARS * sizeof(WCHAR)));

    return true;
}





/** PARTED */
void UnformatVolName(std::string& sVolName)
{
    size_t pos = 0;
    if (false == sVolName.empty())
    {
        if ((pos = sVolName.find("\\\\.\\", 0)) != std::string::npos)
        {
            sVolName.replace(pos, 4, "");
        }
    }
}


/** PARTED */

SVERROR UnhideDrive_RO(const char * drive, char const* mountPoint, const char* fs)
{
    SVERROR sve = SVS_OK;
    LocalConfigurator localConfigurator;
    if (!localConfigurator.IsFilterDriverAvailable())
    {
        DebugPrintf(SV_LOG_ERROR, "The filter driver is not present. so the device %s can not be unhidden in read only mode.\n", drive);
        sve = SVE_FAIL;
        return sve;
    }

    (void)FlushFileSystemBuffers(drive);
    //__asm int 3;

    /*
    *  Open the volume in exclusive mode  with unmount = true
    *  Stop Filtering
    *  Remove the SVFS tag
    *  Set Read Only
    *  Close Volume
    */

    ACE_HANDLE handleVolume = ACE_INVALID_HANDLE;
    std::string sVolumeGuid;
    std::string volumeName = drive;
    FormatVolumeName(volumeName);
    bool isVirtualVolume = false;

    if (NULL == drive)
    {
        sve = EINVAL;
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RO()... err = EINVAL\n");
        return sve;
    }

    // First unformat the volume name to get it in the form E:
    // Use UnformatVolName

    std::string sVolName = drive;
    UnformatVolName(sVolName);

    std::string sGuid = sVolName;
    FormatVolumeNameToGuid(sGuid);
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));

    sve = OpenVolumeExclusive(&handleVolume, sVolumeGuid, sVolName.c_str(), mountPoint, true, false);
    if (sve.failed() || ACE_INVALID_HANDLE == handleVolume)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RO()... err = %s\n", sve.toString());
        goto cleanup;
    }

    isVirtualVolume = IsSparseVolume(volumeName);
    if (!isVirtualVolume)
    {
        DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf("CALLING StopFiltering() on drive : %s ...\n", drive);

        if (StopFilter(drive, VolumeGUID).failed())
        {
            //When StopFilter fails, sve should indicate failure
            sve = SVS_FALSE;
            goto cleanup;
        }

    }
    DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
    DebugPrintf("Trying to unhide drive : %s ...\n", drive);

    do
    {
        int sectorSize = 4096;
        //Read the first 4096.. bytes?????. We already read in GetFilesystemType. Should we read it again??
        SharedAlignedBuffer buffer(sectorSize, sectorSize);

        char * pchDeltaBuffer = buffer.Get();
        //Calculating size of pchDeltaBuffer to send as a argument to UnHideFileSystem()
        int size = buffer.Size();
        if (NULL == pchDeltaBuffer)
        {
            sve = ENOMEM;
            break;
        }

        //Read one sector
        ssize_t cbRead = ACE_OS::read(handleVolume, pchDeltaBuffer, sectorSize);

        if (cbRead != sectorSize)
        {
            sve = ACE_OS::last_error();
            std::string errstr = ACE_OS::strerror(ACE_OS::last_error());
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read() failed for volume %s with error %s - %s\n", volumeName.c_str(), sve.toString(), errstr.c_str());
            break;
        }

        if (UnHideFileSystem(pchDeltaBuffer, size).succeeded())
        {
            //Seek to the begining of the volume and write the modified buffer
#ifndef LLSEEK_NOT_SUPPORTED
            if (ACE_OS::llseek(handleVolume, 0, SEEK_SET) < 0) {
#else
            if (ACE_OS::lseek(handleVolume, 0, SEEK_SET) < 0) {
#endif
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::llseek, err = %s\n", sve.toString());
                break;
            }

            ssize_t cbWritten = ACE_OS::write(handleVolume, pchDeltaBuffer, sectorSize);

            if (cbWritten != sectorSize)
            {
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read()... err = %s\n", sve.toString());
                break;
            }
            }
        }while (FALSE);

        DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf("CALLING SetReadOnly() ...\n");
        if (SetReadOnly(drive, VolumeGUID, isVirtualVolume).failed())
        {
            //Bug #7135
            sve = SVS_FALSE;
            goto cleanup;
        }


    cleanup:

        if (ACE_INVALID_HANDLE != handleVolume)
        {

            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf("CALLING CloseVolume()...\n");
            sve = CloseVolumeExclusive(handleVolume, sVolumeGuid.c_str(), sVolName.c_str(), mountPoint);
            if (sve.failed())
            {
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RO()... err = %s\n", sve.toString());
            }
        }

        //Added the following for bug 1271
        char szOutVolumeName[BUFSIZ];
        DWORD dwMaxFilenameLength = 0;
        DWORD dwFlags = 0;
        bool setlabel = false;
        std::string sVolume = drive;
        UnformatVolumeNameToDriveLetter(sVolume);
        FormatVolumeName(sVolume, false);
        if (GetVolumeInformation(sVolume.c_str(),
            szOutVolumeName,
            sizeof(szOutVolumeName),
            NULL,
            &dwMaxFilenameLength,
            &dwFlags,
            NULL,
            0))
        {
            setlabel = true;
        }

        if (sve.succeeded() && setlabel)
        {
            SetVolumeLabel(sVolume.c_str(), szOutVolumeName);
        }

        return sve;
    }



/** PARTED */

SVERROR UnhideDrive_RW(const char * drive, char const* mountPoint, const char* fs)
{
    SVERROR sve = SVS_OK;

    (void)FlushFileSystemBuffers(drive);
    //__asm int 3;
    /*
    *  Open the volume in exclusive mode  with unmount = true
    *  Reset Read Only
    *  Remove the SVFS tag
    *  Close Volume
    */

    ACE_HANDLE handleVolume = ACE_INVALID_HANDLE;
    std::string sVolumeGuid;
    std::string volumeName = drive;
    FormatVolumeName(volumeName);
    bool isVirtualVolume = false;

    if (NULL == drive)
    {
        sve = EINVAL;
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = EINVAL\n");
        return sve;
    }

    // First unformat the volume name to get it in the form E:
    // Use UnformatVolName

    std::string sVolName = drive;
    UnformatVolName(sVolName);
    const char * pszVolume = sVolName.c_str();

    char DriveLetter = pszVolume[0];
    std::string sGuid = sVolName;
    FormatVolumeNameToGuid(sGuid);
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));

    sve = OpenVolumeExclusive(&handleVolume, sVolumeGuid, pszVolume, mountPoint, true, false);
    if (sve.failed() || ACE_INVALID_HANDLE == handleVolume)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = %s\n", sve.toString());
        goto cleanup;
    }

    isVirtualVolume = IsSparseVolume(volumeName);

    DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
    DebugPrintf("CALLING ResetReadOnly() ...\n");
    if (ResetReadOnly(drive, VolumeGUID, isVirtualVolume).failed())
    {
        //Bug #7135
        sve = SVS_FALSE;
        goto cleanup;
    }

    DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
    DebugPrintf("Trying to unhide drive : %s ...\n", drive);

    do
    {
        int sectorSize = 4096;
        //Read the first 4096.. bytes?????. We already read in GetFilesystemType. Should we read it again??
        SharedAlignedBuffer buffer(sectorSize, sectorSize);

        char * pchDeltaBuffer = buffer.Get();
        int size = buffer.Size();
        if (NULL == pchDeltaBuffer)
        {
            sve = ENOMEM;
            break;
        }

        //Read one sector
        ssize_t cbRead = ACE_OS::read(handleVolume, pchDeltaBuffer, sectorSize);

        if (cbRead != sectorSize)
        {
            sve = ACE_OS::last_error();
            std::string errstr = ACE_OS::strerror(ACE_OS::last_error());
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read() failed for volume %s with error %s - %s\n", volumeName.c_str(), sve.toString(), errstr.c_str());
            break;
        }

        if (UnHideFileSystem(pchDeltaBuffer, size).succeeded())
        {
            //Seek to the begining of the volume and write the modified buffer
#ifndef LLSEEK_NOT_SUPPORTED
            if (ACE_OS::llseek(handleVolume, 0, SEEK_SET) < 0) {
#else
            if (ACE_OS::lseek(handleVolume, 0, SEEK_SET) < 0) {
#endif
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::llseek, err = %s\n", sve.toString());
                break;
            }

            ssize_t cbWritten = ACE_OS::write(handleVolume, pchDeltaBuffer, sectorSize);

            if (cbWritten != sectorSize)
            {
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read()... err = %s\n", sve.toString());
                break;
            }
            }
        }while (FALSE);

    cleanup:

        if (ACE_INVALID_HANDLE != handleVolume)
        {

            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf("CALLING UnhideDrive_RW()...\n");
            sve = CloseVolumeExclusive(handleVolume, sVolumeGuid.c_str(), pszVolume, mountPoint);
            if (sve.failed())
            {
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = %s\n", sve.toString());
            }
        }

        //Added the following for bug 1271
        char szOutVolumeName[BUFSIZ];
        DWORD dwMaxFilenameLength = 0;
        DWORD dwFlags = 0;
        bool setlabel = false;
        std::string sVolume = drive;
        UnformatVolumeNameToDriveLetter(sVolume);
        FormatVolumeName(sVolume, false);

        if (GetVolumeInformation(sVolume.c_str(),
            szOutVolumeName,
            sizeof(szOutVolumeName),
            NULL,
            &dwMaxFilenameLength,
            &dwFlags,
            NULL,
            0))
        {
            setlabel = true;
        }

        if (sve.succeeded() && setlabel)
        {
            SetVolumeLabel(sVolume.c_str(), szOutVolumeName);
        }

        return sve;

    }



/** PARTED */

bool MountVolume(const std::string& device, const std::string& mountPoint, const std::string& fsType, bool bSetReadOnly)
{
    return true;
}


/** PARTED */
void ExtractGuidFromVolumeName(std::string& sVolumeNameAsGuid)
{
    std::string sGuid = sVolumeNameAsGuid;
    if (!sGuid.empty())
    {
        size_t pos = 0;
        if ((pos = sGuid.find("{")) != std::string::npos)
        {
            sGuid = sGuid.erase(0, pos + 1);
        }

        if ((pos = sGuid.find("}")) != std::string::npos)
        {
            sGuid = sGuid.erase(pos, sGuid.length() - 1);
        }
    }
    sVolumeNameAsGuid = sGuid;
}





/** PARTED */

SVERROR HideDrive(const char * drive, char const* mountPoint, std::string& output, std::string& error, bool checkformultipaths)
{
    SVERROR sve = SVS_OK;
    LocalConfigurator localConfigurator;
    if (NULL == drive)
    {
        sve = EINVAL;
        error = "HideDrive: The drive name is invalid (null)\n";
        return sve;
    }
    bool skipcheck = localConfigurator.IsCdpcliSkipCheck();
    //as the parameter drive to this function may be a drive letter or volume guid, 
    //so for checking the system drives,pagefile drives,cachevolume we need the mountpoint
    //so making a check if mountpoint is not supplied then assigning the drive
    std::string mountdrive;
    if (NULL != mountPoint)
    {
        mountdrive = mountPoint;
    }
    else
    {
        mountdrive = drive;
    }

    if ((!skipcheck) && IsCacheVolume(mountdrive))
    {
        sve = SVE_FAIL;
        std::ostringstream ostr;
        ostr << "Cannot Hide " << mountdrive << " as it is cache volume." << std::endl;
        error = ostr.str().c_str();
        return sve;
    }

    if ((!skipcheck) && IsInstallPathVolume(mountdrive))
    {
        sve = SVE_FAIL;
        std::ostringstream ostr;
        ostr << "Cannot Hide " << mountdrive << " as it is vx install volume." << std::endl;
        error = ostr.str().c_str();
        return sve;
    }
    if (!skipcheck)
    {
        TCHAR rgtszSystemDirectory[MAX_PATH + 1];
        if (GetSystemDirectory(rgtszSystemDirectory, MAX_PATH + 1))
        {
            if ((IsDrive(mountdrive)) &&
                (((mountdrive.size() <= 3) && (rgtszSystemDirectory[0] == mountdrive[0])) ||
                (('\\' == mountdrive[0]) && (rgtszSystemDirectory[0] == mountdrive[4]))))
            {
                sve = SVE_FAIL;
                std::ostringstream ostr;
                ostr << "Cannot Hide " << mountdrive << " as the drive is a system volume.\n";
                error = ostr.str().c_str();
                return sve;
            }
        }

        if (IsDrive(mountdrive))
        {
            DWORD pageDrives = getpagefilevol();
            DWORD dwSystemDrives = 0;
            if ('\\' == mountdrive[0])
                dwSystemDrives = 1 << (toupper(mountdrive[4]) - 'A');
            else
                dwSystemDrives = 1 << (toupper(mountdrive[0]) - 'A');

            if (pageDrives & dwSystemDrives)
            {
                sve = SVE_FAIL;
                std::ostringstream ostr;
                ostr << "Cannot Hide " << mountdrive << " as the drive contains pagefile.\n";
                error = ostr.str().c_str();
                return sve;
            }
        }
    }

    std::string nestedmountpoints;
    if ((!skipcheck) && containsMountedVolumes(mountdrive, nestedmountpoints))
    {
        sve = SVE_FAIL;
        std::ostringstream ostr;
        ostr << "Cannot Hide " << mountdrive << " as the drive contains mount points " << nestedmountpoints << ".\n";
        error = ostr.str().c_str();
        return sve;
    }

    if ((!skipcheck) && containsVolpackFiles(mountdrive))
    {
        sve = SVE_FAIL;
        std::ostringstream ostr;
        ostr << "Cannot Hide " << mountdrive << " as the volume contains virtual volume data.\n";
        error = ostr.str().c_str();
        return sve;
    }

    //__asm int 3;
    ACE_HANDLE handleVolume = ACE_INVALID_HANDLE;
    std::string sVolumeGuid;
    bool bNTFSVolume = false;
    bool bFATVolume = false;

    // First unformat the volume name to get it in the form E:
    // Use UnformatVolName

    std::string sVolName = drive;
    UnformatVolName(sVolName);
    const char * pszVolume = sVolName.c_str();

    std::string volumeDirectory = pszVolume;
    std::string sGuid = "";

    FormatVolumeNameToGuid(volumeDirectory);
    sGuid = volumeDirectory;
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR VolumeGUID[GUID_SIZE_IN_CHARS];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));

    int fstype;
    bool hidden = false;
    GetFileSystemTypeForVolume(pszVolume, fstype, hidden);

    if (hidden)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: %s already hidden.\n", FUNCTION_NAME, mountdrive.c_str());
        return sve;
    }

    //16211 - For raw volumes, we don't need to open the volume exclusively.
    if (fstype != TYPE_UNKNOWNFS)
    {
        sve = OpenVolumeExclusive(&handleVolume, sVolumeGuid, pszVolume, mountPoint, true, false);
        if (sve.failed() || ACE_INVALID_HANDLE == handleVolume)
        {
            error = "HideDrive: ";
            error += mountdrive;
            error += " OpenVolumeExclusive failed\n";
            return sve;
        }
    }

    std::string volumeName = mountPoint;
    FormatVolumeName(volumeName);
    bool isVirtualVolume = false;

    isVirtualVolume = IsSparseVolume(volumeName);

    // Reset the read only volume flag
    if (ResetReadOnly(mountPoint, VolumeGUID, isVirtualVolume).failed())
    {
        //Bug# 7135
        sve = SVE_FAIL;
        error = "HideDrive: ";
        error += mountdrive;
        error += " reset read-only flag failed\n";
        goto cleanup;
    }

    if ((!isVirtualVolume) && localConfigurator.IsFilterDriverAvailable())
    {
        // Stop Filtering
        if (StopFilter(drive, VolumeGUID).failed())
        {
            sve = SVE_FAIL;
            goto cleanup;
        }
    }

    do
    {
        if (fstype != TYPE_UNKNOWNFS)
        {
            int sectorSize = 4096;
            //Read the first 4096.. bytes?????. We already read in GetFilesystemType. Should we read it again??
            SharedAlignedBuffer buffer(sectorSize, sectorSize);

            char * pchDeltaBuffer = buffer.Get();
            if (NULL == pchDeltaBuffer)
            {
                sve = ENOMEM;
                break;
            }

            //Read one sector
            ssize_t cbRead = ACE_OS::read(handleVolume, pchDeltaBuffer, sectorSize);

            if (cbRead != sectorSize)
            {
                sve = ACE_OS::last_error();
                std::string errstr = ACE_OS::strerror(ACE_OS::last_error());
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read() failed for volume %s with error %s - %s\n", volumeName.c_str(), sve.toString(), errstr.c_str());
                break;
            }

            HideFileSystem(pchDeltaBuffer, buffer.Size());

            //Seek to the begining of the volume and write the modified buffer
#ifndef LLSEEK_NOT_SUPPORTED
            if (ACE_OS::llseek(handleVolume, 0, SEEK_SET) < 0) {
#else
            if (ACE_OS::lseek(handleVolume, 0, SEEK_SET) < 0) {
#endif
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::llseek, err = %s\n", sve.toString());
                break;
            }

            ssize_t cbWritten = ACE_OS::write(handleVolume, pchDeltaBuffer, sectorSize);

            if (cbWritten != sectorSize)
            {
                sve = ACE_OS::last_error();
                DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read()... err = %s\n", sve.toString());
                break;
            }

            }
        }while (0);

        if (sve.failed())
        {
            DebugPrintf(SV_LOG_DEBUG, "HideDrive %s failed.\n", mountdrive.c_str());
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "HideDrive %s suceeded.\n", mountdrive.c_str());
        }

    cleanup:

        if (ACE_INVALID_HANDLE != handleVolume)
        {
            sve = CloseVolumeExclusive(handleVolume, sVolumeGuid.c_str(), pszVolume, mountPoint);
            if (sve.failed())
            {
                error += "HideDrive: ";
                error += pszVolume;
                error += " failed to close handle.\n";
            }
        }

        return sve;
    }



/** PARTED */

SVERROR SetReadOnly(const char * drive)
{
    SVERROR sve = SVS_OK;
    std::string sVolName = drive;
    UnformatVolName(sVolName);
    const char * pszVolume = sVolName.c_str();

    char DriveLetter = pszVolume[0];
    std::string sGuid = sVolName;
    FormatVolumeNameToGuid(sGuid);
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));
    //    if (false == DriveLetterToGUID(DriveLetter, VolumeGUID, sizeof(VolumeGUID)) ) 
    //    {
    //        DebugPrintf(SV_LOG_ERROR, "DriveLetterToGUID failed to convert DriveLetter(%c) to GUID", DriveLetter);
    //        sve = SVS_FALSE;
    //        return sve ;
    //    }
    bool isVirtualVolume = IsSparseVolume(drive);
    sve = SetReadOnly(drive, VolumeGUID, isVirtualVolume);
    if (sve.failed())
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED to set READ ONLY flag in HideDrive.... err = %s\n", sve.toString());
    }
    return sve;
}


/** PARTED */

SVERROR ResetReadOnly(const char * drive)
{
    SVERROR sve = SVS_OK;
    std::string sVolName = drive;
    UnformatVolName(sVolName);
    const char * pszVolume = sVolName.c_str();

    char DriveLetter = pszVolume[0];
    std::string sGuid = sVolName;
    FormatVolumeNameToGuid(sGuid);
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));

    bool isVirtualVolume = IsSparseVolume(drive);
    sve = ResetReadOnly(drive, VolumeGUID, isVirtualVolume);
    if (sve.failed())
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED to reset READ ONLY flag.... err = %s\n", sve.toString());
    }

    return sve;
}







/** PARTED */

SVERROR MakeReadOnly(const char *drive, void *VolumeGUID, etBitOperation ReadOnlyBitFlag)
{
    SVERROR sve = SVS_OK;

    assert(drive != NULL);
    assert(0 != strlen(drive));
    assert(NULL != VolumeGUID);

    char DriveLetter = drive[0];

    VOLUME_FLAGS_INPUT  VolumeFlagsInput;
    VOLUME_FLAGS_OUTPUT VolumeFlagsOutput;
    BOOL    bResult;
    DWORD    dwReturn;

    DebugPrintf("Opening File %s\n", INMAGE_FILTER_DOS_DEVICE_NAME);
    // PR#10815: Long Path support
    HANDLE hVFCtrlDevice = SVCreateFile(
        INMAGE_FILTER_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );


    if (INVALID_HANDLE_VALUE == hVFCtrlDevice) {
        DebugPrintf(SV_LOG_ERROR, "CreateFile %s Failed with Error = %#x\n", INMAGE_FILTER_DOS_DEVICE_NAME, GetLastError());
        sve = SVS_FALSE;
        goto cleanup;
    }

    inm_memcpy_s(VolumeFlagsInput.VolumeGUID, sizeof(VolumeFlagsInput.VolumeGUID), VolumeGUID, (GUID_SIZE_IN_CHARS * sizeof(WCHAR)));
    VolumeFlagsInput.eOperation = ReadOnlyBitFlag;
    VolumeFlagsInput.ulVolumeFlags = VOLUME_FLAG_READ_ONLY;
    bResult = DeviceIoControl(
        hVFCtrlDevice,
        (DWORD)IOCTL_INMAGE_SET_VOLUME_FLAGS,
        &VolumeFlagsInput,
        sizeof(VOLUME_FLAGS_INPUT),
        &VolumeFlagsOutput,
        sizeof(VOLUME_FLAGS_OUTPUT),
        &dwReturn,
        NULL        // Overlapped
        );
    if (bResult) {
        if (ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf("Reset Read Only Flag on Volume %s: Succeeded\n", drive);
        else
            DebugPrintf("Set Read Only Flag on Volume %s: Succeeded\n", drive);
    }
    else {
        if (ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf(SV_LOG_ERROR, "FAILED to Reset Read Only flag on Volume %s: Failed\n", drive);
        else
            DebugPrintf(SV_LOG_ERROR, "FAILED to Set Read Only flag on Volume %s: Failed\n", drive);
        sve = SVS_FALSE;
        goto cleanup;
    }

    if (sizeof(VOLUME_FLAGS_OUTPUT) != dwReturn) {
        DebugPrintf(SV_LOG_ERROR, "FAILED while modifying Read Only flag: dwReturn(%#x) did not match VOLUME_FLAGS_OUTPUT size(%#x)\n", dwReturn, sizeof(VOLUME_FLAGS_OUTPUT));
        sve = SVS_FALSE;
        goto cleanup;
    }

cleanup:
    if (INVALID_HANDLE_VALUE != hVFCtrlDevice)
        CloseHandle(hVFCtrlDevice);

    return sve;
}



/** PARTED */

SVERROR StopFilter(const char * drive, void * VolumeGUID)
{

    SVERROR sve = SVS_OK;

    assert(drive != NULL);
    assert(0 != strlen(drive));
    assert(NULL != VolumeGUID);

    STOP_FILTERING_INPUT stopFilteringInput;
    memset(&stopFilteringInput, 0, sizeof stopFilteringInput);
    inm_wcsncpy_s(stopFilteringInput.VolumeGUID, ARRAYSIZE(stopFilteringInput.VolumeGUID), (const wchar_t *)VolumeGUID, GUID_SIZE_IN_CHARS);

    char DriveLetter = drive[0];
    DebugPrintf("Opening File %s\n", INMAGE_FILTER_DOS_DEVICE_NAME);
    // PR#10815: Long Path support
    HANDLE hVFCtrlDevice = SVCreateFile(
        INMAGE_FILTER_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );


    if (INVALID_HANDLE_VALUE == hVFCtrlDevice) {
        DebugPrintf(SV_LOG_ERROR, "CreateFile %s Failed with Error = %#x\n", INMAGE_FILTER_DOS_DEVICE_NAME, GetLastError());
        sve = SVS_FALSE;
        goto cleanup;
    }


    BOOL    bResult;
    DWORD    dwReturn;

    bResult = DeviceIoControl(
        hVFCtrlDevice,
        IOCTL_INMAGE_STOP_FILTERING_DEVICE,
        &stopFilteringInput,
        sizeof stopFilteringInput,
        NULL,
        0,
        &dwReturn,
        NULL        // Overlapped
        );
    if (bResult)
    {
        // FIX_LOG_LEVEL: note for now we are going to use SV_LOG_ERROR even on success so that we can
        // track this at the cx, eventually a new  SV_LOG_LEVEL will be added that is not an error 
        // but still allows this to be sent to the cx
        DebugPrintf(SV_LOG_ERROR, "NOTE: this is not an error StopFilter portablehelpers for volume %s\n", drive);
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Returned Failed from stop filtering DeviceIoControl call for volume %s:\n", drive);
        sve = SVS_FALSE;
        goto cleanup;
    }

cleanup:
    if (INVALID_HANDLE_VALUE != hVFCtrlDevice)
        CloseHandle(hVFCtrlDevice);

    return sve;

}



/** PARTED */

SVERROR StartFilter(const char * drive, void * VolumeGUID)
{
    SVERROR sve = SVS_OK;

    assert(drive != NULL);
    assert(0 != strlen(drive));
    assert(NULL != VolumeGUID);

    char DriveLetter = drive[0];

    DebugPrintf("Opening File %s\n", INMAGE_FILTER_DOS_DEVICE_NAME);
    // PR#10815: Long Path support
    HANDLE hVFCtrlDevice = SVCreateFile(
        INMAGE_FILTER_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );


    if (INVALID_HANDLE_VALUE == hVFCtrlDevice) {
        DebugPrintf(SV_LOG_ERROR, "CreateFile %s Failed with Error = %#x\n", INMAGE_FILTER_DOS_DEVICE_NAME, GetLastError());
        sve = SVS_FALSE;
        goto cleanup;
    }


    BOOL    bResult;
    DWORD    dwReturn;

    bResult = DeviceIoControl(
        hVFCtrlDevice,
        IOCTL_INMAGE_START_FILTERING_DEVICE,
        VolumeGUID,
        sizeof(WCHAR) * GUID_SIZE_IN_CHARS,
        NULL,
        0,
        &dwReturn,
        NULL        // Overlapped
        );
    if (bResult)
        // only log this locally as there are too many to send to cx
        DebugPrintf(SV_LOG_INFO, "NOTE: this is not an error StartFilter portablehelpers for volume %s\n", drive);
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Returned Failed from start filtering DeviceIoControl call for volume %s\n", drive);
        sve = SVS_FALSE;
        goto cleanup;
    }

cleanup:
    if (INVALID_HANDLE_VALUE != hVFCtrlDevice)
        CloseHandle(hVFCtrlDevice);

    return sve;

}


/** PARTED */

SVERROR isReadOnly(const char * drive, bool & rvalue)
{
    SVERROR sve = SVS_OK;
    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s, drive = %s\n", LINE_NO, FILE_NAME, drive);
    LocalConfigurator localConfigurator;
    if (!localConfigurator.IsFilterDriverAvailable())
    {
        DebugPrintf(SV_LOG_DEBUG, "The filter driver is not present. So considering the volume in read write mode.\n");
        rvalue = false;
        return sve;
    }

    if (NULL == drive)
    {
        sve = EINVAL;
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = EINVAL\n");
        return sve;
    }

    BOOL    bResult;
    DWORD    dwReturn;
    PVOLUME_STATS_DATA  pVolumeStatsData = NULL;

    std::string volumeName = drive;
    FormatVolumeName(volumeName);
    std::string sGuid = drive;
    FormatVolumeNameToGuid(sGuid);
    ExtractGuidFromVolumeName(sGuid);
    USES_CONVERSION;
    WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    inm_memcpy_s(VolumeGUID, sizeof(VolumeGUID), A2W(sGuid.c_str()), GUID_SIZE_IN_CHARS*sizeof(WCHAR));

    bool isVirtualVolume = IsSparseVolume(volumeName);
    if (isVirtualVolume)
    {
        return isVirtualVolumeReadOnly(VolumeGUID, rvalue);
    }


    DebugPrintf("Opening File %s\n", INMAGE_FILTER_DOS_DEVICE_NAME);
    // PR#10815: Long Path support
    HANDLE hVFCtrlDevice = SVCreateFile(
        INMAGE_FILTER_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );


    if (INVALID_HANDLE_VALUE == hVFCtrlDevice) {
        DebugPrintf(SV_LOG_ERROR, "CreateFile %s Failed with Error = %#x\n", INMAGE_FILTER_DOS_DEVICE_NAME, GetLastError());
        sve = SVS_FALSE;
        goto cleanup;
    }



    pVolumeStatsData = (PVOLUME_STATS_DATA)malloc(sizeof(VOLUME_STATS_DATA) + sizeof(VOLUME_STATS));
    if (NULL == pVolumeStatsData) {
        sve = ENOMEM;
        DebugPrintf(SV_LOG_ERROR, "FAILED isReadOnly() in memory allocation\n");
        goto cleanup;
    }

    bResult = DeviceIoControl(
        hVFCtrlDevice,
        (DWORD)IOCTL_INMAGE_GET_VOLUME_STATS,
        VolumeGUID,
        sizeof(WCHAR) * GUID_SIZE_IN_CHARS,
        pVolumeStatsData,
        sizeof(VOLUME_STATS_DATA) + sizeof(VOLUME_STATS),
        &dwReturn,
        NULL        // Overlapped
        );

    //assert(pVolumeStatsData->ulVolumesReturned == 1);

    if (bResult)
    {
        PVOLUME_STATS	VolumeStats = (PVOLUME_STATS)((PUCHAR)pVolumeStatsData + sizeof(VOLUME_STATS_DATA));
        if (VolumeStats->ulVolumeFlags & VSF_READ_ONLY)
        {
            DebugPrintf(" [INFO] VSF_READ_ONLY is set for %s\n", drive);
            rvalue = true;
        }
        else
        {
            DebugPrintf(" [INFO] VSF_READ_ONLY is not set for %s\n", drive);
            rvalue = false;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Returned Failed from GET_VOLUME_STATS DeviceIoControl call for volume %s:\n", drive);
        sve = SVS_FALSE;
        goto cleanup;
    }



cleanup:
    if (INVALID_HANDLE_VALUE != hVFCtrlDevice)
        CloseHandle(hVFCtrlDevice);

    if (pVolumeStatsData)
        free(pVolumeStatsData);


    return sve;

}



/** PARTED */

VOLUME_STATE GetVolumeState(const char * drive)
{
    DebugPrintf(SV_LOG_DEBUG, "Entered GetVolumeState\n");

    bool bReadOnly = false;

    if (NULL == drive)
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED GetVolumeState: Invalid Argument \n");
        return VOLUME_UNKNOWN;
    }

    ///
    /// First unformat the volume name to get it in the form E:
    ///
    std::string sVolName = drive;
    UnformatVolName(sVolName);
    const char * pszVolume = sVolName.c_str();

    do {
        if (!IsVolpackDriverAvailable())
        {
            std::string sparsefile;
            std::string virtualvol = drive;
            FormatVolumeName(virtualvol);
            bool new_sparsefile_format = false;
            if (IsVolPackDevice(virtualvol.c_str(), sparsefile, new_sparsefile_format))
            {
                return VOLUME_HIDDEN;
            }
        }
        int fstype;
        bool hidden = false;
        if (GetFileSystemTypeForVolume(pszVolume, fstype, hidden).succeeded())
        {
            if (hidden)
            {
                return VOLUME_HIDDEN;
            }
            else
            {
                break;
            }
        }
        else
        {
            return VOLUME_UNKNOWN;
        }
    } while (FALSE);

    // Check if it is read only
    LocalConfigurator localConfigurator;
    if (!localConfigurator.IsFilterDriverAvailable())
        return VOLUME_VISIBLE_RW;

    if (isReadOnly(pszVolume, bReadOnly).failed())
        return VOLUME_UNKNOWN;

    if (bReadOnly)
        return VOLUME_VISIBLE_RO;
    else
        return VOLUME_VISIBLE_RW;
}


/** PARTED */

bool IsValidDevfile(std::string devname)
{
    return true;
}


/** PARTED */

bool IsVolumeMounted(const std::string volume, std::string& sMountPoint, std::string &mode)
{
    bool bMounted = false;
    sMountPoint = "";
    bMounted = true;
    sMountPoint = volume;
    return bMounted;
}









/** PARTED */

//This function flushes all pending file system buffers (both data and metadata) in a volume. The argument
// should be in the form A: or B: or C: etc

SVERROR FlushFileSystemBuffers(const char *Volume)
{
    std::string volName;
    ACE_HANDLE hVolume = ACE_INVALID_HANDLE;
    SVERROR sve = SVS_OK;

    do
    {
        SV_UINT flags = 0;
        volName = Volume;

        FormatVolumeNameToGuid(volName);
        flags = O_RDWR;

        // PR#10815: Long Path support
        hVolume = ACE_OS::open(getLongPathName(volName.c_str()).c_str(), flags, ACE_DEFAULT_OPEN_PERMS);

        if (ACE_INVALID_HANDLE == hVolume)
        {
            sve = ACE_OS::last_error();
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "ACE_OS::open failed in FlushFileSystemBuffers.We still proceed %s, err = %s\n", volName.c_str(), sve.toString());
            break;
        }

        if (ACE_OS::fsync(hVolume) == -1)
        {
            sve = ACE_OS::last_error();
            DebugPrintf(SV_LOG_WARNING, "FlushFileBuffers for Volume %s did not succeed, err = %s\n", volName.c_str(), sve.toString());
        }
        else
            DebugPrintf(SV_LOG_DEBUG, "FlushFileBuffers for Volume %s Succeeded\n", volName.c_str());

        ACE_OS::close(hVolume);

    } while (FALSE);

    return sve;
}

/** PARTED */

SVERROR SVMakeSureDirectoryPathExists(const char* pszDestPathname)
{
    //TODO: re-use tal code which creates missing directories.
    ACE_Guard<ACE_Recursive_Thread_Mutex> dirCreateMutexGuard(dirCreateMutex);
    SVERROR rc = SVS_OK;
    if (NULL == pszDestPathname || 0 == pszDestPathname[0])
    {
        rc = SVE_INVALIDARG;
        return rc;
    }

    size_t pathnamelen;
    INM_SAFE_ARITHMETIC(pathnamelen = InmSafeInt<size_t>::Type(strlen(pszDestPathname)) + 2, INMAGE_EX(strlen(pszDestPathname)))
        char *szPathname = new char[pathnamelen];
    memset(szPathname, 0, pathnamelen);
    inm_strcpy_s(szPathname, pathnamelen, pszDestPathname);

    do
    {
        SVERROR hr = ReplaceChar(szPathname, '/', '\\');
        if (hr.failed())
        {
            rc = hr;
            break;
        }

        size_t cch = strlen(szPathname);

        if (cch < (MAX_PATH - 20))
        {
            if ('\\' != szPathname[cch - 1])
            {
                szPathname[cch + 1] = 0;
                szPathname[cch] = '\\';
            }

            try
            {
                CreatePaths::createPathsAsNeeded(szPathname);
            }
            catch (std::exception ex)
            {
                DebugPrintf(SV_LOG_ERROR, "%s failed: %s\n", FUNCTION_NAME, ex.what());
                rc = SVS_FALSE;
                break;
            }
        }
        else
        {
            std::wstring szPrefix;
            USES_CONVERSION;
            std::wstring tempName = A2W(szPathname);
            std::wstring::size_type index1 = 0;
            std::wstring::size_type index2;
            index2 = tempName.find(L'\\', 2);

            while (index2 < tempName.size())
            {
                index1 = index2 + sizeof(std::wstring::value_type);
                index2 = tempName.find(L'\\', index1);
                if (index2 == std::string::npos)
                    index2 = tempName.size();
                if (szPathname[0] == '\\' && szPathname[1] == '\\' &&
                    szPathname[2] == '?' &&	szPathname[3] == '\\')
                {
                    szPrefix = L"";
                }
                else
                {
                    szPrefix = L"\\\\?\\";
                }
                szPrefix += tempName.substr(0, index2);

                //remove repeting '\\' if any
                std::wstring::size_type index = -1;
                while ((index = szPrefix.find(L'\\', ++index)) != szPrefix.npos)
                    if (index >= 4 && szPrefix[index - 1] == L'\\')
                        szPrefix.erase(szPrefix.begin() + index--);

                DebugPrintf(SV_LOG_DEBUG, "Directory Path: %S\n", szPrefix.c_str());
                if (szPrefix.size() > 0)
                {
                    bool bCreated = CreateDirectoryW(szPrefix.c_str(), 0);
                    if (!bCreated)
                    {
                        DWORD dwError = GetLastError();
                        if (dwError != ERROR_ALREADY_EXISTS)
                        {
                            DebugPrintf(SV_LOG_ERROR, "FAILED Couldn't mkdir %S, "
                                "error = %d\n", szPrefix.c_str(), dwError);
                            rc = dwError;
                            break;
                        }
                    }
                }
            }
        }
    } while (FALSE);

    delete[] szPathname;
    return(rc);
}


/** PARTED */

//-----------------------------------------------------------------------------
// This is a function that works under W2K and later to get the correct volume
// size on all flavors of volumes (static/dynamic, raid/striped/mirror/spanned)
// On WXP and later, IOCTL_DISK_GET_LENGTH_INFO already does this, so that is
// what we try first. This code is a direct translation to user mode of some
// kernel code in host\driver\win32\shared\MntMgr.cpp
//
// IMPORTANT: It turns out from user mode, a file system will slightly reduce
// the size of a volume by hiding some blocks. You need to call the ioctl
// DeviceIoControl(volumeHandle,
//                 FSCTL_ALLOW_EXTENDED_DASD_IO, 
//                 NULL, 0, NULL, 0, &status, NULL);
// on any volume that uses this function, or expect to read or write ALL of a
// volume. This tells the filesystem to let the underlying volume do the bounds
// checking, not the filesystem This ioctl will probably fail on raw volumes,
// just ignore the error and call it on every volume. You may 
// need #define _WIN32_WINNT 0x0510 before your headers to get this ioctl
//
// This function expects as input a handle opened on a volume with at least
// read access. A status value from GetLastErrro() will be return if needed
// The volume size will be in bytes, although always a multiple of 512
//-----------------------------------------------------------------------------
SVERROR GetVolumeSize(ACE_HANDLE volumeHandle,
    unsigned long long* volumeSize)
{

    SVERROR sve = SVS_OK;


#define MAXIMUM_SECTORS_ON_VOLUME (1024i64 * 1024i64 * 1024i64 * 1024i64)
#define DISK_SECTOR_SIZE (512)

    PVOLUME_DISK_EXTENTS pDiskExtents; // dynamically sized allocation based on number of extents
    GET_LENGTH_INFORMATION  lengthInfo;
    DWORD bytesReturned;
    DWORD returnBufferSize;
    BOOLEAN fSuccess;
    LONGLONG currentGap;
    LONGLONG currentOffset;
    LARGE_INTEGER largeInt;
    PUCHAR sectorBuffer;

    // try a simple IOCTl to get size, should work on WXP and later
    fSuccess = DeviceIoControl(volumeHandle,
        IOCTL_DISK_GET_LENGTH_INFO,
        NULL,
        0,
        &lengthInfo,
        sizeof(lengthInfo),
        &bytesReturned,
        NULL);
#if (PRINT_SIZES)
    printf("Volume size via IOCTL_DISK_GET_LENGTH_INFO is hex:%I64X or decimal:%I64i\n", lengthInfo.Length.QuadPart, lengthInfo.Length.QuadPart);
#endif 

#if (TEST_GET_EXTENTS || TEST_BINARY_SEARCH || TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (fSuccess) {
        *volumeSize = lengthInfo.Length.QuadPart;
        return SVS_OK;
    }

    // next attempt, see if the volume is only 1 extent, if yes use it
    returnBufferSize = sizeof(VOLUME_DISK_EXTENTS);
    pDiskExtents = (PVOLUME_DISK_EXTENTS) new UCHAR[returnBufferSize];
    fSuccess = DeviceIoControl(volumeHandle,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL,
        0,
        pDiskExtents,
        returnBufferSize,
        &bytesReturned,
        NULL);
#if (PRINT_SIZES)
    printf("Volume size via IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS is hex:%I64X or decimal:%I64i\n", pDiskExtents->Extents[0].ExtentLength.QuadPart, pDiskExtents->Extents[0].ExtentLength.QuadPart);
#endif
#if (TEST_BINARY_SEARCH || TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (fSuccess) {
        // must only have 1 extent as the buffer only has space for one
        assert(pDiskExtents->NumberOfDiskExtents == 1);
        *volumeSize = pDiskExtents->Extents[0].ExtentLength.QuadPart;
        delete pDiskExtents;
        return SVS_OK;
    }

    // use harder ways of finding the size
    sve = GetLastError();
#if (TEST_BINARY_SEARCH)
    sve = ERROR_MORE_DATA;
#endif
    if (ERROR_MORE_DATA != sve.error.hr) {
        // some error other than buffer too small happened
        delete pDiskExtents;
        return sve;
    }

    // has multiple extents
    DWORD nde;
    INM_SAFE_ARITHMETIC(nde = InmSafeInt<DWORD>::Type(pDiskExtents->NumberOfDiskExtents) - 1, INMAGE_EX(pDiskExtents->NumberOfDiskExtents))
        INM_SAFE_ARITHMETIC(returnBufferSize = sizeof(VOLUME_DISK_EXTENTS) + (nde * InmSafeInt<size_t>::Type(sizeof(DISK_EXTENT))), INMAGE_EX(sizeof(VOLUME_DISK_EXTENTS))(nde)(sizeof(DISK_EXTENT)))
        delete pDiskExtents;
    pDiskExtents = (PVOLUME_DISK_EXTENTS) new UCHAR[returnBufferSize];
    fSuccess = DeviceIoControl(volumeHandle,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL, 0,
        pDiskExtents, returnBufferSize,
        &bytesReturned,
        NULL);
#if (TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (!fSuccess) {
        currentOffset = MAXIMUM_SECTORS_ON_VOLUME; // if IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed, do a binary search with a large limit
    }
    else {
        // must be multiple extents, so will have to binary search last valid sector

        currentOffset = 0;
        for (unsigned int i = 0; i < pDiskExtents->NumberOfDiskExtents; i++) {
            currentOffset += pDiskExtents->Extents[i].ExtentLength.QuadPart;
        }

        // scale things now so we don't need to divide on every read
        currentOffset += DISK_SECTOR_SIZE; // these two are needed to make the search algorithm work
        currentOffset /= DISK_SECTOR_SIZE;
    }

    delete pDiskExtents;

    // the search gap MUST be a power of two, so find an appropriate value
    currentGap = 1i64;
    while ((currentGap * 2i64) < currentOffset) {
        currentGap *= 2i64;
    }

    // we are all ready to binary search for the last valid sector
    sectorBuffer = (PUCHAR)VirtualAlloc(NULL, DISK_SECTOR_SIZE, MEM_COMMIT, PAGE_READWRITE);
    do {
        largeInt.QuadPart = currentOffset * DISK_SECTOR_SIZE;
        SetFilePointerEx(volumeHandle,
            largeInt,
            NULL,
            FILE_BEGIN);

        fSuccess = ReadFile(volumeHandle,
            sectorBuffer,
            DISK_SECTOR_SIZE,
            &bytesReturned,
            NULL);

        if (fSuccess && (bytesReturned == DISK_SECTOR_SIZE)) {
            currentOffset += currentGap;
            if (currentGap == 0) {
                *volumeSize = (currentOffset * DISK_SECTOR_SIZE) + DISK_SECTOR_SIZE;
                sve = SVS_OK;
                break;
            }
        }
        else {
            currentOffset -= currentGap;
            if (currentGap == 0) {
                *volumeSize = currentOffset * DISK_SECTOR_SIZE;
                sve = SVS_OK;
                break;
            }
        }
        currentGap /= 2;

    } while (TRUE);

    VirtualFree(sectorBuffer, DISK_SECTOR_SIZE, MEM_DECOMMIT);

#if (PRINT_SIZES)
    printf("Volume size via binary search is hex:%I64X or decimal:%I64i\n", *volumeSize, *volumeSize);
#endif


    return sve;
}


/** PARTED */

///
/// Returns the total bytes in the volume (not the free space).
/// 0 returned upon failure.
///
SV_LONGLONG GetVolumeCapacity(const char *volume)
{
    SV_LONGLONG capacity = 0;

    std::string volName = volume;
    if (IsDrive(volName))
    {
        UnformatVolumeNameToDriveLetter(volName);
        volName += ":\\";
    }
    else if (IsMountPoint(volName) && ('\\' != volName[volName.length() - 1]))
    {
        volName += "\\";
    }
    ULARGE_INTEGER uliVolumeCapacity;
    // PR#10815: Long Path support
    if (SVGetDiskFreeSpaceEx(volName.c_str(), NULL, &uliVolumeCapacity, NULL))
    {
        capacity = uliVolumeCapacity.QuadPart;
    }
    else
    {
        capacity = 0;
        DebugPrintf(SV_LOG_WARNING, "WARN: An error occurred in determining the capacity of volume %s (OK if offline cluster volume). %s.@LINE %d in FILE %s\n", volume, Error::Msg().c_str(), LINE_NO, FILE_NAME);
    }

    return(capacity);
}


/** PARTED */
std::string GetVolumeDirName(std::string volName)
{
    std::string dirname;

    dirname = volName;
    if (IsDrive(dirname))
    {
        UnformatVolumeNameToDriveLetter(dirname);
    }
    else if (IsMountPoint(dirname))
    {
        std::replace(dirname.begin(), dirname.end(), '\\', '/');
        dirname.erase(std::remove_if(dirname.begin(), dirname.end(), IsColon), dirname.end());
        dirname.erase(std::remove_if(dirname.begin(), dirname.end(), IsQuestion), dirname.end());
    }

    return dirname;
}








/** PARTED */
void StringToSVLongLong(SV_LONGLONG& num, char const* psz)
{
    assert(NULL != psz);
    sscanf(psz, "%I64d", &num);
}



/** PARTED */
//
// NOTE: assumes psz has enough space to hold string
//
char* SVLongLongToString(char* psz, SV_LONGLONG num, size_t size)
{
    assert(NULL != psz);
    inm_sprintf_s(psz, size, "%I64d", num);
    return(psz);
}













/** PARTED */
void FormatVolumeName(std::string& sName, bool bUnicodeFormat)
{
    if (IsVolumeNameInGuidFormat(sName))
        return;

    if (IsDrive(sName))
    {
        if ((true == bUnicodeFormat) && (false == sName.empty()) && (sName.find("\\\\.\\", 0) == std::string::npos))
        {
            if (sName.length() == 1 || (2 == sName.length() && ':' == sName[1]) || (3 == sName.length() && ':' == sName[1] && '\\' == sName[2]))
            {
                //
                // If volume name is a drive letter, map it to \\.\\X:
                // otherwise, just leave it as the device directory (e.g. x:\mnt\my_dir)
                //
                sName = "\\\\.\\" + sName;

                if (sName.rfind(":") == std::string::npos)
                {
                    sName = sName + ":";
                }

                std::string::size_type len = sName.length();
                std::string::size_type idx = len;
                if ('\\' == sName[len - 1] && ':' == sName[len - 2]) {
                    --idx;
                }

                if (idx < len) {
                    sName.erase(idx);
                }
            }
        }
        else
        {
            if (sName.rfind(":") == std::string::npos)
                sName += ":";
            if (sName.rfind("\\") == std::string::npos)
                sName += "\\";
        }
    }
    else if (IsMountPoint(sName))
        FormatVolumeNameForMountPoint(sName);
}


/** PARTED */
bool IsDrive(const std::string& sDrive)
{
    if ((sDrive.length() <= 3) && (isalpha(sDrive[0])))
        return true;
    else if (sDrive.length() > 3)
    {
        if (('\\' == sDrive[0]) &&
            ('\\' == sDrive[1]) &&
            ((sDrive[2] == '?') || (sDrive[2] == '.')) &&
            (sDrive[5] == ':') &&
            (isalpha(sDrive[4])))
            return true;
    }
    return false;
}


/** PARTED */
bool IsVolumeNameInGuidOrGlobalGuidFormat(const std::string& sVolume)
{
    return (IsVolumeNameInGuidFormat(sVolume) || IsVolumeNameInGlobalGuidFormat(sVolume));
}


/** PARTED */
bool IsVolumeNameInGlobalGuidFormat(const std::string& sVolume)
{
    if (sVolume.length() > 3)
    {
        if ((sVolume.find("{") != std::string::npos) && (sVolume.find("}") != std::string::npos))
        {
            if (sVolume.find("\\??\\") != std::string::npos)
                return true;
        }
        else if ((sVolume.find("-") != std::string::npos && sVolume.find("\\") == std::string::npos) && (isalpha(sVolume[0]) || isdigit(sVolume[0])))
            return true;
    }
    return false;
}


/** PARTED */
bool IsVolumeNameInGuidFormat(const std::string& sVolume)
{
    if (sVolume.length() > 3)
    {
        if ((sVolume.find("{") != std::string::npos) && (sVolume.find("}") != std::string::npos))
        {
            if ((sVolume.find("\\\\.\\") != std::string::npos) || (sVolume.find("\\\\?\\") != std::string::npos))
                return true;
        }
        else if ((sVolume.find("-") != std::string::npos && sVolume.find("\\") == std::string::npos) && (isalpha(sVolume[0]) || isdigit(sVolume[0])))
            return true;
    }
    return false;
}


/** PARTED */
bool IsMountPoint(const std::string& sVolume)
{
    if (sVolume.length() > 3)
    {
        if ((sVolume.find("{") != std::string::npos) && (sVolume.find("}") != std::string::npos))
            return true;
        else if (('\\' == sVolume[0]) &&
            ('\\' == sVolume[1]) &&
            ((sVolume[2] == '?') || (sVolume[2] == '.')) &&
            (sVolume.length() > 6) &&
            (isalpha(sVolume[7])))
            return true;
        else if (isalpha(sVolume[0]))
            //	else if (isalpha(sVolume[0]) && (isalpha(sVolume[3])))
            return true;
    }
    return false;
}


/** PARTED */
bool FormatVolumeNameToGuidUtf8(std::string& sVolumeName)
{
    //
    // Given a mount point, return \\?\Volume{guid} as the "unformatted" name
    //
    std::string sName = sVolumeName;

    // Here passing Utf8 string sVolumeName to on Utf8 version of functions like 
    // IsVolumeNameInGuidFormat, IsMountPoint, IsDrive, UnformatVolumeNameToDriveLetter.
    // This is because these fucntions check letters such as driver letters, colons,
    // slashes etc. which are same in both ascii and Utf8 format.
    if (!IsVolumeNameInGuidFormat(sName))
    {
        if (IsMountPoint(sName))
        {
            if (sName[sName.length() - 1] == '\\')
                sName.erase(sName.end() - 1);
        }
        else if (IsDrive(sName))
        {
            UnformatVolumeNameToDriveLetter(sName);
            sName += ":";
        }
        else if ((sName[0] == '\\') && (sName[1] == '\\'))
        {
            DebugPrintf(SV_LOG_DEBUG, "volume name %s is a network drive.\n", sVolumeName.c_str());
            return true;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "volume name %s is neither drive nor a mountpoint.\n", sVolumeName.c_str());
            return false;
        }

        std::string mountPoint = sName + "\\";
        wchar_t wszVolumeName[SV_MAX_PATH];
        memset((void*)wszVolumeName, '\000', sizeof(wszVolumeName));
        if (GetVolumeNameForVolumeMountPointW(convertUtf8ToWstring(mountPoint).c_str(), wszVolumeName, NELEMS(wszVolumeName)))
        {
            sVolumeName = convertWstringToUtf8(wszVolumeName);
            if (sVolumeName.length() > 0)
            {
                //
                // Remove trailing \ because CreateFile() expects it in that format
                //
                if (sVolumeName[sVolumeName.length() - 1] == '\\')
                    sVolumeName.erase(sVolumeName.end() - 1);
            }
        }
        else
        {
            DWORD err = GetLastError();
            DebugPrintf(SV_LOG_ERROR, "GetVolumeNameForVolumeMountPointW failed. Error: %d. @LINE %d in FILE %s \n", err, LINE_NO, FILE_NAME);

            std::string sparsefile;
            bool new_sparsefile_format = false;
            if (!IsVolpackDriverAvailable() && (IsVolPackDevice(sName.c_str(), sparsefile, new_sparsefile_format)))
            {
                sVolumeName = sparsefile;
                return true;
            }
            else
            {
                wszVolumeName[0] = 0;
                return false;
            }
        }
    }
    else
    {
        if (sName.length() > 0)
        {
            //
            // Remove trailing \ because CreateFile() expects it in that format
            //
            if (sName[sName.length() - 1] == '\\')
                sName.erase(sName.end() - 1);
        }
        sVolumeName = sName;
    }
    return true;
}

/** PARTED */
bool FormatVolumeNameToGuid(std::string& sVolumeName)
{
    //
    // Given a mount point, return \\?\Volume{guid} as the "unformatted" name
    //
    std::string sName = sVolumeName;

    if (!IsVolumeNameInGuidFormat(sName))
    {
        char szVolumeName[SV_MAX_PATH];
        std::string mountPoint = "";
        if (IsMountPoint(sName))
        {
            if (sName[sName.length() - 1] == '\\')
                sName.erase(sName.end() - 1);
        }
        else if (IsDrive(sName))
        {
            UnformatVolumeNameToDriveLetter(sName);
            sName += ":";
        }
        else if ((sName[0] == '\\') && (sName[1] == '\\'))
        {
            DebugPrintf(SV_LOG_DEBUG, "volume name %s is a network drive.\n", sVolumeName.c_str());
            return true;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "volume name %s is neither drive nor a mountpoint.\n", sVolumeName.c_str());
            return true;
        }

        mountPoint = sName + "\\";
        if (GetVolumeNameForVolumeMountPointA(mountPoint.c_str(), szVolumeName, sizeof(szVolumeName)))
        {
            sName = szVolumeName;

            if (sName.length() > 0)
            {
                //
                // Remove trailing \ because CreateFile() expects it in that format
                //
                if (sName[sName.length() - 1] == '\\')
                    sName.erase(sName.end() - 1);
            }
            sVolumeName = sName;
        }
        else
        {
            std::string sparsefile;
            bool new_sparsefile_format = false;
            if (!IsVolpackDriverAvailable() && (IsVolPackDevice(sName.c_str(), sparsefile, new_sparsefile_format)))
            {
                sVolumeName = sparsefile;
                return true;
            }
            else
            {
                szVolumeName[0] = 0;
                return false;
            }
        }
    }
    else
    {
        if (sName.length() > 0)
        {
            //
            // Remove trailing \ because CreateFile() expects it in that format
            //
            if (sName[sName.length() - 1] == '\\')
                sName.erase(sName.end() - 1);
        }
        sVolumeName = sName;
    }
    return true;
}


/** PARTED */
void UnformatVolumeNameToDriveLetter(std::string& sVolName)
{
    size_t pos = 0;
    if ((false == sVolName.empty()) && (!IsVolumeNameInGuidFormat(sVolName)))
    {
        if (1 == sVolName.size())
        {
            return;
        }
        else if ((pos = sVolName.find("\\\\.\\", 0)) != std::string::npos)
        {
            sVolName.replace(pos, 4, "");
            //            if ( (pos = sVolName.find(":") ) != std::string::npos )
            //            {   
            //	            sVolName.replace(pos,1,"");
            //            }
        }
        else if ((pos = sVolName.find("\\\\?\\", 0)) != std::string::npos)
        {
            sVolName.replace(pos, 4, "");
        }

        if ((sVolName.length() == 3) && (sVolName[2] == '\\'))
        {
            sVolName.erase(sVolName.end() - 1);
            if (sVolName[1] == ':')
                sVolName.erase(sVolName.end() - 1);
        }
        else if ((sVolName.length() == 2) && (sVolName[1] == ':'))
        {
            sVolName.erase(sVolName.end() - 1);
        }
    }
}


/** PARTED */
void FormatDeviceName(std::string& sName)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    if ((false == sName.empty()) && (sName.find("\\\\.\\", 0) == std::string::npos))
    {
        sName = "\\\\.\\" + sName;
        //		if ( sName->find_last_of(":\\",sName->length()) == std::string::npos )
        //		{
        //			*sName = *sName + ":\\";
        //		}
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

}


/** PARTED */
void FormatNameToUNC(std::string& sName)
{
    if ((false == sName.empty()) && (sName.find("\\?", 0) == std::string::npos))
    {
        sName = "\\\\?\\" + sName;
        if (sName.rfind(":\\", 0) == std::string::npos)
        {
            sName = sName + ":\\";
        }
    }
}



/** PARTED */
void ToStandardFileName(std::string& sFileName)
{
    size_t pos = 0;
    if (false == sFileName.empty())
    {
        if ((pos = sFileName.find("\\\\.\\", 0)) != std::string::npos)
        {
            sFileName.replace(pos, 4, "");
        }

        if (((pos = sFileName.find(":")) == std::string::npos) && (sFileName.length() == 1))
        {
            sFileName = sFileName + ":";
        }

        if (((pos = sFileName.find("\\")) == std::string::npos) && (sFileName.length() == 2))
        {
            sFileName = sFileName + "\\";
        }
    }
}


/** PARTED */
bool IsUNCFormat(const std::string& sName)
{
    size_t pos = 0;
    if (false == sName.empty())
    {
        if ((pos = sName.find("\\\\.\\", 0)) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}


/** PARTED */
// --------------------------------------------------------------------------
// format volume name the way MS expects for volume mount point APIs. 
// final format should be one of 
//  <drive>:\
//  <drive>:\<mntpoint>\
// where: <drive> is the drive letter and <mntpoint> is the mount point name
// e.g. 
//  A:\ for a drive letter
//  B:\mnt\data\ for a mount point
// --------------------------------------------------------------------------
void FormatVolumeNameForMountPoint(std::string& volumeName)
{
    //
    // Commmented this code as it strips off the leading "\\?\" from UnformattedVolumeName for mount point
    //
    // we need to strip off any leading \, ., ? if they exists
    /* std::string::size_type idx = volumeName.find_first_not_of("\\.?");
    if (std::string::npos != idx) {
    volumeName.erase(0, idx);
    }*/

    std::string::size_type len = volumeName.length();
    // TODO:
    // may want more verification that we actually have
    // valid format and only need to possibly add ':' and/or '\\'
    if (1 == len) {
        // assume it is a drive letter
        volumeName += ":\\";
    }
    else {
        // need to make sure it ends with a '\\'
        if ('\\' != volumeName[len - 1]) {
            volumeName += '\\';
        }
    }
}


/** PARTED */

// For Sparse file mounted virtual volumes VSNAP_UNIQUE_ID_PREFIX is not used. 
// So, separate function is written.
bool IsSparseVolume(std::string Vol)
{
    std::string sparsefile;
    bool new_sparsefile_format = false;
    return IsVolPackDevice(Vol.c_str(), sparsefile, new_sparsefile_format);
}

bool EnableDisableCompresson(const std::string & filename, DWORD flags, SV_USHORT compression_format)
{
    bool rv = true;

    do
    {
        HANDLE handle = INVALID_HANDLE_VALUE;
        handle = SVCreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, flags, NULL);
        bool bExist = IS_INVALID_HANDLE_VALUE(handle) == true ? false : true;
        if (!bExist)
        {
            DebugPrintf(SV_LOG_ERROR, "open/creation of %s failed with error: %d.\n",
                filename.c_str(), GetLastError());
            rv = false;
            break;
        }
        SV_ULONG BytesReturned = 0;
        bool result = DeviceIoControl(handle, FSCTL_SET_COMPRESSION, &compression_format, sizeof(SV_USHORT), NULL, 0, &BytesReturned, NULL);
        if (!result)
        {
            DebugPrintf(SV_LOG_ERROR, "ioctl FSCTL_SET_COMPRESSION failed on %s with error %d\n", filename.c_str(), GetLastError());
            SAFE_CLOSEHANDLE(handle);
            rv = false;
            break;
        }
        SAFE_CLOSEHANDLE(handle);
    } while (false);
    return rv;
}

bool DisableCompressonOnDirectory(const std::string & dirname)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    SV_USHORT compression_format = COMPRESSION_FORMAT_NONE;
    bool rv = EnableDisableCompresson(dirname, flags, compression_format);
    if (rv)
        DebugPrintf(SV_LOG_INFO, "Compression disabled Successfully for %s.\n", dirname.c_str());

    return rv;
}

bool EnableCompressonOnDirectory(const std::string & dirname)
{
    DWORD flags = FILE_FLAG_BACKUP_SEMANTICS;
    SV_USHORT compression_format = COMPRESSION_FORMAT_DEFAULT;
    bool rv = EnableDisableCompresson(dirname, flags, compression_format);
    if (rv)
        DebugPrintf(SV_LOG_INFO, "Compression enabled Successfully for %s.\n", dirname.c_str());

    return rv;
}

bool DisableCompressonOnFile(const std::string & filename)
{
    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    SV_USHORT compression_format = COMPRESSION_FORMAT_NONE;
    bool rv = EnableDisableCompresson(filename, flags, compression_format);
    if (rv)
        DebugPrintf(SV_LOG_INFO, "Compression disabled Successfully for %s.\n", filename.c_str());

    return rv;
}

bool EnableCompressonOnFile(const std::string & filename)
{
    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    SV_USHORT compression_format = COMPRESSION_FORMAT_DEFAULT;
    bool rv = EnableDisableCompresson(filename, flags, compression_format);
    if (rv)
        DebugPrintf(SV_LOG_INFO, "Compression enabled Successfully for %s.\n", filename.c_str());

    return rv;
}

/** PARTED */

//TODO: identify the proper location and move this routine 
//      to appropriate os specific directories
bool GetVolumeRootPath(const std::string & path, std::string & strRoot)
{
    strRoot.resize(SV_INTERNET_MAX_PATH_LENGTH, '\0');

    // PR#10815: Long Path support
    return SVGetVolumePathName(path.c_str(), &strRoot[0], strRoot.length());
}


/** PARTED */

// Function: IsLeafDevice
// 
//  On windows:
//  if the volume does not contain any child volumes mounted on a path within it
//		return true
//  else returns false
// 
//  On Linux:
//   if it is a leaf device
//		return true
//	else return false

bool IsLeafDevice(const std::string & DeviceName)
{
    DebugPrintf(SV_LOG_DEBUG, "Entered %s\n", FUNCTION_NAME);
    std::string devname = DeviceName;
    bool rv = true;

    do
    {
        FormatVolumeNameForCxReporting(devname);
        FormatVolumeNameForMountPoint(devname);

        char MountPointBuffer[SV_MAX_PATH + 1];
        HANDLE hMountPoint = FindFirstVolumeMountPoint(devname.c_str(), MountPointBuffer,
            sizeof(MountPointBuffer) - 1);
        if (INVALID_HANDLE_VALUE == hMountPoint)
        {
            rv = true;
            break;
        }
        else
        {
            rv = false;
            FindVolumeMountPointClose(hMountPoint);
            break;
        }
    } while (0);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return rv;

}


/** PARTED */

//
// Gets the root device name from a symbolic link
//
bool GetDeviceNameFromSymLink(std::string &deviceName)
{
    return true;
}



/** PARTED */

//
// Function: GetDeviceNameForMountPt
//  given a mount point, convert it to corresponding device name
//
bool GetDeviceNameForMountPt(const std::string & mtpt, std::string & devName)
{
    devName = mtpt;
    return true;
}


/** PARTED */

SVERROR MakeVirtualVolumeReadOnly(const char *mountpoint, void * volumeGuid, etBitOperation ReadOnlyBitFlag)
{
    SVERROR               sve = SVS_OK;
    ACE_HANDLE	          VVCtrlDevice = ACE_INVALID_HANDLE;
    bool			      bResult;

    VVVOLUME_FLAGS_INPUT  vvVolumeFlagsInput;
    VVVOLUME_FLAGS_OUTPUT vvVolumeFlagsOutput;
    DWORD				  dwReturn;

    // PR#10815: Long Path support
    VVCtrlDevice = SVCreateFile(
        VV_CONTROL_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        NULL,
        NULL
        );


    if (ACE_INVALID_HANDLE == VVCtrlDevice)
        return false;

    inm_memcpy_s(vvVolumeFlagsInput.VolumeGUID, sizeof(vvVolumeFlagsInput.VolumeGUID), volumeGuid, GUID_SIZE_IN_CHARS*sizeof(WCHAR));
    vvVolumeFlagsInput.eOperation = ReadOnlyBitFlag;
    vvVolumeFlagsInput.ulControlFlags = VVOLUME_FLAG_READ_ONLY;

    bResult = DeviceIoControl(VVCtrlDevice,
        IOCTL_INMAGE_SET_VVVOLUME_FLAG,
        &vvVolumeFlagsInput,
        sizeof(VVVOLUME_FLAGS_INPUT),
        &vvVolumeFlagsOutput,
        sizeof(VVVOLUME_FLAGS_OUTPUT),
        &dwReturn,
        NULL);

    if (bResult) {
        if (ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf("Reset Read Only Flag on Volume %s: Succeeded\n", mountpoint);
        else
            DebugPrintf("Set Read Only Flag on Volume %s: Succeeded\n", mountpoint);
    }
    else {
        if (ReadOnlyBitFlag == ecBitOpReset)
            DebugPrintf(SV_LOG_ERROR, "FAILED to Reset Read Only flag on Volume %s: Failed\n", mountpoint);
        else
            DebugPrintf(SV_LOG_ERROR, "FAILED to Set Read Only flag on Volume %s: Failed\n", mountpoint);
        sve = SVS_FALSE;
        goto cleanup;
    }

    if (sizeof(VOLUME_FLAGS_OUTPUT) != dwReturn) {
        DebugPrintf(SV_LOG_ERROR, "FAILED while modifying Read Only flag: dwReturn(%#x) did not match VOLUME_FLAGS_OUTPUT size(%#x)\n", dwReturn, sizeof(VOLUME_FLAGS_OUTPUT));
        sve = SVS_FALSE;
        goto cleanup;
    }


cleanup:
    ACE_OS::close(VVCtrlDevice);
    return sve;
    return sve;
}



/** PARTED */

SVERROR isVirtualVolumeReadOnly(void * VolumeGUID, bool & rvalue)
{

    SVERROR               sve = SVS_OK;
    ACE_HANDLE	          VVCtrlDevice = ACE_INVALID_HANDLE;
    bool			      bResult;

    VVVOLUME_FLAGS_INPUT  vvVolumeFlagsInput;
    VVVOLUME_FLAGS_OUTPUT vvVolumeFlagsOutput;
    DWORD				  dwReturn;

    //std::string sGuid =drive;
    //FormatVolumeNameToGuid(sGuid);
    //ExtractGuidFromVolumeName(sGuid);
    //USES_CONVERSION;
    //WCHAR   VolumeGUID[GUID_SIZE_IN_CHARS + 1];

    // PR#10815: Long Path support
    VVCtrlDevice = SVCreateFile(
        VV_CONTROL_DOS_DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        NULL,
        NULL
        );


    if (ACE_INVALID_HANDLE == VVCtrlDevice)
        return false;

    inm_memcpy_s(vvVolumeFlagsInput.VolumeGUID, sizeof(vvVolumeFlagsInput.VolumeGUID), VolumeGUID, GUID_SIZE_IN_CHARS*sizeof(WCHAR));
    vvVolumeFlagsInput.eOperation = ecBitOpNotDefined;
    vvVolumeFlagsInput.ulControlFlags = VVOLUME_FLAG_READ_ONLY;

    bResult = DeviceIoControl(VVCtrlDevice,
        IOCTL_INMAGE_SET_VVVOLUME_FLAG,
        &vvVolumeFlagsInput,
        sizeof(VVVOLUME_FLAGS_INPUT),
        &vvVolumeFlagsOutput,
        sizeof(VVVOLUME_FLAGS_OUTPUT),
        &dwReturn,
        NULL);

    if (!bResult)
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED while reading \n ");
        sve = SVS_FALSE;
        goto cleanup;
    }

    if (sizeof(VOLUME_FLAGS_OUTPUT) != dwReturn) {
        DebugPrintf(SV_LOG_ERROR, "FAILED while modifying Read Only flag: dwReturn(%#x) did not match VOLUME_FLAGS_OUTPUT size(%#x)\n", dwReturn, sizeof(VOLUME_FLAGS_OUTPUT));
        sve = SVS_FALSE;
        goto cleanup;
    }

    if (vvVolumeFlagsOutput.ulControlFlags & VVOLUME_FLAG_READ_ONLY)
        rvalue = true;
    else
        rvalue = false;


cleanup:
    ACE_OS::close(VVCtrlDevice);
    return sve;
    return sve;

}



/** PARTED */

bool IsProcessRunning(const std::string &sProcessName, uint32_t numExpectedInstances)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
        return false;

    int iNumberOfInstances = 0;
    // Calculate how many process identifiers were returned.

    cProcesses = cbNeeded / sizeof(DWORD);

    // Print the name and process identifier for each process.

    for (i = 0; i < cProcesses; i++)
    {

        TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

        // Get a handle to the process.

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
            PROCESS_VM_READ,
            FALSE, aProcesses[i]);

        // Get the process name.

        if (NULL != hProcess)
        {
            HMODULE hMod;
            DWORD cbBytes;

            if (EnumProcessModules(hProcess, &hMod, sizeof(hMod),
                &cbBytes))
            {
                GetModuleBaseName(hProcess, hMod, szProcessName,
                    sizeof(szProcessName) / sizeof(TCHAR));
                if (szProcessName == sProcessName)
                {
                    ++iNumberOfInstances;
                    DebugPrintf(SV_LOG_DEBUG, "Name: %s PID: %u\n", sProcessName.c_str(), aProcesses[i]);
                }
            }
        }

        // Print the process name and identifier.

        //_tprintf( TEXT("%s  (PID: %u)\n"), szProcessName, aProcesses[i] );


        CloseHandle(hProcess);
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return (iNumberOfInstances > numExpectedInstances);
}


/** PARTED */

dev_t GetDevNumber(std::string devname)
{
    return 0;
}


/** PARTED */

/*
* FUNCTION NAME :  FirstCharToUpperForWindows
*
* DESCRIPTION : covert the first character to upper case for windows volume names
*
*
* INPUT PARAMETERS : target volume name
*
* OUTPUT PARAMETERS : target volume name
*
* NOTES :
*
*
* return value : none
*
*/
void FirstCharToUpperForWindows(std::string & volume)
{
    volume[0] = toupper(volume[0]);
}


bool GetSysVolCapacityAndFreeSpace(std::string &sysVol, unsigned long long &sysVolFreeSpace,
    unsigned long long &sysVolCapacity)
{
    std::string sysDir;
    // Note for CS Legacy, sending sysDir as sysVol
    return GetSysVolCapacityAndFreeSpace(sysDir, sysVol, sysVolFreeSpace, sysVolCapacity);
}

bool GetSysVolCapacityAndFreeSpace(std::string &sysVol, std::string &sysDir, unsigned long long &sysVolFreeSpace,
    unsigned long long &sysVolCapacity)
{
    bool bretval = true;
    ULARGE_INTEGER uliQuota = { 0 };
    ULARGE_INTEGER uliTotalCapacity = { 0 };
    ULARGE_INTEGER uliFreeSpace = { 0 };
    char systemVolume[5];
    memset(systemVolume, 0, 5);
    DWORD dwSystemDrives;
    TCHAR szSystemDirectory[SV_MAX_PATH + 1];
    if (0 == GetWindowsDirectory(szSystemDirectory, ARRAYSIZE(szSystemDirectory)))
    {
        //TODO: Log error msg to cx
        DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, GetWindowsDirectory failed\n", LINE_NO,
            FILE_NAME);
        dwSystemDrives = 0;
        bretval = false;
    }
    else
    {
        dwSystemDrives = 1 << (toupper(szSystemDirectory[0]) - 'A');
        sysDir = szSystemDirectory;
        systemVolume[0] = szSystemDirectory[0];
        inm_strcat_s(systemVolume, ARRAYSIZE(systemVolume), ":\\");
        sysVol = systemVolume;

        // PR#10815: Long Path support
        if (!SVGetDiskFreeSpaceEx(systemVolume, &uliQuota, &uliTotalCapacity, &uliFreeSpace))
        {
            DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, GetDiskFreeSpaceEx failed\n", LINE_NO,
                FILE_NAME);
            bretval = false;
        }
        else
        {
            sysVolFreeSpace = (unsigned long long)uliFreeSpace.QuadPart;
            sysVolCapacity = (unsigned long long)uliTotalCapacity.QuadPart;
        }
    }

    return bretval;
}



bool GetInstallVolCapacityAndFreeSpace(const std::string &installPath,
    unsigned long long &insVolFreeSpace,
    unsigned long long &insVolCapacity)
{
    ULARGE_INTEGER uliQuota = { 0 };
    ULARGE_INTEGER uliTotalCapacity = { 0 };
    ULARGE_INTEGER uliFreeSpace = { 0 };
    bool bretval = true;



    // Extracting the volume letter from installPath.
    char installVolume[MAX_PATH];
    memset(installVolume, 0, MAX_PATH);

    installVolume[0] = installPath[0];
    inm_strcat_s(installVolume, ARRAYSIZE(installVolume), ":\\");

    // PR#10815: Long Path support
    if (!SVGetDiskFreeSpaceEx(installVolume, &uliQuota, &uliTotalCapacity, &uliFreeSpace))
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, GetDiskFreeSpaceEx failed\n", LINE_NO,
            FILE_NAME);
        bretval = false;
    }
    else
    {
        insVolFreeSpace = (unsigned long long)uliFreeSpace.QuadPart;
        insVolCapacity = (unsigned long long)uliTotalCapacity.QuadPart;
    }

    return bretval;
}


int comparevolnames(const char *vol1, const char *vol2)
{
    return stricmp(vol1, vol2);
}


/** COMMONTOALL : START */


void HideBeforeApplyingSyncData(char * buffer, int size)
{

    HideFileSystem(buffer, size);
}

/** COMMONTOALL : END */


// --------------------------------------------------------------------------
// format volume name the way the CX wants it
// final format should be one of
//   <drive>
//   <drive>:\[<mntpoint>\]
// where: <drive> is the drive letter and [<mntpoint>\] is optional
//        and <mntpoint> is the mount point name
// e.g.
//   A:\ for a drive letter
//   B:\mnt\data\ for a mount point
// --------------------------------------------------------------------------
void FormatVolumeNameForCxReporting(std::string& volumeName)
{

    if (IsVolumeNameInGuidFormat(volumeName))
        return;

    // we need to strip off any leading \, ., ? if they exists
    std::string::size_type idx = volumeName.find_first_not_of("\\.?");
    if (std::string::npos != idx) {
        volumeName.erase(0, idx);
    }

    // strip off trailing :\ or : if they exist
    std::string::size_type len = volumeName.length();
    //if we get the len as 0 we are not proceeding and simply returning
    //this is as per the bug 10377
    if (!len)
        return;
    idx = len;
    if ('\\' == volumeName[len - 1] || ':' == volumeName[len - 1]) {
        --idx;
    }

    if ((len > 2) && (':' == volumeName[len - 2]))
    {
        --idx;
    }

    if (idx < len) {
        volumeName.erase(idx);
    }
}


int posix_fadvise_wrapper(ACE_HANDLE fd, SV_ULONGLONG offset, SV_ULONGLONG len, int advice)
{
    return 0; //SUCCESS
}


bool IsReportingRealNameToCx(void)
{
    return true;
}


bool GetLinkNameIfRealDeviceName(const std::string sVolName, std::string &sLinkName)
{
    sLinkName = sVolName;
    return true;
}

std::string GetRawVolumeName(const std::string &dskname)
{
    return dskname;
}

bool containsMountedVolumes(const std::string & volumename, std::string & mountedvolume)
{
    bool rv = false;
    std::string s1 = volumename;

    FormatVolumeNameForCxReporting(s1);
    FormatVolumeNameForMountPoint(s1);

    char MountPointBuffer[MAX_PATH + 1];
    HANDLE hMountPoint = FindFirstVolumeMountPoint(s1.c_str(),
        MountPointBuffer, sizeof(MountPointBuffer) - 1);
    if (INVALID_HANDLE_VALUE == hMountPoint)
    {
        rv = false;
    }
    else
    {
        rv = true;
        mountedvolume = MountPointBuffer;
        FindVolumeMountPointClose(hMountPoint);
    }
    return rv;
}

#ifdef SV_USES_LONGPATHS
std::wstring getLongPathNameUtf8(const char* path)
{
    return ExtendedLengthPath::nameW(convertUtf8ToWstring(path).c_str());
}
std::wstring getLongPathName(const char* path)
{
    /*	std::string unicodePath = path;

    //replace all '/' with '\\'
    std::replace(unicodePath.begin(), unicodePath.end(), '/', '\\');

    //if path is not a drive letter and it contains neither "\\\\?\\" nor "\\\\.\\"
    //add "\\\\?\\"
    if( (unicodePath.length() > 2) &&
    (unicodePath.substr(0, 4) != "\\\\?\\" && unicodePath.substr(0, 4) != "\\\\.\\") )
    {
    unicodePath = "\\\\?\\" + unicodePath;
    }

    //remove repeting '\\' if any
    std::string::size_type index = -1;
    while( (index = unicodePath.find('\\', ++index)) != unicodePath.npos )
    if(index >= 4 && unicodePath[index-1] == '\\')
    unicodePath.erase(unicodePath.begin() + index--);

    //DebugPrintf(SV_LOG_DEBUG,"getLongPathName() converted Path: %s to %s\n", path, unicodePath.c_str());

    //unicode requirements are met. now convert to unicode and return
    return ACE_Ascii_To_Wide(unicodePath.c_str()).wchar_rep();*/
    return ExtendedLengthPath::name(path);
}
#else /* SV_USES_LONGPATHS */
std::string getLongPathName(const char* path)
{
    return path;
}
#endif /* SV_USES_LONGPATHS */

int sv_stat(const char *file, ACE_stat *stat)
{
    return ACE_OS::stat(file, stat);
}

int sv_stat(const wchar_t *wFileName, ACE_stat *stat)
{
    int rv = -1;

    if (ACE_OS::strlen(wFileName) < MAX_PATH)
    {
        std::string fileName = ExtendedLengthPath::nonExtName(wFileName);
        rv = ACE_OS::stat(fileName.c_str(), stat);
    }
    else
    {
        WIN32_FILE_ATTRIBUTE_DATA fileAttribData;
        if (GetFileAttributesExW(wFileName, GetFileExInfoStandard, &fileAttribData) != 0)
        {
            unsigned long long fileSize = fileAttribData.nFileSizeHigh;
            fileSize <<= (sizeof(fileAttribData.nFileSizeHigh) * 8);
            fileSize += fileAttribData.nFileSizeLow;
            stat->st_size = fileSize;
            stat->st_atime = ACE_Time_Value(fileAttribData.ftLastAccessTime).sec();
            stat->st_ctime = ACE_Time_Value(fileAttribData.ftCreationTime).sec();
            stat->st_mtime = ACE_Time_Value(fileAttribData.ftLastWriteTime).sec();
            stat->st_dev = stat->st_rdev = 0; // No equivalent conversion.
            stat->st_gid = stat->st_uid = 0;
            stat->st_ino = stat->st_nlink = 0;
            stat->st_mode = (S_IXOTH | S_IROTH |
                (fileAttribData.dwFileAttributes & FILE_ATTRIBUTE_READONLY ? 0 : S_IWOTH) |
                (fileAttribData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? S_IFDIR : S_IFREG));
            rv = 0;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Failed to get file attributes @ LINE %d in FILE %s. error no:%d \n",
                __LINE__, __FILE__, GetLastError());
            rv = -1;
        }
    }

    return rv;
}

BOOL SVGetVolumePathName(LPCTSTR lpszFileName, LPTSTR lpszVolumePathName, const DWORD volumePathNameLen)
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)

    ACE_TCHAR* pVolumePathName = NULL;
    if (volumePathNameLen != 0)
    {
        pVolumePathName = new ACE_TCHAR[volumePathNameLen];
    }

    rv = GetVolumePathNameW(getLongPathName(lpszFileName).c_str(), pVolumePathName, volumePathNameLen);

    if (rv && lpszVolumePathName != NULL && pVolumePathName != NULL)
    {
        std::string volPathName = ExtendedLengthPath::nonExtName(pVolumePathName);
        inm_strcpy_s(lpszVolumePathName, volumePathNameLen, volPathName.c_str());
    }

    delete[] pVolumePathName;

#else
    rv = GetVolumePathName(lpszFileName, lpszVolumePathName, volumePathNameLen);
#endif

    if (!rv)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s for %s failed with error code: %d\n",
            FUNCTION_NAME, lpszFileName, GetLastError());
    }

    return rv;
}

BOOL SVGetVolumeInformationUtf8(
    LPCTSTR lpRootPathName,
    std::string& strVolumeName,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    std::string& strFileSystemName
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)

    wchar_t pVolumeNameBuffer[MAX_PATH + 1];
    memset(pVolumeNameBuffer, '\000', sizeof(pVolumeNameBuffer));
    wchar_t pFileSystemNameBuffer[MAX_PATH + 1];
    memset(pFileSystemNameBuffer, '\000', sizeof(pFileSystemNameBuffer));
    rv = GetVolumeInformationW(getLongPathNameUtf8(lpRootPathName).c_str(),
        pVolumeNameBuffer,
        NELEMS(pVolumeNameBuffer),
        lpVolumeSerialNumber,
        lpMaximumComponentLength,
        lpFileSystemFlags,
        pFileSystemNameBuffer,
        NELEMS(pFileSystemNameBuffer));

    if (rv)
    {
        strVolumeName = convertWstringToUtf8(pVolumeNameBuffer);
        strFileSystemName = convertWstringToUtf8(pFileSystemNameBuffer);
    }

#else
    char pVolumeNameBuffer[MAX_PATH + 1];
    memset(pVolumeNameBuffer, '\000', sizeof(pVolumeNameBuffer));
    char pFileSystemNameBuffer[MAX_PATH + 1];
    memset(pFileSystemNameBuffer, '\000', sizeof(pFileSystemNameBuffer));
    rv = GetVolumeInformation(lpRootPathName,
        pVolumeNameBuffer,
        MAX_PATH,
        lpVolumeSerialNumber,
        lpMaximumComponentLength,
        lpFileSystemFlags,
        pFileSystemNameBuffer,
        MAX_PATH);
    strVolumeName = pVolumeNameBuffer;
    strFileSystemName = pFileSystemNameBuffer;
#endif

    if (!rv)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s for %s failed with error code: %d\n",
            FUNCTION_NAME, lpRootPathName, GetLastError());
    }

    return rv;
}

BOOL SVGetVolumeInformation(
    LPCTSTR lpRootPathName,
    LPTSTR lpVolumeNameBuffer,
    const DWORD nVolumeNameSize,
    LPDWORD lpVolumeSerialNumber,
    LPDWORD lpMaximumComponentLength,
    LPDWORD lpFileSystemFlags,
    LPTSTR lpFileSystemNameBuffer,
    const DWORD nFileSystemNameSize
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)

    ACE_TCHAR* pVolumeNameBuffer = NULL;
    ACE_TCHAR* pFileSystemNameBuffer = NULL;

    if (nVolumeNameSize != 0)
    {
        pVolumeNameBuffer = new ACE_TCHAR[nVolumeNameSize];
    }
    if (nFileSystemNameSize != 0)
    {
        pFileSystemNameBuffer = new ACE_TCHAR[nFileSystemNameSize];
    }

    rv = GetVolumeInformationW(getLongPathName(lpRootPathName).c_str(),
        pVolumeNameBuffer,
        nVolumeNameSize,
        lpVolumeSerialNumber,
        lpMaximumComponentLength,
        lpFileSystemFlags,
        pFileSystemNameBuffer,
        nFileSystemNameSize);

    if (rv && lpVolumeNameBuffer != NULL && pVolumeNameBuffer != NULL)
    {
        inm_strncpy_s(lpVolumeNameBuffer, nVolumeNameSize, ACE_TEXT_ALWAYS_CHAR(pVolumeNameBuffer), nVolumeNameSize);
    }
    if (rv && lpFileSystemNameBuffer != NULL && pFileSystemNameBuffer != NULL)
    {
        inm_strncpy_s(lpFileSystemNameBuffer, nFileSystemNameSize, ACE_TEXT_ALWAYS_CHAR(pFileSystemNameBuffer), nFileSystemNameSize);
    }

    delete[] pVolumeNameBuffer;
    delete[] pFileSystemNameBuffer;

#else
    rv = GetVolumeInformation(lpRootPathName,
        lpVolumeNameBuffer,
        nVolumeNameSize,
        lpVolumeSerialNumber,
        lpMaximumComponentLength,
        lpFileSystemFlags,
        lpFileSystemNameBuffer,
        nFileSystemNameSize);
#endif

    if (!rv)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s for %s failed with error code: %d\n",
            FUNCTION_NAME, lpRootPathName, GetLastError());
    }

    return rv;
}

BOOL SVGetDiskFreeSpaceExUtf8(
    LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailable,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = GetDiskFreeSpaceExW(getLongPathNameUtf8(lpDirectoryName).c_str(),
        lpFreeBytesAvailable,
        lpTotalNumberOfBytes,
        lpTotalNumberOfFreeBytes);
#else
    rv = GetDiskFreeSpaceEx(lpDirectoryName,
        lpFreeBytesAvailable,
        lpTotalNumberOfBytes,
        lpTotalNumberOfFreeBytes);
#endif

    return rv;
}

BOOL SVGetDiskFreeSpaceEx(
    LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailable,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = GetDiskFreeSpaceExW(getLongPathName(lpDirectoryName).c_str(),
        lpFreeBytesAvailable,
        lpTotalNumberOfBytes,
        lpTotalNumberOfFreeBytes);
#else
    rv = GetDiskFreeSpaceEx(lpDirectoryName,
        lpFreeBytesAvailable,
        lpTotalNumberOfBytes,
        lpTotalNumberOfFreeBytes);
#endif

    return rv;
}

BOOL SVGetDiskFreeSpaceUtf8(
    LPCTSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = GetDiskFreeSpaceW(getLongPathNameUtf8(lpRootPathName).c_str(),
        lpSectorsPerCluster,
        lpBytesPerSector,
        lpNumberOfFreeClusters,
        lpTotalNumberOfClusters);
#else
    rv = GetDiskFreeSpace(lpRootPathName,
        lpSectorsPerCluster,
        lpBytesPerSector,
        lpNumberOfFreeClusters,
        lpTotalNumberOfClusters);
#endif

    return rv;
}

BOOL SVGetDiskFreeSpace(
    LPCTSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = GetDiskFreeSpaceW(getLongPathName(lpRootPathName).c_str(),
        lpSectorsPerCluster,
        lpBytesPerSector,
        lpNumberOfFreeClusters,
        lpTotalNumberOfClusters);
#else
    rv = GetDiskFreeSpace(lpRootPathName,
        lpSectorsPerCluster,
        lpBytesPerSector,
        lpNumberOfFreeClusters,
        lpTotalNumberOfClusters);
#endif

    return rv;
}

DWORD SVGetCompressedFileSize(LPCTSTR lpFileName, LPDWORD lpFileSizeHigh)
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = GetCompressedFileSizeW(getLongPathName(lpFileName).c_str(), lpFileSizeHigh);
#else
    rv = GetCompressedFileSize(lpFileName, lpFileSizeHigh);
#endif

    return rv;
}

SC_HANDLE SVOpenService(SC_HANDLE hSCManager, LPCTSTR lpServiceName, DWORD dwDesiredAccess)
{
#if defined (SV_USES_LONGPATHS)
    return OpenServiceW(hSCManager, ACE_TEXT_CHAR_TO_TCHAR(lpServiceName), dwDesiredAccess);
#else
    return OpenService(hSCManager, lpServiceName, dwDesiredAccess);
#endif
}

BOOL SVCopyFile(LPCTSTR lpExistingFileName, LPCTSTR lpNewFileName, BOOL bFailIfExists)
{
    BOOL rv;

#if defined (SV_USES_LONGPATHS)
    rv = CopyFileW(getLongPathName(lpExistingFileName).c_str(),
        getLongPathName(lpNewFileName).c_str(),
        bFailIfExists);
#else
    rv = CopyFile(lpExistingFileName, lpNewFileName, bFailIfExists);
#endif

    return rv;
}

HANDLE SVCreateFileUtf8(
    LPCTSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
#if defined (SV_USES_LONGPATHS)
    return CreateFileW(getLongPathNameUtf8(lpFileName).c_str(),
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
#else
    return CreateFile(lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
#endif
}

HANDLE SVCreateFile(
    LPCTSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    )
{
#if defined (SV_USES_LONGPATHS)
    return CreateFileW(getLongPathName(lpFileName).c_str(),
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
#else
    return CreateFile(lpFileName,
        dwDesiredAccess,
        dwShareMode,
        lpSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile);
#endif
}


bool HasPatternInWMIDB(const std::vector<std::string> &wmis,
    Pats &pats)
{
    bool bhaspat = false;

    return bhaspat;
}


bool GetWMIOutput(const std::string &wminame, std::string &output, std::string &error, int &ecode)
{
    return false;
}

bool IsOpenVzVM(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;
    return bisvm;
}


bool IsVMFromCpuInfo(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;
    return bisvm;
}


bool IsXenVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisxen = false;
    return bisxen;
}

bool GetBiosId(std::string& biosId)
{
    BIOSRecordProcessor::WMIBIOSClass p;
    BIOSRecordProcessor biosRecordProcessor(&p);
    bool breturn = false;
    GenericWMI gwmi(&biosRecordProcessor);
    SVERROR sv = gwmi.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        breturn = true;
        gwmi.GetData(WMIBIOS);
    }
    DebugPrintf(SV_LOG_DEBUG, "BIOS Serial %s\n", p.serialNo.c_str());
    biosId = p.serialNo;
    return breturn;
}


bool GetInstalledProducts(std::list<InstalledProduct>& installedProds)
{
    InstalledProductsProcessor installedProdsProcessor(&installedProds);
    bool breturn = false;
    GenericWMI qfip(&installedProdsProcessor);
    SVERROR sv = qfip.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        breturn = true;
        qfip.GetData("Win32_Product");
    }
    DebugPrintf(SV_LOG_ERROR, "Total No of Installed products %d\n", installedProds.size());
    return breturn;

}
bool GetInstalledPatches(std::list<std::string>& hotfixList)
{
    QuickFixEngineeringProcessor::WMIHotFixInfoClass p;
    QuickFixEngineeringProcessor quickFixEnggProcessor(&p);
    bool breturn = false;
    GenericWMI qfep(&quickFixEnggProcessor);
    SVERROR sv = qfep.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        breturn = true;
        qfep.GetData("Win32_QuickFixEngineering");
    }
    DebugPrintf(SV_LOG_ERROR, "Total No of Installed Patches %d\n", p.hotfixList.size());
    hotfixList = p.hotfixList;
    return breturn;

}
bool GetBaseBoardId(std::string& baseboardId)
{
    BaseBoardProcessor::WMIBasedBoardClass p;
    BaseBoardProcessor baseboardprocessor(&p);
    bool breturn = false;
    GenericWMI gwmi(&baseboardprocessor);
    SVERROR sv = gwmi.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        breturn = true;
        gwmi.GetData(WMIBIOS);
    }
    DebugPrintf(SV_LOG_DEBUG, "BIOS Serial %s\n", p.serialNo.c_str());
    baseboardId = p.serialNo;
    return breturn;
}

int CompareProudctVersion(const InstalledProduct& first, const InstalledProduct& second)
{
    return (first.version.compare(second.version));
}

SVERROR GetInstalledProductFromRegistry(const std::string& strKey,
    InstalledProduct &installedProduct,
    USHORT accessMode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s: %s\n", FUNCTION_NAME, strKey.c_str());

    SVERROR retStatus = SVS_OK;

    std::vector<std::string> keyNames = { "DisplayName", "DisplayVersion", "Publisher", "InstallLocation" };

    for (auto iterKeyName = keyNames.begin(); iterKeyName != keyNames.end(); iterKeyName++)
    {
        std::string value;
        if (getStringValue(strKey, *iterKeyName, value, accessMode) != SVS_OK)
        {
            retStatus = SVE_FAIL;
            break;
        }

        if (*iterKeyName == "DisplayName")
        {
            installedProduct.name = value;
        }
        else if (*iterKeyName == "DisplayVersion")
        {
            installedProduct.version = value;
        }
        else if (*iterKeyName == "Publisher")
        {
            installedProduct.vendor = value;
        }
        else if (*iterKeyName == "InstallLocation")
        {
            boost::erase_all(value, "\"");
            installedProduct.installLocation = value;
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retStatus;
}

SVERROR GetInstalledProductsFromRegistry(const std::string& prodName,
    std::list<InstalledProduct> &installedProducts)
{
    DebugPrintf(SV_LOG_INFO, "Registry lookup for the product : %s\n", prodName.c_str());
    SVERROR retStatus = SVS_OK;

    std::string strUninstallRegKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    std::vector<USHORT> accessModes = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };

    for (auto iterAccessMode = accessModes.begin(); iterAccessMode != accessModes.end(); iterAccessMode++)
    {
        std::list<std::string> listSubKeys;
        if (getSubKeysListEx(strUninstallRegKey, listSubKeys, *iterAccessMode) != SVS_OK)
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: Failed to get subkeys from %s.\n",
                FUNCTION_NAME,
                strUninstallRegKey.c_str());
        }
        else
        {
            for (auto iterSubKey = listSubKeys.begin(); iterSubKey != listSubKeys.end(); iterSubKey++)
            {
                InstalledProduct prod;
                std::string subKey = strUninstallRegKey + "\\" + *iterSubKey;
                if (GetInstalledProductFromRegistry(subKey, prod, *iterAccessMode) != SVS_OK)
                {
                    DebugPrintf(SV_LOG_DEBUG, "%s: Failed to get product details from key %s.\n",
                        FUNCTION_NAME,
                        subKey.c_str());
                }
                else
                {
                    if (prod.name == prodName)
                    {
                        installedProducts.push_back(prod);
                    }
                }
            }
        }
    }

    if (installedProducts.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "Installtion details for product key %s is not found in registry path %s.\n",
            prodName.c_str(),
            strUninstallRegKey.c_str());
        retStatus = SVS_FALSE;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retStatus;
}

SVERROR GetInMageInstalledProductFromRegistry(const std::string& strKey,
    InstalledProduct &installedProduct,
    USHORT accessMode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s: %s\n", FUNCTION_NAME, strKey.c_str());

    SVERROR retStatus = SVS_OK;

    std::vector<std::string> keyNames = { "Product_Name", "Product_Version", "InstallDirectory" };

    for (auto iterKeyName = keyNames.begin(); iterKeyName != keyNames.end(); iterKeyName++)
    {
        std::string value;
        if (getStringValue(strKey, *iterKeyName, value, accessMode) != SVS_OK)
        {
            retStatus = SVE_FAIL;
            break;
        }

        if (*iterKeyName == "Product_Name")
        {
            installedProduct.name = value;
        }
        else if (*iterKeyName == "Product_Version")
        {
            installedProduct.version = value;
        }
        else if (*iterKeyName == "InstallDirectory")
        {
            boost::erase_all(value, "\"");
            installedProduct.installLocation = value;
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retStatus;
}

SVERROR GetInMageInstalledProductsFromRegistry(const std::string& productKeyName,
    std::list<InstalledProduct> &installedProducts)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    DebugPrintf(SV_LOG_INFO, "Registry lookup for the product key name: %s\n", productKeyName.c_str());
    SVERROR retStatus = SVS_OK;

    std::string strInstallRegKey = "SOFTWARE\\InMage Systems\\Installed Products";
    std::vector<USHORT> accessModes = { KEY_WOW64_64KEY, KEY_WOW64_32KEY };

    for (auto iterAccessMode = accessModes.begin(); iterAccessMode != accessModes.end(); iterAccessMode++)
    {
        std::list<std::string> listSubKeys;
        if (getSubKeysListEx(strInstallRegKey, listSubKeys, *iterAccessMode) != SVS_OK)
        {
            DebugPrintf(SV_LOG_DEBUG, "%s: Failed to get subkeys from %s.\n",
                FUNCTION_NAME,
                strInstallRegKey.c_str());
        }
        else
        {
            for (auto iterSubKey = listSubKeys.begin(); iterSubKey != listSubKeys.end(); iterSubKey++)
            {
                InstalledProduct prod;
                if (productKeyName == *iterSubKey)
                {
                    std::string subKey = strInstallRegKey + "\\" + *iterSubKey;
                    if (GetInMageInstalledProductFromRegistry(subKey, prod, *iterAccessMode) != SVS_OK)
                    {
                        DebugPrintf(SV_LOG_DEBUG, "%s: Failed to get product details from key %s.\n",
                            FUNCTION_NAME,
                            subKey.c_str());
                    }
                    else
                    {
                        installedProducts.push_back(prod);
                    }
                }
            }
        }
    }

    if (installedProducts.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "Installtion details for product key %s is not found in registry path %s.\n",
            productKeyName.c_str(),
            strInstallRegKey.c_str());
        retStatus = SVE_FAIL;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return retStatus;
}

void DumpProductDetails(std::list<InstalledProduct>& installedProds)
{
    for (auto iter = installedProds.begin(); iter != installedProds.end(); iter++)
    {
        DebugPrintf(SV_LOG_DEBUG,
            "Product package name: %s, name :%s, version: %s, vendor: %s, install location: %s.\n", 
            iter->pkgname.c_str(),
            iter->name.c_str(),
            iter->version.c_str(),
            iter->vendor.c_str(),
            iter->installLocation.c_str());
    }
    return;
}

bool GetMarsProductDetails(std::list<InstalledProduct>& marsProduct)
{
    bool breturn = true;
    const std::string MarsAgentName("Microsoft Azure Recovery Services Agent");
    if (GetInstalledProductsFromRegistry(MarsAgentName, marsProduct) != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to discover MARS agent product details.\n");
        
        InstalledProductsProcessor installedProdsProcessor(&marsProduct);
        GenericWMI qfip(&installedProdsProcessor);
        SVERROR sv = qfip.init();
        if (sv != SVS_OK)
        {
            breturn = false;
            DebugPrintf(SV_LOG_ERROR, "Initializing generic wmi failed\n");
        }
        else
        {
            breturn = true;
            std::string query("SELECT * FROM Win32_Product where Name = \"");
            query += MarsAgentName;
            query += "\"";
            qfip.GetDataUsingQuery(query);
        }
    }
    return breturn;
}
bool IsVmFromDeterministics(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;
    WmiComputerSystemRecordProcessor::WmiComputerSystemClass cs;
    WmiComputerSystemRecordProcessor p(&cs);
    GenericWMI gwmi(&p);

    SVERROR sv = gwmi.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        gwmi.GetData(WMICOMPUTERSYSTEM);
        cs.Print();

        const char *pmdl = cs.model.c_str();
        const char *pmf = cs.manufacturer.c_str();
        size_t lenmf = strlen(HYPERVMANUFACTURER);
        size_t lenmdl = strlen(HYPERVMODEL);
        if ((0 == strncmp(HYPERVMANUFACTURER, pmf, lenmf)) &&
            (0 == strncmp(HYPERVMODEL, pmdl, lenmdl)))
        {
            bisvm = true;
            hypervisorname = HYPERVNAME;
        }

        if (false == bisvm)
        {
            lenmf = strlen(VMWAREPAT);
            if (0 == strncmp(VMWAREPAT, pmf, lenmf))
            {
                bisvm = true;
                hypervisorname = VMWARENAME;
            }
        }

        if (false == bisvm)
        {
            lenmf = strlen(XENPAT);
            if (0 == strncmp(XENPAT, pmf, lenmf))
            {
                bisvm = true;
                hypervisorname = XENNAME;
            }
        }

        if (false == bisvm)
        {
            bisvm = pats.IsMatched(cs.manufacturer, hypervisorname);
        }

        /* TODO: should we grep in model too ? */
        if (false == bisvm)
        {
            bisvm = pats.IsMatched(cs.model, hypervisorname);
        }
    }

    return bisvm;
}


bool IsNativeVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    return false;
}


bool MaskRequiredSignals(void)
{
    return true;
}


std::string findSecond(const Options_t & map, const std::string & key)
{
    ConstOptionsIter_t it = map.find(key);
    std::string status;

    if (it != map.end())
    {
        status = it->second;
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Transfer Status not found for %s\n", key.c_str());
    }
    return status;
}


void mountVolume(char volumeGUID[])
{
    LocalConfigurator localConfig;
    std::stringstream mountPointNamePath;
    mountPointNamePath << INM_BOOT_VOLUME_MOUNTPOINT;
    SVMakeSureDirectoryPathExists(mountPointNamePath.str().c_str());
    if (SetVolumeMountPoint(mountPointNamePath.str().c_str(), volumeGUID) == 0)
        DebugPrintf(SV_LOG_DEBUG, "Boot volume mount failed. Mount Point Name: %s, Volume GUID: %s. HRESULT = %08X \n", mountPointNamePath.str().c_str(), volumeGUID, HRESULT_FROM_WIN32(GetLastError()));
}

bool isBootableVolume(char volumeGUID[])
{
    bool bSRV = false;
    if (strlen(volumeGUID) > 2)
    {
        volumeGUID[2] = '.';
        volumeGUID[strlen(volumeGUID) - 1] = '\0';
        HANDLE hVol = INVALID_HANDLE_VALUE;
        hVol = SVCreateFile(volumeGUID, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (hVol == INVALID_HANDLE_VALUE) // cannot open the drive
        {
            DebugPrintf(SV_LOG_ERROR, "FAILED SVCreateFile(). Volume GUID %s . HRESULT = %08X\n", volumeGUID, HRESULT_FROM_WIN32(GetLastError()));
        }
        else
        {
            PARTITION_INFORMATION OutBuffer;
            DWORD lpBytesReturned = 0;
            if (DeviceIoControl((HANDLE)hVol,                // handle to a partition
                IOCTL_DISK_GET_PARTITION_INFO,   // dwIoControlCode
                (LPVOID)NULL,                   // lpInBuffer
                (DWORD)0,                       // nInBufferSize
                &OutBuffer,            // output buffer
                sizeof(PARTITION_INFORMATION),          // size of output buffer
                &lpBytesReturned,       // number of bytes returned
                (LPOVERLAPPED)NULL) != 0)   // OVERLAPPED structure			
            {
                if (OutBuffer.BootIndicator == TRUE)
                {
                    DebugPrintf(SV_LOG_DEBUG, "Volume %s is boot volume \n", volumeGUID);
                    bSRV = true;
                }
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "FAILED DeviceIoControl(). HRESULT = %08X\n", HRESULT_FROM_WIN32(GetLastError()));
            }
            CloseHandle(hVol);
        }
        volumeGUID[2] = '?';
        volumeGUID[strlen(volumeGUID)] = '\\';
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Invalid Volume GUID. it is too short. GUID: %s \n", volumeGUID);
    }
    return bSRV;
}

void mountBootableVolumesIfRequried()
{
    static bool isVolFilter = false;
    static bool isDiskFilter = false;
    LocalConfigurator localConfigurator;
    if (!isDiskFilter && !isVolFilter && localConfigurator.IsFilterDriverAvailable())
    {
        DeviceStream::Ptr pDeviceStream(new DeviceStream(INMAGE_DISK_FILTER_DOS_DEVICE_NAME));
        if (SV_SUCCESS == pDeviceStream->Open(DeviceStream::Mode_Open | DeviceStream::Mode_Read | DeviceStream::Mode_ShareRW))
        {
            isDiskFilter = true;
        }
        else
        {
            pDeviceStream.reset(new DeviceStream(INMAGE_FILTER_DEVICE_NAME));
            if (SV_SUCCESS == pDeviceStream->Open(DeviceStream::Mode_Open | DeviceStream::Mode_Read | DeviceStream::Mode_ShareRW))
            {
                isVolFilter = true;
            }
        }
    }
    /* GetVolumePathNamesForVolumeName is not supported in win2k */
#ifndef SCOUT_WIN2K_SUPPORT
    char volGuid[256] = { 0 };  // buffer to hold GUID of a volume.
    HANDLE pHandle = NULL;
    pHandle = FindFirstVolume(volGuid, sizeof(volGuid) - 1);
    if (pHandle != INVALID_HANDLE_VALUE)
    {
        do
        {
            LPTSTR VolumePathName = NULL;
            DWORD rlen = 0;
            GetVolumePathNamesForVolumeName(volGuid, VolumePathName, 0, &rlen);
            if (GetLastError() == ERROR_MORE_DATA)
            {
                size_t pathnamelen;
                INM_SAFE_ARITHMETIC(pathnamelen = InmSafeInt<DWORD>::Type(rlen) * 2, INMAGE_EX(rlen))
                    VolumePathName = (LPTSTR)malloc(pathnamelen);
                memset(VolumePathName, 0, rlen * 2);
                if (VolumePathName != NULL && GetVolumePathNamesForVolumeName(volGuid, VolumePathName, rlen * 2, &rlen))
                {
                    if (VolumePathName[0] == '\0')
                    {
                        // This volume does not have drive letter.
                        //check whether it is SRV or not.						 
                        if (isVolFilter && isBootableVolume(volGuid))
                        {
                            mountVolume(volGuid);
                        }
                    }
                    else
                    {
                        if (isDiskFilter && (0 == strcmp(VolumePathName, INM_BOOT_VOLUME_MOUNTPOINT)) && isBootableVolume(volGuid))
                        {
                            boost::filesystem::exists(std::string(INM_BOOT_VOLUME_MOUNTPOINT)) &&
                                DeleteVolumeMountPoint(INM_BOOT_VOLUME_MOUNTPOINT) &&
                                boost::filesystem::remove(std::string(INM_BOOT_VOLUME_MOUNTPOINT));
                        }
                    }
                }
                if (VolumePathName != NULL)
                {
                    free(VolumePathName);
                    VolumePathName = NULL;
                }
            }
        } while (FindNextVolume(pHandle, volGuid, sizeof(volGuid) - 1));
        FindVolumeClose(pHandle);
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "FindFirstVolume() Failed. \n");
    }
#endif
}
//16211 - Sets the given volume mount point for the guid passed
bool ReattachDeviceNameToGuid(std::string deviceName, std::string sGuid)
{
    bool rv = false;
    DebugPrintf(SV_LOG_DEBUG, "ENTERING %s\n", FUNCTION_NAME);
    // Mount the volume back at it's previous drive letter
    std::string guidVol = sGuid;
    //if(!IsVolumeNameInGuidFormat(sGuid))
    {
        guidVol = "\\\\?\\Volume{" + sGuid + "}";
    }
    if (guidVol[guidVol.length() - 1] != '\\')
    {
        guidVol += '\\';
    }

    if (!SVSetVolumeMountPoint(deviceName, guidVol))
    {
        DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        DebugPrintf(SV_LOG_ERROR, "ReattachDeviceNameToGuid::SVSetVolumeMountPoint failed while mounting: %s at %s err \n", sGuid.c_str(), deviceName.c_str());
    }
    else
    {
        DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
        rv = true;
    }
    DebugPrintf(SV_LOG_DEBUG, "Leaving %s\n", FUNCTION_NAME);
    return rv;
}

//16211 - Returns a map of drive letters and corresponding guids that are missing.
std::map<std::string, std::string> GetDetachedVolumeEntries()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERING %s\n", FUNCTION_NAME);
    std::map<std::string, std::string> guidmap;
    LocalConfigurator localConfigurator;
    string cacheDir = localConfigurator.getCacheDirectory();

    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cacheDir += "pendingactions";
    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    SVMakeSureDirectoryPathExists(cacheDir.c_str());

    string fileName = cacheDir + DETACHED_DRIVE_DATA_FILE;

    ifstream guidMapFile(fileName.c_str());
    guidmap.clear();

    if (guidMapFile.is_open())
    {
        while (!guidMapFile.eof())
        {
            std::string data;
            //guidMapFile >> data;
            getline(guidMapFile, data);
            if (data.empty())
                break;
            std::string mountpt = data;
            std::string guid = data;
            mountpt.erase(mountpt.find("="));
            guid.erase(0, guid.find("=") + 1);
            guidmap[mountpt] = guid;

        }
        guidMapFile.close();
    }
    else {

    }
    DebugPrintf(SV_LOG_DEBUG, "LEAVING %s\n", FUNCTION_NAME);

    return guidmap;

}

//16211 - Updates the given map of drive letters and corresponding guids to a persistent file.
bool UpdateDetachedVolumeEntries(std::map<std::string, std::string> & guidMap)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERING %s\n", FUNCTION_NAME);
    bool rv = true;
    LocalConfigurator localConfigurator;
    string cacheDir = localConfigurator.getCacheDirectory();

    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cacheDir += "pendingactions";
    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    SVMakeSureDirectoryPathExists(cacheDir.c_str());

    string tempFileName = cacheDir + DETACHED_DRIVE_DATA_FILE + ".temp1";
    string fileName = cacheDir + DETACHED_DRIVE_DATA_FILE;

    if (!guidMap.empty())
    {
        FILE * tempFile = fopen(tempFileName.c_str(), "w+");

        if (tempFile != NULL)
        {
            int fd = fileno(tempFile);
            std::map<std::string, std::string>::iterator it = guidMap.begin();
            std::stringstream data;
            while (it != guidMap.end())
            {
                data << it->first << "=" << it->second << std::endl;
                it++;
            }

            fwrite(data.str().c_str(), data.str().size(), 1, tempFile);
            fflush(tempFile);

            //Commit the changes done in the file to disk.
            if (fd>0)
            {
                if (_commit(fd) < 0)
                {
                    DebugPrintf(SV_LOG_ERROR,
                        "\n%s encountered error while trying to flush the filesystem for the file %s. Failed with error: %d\n",
                        FUNCTION_NAME, tempFileName.c_str(), errno);
                    rv = false;
                }
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR,
                    "\n%s encountered error while trying to open the file %s. Failed with error: %d\n",
                    FUNCTION_NAME, tempFileName.c_str(), errno);
                rv = false;
            }

            fclose(tempFile);

            if (rv)
            {
                ACE_OS::rename(tempFileName.c_str(), fileName.c_str());
            }
        }
        else
        {
            rv = false;
            DebugPrintf(SV_LOG_ERROR, "%s: Could not open file %s with error : %d\n", FUNCTION_NAME, tempFileName.c_str(), GetLastError());
        }
    }
    else
    {
        ACE_OS::unlink(fileName.c_str());
    }
    DebugPrintf(SV_LOG_DEBUG, "Leaving %s\n", FUNCTION_NAME);
    return rv;
}

//16211 - Updates the given drive letters and corresponding guids to a persistent file.
bool PersistDetachedVolumeEntry(std::string mountpt, std::string guid)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERING %s\n", FUNCTION_NAME);
    ACE_Guard<ACE_Recursive_Thread_Mutex> autoGuardMutex(g_PersistDetachedVolumeEntries);
    //Acquire lock on the guidmap file.
    LocalConfigurator localConfigurator;
    string cacheDir = localConfigurator.getCacheDirectory();

    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cacheDir += "pendingactions";
    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    SVMakeSureDirectoryPathExists(cacheDir.c_str());

    string fileLockName = cacheDir + DETACHED_DRIVE_DATA_FILE + ".lck";
    ACE_File_Lock fileLock(ACE_TEXT_CHAR_TO_TCHAR(fileLockName.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    ACE_Guard<ACE_File_Lock> autoGuardFileLock(fileLock);

    std::map<std::string, std::string> guidmap = GetDetachedVolumeEntries();
    DebugPrintf(SV_LOG_DEBUG, "%s:Adding Entry Volume %s Guid %s\n", FUNCTION_NAME, mountpt.c_str(), guid.c_str());
    //guidmap.insert(std::pair<std::string,std::string>(mountpt,guid));
    guidmap[mountpt] = guid;
    if (UpdateDetachedVolumeEntries(guidmap) == false)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Failed to add volume %s and guid %s\n", FUNCTION_NAME, mountpt.c_str(), guid.c_str());
    }
    DebugPrintf(SV_LOG_DEBUG, "Leaving %s\n", FUNCTION_NAME);
    return true;
}

//16211 - Removes the given drive letters and corresponding guids from the persistent file.
bool RemoveAttachedVolumeEntry(std::string mountpt, std::string guid)
{
    //Acquire lock on the guidmap file.
    DebugPrintf(SV_LOG_DEBUG, "ENTERING %s\n", FUNCTION_NAME);
    bool rv = true;
    ACE_Guard<ACE_Recursive_Thread_Mutex> autoGuardMutex(g_PersistDetachedVolumeEntries);

    LocalConfigurator localConfigurator;
    string cacheDir = localConfigurator.getCacheDirectory();

    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    cacheDir += "pendingactions";
    cacheDir += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    SVMakeSureDirectoryPathExists(cacheDir.c_str());

    string fileLockName = cacheDir + DETACHED_DRIVE_DATA_FILE + ".lck";
    ACE_File_Lock fileLock(ACE_TEXT_CHAR_TO_TCHAR(fileLockName.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    ACE_Guard<ACE_File_Lock> autoGuardFileLock(fileLock);

    std::map<std::string, std::string> guidmap = GetDetachedVolumeEntries();
    std::map<std::string, std::string>::iterator it = guidmap.find(mountpt);
    if (it != guidmap.end())
    {
        DebugPrintf(SV_LOG_DEBUG, "%s:Removing Entry Volume %s Guid %s\n", FUNCTION_NAME, mountpt.c_str(), guid.c_str());
        guidmap.erase(it);
        if (UpdateDetachedVolumeEntries(guidmap) == false)
        {
            rv = false;
            DebugPrintf(SV_LOG_DEBUG, "%s:Failed removing Entry Volume %s Guid %s\n", FUNCTION_NAME, mountpt.c_str(), guid.c_str());
        }
    }
    else
    {
        rv = false;
        DebugPrintf(SV_LOG_ERROR, "%s: Could not find the volume entry for %s for guid %s\n", FUNCTION_NAME, mountpt.c_str(), guid.c_str());
    }
    DebugPrintf(SV_LOG_DEBUG, "Leaving %s\n", FUNCTION_NAME);
    return rv;
}


bool InmRename(const std::string &oldfile, const std::string &newfile, std::string &errmsg)
{
    DWORD dwFlags = 0 | MOVEFILE_REPLACE_EXISTING /* | MOVEFILE_WRITE_THROUGH */;
    BOOL brenamed = MoveFileEx(oldfile.c_str(), newfile.c_str(), dwFlags);
    if (!brenamed)
    {
        DWORD err = GetLastError();
        std::stringstream sserr;
        sserr << err;
        errmsg = "Failed to rename file ";
        errmsg += oldfile;
        errmsg += " to ";
        errmsg += newfile;
        errmsg += " with error number: ";
        errmsg += sserr.str();
    }

    return brenamed;
}

void setdirectmode(int &mode)
{
    mode |= (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH);
}

void setsharemode_for_all(mode_t &share)
{
    share = (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
}

void setumask()
{
}

/*  Copies from the in param VARIANT, if it's of type BStr/Array, to the supplied out param.
*  Default out param type is string which will be concatenated with all the values separated by the delimiter.
*  Call to this method can be overloaded with additional out param. For now, it's a referene to vector of strings.
*  Method itself is not overloaded for performance and code reuse
*/
void CopyFromBstrSafeArrayToOutParams(VARIANT *pvt, std::string &valueString, const char &delim,
    const bool fillVector, std::vector<std::string>& valuesVector)
{
    if ((VT_ARRAY | VT_BSTR) == V_VT(pvt))
    {
        USES_CONVERSION;
        SAFEARRAY *psa = V_ARRAY(pvt);
        LONG lbound, ubound;
        HRESULT hr;

        hr = SafeArrayGetLBound(psa, 1, &lbound);
        if (FAILED(hr)) { return; }

        hr = SafeArrayGetUBound(psa, 1, &ubound);
        if (FAILED(hr)) { return; }

        BSTR *p;
        hr = SafeArrayAccessData(psa, (void **)&p);
        if (FAILED(hr)) { return; }

        for (LONG i = lbound; i <= ubound; i++)
        {
            valueString += W2A(p[i]); // fill the string
            if (fillVector)
            {
                valuesVector.push_back(W2A(p[i])); // fill the output vector
            }
            if (i < ubound) // avoid trailing delimiter
            {
                valueString += delim;
            }
        }
        SafeArrayUnaccessData(psa);
    }
}


bool HasNicInfoPhysicaIPs(const NicInfos_t &nicinfos)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bPhysicalIPFound = false;

    for (ConstNicInfosIter_t it = nicinfos.begin(); it != nicinfos.end() && !bPhysicalIPFound; it++)
    {
        const Objects_t &nicconfs = it->second;
        for (ConstObjectsIter_t cit = nicconfs.begin(); cit != nicconfs.end(); cit++)
        {
            ConstAttributesIter_t aitr = cit->m_Attributes.find(NSNicInfo::IP_TYPES);
            if((aitr != cit->m_Attributes.end()) &&
                (aitr->second.find(NSNicInfo::IP_TYPE_PHYSICAL) != string::npos))
            {
                bPhysicalIPFound = true;
                break;
            }
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bPhysicalIPFound;
}


void GetNicInfos(NicInfos_t &nicinfos)
{
    // cache the discovery info as the NIC changes are not that often
    // this avoids repetitive discovery in short duration
    static NicInfos_t s_nicInfos;
    static steady_clock::time_point s_lastDiscoveryTimepoint = steady_clock::time_point::min();
    if (steady_clock::now() < s_lastDiscoveryTimepoint + minutes(2))
    {
        DebugPrintf(SV_LOG_DEBUG, "%s: returning cached NIC discovery info.\n", FUNCTION_NAME);
        nicinfos = s_nicInfos;
        return;
    }
    WmiNetworkAdapterRecordProcessor p(&nicinfos);
    WmiNetworkAdapterConfRecordProcessor pc(&nicinfos);
    GenericWMI::WmiRecordProcessor *processors[] = { &p, &pc };
    const char *classnames[] = { "Win32_NetworkAdapter", "Win32_NetworkAdapterConfiguration" };

    for (int i = 0; i < NELEMS(processors); i++)
    {
        GenericWMI gwmi(processors[i]);
        SVERROR sv = gwmi.init();
        if (sv != SVS_OK)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to initialize the generice wmi\n");
            break;
        }
        else
        {
            gwmi.GetData(classnames[i]);
        }
    }

    s_nicInfos = nicinfos;
    s_lastDiscoveryTimepoint = steady_clock::now();
    return;
}


void GetDiskAttributesFromDli(DRIVE_LAYOUT_INFORMATION_EX *pdli, std::string &storagetype,
    VolumeSummary::FormatLabel &fmt, std::string &volumegroupname)
{
    if (NULL == pdli) {
        return;
    }

    const std::string DYNAMIC_DISK_VGNAME = "__INMAGE__DYNAMIC__DG__";

    for (DWORD i = 0; i < pdli->PartitionCount; i++)
    {
        PARTITION_INFORMATION_EX &p = pdli->PartitionEntry[i];
        PrintPartitionInfoEx(p);
    }

    if (PARTITION_STYLE_MBR == pdli->PartitionStyle) {
        fmt = VolumeSummary::MBR;
    }
    else if (PARTITION_STYLE_GPT == pdli->PartitionStyle) {
        fmt = VolumeSummary::GPT;
    }

    storagetype = NsVolumeAttributes::BASIC;
    if (IsDiskDynamic(pdli)) {
        volumegroupname = DYNAMIC_DISK_VGNAME;
        storagetype = NsVolumeAttributes::DYNAMIC;
    }
}


std::string ConvertDWordToString(const DWORD &dw)
{
    std::ostringstream stream;
    stream << dw;
    return stream.str();
}

//TODO fix the size 36 magic number to proper handover from driver
void StrGuidToWcharGuid(const std::string &strGuid, WCHAR szGUID[36])
{
    USES_CONVERSION;
    inm_memcpy_s(szGUID, sizeof(szGUID), A2W(strGuid.c_str()), (36 * sizeof(wchar_t)));
}

void GetDiskAttributes(const std::string &diskname, std::string &storagetype, VolumeSummary::FormatLabel &fmt,
    VolumeSummary::Vendor &volumegroupvendor, std::string &volumegroupname, std::string &diskGuid)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with diskname %s\n", FUNCTION_NAME, diskname.c_str());
    DRIVE_LAYOUT_INFORMATION_EX *dli = IoctlDiskGetDriveLayoutEx(diskname);
    if (!dli)
    {
        return;
    }
    volumegroupvendor = VolumeSummary::NATIVE;
    if (GetDiskGuidFromDli(dli, diskGuid))
    {
        DebugPrintf(SV_LOG_DEBUG, "DiskGuid for %s is %s\n", diskname.c_str(), diskGuid.c_str());
        volumegroupname = diskGuid;
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "No DiskGuid for %s as its partition style is raw\n", diskname.c_str());
    }
    GetDiskAttributesFromDli(dli, storagetype, fmt, volumegroupname);
    free(dli);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}

std::string GetDiskScsiId(const std::string &diskname)
{
    DeviceIDInformer deviceIdInformer;
    return deviceIdInformer.GetID(diskname);
}

bool enableDiableFileSystemRedirection(const bool bDisableRedirect)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool bRet = false;
    BOOL bIsWow64 = FALSE;
    PVOID OldValue = NULL;
    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
    if (NULL != fnIsWow64Process)
    {
        if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
            if (bIsWow64)
            {
                if (bDisableRedirect)
                {
                    fnWow64DisableWow64FsRedirection = (LPFN_WOW64DISABLEWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64DisableWow64FsRedirection");
                    if (NULL != fnWow64DisableWow64FsRedirection)
                    {
                        if (fnWow64DisableWow64FsRedirection(&OldValue))
                        {
                            bRet = true;
                            DebugPrintf(SV_LOG_DEBUG, "Successfully disabled filesystem redirection\n");
                        }
                    }
                }
                else
                {
                    fnWow64RevertWow64FsRedirection = (LPFN_WOW64REVERTWOW64FSREDIRECTION)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "Wow64RevertWow64FsRedirection");
                    if (NULL != fnWow64RevertWow64FsRedirection)
                    {
                        if (fnWow64RevertWow64FsRedirection(OldValue))
                        {
                            bRet = true;
                            DebugPrintf(SV_LOG_DEBUG, "Successfully re-enabled the file system re-direction\n");
                        }
                    }
                }
            }
            else
            {
                bRet = true;
            }
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}

void FormatVacpErrorCode(std::stringstream& stream, SV_ULONG& exitCode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    if (exitCode != 0)
    {
        stream << "Vacp failed to issue the consistency. Failed with Error Code:";
        switch (exitCode)
        {
        case VACP_E_GENERIC_ERROR: //(10001) 
            stream << "1 (Vacp Generic Error)";
            break;
        case VACP_E_INVALID_COMMAND_LINE:
            stream << "10001. ( Invalid command line options specified for vacp.exe. Use Vacp -h to know more about command line options )";
            break;
        case VACP_E_INVOLFLT_DRIVER_NOT_INSTALLED: //(10002) 
            stream << "10002. (Scout Filter driver is not loaded. Ensure VX is installed and machine is rebooted)";
            break;
        case VACP_E_DRIVER_IN_NON_DATA_MODE: //(10003)
            stream << "10003, (Some of the protected volumes are not in write order data mode. Consistency point will be tried in next scheduled interval.)";
            break;
        case VACP_E_NO_PROTECTED_VOLUMES: //10004
            stream << "10004, (There are no protected volumes to issue consistency bookmark. If the volumes are deleted, make sure to recreate them or recover them. If not protected remove them from the vacp command line.) ";
            break;
        case VACP_E_NON_FIXED_DISK:
            stream << "10005. ( Some or all of the Volumes are Invalid or Deleted or Unavailable volumes.)";
            break;
        case VACP_E_ZERO_EFFECTED_VOLUMES:
            stream << "10006. ( There are zero or no affected volumes  to issue a consistency tag.)";
            break;
        case VACP_E_VACP_SENT_REVOCATION_TAG:
            stream << "10007. ( Vacp has sent a revocation tag as some or all of the participating VSS Writers are not in a Consistent State.)";
            break;
        case VACP_E_VACP_MODULE_FAIL:
            stream << "0x80004005L. (Vacp failed for generic reasons.)";
            break;
        case VACP_E_PRESCRIPT_FAILED:
            stream << "10009. (The pre-script invoked has failed.)";
            break;
        case VACP_E_POSTSCRIPT_FAILED:
            stream << "10010. (The post-script invoked has failed.)";
            break;

        case VACP_PRESCRIPT_WAIT_TIME_EXCEEDED:
            stream << "10998. (The pre-script invoked has exceeded the maximum time to finsh and hence being terminated.)";
            break;
        case VACP_POSTSCRIPT_WAIT_TIME_EXCEEDED:
            stream << "10999. (The post-script invoked has exceeded the maximum time to finsh and hence being terminated.)";
            break;

        case VSS_E_BAD_STATE: stream << "0x80042301L . ( The backup components object is not initialized, this method has been called during a restore operation, or this method has not been called within the correct sequence. )";
            break;
        case VSS_E_PROVIDER_ALREADY_REGISTERED: stream << "0x80042303L . ( Provider already registered )";
            break;
        case VSS_E_PROVIDER_NOT_REGISTERED: stream << "0x80042304L . ( The provider has already been registered on this computer. )";
            break;
        case VSS_E_PROVIDER_VETO: stream << "0x80042306L . ( An unexpected provider error occurred. If this is returned, the error must be described in an entry in the application event log, giving the user information on how to resolve the problem.)";
            break;
        case VSS_E_PROVIDER_IN_USE: stream << "0x80042307L . ( Provider already in use. Retry after some time )";
            break;
        case VSS_E_OBJECT_NOT_FOUND: stream << "0x80042308L . ( VSS object not found )";
            break;
        case VSS_S_ASYNC_PENDING: stream << "0x42309L . ( VSS async pending The asynchronous operation is still running.)";
            break;
        case VSS_S_ASYNC_FINISHED: stream << "0x4230aL . ( VSS async finished.The asynchronous operation was completed successfully. )";
            break;
        case VSS_S_ASYNC_CANCELLED: stream << "0x4230bL . ( VSS async cacelled.The asynchronous operation had been canceled prior to calling \"Cancel\" method. )";
            break;
        case VSS_E_VOLUME_NOT_SUPPORTED: stream << "0x8004230cL . ( VSS does not support issuing tag on one of the volumes.No VSS provider indicates that it supports the specified volume. )";
            break;
        case VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER: stream << "0x8004230eL . ( Provider does not support issuing tag on one of the volumes.The volume is not supported by the specified provider. )";
            break;
        case VSS_E_OBJECT_ALREADY_EXISTS: stream << "0x8004230dL . ( VSS object already exists )";
            break;
        case VSS_E_UNEXPECTED_PROVIDER_ERROR:
            stream << "0x8004230fL . ( VSS unexpected provider error )";
            break;
        case VSS_E_CORRUPT_XML_DOCUMENT:
            stream << "0x80042310L . ( VSS XML document corrupted. )";
            break;
        case VSS_E_INVALID_XML_DOCUMENT:
            stream << "0x80042311L . ( invalid XML document.The given XML document is invalid.  It is either incorrectly-formed XML or it does not match the schema. )";
            break;
        case VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED:
            stream << "0x80042312L . ( The provider has reached the maximum number of volumes it can support.OR the maximum number of volumes has been added to the shadow copy set. The specified volume was not added to the shadow copy set. )";
            break;
        case VSS_E_FLUSH_WRITES_TIMEOUT:
            stream << "0x80042313L . ( The shadow copy provider timed out while flushing data to the volume being shadow copied.This is probably due to excessive activity on the volume. Try again later when the volume is not being used so heavily. )";
            break;
        case VSS_E_HOLD_WRITES_TIMEOUT:
            stream << "0x80042314L . (  The shadow copy provider timed out while holding writes to the volume being shadow copied. This is probably due to excessive activity on the volume by an application or a system service. Try again later when activity on the volume is reduced.OR the system was unable to hold I/O writes. This can be a transient problem. It is recommended to wait ten minutes and try again, up to three times. )";
            break;
        case VSS_E_UNEXPECTED_WRITER_ERROR:
            stream << "0x80042315L . ( VSS encountered problems while sending events to writers. )";
            break;
        case VSS_E_SNAPSHOT_SET_IN_PROGRESS:
            stream << "0x80042316L . ( Another shadow copy creation is already in progress. Please wait a few moments and try again. )";
            break;
        case VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED:
            stream << "0x80042317L . ( The provider has reached the maximum number of shadow copies that it can support OR the specified volume has already reached its maximum number of shadow copies.)";
            break;
        case VSS_E_WRITER_INFRASTRUCTURE:
            stream << "0x80042318L . ( An error was detected in the Volume Shadow Copy Service (VSS). The problem occurred while trying to contact VSS writers.Please verify that the Event System service and the VSS service are running and check for associated errors in the event logs.)";
            break;
        case VSS_E_WRITER_NOT_RESPONDING:
            stream << "0x80042319L . ( A writer did not respond to a GatherWriterStatus call.  The writer may either have terminated or it may be stuck.  Check the system and application event logs for more information.)";
            break;
        case VSS_E_WRITER_ALREADY_SUBSCRIBED:
            stream << "0x8004231aL . ( The writer has already sucessfully called the Subscribe function.  It cannot call subscribe multiple times. )";
            break;
        case VSS_E_UNSUPPORTED_CONTEXT:
            stream << "0x8004231bL . ( The shadow copy provider does not support the specified shadow copy type.)";
            break;
        case VSS_E_VOLUME_IN_USE:
            stream << "0x8004231dL . ( The specified shadow copy storage association is in use and so can't be deleted.)";
            break;
        case VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED:
            stream << "0x8004231eL . ( Maximum number of shadow copy storage associations already reached.OR the maximum number of shadow copy storage areas has been added to the shadow copy source volume.The specified shadow copy storage volume was not associated with the specified shadow copy source volume. )";
            break;
        case VSS_E_INSUFFICIENT_STORAGE:
            stream << "0x8004231fL . ( Insufficient storage available to create either the shadow copy storage file or other shadow copy data. OR the system or provider has insufficient storage space. If possible delete any old or unnecessary persistent shadow copies and try again. )";
            break;
        case VSS_E_NO_SNAPSHOTS_IMPORTED:
            stream << "0x80042320L . ( No shadow copies were successfully imported. )";
            break;
        case VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED:
            stream << "0x00042321L . (Some shadow copies were not succesfully imported. )";
            break;
        case VSS_E_SOME_SNAPSHOTS_NOT_IMPORTED:
            stream << "0x800423221L . (Some shadow copies were not succesfully imported. )";
            break;
        case VSS_E_MAXIMUM_NUMBER_OF_REMOTE_MACHINES_REACHED:
            stream << "0x80042322L . ( The maximum number of remote machines for this operation has been reached. )";
            break;
        case VSS_E_REMOTE_SERVER_UNAVAILABLE:
            stream << "0x80042323L . (The remote server is unavailable. )";
            break;
        case VSS_E_REMOTE_SERVER_UNSUPPORTED:
            stream << "0x80042324L . ( The remote server is running a version of the Volume Shadow Copy Service that does not support remote shadow-copy creation. )";
            break;
        case VSS_E_REVERT_IN_PROGRESS:
            stream << "0x80042325L . ( A revert is currently in progress for the specified volume.  Another revert cannot be initiated until the current revert completes. )";
            break;
        case VSS_E_REVERT_VOLUME_LOST:
            stream << "0x80042326L . ( The volume being reverted was lost during revert.)";
            break;
        case VSS_E_REBOOT_REQUIRED:
            stream << "0x80042327L. ( A reboot is required after completing this operation.)";
            break;
        case VSS_E_TRANSACTION_FREEZE_TIMEOUT:
            stream << "0x80042328L. ( A timeout occured while freezing a transaction manager.)";
            break;
        case VSS_E_TRANSACTION_THAW_TIMEOUT:
            stream << "0x80042329L. (Too much time elapsed between freezing a transaction manager and thawing the transaction manager.)";
            break;
        case VSS_E_WRITERERROR_INCONSISTENTSNAPSHOT:
            stream << "0x800423F0L. (The shadow-copy set only contains only a subset of the volumes needed to correctly backup the selected components of the writer.)";
            break;
        case VSS_E_WRITERERROR_OUTOFRESOURCES:
            stream << "0x800423F1L. (A resource allocation failed while processing this operation.)";
            break;
        case VSS_E_WRITERERROR_TIMEOUT:
            stream << "0x800423F2L. (The writer's timeout expired between the Freeze and Thaw events.)";
            break;
        case VSS_E_WRITERERROR_RETRYABLE:
            stream << "0x800423F3L. (The writer experienced a transient error.  If the backup process is retried,the error may not reoccur.)";
            break;
        case VSS_E_WRITERERROR_NONRETRYABLE:
            stream << "0x800423F4L. (The writer experienced a non-transient error.  If the backup process is retried,the error is likely to reoccur.)";
            break;
        case VSS_E_WRITERERROR_RECOVERY_FAILED:
            stream << "0x800423F5L. (The writer experienced an error while trying to recover the shadow-copy volume.)";
            break;
        case VSS_E_BREAK_REVERT_ID_FAILED:
            stream << "0x800423F6L. (The shadow copy set break operation failed because the disk/partition identities could not be reverted. The target identity already exists on the machine or cluster and must be masked before this operation can succeed.)";
            break;
        case VSS_E_LEGACY_PROVIDER:
            stream << "0x800423F7L. (This version of the hardware provider does not support this operation.)";
            break;
            /*
            case VSS_E_BREAK_FAIL_FROM_PROVIDER	:
            stream << "0x800423F8L. (At least one of the providers in this Shadow Copy set failed the break operation for a snapshot.)" ;
            break;
            */
        case VSS_E_ASRERROR_DISK_ASSIGNMENT_FAILED:
            stream << "0x80042401L. (There are too few disks on this computer or one or more of the disks is too small. Add or change disks so they match the disks in the backup, and try the restore again.)";
            break;
        case VSS_E_ASRERROR_DISK_RECREATION_FAILED:
            stream << "0x80042402L. (Windows cannot create a disk on this computer needed to restore from the backup. Make sure the disks are properly connected, or add or change disks, and try the restore again.)";
            break;
        case VSS_E_ASRERROR_NO_ARCPATH:
            stream << "0x80042403L. (The computer needs to be restarted to finish preparing a hard disk for restore. To continue, restart your computer and run the restore again.)";
            break;
        case VSS_E_ASRERROR_MISSING_DYNDISK:
            stream << "0x80042404L. (The backup failed due to a missing disk for a dynamic volume. Please ensure the disk is online and retry the backup.)";
            break;
        case VSS_E_ASRERROR_SHARED_CRIDISK:
            stream << "0x80042405L. (Automated System Recovery failed the shadow copy, because a selected critical volume is located on a cluster shared disk. This is an unsupported configuration.)";
            break;
        case VSS_E_ASRERROR_DATADISK_RDISK0:
            stream << "0x80042406L. (A data disk is currently set as active in BIOS. Set some other disk as active or use the DiskPart utility to clean the data disk, and then retry the restore operation.)";
            break;
        case VSS_E_ASRERROR_RDISK0_TOOSMALL:
            stream << "0x80042407L. (The disk that is set as active in BIOS is too small to recover the original system disk. Replace the disk with a larger one and retry the restore operation.)";
            break;
        case VSS_E_ASRERROR_CRITICAL_DISKS_TOO_SMALL:
            stream << "0x80042408L. (There is not enough disk space on the system to perform the restore operation. Add another disk or replace one of the existing disks and retry the restore operation.)";
            break;
        case VSS_E_WRITER_STATUS_NOT_AVAILABLE:
            stream << "0x80042409L. (Writer status is not available for one or more writers.  A writer may have reached the limit to the number of available backup-restore session states.)";
            break;
        default:
            stream << hex << "0x" << exitCode << " . (This error code is returned by VSS framework. Please refer to the online MSDN documentation for  the description of this error code.)";
            break;
        }
        stream << std::endl;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}


bool IsSparseFile(const std::string fileName)
{
    bool rv = false;
    ACE_HANDLE hFile = ACE_INVALID_HANDLE;
    // Open the file for read
    do
    {
        if ((hFile = ACE_OS::open(getLongPathName(fileName.c_str()).c_str(), O_RDONLY)) == ACE_INVALID_HANDLE)
        {
            rv = false;
            break;
        }
        // Get file information
        BY_HANDLE_FILE_INFORMATION bhfi;
        if (!GetFileInformationByHandle(hFile, &bhfi))
        {
            rv = false;
            break;
        }

        if (bhfi.dwFileAttributes & FILE_ATTRIBUTE_SPARSE_FILE)
        {
            rv = true;
            break;
        }

    } while (false);

    if (hFile != ACE_INVALID_HANDLE)
    {
        ACE_OS::close(hFile);
    }
    return rv;
}

char* AllocatePageAlignedMemory(const size_t &length)
{
    return (new (std::nothrow) char[length]);
}

void FreePageAlignedMemory(char *readdata)
{
    delete[] readdata;
}

SVERROR GetFileSystemTypeForVolumeUtf8(const char * volume, int & fstype, bool & hidden)
{
    SVERROR sve = SVS_OK;
    int bRevertFilePtr = FALSE;
    ACE_HANDLE handle = ACE_INVALID_HANDLE;
    char *pchDeltaBuffer = NULL;
    long cbRead = 0;
    unsigned int sectorSize = 4096;
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    do
    {
        if (NULL == volume)
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_ERROR, "FAILED @ LINE %d in FILE %s in Function %s err = EINVAL\n", __LINE__, __FILE__, FUNCTION_NAME);
            break;
        }

        sve = OpenVolumeExtendedUtf8(&handle, (char*)volume, GENERIC_READ);
        if (sve.failed() || ACE_INVALID_HANDLE == handle)
        {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "%s: GetFileSystemTypeForVolumeUtf8 failed for volume %s with err = %s \n", FUNCTION_NAME, volume, sve.toString());
            break;
        }

        SharedAlignedBuffer buffer(sectorSize, sectorSize);

        pchDeltaBuffer = buffer.Get();
        if (NULL == pchDeltaBuffer)
        {
            sve = ENOMEM;
            break;
        }

        //Read one sector
        cbRead = ACE_OS::read(handle, pchDeltaBuffer, sectorSize);

        if (cbRead != sectorSize)
        {
            sve = ACE_OS::last_error();
            std::string errstr = ACE_OS::strerror(ACE_OS::last_error());
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read() failed for volume %s with error %s - %s\n", volume, sve.toString(), errstr.c_str());
            break;
        }

        sve = GetFileSystemType(pchDeltaBuffer, fstype, hidden);

    } while (FALSE);

    // Close any handles
    (void)CloseVolume(handle);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return sve;
}

SVERROR GetFileSystemTypeForVolume(const char * volume, int & fstype, bool & hidden)
{
    SVERROR sve = SVS_OK;
    int bRevertFilePtr = FALSE;
    ACE_HANDLE handle = ACE_INVALID_HANDLE;
    char *pchDeltaBuffer = NULL;
    long cbRead = 0;
    unsigned int sectorSize = 4096;
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    do
    {
        if (NULL == volume)
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_ERROR, "FAILED @ LINE %d in FILE %s in Function %s err = EINVAL\n", __LINE__, __FILE__, FUNCTION_NAME);
            break;
        }

        sve = OpenVolumeExtended(&handle, (char*)volume, GENERIC_READ);
        if (sve.failed() || ACE_INVALID_HANDLE == handle)
        {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_DEBUG, "%s: OpenVolumeExtended failed for volume %s with err = %s \n", FUNCTION_NAME, volume, sve.toString());
            break;
        }

        SharedAlignedBuffer buffer(sectorSize, sectorSize);

        pchDeltaBuffer = buffer.Get();
        if (NULL == pchDeltaBuffer)
        {
            sve = ENOMEM;
            break;
        }

        //Read one sector
        cbRead = ACE_OS::read(handle, pchDeltaBuffer, sectorSize);

        if (cbRead != sectorSize)
        {
            sve = ACE_OS::last_error();
            std::string errstr = ACE_OS::strerror(ACE_OS::last_error());
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(SV_LOG_ERROR, "FAILED ACE_OS::read() failed for volume %s with error %s - %s\n", volume, sve.toString(), errstr.c_str());
            break;
        }

        sve = GetFileSystemType(pchDeltaBuffer, fstype, hidden);

    } while (FALSE);

    // Close any handles
    (void)CloseVolume(handle);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return sve;
}

SVERROR GetFileSystemType(const char * buffer, int & fstype, bool & hidden)
{
    SVERROR sve = SVS_OK;

    do
    {
        if (NULL == buffer)
        {
            sve = EINVAL;
            DebugPrintf(SV_LOG_ERROR, "FAILED @ LINE %d in FILE %s in Function %s err = EINVAL\n", __LINE__, __FILE__, FUNCTION_NAME);
            break;
        }

        sve = SVE_FAIL;

        int num_of_fs_supported = sizeof(FileSystemTag) / sizeof(FileSystemTag[0]);
        int index = 1; //Starting with 1 as we handle FAT masking in a different way
        for (; index < num_of_fs_supported; index++)
        {
            // Check if this is of input FS type
            if (0 == memcmp(&buffer[SV_NTFS_MAGIC_OFFSET], FileSystemTag[index], SV_FSMAGIC_LEN))
            {
                DebugPrintf(SV_LOG_DEBUG, "%s: FileSystem type is %s\n", FUNCTION_NAME, FileSystemTag[index]);
                sve = SVS_OK;
                break;
            }
        }
        fstype = index;

        //Check if the volume is hidden.
        if (fstype == TYPE_UNKNOWNFS)
        {
            for (index = 1; index < num_of_fs_supported; index++)
            {
                // Check if this is of input FS type
                if (0 == memcmp(&buffer[SV_NTFS_MAGIC_OFFSET], SvFileSystemMaskTag[index], SV_FSMAGIC_LEN))
                {
                    DebugPrintf(SV_LOG_DEBUG, "%s: FileSystem type is %s\n", FUNCTION_NAME, SvFileSystemMaskTag[index]);
                    hidden = true;
                    sve = SVS_OK;
                    break;
                }
            }
            fstype = index;
        }


        //Check if the filesystem is FAT. FAT should be checked only after the possibility of other filesystems is ruled out.
        //An NTFS or corresponing SVFS0001 drive might seem like a FAT fs as the following might be true for them.
        if (fstype == TYPE_UNKNOWNFS)
        {
            Bpb *bpb = (Bpb *)(buffer);
            if (bpb->BS_jmpBoot[0] == BS_JMP_BOOT_VALUE_0xEB || bpb->BS_jmpBoot[0] == BS_JMP_BOOT_VALUE_0xE9) {
                if (buffer[FST_SCTR_OFFSET_510] == FST_SCTR_OFFSET_510_VALUE &&
                    buffer[FST_SCTR_OFFSET_511] == FST_SCTR_OFFSET_511_VALUE)
                {
                    DebugPrintf(SV_LOG_DEBUG, "%s: FileSystem type is %s\n", FUNCTION_NAME, "FAT");
                    fstype = TYPE_FAT;
                    sve = SVS_OK;
                    break;
                }
            }
        }

        //Check if the filesystem is HIDDEN FAT
        if (fstype == TYPE_UNKNOWNFS)
        {
            //Check for FAT configuration.
            Bpb *bpb = (Bpb *)(buffer);
            if (bpb->BS_jmpBoot[0] == (BS_JMP_BOOT_VALUE_0xEB + 1) || bpb->BS_jmpBoot[0] == (BS_JMP_BOOT_VALUE_0xE9 + 1)) {
                if (buffer[FST_SCTR_OFFSET_510] == FST_SCTR_OFFSET_510_VALUE &&
                    buffer[FST_SCTR_OFFSET_511] == FST_SCTR_OFFSET_511_VALUE)
                {
                    DebugPrintf(SV_LOG_DEBUG, "%s: FileSystem type is %s\n", FUNCTION_NAME, "FAT (hidden)");
                    hidden = true;
                    fstype = TYPE_FAT;
                    sve = SVS_OK;
                    break;
                }
            }
        }


    } while (FALSE);

    return sve;
}


SVERROR HideFileSystem(char * buffer, int size)
{
    SVERROR sve = SVS_OK;
    do
    {
        //Get the filesystem type. If the file system is UNKNOWN return.
        int fstype;
        bool hidden = false;
        if (!GetFileSystemType(buffer, fstype, hidden).succeeded())
        {
            sve = SVS_FALSE;
            DebugPrintf(SV_LOG_DEBUG, "%s: Filesystem is UNKNOWN, could be a raw volume\n", FUNCTION_NAME);
            break;
        }
        else if (hidden)
        {
            sve = SVE_FAIL;
            DebugPrintf(SV_LOG_DEBUG, "%s: Filesystem is Hidden\n", FUNCTION_NAME);
            break;
        }

        if (fstype == TYPE_FAT)
        {
            Bpb * bpb = (Bpb *)buffer;
            // Change the BPB to hide it
            // i.e. We make jmpboot instruction to be either 0xEC, or 0xEA 
            // Which makes it an invalid FAT volume
            bpb->BS_jmpBoot[0] += 1;
        }
        else
        {
            // Patch the MAGIC to make it an SVFS volume.
            inm_memcpy_s(&buffer[SV_NTFS_MAGIC_OFFSET], (size - SV_NTFS_MAGIC_OFFSET), SvFileSystemMaskTag[fstype], SV_FSMAGIC_LEN);
        }
    } while (FALSE);
    return sve;
}


SVERROR UnHideFileSystem(char * buffer, int size)
{
    SVERROR sve = SVS_OK;
    do
    {
        //Get the filesystem type. If the file system is UNKNOWN return.
        int fstype;
        bool hidden = false;
        if (!GetFileSystemType(buffer, fstype, hidden).succeeded())
        {
            sve = SVS_FALSE;
            DebugPrintf(SV_LOG_DEBUG, "%s: Filesystem is UNKNOWN, could be a raw volume\n", FUNCTION_NAME);
            break;
        }
        else if (!hidden)
        {
            sve = SVE_FAIL;
            DebugPrintf(SV_LOG_DEBUG, "%s: Filesystem is Not Hidden\n", FUNCTION_NAME);
            break;
        }

        if (fstype == TYPE_FAT)
        {
            Bpb * bpb = (Bpb *)buffer;
            // Change the BPB to hide it
            // i.e. We make jmpboot instruction to be either 0xEC, or 0xEA 
            // Which makes it an invalid FAT volume
            bpb->BS_jmpBoot[0] -= 1;
        }
        else
        {
            // Patch the MAGIC to make it an SVFS volume.
            inm_memcpy_s(&buffer[SV_NTFS_MAGIC_OFFSET], (size - SV_NTFS_MAGIC_OFFSET), FileSystemTag[fstype], SV_FSMAGIC_LEN);
        }
    } while (FALSE);
    return sve;
}
unsigned int retrieveBusType(std::string volume)
{
    BYTE buffer[1024];
    DWORD length = 0;
    unsigned int bustype = 0;
    DWORD ret;
    DWORD returned;
    DWORD err;
    string UniqueId;
    UniqueId.clear();
    UniqueId = volume.c_str();
    UniqueId[0] = tolower(UniqueId[0]);
    FormatVolumeName(UniqueId);
    FormatVolumeNameToGuid(UniqueId);

    HANDLE hVolume = SVCreateFile(UniqueId.c_str(), FILE_READ_ATTRIBUTES, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
    if (INVALID_HANDLE_VALUE != hVolume)
    {
        STORAGE_PROPERTY_QUERY StoragePropertyQuery;
        StoragePropertyQuery.PropertyId = StorageDeviceProperty;
        StoragePropertyQuery.QueryType = PropertyStandardQuery;

        ret = DeviceIoControl(hVolume, IOCTL_STORAGE_QUERY_PROPERTY, &StoragePropertyQuery, sizeof(STORAGE_PROPERTY_QUERY), buffer, 1024, &returned, NULL);
        PSTORAGE_DEVICE_DESCRIPTOR StorageDeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;
        if (ret)
        {
            bustype = StorageDeviceDescriptor->BusType;
        }
        else
        {
            err = GetLastError();
            std::stringstream errmsg;
            errmsg << "IOCTL_DISK_GET_CACHE_INFORMATION for " << volume.c_str()
                << " failed with error = " << err
                << '\n';
            DebugPrintf(SV_LOG_ERROR, "%s", errmsg.str().c_str());

        }
        CloseHandle(hVolume);
    }
    else
    {
        err = GetLastError();
        std::stringstream errmsg;
        errmsg << "SVCreateFile for " << volume.c_str()
            << " failed with error = " << err
            << '\n';
        DebugPrintf(SV_LOG_ERROR, "%s", errmsg.str().c_str());
    }
    return bustype;
}

long GetAgentHeartBeatTime()
{
    CRegKey regkey;
    DWORD err;
    DWORD heartbeattime = 0;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.QueryDWORDValue("SuccessfulAgentHeartBeatTimeInGMT", heartbeattime)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to query the agent heart beat %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
    return heartbeattime;
}



void UpdateRepositoryAccess()
{
    CRegKey regkey;
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.SetDWORDValue("SuccessfulRepoAccessTimeInGMT", time(NULL))) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update last successful repo access time stamp %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
}

long GetRepositoryAccessTime()
{
    CRegKey regkey;
    DWORD err;
    DWORD dwordValue = 0;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.QueryDWORDValue("SuccessfulRepoAccessTimeInGMT", dwordValue)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update last successful repo access time stamp %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
    return dwordValue;
}

void UpdateBookMarkSuccessTimeStamp()
{
    CRegKey regkey;
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.SetDWORDValue("SuccessfulBookMarkTimeInGMT", time(NULL))) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update last successful bookmark time time stamp %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
}
long GetBookMarkSuccessTimeStamp()
{
    CRegKey regkey;
    DWORD err;
    DWORD dwordValue = 0;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.QueryDWORDValue("SuccessfulBookMarkTimeInGMT", dwordValue)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_WARNING, "Failed to query last successful bookmark time stamp %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
    return dwordValue;
}

void GetAgentRepositoryAccess(int& accessError, SV_LONG& lastSuccessfulTs, std::string& errmsg)
{
    CRegKey regkey;
    DWORD err;
    accessError = -1;
    lastSuccessfulTs = -1;
    errmsg = "";
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        DWORD dwordValue = 0;
        if ((err = regkey.QueryDWORDValue("AgentRepoAccessErrorCode", dwordValue)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to query the repository access error code %ld\n", err);
        }
        else
        {
            accessError = dwordValue;
        }
        dwordValue = 0;
        if ((err = regkey.QueryDWORDValue("SuccessfulAgentRepoAccessTimeInGMT", dwordValue)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to query last successful repo access time stamp by agent %ld\n", err);
        }
        else
        {
            lastSuccessfulTs = dwordValue;
        }
        char szValue[1024];
        memset(szValue, '\0', sizeof(dwordValue));
        DWORD dwCount = sizeof(szValue);
        if ((err = regkey.QueryStringValue("AgentRepoAccessErrorStr", szValue, &dwCount)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to query agent repo access error string %ld\n", err);

        }
        else
        {
            errmsg = szValue;
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
}


void UpdateAgentRepositoryAccess(int accessError, const std::string& errmsg)
{
    CRegKey regkey;
    DebugPrintf(SV_LOG_DEBUG, "Setting the access error %d\n", accessError);
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.SetDWORDValue("AgentRepoAccessErrorCode", accessError)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update the repository access error code %ld\n", err);
        }
        if (accessError == 0)
        {
            if ((err = regkey.SetDWORDValue("SuccessfulAgentRepoAccessTimeInGMT", time(NULL))) != ERROR_SUCCESS)
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to update last successful repoaccess time stamp  by agent %ld\n", err);
            }
        }
        if ((err = regkey.SetStringValue("AgentRepoAccessErrorStr", errmsg.c_str())) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update the repository access error string %ld\n", err);

        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
}

void UpdateBackupAgentHeartBeatTime()
{
    CRegKey regkey;
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.SetDWORDValue("SuccessfulAgentHeartBeatTimeInGMT", time(NULL))) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update the agent heart beat %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
}
void UpdateServiceStopReason(std::string& reason)
{
    CRegKey regkey;
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        if ((err = regkey.SetStringValue("ServiceStopReason", reason.c_str())) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to update the agent heart beat %ld\n", err);
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }

}

std::string GetServiceStopReason()
{
    std::string serviceStopReason;
    CRegKey regkey;
    DWORD err;
    if ((err = regkey.Open(HKEY_LOCAL_MACHINE, "Software\\SV Systems\\VxAgent")) == ERROR_SUCCESS)
    {
        char szReason[4 * MAX_PATH];
        DWORD dwCount = sizeof(szReason);
        if ((err = regkey.QueryStringValue("ServiceStopReason", szReason, &dwCount)) != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_DEBUG, "Failed to query the service stop reason code %ld\n", err);
        }
        else
        {
            serviceStopReason = szReason;
        }
        regkey.Close();
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open registry %ld\n", err);
    }
    return serviceStopReason;
}


void PrintDiskLayout(const std::string &diskname)
{
    std::stringstream ss;
    HANDLE  h = CreateFile(diskname.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL
        );

    if (h == INVALID_HANDLE_VALUE)
    {
        ss << "Failed to open disk " << diskname << " with error " << GetLastError();
        DebugPrintf(SV_LOG_ERROR, "%s\n", ss.str().c_str());
        return;
    }
    int EXPECTEDPARTITIONS = 2;
    bool bexit = false;
    DRIVE_LAYOUT_INFORMATION_EX *dli = NULL;
    unsigned int drivelayoutsize;
    DWORD bytesreturned;
    DWORD err;

    do
    {
        if (dli)
        {
            free(dli);
            dli = NULL;
        }
        drivelayoutsize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + ((EXPECTEDPARTITIONS - 1) * sizeof(PARTITION_INFORMATION_EX));
        dli = (DRIVE_LAYOUT_INFORMATION_EX *)calloc(1, drivelayoutsize);

        if (NULL == dli)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to allocate %u bytes for %d expected partitions, "
                "for IOCTL_DISK_GET_DRIVE_LAYOUT_EX\n", drivelayoutsize,
                EXPECTEDPARTITIONS);
            break;
        }

        bytesreturned = 0;
        err = 0;
        if (DeviceIoControl(h, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, dli, drivelayoutsize, &bytesreturned, NULL))
        {
            DebugPrintf(SV_LOG_DEBUG, "For disk %s\n", diskname.c_str());
            PrintDriveLayoutInfoEx(dli, diskname);
            bexit = true;
        }
        else if ((err = GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
        {
            DebugPrintf(SV_LOG_DEBUG, "with EXPECTEDPARTITIONS = %d, IOCTL_DISK_GET_DRIVE_LAYOUT_EX says insufficient buffer\n", EXPECTEDPARTITIONS);
            EXPECTEDPARTITIONS *= 2;
        }
        else
        {
            ss << "IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed on disk " << diskname << " with error " << err;
            DebugPrintf(SV_LOG_ERROR, "%s\n", ss.str().c_str());
            bexit = true;
        }
    } while (!bexit);

    if (dli)
    {
        free(dli);
        dli = NULL;
    }

    CloseHandle(h);
}


void PrintDriveLayoutInfoEx(const DRIVE_LAYOUT_INFORMATION_EX *pdli, const std::string &diskname)
{
    std::ostringstream ss;
    ss << "DRIVE_LAYOUT_INFORMATION_EX:\n";
    ss << "PartitionStyle: " << pdli->PartitionStyle
        << ", PartitionCount: " << pdli->PartitionCount << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());

    if (PARTITION_STYLE_MBR == pdli->PartitionStyle)
        PrintDriveLayoutInfoMbr(pdli->Mbr);
    else if (PARTITION_STYLE_GPT == pdli->PartitionStyle)
        PrintDriveLayoutInfoGpt(pdli->Gpt);

    for (DWORD i = 0; i < pdli->PartitionCount; i++)
    {
        const PARTITION_INFORMATION_EX &p = pdli->PartitionEntry[i];
        PrintPartitionInfoEx(p);
    }
}


void PrintDriveLayoutInfoMbr(const DRIVE_LAYOUT_INFORMATION_MBR &mbr)
{
    std::ostringstream ss;
    ss << "DRIVE_LAYOUT_INFORMATION_MBR:\n"
        << "Signature: " << mbr.Signature << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
}


void PrintDriveLayoutInfoGpt(const DRIVE_LAYOUT_INFORMATION_GPT &gpt)
{
    std::ostringstream ss;
    ss << "DRIVE_LAYOUT_INFORMATION_GPT:\n";
    ss << "DiskId:\n";
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
    PrintGUID(gpt.DiskId);

    std::ostringstream remss;
    remss << "StartingUsableOffset: " << gpt.StartingUsableOffset.QuadPart
        << ", UsableLength: " << gpt.UsableLength.QuadPart
        << ", MaxPartitionCount: " << gpt.MaxPartitionCount << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", remss.str().c_str());
}


void PrintGUID(const GUID &guid)
{
    std::ostringstream ss;
    ss << std::hex;
    ss << "data1: " << guid.Data1
        << ", data2: " << guid.Data2
        << ", data3: " << guid.Data3
        << ", data4: " << ((unsigned int)guid.Data4[0])
        << ' ' << ((unsigned int)guid.Data4[1])
        << ' ' << ((unsigned int)guid.Data4[2])
        << ' ' << ((unsigned int)guid.Data4[3])
        << ' ' << ((unsigned int)guid.Data4[4])
        << ' ' << ((unsigned int)guid.Data4[5])
        << ' ' << ((unsigned int)guid.Data4[6])
        << ' ' << ((unsigned int)guid.Data4[7]) << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
}


void PrintPartitionInfoEx(const PARTITION_INFORMATION_EX &p)
{
    std::ostringstream ss;
    ss << "PARTITION_INFORMATION_EX:\n";
    ss << "PartitionStyle: " << p.PartitionStyle
        << ", StartingOffset: " << p.StartingOffset.QuadPart
        << ", PartitionLength: " << p.PartitionLength.QuadPart
        << ", PartitionNumber: " << p.PartitionNumber
        << ", RewritePartition: " << STRBOOL(p.RewritePartition) << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
    if (PARTITION_STYLE_MBR == p.PartitionStyle)
        PrintPartitionInfoMbr(p.Mbr);
    else if (PARTITION_STYLE_GPT == p.PartitionStyle)
        PrintPartitionInfoGpt(p.Gpt);
}


void PrintPartitionInfoMbr(const PARTITION_INFORMATION_MBR &mbr)
{
    std::ostringstream ss;
    ss << "PARTITION_INFORMATION_MBR:\n";
    ss << "PartitionType: " << (unsigned int)mbr.PartitionType
        << ", BootIndicator: " << STRBOOL(mbr.BootIndicator)
        << ", RecognizedPartition: " << STRBOOL(mbr.RecognizedPartition)
        << ", HiddenSectors: " << mbr.HiddenSectors << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
}


void PrintPartitionInfoGpt(const PARTITION_INFORMATION_GPT &gpt)
{
    std::ostringstream ss;
    ss << "PARTITION_INFORMATION_GPT:\n";
    ss << "PartitionType:\n";
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
    PrintGUID(gpt.PartitionType);

    std::ostringstream pss;
    pss << "PartitionId:\n";
    DebugPrintf(SV_LOG_DEBUG, "%s", pss.str().c_str());
    PrintGUID(gpt.PartitionId);

    std::ostringstream remss;
    remss << std::hex;
    remss << "Attributes: " << gpt.Attributes;
    remss << std::dec;
    std::wstring wsname(gpt.Name);
    std::string name(wsname.begin(), wsname.end());
    remss << ", Name: " << name << '\n';
    DebugPrintf(SV_LOG_DEBUG, "%s", remss.str().c_str());
}

/*
Description:
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
    LPTSTR szCommand)
{
    DWORD dwRet = ERROR_SUCCESS;
    do {
        if (!hService || hService == INVALID_HANDLE_VALUE)
        {
            dwRet = ERROR_INVALID_HANDLE;
            break;
        }

        SERVICE_FAILURE_ACTIONS srv_failure_actions = { 0 };
        srv_failure_actions.dwResetPeriod = resetPeriodSec;
        srv_failure_actions.lpRebootMsg = szRebootMsg;
        srv_failure_actions.lpCommand = szCommand;
        srv_failure_actions.cActions = cActions;
        srv_failure_actions.lpsaActions = lpActions;

        if (!ChangeServiceConfig2(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &srv_failure_actions))
            dwRet = GetLastError();

    } while (false);

    return dwRet;
}

DWORD SetServiceFailureActionsRecomended(SC_HANDLE hService)
{
    SC_ACTION scActions[3];

    //On first failure restart the service
    scActions[0].Delay = INM_SERVICE_FIRST_FAILURE_RESTART_DELAY_MSEC;
    scActions[0].Type = SC_ACTION_RESTART;

    //On second failure restart the service
    scActions[1].Delay = INM_SERVICE_SECOND_FAILURE_RESTART_DELAY_MSEC;
    scActions[1].Type = SC_ACTION_RESTART;

    //No-action on further failures
    scActions[2].Delay = 0;
    scActions[2].Type = SC_ACTION_NONE;

    //Set failure count reset period
    DWORD dwRestPeriodSec = INM_SERVICE_FAILURE_COUNT_RESET_PERIOD_SEC;

    return SetServiceFailureActions(
        hService,
        dwRestPeriodSec,
        sizeof(scActions) / sizeof(SC_ACTION),
        scActions,
        NULL,
        NULL
        );
}

DWORD GetVolumeDiskExtents(const std::string& volName, disk_extents_t& extents)
{
    DWORD dwRet = ERROR_SUCCESS;
    HANDLE hVolume = INVALID_HANDLE_VALUE;
    DWORD dwBytesReturned = 0;
    DWORD cbInBuff = 0;
    PVOLUME_DISK_EXTENTS pVolDiskExt = NULL;
    std::string volumeName = volName;

    if (volumeName.empty())
    {
        DebugPrintf(SV_LOG_ERROR, "%s:Line %d: ERROR: Volume Name can not be empty.\n", __FUNCTION__, __LINE__);
        return ERROR_INVALID_PARAMETER;
    }

    do {
        //Get the volume guid name of the volume/mountpoint. The \\?\Volume{guid} will be used to open the volume handle.
        if (!FormatVolumeNameToGuid(volumeName))
        {
            DebugPrintf(SV_LOG_ERROR, "%s:Line %d: Could not get volume guid name for the volume: %s.\n", __FUNCTION__, __LINE__, volumeName.c_str());
            dwRet = ERROR_INVALID_PARAMETER;
            break;
        }

        //Get the volume handle
        HANDLE hVolume = INVALID_HANDLE_VALUE;
        hVolume = CreateFile(
            volumeName.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
            NULL
            );

        if (hVolume == INVALID_HANDLE_VALUE)
        {
            dwRet = GetLastError();
            DebugPrintf(SV_LOG_ERROR, "%s:Line %d: Could not open the volume-%s. CreateFile failed with Error Code - %lu \n", __FUNCTION__, __LINE__, volumeName.c_str(), dwRet);
            break;
        }

        //  Allocate default buffer sizes. 
        //  If the volume is created on basic disk then there will be only one disk extent for the volume, 
        //     and this default buffer will be enough to accomodate extent info.
        cbInBuff = sizeof(VOLUME_DISK_EXTENTS);
        pVolDiskExt = (PVOLUME_DISK_EXTENTS)malloc(cbInBuff);
        if (NULL == pVolDiskExt)
        {
            dwRet = ERROR_OUTOFMEMORY;
            DebugPrintf(SV_LOG_FATAL, "%s:Line %d: Out of memory\n", __FUNCTION__, __LINE__);
            break;
        }

        if (!DeviceIoControl(
            hVolume,
            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
            NULL,
            0,
            pVolDiskExt,
            cbInBuff,
            &dwBytesReturned,
            NULL
            ))
        {
            dwRet = GetLastError();

            if (dwRet == ERROR_MORE_DATA)
            {
                //  If the volume is created on dynamic disk then there will be posibility that more than one extent exist for the volume.
                //  Calculate the size required to accomodate all extents.
                cbInBuff = FIELD_OFFSET(VOLUME_DISK_EXTENTS, Extents) + pVolDiskExt->NumberOfDiskExtents * sizeof(DISK_EXTENT);

                //  Re-allocate the memory to new size
                pVolDiskExt = (PVOLUME_DISK_EXTENTS)realloc(pVolDiskExt, cbInBuff);
                if (NULL == pVolDiskExt)
                {
                    dwRet = ERROR_OUTOFMEMORY;
                    DebugPrintf(SV_LOG_FATAL, "%s:Line %d: Out of memory\n", __FUNCTION__, __LINE__);
                    break;
                }

                if (!DeviceIoControl(
                    hVolume,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,
                    0,
                    pVolDiskExt,
                    cbInBuff,
                    &dwBytesReturned,
                    NULL
                    ))
                {
                    dwRet = GetLastError();
                    DebugPrintf(SV_LOG_ERROR, "%s:Line %d: Cloud not get the volume disk extents. DeviceIoControl failed with Error %lu\n", __FUNCTION__, __LINE__, dwRet);
                    break;
                }
                else
                {
                    dwRet = ERROR_SUCCESS;
                }
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "%s:Line %d: Cloud not get the volume disk extents. DeviceIoControl failed with Error %lu\n", __FUNCTION__, __LINE__, dwRet);
                break;
            }
        }

        //Fill disk_extents_t structure with retrieved disk extents
        for (int i_extent = 0; i_extent < pVolDiskExt->NumberOfDiskExtents; i_extent++)
        {
            std::stringstream diskName;
            diskName << "\\\\.\\PhysicalDrive" << pVolDiskExt->Extents[i_extent].DiskNumber;

            //Here disk_id will be a signature if disk is MBR type, a GUID if GPT type.
            std::string storage_type, vg_name, disk_id;
            VolumeSummary::FormatLabel lable;
            VolumeSummary::Vendor vendor;
            GetDiskAttributes(diskName.str(), storage_type, lable, vendor, vg_name, disk_id);

            disk_extent extent(
                disk_id,
                pVolDiskExt->Extents[i_extent].StartingOffset.QuadPart,
                pVolDiskExt->Extents[i_extent].ExtentLength.QuadPart
                );

            extents.push_back(extent);
        }

    } while (false);

    if (NULL != pVolDiskExt)
        free(pVolDiskExt);

    if (INVALID_HANDLE_VALUE != hVolume)
        CloseHandle(hVolume);

    return dwRet;
}


SV_ULONGLONG GetDiskSize(const HANDLE &h, std::string &errMsg)
{
    SV_ULONGLONG size = 0;

    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    if (INVALID_HANDLE_VALUE == h) {
        errMsg = "Handle provided to find disk size is invalid.";
        return 0;
    }

    GET_LENGTH_INFORMATION dInfo;
    DWORD dInfoSize = sizeof(GET_LENGTH_INFORMATION);
    DWORD dwValue = 0;
    bool gotcapacity = DeviceIoControl(h, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &dInfo, dInfoSize, &dwValue, NULL);
    if (gotcapacity) {
        size = dInfo.Length.QuadPart;
        DebugPrintf(SV_LOG_DEBUG, "Device Size: %I64u\n", size);
    }
    else {
        DWORD err = GetLastError();
        std::stringstream sserr;
        sserr << err << ", bytes returned: " << dwValue;
        errMsg = "IOCTL_DISK_GET_LENGTH_INFO failed with error code: ";
        errMsg += sserr.str().c_str();
        DebugPrintf(SV_LOG_DEBUG, "%s\n", errMsg.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return size;
}

std::string convertWstringToUtf8(const std::wstring &wstr)
{
    if (wstr.empty())
        return std::string("");

    const int reqSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strResult(reqSize, 0);
    int ret = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)&wstr[0], (int)wstr.length(), (LPSTR)&strResult[0], strResult.size(), NULL, NULL);
    if (ret == 0)
    {
        DWORD err = GetLastError();
        DebugPrintf(SV_LOG_ERROR,
            "WideCharToMultiByte Failed. Error: %d. @LINE %d in FILE %s \n",
            err,
            LINE_NO,
            FILE_NAME);
        throw ERROR_EXCEPTION << "WideCharToMultiByte failed. Error: " << err << '\n';
    }

    return strResult;
}

std::wstring convertUtf8ToWstring(const std::string &str)
{
    if (str.empty())
        return std::wstring(L"");

    const int reqSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&str[0], (int)str.size(), NULL, 0);
    std::wstring wstrResult(reqSize, 0);
    int ret = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)&str[0], (int)str.length(), (LPWSTR)&wstrResult[0], wstrResult.size());
    if (ret == 0)
    {
        DWORD err = GetLastError();
        DebugPrintf(SV_LOG_ERROR,
            "MultiByteToWideChar Failed. Error: %d. @LINE %d in FILE %s \n",
            err,
            LINE_NO,
            FILE_NAME);
        throw ERROR_EXCEPTION << "MultiByteToWideChar failed. Error: " << err << '\n';
    }
    return wstrResult;
}

bool IsRecoveryInProgress()
{
    DWORD dwRecoveryInProgress = 0;

    VERIFY_REG_STATUS(
        GetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_RECOVERY_INPROGRESS, dwRecoveryInProgress, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not determine Recovery InPogress"
        );

    return dwRecoveryInProgress == 1;
}

void SetRecoveryCompleted()
{
    VERIFY_REG_STATUS(
        SetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_RECOVERY_INPROGRESS, 0, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not set Recovery Complete"
        );
}

void SetRecoveryInProgress(const std::string& hiveRoot)
{
    VERIFY_REG_STATUS(
        SetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_RECOVERY_INPROGRESS, 1, hiveRoot),
        "Could not set Recovery InProgress"
        );
}

bool IsItTestFailoverVM()
{
    DWORD dwTestFailover = 0;

    VERIFY_REG_STATUS(
        GetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_TEST_FAILOVER, dwTestFailover, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not determine test failover"
        );

    return dwTestFailover == 1;
}

void GetAgentVersionFromReg(std::string& prod_ver, const std::string& hiveRoot)
{
    VERIFY_REG_STATUS(
        GetSVSystemStringValue(REG_SV_SYSTEMS::VALUE_NAME_PROD_VERSION, prod_ver, hiveRoot, "\\VxAgent"),
        "Could not get VxAgent prod version"
        );
}

void GetNewHostIdFromReg(std::string& newHostId)
{
    VERIFY_REG_STATUS(
        GetSVSystemStringValue(REG_SV_SYSTEMS::VALUE_NAME_NEW_HOSTID, newHostId, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not get New HostId from registry"
        );
}

void SetEnableRdpFlagInReg(const std::string& hiveRoot, DWORD dwValue)
{
    VERIFY_REG_STATUS(
        SetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_ENABLE_RDP, dwValue, hiveRoot),
        "Could not set EnableRDP from registry"
        );
}

void ResetEnableRdpFlagInReg()
{
    SetEnableRdpFlagInReg(REG_SV_SYSTEMS::SOFTWARE_HIVE, 0);
}

bool GetEnableRdpFlagFromReg()
{
    DWORD dwEnableRdp = 0;

    VERIFY_REG_STATUS(
        GetSVSystemDWORDValue(REG_SV_SYSTEMS::VALUE_NAME_ENABLE_RDP, dwEnableRdp, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not get EnableRDP from registry"
        );

    return dwEnableRdp == 1;
}

void SetNewHostIdInReg(const std::string& newHostId, const std::string& hiveRoot)
{
    VERIFY_REG_STATUS(
        SetSVSystemStringValue(REG_SV_SYSTEMS::VALUE_NAME_NEW_HOSTID, newHostId, hiveRoot),
        "Could not set New HostId in registry"
        );
}

void GetRecoveredEnv(std::string& recoveredEnd)
{
    VERIFY_REG_STATUS(
        GetSVSystemStringValue(REG_SV_SYSTEMS::VALUE_NAME_RECOVERED_ENV, recoveredEnd, REG_SV_SYSTEMS::SOFTWARE_HIVE),
        "Could not get recovered environment from registry"
        );
}

void SetRecoveredEnv(const std::string& recoveredEnd, const std::string& hiveRoot)
{
    VERIFY_REG_STATUS(
        SetSVSystemStringValue(REG_SV_SYSTEMS::VALUE_NAME_RECOVERED_ENV, recoveredEnd, hiveRoot),
        "Could not set recovered environment in registry"
        );
}

std::string GetRecoveryScriptCmd()
{
    LocalConfigurator lConfig;

    std::string recoveredEnv;
    GetRecoveredEnv(recoveredEnv);
    if (recoveredEnv.empty())
        throw std::string("CloudEnv value should not be empty");

    std::string newHostId;
    GetNewHostIdFromReg(newHostId);
    if (newHostId.empty())
        throw std::string("NewHostId should not be empty");

    DebugPrintf(SV_LOG_ALWAYS, "Failover HostId: %s\n", newHostId.c_str());
    std::stringstream cmd;
    cmd << "\"" << lConfig.getInstallPath() << "\\EsxUtil.exe\""
        << " -atbooting -resetguid"
        << " -cloudenv " << recoveredEnv
        << (boost::iequals(recoveredEnv, "vmware") ? " -p2v" : "")
        << (IsItTestFailoverVM() ? " -testfailover" : "")
        << (GetEnableRdpFlagFromReg() ? " -enablerdpfirewall" : "")
        << " -newhostid " << newHostId
        << " -file \"" << lConfig.getInstallPath() << "\\Failover\\Data\\nw.txt\"";

    return cmd.str();
}

// Returns the UUID value of Win32_ComputerSystemProduct class
// If any failure happens then empty string will be returned.
std::string GetSystemUUID()
{
    WmiComputerSystemProductProcessor::WmiComputerSystemProductClass csProduct;
    WmiComputerSystemProductProcessor csProductProcessor(&csProduct);

    string systemUUID;
    GenericWMI gwmi(&csProductProcessor);
    SVERROR sv = gwmi.init();
    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed\n");
    }
    else
    {
        gwmi.GetData(WMICOMPUTERSYSTEMPRODUCT);
        systemUUID = csProduct.uuid;
    }
    DebugPrintf(SV_LOG_DEBUG, "Computer System Product UUID: %s\n", systemUUID.c_str());

    return boost::to_lower_copy(systemUUID);
}

std::string GetChassisAssetTag()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    WmiSystemEnclosureProcessor     sysEnclosureProcessor;
    GenericWMI qfip(&sysEnclosureProcessor);
    SVERROR sv = qfip.init();

    if (sv != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "initializating generic wmi failed sv=%x\n", sv);
        return "";
    }

    qfip.GetData("Win32_SystemEnclosure");
    
    std::string smbiosAssetTag = sysEnclosureProcessor.GetSMBIOSAssetTag();

    DebugPrintf(SV_LOG_ALWAYS, "SMBIOS Asset tag is %s\n", smbiosAssetTag.c_str());
    DebugPrintf(SV_LOG_DEBUG, "EXITED%s\n", FUNCTION_NAME);
    return smbiosAssetTag;
}

/*
* check for root UEFI disk with number of partitions > 4 and OS Windows8 Or Greater
*/
bool SystemDiskUEFICheck(std::string &errormsg)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool retval = true;
    DWORD   errcode;
    DWORD   dwBytesPerSector;
    SV_UINT uiMaxSupportedUefiPartitions = 0;

    LocalConfigurator   lc;
    try {
        uiMaxSupportedUefiPartitions = lc.getMaxSupportedPartitionsCountUEFIBoot();
    }
    catch (const std::exception &e)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed to get the uefi config params with exception %s\n",
            FUNCTION_NAME,
            e.what());
        return true;
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: failed to get the uefi config params with unknown exception.\n",
            FUNCTION_NAME);
        return true;
    }

    PDRIVE_LAYOUT_INFORMATION_EX pLayoutEx = NULL;
    ON_BLOCK_EXIT(boost::bind<void>(&free, pLayoutEx));
    if (!GetRootDiskLayoutInfo(pLayoutEx, dwBytesPerSector, errormsg, errcode))
    {
        return false;
    }

    if (PARTITION_STYLE_GPT != pLayoutEx->PartitionStyle)
    {
        // system disk is not UEFI, return success
        return true;
    }

    if (DEFAULT_SECTOR_SIZE != dwBytesPerSector) {
        std::stringstream ss;
        ss << "system disk has UEFI with " << dwBytesPerSector << " bytes per sector. ";
        ss << "Only uefi system disk with size of 512 is supported.";
        errormsg = ss.str();
        return  false;
    }

    if (pLayoutEx->PartitionCount > uiMaxSupportedUefiPartitions)
    {
        std::stringstream ss;
        ss << "system disk has UEFI with " << pLayoutEx->PartitionCount << " partitions. ";
        ss << "max " << uiMaxSupportedUefiPartitions << " partitions supported for UEFI system/root disk.";
        errormsg = ss.str();
        return false;
    }

    if (!IsWindowsVersionOrGreater(6, 2, 0))
    {
        std::stringstream ss;
        ss << "OS version check failed. Min supported Windows OS v6.2 Or greater if system disk is UEFI.";
        errormsg = ss.str();
        return false;
    }

    return true;
}

bool IsSystemAndBootDiskSame(const VolumeSummaries_t &volumeSummaries, std::string &errormsg)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    /*
    - Check all devices with isSystemVolume set and collect their volumeGroup (disk signature here) in vector vSysVG
    - Fail if no device found - bug
    - Fail if volumeGroup are different
    */
    std::set<std::string> vSysVG;
    std::set<std::string> vBootableVG;
    std::string strSysVG;
    ConstVolumeSummariesIter itrVS(volumeSummaries.begin());
    for (/*empty*/; itrVS != volumeSummaries.end(); ++itrVS)
    {
        if (itrVS->isSystemVolume)
        {
            DebugPrintf(SV_LOG_DEBUG, "vSysVG: %s\n", itrVS->volumeGroup.c_str());
            vSysVG.insert(itrVS->volumeGroup);
            strSysVG += itrVS->volumeGroup;
            strSysVG += ", ";
        }
    }

    if (vSysVG.size() == 0)
    {
        /* Note: If we are here then its a bug in VIC */
        errormsg = "No device found in volumeSummaries with System volume set.";
        return false;
    }
    if (vSysVG.size() > 1)
    {
        errormsg = "System/root volume identified on multiple disks (";
        for (auto it : vSysVG) {
            errormsg += (it + ",");
        }
        errormsg += ").";
        return false;
    }

    /*
    - Find first disk in volumeSummaries with bootable partition attribute set
    - Fail if not found - bug
    */
    for (itrVS = volumeSummaries.begin(); itrVS != volumeSummaries.end(); ++itrVS)
    {
        ConstAttributesIter_t it = itrVS->attributes.find(NsVolumeAttributes::HAS_BOOTABLE_PARTITION);
        if (it != itrVS->attributes.end() && (0 == it->second.compare(STRBOOL(true))))
        {
            DebugPrintf(SV_LOG_DEBUG, "vBootableVG: %s\n", itrVS->volumeGroup.c_str());
            vBootableVG.insert(itrVS->volumeGroup);
        }
    }

    if (0 == vBootableVG.size())
    {
        /* Note: If we are here then its a bug in VIC */
        errormsg = "No device found in volumeSummaries with has_bootable_partition attribute set.";
        return false;
    }

    bool    isSuccess = true;
    /*
    - Check system volume group and boot volume group match
    - Fail if different - boot and root on different disk
    */
    if (vBootableVG.find(vSysVG.begin()->c_str()) == vBootableVG.end())
    {
        errormsg = "System/root volume and bootable volume identified on different disks.";
        errormsg += " System disk: ";
        errormsg += *(vSysVG.begin());
        errormsg += ", boot disk: ";
        for (auto it : vBootableVG) {
            errormsg += it;
            errormsg += ",";
        }
        errormsg += ".";
        isSuccess = false;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s check %s\n", FUNCTION_NAME, (isSuccess)? "SUCCEEDED" : "FAILED");
    return isSuccess;
}


/*
* FUNCTION NAME     :  GetDriveGeometry
*
* DESCRIPTION       :  Gets information about the physical disk's geometry using
*                      IOCTL_DISK_GET_DRIVE_GEOMETRY ioctl and fills DISK_GEOMETRY
*
* INPUT PARAMETERS  :  diskhandle, diskname
*
* OUTPUT PARAMETERS :  DISK_GEOMETRY
*
* return value      :  NONE
*
*/
BOOL GetDriveGeometry(const HANDLE &hDisk, const std::string & diskname, DISK_GEOMETRY & geometry, std::string& errormessage)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED : %s \n", FUNCTION_NAME);

    DWORD               bytesReturned;
    BOOL                bResult;
    std::stringstream   sserror;

    assert(INVALID_HANDLE_VALUE != hDisk);

    bResult = DeviceIoControl(hDisk,
        IOCTL_DISK_GET_DRIVE_GEOMETRY,
        NULL,
        0,
        &geometry,
        sizeof(geometry),
        &bytesReturned,
        NULL);

    if (bResult)
    {
        DebugPrintf(SV_LOG_DEBUG, "%s : Disk Geometry bps: %lu cyl: %llu spt: %lu tpc: %lu \n",
                                                                    diskname.c_str(),
                                                                    geometry.BytesPerSector,
                                                                    geometry.Cylinders.QuadPart,
                                                                    geometry.SectorsPerTrack,
                                                                    geometry.TracksPerCylinder);
    }
    else
    {
        sserror << FUNCTION_NAME << " : IOCTL_DISK_GET_DRIVE_GEOMETRY failed for disk " << diskname << " error: " << GetLastError();
        errormessage = sserror.str();
        DebugPrintf(SV_LOG_ERROR, errormessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED : %s \n", FUNCTION_NAME);
    return bResult;
}

BOOL GetDeviceNumber(const HANDLE &hDisk, const std::string & diskname, dev_t& diskIndex, std::string& errormessage)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED : %s \n", FUNCTION_NAME);
    assert(INVALID_HANDLE_VALUE != hDisk);

    DWORD                   bytesReturned;
    BOOL                    bResult;
    STORAGE_DEVICE_NUMBER   deviceNumber = { 0 };
    std::stringstream       ssError;

    bResult = DeviceIoControl(
        hDisk,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL,
        0,
        &deviceNumber,
        sizeof(deviceNumber),
        &bytesReturned,
        NULL
        );

    if (bResult) {
        diskIndex = deviceNumber.DeviceNumber;
        DebugPrintf(SV_LOG_DEBUG, "%s : %s Device Number: %d\n", FUNCTION_NAME, diskname.c_str(), diskIndex);
    }
    else {
        ssError << FUNCTION_NAME << " : IOCTL_STORAGE_GET_DEVICE_NUMBER failed for disk " << diskname << " error: " << GetLastError();
        errormessage = ssError.str();
        DebugPrintf(SV_LOG_ERROR, errormessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED : %s \n", FUNCTION_NAME);
    return bResult;
}

/*
* FUNCTION NAME      : GetSCSIAddress
*
* DESCRIPTION        : Fills SCSI_ADDRESS by sending IOCTL_SCSI_GET_ADDRESS ioctl to the given disk handle
*
* INPUT PARAMETERS   : diskhandle, diskname, errormessage
*
* OUTPUT PARAMETERS  : SCSI_ADDRESS
*
* return value       : NONE
*
*/
BOOL GetSCSIAddress(const HANDLE &hDisk, const std::string & diskname, SCSI_ADDRESS & scsiAddress, std::string& errormessage)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED : %s \n", FUNCTION_NAME);

    DWORD                   bytesReturned;
    BOOL                    bResult;
    std::stringstream       sserror;

    assert(INVALID_HANDLE_VALUE != hDisk);

    bResult = DeviceIoControl(
        hDisk,
        IOCTL_SCSI_GET_ADDRESS,
        NULL,
        0,
        &scsiAddress,
        sizeof(scsiAddress),
        &bytesReturned,
        NULL
        );

    if (bResult) {
        DebugPrintf(SV_LOG_DEBUG, "%s : %s scsi PortNumber: %u TargetId: %u PathId: %u Lun: %u\n",
            FUNCTION_NAME,
            diskname.c_str(),
            scsiAddress.PortNumber,
            scsiAddress.TargetId,
            scsiAddress.PathId,
            scsiAddress.Lun);
    }
    else {
        sserror << FUNCTION_NAME << ": IOCTL_SCSI_GET_ADDRESS failed for disk " << diskname << " error: " << GetLastError();
        errormessage = sserror.str();
        DebugPrintf(SV_LOG_ERROR, "%s\n", errormessage.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED : %s \n", FUNCTION_NAME);
    return bResult;
}


bool
RebootMachine()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    HANDLE              hToken;
    TOKEN_PRIVILEGES    tkp;

    // Get a token for this process. 

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        DebugPrintf(SV_LOG_ERROR, "GetCurrentProcess failed with error=%d\n", GetLastError());
        return false;
    }

    // Get the LUID for the shutdown privilege. 

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process. 

    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0);

    if (GetLastError() != ERROR_SUCCESS) {
        DebugPrintf(SV_LOG_ERROR, "AdjustTokenPrivileges failed with error=%d\n", GetLastError());
        return false;
    }

    if (!InitiateSystemShutdownExA(
        NULL,
        "Azure Site Recovery: Recovering Failover VM",
        0,
        TRUE,
        TRUE,
        SHTDN_REASON_MINOR_RECONFIG)) {
        DebugPrintf(SV_LOG_ERROR, "InitiateSystemShutdownExA failed with error=%d\n", GetLastError());
        return false;
    }

    //shutdown was successful
    DebugPrintf(SV_LOG_DEBUG, "Shutdown is successful... Exiting %s\n", FUNCTION_NAME);

    return true;
}

