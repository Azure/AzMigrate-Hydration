#include "portablehelpers.h"
#include "portablehelperscommonheaders.h"
#include "portablehelpersmajor.h"
#include "portablehelpersminor.h"
#include <libgen.h>
#include <sys/ioctl.h>   /* for _IO */
#include <sys/statvfs.h> 
#ifndef SV_AIX
#include <sys/mount.h> 
#endif
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include "cdplock.h"
#include "cdputil.h"
#include <sstream>
#include <signal.h>

#include <ace/Guard_T.h>
#include "ace/Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include "volumereporter.h"
#include "volumereporterimp.h"
#include "configwrapper.h"
#include "inmsafeint.h"
#include "inmageex.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

ACE_Recursive_Thread_Mutex g_fstabLock;

ACE_Recursive_Thread_Mutex g_executecmdLock;

std::string ParseGateways(std::stringstream &results, const int &destpos, 
                          const int &gwpos, const int &flagspos, const char &delim);

bool ParseSparseFileNameAndVolPackName(const std::string lineinconffile, std::string &sparsefilename, std::string &volpackname)
{
    bool bretval = false;
    const std::string DELIM_STR = "--/dev/volpack";
    const std::string ACTUAL_DELIM_STR = "--";
    size_t found = 0;
    if(IsVolpackDriverAvailable())
        found = lineinconffile.find(DELIM_STR);
    else
        found = lineinconffile.find(ACTUAL_DELIM_STR);

    if (std::string::npos != found)
    {
        sparsefilename = lineinconffile.substr(0, found);
        size_t volindex = found + ACTUAL_DELIM_STR.length();
        volpackname = lineinconffile.substr(volindex);
        bretval = true;
    }

    return bretval;
}


/* UNIX */

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
        DebugPrintf( SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_DEBUG, "ENTERED OpenVolumeExtended()...\n" );
        DebugPrintf( SV_LOG_DEBUG, "OpenVolumeExtended::Volume is %s.\n",pch_VolumeName );

        if( ( NULL == pHandleVolume ) ||
            ( NULL == pch_VolumeName ) )
        {
            sve = EINVAL;
            DebugPrintf( SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED OpenVolumeExtended()... sve = EINVAL\n");
            break;
        }

        if (ACE_BIT_ENABLED (openMode, O_WRONLY))
        {
            // Need to use configurator API for checking system volume
        }

        std::string DestVolName = pch_VolumeName;

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING CreateFile()...\n" );

        ACE_OS::last_error(0);
        // PR#10815: Long Path support
        *pHandleVolume = ACE_OS::open(getLongPathName(DestVolName.c_str()).c_str(), openMode, ACE_DEFAULT_OPEN_PERMS); 

        std::ostringstream errmsg;
        std::string errstr;
        if( ACE_INVALID_HANDLE == *pHandleVolume )
        {
            sve = EINVAL;
            errstr = ACE_OS::strerror(ACE_OS::last_error());
            errmsg.clear();
            errmsg << DestVolName << " - " << errstr << std::endl;
            DebugPrintf(SV_LOG_DEBUG, "%s\n", errmsg.str().c_str());
            break;
        }

    }
    while( FALSE );

    return( sve );
}


/* UNIX */

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
    char szDestVolume[ 256 ];
    char szGuidDestVolume[ 256 ] = {0};
    char *ptrVol = NULL;
    SVERROR sve = SVS_OK;
    int flags = FILE_ATTRIBUTE_NORMAL; // FILE_ATTRIBUTE_NORMAL is required for CreateFile calls
    do
    {

        ptrVol = (char *)pch_VolumeName;

        // PR#10815: Long Path support
        *pHandleVolume = ACE_OS::open(getLongPathName(ptrVol).c_str(), O_RDWR | flags, FILE_SHARE_READ);

        if( ACE_INVALID_HANDLE == *pHandleVolume )
        {
            sve = ACE_OS::last_error();

            DebugPrintf( SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            if( bUnmount ) {
                DebugPrintf( SV_LOG_ERROR, "OpenVolumeEx() did not succeed, Volume: %s; VolGuid: %s err = %s\n", pch_VolumeName, outs_VolGuidName.c_str(), sve.toString() );
            }
            else {
                DebugPrintf( SV_LOG_ERROR, "OpenVolumeEx() did not succeed, Volume: %s err= %s\n", pch_VolumeName, sve.toString());
            }

            break;
        }
    } while ( FALSE );

    return sve;
}


/* UNIX */

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
        DebugPrintf(SV_LOG_DEBUG,"@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf(SV_LOG_DEBUG, "ENTERED OpenVolumeEx()...\n" );

        if( ( NULL == pHandleVolume  ) ||
            ( NULL == pch_VolumeName )
            )
        {
            sve = EINVAL;
            DebugPrintf( SV_LOG_ERROR, "Invalid argument passed to OpenVolumeExClusive\n");
            return sve;
        }

        DebugPrintf(SV_LOG_DEBUG, "Attempting Volume Open on %s...\n", pch_VolumeName);

        sve = OpenExclusive(pHandleVolume, outs_VolGuidName, pch_VolumeName, mountPoint, bUnmount, bUnbufferIO);
        if( sve.failed() )
        {
            // FIXME: In Win 2k, after a issuing FSCTL_DISMOUNT_VOLUME ioctl, the first call to CreateFile() to get 
            // exclusive access always fails eventhough there are not any open handles , but the second call succeeds.
            // As it seemed like a windows 2k bug, we are doing a retry for one more time to get exclusive access. When
            // actual reason for CreateFile() failure is known, this code can be removed by making necessary changes.
            // For more info on this Bug, refer to Bug id #635 in bugzilla.
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf(SV_LOG_INFO, "First attempt to get exclusive access did not succeed, retrying ...\n");
            sve = OpenExclusive(pHandleVolume, outs_VolGuidName, pch_VolumeName, mountPoint, bUnmount, bUnbufferIO);
        }
    } while ( FALSE );

    if(sve.failed())
    {
        DebugPrintf( SV_LOG_ERROR, "Error while trying to obtain exclusive access on volume : %s\n", pch_VolumeName);
    }

    return( sve );
}


/* UNIX */

/// Closes a volume that we opened for exclusive access using OpenVolumeExclusive.
/// See also CloseVolume
SVERROR CloseVolumeExclusive( 
                             /* in */ ACE_HANDLE handleVolume ,
                             /* in */ const char *pch_VolGuidName,// Volume in GUID format
                             /* in */ const char *pch_VolumeName,  // Drive letter volume was prev mounted on
                             /* in */ const char *pszMountPoint     // Optional: pathname to directory where volume mounted
                             )
{
    SVERROR sve = SVS_OK;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED CloseVolumeEx()...\n" );

        if( ( NULL == pch_VolumeName ) || 
            ( NULL == pch_VolGuidName )
            )
        {
            sve = EINVAL;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED CloseVolumeEx()... err = EINVAL\n");
            break;
        }

        DebugPrintf( "Attempting Volume Close on %s...\n", pch_VolumeName);

        if( ( ACE_INVALID_HANDLE == handleVolume ) ) 
        {
            sve = ACE_OS::last_error();
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED CloseVolume()... err = %s\n", sve.toString() );
            DebugPrintf( SV_LOG_ERROR, "handleVolume == ACE_INVALID_HANDLE... \n");
            break;
        }

        if( ( ACE_INVALID_HANDLE != handleVolume ) &&
            ACE_OS::close( handleVolume ) )
        {
            sve = ACE_OS::last_error();
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED CloseVolumeExclusive()... err = %s\n", sve.toString() );
        }

    }
    while( FALSE );

    return( sve );
}


/* UNIX */

bool GetDriverVolumeSize(char *vol, SV_ULONGLONG* size)
{
    (*size) = 0;
    bool ok = false;    

    // On unix, driver volume size and file system volume size are equal
    unsigned int sectorSize;
    ok = GetFileSystemVolumeSize(vol, size, &sectorSize);

    // OK now see if we should use the driver of file system size
    return ok;
}


/* UNIX */

bool CanReadDriverSize(char *vol, SV_ULONGLONG offset, unsigned int sectorSize)
{
    bool canRead = false;

    if (NULL != vol)
    {
        //On unix we always be able to read driver size
        canRead = true;
    }

    return canRead;
}


/* UNIX */

// check if volume locked by us. 
// as a convenience if the fsType buffer is supplied also return what type of file system it is
bool IsVolumeLocked(const char *pszVolume, int fsTypeSize, char * fsType)
{
    std::string sMount= "";
    std::string mode;
    return (!IsVolumeMounted(pszVolume,sMount,mode));
}


/* UNIX */

SVERROR UnhideDrive_RO(const char * drive, char const* mountPoint,const char* fs )
{
    SVERROR sve = SVS_OK;

    (void)FlushFileSystemBuffers(drive);
    sve = SVS_OK;


    if((strcmp(mountPoint,"") == 0) )
    {
        DebugPrintf(SV_LOG_ERROR, "empty mountpoint.\n");
        return SVE_FAIL;
    }

    if(SVMakeSureDirectoryPathExists(mountPoint).failed())
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to create mount point directory %s.\n", mountPoint);
        sve = SVE_FAIL;
        return sve;
    }

    /** 
    * MountVolume has separate implementations in
    * linux and sun 
    */

    if ( !MountVolume(drive,mountPoint,fs,true) )
    {
        sve=SVE_FAIL;
        DebugPrintf(SV_LOG_ERROR,"Mount operation failed. Unable to Unhide volume in Read-Only mode.Ensure system mounts are not specified.\n");
    }

    return sve;
}


/* UNIX */

SVERROR UnhideDrive_RW(const char * drive, char const* mountPoint,const char* fs )
{
    SVERROR sve = SVS_OK;

    (void)FlushFileSystemBuffers(drive);
    sve=SVS_OK;


    if((strcmp(mountPoint,"") == 0) )
    {
        DebugPrintf(SV_LOG_ERROR, "empty mountpoint.\n");
        return SVE_FAIL;
    }
    if(SVMakeSureDirectoryPathExists(mountPoint).failed())
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to create mount point directory %s.\n", mountPoint);
        return SVE_FAIL;
    }

    /** 
    * MountVolume has separate implementations in
    * linux and sun 
    */
    if ( !MountVolume(drive,mountPoint,fs,false) )
    {
        sve = SVE_FAIL;
        DebugPrintf(SV_LOG_ERROR,"Mount operation failed. Unable to Unhide volume in Read-Write mode.Ensure system mounts are not specified.\n");
    }

    return sve;
}


