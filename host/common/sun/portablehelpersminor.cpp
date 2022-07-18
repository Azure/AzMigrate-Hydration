#include <unistd.h>
#include <fcntl.h>
#include <procfs.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <vector>
#include <algorithm>
#include <utility>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <limits>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <dirent.h>

#include "portablehelpers.h"
#include "portablehelperscommonheaders.h"
#include "portablehelpersmajor.h"
#include "portablehelpersminor.h"
#include "inmdefines.h"
#include "configwrapper.h"
#include <sys/vfstab.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/param.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/mnttab.h>
#include "volumemanagerfunctionsminor.h"
#include "executecommand.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "boost/algorithm/string/replace.hpp"

#include <ace/Guard_T.h>
#include "ace/Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include <sys/systeminfo.h>
#include <sys/utsname.h>

#define SIZE_LIMIT_TOP     0x100000000000LL
#define NUM_SECTORS_PER_TB    (1LL << 31)
#define PSNAMELEN 100

/** 
*
* This header file is not present in solaris 8. Need to
* check if it is available in all versions of solaris 9.
* The solaris 9 build machine has this.
* Need to separate this out based on the solaris version we are
* using.
*
*/

#ifndef SV_SUN_5DOT8
#include "sys/efi_partition.h"
bool GetBasicVolInfoFromEFI(int fd, const std::string &dskdevice, SV_BASIC_VOLINFO *pbasicvolinfo);
bool IsFullEFIRawDisk(const char *disk);
#endif /* SV_SUN_5DOT8 */

ACE_Recursive_Thread_Mutex g_lockforgetbasicvolinfo;

#define THRSHOLD_NUM_FIELDSINFSTAB 7
#define FLAG_FIELD_IN_FSTAB 6

bool ShouldCollectNicInfo(const int &sockfd, struct lifreq *plifr, const char *ipaddr);
bool FillNicInfos(const int &sockfd, struct lifconf *plic, struct lifnum *pnum, NicInfos_t & nicInfos);
void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask, E_INM_TRISTATE &isdhcpenabled);
void InsertNicInfo(NicInfos_t &nicInfos, const std::string &name, const std::string &hardwareaddress, const std::string &ipaddress, 
                   const std::string &netmask, const E_INM_TRISTATE &isdhcpenabled);
void UpdateNicInfoAttr(const std::string &attr, const std::string &value, NicInfos_t &nicinfos);

static ACE_Mutex getmntent_lock;
/* SUN */
SVERROR GetFsVolSize(const char *volName, unsigned long long *pfsSize)
{
    SVERROR sv = SVE_FAIL;
    if (!(volName && pfsSize))
    {
        /* We need to log a message but if caller has not initialized log then
        * crash may occur */ 
        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, invalid arguements supplied\n", LINE_NO, FILE_NAME);
    }
    else
    {
        *pfsSize = GetRawVolSize(volName);
        if (*pfsSize)
        {
            sv = SVS_OK;
        }
        else
        {
            /* We need to log a message but if caller has not initialized log then
            * crash may occur */ 
            DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, GetRawVolSize failed for volume %s\n", LINE_NO, FILE_NAME, volName);
        } 
    }
    return sv;
}

/* SUN Adding a function get raw volume size */
/* Returns 0 on success */
SV_ULONGLONG GetRawVolSize(const std::string &sVolume)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s with sVolume = %s\n",FUNCTION_NAME, sVolume.c_str());
    SV_BASIC_VOLINFO basicvolinfo;
    SV_ULONGLONG rawsize = 0;
    if (!GetBasicVolInfo(sVolume, &basicvolinfo))
    {
        DebugPrintf(SV_LOG_ERROR, "@LINE %d in FILE %s, GetBasicVolInfo failed for volume %s\n", 
            LINE_NO, FILE_NAME, sVolume.c_str());
    }
    else
    {
        rawsize = basicvolinfo.m_ullrawsize;
    }

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s with rawsize = " ULLSPEC "\n",FUNCTION_NAME, rawsize);
    return rawsize;
}

SVERROR GetFsSectorSize(const char *volName, unsigned int *psectorSize)
{
    SVERROR sv;
    sv = GetRawVolumeSectorSize(volName, psectorSize);
    return sv;
}

/* SUN */
SVERROR GetRawVolumeSectorSize(const char *volName, unsigned int *psectorSize)
{
    SVERROR sv = SVE_FAIL;
    if (!(volName && psectorSize))
    {
        /* We need to log a message but if caller has not initialized log then
        * crash may occur */ 
        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, invalid arguements supplied\n", LINE_NO, FILE_NAME);
    }
    else
    {
        SV_BASIC_VOLINFO basicvolinfo;
        if (!GetBasicVolInfo(volName, &basicvolinfo))
        {
            DebugPrintf(SV_LOG_ERROR, "@LINE %d in FILE %s, GetBasicVolInfo failed for volume %s\n", 
                LINE_NO, FILE_NAME, volName);
        }
        else
        {
            *psectorSize = basicvolinfo.m_uhsectorsz;
            sv = SVS_OK;
        }

        /* sector size may be 0
        * This is case with sun volume manager 
        * volumes 
        */
    }
    return sv;
}


/* SUN */

bool addRWFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, const std::string& type,
                  std::vector<std::string>& fsents)
{
    bool rv=true;

    do
    {
        if(!removeFSEntry(device, label, fsents))
        {
            rv =false;
            break;
        }

        //device       device       mount      FS      fsck    mount      mount
        //to mount     to fsck      point      type    pass    at boot    options

        std::string realDevice = device;
        if (!GetDeviceNameFromSymLink(realDevice))
        {
            rv = false;
            break;
        }

        std::vector<std::string>::iterator fsent_iter = fsents.begin();
        std::vector<std::string>::iterator fsent_end = fsents.end();
        std::vector<std::string> tempFSEnts;
        bool addNewEntry = true;
        for(; fsent_iter != fsent_end; ++fsent_iter)
        {    
            std::string fsent = *fsent_iter;

            if ('#' == fsent[0])
            {
                svector_t fields;
                Tokenize(fsent, fields, " \t\n");
                if(fields.size() != THRSHOLD_NUM_FIELDSINFSTAB)
                {
                    tempFSEnts.push_back(fsent);
                    continue;
                }

                /* fstab_devtomnt may be device or real device */
                std::string fstab_devtomnt(fields[0].begin()+1, fields[0].end());
                std::string fstab_devtofsck = fields[1];
                std::string fstab_mntpnt = fields[2];
                std::string fstab_fstype = fields[3];
                std::string fstab_fsckpass = fields[4];
                std::string fstab_mntatboot = fields[5];
                std::string fstab_mntopts = fields[6];

                std::string fstab_realdev = fstab_devtomnt;
                if (!GetDeviceNameFromSymLink(fstab_realdev))
                {
                    tempFSEnts.push_back(fsent);
                }
                else 
                {
                    /* We have in hand, device and realdevice. The fstab_devtomnt 
                    can be equal to any of them or both */
                    if (realDevice == fstab_realdev)
                    {
                        if (fstab_fstype == type)
                        {
                            if ((mountpoint == "") || (fstab_mntpnt == mountpoint))
                            {
                                /* mountpoint is empty or fstab_mntpnt and mountpoint match then
                                * uncomment
                                * (can mountpoint be empty?)
                                */ 
                                //mounting rw with no mountpoint specified or mounting at previous location; uncomment
                                //If entry has real device, it is left as it is
                                std::string uncommentedfsent(fsent.begin()+1, fsent.end());
                                tempFSEnts.push_back(uncommentedfsent);
                            }
                            else
                            {
                                std::string uncommentedfsent; 

                                //device       device       mount      FS      fsck    mount      mount
                                //to mount     to fsck      point      type    pass    at boot    options

                                uncommentedfsent = device + "\t" + 
                                    fstab_devtofsck + "\t" + 
                                    mountpoint + "\t" + 
                                    type + "\t" + 
                                    fstab_fsckpass + "\t" + 
                                    fstab_mntatboot + "\t" + 
                                    fstab_mntopts + "\n";
                                tempFSEnts.push_back(uncommentedfsent);
                            }
                            addNewEntry = false;
                        } /* end if of if (fstab_fstype == type)) */
                        else
                        {
                            /* fstyp does not match. delete this entry */
                            ; 
                        }
                    } /* end of if (realDevice == fstab_devtomnt) */
                    else
                    {
                        /* device does not match. push the entry as it is */ 
                        tempFSEnts.push_back(fsent);
                    }
                } /* end of else */
            } /* end of if ('#' == fsent[0]) */
            else // case: uncommented entries
            {
                //write all uncommented entries as they are.
                tempFSEnts.push_back(fsent);
            }
        } /* End of for */

        if(fsent_iter != fsent_end)
        {
            rv = false;
            break;
        }

        if(!rv)
            break;

        // if there were no matching commented entries for this in the fstab, create new one
        if(addNewEntry)
        {
            std::string newFSEntry;
            newFSEntry = device + "\t" + "-" + "\t" + mountpoint + "\t" + type + "\t" + "-" + "\t" + "yes" + "\t" + "rw" + "\n";
            tempFSEnts.push_back(newFSEntry);
        }

        fsents.assign(tempFSEnts.begin(),tempFSEnts.end());

    }while(false);

    return rv;
}



/* SUN */

