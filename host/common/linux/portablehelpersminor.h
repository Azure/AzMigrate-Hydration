#ifndef PORTABLEHELPERS__MINORPORT__H_
#define PORTABLEHELPERS__MINORPORT__H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mntent.h>

#define BLKGETSIZE   _IO(0x12,96)    /* return device size */
#define BLKSSZGET    _IO(0x12,104)   /* get block device sector size */
#define BLKGETSIZE64 _IOR(0x12,114,size_t)  /* return device size.  */

#define INM_POSIX_FADV_NORMAL POSIX_FADV_NORMAL
#define INM_POSIX_FADV_SEQUENTIAL POSIX_FADV_SEQUENTIAL 
#define INM_POSIX_FADV_RANDOM POSIX_FADV_RANDOM
#define INM_POSIX_FADV_NOREUSE POSIX_FADV_NOREUSE
#define INM_POSIX_FADV_WILLNEED POSIX_FADV_WILLNEED 
#define INM_POSIX_FADV_DONTNEED POSIX_FADV_DONTNEED

#define SV_PROC_MNTS        "/etc/mtab"
#define SV_FSTAB            "/etc/fstab"
#define SV_LAZYUMOUNT       "umount -l "
#define SV_FAST_PROC_MOUNT  "/proc/mounts"
#define SV_PROC_PARTITIONS  "/proc/partitions"

#define SV_FUSER_FORDEV   "fuser -m "
#define SV_FUSER_DEV_KILL        "fuser -k -m "
#define SV_FUSER_FOR_MNTPNT SV_FUSER_FORDEV
#define SV_FUSER_MNTPNT_KILL  SV_FUSER_DEV_KILL

#define SV_LABEL            "/sbin/e2label "

#define ETC_FSTAB    "/etc/fstab"
#define PROC_MOUNTS    "/etc/mtab"
#define PROC_PARTITIONS "/proc/partitions"

#define VSNAP_DEVICE_DIR "/dev/vs/"

enum LINUX_TYPE
{
 REDHAT_RELEASE = 1, 
 SUSE_RELEASE = 2,
 DEBAIN_RELEASE = 3,
 UBUNTU_RELEASE = 4,
 OEL5_RELEASE = 5,
 OEL_RELEASE = 6
};
bool IsValidMntEnt(struct mntent *entry);

const char * const DeterministicVmPats[] = {VMWAREMFPAT, MICROSOFTCORPMFPAT, VIRTUALBOXMFPAT};
/* TODO: DMICROSOFTPAT can mean hyperv or virtual pc; hence using hypervisor name as MICROSOFTNAME */
const char * const DeterministicHypervisors[] = {VMWARENAME, MICROSOFTNAME, VIRTUALBOXNAME};

#define BIOSREADCMD "dmidecode"
#define BIOSREADCMD2 "/bin/sh -c 'echo Manufacturer: `cat /sys/class/dmi/id/sys_vendor`'"
const char * const DeterministicVmCmds[] = {BIOSREADCMD, BIOSREADCMD2};

// dmidecode may not be available on all platforms
// so first we check the /sys interface
#define SYSUUIDCMD "cat /sys/class/dmi/id/product_uuid"
#define BIOSIDCMD "cat /sys/class/dmi/id/product_serial"
#define ASSETTAGCMD "cat /sys/devices/virtual/dmi/id/chassis_asset_tag"

// in case of RHEL5 the /sys/class/dmi is not populated
// so below are the alternate commands
#define SYSUUIDCMD2 "/usr/sbin/dmidecode -s system-uuid"
#define BIOSIDCMD2 "/usr/sbin/dmidecode -s system-serial-number"
#define ASSETTAGCMD2 "/usr/sbin/dmidecode -s chassis-asset-tag"


#define HYPERVISOROPENVZ "openvz"
#define OPENVZVMVZDIR "/proc/vz"
#define OPENVZVMBCDIR "/proc/bc"

#define CPUINFOFILE "/proc/cpuinfo"
#define QEMUPAT "QEMU"
#define UMLPAT "UML"

/* drivers that start with xen */
#define ISXENCMD "lsmod | grep -i '^xen'"

SVERROR getLinuxReleaseValue( std::string & linuxetcReleaseValue, LINUX_TYPE& linuxFlavourType );
bool isLocalMachine32Bit( std::string machine);

#define ROUTINGTABLECMD "netstat -rn"

#endif /* PORTABLEHELPERS__MINORPORT__H_ */