/* UNIX */

bool executeAceProcess(std::string & fsmount,std::string  device)
{
    ACE_Process_Manager* pm = ACE_Process_Manager::instance();
    ACE_exitcode status = 0;
    pid_t pid = 0;
    pid_t rc = 0;
    ACE_Process_Options options;
    std::ostringstream args;
    args << fsmount.c_str() ;
    int fd = open("/dev/null", O_RDWR);

    options.command_line(ACE_TEXT("%s"), args.str().c_str());
    options.set_handles(fd,fd,fd);
    pid = pm->spawn(options);

    if (ACE_INVALID_PID == pid)
    {
        DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :failed to unhide the device %s\n",
            FUNCTION_NAME, LINE_NO, FILE_NAME,device.c_str());
        options.release_handles();
        return false;
    }

    rc = pm ->wait(pid,&status);
    options.release_handles();

    if (-1 != fd)
    {
        close(fd);
    }

    if ((ACE_INVALID_PID == rc) || (0 != status ))
    {
        DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s \n:%s failed with status %d\n",
            FUNCTION_NAME, LINE_NO, FILE_NAME,fsmount.c_str(), status);
        return false;
    }
    return true;
}


/* UNIX */

void ExtractGuidFromVolumeName(std::string& sVolumeNameAsGuid)
{
    //Place Holder
}


/* UNIX */

SVERROR SetReadOnly(const char * drive)
{
    SVERROR sve = SVS_OK;
    std::string sVolName = drive;
    const char * pszVolume = sVolName.c_str();

    char DriveLetter = pszVolume[0];
    char    VolumeGUID[GUID_SIZE_IN_CHARS + 1];
    bool isVirtualVolume=IsSparseVolume(drive); 
    sve = SetReadOnly(drive, VolumeGUID,isVirtualVolume);
    if ( sve.failed() )
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf(SV_LOG_ERROR, "FAILED to set READ ONLY flag in HideDrive.... err = %s\n", sve.toString());
    }
    return sve;
}


/* UNIX */

SVERROR ResetReadOnly(const char * drive)
{
    SVERROR sve = SVS_OK;
    std::string sVolName = drive;
    const char * pszVolume = sVolName.c_str();

    char DriveLetter = pszVolume[0];
    char    VolumeGUID[GUID_SIZE_IN_CHARS + 1];

    bool isVirtualVolume=IsSparseVolume(drive); 
    sve = ResetReadOnly(drive, VolumeGUID,isVirtualVolume);
    if (sve.failed() )
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf(SV_LOG_ERROR, "FAILED to reset READ ONLY flag.... err = %s\n", sve.toString());
    }

    return sve;
}


/* UNIX */

bool IsValidDevfile(std::string devname)
{
    bool breturn = true;
    struct stat statb;

    memset(&statb, 0, sizeof(statb));


    if ((stat(devname.c_str(), &statb) == -1) || (!(S_ISBLK(statb.st_mode) || S_ISCHR(statb.st_mode))))
    {
        try //if drscout.conf is not present it will throw an exception 
        {
            std::string sparsefile;
            bool new_sparsefile_format = false;
            bool is_volpack = IsVolPackDevice(devname.c_str(),sparsefile,new_sparsefile_format);
            if(!IsVolpackDriverAvailable() && is_volpack)
            {
            	if(new_sparsefile_format)
            		breturn =  true;
                else if (stat(sparsefile.c_str(),&statb) != -1) 
                    breturn =  true;
                else
                    breturn =  false;
            }
            else // not a volpack device
            {
                breturn = false;
            }
        }
        catch(...)
        {
            breturn = false;
        }
    }     
    return breturn;
}


/* UNIX */

std::string GetVolumeDirName(std::string volName)
{
    std::string dirname;

    //Get basename of volume (i.e sda1 for /dev/sda1)
    //dirname = ACE::basename (volName.c_str(), ACE_DIRECTORY_SEPARATOR_CHAR);

    //DO NOTHING for now as CX creates resync files under /home/svsystems/dev/sda1/
    dirname = volName;

    return dirname;
}


/* UNIX */

void FormatVolumeName(std::string& sName,bool bUnicodeFormat)
{
    //Place Holder 
}


/* UNIX */

bool IsDrive(const std::string& sDrive)
{
    return false;
}


/* UNIX */

bool IsMountPoint(const std::string& sVolume)
{
    return false;
}


/* UNIX */

bool FormatVolumeNameToGuid(std::string& sVolumeName)
{
    std::string sName = sVolumeName;
    std::string sparsefile;
    bool new_sparsefile_format = false;
	bool rv = true;
    struct stat statb;
    memset(&statb, 0, sizeof(statb));
	if ((stat(sVolumeName.c_str(), &statb) == -1) || (!(S_ISBLK(statb.st_mode) || S_ISCHR(statb.st_mode))))
	{
		try
		{
			if(!IsVolpackDriverAvailable() && (IsVolPackDevice(sName.c_str(),sparsefile,new_sparsefile_format)))
    {
        sVolumeName =  sparsefile;
    }
		}
		catch(...)
		{
			rv = false;
		}
	}
    return rv;
}


/* UNIX */

void UnformatVolumeNameToDriveLetter(std::string& sVolName)
{
}


/* UNIX */

void ToStandardFileName(std::string& sFileName)
{
    //Place Holder
}


/* UNIX */

void FormatVolumeNameForMountPoint(std::string& volumeName)
{  
    //Place Holder
}


// For Sparse file mounted virtual volumes VSNAP_UNIQUE_ID_PREFIX is not used. 
// So, separate function is written.
bool IsSparseVolume(std::string Vol) 
{
    ACE_HANDLE	VVCtrlDevice = ACE_INVALID_HANDLE;
    wchar_t	VolumeLink[256];
    int		bResult;
    SV_ULONG   BytesReturned = 0;
    std::string	UniqueId;


    char*		Buffer = NULL;
    SV_ULONG 	ulLength = 0;
    size_t		UniqueIdLength;
    UniqueId.clear();
    int hDevice = 0;
    char *vsnap_dev = "/dev/vsnapcontrol";

    hDevice = open(vsnap_dev, O_RDONLY, 0); 

    VVCtrlDevice =  (ACE_HANDLE)hDevice;

    if(ACE_INVALID_HANDLE == VVCtrlDevice)
        return false;

#define VERIFY_DEVICE_ENTRY 4
    /** 
    * TODO: TEMPORARY FIX
    * Kush, contact me when you see this! :-)
    **/
    //UniqueId = VSNAP_UNIQUE_ID_PREFIX;
    //UniqueId += Vol;
    UniqueId = Vol;
    UniqueIdLength = UniqueId.size();

    int ByteOffset  = 0;
    INM_SAFE_ARITHMETIC(ulLength    = (SV_ULONG) sizeof(SV_USHORT) + (InmSafeInt<size_t>::Type(UniqueIdLength) + 1), INMAGE_EX(sizeof(SV_USHORT))(UniqueIdLength))
    Buffer      = (char*) calloc(ulLength , 1);
    *(SV_USHORT*)Buffer = (SV_USHORT)(UniqueIdLength + 1);
    ByteOffset += sizeof(SV_USHORT);
	inm_memcpy_s(Buffer + ByteOffset, (ulLength - ByteOffset), UniqueId.c_str(), (UniqueIdLength + 1));

    if (ioctl(VVCtrlDevice , VERIFY_DEVICE_ENTRY, Buffer) < 0) 
    {
        close(VVCtrlDevice);
        return false;
    }

    free(Buffer);
    ACE_OS::close(VVCtrlDevice);
    return true;
}


/* UNIX */
/** 
* TODO: Need to understand in linux and sun 
*/ 

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

    /*
    do 
    {
    VolumeSummaries_t volumeSummaries;
    VolumeInfoCollector volumeCollector;
    volumeCollector.GetVolumeInfos(volumeSummaries, false); 

    VolumeSummaries_t::const_iterator volumeIterator(volumeSummaries.begin());
    VolumeSummaries_t::const_iterator volumeIteratorEnd(volumeSummaries.end());
    for( ; volumeIterator != volumeIteratorEnd; volumeIterator++)
    {
    std::string leafdevname = (*volumeIterator).deviceName;
    bool bIsStartingAtBlockZero = (*volumeIterator).isStartingAtBlockZero; 
    if((devname == leafdevname) && (!bIsStartingAtBlockZero))
    {
    rv = true;
    break;
    }

    }

    if(volumeIterator == volumeIteratorEnd)
    rv = false;

    } while (0);
    */

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return rv;

}


/* UNIX */

SVERROR MakeVirtualVolumeReadOnly(const char *mountpoint, void * volumeGuid, etBitOperation ReadOnlyBitFlag)
{
    SVERROR sve = SVS_OK;
    return sve ;
}


/* UNIX */

dev_t GetDevNumber(std::string devname)
{
    struct stat statb;

    if(IsValidDevfile(devname) == false)
        return 0;

    stat(devname.c_str(), &statb);
    return statb.st_rdev;
}


/* UNIX */

void FirstCharToUpperForWindows(std::string & volume)
{
    //Place Holder
}


/* UNIX */

/*
* FUNCTION NAME :     containsMountedVolumes
*
* DESCRIPTION :   Detemines whether a device or mountpoint contains any nested volumes
*                  
*
* INPUT PARAMETERS :  path under which the nested volumes needs to be checked
*
* OUTPUT PARAMETERS : list of volumes mounted beneath the path
*
* NOTES : Algorithm:
* 
* Input can be a devicename e.g /dev/sdb1 or mountpoint e.g /home/1
*
* Case "devicename": 1. Obtain the list of mountpoints where the device is mounted.				
*					  2. For each mountpoint, obtain the list of nested volumes	
*					  3. Return the total accumulated nested volumes
*	
*
* Case "mountpoint": If the mountpoint corresponds to a device, 
*					  1. Find the nested volumes beneath the mountpoint
*					  2. Return the accumulated nested volumes(mountpoint + nested volumes)
*
* return value : true if nested volumes are present. otherwise false
*
*/
bool containsMountedVolumes( const std::string& path, std::string& volumes )
{
    std::vector<std::string> mountpoints;
    std::string path1 = path;
    char* strpath = (char *)path1.c_str();
    TruncateTrailingSlash(strpath);
    path1 = strpath;

    GetDeviceNameFromSymLink( path1 );
    if( IsValidDevfile( path1 ) )
    {
        /* This handles the GetDeviceNameFromSymLink function call by itself
        * since it compares path1 with actual name as well as link name */
        GetMountPoints( path1.c_str(), mountpoints );
        for(std::vector<std::string>::iterator start = mountpoints.begin(); start != mountpoints.end(); ++start)
        {
            std::string mounts;
            mountedvolume( (*start), mounts );
            volumes += mounts;
        }
    }
    else
    {
        if( IsDirectoryExisting(path1) )
        {
            std::string deviceName;
            if(GetdevicefilefromMountpoint( path1.c_str(), deviceName ) && !deviceName.empty())
            {
                volumes += path1;
            }
            path1 += "/";
            mountedvolume( path1, volumes );
        }
    }

    if( !volumes.empty() )
    {
        return true;
    }
    return false;
}