bool addROFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, const std::string& type,
                  std::vector<std::string>& fsents)
{
    bool rv=true;

    do
    {
        if(!removeFSEntry(device, label, fsents))
        {
            rv =false;
            break;
        }

        //device       device       mount      FS      fsck    mount      mount
        //to mount     to fsck      point      type    pass    at boot    options

        std::string newFSEntry;
        newFSEntry = device + "\t" + "-" + "\t" + mountpoint + "\t" + type + "\t" + "-" + "\t" + "yes" + "\t" + "ro" + "\n";

        fsents.push_back(newFSEntry);
    }while(false);
    return rv;
}



/* SUN */
bool MountVolume(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly)
{
    int exitCode = 0;
    std::string errorMsg = "";
    bool bmounted = MountDevice(device, mountPoint, fsType, bSetReadOnly, exitCode, errorMsg); 
    std::string flags;

    if(bSetReadOnly)
        flags="ro ";
    else
        flags="rw ";

    std::string sparsefile;
    bool new_sparsefile_format = false;
    bool is_volpack = IsVolPackDevice(device.c_str(),sparsefile,new_sparsefile_format);
    if (bmounted && (!is_volpack))
    {
        if(!AddFSEntry(device.c_str(),mountPoint.c_str(),fsType.c_str(),flags.c_str(),false))
        {
            DebugPrintf(SV_LOG_INFO, "Note: /etc/vfstab was not updated. please update it manually.\n");
        }
    }
    return bmounted;
}

int posix_fadvise_wrapper(ACE_HANDLE fd, SV_ULONGLONG offset, SV_ULONGLONG len, int advice)
{
    return 0; //SUCCESS
}


/* SUN */
void setdirectmode(unsigned long int &access)
{
    return;
}

void setdirectmode(int &access)
{
    return;
}

/*
* FUNTION NAME : removeFSEntry
*
* DESCRIPTION : 
*
*   This is a lowlevel function of DeleteFSEnt() which takes the fstab entries and
*   the device name on which the operation is performed, deletes the matching 
*   readonly or default entries for this device, comments the maching entries which
*   contain other flags like noexec,nosuid. It leaves the other entries as they are
*
* ALGORITHM : 
*
*   for each entry fsent in fsents
*       if the device name or label matches
*           if the flags contain 'ro' OR only 'defaults' OR only 'rw'
*               skip this entry
*           else
*               comment this entry
*       else write the entry as it is
*
* INPUT PARAMETERS :  
*   
*   device  : name of the device on which action is performed
*   label   : lable of the device on which action is performed
*   fsents  : contains all the entries in the fstab file
*
* OUTPUT PARAMETERS : 
*
*   fsents  : contains the modified entries after the operation is performed
*
* RETURN VALUE : true if succeeds false otherwise.
*/
bool removeFSEntry(const std::string& device, const std::string& label, std::vector<std::string>& fsents)
{
    bool rv=true;
    std::string realDevice = device;
    GetDeviceNameFromSymLink(realDevice);

    std::vector<std::string>::iterator fsent_iter = fsents.begin();
    std::vector<std::string>::iterator fsent_end = fsents.end();
    std::vector<std::string> tempFSEnts;

    for(; fsent_iter != fsent_end; ++fsent_iter)
    {    
        std::string fsent = *fsent_iter;

        svector_t fields;

        Tokenize(fsent, fields, " \t\n");
        if(fields.size() < THRSHOLD_NUM_FIELDSINFSTAB)
        {
            tempFSEnts.push_back(fsent);
            continue;
        }
        std::string fstab_dev = fields[0];
        if ('#' == fstab_dev[0])
        {
            tempFSEnts.push_back(fsent);
            continue;
        }
        std::string fstab_flags = fields[FLAG_FIELD_IN_FSTAB];
        svector_t flags = split(fstab_flags, "," );

        std::string fstab_realdev = fstab_dev;

        if(GetDeviceNameFromSymLink(fstab_realdev) && (fstab_realdev == realDevice))
        {//if the device matches this fstab entry do nothing
            if( (find(flags.begin(), flags.end(), "ro") != flags.end()) ||
                ((find(flags.begin(), flags.end(), "rw") != flags.end()) && (flags.size() == 1))) 
                ;//do not include this entry. equivalent to deleting.
            else
            {
                std::string tempFSEnt = "#" + *fsent_iter;
                tempFSEnts.push_back(tempFSEnt);
            }
        }
        else
        {//if the entry does not match the device or lable we are looking write it as it is.
            tempFSEnts.push_back(fsent);
        }
    } // for loop

    fsents.assign(tempFSEnts.begin(),tempFSEnts.end());

    return rv;
}


bool GetMountPoints(const char * dev, std::vector<std::string> &mountPoints)
{
    bool rv = false;
    FILE * fp = NULL;
    std::string proc_fsname = "";
    struct mnttab *entry = NULL;
	char buffer[MAXPATHLEN*4];
    if (!dev)
    {
        DebugPrintf(SV_LOG_ERROR,
            "Function %s @LINE %d in FILE %s : invalid arguement dev is NULL\n", 
            FUNCTION_NAME, LINE_NO, FILE_NAME);
        return rv;
    }

    do
    {
        fp = fopen(SV_PROC_MNTS, "r");

        if (fp == NULL)
        {
            DebugPrintf(SV_LOG_ERROR,
                "Function %s @LINE %d in FILE %s :failed to open %s \n", 
                FUNCTION_NAME, LINE_NO, FILE_NAME, SV_PROC_MNTS);
            rv = false;
            break;
        }

        std::string sMountPoint="";
        resetmnttab(fp);
        entry = (struct mnttab *) malloc(sizeof (struct mnttab));
        while( SVgetmntent(fp, entry,buffer,sizeof(buffer)) != -1  )
        {
            if (!IsValidDevfile(entry->mnt_special))
                continue;
            proc_fsname = entry->mnt_special;
            std::string proc_realfsname = proc_fsname;
            GetDeviceNameFromSymLink(proc_realfsname);
            std::string realdev = dev;
            std::string device = dev;
            if (!GetDeviceNameFromSymLink(realdev))
            {
                DebugPrintf(SV_LOG_ERROR,
                    "Function %s @LINE %d in FILE %s :GetDeviceNameFromSymLink failed.\n", 
                    FUNCTION_NAME, LINE_NO, FILE_NAME);
                rv = false;
                break;
            }
            if (proc_realfsname == realdev)
            {
                mountPoints.push_back(entry->mnt_mountp);
            }
        }

        rv = true;
    }while(0);

    if ( NULL != fp )
    {
        free (entry);
        resetmnttab(fp);
        fclose(fp);
    }

    return rv;
}


SVERROR MakeReadOnly(const char *drive, void *VolumeGUID, etBitOperation ReadOnlyBitFlag)
{
    DebugPrintf("MakeReadOnly (sun) on %s: not implemented\n", drive);
    return SVE_FAIL;
}


SVERROR isReadOnly(const char * drive, bool & rvalue)
{
    SVERROR sve = SVS_OK;
    DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s, drive = %s\n", LINE_NO, FILE_NAME, drive);

    if( NULL == drive )
    {
        sve = EINVAL;
        DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_ERROR, "FAILED UnhideDrive_RW()... err = EINVAL\n");
        return sve;
    }

    std::string sMount = "";
    std::string mode;
    if (!IsVolumeMounted(drive,sMount,mode))
    {
        DebugPrintf(SV_LOG_DEBUG,"Device %s is not mounted. It means it is hidden .\n",drive);
        rvalue=false;
        return SVS_FALSE ;
    }

    SV_BASIC_VOLINFO basicvolinfo;
    if (!GetBasicVolInfo(drive, &basicvolinfo))
    {
        DebugPrintf(SV_LOG_ERROR, "@LINE %d in FILE %s, GetBasicVolInfo failed for volume %s\n", 
            LINE_NO, FILE_NAME, drive);
        sve = SVE_FAIL;
    }
    else
    {
        rvalue = basicvolinfo.m_breadonly;
    }

    return sve;
}


VOLUME_STATE GetVolumeState(const char * drive)
{    
    DebugPrintf( SV_LOG_DEBUG, "Entered GetVolumeState\n");
    // Algorithm:
    // Get actual device name.
    // Get all current mount points associated with the device.
    // If mount points empty, return HIDDEN
    //   For each mount point...
    //     Is it mounted read write, return UNHIDDEN_RW
    //   
    // Return UNHIDDEN_RO
    //

    bool rv  = false;
    FILE *fp = NULL;
    struct mnttab *entry = NULL;

    bool mounted = false;
    bool rw = false;
    VOLUME_STATE vs = VOLUME_UNKNOWN;
    std::string realdevname = drive;
    std::string devname = drive;
	char buffer[MAXPATHLEN*4];
    do 
    {
        if(!IsVolpackDriverAvailable())
        {
            std::string sparsefile;
            bool new_sparsefile_format = false;
            bool is_volpack = IsVolPackDevice(devname.c_str(),sparsefile,new_sparsefile_format);
            if(is_volpack)
            {
                vs = VOLUME_HIDDEN;
                break;
            }
        }
        rv = GetDeviceNameFromSymLink(realdevname);
        if(!rv)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,realdevname.c_str());
            vs = VOLUME_UNKNOWN;
            break;
        }

        rv = IsValidDevfile(realdevname);
        if(!rv)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :invalid device name %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,realdevname.c_str());
            vs = VOLUME_UNKNOWN;
            break;
        }

        fp = fopen(SV_PROC_MNTS, "r");

        if (fp == NULL)
        {
            DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :unable to open %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,SV_PROC_MNTS);
            vs = VOLUME_UNKNOWN;
            break;
        }
        resetmnttab(fp);
        entry = (struct mnttab *) malloc(sizeof (struct mnttab));
        while(SVgetmntent(fp, entry,buffer,sizeof(buffer)) != -1  )
        {
            std::string procdevName = entry -> mnt_special;
            std::string realprocdevName = procdevName;
            if(!GetDeviceNameFromSymLink(realprocdevName))
                continue;
            if (!IsValidDevfile(realprocdevName))
                continue;
            if (realprocdevName == realdevname)
            {
                mounted = true;
                if( 0 != strstr(entry->mnt_mntopts, "rw") )
                {
                    rw = true;
                    break;
                }
            }
        }
        if ( fp )
        {
            free (entry);
            resetmnttab(fp);
            fclose(fp);
        }

        if(mounted)
        {
            if(rw)
                vs = VOLUME_VISIBLE_RW;
            else
                vs = VOLUME_VISIBLE_RO;
        }
        else
        {
            vs = VOLUME_HIDDEN;
        }

    } while (0);

    DebugPrintf( SV_LOG_DEBUG, "Exited GetVolumeState\n");
    return vs;
}


