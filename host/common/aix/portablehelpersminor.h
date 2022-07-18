#ifndef PORTABLEHELPERS__MINORPORT__H_
#define PORTABLEHELPERS__MINORPORT__H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SV_FUSER_FORDEV   "fuser "
#define SV_FUSER_DEV_KILL        "fuser -k "
#define SV_FUSER_FOR_MNTPNT "fuser -c "
#define SV_FUSER_MNTPNT_KILL "fuser -k -c " 

#define SV_PROC_MNTS        "/etc/filesystems"
#define SV_FSTAB            "/etc/filesystems"
#define SV_LAZYUMOUNT       "umount -f "

#define ETC_FSTAB    "/etc/vfstab"
#define PROC_MOUNTS    "/etc/mnttab"
#define PROC_PARTITIONS "/proc/partitions"

#define INM_POSIX_FADV_NORMAL 0
#define INM_POSIX_FADV_SEQUENTIAL 0 
#define INM_POSIX_FADV_RANDOM 0
#define INM_POSIX_FADV_NOREUSE 0
#define INM_POSIX_FADV_WILLNEED 0 
#define INM_POSIX_FADV_DONTNEED 0

#define VSNAP_DEVICE_DIR "/dev/vs/"
#define AIX_DEVICE_DIR "/dev/"
#define AIX_RDEVICE_PREFIX "/dev/r"
#define MKDEV_CMD "/usr/sbin/mkdev -c disk -s vscsi -t inmage -l "
#define RMDEV_CMD "/usr/sbin/rmdev -l "
#define RMDEV_CMD_PREFIX " -d"
#define VSNAP_RDEVICE_PREFIX "/dev/vs/r"
#define LISTALLPVSCMD       "/usr/sbin/lspv "
#define LISTLVSFROMVGCMD    "/usr/sbin/lsvg -l "
#define DEACTIVATEVGCMD     "/usr/sbin/varyoffvg "
#define CLEANUPVGANDLVSCMD  "/usr/sbin/exportvg "
#define QUERYVGFROMPVCMD    "/usr/sbin/lqueryvg -tAp "
#define VGNAMEFROMVGIDCMD   "/usr/sbin/getlvodm -t "
#define RECREATEVGCMD       "/usr/sbin/recreatevg -y "

enum MOUNT_FLAGS
{
    MOUNT_UNKNOWN = -1,
    MOUNT_RO,
    MOUNT_RW,
    MOUNT_RBR,
    MOUNT_RBW,
    MOUNT_RBRW
};

struct mount_info
{
   std::string m_nodeName;
   std::string m_deviceName; /* mounted */
   std::string m_mountedOver; /* where it is mounted */
   std::string m_vfs; /* vitual file system */
   std::string m_date; /* mounted date */
   MOUNT_FLAGS m_flag; /* permission type */
   std::string m_options; 
};

SV_ULONGLONG GetRawVolSize(const std::string sVolume);
bool IsSizeValid(int fd, const std::string devicename, 
                        unsigned long long size, unsigned int blksize);
unsigned long long GetSizeThroughRead(int fd, const std::string dskdevicename);

std::string GetBlockVolumeName(const std::string &rdskname);

#define MOUNT_COMMAND "/usr/sbin/mount"
#define OPTION_TO_SPECIFY_FILESYSTEM "-v"
#define RETRIES_FOR_MOUNT 3

#define FSCK_COMMAND "/usr/sbin/fsck"
#define FSCK_FS_OPTION "-f"
#define FSCK_FULL_OPTION " -o full,nolog"

#ifdef _LP64
#define LIBEFI_DIRPATH "/lib/64"
#else
#define LIBEFI_DIRPATH "/lib"
#endif /* _LP64 */

#define LIBEFINAME "libefi.so"

#define BIOSREADCMD "/usr/sbin/smbios"

SVERROR getMountedInfo(std::vector<mount_info>& mountInfoList);
void dumpMountInfo(const std::vector<mount_info>& mountInfoList);
MOUNT_FLAGS getPermissionType( std::string& optionsStr );
const char * const DeterministicVmCmds[] = {BIOSREADCMD /*, PRTDIAGCMD , PRTCONFCMD */ };

const char * const DeterministicVmPats[] = {VMWAREMFPAT, XENMFPAT, MICROSOFTMFPAT};

const char * const DeterministicHypervisors[] = {VMWARENAME, XENNAME, MICROSOFTNAME};

#define ZONEHYPERVISOR "aix zone"
#define GLOBALZONEDIR "/proc/1"
bool IsNonGlobalZone(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool getMountPointName(const std::string& deviceName, std::string& mountPoint, bool bFromFileSytemFile = true);
bool addFSEntryWithOptions(const std::string& device1, const std::string& label, const std::string& mountpoint1, const std::string& type,
                           bool readonlyflag,std::vector<std::string>& fsents);

std::string getFSString( int code);

#define ROUTINGTABLECMD "/usr/bin/netstat -rn"


std::string GetCustomizedDeviceParent(const std::string &device, std::string &errmsg);

std::string GetCustomizedAttributeValue(const std::string &name, const std::string &attribute, std::string &errmsg);
bool CleanStaleVgAndRecreateVgForVsnapOnReboot(const std::string & devname);
bool HideAllLvs(std::vector<std::string> & lvs,std::string& output, std::string& error);
bool GetAllLvsForVg(const std::string & vgname,std::vector<std::string> & lvs);
bool GetVgNameForPvIfExist(const std::string & pvname,std::string & vgname);


#endif /* PORTABLEHELPERS__MINORPORT__H_ */