/** 
* Function that fills in the system volume 
* , its size and free capacity 
* This is used from function RegisterHost
*/

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



    sysVol = sysDir = "/";
    struct statvfs64 sysVolBuf;
    if ( statvfs64(sysVol.c_str(),&sysVolBuf) < 0 )
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, statvfs failed on volume: %s\n", LINE_NO, 
            FILE_NAME, sysVol.c_str());
        bretval = false;
    }
    else
    {
        sysVolCapacity = GetTotalFileSystemSpace(sysVolBuf);
        sysVolFreeSpace= GetFreeFileSystemSpace(sysVolBuf);
    }

    return bretval;
}


/** 
* Function that fills in the install volume 
* , its size and free capacity 
* This is used from function RegisterHost
*/

bool GetInstallVolCapacityAndFreeSpace(const std::string &installPath,
                                       unsigned long long &insVolFreeSpace,
                                       unsigned long long &insVolCapacity)
{
    bool bretval = true;



    struct statvfs64 insVolBuf;
    if ( statvfs64((char*)installPath.c_str(),&insVolBuf) < 0)
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, statvfs failed on volume: %s\n", LINE_NO, 
            FILE_NAME, installPath.c_str());
        bretval = false;
    }
    else
    {
        insVolCapacity = GetTotalFileSystemSpace(insVolBuf);
        insVolFreeSpace= GetFreeFileSystemSpace(insVolBuf);
    }

    return bretval;
}


/**
*
* This function has been made common to linux
* and sun by using standart c++ ifstream,
* std::getline for reading a line from 
* SV_FSTAB
* 
*/ 

bool DeleteFSEntry(const char * dev)
{
    bool rv = false;
    std::vector<std::string> fsents;
    ACE_HANDLE handle = ACE_INVALID_HANDLE ;

    // Added to fix runtime fstab updation race condition
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard( g_fstabLock);

    do 
    {
        CDPLock lock(SV_FSTAB,10);
        if(!lock.acquire())
            return false;

        std::string device = dev;
        std::string label;

        /* This handles the GetDeviceNameFromSymLink function call by itself
        * since it compares device with actual name as well as link name */
        if(!GetDeviceNameFromSymLink(device))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to get actual device name for %s\n"
                ,FUNCTION_NAME, LINE_NO, FILE_NAME, dev);
            rv = false;
            break;
        }

        std::ifstream ifstab(SV_FSTAB, std::ifstream :: in);
        if(!ifstab.is_open())
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_FSTAB);
            rv = false;
            break;
        }

        std::string line;
        size_t len = 0;

        while(std::getline(ifstab, line))
        {
            /** 
            * Need to manually add a '\n' character at the end
            * since std::getline does not add it
            */
            line += '\n';
            fsents.push_back(line.c_str());
        }


        ifstab.close();

        /* 
        * This function performs the functionality
        * of "e2label" and fills label on linux.
        * On solaris, it is just a stub
        */
        if (!FormVolumeLabel(device, label))
        {
            DebugPrintf(SV_LOG_ERROR,
                "In Function %s @LINE %d in FILE %s :FormVolumeLabel failed \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME);
            rv = false;
            break;
        }

        std::string fstabCopy = SV_FSTAB;
        fstabCopy += ".copy";
        fstabCopy += boost::lexical_cast<std::string>(ACE_OS::getpid());

        // PR#10815: Long Path support
        handle =	ACE_OS::open( getLongPathName(fstabCopy.c_str()).c_str(),O_WRONLY | O_CREAT | O_EXCL);
        if(ACE_INVALID_HANDLE == handle)
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str());
            rv = false;
            break;
        }

        if(!removeFSEntry(device, label, fsents))
        {
            rv = false;
            break;
        }
        bool bWriteSuccess = true;
        std::vector<std::string>::iterator fsent_iter = fsents.begin();
        std::vector<std::string>::iterator fsent_end = fsents.end();
        for(; fsent_iter != fsent_end; ++fsent_iter)
        {	
            std::string fsent = *fsent_iter;
            if(ACE_OS::write(handle,fsent.c_str(), strlen(fsent.c_str())) != strlen(fsent.c_str()))
            {
                DebugPrintf(SV_LOG_ERROR,
                    "Function %s @LINE %d in FILE %s :failed to update file %s Error Code %d \n",
                    FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str(), ACE_OS::last_error());
                rv = false;
                bWriteSuccess = false;
                break;
            }
        }

        ACE_OS::close(handle);
        handle = ACE_INVALID_HANDLE;
        if(bWriteSuccess == false)
        {
            break;
        }
        if( -1 == rename(fstabCopy.c_str(), SV_FSTAB))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to rename %s as %s. errno=%d\n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str(), SV_FSTAB, errno);
            rv = false;
            break;
        }

        rv = true;
    } while (0);

    if(ACE_INVALID_HANDLE != handle)
        ACE_OS::close(handle);

    return rv;
}

bool CanUnmountMountPointsIncludingMultipath(const char * dev, std::vector<std::string> &mountPoints,std::string & errormsg)
{
    bool rv = true;
    VolumeSummaries_t volumeSummaries;
    VolumeDynamicInfos_t volumeDynamicInfos;
    VolumeInfoCollector volumeCollector;
    volumeCollector.GetVolumeInfos(volumeSummaries, volumeDynamicInfos, false);
    boost::shared_ptr<VolumeReporter> volinfo(new VolumeReporterImp());
    VolumeReporter::VolumeReport_t volume_write_info;
    volinfo->GetVolumeReport(dev,volumeSummaries,volume_write_info);
    volinfo->PrintVolumeReport(volume_write_info);
    std::stringstream out;
    std::set<std::string> devListToUnmount;
    VolumeReporter::EntityWriteStatesIter_t entityIter = volume_write_info.m_EntityWriteStates.begin();
    VolumeReporter::EntityWriteStatesIter_t entityEndIter = volume_write_info.m_EntityWriteStates.end();
    for(;entityIter != entityEndIter; ++entityIter)
    {
        VolumeReporter::WriteStates_t::iterator writeStateIter = entityIter->second.begin();
        VolumeReporter::WriteStates_t::iterator writeStateEndIter = entityIter->second.end();
        for(;writeStateIter != writeStateEndIter;++writeStateIter)
        {
            if(((*writeStateIter) != VolumeReporter::Mounted) && 
                ((*writeStateIter) != VolumeReporter::Locked) && 
                ((*writeStateIter) != VolumeReporter::Writable))
            {
                out << volinfo->GetEntityTypeStr(entityIter->first.m_Type) << " " << entityIter->first.m_Name;
                if(entityIter->first.m_Type != VolumeReporter::Entity_t::EntityVolume)
                    out << " of device " << dev;
                out << " " << volinfo->GetWriteStateStr((*writeStateIter)) << "\n";
                errormsg = out.str();
                return false;
            }
            if(((entityIter->first.m_Type == VolumeReporter::Entity_t::EntityChild) ||
                (entityIter->first.m_Type == VolumeReporter::Entity_t::EntityMultipath)) && ((*writeStateIter) == VolumeReporter::Mounted))
            {
                devListToUnmount.insert(entityIter->first.m_Name);
            }
        }
    }

    if(IsInstallPathVolume(dev))
    {
        std::ostringstream ostr;
        ostr << "Cannot Hide " << dev << " as it is agent installation volume." << std::endl;
        errormsg = ostr.str().c_str();
        return false;
    }

    std::string nestedmountpoints;
    if(containsMountedVolumes(dev,nestedmountpoints))
    {		
        std::ostringstream ostr;
        ostr << "Cannot Hide " << dev << " as the drive contains mount points.\n";
        errormsg = ostr.str().c_str();
        return false;
    }

    if(containsVolpackFiles(dev))
    {
        std::ostringstream ostr;
        ostr << "Cannot Hide " << dev << " as the volume contains virtual volume data.\n";
        errormsg = ostr.str().c_str();
        return false;
    }

    std::set<std::string>::iterator volumeIterator(devListToUnmount.begin());
    std::set<std::string>::iterator volumeIteratorEnd(devListToUnmount.end());
    for(; volumeIterator != volumeIteratorEnd; volumeIterator++)
    {
        if(IsInstallPathVolume(*volumeIterator))
        {
            std::ostringstream ostr;
            ostr << "Cannot Hide " << (*volumeIterator) << " as it is agent installation volume." << std::endl;
            errormsg = ostr.str().c_str();
            rv = false;
            break;
        }

        std::string nestedmountpoints;
        if(containsMountedVolumes(*volumeIterator,nestedmountpoints))
        {
            std::ostringstream ostr;
            ostr << "Cannot Hide " << (*volumeIterator) << " as the drive contains mount points.\n";
            errormsg = ostr.str().c_str();
            rv = false;
            break;
        }

        if(containsVolpackFiles(*volumeIterator))
        {
            std::ostringstream ostr;
            ostr << "Cannot Hide " << (*volumeIterator) << " as the volume contains virtual volume data.\n";
            errormsg = ostr.str().c_str();
            rv = false;
            break;
        }
        std::string devicename=(*volumeIterator);
        if(!GetDeviceNameFromSymLink(devicename))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed GetDeviceNameFromSymLink for %s \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME, devicename.c_str());
            errormsg = "CanUnmountMountPointsIncludingMultipath(): Unable to obtain DeviceName from SymLink ";
            errormsg += devicename;
            rv = false;
            break;
        }
        if(!GetMountPoints(devicename.c_str(),mountPoints))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed GetMountPoints for %s \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME, dev);
            errormsg = "Unable to retrieve mountpoints associated with ";
            errormsg += (*volumeIterator);
            rv = false;
            break;
        }
    }
    return rv;
}