bool IsVolumeMounted(const std::string volume,std::string& sMountPoint, std::string &mode)
{
    bool bMounted = false;
	char buffer[MAXPATHLEN*4];
    sMountPoint = "";
    if(volume.empty())
    {
        DebugPrintf(SV_LOG_ERROR,"Null volume name passed.\n");
        return false;
    }

    std::string devName = volume;
    std::string realdevName = volume;
    std::string proc_fsname = "";
    GetDeviceNameFromSymLink(realdevName);

    FILE *fp;
    struct mnttab *entry = NULL;
    fp = fopen(SV_PROC_MNTS, "r");
    if (fp == NULL)
    {
        DebugPrintf(SV_LOG_ERROR, "Function %s @LINE %d in FILE %s :unable to open %s\n", FUNCTION_NAME, LINE_NO, FILE_NAME,SV_PROC_MNTS);
        return false;
    }
    resetmnttab(fp);
    entry = (struct mnttab *) malloc(sizeof (struct mnttab));
    while(SVgetmntent(fp, entry,buffer,sizeof(buffer)) != -1  )
    {
        if (!IsValidDevfile(entry->mnt_special))
            continue;
        proc_fsname = entry->mnt_special;
        std::string realproc_fsname = proc_fsname;
        GetDeviceNameFromSymLink(realproc_fsname);
        if (realproc_fsname == realdevName)
        {
            bMounted = true;
            sMountPoint = entry->mnt_mountp;
        }
    }
    if ( fp )
    {
        free (entry);
        resetmnttab(fp);
        fclose(fp);
    }
    return bMounted;
}



SVERROR FlushFileSystemBuffers(const char *Volume)
{
    std::string volName;
    ACE_HANDLE hVolume = ACE_INVALID_HANDLE;
    SVERROR sve = SVS_OK;

    do
    {
        SV_UINT flags = 0;
        volName = Volume;

        flags = O_RDWR | O_SYNC | O_RSYNC ;

        // PR#10815: Long Path support
        hVolume = ACE_OS::open(getLongPathName(volName.c_str()).c_str(), flags, ACE_DEFAULT_OPEN_PERMS);

        if(ACE_INVALID_HANDLE == hVolume)
        {
            sve = ACE_OS::last_error();
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "ACE_OS::open failed in FlushFileSystemBuffers.We still proceed %s, err = %s\n", volName.c_str(), sve.toString());
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



bool GetVolumeRootPath(const std::string & path, std::string & root)
{
    //    char rootPath[SV_INTERNET_MAX_PATH_LENGTH];
    struct mnttab *mntptr;
    size_t lenthOfmntdir = 0;
    bool found = false;
	char buffer[MAXPATHLEN*4];
    root.clear();

    FILE *fp = fopen(SV_PROC_MNTS,"r");
    if(fp == NULL)
        return false;
    resetmnttab(fp);
    mntptr = (struct mnttab *) malloc(sizeof (struct mnttab));
    while(SVgetmntent(fp, mntptr,buffer,sizeof(buffer)) != -1  )
    {
        lenthOfmntdir = strlen(mntptr->mnt_mountp);
        if(!strncmp(mntptr->mnt_mountp, path.c_str(), lenthOfmntdir)) {
            if ( lenthOfmntdir > root.size()) {
                root = mntptr->mnt_mountp;
                found = true;
            }
        }
    }
    if (fp)
    {
        free(mntptr);
        resetmnttab(fp);
        fclose(fp);
    }

    return found;
}



//
// Function: GetDeviceNameForMountPt
//  given a mount point, convert it to corresponding device name
//
bool GetDeviceNameForMountPt(const std::string & mtpt, std::string & devName)
{
    bool rv = false;
	char buffer[MAXPATHLEN*4];
    FILE *fp;
    struct mnttab *entry = NULL;

    fp = fopen(SV_PROC_MNTS, "r");

    if (fp == NULL)
    {
        DebugPrintf(SV_LOG_ERROR, "Could not able to open %s\n", SV_PROC_MNTS);
        return false;
    }
    resetmnttab(fp);
    entry = (struct mnttab *) malloc(sizeof (struct mnttab));
    while(SVgetmntent(fp, entry,buffer,sizeof(buffer)) != -1 )
    {
        if ( 0 == strcmp(entry->mnt_mountp, mtpt.c_str()))
        {
            devName = entry->mnt_special;
            if(IsValidDevfile(entry->mnt_special))
            {
                /** NEEDTOASK : Whether to return device name or
                * real device name?  This is very important
                * as We need to determine which areas get
                * affected because of real name and device name
                * are different. Mostly retention and other areas
                * Currently returning what ever is there in 
                * mnttab. This strongly assumes that mnttab
                * must have "/dev/dsk/c0t1d0s7", otherwise
                * This function may cause erroneous results
                * Need to check if cdpcli needs a real name
                * or name reported to CX */
                std::string realdevName = devName;
                if(!GetDeviceNameFromSymLink(realdevName))
                    continue;
            }
            rv = true;
            break;
        }
    }
    if ( fp )
    {
        free(entry);
        resetmnttab(fp);
        fclose(fp);
    }

    return rv;
}



bool IsValidMntEnt(struct mnttab *entry)
{
    bool valid = true;

    if (entry == NULL) valid = false;

    if (strcmp(entry->mnt_special, "none") == 0) valid = false;
    if (strncmp(entry->mnt_mountp, "/dev", strlen("/dev")) == 0) valid = false;
    if (strcmp(entry->mnt_fstype, "nfs") == 0) valid = false;
    if (strcmp(entry->mnt_fstype, "auto") == 0) valid = false; // all removable media
    if (strncmp(entry->mnt_mountp, "/proc", strlen("/proc")) == 0) valid = false;

    return valid;
}


/*
* FUNCTION NAME :     mountedVolume
*
* DESCRIPTION :   Given a pathname, return the nested volumes beneath it
*                  
*
* INPUT PARAMETERS :  pathname under which nested volumes need to be checked
*
* OUTPUT PARAMETERS :  nested volumes
*
* NOTES : doesn't count the provided mountpoint/devicename as a mounted volume
*
* return value : true if nested volumes are found, otherwise false
*
*/
bool mountedvolume( const std::string& pathname, std::string& mountpoints )
{
    std::string mntpath = pathname + "/";
	char buffer[MAXPATHLEN*4];
    FILE* fp = fopen(SV_PROC_MNTS, "r");
    if( !pathname.empty() )
    {
        struct mnttab* line = NULL;
        resetmnttab(fp);
        line = (struct mnttab *) malloc(sizeof (struct mnttab)); 
        while(SVgetmntent(fp, line,buffer,sizeof(buffer))!= -1 )
        {
            if( !strncmp( line->mnt_special, "/dev/", 5 ) && strcmp( line->mnt_mountp, pathname.c_str()))
            {
                if( !strncmp( line->mnt_mountp, mntpath.c_str(), mntpath.size()))
                {    
                    mountpoints += line->mnt_mountp;
                    mountpoints += "\n";
                }    
            }
        }
        if ( NULL != fp )
        {
            free(line);
            resetmnttab(fp);
            fclose(fp);
        }
    }
    return mountpoints.empty() ? false : true;
}


bool FormVolumeLabel(const std::string device, std::string &label)
{
    bool bretval = true;


    label = ""; 
    return bretval;
}


std::string GetCommandPath(const std::string &cmd)
{
    /* APART FROM COMMON PATHS, WE SHOULD FIND AN EQUIVALENT OF "which" COMMAND */
    return "/bin:/sbin:/usr/bin:/usr/local/bin:/usr/sbin";
}


bool GetVolSizeWithOpenflag(const std::string vol, const int oflag, SV_ULONGLONG &refvolsize)
{
    bool bretval = true;
	std::string dev_name = vol + "s2";
	ACE_stat s;
	if(0 == sv_stat(getLongPathName(vol.c_str()).c_str(), &s ))
    	refvolsize = GetRawVolSize(vol);
	else if(0 == sv_stat(getLongPathName(dev_name.c_str()).c_str(), &s ))
	{
		char symlnk[PATH_MAX + 1] = {0};
		ssize_t len;
		len =  readlink(dev_name.c_str(), symlnk, sizeof(symlnk) - 1);
		std::string linkname = symlnk;
		linkname.erase(len - 2);
		linkname += ",raw";
		refvolsize = GetRawVolSize(linkname);
	}

    if (!refvolsize)
    {
        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, GetRawVolSize failed for volume %s\n", LINE_NO, FILE_NAME, vol.c_str());
        bretval = false;
    }

    return bretval;
}


void FormatVolumeNameForCxReporting(std::string& volumeName)
{
    // we need to strip off any leading \, ., ? if they exists
    std::string::size_type idx = volumeName.find_first_not_of("\\.?");
    if (std::string::npos != idx) {
        volumeName.erase(0, idx);
    }

    // strip off trailing :\ or : if they exist
    std::string::size_type len = volumeName.length();
    idx = len;
    if ('\\' == volumeName[len - 1] || ':' == volumeName[len - 1]) {
        --idx;
    }

    if (idx < len) {
        volumeName.erase(idx);
    }
}


bool GetSectorSizeWithFd(int fd, const std::string vol, SV_ULONG &SectorSize)
{
    bool bretval = false;
    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
    }
    else
    {
        SVERROR sv;
        unsigned int volsectorsize = 0;
        sv = GetRawVolumeSectorSize(vol.c_str(), &volsectorsize);
        if (sv.failed())
        {
            DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, failed to get sector size for volume = %s\n", LINE_NO, FILE_NAME, vol.c_str()); 
        }
        else
        {
            SectorSize = volsectorsize;
            bretval = true;
        }
    }

    return bretval;
}


bool GetNumBlocksWithFd(int fd, const std::string vol, SV_ULONGLONG &NumBlks)
{
    bool bretval = false;
    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid fd = %d\n", LINE_NO, FILE_NAME, fd); 
    }
    else
    {
        SV_BASIC_VOLINFO basicvolinfo;
        if (!GetBasicVolInfo(vol, &basicvolinfo))
        {
            DebugPrintf(SV_LOG_ERROR, "@LINE %d in FILE %s, GetBasicVolInfo failed for volume %s\n", 
                LINE_NO, FILE_NAME, vol.c_str());
        }
        else
        {
            NumBlks = basicvolinfo.m_lnumblocks;
            bretval = true;
        }
    }

    return bretval;
}