SVERROR HideDrive(const char * drive, char const* mountPoint, std::string& output, std::string& error, bool checkformultipaths )
{
    SVERROR sve = SVS_OK;
    if( NULL == drive )
    {
        sve = EINVAL;
        DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_ERROR, "FAILED HideDrive()... err = EINVAL\n");
        error = "HideDrive(): The drive name is empty";
        return sve;
    }


    /**************************************************
    * This function has been made common to linux and *
    * and solaris, by means of following:             *
    * 1. The command names have been defined as macros*
    *    in implementation specific header files of   *
    *    sun and linux.                               *
    * 2. Added a function "GetCommandPath" that       *
    *    actually returns the specific path where     *
    *    the commands like fuser, umount are there    *
    **************************************************/ 

    /** 
    *
    * TODO: For solaris, since lazy unmount of 
    * device is not supported, but since there 
    * is support for forcible umount with -f option,
    * use of fuser can be eliminated.
    *
    */

    std::vector<std::string> mountPoints;

    std::string devicename=drive;
    if(!GetDeviceNameFromSymLink(devicename))
    {
        DebugPrintf(SV_LOG_ERROR, 
            "Function %s @LINE %d in FILE %s :failed GetDeviceNameFromSymLink for %s \n", 
            FUNCTION_NAME, LINE_NO, FILE_NAME, drive);
        error = "HideDrive(): Unable to obtain DeviceName from SymLink ";
        error += devicename;
        return SVE_FAIL;
    }

    const char * dev = devicename.c_str();
    /* This handles the GetDeviceNameFromSymLink function call by itself
    * since it compares device with actual name as well as link name */
    if(!GetMountPoints(dev,mountPoints))
    {
        DebugPrintf(SV_LOG_ERROR, 
            "Function %s @LINE %d in FILE %s :failed GetMountPoints for %s \n", 
            FUNCTION_NAME, LINE_NO, FILE_NAME, drive);
        error = "HideDrive(): Unable to retrieve mountpoints associated with ";
        error += dev;
        return SVE_FAIL;
    }
    /*
    * In case of vsnaps,we should not call CanUnmountMountPointsIncludingMultipath'
    */
    if(checkformultipaths)
    {
        if(!CanUnmountMountPointsIncludingMultipath(drive,mountPoints,error))
        {
            DebugPrintf(SV_LOG_DEBUG,
                "Function %s @LINE %d in FILE %s :failed CanUnmountMountPointsIncludingMultipath for %s \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME, drive);
            return SVE_FAIL;
        }
    }

    if(mountPoints.empty())
    {
        DebugPrintf(SV_LOG_INFO, "Mountpoints corresponding to %s are already removed.\n", drive);
        DeleteFSEntry(drive);
        return SVS_OK;
    }

	LocalConfigurator localConfigurator;
	SV_UINT max_unmount_reties = localConfigurator.getMaxUnmountRetries();
    std::vector<std::string>::iterator mnt_iter = mountPoints.begin();
    std::vector<std::string>::iterator mnt_end = mountPoints.end();
    for ( ; mnt_iter != mnt_end ; ++mnt_iter)
    {
		std::string errmsg;
		if(!IsValidMountPoint((*mnt_iter),errmsg))
		{
			sve = SVE_FAIL;
			std::ostringstream ostr;
			ostr << "Cannot Hide " << drive << ". " << errmsg <<std::endl;
			error = ostr.str().c_str();
			return sve;
		}
		//Bug #7348 - mountpoint with spaces
		std::string mountpoint = "\"";
		mountpoint += (*mnt_iter);
		mountpoint += "\"";
		const char * mntpt = mountpoint.c_str();
		//changes for Bug 4147
		//checking for PWD is same as hide drive mount point directory or not
		//CHK_DIR: is added in the error string to identify the below condition failed
		//this is added to identify not a failed case in the vsnap unmount
		char cwd_path[MAXPATHLEN];
		if (ACE_OS::getcwd (cwd_path, sizeof (cwd_path)) == 0)
		{
			if(ENOENT != ACE_OS::last_error())
			{
				std::stringstream errmsgcwd;
				errmsgcwd << "Failed to get the current directory.\n";
				errmsgcwd <<"errno = " << ACE_OS::last_error() << "\nerror = " << ACE_OS::strerror(ACE_OS::last_error());
				errmsgcwd <<" .\n";
				error = "CHK_DIR: " + errmsgcwd.str();
				DebugPrintf(SV_LOG_INFO, "%s\n",errmsgcwd.str().c_str());
				return SVE_FAIL;
			}
		}
		else
		{
			std::string curdir = cwd_path;
			std::string mountdir = (*mnt_iter);
			if(0 == curdir.find(mountdir, 0))
			{
				std::string errmsg;
				errmsg = "The mount directory is ";
				errmsg += mountdir;
				errmsg += ".\nThe present working directory is ";
				errmsg += curdir;
				errmsg += ".\nThe present working directory is inside the mount directory.\n";
				error = "CHK_DIR: " + errmsg;
				DebugPrintf(SV_LOG_INFO, "%s\n",errmsg.c_str());
				return SVE_FAIL;
			}
		}
		DebugPrintf(SV_LOG_INFO, "\n\nUnMounting %s ...\n", mntpt);

		SV_UINT retrycount = 0; 
		int retval = 0;
		do
		{
			retval = TerminateUsersAndUnmountMountPoint(mntpt,error);
			if(retval == -1 || (retval == 1 && retrycount == max_unmount_reties))
			{
				return SVE_FAIL;
			}
			else if(retval == 1)
			{
				DebugPrintf(SV_LOG_INFO, "Attempting to retry unmount operation on %s.\n", mntpt);
				retrycount++;
			}
		}while(retval);
        //        VsnapDeleteMntPoint(mntpt) ;
    } // finished removing the mount points

    if(mnt_iter != mnt_end)
    {
        DebugPrintf(SV_LOG_ERROR, "Removal of Mountpoints corresponding to %s failed.\n", drive);
        error = "HideDrive(): Removal of Mountpoints corresponding to ";
        error += drive;
        error += " failed";
        //if(fd != -1) close(fd);
        return SVE_FAIL;
    }

    // fuser -m returns 1 if nobody accessess the specified block device
    // It returns 0 if atleast one process accesses the block device

    DebugPrintf(SV_LOG_INFO, "\nTrying to find processes accessing %s ...\n", drive);

    std::string command = SV_FUSER_FORDEV;

    InmCommand fuserdevice(command + dev);

    std::string pathvar = "PATH=";
    pathvar += GetCommandPath(command);
    fuserdevice.PutEnv(pathvar);

    InmCommand::statusType status = fuserdevice.Run();

    if (status != InmCommand::completed) {
        DebugPrintf(SV_LOG_ERROR, 
            "Function %s @LINE %d in FILE %s :failed to find processes accessing %s\n",
            FUNCTION_NAME, LINE_NO, FILE_NAME,drive);
        error = "HideDrive(): Failed to find processes accessing ";
        error += drive;
        if(!fuserdevice.StdErr().empty())
        {
            error += fuserdevice.StdErr();
        }
        //if(fd != -1) close(fd);
        return SVE_FAIL;
    }

    if(!fuserdevice.ExitCode()) //Device is busy, exitcode returned is 0
    {
        DebugPrintf(SV_LOG_INFO, "\nTrying to shutdown processes accessing %s ...\n", drive);
        command = SV_FUSER_DEV_KILL;

        InmCommand fuserdevicekill(command + dev);
        pathvar = "PATH=";
        pathvar += GetCommandPath(command);
        fuserdevicekill.PutEnv(pathvar);

        status = fuserdevicekill.Run();

        if (status != InmCommand::completed) {
            DebugPrintf(SV_LOG_ERROR, 
                "Function %s @LINE %d in FILE %s :failed to shutdown processes accessing %s\n",
                FUNCTION_NAME, LINE_NO, FILE_NAME,drive);
            error = "HideDrive(): Failed to shutdown processes accessing ";
            error += drive;
            if(!fuserdevicekill.StdErr().empty())
            {
                error += fuserdevicekill.StdErr();
            }
            //if(fd != -1) close(fd);
            return SVE_FAIL;
        }

        if (fuserdevicekill.ExitCode()) {
            DebugPrintf(SV_LOG_ERROR, 
                "Function %s @LINE %d in FILE %s :failed to shutdown processes accessing %s\n",
                FUNCTION_NAME, LINE_NO, FILE_NAME,drive);
            error = "HideDrive(): Failed to shutdown processes accessing ";
            error += drive;
            std::ostringstream msg;
            msg << "Exit Code = " << fuserdevicekill.ExitCode() << std::endl;
            if(!fuserdevicekill.StdOut().empty())
            {
                msg << "Output = " << fuserdevicekill.StdOut() << std::endl;
            }
            if(!fuserdevicekill.StdErr().empty())
            {
                msg << "Error = " << fuserdevicekill.StdErr() << std::endl;
            }
            error += msg.str();
            //if(fd != -1) close(fd);
            return SVE_FAIL;
        }

    }

    DebugPrintf(SV_LOG_INFO, "Removing entries for the device %s from %s ...\n", drive, SV_FSTAB);
    if(!DeleteFSEntry(drive))
    {
        DebugPrintf(SV_LOG_INFO, "Note: %s was not updated. please update it manually.\n", SV_FSTAB);
    }

    DebugPrintf(SV_LOG_INFO, "Removal of Mountpoints corresponding to %s succeeded\n", drive);

    return sve;
}


int comparevolnames(const char *vol1, const char *vol2)
{
    return strcmp(vol1, vol2);
}


bool  VsnapDeleteMntPoint(const char *MntPoint ) 
{
    std::string deleteDir;
    /* Adding PATH of rmdir explicitly */
    deleteDir ="rmdir  ";
    std::string pathvar = "PATH=";
    pathvar += GetCommandPath(deleteDir);
    deleteDir += MntPoint;


    std::ostringstream args;

    args << deleteDir.c_str() ;
    InmCommand rmdir(args.str());

    rmdir.PutEnv(pathvar);
    InmCommand::statusType status = rmdir.Run();
    if(status != InmCommand::completed || rmdir.ExitCode())
    {
        DebugPrintf(SV_LOG_ERROR,"Function %s @LINE %d in FILE %s :InValid PID get while removing the mount point directory  accessing %s\n",FUNCTION_NAME, LINE_NO, FILE_NAME,MntPoint);
        DebugPrintf(SV_LOG_ERROR,"%s\n", rmdir.StdErr().c_str());
        return false;
    }
    return true ;
}