/* SUN SPECIFIC */
/* The reason is that in linux, the device name
* is although a link to the real name, read link
* on device name gives absolute path of the real name
* but in solaris, readlink on device name gives the
* relative path. This function can be made common
* to unix once the solaris version(which should work
* for both solaris and linux) is tested on linux)
*/

//
// Gets the root device name from a symbolic link
//
bool GetDeviceNameFromSymLink(std::string &deviceName)
{
    bool bretval = false;
    const std::string saveDeviceName = deviceName;
    struct stat64 st_buf;

    if (NULL == deviceName.c_str()) 
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, no device name supplied\n", LINE_NO, FILE_NAME);
    }
    else if(stat64(deviceName.c_str(), &st_buf))
    {
        DebugPrintf(SV_LOG_WARNING, "@ LINE %d in FILE %s, stat failed on %s with errno = %d\n", LINE_NO, FILE_NAME, deviceName.c_str(), errno);
    } 
    else
    {
        /**
        *
        * Commenting below debug printf since it fills in the log file. 
        * Can be uncommented when needed.
        *
        */
        /* DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with deviceName = %s\n",FUNCTION_NAME, deviceName.c_str()); */
        const char UNIXROOTDIR = '/';
        const char UNIXDIRSEPARATOR = '/';

        // make sure device is valid
        struct stat64 volStat;

        // make sure device file exists
        if (0 == lstat64(deviceName.c_str(), &volStat)) 
        {
            if (S_ISLNK(volStat.st_mode)) 
            {
                ssize_t len;
                /* Not initialized since terminating finally with \0 */
                char symlnk[PATH_MAX + 1]; 
                do 
                {
                    len = readlink(deviceName.c_str(), symlnk, sizeof(symlnk) - 1);
                    if (-1 == len) 
                    {
                        /* This is the final device. so return true */
                        if (0 == lstat64(deviceName.c_str(), &volStat)) 
                        {
                            bretval = true;
                        }
                        break;
                    }
                    symlnk[len] = '\0';
                    if (UNIXROOTDIR != symlnk[0])
                    {
                        /* This is a relative path */
                        char dirnameofdevice[PATH_MAX + 1] = {'\0'};
                        SVERROR sv = DirName(deviceName.c_str(), dirnameofdevice);
                        if (sv.failed())
                        {
                            DebugPrintf(SV_LOG_ERROR,"DirName failed @ LINE %d in FILE %s.\n", LINE_NO, FILE_NAME);
                            break;
                        }
                        std::string catenatedlinkname = dirnameofdevice;
                        catenatedlinkname +=  UNIXDIRSEPARATOR;
                        catenatedlinkname += symlnk;
                        char absolutename[PATH_MAX + 1] = {'\0'};
                        if (NULL == realpath(catenatedlinkname.c_str(), absolutename))
                        {
                            DebugPrintf(SV_LOG_ERROR,"realpath failed @ LINE %d in FILE %s for file %s with errno = %d.\n", LINE_NO, FILE_NAME, 
                                deviceName.c_str(), errno);
                            break;
                        }
                        deviceName = absolutename;
                    }
                    else
                    {
                        /* symlink is in absolute form */
                        deviceName = symlnk;
                    }
                } while (true);
            }
            else 
            {
                /* This is not a link file */
                bretval = true;
            }
        } 
    } /* else */

    if (!bretval)
    {
        DebugPrintf(SV_LOG_WARNING, "Unable to determine attributes of file  %s @LINE %d in FILE %s\n",saveDeviceName.c_str(),
            LINE_NO, FILE_NAME);
    }

    /**
    *
    * Commenting below debug printf since it fills in the log file. 
    * Can be uncommented when needed.
    *
    */
    /* DebugPrintf(SV_LOG_DEBUG,"EXITING %s with deviceName = %s and bretval = %s\n",FUNCTION_NAME, deviceName.c_str(), bretval?"true":"false"); */
    return bretval;
}


/* This function assumes name coming is of form "/dev/dsk/c0t1d0s7" or "/dev/md/dsk/d20" */
bool GetBasicVolInfo(const std::string &sDevName, SV_BASIC_VOLINFO *pbasicvolinfo)
{
    bool bretval = false;

    if (!pbasicvolinfo)
    {
        DebugPrintf(SV_LOG_ERROR, "@ LINE %d in FILE %s, invalid arguement pbasicvolinfo = %p\n", 
            LINE_NO, FILE_NAME, pbasicvolinfo);
        return bretval;
    }


    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(g_lockforgetbasicvolinfo);

    if (!guard.locked())
    {
        DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s unable to apply lock\n", __LINE__, __FILE__);
        return bretval;
    }

    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with deviceName = %s\n",FUNCTION_NAME, sDevName.c_str());

    std::string sRawVolName = GetRawVolumeName(sDevName);
    int fd;
    int read_vtoc_retval = 0;
    struct vtoc vt;

    fd = open(sRawVolName.c_str(), O_NDELAY | O_RDONLY);

    if (-1 == fd)
    {
        DebugPrintf(SV_LOG_ERROR, "open() failed for raw volume %s at line %d in file %s with errno = %d\n", sRawVolName.c_str(),
            __LINE__, __FILE__, errno);
    }
    else
    {
        read_vtoc_retval = read_vtoc(fd, &vt);

        if (read_vtoc_retval < 0)
        {
#ifndef SV_SUN_5DOT8
            if (VT_ENOTSUP == read_vtoc_retval)
            {
                if (GetBasicVolInfoFromEFI(fd, sDevName, pbasicvolinfo))
                {
                    DebugPrintf(SV_LOG_DEBUG, "reading efi label succeeded for raw volume %s at line %d in file %s\n", sRawVolName.c_str(),
                        __LINE__, __FILE__);
                    bretval = true; 
                }
                else
                {
                    DebugPrintf(SV_LOG_ERROR, "reading efi label failed for raw volume %s at line %d in file %s with errno = %d\n", sRawVolName.c_str(),
                        __LINE__, __FILE__, errno);
                }
            }
            else
            {
#endif /* SV_SUN_5DOT8 */
                DebugPrintf(SV_LOG_ERROR, "read_vtoc() failed for raw volume %s at line %d in file %s with errno = %d\n", sRawVolName.c_str(),
                    __LINE__, __FILE__, errno);
#ifndef SV_SUN_5DOT8
            }
#endif /* SV_SUN_5DOT8 */
        }
        else if (READVTOCRVALFORP0 == read_vtoc_retval)
        {
            DebugPrintf(SV_LOG_DEBUG, "For %s, read vtoc returned %d." 
                                      "This is a fdisk partition p0\n", 
                                      sDevName.c_str(), read_vtoc_retval);
            bretval = GetBasicVolInfoForP0(fd, sDevName, pbasicvolinfo);
        }
        else if (read_vtoc_retval < vt.v_nparts)
        {
            DebugPrintf(SV_LOG_DEBUG, "read_vtoc returned %d for raw volume %s at line %d in file %s\n", 
                                      read_vtoc_retval, sRawVolName.c_str(), __LINE__, __FILE__);
            pbasicvolinfo->m_breadonly = (vt.v_part[read_vtoc_retval].p_flag & V_RONLY);
            pbasicvolinfo->m_lnumblocks = vt.v_part[read_vtoc_retval].p_size;
            pbasicvolinfo->m_uhsectorsz = vt.v_sectorsz ? vt.v_sectorsz : DEV_BSIZE;
            /* pbasicvolinfo->m_ullrawsize = GiveValidSize(sDevName, pbasicvolinfo->m_lnumblocks, pbasicvolinfo->m_uhsectorsz, E_ATMORETHANSIZE); */
            pbasicvolinfo->m_ullrawsize = ((unsigned long long)pbasicvolinfo->m_lnumblocks) * ((unsigned long long)pbasicvolinfo->m_uhsectorsz);

            bretval = true;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "read_vtoc returned %d for raw volume %s which "
                                      "is greater than the number of partition structures\n",
                                      read_vtoc_retval, sDevName.c_str());
        }
        close(fd);
    }

    DebugPrintf(SV_LOG_DEBUG,"EXITING %s with bretval = %s\n",FUNCTION_NAME, bretval?"true":"false");
    return bretval;
}


bool IsReportingRealNameToCx(void)
{
    return false;
}



/**
*
* Finds out and gets the link name from actual device name
* for only physical disks since does cfgadm
* The actual device name for 
* solaris volume manager are not documented 
* and so we should use solaris volume manager name d[0-9]+
* for the cdpcli input. 
* Also ls -l on a solaris volume 
* manager volume gives:
* root@IMITSF1 # ls -l /dev/md/dsk/d50
* lrwxrwxrwx 1 root root 37 Mar  3 17:54 /dev/md/dsk/d50 -> ../../../devices/pseudo/md@0:0,50,blk
* root@IMITSF1 #
*
*/

bool GetLinkNameIfRealDeviceName(const std::string sVolName, std::string &sLinkName)
{
    bool bretval;
    /** 
    * This will contain the actual device name when 
    * GetDeviceNameFromSymLink is called on sVolName
    * So that we always have realname for comparision
    * We compare with real name
    */
    std::string realname;



    /** 
    * Assign the volume name as the first step
    */
    realname = sLinkName = sVolName;
    bretval = true;

    if (!GetDeviceNameFromSymLink(realname))
    {
        return !(bretval); 
    }

    if (realname == sVolName)
    {
        /**
        *
        * realname and the sVolName are same
        * This means sVolName is a real name.
        * Always support to get the link name
        * of the physical disk slice since
        * solaris volume manager volume has
        * a link or not is not documented.
        * and VxVM volumes under "/dev/vx/dsk"
        * do not have any links. 
        *
        */

        /**
        *
        * The algorithm:
        * ============= 
        * 1. Do 
        *
        */

        DIR *dirp;
        struct dirent *dentp, *dentresult;
        size_t direntsize = sizeof(struct dirent) + PATH_MAX + 1;
        const std::string DskDir = "/dev/dsk";



        dentresult = NULL;

        dirp = opendir(DskDir.c_str());

        if (dirp)
        {
            dentp = (struct dirent *)calloc(direntsize, 1);

            if (dentp)
            {
                while ((0 == readdir_r(dirp, dentp, &dentresult)) && dentresult)
                {
                    if (strcmp(dentp->d_name, ".") &&
                        strcmp(dentp->d_name, ".."))                     //skip . and ..
                    {
                        std::string dskdevice = DskDir + "/" + dentp->d_name;
                        std::string dskdevicerealname = dskdevice;

                        if (GetDeviceNameFromSymLink(dskdevicerealname) && 
                            (dskdevicerealname == sVolName))
                        {
                            sLinkName = dskdevice; 
                            break;
                        }
                    } /* end of skipping . and .. */
                    memset(dentp, 0, direntsize);
                } /* end of while readdir_r */
                free(dentp);
            } /* endif of if (dentp) */
            else
            {
                bretval = false;
            }
            closedir(dirp); 
        } /* end of if (dirp) */
    } /* end of if (realname == sVolName) */

    return bretval;
}


#ifndef SV_SUN_5DOT8
bool GetBasicVolInfoFromEFI(int fd, const std::string &dskdevice, SV_BASIC_VOLINFO *pbasicvolinfo)
{
    bool bretval = false;
    std::string sRawVolName = GetRawVolumeName(dskdevice);



    if (!pbasicvolinfo)
    {
        return bretval;
    }

    int efi_alloc_and_read_rval = -1;  
    dk_gpt_t *vt = NULL;
    void *libefihandle = NULL;
    int (*ptr_efi_alloc_and_read)(int fd, dk_gpt_t **vtoc) = NULL;
    void (*ptr_efi_free)(dk_gpt_t *vtoc) = NULL;
    void *pv_efi_alloc_and_read = NULL, *pv_efi_free = NULL;
    std::string libefiso;

    libefiso = LIBEFI_DIRPATH;
    libefiso += UNIX_PATH_SEPARATOR;
    libefiso += LIBEFINAME;
    DebugPrintf(SV_LOG_DEBUG, "@LINE %d in FILE %s libefiso path is %s\n", __LINE__, __FILE__, libefiso.c_str());
    libefihandle = dlopen(libefiso.c_str(), RTLD_LAZY);

    if (libefihandle)
    {
        const std::string EFI_ALLOC_AND_READ_FUNCTIONNAME = "efi_alloc_and_read";
        const std::string EFI_FREE_FUNCTIONNAME = "efi_free";

        pv_efi_alloc_and_read = dlsym(libefihandle, EFI_ALLOC_AND_READ_FUNCTIONNAME.c_str());
        pv_efi_free = dlsym(libefihandle, EFI_FREE_FUNCTIONNAME.c_str());
        if (pv_efi_alloc_and_read && pv_efi_free)
        {
            ptr_efi_alloc_and_read = (int (*)(int fd, dk_gpt_t **vtoc))pv_efi_alloc_and_read;
            ptr_efi_free = (void (*)(dk_gpt_t *vtoc))pv_efi_free;
            efi_alloc_and_read_rval = ptr_efi_alloc_and_read(fd, &vt);

            if ((efi_alloc_and_read_rval >= 0) && vt)
            {
                /**
                *
                *
                * When a disk is under disk set of solaris volume manager,
                * efi_version is coming as 0, but the structure is filled
                * in properly. 
                *
                * NOTE: Keep the commented check of EFI_VERSION_CURRENT
                * as it is.
                */
                //if (EFI_VERSION_CURRENT == vt->efi_version)
                //{
                if (FULLEFIIDX == efi_alloc_and_read_rval)
                {
                    DebugPrintf(SV_LOG_DEBUG,"@ LINE %d in FILE %s, %s is a full efi label disk\n", LINE_NO, FILE_NAME, sRawVolName.c_str());

                    /** 
                    *
                    * For EFI entire disk, should not use the efi_alloc_and_read_rval since
                    * has no significance. But still efi_alloc_and_read_rval is positive
                    * and the dk_gpt_t structure is properly filled in.
                    * currently putting read only as false for EFI labelled full disk since
                    * need to find out a way of finding it out.
                    *
                    */

                    pbasicvolinfo->m_breadonly = false;
                    pbasicvolinfo->m_lnumblocks = vt->efi_last_lba + 1;
                    pbasicvolinfo->m_uhsectorsz = vt->efi_lbasize;
                    pbasicvolinfo->m_ullrawsize = GiveValidSize(sRawVolName,
                                                  pbasicvolinfo->m_lnumblocks,
                                                  pbasicvolinfo->m_uhsectorsz,
                                                  E_ATLESSTHANSIZE);
                }
                else
                {
                    DebugPrintf(SV_LOG_DEBUG,"@ LINE %d in FILE %s, %s is a partition on efi label disk\n", LINE_NO, FILE_NAME, sRawVolName.c_str());
                    pbasicvolinfo->m_breadonly = (vt->efi_parts[efi_alloc_and_read_rval].p_flag & V_RONLY);
                    pbasicvolinfo->m_lnumblocks = vt->efi_parts[efi_alloc_and_read_rval].p_size;
                    pbasicvolinfo->m_ullrawsize = ((unsigned long long)vt->efi_parts[efi_alloc_and_read_rval].p_size) *
                        ((unsigned long long)vt->efi_lbasize);
                    pbasicvolinfo->m_uhsectorsz = vt->efi_lbasize;
                }
                bretval = true;

                //}
                //else
                //{
                //    DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, vt->efi_version is not EFI_VERSION_CURRENT\n", LINE_NO, FILE_NAME);
                //}

                ptr_efi_free(vt);

            } /* end of if ((efi_alloc_and_read_rval >= 0) && vt) */
            else
            {
                DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s efi_alloc_and_read on %s failed\n", __LINE__, __FILE__, 
                    sRawVolName.c_str()); 
            }
        } /* end if of  if (pv_efi_alloc_and_read && pv_efi_free) */
        else
        {
            DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s efi_free or efi_alloc_and_read are not present in libefi.so\n", 
                __LINE__, __FILE__);
        }
        dlclose(libefihandle);
    } /* end of if (libefihandle) */
    else
    {
        DebugPrintf(SV_LOG_WARNING, "@LINE %d in FILE %s libefi.so is not present on system\n", 
            __LINE__, __FILE__);
    }

    return bretval;
}


bool IsFullEFIRawDisk(const char *disk)
{
    bool bretval = false;



    if (disk)
    {
        const std::string RDSKPREFIX = "/dev/rdsk/";
        if (!strncmp(disk, RDSKPREFIX.c_str(), strlen(RDSKPREFIX.c_str())))
        {
            size_t len = strlen(disk);
            const char *p = disk + (len - 1);

            while (p >= disk)
            {
                if (isdigit(*p))
                {
                    p--;
                }
                else
                {
                    bretval = ('d' == *p)?true:false;
                    break;
                }
            }
        }
    }

    return bretval;
}
#endif /* SV_SUN_5DOT8 */


std::string GetRawVolumeName(const std::string &dskname)
{
    std::string rdskname = dskname;
    boost::algorithm::replace_first(rdskname, DSKDIR, RDSKDIR);
    boost::algorithm::replace_first(rdskname, DMPDIR, RDMPDIR);
    return rdskname;
}