/**
*
* This function has been made common to linux
* and sun by using standart c++ ifstream,
* std::getline for reading a line from 
* SV_FSTAB
* 
*/ 
bool AddFSEntry(const char * dev,std::string  mountpoint, std::string type, std::string flags, bool remount)
{
    bool rv = true;
    std::vector<std::string> fsents;
    std::string fsent_device;
    ACE_HANDLE handle = ACE_INVALID_HANDLE ;

    // Added to fix runtime fstab updation race condition
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard( g_fstabLock);

    do 
    {
        CDPLock lock(SV_FSTAB,10);
        if(!lock.acquire())
            return false;

        std::string realdevice = dev;
        std::string label;

        if(!GetDeviceNameFromSymLink(realdevice))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to get actual device name for %s\n"
                ,FUNCTION_NAME, LINE_NO, FILE_NAME, dev);
            rv = false;
            break;
        }

        /** 
        *
        * For Solaris, currently not passing 
        * the actual device name to the functions
        * addRWFSEntry or addROFSEntry but instead
        * passing the name of form "/dev/dsk/c0t1d0s7"
        *
        */

        std::string device = dev;
        if (IsReportingRealNameToCx())
        {
            device = realdevice;
        }

        std::ifstream ifstab(SV_FSTAB, std::ifstream :: in);
        if(!ifstab.is_open())
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_FSTAB);
            rv = false;
            break;
        }

        std::string line;
        size_t len = 0;

        while(std::getline(ifstab, line))
        {
            line += '\n';
            fsents.push_back(line.c_str());
        }

        ifstab.close();

        if (!FormVolumeLabel(device, label))
        {
            DebugPrintf(SV_LOG_ERROR,
                "In Function %s @LINE %d in FILE %s :FormVolumeLabel failed \n",
                FUNCTION_NAME, LINE_NO, FILE_NAME);
            rv = false;
            break;
        }

        std::string fstabCopy = SV_FSTAB;
        fstabCopy += ".copy";
        fstabCopy += boost::lexical_cast<std::string>(ACE_OS::getpid());

        // PR#10815: Long Path support
        handle = ACE_OS::open( getLongPathName(fstabCopy.c_str()).c_str(),O_WRONLY | O_CREAT | O_EXCL);
        if(ACE_INVALID_HANDLE == handle)
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str());
            rv = false;
            break;
        }

        if(flags == "rw ")
        {
            if(!addRWFSEntry(device, label, mountpoint, type, fsents))
            {
                rv=false;
                break;
            }
        }
        else
        {
            if(!addROFSEntry(device, label, mountpoint, type, fsents))
            {
                rv=false;
                break;
            }
        }

        std::vector<std::string>::iterator fsent_iter = fsents.begin();
        std::vector<std::string>::iterator fsent_end = fsents.end();
        for(; fsent_iter != fsent_end; ++fsent_iter)
        {	
            std::string fsent = *fsent_iter;
            if(ACE_OS::write(handle,fsent.c_str(), strlen(fsent.c_str())) != strlen(fsent.c_str()))
            {
                DebugPrintf(SV_LOG_ERROR,
                    "Function %s @LINE %d in FILE %s :failed to update %s \n",
                    FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str());
                rv = false;
                break;
            }
        }

        ACE_OS::close(handle);
        handle = ACE_INVALID_HANDLE ;

        if( -1 == rename(fstabCopy.c_str(), SV_FSTAB))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to rename %s as %s. errno=%d\n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str(), SV_FSTAB, errno);
            rv = false;
            break;
        }

        rv = true;
    } while (0);

    if(ACE_INVALID_HANDLE != handle)
        ACE_OS::close(handle);

    return rv;

}

bool AddCIFS_FSEntry(const char * dev, std::string  mountpoint, std::string type)
{
    bool rv = true;
    std::vector<std::string> fsents;
    std::string fsent_device;
    ACE_HANDLE handle = ACE_INVALID_HANDLE ;

    // Added to fix runtime fstab updation race condition
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard( g_fstabLock);

    do 
    {
        CDPLock lock(SV_FSTAB,10);
        if(!lock.acquire())
            return false;

        std::string realdevice = dev;
        std::string label;

        std::ifstream ifstab(SV_FSTAB, std::ifstream :: in);
        if(!ifstab.is_open())
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_FSTAB);
            rv = false;
            break;
        }

        std::string line;
        size_t len = 0;

        while(std::getline(ifstab, line))
        {
            line += '\n';
            fsents.push_back(line.c_str());
        }

        ifstab.close();

        std::string fstabCopy = SV_FSTAB;
        fstabCopy += ".copy";
        fstabCopy += boost::lexical_cast<std::string>(ACE_OS::getpid());

        // PR#10815: Long Path support
        handle = ACE_OS::open( getLongPathName(fstabCopy.c_str()).c_str(),O_WRONLY | O_CREAT | O_EXCL);
        if(ACE_INVALID_HANDLE == handle)
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str());
            rv = false;
            break;
        }
        if(!addRWFSEntry(realdevice, label, mountpoint, type, fsents))
        {
            rv=false;
            break;
        }
        std::vector<std::string>::iterator fsent_iter = fsents.begin();
        std::vector<std::string>::iterator fsent_end = fsents.end();
        for(; fsent_iter != fsent_end; ++fsent_iter)
        {	
            std::string fsent = *fsent_iter;
            if(ACE_OS::write(handle,fsent.c_str(), strlen(fsent.c_str())) != strlen(fsent.c_str()))
            {
                DebugPrintf(SV_LOG_ERROR,
                    "Function %s @LINE %d in FILE %s :failed to update %s \n",
                    FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str());
                rv = false;
                break;
            }
        }

        ACE_OS::close(handle);
        handle = ACE_INVALID_HANDLE ;

        if( -1 == rename(fstabCopy.c_str(), SV_FSTAB))
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to rename %s as %s. errno=%d\n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, fstabCopy.c_str(), SV_FSTAB, errno);
            rv = false;
            break;
        }

        rv = true;
    } while (0);

    if(ACE_INVALID_HANDLE != handle)
        ACE_OS::close(handle);

    return rv;

}

/*
* FUNCTION NAME :     ExecuteInmCommand
*
* DESCRIPTION :   Execute the command on the system 
*                  
*
* INPUT PARAMETERS :  string fsmount ( contains the command to be run)
*
* OUTPUT PARAMETERS :  string errormsg (Error Massage ) , int ecode (Error code)
*
* return value : true if command execution succeeded, otherwise false
*
*/
bool ExecuteInmCommand(const std::string& fsmount, std::string& errormsg, int & ecode)
{
    bool rv = true;
    InmCommand mount(fsmount);
    InmCommand::statusType status = mount.Run();
    if (status != InmCommand::completed)
    {
        if(!mount.StdErr().empty())
        {
            errormsg = "Error = ";
            errormsg += mount.StdErr();
            errormsg += "\n";
            ecode = mount.ExitCode(); 
        }
        rv = false;
    }
    else if (mount.ExitCode())
    {
        std::ostringstream msg;
        ecode = mount.ExitCode();
        msg << "Exit Code = " << mount.ExitCode() << std::endl;
        if(!mount.StdOut().empty())
        {
            msg << "Output = " << mount.StdOut() << std::endl;
        }
        if(!mount.StdErr().empty())
        {
            msg << "Error = " << mount.StdErr() << std::endl;
        }
        errormsg = msg.str();
        rv = false;
    }
    return rv;
}


bool LinkExists(const std::string linkfile)
{
    bool retval = false;
    int lstatretval;
    struct stat stbuf;

    lstatretval = lstat(linkfile.c_str(), &stbuf);

    if (!lstatretval)
    {
        retval = true;
    }

    return retval;
}


bool FileExists(const std::string file)
{
    bool retval = false;
    int statretval;
    struct stat stbuf;

    statretval = stat(file.c_str(), &stbuf);

    if (!statretval)
    {
        retval = true;
    }

    return retval;
}

bool IsValidMountPoint(const std::string & volume,std::string & errmsg)
{
    bool rv = true;
    LocalConfigurator localConfigurator;
    std::string sMount = volume;
    std::string::size_type len = sMount.length();
    std::string::size_type idx = len;
    if ((len > 1) && ('/' == sMount[len - 1]))
        --idx;
    if (idx < len)
        sMount.erase(idx);
    std::string invalidmntdirfilename = localConfigurator.getNotAllowedMountPointFileName();

    do
    {
        if (sMount.empty())
        {
            errmsg += "Specified mount point is empty.\n";
            rv = true;
            break;
        }
        if (strcmp(sMount.c_str(), "none") == 0)
        {
            errmsg += "The mount point specified as none.\n";
            rv = false;
            break;
        }
        std::string cmpdirname;
        if(sMount.size() >= strlen("/dev/"))
            cmpdirname = "/dev/";
        else
            cmpdirname = "/dev";

        if (strncmp(sMount.c_str(), cmpdirname.c_str(), cmpdirname.size()) == 0) 
        {
            errmsg += "The directory /dev is used for device file\n. So mounting any volume to /dev or its subdirectories is not allowed.\n";
            rv = false;
            break;
        }
        cmpdirname.clear();
        if(sMount.size() >= strlen("/proc/"))
            cmpdirname = "/proc/";
        else
            cmpdirname = "/proc";

        if (strncmp(sMount.c_str(), cmpdirname.c_str(), cmpdirname.size()) == 0) 
        {
            errmsg += "The directory /proc is used by system\n. So mounting any volume to /proc or its subdirectories is not allowed.\n";
            rv = false;
            break;
        }
        if (strcmp(sMount.c_str(), "/") == 0) 
        {
            errmsg += "The directory / is used as root. So mounting any volume to / is not allowed.\n";
            rv = false;
            break;
        }
        std::string cacheDir = localConfigurator.getCacheDirectory();
        std::string mountpath = sMount;
        if(cacheDir.size() > mountpath.size())
            mountpath += "/";
        if((cacheDir.size() >= mountpath.size()) && 
            (strncmp(cacheDir.c_str(),mountpath.c_str(),mountpath.size()) == 0))
        {
            errmsg += "The mount point ";
            errmsg += mountpath;
            errmsg += " is a parent directory of cache location.\n";
            errmsg += "So mounting any volume to ";
            errmsg += cacheDir;
            errmsg += " or its parent directories is not allowed.\n";
            rv = false;
            break;
        }

        std::string installDir = localConfigurator.getInstallPath();
        mountpath = sMount;
        if(installDir.size() > mountpath.size())
            mountpath += "/";
        if((installDir.size() >= mountpath.size()) && 
            (strncmp(installDir.c_str(),mountpath.c_str(),mountpath.size()) == 0))
        {
            errmsg += "The mount point ";
            errmsg += mountpath;
            errmsg += " is a parent directory of Vx install location.\n";
            errmsg += "So mounting any volume to ";
            errmsg += installDir;
            errmsg += " or its parent directories is not allowed.\n";
            rv = false;
            break;
        }
        else
        {
            std::string installbindir = installDir;
            installbindir += "/bin";
            std::string installetcdir = installDir;
            installetcdir += "/etc";
            std::string installbin1dir = installDir;
            installetcdir += "/bin1";
            if((strcmp(sMount.c_str(), installbindir.c_str()) == 0)||
                (strcmp(sMount.c_str(), installetcdir.c_str()) == 0)||
                (strcmp(sMount.c_str(), installbin1dir.c_str()) == 0))
            {
                errmsg += "The directory ";
                errmsg += sMount;
                errmsg += " is used as vx installation directory. So mounting any volume to ";
                errmsg += sMount;
                errmsg += "is not allowed.\n";
                rv = false;
                break;
            }
        }

        int counter = localConfigurator.getVirtualVolumesId();
        while(counter!=0)
        {
            char regData[26];
			inm_sprintf_s(regData, ARRAYSIZE(regData), "VirVolume%d", counter);
            std::string data = localConfigurator.getVirtualVolumesPath(regData);
            std::string sparsefile;
            std::string virtualvolume;
            if(!ParseSparseFileNameAndVolPackName(data, sparsefile, virtualvolume))
            {
                counter--;
                continue;
            }
            std::string sparsefiledir = dirname_r(sparsefile.c_str());
            mountpath = sMount;
            if(sparsefiledir.size() > mountpath.size())
                mountpath += "/";
            if((sparsefiledir.size() >= mountpath.size()) && 
                (strncmp(sparsefiledir.c_str(),mountpath.c_str(),mountpath.size()) == 0))
            {
                errmsg += "The mount point ";
                errmsg += sparsefiledir;
                errmsg += " contains virtual volume data.\n";
                errmsg += "So mounting any volume to ";
                errmsg += sparsefiledir;
                errmsg += " or its parent directories is not allowed.\n";
                rv = false;
                break;
            }
            counter--;
        }
        if(!rv)
            break;

        std::ifstream inFile(invalidmntdirfilename.c_str());
        std::stringstream msg;
        if(!inFile)
        {
            DebugPrintf(SV_LOG_INFO, "Unable to open the invalid mount point diectories file %s\n",invalidmntdirfilename.c_str());
            break;
        }
        msg << inFile.rdbuf();
        inFile.close();
        std::string data = msg.str();
        svector_t dirnames = CDPUtil::split(data, "\n");
        svector_t::iterator iter = dirnames.begin();
        for(;iter != dirnames.end();++iter)
        {
            std::string invalidmntdir = (*iter);
            if(strcmp(sMount.c_str(), invalidmntdir.c_str()) == 0)
            {
                errmsg += "The directory ";
                errmsg += sMount;
                errmsg += " is used as system directory. So mounting any volume to ";
                errmsg += sMount;
                errmsg += " is not allowed.\n";
                rv = false;
                break;
            }
        }

    } while(false);
    return rv;
}

#ifdef SV_USES_LONGPATHS
std::wstring getLongPathName(const char* path)
{
    std::string unicodePath = path;
    std::replace(unicodePath.begin(), unicodePath.end(), '\\', '/');
    return ACE_Ascii_To_Wide(unicodePath.c_str()).wchar_rep();
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

int sv_stat(const wchar_t *file, ACE_stat *stat)
{

    return ACE_OS::stat(file, stat);
}


bool IsVmFromDeterministics(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisvm = false;

    for (int i = 0; (i < NELEMS(DeterministicVmCmds)) && (false == bisvm); i++)
    {
        std::string output, err;
        int ecode = 0;
        bool bhaspat = false;
        bool bsucceeded = RunInmCommand(DeterministicVmCmds[i], output, err, ecode);
        if (bsucceeded)
        {
            for (int j = 0; j < NELEMS(DeterministicVmPats); j++)
            {
                bhaspat = (NULL != strstr(output.c_str(), DeterministicVmPats[j]));
                if (bhaspat)
                {
                    hypervisorname = DeterministicHypervisors[j];
                    break;
                }
            }
            if (false == bhaspat)
            {
                bhaspat = pats.IsMatched(output, hypervisorname);
            }
        }
        bisvm = bhaspat;
    }

    return bisvm;
}


bool HasPatternInWMIDB(const std::vector<std::string> &wmis, 
                       Pats &pats)
{
    return false;
}


bool IsDeviceReadable(const int &fd, const std::string &devicename, 
                      const unsigned long long nsecs, 
                      const unsigned long long blksize,
                      const E_VERIFYSIZEAT everifysizeat)
{
    /* DebugPrintf(SV_LOG_DEBUG,"ENTERED %s with devicename %s\n", FUNCTION_NAME, devicename.c_str()); */
    bool bretval = false;
    off_t lseekrval = -1;
    char *buf = NULL;
    std::stringstream error;
    unsigned long long size = nsecs * blksize;

    off_t savecurrpos = lseek(fd, 0, SEEK_CUR);
    if ((off_t)-1 != savecurrpos)
    {
        off_t seekoff = (E_ATLESSTHANSIZE == everifysizeat) ? (size - blksize) : size;
        lseekrval = lseek(fd, seekoff, SEEK_SET);

        if (seekoff == lseekrval)
        {
            buf = (char *)malloc(blksize);
            if (buf)
            {
                ssize_t bytesread;
                bytesread = read(fd, buf, blksize); 
                if ((-1 != bytesread) && (bytesread == blksize))
                {
                    bretval = true;
                }
                else
                {
                    error << "read on device " << devicename << " from " << seekoff; 
                    error << " failed with errno = " << errno << '\n';
                    /* DebugPrintf(SV_LOG_DEBUG, error.str().c_str()); */
                }
                free(buf);    
            }
            else
            {
                error << "malloc failed with errno = " << errno << '\n';
                DebugPrintf(SV_LOG_ERROR, error.str().c_str());
            }
            off_t savebackpos = lseek(fd, savecurrpos, SEEK_SET);
            if (savebackpos != savecurrpos)
            {
                bretval = false;
                error << "lseek on device " << devicename << " to save back original position failed with errno = " << errno << '\n';
                DebugPrintf(SV_LOG_ERROR, error.str().c_str());
            }
        }
        else
        {
            error << "lseek on device " << devicename << " to " << seekoff; 
            error << " failed with errno = " << errno << '\n';
            /* DebugPrintf(SV_LOG_WARNING, error.str().c_str()); */
        }
    }
    else
    {
        error << "lseek on device " << devicename << " to save existing position failed with errno = " << errno << '\n';
        DebugPrintf(SV_LOG_ERROR, error.str().c_str());
    }

    /*
    DebugPrintf(SV_LOG_DEBUG,"EXITED %s. size valid = %s\n",FUNCTION_NAME, 
    bretval?"true":"false");
    */

    return bretval;
}


bool MaskRequiredSignals(void)
{
    return MaskSignal(SIGPIPE);
}


bool MaskSignal(int signo)
{
    return (SIG_ERR != signal(signo, SIG_IGN));
}

void set_resource_limits()
{
    unsigned long long lim = 1048576; //setting deafult NOFILE limt
    struct rlimit rLimit;
    rLimit.rlim_cur = (rlim_t)RLIM_INFINITY;
    rLimit.rlim_max = (rlim_t)RLIM_INFINITY;

    // RLIMIT_FSIZE :  The largest size, in bytes, of any single file that can be created with in a process
    setrlimit(RLIMIT_FSIZE,  &rLimit);

    // RLIMIT_DATA :  The maximum size, in bytes, of the data region for a process a.k.a. Data Segment
    setrlimit(RLIMIT_DATA,  &rLimit);


    // RLIMIT_STACK : The  maximum size of the process stack,
    setrlimit(RLIMIT_STACK,  &rLimit);

    //RLIMIT_AS : maximum size of a process's total available memory,
    setrlimit(RLIMIT_AS,  &rLimit);
	
	//RLIMIT_CORE :  Maximum size of core file
    setrlimit(RLIMIT_CORE,  &rLimit);

#ifdef SV_AIX
#ifdef RLIMIT_THREADS
    //RLIMIT_THREADS :  The maximum number of threads each process can create.
    setrlimit(RLIMIT_THREADS,  &rLimit);
#endif

#endif
	
    // RLIMIT_NOFILE :  Specifies a value one greater than the maximum file descriptor number that can be opened by this process.
	//Try initally with 1048576 count and decrement till succeeded
    while(lim > 1024) //by default NOFILE limit is set as 1024,so not calling setrlimit once we reach this limit. 
    {
		rLimit.rlim_cur = lim;
        rLimit.rlim_max = lim;
        if(setrlimit(RLIMIT_NOFILE,  &rLimit) == 0)
        {
            break;
        }
        else
           --lim;
     }
}


bool InmRename(const std::string &oldfile, const std::string &newfile, std::string &errmsg)
{
    bool brenamed = (0 == rename(oldfile.c_str(), newfile.c_str()));
    if (!brenamed)
    {
        errmsg = "Failed to rename file ";
        errmsg += oldfile;
        errmsg += " to ";
        errmsg += newfile;
        errmsg += " with error message: ";
        errmsg += strerror(errno);
    }

    /* NOTE: need nothing here for now
    else
    {
        std::string nerrmsg;
        brenamed = SyncFile(newfile, nerrmsg);
        if (!brenamed)
        {
            errmsg = "Failed to rename file ";
            errmsg += oldfile;
            errmsg += " to ";
            errmsg += newfile;
            errmsg += " because sync on newfile failed with error message: ";
            errmsg += nerrmsg;
        }
    }
    */

    return brenamed;
}

void setsharemode_for_all(mode_t &share)
{
    share = ACE_DEFAULT_FILE_PERMS;
}

void setumask()
{
    umask(S_IWGRP | S_IWOTH);
}

bool enableDiableFileSystemRedirection(const bool bDisableRedirect)
{
    return true ;
}
void FormatVacpErrorCode( std::stringstream& stream, SV_ULONG& exitCode )
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;
    if(exitCode != 0)
	{
		stream << "Vacp failed to issue the consistency. Failed with Error Code:" ;
		switch( exitCode )
		{
            case VACP_E_GENERIC_ERROR: //(10001) 
                stream << "1 (Vacp Generic Error)" ;
                break;
            case VACP_E_INVOLFLT_DRIVER_NOT_INSTALLED: //(10002) 
                stream << "10002. (Scout Filter driver is not loaded. Ensure VX is installed and machine is rebooted)" ;
                break;
            case VACP_E_INVALID_COMMAND_LINE: 
                stream << "0x80004005L. ( Invalid command line. Use Vacp -h to know more about command line options )" ;             
                break;
            case VACP_E_DRIVER_IN_NON_DATA_MODE: //(10003)
                stream << "10003, (Some of the protected volumes is not in write order data mode. Consistency point will be tried in next schedule)" ;
                break ;
            case VACP_E_NO_PROTECTED_VOLUMES: //10004
                stream << "10004, (One of the volumes is either not available or not protected. If the volume is deleted, make sure to recreate it or recover it. If not protected remove it from the vacp command line.) " ;
                break ;
        }
		stream << std::endl ;
	}
	DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME ) ;
}