std::string GetBlockVolumeName(const std::string &rdskname)
{
    std::string dskname = rdskname;
    boost::algorithm::replace_first(dskname, RDSKDIR, DSKDIR);
    boost::algorithm::replace_first(dskname, RDMPDIR, DMPDIR);
    return dskname;
}


/* SUN */

bool IsProcessRunning(const std::string &sProcessName)
{
    const char *procdir = "/proc";  /* standard /proc directory */
    bool rv = false;
    psinfo_t info; /* process information structure from /proc */
    DIR *dirp;
    size_t pdlen;
    char psname[PSNAMELEN];
    struct dirent *dentp;
    int cnt = 0;

    do
    {
        if ((dirp = opendir(procdir)) == NULL)
        {
            printf("Failed Opening proc directory \n");
            rv = false;
            break;
        }

        (void) inm_strcpy_s(psname, ARRAYSIZE(psname), procdir);
        pdlen = strlen(psname);
        psname[pdlen++] = '/';

        /* for each active process --- */
        while (dentp = readdir(dirp))
        {
            int psfd;                           /* file descriptor for /proc/nnnnn/stat */

            if (dentp->d_name[0] == '.')        /* skip . and .. */
                continue;

            (void) inm_strcpy_s(psname + pdlen, ARRAYSIZE(psname) - pdlen, dentp->d_name);
            (void) inm_strcat_s(psname, ARRAYSIZE(psname), "/psinfo");

            if ((psfd = open(psname, O_RDONLY)) == -1)
                continue;

            memset(&info, 0, sizeof info);
            ssize_t bytesread = read(psfd, &info, sizeof (info));
            (void) close(psfd);
            if (bytesread == sizeof (info))
            {
                if (0 == strcmp(sProcessName.c_str(), info.pr_fname))
                {
                    cnt++;
                    if (cnt > 1)
                    {
                        rv = true;
                        break;
                    }
                }
            }
        }
        closedir(dirp);
    }while(0);

    return rv;
}


unsigned long long GetSizeThroughRead(int fd, const std::string &device)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s with fd = %d and device = %s\n", FUNCTION_NAME, fd, device.c_str());
    char            buf[DEV_BSIZE];
    diskaddr_t      read_failed_from_top = 0;
    diskaddr_t      read_succeeded_from_bottom = 0;
    diskaddr_t      current_file_position;
    unsigned long long sizefromread = 0;
    off_t lseekrval = 0;
    int fd_dsk = -1; 
    
    if (((lseek(fd, (offset_t)0, SEEK_SET)) == -1) || ((read(fd, buf, DEV_BSIZE)) == -1))
    {
        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, lseek failed for device %s with errno = %d\n", LINE_NO, FILE_NAME, device.c_str(), errno);
        return 0;  
    }

    for (current_file_position = NUM_SECTORS_PER_TB * 4; read_failed_from_top == 0 && current_file_position < SIZE_LIMIT_TOP; current_file_position += 4 * NUM_SECTORS_PER_TB) 
    {
        if (((lseek(fd, (offset_t)(current_file_position * DEV_BSIZE), SEEK_SET)) == -1) || ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
        {
            read_failed_from_top = current_file_position;
        }
        else
        {
            read_succeeded_from_bottom = current_file_position;
        }
    }

    if (read_failed_from_top == 0)
    {
        DebugPrintf(SV_LOG_ERROR,"@ LINE %d in FILE %s, read failed for device %s with errno = %d\n", LINE_NO, FILE_NAME, device.c_str(), errno);
        return 0;
    }

    while (read_failed_from_top - read_succeeded_from_bottom > 1) 
    {
        current_file_position = read_succeeded_from_bottom + (read_failed_from_top - read_succeeded_from_bottom)/2;
        if (((lseek(fd, (offset_t)(current_file_position * DEV_BSIZE), SEEK_SET)) == -1) || ((read(fd, buf, DEV_BSIZE)) != DEV_BSIZE))
        {
            read_failed_from_top = current_file_position;
        }
        else
        {
            read_succeeded_from_bottom = current_file_position;
        }
    }

    sizefromread = (((unsigned long long)(read_succeeded_from_bottom + 1)) * ((unsigned long long)DEV_BSIZE));

    /*
    DebugPrintf(SV_LOG_DEBUG,"@ LINE %d in FILE %s, size of device %s from read = %llu\n", 
                             LINE_NO, FILE_NAME, device.c_str(), sizefromread);
    */

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s with size = %llu for device %s\n",FUNCTION_NAME, sizefromread, device.c_str()); 
    return sizefromread;
}


/* SUN */
bool MountDevice(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly, int &exitCode, 
                 std::string &errorMsg)
{
    if ( mountPoint.empty())
    {
        std::stringstream strerr;
        strerr << "Empty mountpoint specified.";
        strerr << "Cannot mount device " << device << '\n';
        DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
        errorMsg = strerr.str();
        return false;    
    }

    std::string errmsg;
    if(!IsValidMountPoint(mountPoint,errmsg))
    {
        std::stringstream strerr;
        strerr << "Cannot mount device " << device << " on mount point " << mountPoint << ". " << errmsg << "\n";
        DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
        errorMsg = strerr.str();
        return false;
    }

    if(SVMakeSureDirectoryPathExists(mountPoint.c_str()).failed())
    {
        std::stringstream strerr;
        strerr << "Failed to create mount point directory " << mountPoint
            << '\n';
        DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
        errorMsg = strerr.str();
        return false;
    }

    bool bCanMount = false;
    bool remount   = false;
    std::string flag;

    if(bSetReadOnly)
        flag="ro ";
    else
        flag="rw ";


    std::string sFileSystem = fsType;
    if(sFileSystem == "")
    {
        /* NEEDTOASK:
        * Linux handles this situation by specifying
        * auto as the filesystem. There is not equivalent
        * of auto in solaris. No manual page mentions about autofs
        * being similar to auto filesystem type in linux.
        * (linux manual page recommends against use of auto)
        * and even if we mount using autofs, it gets mounted
        * but ls on mountpoint hangs
        * sFileSystem = "autofs"; */

        std::stringstream strerr;
        strerr << "The filesystem supplied to mount is empty.";
        strerr << "Hence cannot mount device " << device << '\n';
        DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
        errorMsg = strerr.str();
        return false;    
    }

    std::string flags = flag;   //This is used only in case of Uncomment FSTAB. flag gets updated with -o remount where as flags doesn't.
    std::string sMount = "";
    std::string mode;
    if ( IsVolumeMounted(device,sMount,mode))
    {
        if ( sMount == mountPoint) //need to check if mode is same or not. If the mode is same, we will say already mounted, else we will do ewmount
        {
            if ("ufs" == sFileSystem)
            {
                bCanMount = true; 
                flag+="-o remount ";
            }
            else
            {
                std::stringstream strerr;
                strerr << "the device " << device 
                    << " is already mounted on " << sMount << '\n';
                DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
                errorMsg = strerr.str();
                return false;
            }
        }
        else
        {
            bCanMount = false;
            std::stringstream strerr;
            strerr << "Failed to unhide volume "
                << "since already visible on mount point " << sMount
                << " and requested mount point is " << mountPoint << '\n';
            DebugPrintf(SV_LOG_ERROR,"%s", strerr.str().c_str());
            errorMsg = strerr.str();
            return false;
        }
    }
    else /* volume is  not mounted */
    {    
        bCanMount = true;    
    }

    if ( bCanMount)
    {
        sFileSystem = ToLower(sFileSystem);
        if(!strcmp(sFileSystem.c_str(),"fat32") ||
            !strcmp(sFileSystem.c_str(),"fat")   ||
            !strcmp(sFileSystem.c_str(), "vfat"))
        {
            sFileSystem = "pcfs";
        }

        std::string fsmount;

        /* mount -F pcfs -o rw special mntpoint */
        fsmount += MOUNT_COMMAND;
        fsmount += " ";
        fsmount += OPTION_TO_SPECIFY_FILESYSTEM;
        fsmount += " ";
        fsmount += sFileSystem.c_str();
        fsmount += " -o ";
        fsmount += flag ;
        fsmount += " ";
        fsmount += device.c_str() ;
        fsmount += " ";
        fsmount += "\"";
        fsmount += mountPoint.c_str() ;
        fsmount += "\"";

        DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s THE MOUNT COMMAND FORMED is %s\n", LINE_NO, FILE_NAME, fsmount.c_str());

        std::string fsck_replay;
        std::string fsck_full;

        // For vxfs filesystems, we need to run fsck 
        // with log replay when trying to perform mount
        // on a non frozen source image (ie without issuing a tag)

        // log replay: fsck -F vxfs -y special 
        fsck_replay += FSCK_COMMAND;
        fsck_replay += " ";
        fsck_replay += FSCK_FS_OPTION ;
        fsck_replay += " ";
        fsck_replay += sFileSystem.c_str();
        fsck_replay += " -y ";
        fsck_replay += device.c_str() ;

        // full fsck: fsck -F vxfs -y  -o full,nolog special
        fsck_full += FSCK_COMMAND;
        fsck_full += " ";
        fsck_full += FSCK_FS_OPTION ;
        fsck_full += " ";
        fsck_full += sFileSystem.c_str();
        fsck_full += " -y ";
        fsck_full += FSCK_FULL_OPTION ;
        fsck_full += " ";
        fsck_full += device.c_str() ;


        if(flag!=flags)
            remount=true;

        int retriesformount = 0;
        bool mounted = false; 
        bool fsck_replay_done = false;
        bool fsck_full_done = false;

        // fsck needs to be done only for vxfs when performing
        // read write mount. in other cases, we will set
        // fsck done as true so it does not get executed
        if(bSetReadOnly || strcmp(sFileSystem.c_str(),"vxfs"))
        {
            fsck_replay_done = true;
            fsck_full_done = true;
        }

        do
        {
            DebugPrintf(SV_LOG_INFO, "executing %s...\n", fsmount.c_str());
            mounted = ExecuteInmCommand(fsmount, errorMsg, exitCode);
            if (mounted)
            {
                return true;
            } 
            else
            {
                DebugPrintf(SV_LOG_DEBUG, 
                    "mount %s with filesystem %s failed on %s with error message = %s. Retrying again\n",
                    device.c_str(),sFileSystem.c_str(),mountPoint.c_str(), errorMsg.c_str());
            }

            retriesformount++;
            if(!fsck_replay_done)
            {
                DebugPrintf(SV_LOG_INFO, "executing %s...\n", fsck_replay.c_str());
                if(!ExecuteInmCommand(fsck_replay, errorMsg, exitCode))
                {
                    DebugPrintf(SV_LOG_INFO,
                        "%s failed with exit code (%d) error message = %s.\n",
                        fsck_replay.c_str(), exitCode, errorMsg.c_str());
                }            
                fsck_replay_done = true;
                continue;
            }

            if(!fsck_full_done)
            {
                DebugPrintf(SV_LOG_INFO, "executing %s...\n", fsck_full.c_str());
                if(!ExecuteInmCommand(fsck_full, errorMsg, exitCode))
                {
                    DebugPrintf(SV_LOG_INFO,
                        "%s failed with exit code (%d) error message = %s.\n",
                        fsck_full.c_str(), exitCode, errorMsg.c_str());
                }
                fsck_full_done = true;
                continue;
            }

        } while (retriesformount < RETRIES_FOR_MOUNT);

        DebugPrintf(SV_LOG_ERROR, 
            "mount %s with filesystem %s failed on %s with error message = %s\n",
            device.c_str(),sFileSystem.c_str(),mountPoint.c_str(), errorMsg.c_str());

        return false;
    }
    return true;
}