bool GetBiosId(std::string& biosId)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;

    std::string output, err;
    int ecode = 0;
    bool bsucceeded = false;
    std::stringstream ssErrorMsg;
    
    std::list<std::string> biosCommandList;
    biosCommandList.push_back(BIOSIDCMD);
    biosCommandList.push_back(BIOSIDCMD2);

    std::list<std::string>::iterator iter = biosCommandList.begin();
    for (/*empty*/; iter != biosCommandList.end(); iter++)
    {
        bsucceeded = RunInmCommand(*iter, output, err, ecode);
        if (bsucceeded)
        {
            std::stringstream results(output);
            std::string biosid((std::istreambuf_iterator<char>(results)),
                                       std::istreambuf_iterator<char>());
            boost::trim(biosid);
            biosId = biosid;
            DebugPrintf(SV_LOG_DEBUG, "BIOS ID: %s\n", biosId.c_str());
            break;
        }
        else
        {
            ssErrorMsg << "BIOS ID command ";
            ssErrorMsg << *iter;
            ssErrorMsg << " failed with exit code ";
            ssErrorMsg << ecode;
            ssErrorMsg << ".";
            
        }
    }

    if (!bsucceeded)
        DebugPrintf(SV_LOG_ERROR, "%s\n", ssErrorMsg.str().c_str());

    DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME ) ;

	return bsucceeded;
}

bool GetBaseBoardId(std::string& baseboardId)
{
 /* nothin to do */
 return false ;

}
std::string GetDefaultGateways(const char &delim)
{
    std::string output, err;
    int ecode = 0;
    std::string gw;

    bool bset = RunInmCommand(ROUTINGTABLECMD, output, err, ecode);
    if (bset)
    {
        const std::string DESTTOK = "Destination";
        const std::string GWTOK = "Gateway";
        const std::string FLAGSTOK = "Flags";
        int destpos, gwpos, flagspos;
        std::stringstream results(output);
        destpos = gwpos = flagspos = -1;
        std::string str;

        while (!results.eof())
        {
            str.clear();
            results >> str;
            if (DESTTOK == str)
            {
                destpos = 0;
                break;
            }
            else
            {
                std::getline(results, str);
            }
        }

        if (-1 != destpos)
        {
            int pos = 1;
            while (!results.eof())
            {
                str.clear();
                results >> str;
                if (GWTOK == str)
                {
                    gwpos = pos;
                }
                if (FLAGSTOK == str)
                {
                    flagspos = pos;
                }
                if ((gwpos != -1) && (flagspos != -1))
                {
                    std::getline(results, str);
                    break;
                }
                pos++;
            }
        }

        DebugPrintf(SV_LOG_DEBUG, "destpos = %d, gwpos = %d, flagspos = %d\n", destpos, gwpos, flagspos);
        if ((destpos != -1) && (gwpos != -1) && (flagspos != -1))
        {
            gw = ParseGateways(results, destpos, gwpos, flagspos, delim);
        }
    }
    else
    {
        std::stringstream ssecode;
        ssecode << ecode;
        std::string errormsg = "getting default gateway command failed with exit code ";
        errormsg += ssecode.str();
        DebugPrintf(SV_LOG_DEBUG, "%s\n", errormsg.c_str());
    }

    return gw;
}


std::string ParseGateways(std::stringstream &results, const int &destpos, 
                          const int &gwpos, const int &flagspos, const char &delim)
{
    std::string str;
    const std::string DEFAULTSTR = "default";
    const std::string DEFAULTDEST = "0.0.0.0";
    const char UPFLAG = 'U';
    const char GWFLAG = 'G';
    std::string gw, flags;
    std::string rgw;

    while (!results.eof())
    {
        str.clear();
        results >> str;
        if ((DEFAULTSTR == str) || (DEFAULTDEST == str))
        {
            int pos = 1;
            while (pos <= flagspos)
            {
                str.clear();
                results >> str;
                if (pos == gwpos)
                {
                    gw = str;
                }
                if (pos == flagspos)
                {
                    flags = str;
                }
                if (!gw.empty() && !flags.empty())
                {
                    break;
                }
                pos++;
            }
            std::getline(results, str);
            if (!gw.empty() && !flags.empty() && 
                (std::string::npos != flags.find(GWFLAG)) && 
                (std::string::npos != flags.find(UPFLAG)) &&
                IsValidIP(gw))
            {
                if (rgw.empty())
                {
                    rgw = gw;
                }
                else
                {
                    rgw += delim;
                    rgw += gw;
                }
            }
        }
        else
        {
            std::getline(results, str);
        }
    }

    return rgw;
}


std::string GetNameServerAddresses(const char &delim)
{
    const char *file = "/etc/resolv.conf";
    std::ifstream in(file);
    std::string addrs;

    if (!in.is_open())
    {
        DebugPrintf(SV_LOG_DEBUG, "could not open file %s to find nameserver addresses\n", file);
    }
    else
    {
        std::string nstok, ns;
        const std::string NSKEY = "nameserver";
        while (!in.eof())
        {
            nstok.clear();
            ns.clear();
            in >> nstok >> ns;
            if ((nstok == NSKEY) && IsValidIP(ns))
            {
                if (addrs.empty())
                {
                    addrs = ns;
                }
                else
                {
                    addrs += delim;
                    addrs += ns;
                }
            }
        }
        in.close();
    }

    return addrs;
}

/*
 * Terminates any processes accessing the mount point and unmounts the mount point
 * Input - Mount point to be unmounted.
 * Return Value - 0 Unmount successful
 *                1 Command status failed
 *                -1 Command could not be executed.
 */
int TerminateUsersAndUnmountMountPoint(const char * mntpt,std::string& error)
{
	int retval = 0; //Unmount successful.
	do
	{
		// Bug #4044
		// Using InmCommand to execute the command
		// and capture the error output and exitcode
		DebugPrintf(SV_LOG_INFO, "Trying to find processes accessing %s ...\n", mntpt);

		std::string command = SV_FUSER_FOR_MNTPNT;

		InmCommand fusermnt_cmd(command + mntpt);

		std::string pathvar = "PATH=";
		pathvar += GetCommandPath(command);
		fusermnt_cmd.PutEnv(pathvar);

		InmCommand::statusType status = fusermnt_cmd.Run();

		if(status != InmCommand::completed) {
			DebugPrintf(SV_LOG_ERROR, 
				"Function %s @LINE %d in FILE %s :failed to find processes accessing %s\n",
				FUNCTION_NAME, LINE_NO, FILE_NAME,mntpt);
			error = "Failed to execute the command ";
			error += command;
			error += mntpt;
			if(!fusermnt_cmd.StdErr().empty())
			{
				error += fusermnt_cmd.StdErr();
			}
			retval = -1;
			break;
		}

		if(!fusermnt_cmd.ExitCode())
		{
			DebugPrintf(SV_LOG_INFO, "Trying to shutdown processes accessing %s ...\n", mntpt);
			command = SV_FUSER_MNTPNT_KILL;
			InmCommand fuserkill(command + mntpt);

			pathvar = "PATH=";
			pathvar += GetCommandPath(command);
			fuserkill.PutEnv(pathvar);

			status = fuserkill.Run();


			if (status != InmCommand::completed) { 
				DebugPrintf(SV_LOG_ERROR, 
					"Function %s @LINE %d in FILE %s :failed to shutdown processes accessing %s\n",
					FUNCTION_NAME, LINE_NO, FILE_NAME,mntpt);
				error = "Failed to shutdown processes accessing ";
				error += mntpt;
				if(!fuserkill.StdErr().empty())
				{
					error += fuserkill.StdErr();
				}
				retval = -1;
				break;
			}

			if(fuserkill.ExitCode())
			{
				DebugPrintf(SV_LOG_ERROR, 
					"Function %s @LINE %d in FILE %s :failed to shutdown processes accessing %s\n",
					FUNCTION_NAME, LINE_NO, FILE_NAME,mntpt);
				error = "Failed to shutdown processes accessing ";
				error += mntpt;
				std::ostringstream msg;
				msg << "Exit Code = " << fuserkill.ExitCode() << std::endl;
				if(!fuserkill.StdOut().empty())
				{
					msg << "Output = " << fuserkill.StdOut() << std::endl;
				}
				if(!fuserkill.StdErr().empty())
				{
					msg << "Error = " << fuserkill.StdErr() << std::endl;
				}
				error += msg.str();
				retval = 1;
				break;
			}
		}

		DebugPrintf(SV_LOG_INFO, "Performing unmount operation on %s ...\n", mntpt);
		command = SV_UMOUNT;
		InmCommand unmount(command + mntpt);
		pathvar = "PATH=";
		pathvar += GetCommandPath(command);
		unmount.PutEnv(pathvar);
		status = unmount.Run();

		if (status != InmCommand::completed) {
			DebugPrintf(SV_LOG_ERROR, 
				"Function %s @LINE %d in FILE %s :failed to unmount %s\n",
				FUNCTION_NAME, LINE_NO, FILE_NAME,mntpt);
			error = "Failed to unmount ";
			error += mntpt;
			if(!unmount.StdErr().empty())
			{
				error += unmount.StdErr();
			}
			retval = -1;
			break;
		}

		if(unmount.ExitCode())
		{
			error = "Failed to unmount ";
			error += mntpt;
			std::ostringstream msg;
			msg << " Exit Code = " << unmount.ExitCode()<< std::endl;
			if(!unmount.StdOut().empty())
			{
				msg << "Output = " << unmount.StdOut() << std::endl;
			}
			if(!unmount.StdErr().empty())
			{
				msg << "Error = " << unmount.StdErr() << std::endl;
			}
			error += msg.str();

			DebugPrintf(SV_LOG_INFO, 
				"Function %s @LINE %d in FILE %s :failed to unmount %s\n : %s\n",
				FUNCTION_NAME, LINE_NO, FILE_NAME,mntpt, error.c_str());
			retval = 1;
			break;
		}
	}while(0);

	return retval;
}