std::string GetExecdName(void)
{
    std::string ExecdName;    

    psinfo_t info;
    memset(&info, 0, sizeof info);
    pid_t pid = getpid();
    std::stringstream proccessfile;
    proccessfile << "/proc/" << pid << "/psinfo";
    int psfd;           
    do
    {
        if ((psfd = open(proccessfile.str().c_str(), O_RDONLY)) == -1)
            break;
        if (read(psfd, &info, sizeof (info)) == sizeof (info))
        {
            ExecdName = info.pr_fname;
        }
        (void) close(psfd);
    }while(false);
    return ExecdName;
}


bool IsNonGlobalZone(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    bool bisnonglobal = false;

    DIR *dirp;
    dirp = opendir(GLOBALZONEDIR);
    if (dirp)
    {
        closedir(dirp);
    }
    else
    {
        hypervisorname = ZONEHYPERVISOR;
        bisnonglobal = true;
    }

    return bisnonglobal;
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
    bool bisvm = false;
    return bisvm;
}

bool IsNativeVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    return IsNonGlobalZone(pats, hypervisorname, hypervisorvers);
}


void GetNicInfos(NicInfos_t & nicInfos)
{
    bool bgotnicinfos = false;
    int sockfd = -1;
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (-1 != sockfd)
    {
        struct lifnum num;
        int ioctlrval = -1;
        num.lifn_family = AF_INET;
        num.lifn_flags = 0;
        ioctlrval = ioctl(sockfd, SIOCGLIFNUM, &num);
        if (!ioctlrval)
        {
            DebugPrintf(SV_LOG_DEBUG, "number of network interfaces = %d\n", num.lifn_count);
            struct lifconf lic;
            lic.lifc_family = AF_INET;
            lic.lifc_flags = 0;
            INM_SAFE_ARITHMETIC(lic.lifc_len = (num.lifn_count * InmSafeInt<size_t>::Type(sizeof (struct lifreq))), INMAGE_EX(num.lifn_count)(sizeof (struct lifreq)))
            lic.lifc_req = (struct lifreq *)calloc(lic.lifc_len, 1);
            if (lic.lifc_req)
            {
                ioctlrval = ioctl(sockfd, SIOCGLIFCONF, &lic);
                if (!ioctlrval)
                {
                    bgotnicinfos = FillNicInfos(sockfd, &lic, &num, nicInfos);
                }
                else
                {
                    DebugPrintf(SV_LOG_ERROR, "ioctl SIOCGLIFCONF failed with errno = %d\n", errno);
                }
                free(lic.lifc_req); 
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "calloc to allocate memory for network interfaces failed with errno = %d\n", errno);
            }
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "ioctl SIOCGLIFNUM failed with errno = %d\n", errno);
        }
        close(sockfd);
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "socket call failed with errno = %d\n", errno);
    }

    if (bgotnicinfos)
    {
        std::string gws = GetDefaultGateways(NSNicInfo::DELIM);
        DebugPrintf(SV_LOG_DEBUG, "gateways = %s\n", gws.c_str());
        if (!gws.empty())
        {
            UpdateNicInfoAttr(NSNicInfo::DEFAULT_IP_GATEWAYS, gws, nicInfos);
        }

        std::string nameservers = GetNameServerAddresses(NSNicInfo::DELIM);
        DebugPrintf(SV_LOG_DEBUG, "nameserver ip addresses = %s\n", nameservers.c_str());
        if (!nameservers.empty())
        {
            UpdateNicInfoAttr(NSNicInfo::DNS_SERVER_ADDRESSES, nameservers, nicInfos);
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "cannot collect the network interfaces information\n");
    }
    DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
}


bool FillNicInfos(const int &sockfd, struct lifconf *plic, struct lifnum *pnum, NicInfos_t & nicInfos)
{
    bool bfillnicinfos = true;
    char ipaddr[INET_ADDRSTRLEN] = "\0";
    const char *prval = NULL;
    struct sockaddr_in *sa = NULL;
    bool bcollect;
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    for (int i = 0; i < pnum->lifn_count; i++)
    {
        sa = (struct sockaddr_in *)&(plic->lifc_req[i].lifr_addr);
        prval = inet_ntop(AF_INET, &(sa->sin_addr.s_addr), ipaddr, sizeof ipaddr);
        if (prval)
        {
            bcollect = ShouldCollectNicInfo(sockfd, plic->lifc_req + i, ipaddr);
            if (bcollect)
            {
                std::string hardwareaddress;
                std::string netmask;
                E_INM_TRISTATE isdhcpenabled = E_INM_TRISTATE_FLOATING;
				GetNetIfParameters(plic->lifc_req[i].lifr_name, hardwareaddress, netmask, isdhcpenabled);
                if (hardwareaddress.empty())
                {
                    continue;
                }
                InsertNicInfo(nicInfos, plic->lifc_req[i].lifr_name, hardwareaddress, ipaddr, netmask, isdhcpenabled);
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "Not collecting information for nic %s\n", plic->lifc_req[i].lifr_name);
            }
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "converting network ip address for interface %s to" 
                " readble string failed with errno = %d."
                " processing next interface if available\n", 
                plic->lifc_req[i].lifr_name, errno);
        }
    } /* for */
    DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
    return bfillnicinfos;
}


void GetNetIfParameters(const std::string &ifrName, std::string& hardwareAddress, std::string &netmask,
                        E_INM_TRISTATE &isdhcpenabled)
{
    /* Place holder function */
    throw std::string("GetNetIfParameters is an obsolete now.");
}


bool ShouldCollectNicInfo(const int &sockfd, struct lifreq *plifr, const char *ipaddr)
{
    bool bshouldcollect = false;
    int ioctlrval = -1;
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    ioctlrval = ioctl(sockfd, SIOCGLIFFLAGS, plifr);
    if (!ioctlrval)
    {
        if (plifr->lifr_flags & IFF_UP)
        {
            bshouldcollect = !(plifr->lifr_flags & IFF_LOOPBACK);
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "ioctl SIOCGLIFFLAGS failed on interface %s." 
            " However trying to report IP and Interface name\n", plifr->lifr_name);
        bshouldcollect = true;
    }
    if (bshouldcollect)
    {
        bshouldcollect = IsValidIP(ipaddr);
    }
    DebugPrintf( SV_LOG_DEBUG, "EXITING %s\n", FUNCTION_NAME);
    return bshouldcollect;
}


SV_ULONGLONG GetTotalFileSystemSpace(const struct statvfs64 &vfs)
{
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_blocks) * ((SV_ULONGLONG)vfs.f_frsize);
    return space;
}


SV_ULONGLONG GetFreeFileSystemSpace(const struct statvfs64 &vfs)
{
    SV_ULONGLONG space = ((SV_ULONGLONG)vfs.f_bavail) * ((SV_ULONGLONG)vfs.f_frsize);
    return space;
}


bool GetBasicVolInfoForP0(const int &fd, const std::string &sDevName, SV_BASIC_VOLINFO *pbasicvolinfo)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with disk name %s\n", FUNCTION_NAME, sDevName.c_str());
    bool bgotbasicvolinfo = false;
    struct dk_minfo mi;
    struct part_info pi;
    memset(&pi, 0, sizeof pi);
    memset(&mi, 0, sizeof mi);
    unsigned long long secsize = NBPSCTR;
    int mirval = ioctl(fd, DKIOCGMEDIAINFO, &mi);
    if (0 == mirval)
    {
        secsize = mi.dki_lbsize;
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "For %s, ioctl DKIOCGMEDIAINFO failed with errno = %d, " 
                                  "while getting size for fdisk p0\n", sDevName.c_str(), errno);
    }
    unsigned long long size = 0;
    unsigned long long nsecs = 0;
    int pirval = ioctl(fd, DKIOCPARTINFO, &pi);
    if (0 == pirval)
    {
        nsecs = pi.p_length;
        size = nsecs * secsize;
        if (size)
        {
            pbasicvolinfo->m_ullrawsize = size;
            pbasicvolinfo->m_uhsectorsz = secsize;
            pbasicvolinfo->m_lnumblocks = nsecs;
            pbasicvolinfo->m_breadonly = false;
            bgotbasicvolinfo = true;
            DebugPrintf(SV_LOG_DEBUG, "For %s, size got is " ULLSPEC " for fdisk p0\n", sDevName.c_str(), size);
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "For %s, size got is zero for fdisk p0\n", sDevName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "For %s, ioctl DKIOCPARTINFO failed with errno = %d for fdisk p0\n", sDevName.c_str(), errno);
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s with disk name %s\n", FUNCTION_NAME, sDevName.c_str());
    return bgotbasicvolinfo;
}


unsigned long long GiveValidSize(const std::string &device, 
                                 const unsigned long long nsecs, 
                                 const unsigned long long secsz, 
                                 const E_VERIFYSIZEAT everifysizeat)
{
    unsigned long long size = 0;

    int fd = open(device.c_str(), O_RDONLY);
    if (-1 != fd)
    {
        bool bisreadable = IsDeviceReadable(fd, device, nsecs, secsz, everifysizeat);
        bool bshouldbrute = bisreadable ? (E_ATMORETHANSIZE == everifysizeat) : (E_ATLESSTHANSIZE == everifysizeat);
        if (bshouldbrute)
        {
             DebugPrintf(SV_LOG_WARNING, "For device: %s, size: " ULLSPEC " is invalid. Getting size by reading\n", device.c_str(), nsecs * secsz);
             size = GetSizeThroughRead(fd, device);
             DebugPrintf(SV_LOG_DEBUG, "For device: %s, size is " ULLSPEC " is got by reading\n", device.c_str(), size);
        }
        else
        {
            size = nsecs * secsz;
        }
        close(fd);
    }

    return size;
}

int SVgetmntent(FILE *fp, struct mnttab *mp,char* buffer,int buflength)
{
    /* place holder function */
    throw std::string("SVgetmntent is an obsolete now.");
    return 0;
}

bool RemoveVgWithAllLvsForVsnap(const std::string & device,std::string& output, std::string& error,bool needvgcleanup)
{
    return true;
}


bool GetZpoolsWithStorageDevices(ZpoolsWithStorageDevices_t &zpoolswithstoragedevices)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);

    std::string cmd("/usr/sbin/zpool status 2> /dev/null | /usr/bin/grep -v \'^$\'");
    std::stringstream results;

    if (!executePipe(cmd, results)) {
        DebugPrintf(SV_LOG_ERROR,"Unable to run zpool status. vsnaps will not be deleted. please try again\n");
        return false;
    }

    if (results.str().empty()) {
        DebugPrintf(SV_LOG_DEBUG,"EXITING %s with retval true. since zpool status has given empty output\n",FUNCTION_NAME);
        return true;
    }

    const std::string POOLNAMETOKEN = "pool:"; 
    std::string::size_type prevPoolIdx, nextPoolIdx;

    //find first pool
    prevPoolIdx = results.str().find(POOLNAMETOKEN, 0);
    if (std::string::npos == prevPoolIdx) {
        DebugPrintf(SV_LOG_DEBUG,"EXITING %s with retval true. since zpool status has given no pools\n",FUNCTION_NAME);
        return true;
    }

    do {
        //exit condition: no more output to search for pools
        if (prevPoolIdx == results.str().length())
            break;

        //find next pool
        nextPoolIdx = results.str().find(POOLNAMETOKEN, prevPoolIdx+1);

        //handle last pool
        if (std::string::npos == nextPoolIdx)    
            nextPoolIdx = results.str().length();

        std::stringstream zpoolstream(std::string(results.str().substr(prevPoolIdx, nextPoolIdx - prevPoolIdx)));
        prevPoolIdx = nextPoolIdx;
        
        GetZpoolStorageDevices(zpoolstream, zpoolswithstoragedevices);

    } while (!results.eof());

    DebugPrintf(SV_LOG_DEBUG,"EXITING %s with retval as true\n",FUNCTION_NAME);
    return true;
}


void GetZpoolStorageDevices(std::stringstream &zpoolstream, ZpoolsWithStorageDevices_t &zpoolswithstoragedevices)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    DebugPrintf(SV_LOG_DEBUG, "============= ZPOOL START =============\n");
    DebugPrintf(SV_LOG_DEBUG, "%s", zpoolstream.str().c_str());
    DebugPrintf(SV_LOG_DEBUG, "============= ZPOOL END =============\n");

    const std::string DskDir = "/dev/dsk/";

    /* sample output:
    *  =============
    *(03-03-2014 12:55:02):   DEBUG  17539 1 ============= ZPOOL START =============
    *(03-03-2014 12:55:02):   DEBUG  17539 1 pool: BUILDS
    * state: UNAVAIL
    *status: One or more devices are unavailable in response to persistent errors.
    *    There are insufficient replicas for the pool to continue functioning.
    *action: Destroy and re-create the pool from a backup source. Manually marking
    *    the device repaired using 'zpool clear' or 'fmadm repaired' may
    *    allow some data to be
    *    recovered.
    *    Run 'zpool status -v' to see device specific details.
    *  scan: none requested
    *config:
    *    NAME      STATE     READ WRITE CKSUM
    *    BUILDS    UNAVAIL      0     0     0
    *      c8t1d0  UNAVAIL      0     0     0
    *  (03-03-2014 12:55:02):   DEBUG  17539 1 ============= ZPOOL END =============
    *(03-03-2014 12:55:02):   DEBUG  17539 1 ============= ZPOOL START =============
    *(03-03-2014 12:55:02):   DEBUG  17539 1 pool: rpool
    * state: ONLINE
    *  scan: none requested
    *config:
    *    NAME      STATE     READ WRITE CKSUM
    *    rpool     ONLINE       0     0     0
    *      c8t0d0  ONLINE       0     0     0
    *errors: No known data errors
    *(03-03-2014 12:55:02):   DEBUG  17539 1 ============= ZPOOL END =============
    */

    //poolname
    std::string poollabel, poolname;
    zpoolstream >> poollabel;
    std::getline(zpoolstream, poolname);
    Trim(poolname, " ");

    //reach state:
    std::string tmpstr;
    while (!zpoolstream.eof()) {
        std::getline(zpoolstream, tmpstr);
        if (tmpstr.empty())
            break;
        Trim(tmpstr, " ");
        if (0 == tmpstr.find("state:"))
            break;
        tmpstr.clear();
    }

    std::stringstream ssstate(tmpstr);
    std::string state;  
    ssstate >> tmpstr >> state;
    DebugPrintf(SV_LOG_DEBUG, "poollabel %s, poolname %s, state %s\n", poollabel.c_str(), poolname.c_str(), state.c_str());
    /* collect only online pools */
    if (("pool:" == poollabel) && !poolname.empty() && ("ONLINE" == state)) {
        std::pair<ZpoolsWithStorageDevicesIter_t, bool> p = zpoolswithstoragedevices.insert(std::make_pair(poolname, svector_t()));
        ZpoolsWithStorageDevicesIter_t it = p.first;
        svector_t &storagedevices = it->second;

        //reach config:
        tmpstr.clear();
        while (!zpoolstream.eof()) {
            std::getline(zpoolstream, tmpstr);
            if (tmpstr.empty())
                break;
            Trim(tmpstr, " ");
            if (0 == tmpstr.find("config:"))
                break;
            tmpstr.clear();
        }

        //reach NAME      STATE     READ WRITE CKSUM
        tmpstr.clear();
        while (!zpoolstream.eof()) {
            std::getline(zpoolstream, tmpstr);
            if (tmpstr.empty())
                break;
            Trim(tmpstr, " \t");
            if ((0 == tmpstr.find("NAME")) && (std::string::npos != tmpstr.find("STATE")))
                break;
            tmpstr.clear();
        }

        //skip "rpool     ONLINE       0     0     0" line
        zpoolstream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        //pick the first devices until end is reached or errors: is reached.
        tmpstr.clear();
        while (!zpoolstream.eof()) {
            std::getline(zpoolstream, tmpstr);
            if (tmpstr.empty())
                break;
            Trim(tmpstr, " \t");
            if (0 == tmpstr.find("errors:"))
                break;
            std::stringstream ssdeviceline(tmpstr);
            std::string storagedevice;
            ssdeviceline >> storagedevice;
            if (0 != storagedevice.find('/'))
                storagedevice = DskDir + storagedevice;
            storagedevices.push_back(storagedevice);
            tmpstr.clear();
        }
    } else
        DebugPrintf(SV_LOG_DEBUG, "No pool name or offline pool found\n");

    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
}


void PrintZpoolsWithStorageDevices(const ZpoolsWithStorageDevices_t &zpoolswithstoragedevices)
{
    for (ConstZpoolsWithStorageDevicesIter_t cit = zpoolswithstoragedevices.begin(); cit != zpoolswithstoragedevices.end(); cit++) {
        const std::string &poolname = cit->first;
        const svector_t &storagedevices = cit->second;
        DebugPrintf(SV_LOG_DEBUG, "poolname %s has below devices:\n", poolname.c_str());
        for_each(storagedevices.begin(), storagedevices.end(), PrintString);
        DebugPrintf(SV_LOG_DEBUG, "==============\n");
    }
}