bool PerformPerPlatformRegisterTasks(Configurator* configurator, const bool &registerinanycase, const VolumeSummaries_t& volumeSummaries, Options_t & miscInfo)
{
    /* nothing to do currently */
    return true;
}

void mountBootableVolumesIfRequried()
{
}

bool IsSparseFile(const std::string fileName)
{
	return false;
}

char* AllocatePageAlignedMemory(const size_t &length)
{
    return (reinterpret_cast<char *>(valloc(length)));
}

void FreePageAlignedMemory(char *readdata)
{
    free(readdata);
}
void GetAgentRepositoryAccess(int& accessError, SV_LONG& lastSuccessfulTs, std::string& errmsg)
{
	//To do
}
void UpdateAgentRepositoryAccess(int error, const std::string& errmsg)
{
	//To do
}
void UpdateBookMarkSuccessTimeStamp()
{
	//To do
}
long GetBookMarkSuccessTimeStamp()
{
	return 0 ;//for now
}
void UpdateBackupAgentHeartBeatTime()
{
	//To do
}
long GetAgentHeartBeatTime()
{
	return 0 ;//for now
}
long GetRepositoryAccessTime()
{
	return 0 ; //for now
}
std::string GetServiceStopReason()
{
	return NULL ;//for now
}
void UpdateServiceStopReason(std::string& reason)
{
	//To do
}
void UpdateRepositoryAccess()
{
	//To do
}
unsigned int retrieveBusType(std::string volume)
{
	return 0 ;//for now
}
bool IsRecoveryInProgress()
{
	//To do
	return false;
}
bool IsItTestFailoverVM()
{
	//To do
	return false;
}
std::string GetRecoveryScriptCmd()
{
	std::string cmd;
	//To do
	return cmd;
}
std::string GetSystemUUID()
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;

	std::string systemUUID;
    std::string output, err;
    int ecode = 0;
    std::stringstream ssErrorMsg;
    
	//First check /sys interface and if it is not populated, check /usr/sbin/dmidecode
	//On RHEL5, the /sys interface is not available so using "/usr/sbin/dmidecode -s system-uuid" command to get the BIOS-ID
	//These order is used in UnifiedAgentConfigurator.sh script in update_drscout_config function as well.
	//Do not change this order.
    std::list<std::string> sysCommandList;
    sysCommandList.push_back(SYSUUIDCMD);
    sysCommandList.push_back(SYSUUIDCMD2);

    std::list<std::string>::iterator iter = sysCommandList.begin();
    for (/*empty*/; iter != sysCommandList.end(); iter++)
    {
        if(RunInmCommand(*iter, output, err, ecode))
        {
            std::stringstream results(output);
            results >> systemUUID;
            DebugPrintf(SV_LOG_DEBUG, "System UUID : %s\n", systemUUID.c_str());
            break;
        }
        else
        {
            ssErrorMsg << "getting system UUID from command ";
            ssErrorMsg << *iter;
            ssErrorMsg << " failed with exit code ";
            ssErrorMsg << ecode;
            ssErrorMsg << ".";
        }
    }

    if (systemUUID.empty())
        DebugPrintf(SV_LOG_ERROR, "%s\n", ssErrorMsg.str().c_str()) ;

    DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME ) ;

	return systemUUID;
}

std::string GetChassisAssetTag()
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;

	std::string assetTag;
    std::string output, err;
    int ecode = 0;
    std::stringstream ssErrorMsg;

    std::list<std::string> tagCommandList;
    tagCommandList.push_back(ASSETTAGCMD);
    tagCommandList.push_back(ASSETTAGCMD2);

    std::list<std::string>::iterator iter = tagCommandList.begin();
    for (/*empty*/; iter != tagCommandList.end(); iter++)
    {
        if(RunInmCommand(*iter, output, err, ecode))
        {
            std::stringstream results(output);
            results >> assetTag;
            DebugPrintf(SV_LOG_DEBUG, "Asset tag: %s\n", assetTag.c_str());
            break;
        }
        else
        {
            ssErrorMsg << "getting asset tag from command ";
            ssErrorMsg << *iter;
            ssErrorMsg << " failed with exit code ";
            ssErrorMsg << ecode;
            ssErrorMsg << ".";
        }
    }

    if (assetTag.empty())
        DebugPrintf(SV_LOG_ERROR, "%s\n", ssErrorMsg.str().c_str()) ;

    DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME ) ;

	return assetTag;
}

bool IsSystemAndBootDiskSame(const VolumeSummaries_t & volumeSummaries, std::string & errormsg)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;
    /*
        - Find all devices with attribute has_bootable_partition set
            - Fail if none is found - bug in VIC
            - Fail if more than one found - bug in VIC
    */
    VolumeSummaries_t vVSBootPart;
    std::string strVSBootPart;
    ConstVolumeSummariesIter itrVS(volumeSummaries.begin());
    for (/*empty*/; itrVS != volumeSummaries.end(); ++itrVS)
    {
        ConstAttributesIter_t it = itrVS->attributes.find(NsVolumeAttributes::HAS_BOOTABLE_PARTITION);
        if (it != itrVS->attributes.end() && (0 == it->second.compare("true")))
        {
            DebugPrintf( SV_LOG_DEBUG, "vVSBootPart: %s\n", itrVS->name.c_str() ) ;
            vVSBootPart.push_back(*itrVS);
            strVSBootPart += itrVS->name;
            strVSBootPart += ", ";
        }
    }
    if ( 0 == vVSBootPart.size() )
    {
        /* Note: If we are here then its a bug in VIC */
        errormsg = "No device found in volumeSummaries with bootable partition attribute set.";
        return false;
    }
    if ( vVSBootPart.size() > 1 )
    {
        /* Note: If we are here then its a bug in VIC */
        errormsg = "bootable partition identified on multiple disks in volumeSummaries. ";
        errormsg += strVSBootPart;
        return false;
    }
    
    /*
        - Find logical volume with mount point root "/"
            - Copy volume group for this lv in string rootVG
            - Find all disks matching rootVG
                - Fail if none is found - Bug in VIC
                - Fail if more than one found - root volume on multiple disk
        - If root is mounted on partition volume
            - search through children of each disk
            - Copy name of root partition to rootPartition
    */
    std::string rootDisk;
    std::string rootVG;
    std::string rootPartition;
    for (itrVS = volumeSummaries.begin(); itrVS != volumeSummaries.end(); ++itrVS)
    {
        if ( (0 == itrVS->mountPoint.compare("/")) && (VolumeSummary::LOGICAL_VOLUME == itrVS->type) )
        {
            rootVG = itrVS->volumeGroup;
            break;
        }
        else if ( VolumeSummary::DISK == itrVS->type )
        {
            ConstVolumeSummariesIter itrVSChild(itrVS->children.begin());
            for (/*empty*/; itrVSChild != itrVS->children.end(); ++itrVSChild)
            {
                if ( 0 == itrVSChild->mountPoint.compare("/") )
                {
                    rootPartition = itrVSChild->name;
                    rootDisk = itrVS->name;
                    break;
                }
            }
            if (0 == itrVS->mountPoint.compare("/")) {
                rootPartition = itrVS->name;
                rootDisk = itrVS->name;
                break;
            }
        }
    }
    
    DebugPrintf( SV_LOG_DEBUG, "%s: rootVG=%s, rootPartition=%s\n", FUNCTION_NAME, rootVG.c_str(), rootPartition.c_str()) ;
    if ( rootVG.length() )
    {
        VolumeSummaries_t vVSRootVGDisks;
        for (itrVS = volumeSummaries.begin(); itrVS != volumeSummaries.end(); ++itrVS)
        {
            if ( (VolumeSummary::DISK == itrVS->type) && (0 == itrVS->volumeGroup.compare(rootVG)) )
            {
                vVSRootVGDisks.push_back(*itrVS);
                rootDisk += itrVS->name;
                rootDisk += ", ";
                DebugPrintf( SV_LOG_DEBUG, "vVSRootVGDisks: %s\n", itrVS->name.c_str() ) ;
            }
        }
        
        if ( vVSRootVGDisks.size() > 1)
        {
            DebugPrintf( SV_LOG_DEBUG, "Root is laid on an LVM device coming from multiple disks");
        }
        else if ( vVSRootVGDisks.size() < 1)
        {
            errormsg = "System/root volume not linked to any disks.";
            errormsg += " System disk: ";
            errormsg += rootDisk;
            errormsg += " Root VG: ";
            errormsg += rootVG;
            return false;
        }
    }
    else if ( !rootPartition.length() )
    {
        /* Note: If we are here then its a bug in VIC */
        errormsg = "System/root volume not found in volumeSummaries.";
        return false;
    }

    DebugPrintf( SV_LOG_DEBUG, "rootDisk = %s\n", rootDisk.c_str());
    return true;
}

bool IsUEFIBoot(void)
{
    std::string uefiFolder("/sys/firmware/efi");

    try {
        if (boost::filesystem::exists(uefiFolder) && boost::filesystem::is_directory(uefiFolder)) {
            DebugPrintf(SV_LOG_DEBUG, "Firmware Type: UEFI Boot\n");
            return true;
        }
    }
    catch (const std::exception &e)
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to find %s with exception %s\n", uefiFolder.c_str(), e.what());
    }
    catch (...) {
        DebugPrintf(SV_LOG_DEBUG, "Encountered exception in finding %s\n", uefiFolder.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "Firmware Type: BIOS Boot\n");
    return false;
}
