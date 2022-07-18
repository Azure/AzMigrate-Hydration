#ifndef PORTABLEHELPERS__MINORPORT__H_
#define PORTABLEHELPERS__MINORPORT__H_

#include <string>
#include <sstream>
#include <map>
#include <utility>

#include <sys/mnttab.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/fcntl.h>

#include "portablehelpers.h"

#define SV_FUSER_FORDEV   "fuser "
#define SV_FUSER_DEV_KILL        "fuser -k "
#define SV_FUSER_FOR_MNTPNT "fuser -c "
#define SV_FUSER_MNTPNT_KILL "fuser -k -c " 

#define SV_PROC_MNTS        "/etc/mnttab"
#define SV_FSTAB            "/etc/vfstab"
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

#define VSNAP_DSK_DIR "/dev/vs/dsk/"
#define VSNAP_RDSK_DIR "/dev/vs/rdsk/"
#define VSNAP_DEVICE_NAMESPACE "/devices/pseudo/linvsnap@0:vsnap"
#define VSNAP_RDEVICE_SUFFIX ",raw"


typedef struct basicvolinfo
{
   SV_ULONGLONG m_ullrawsize; 
   bool m_breadonly;
   SV_USHORT m_uhsectorsz;
   SV_LONG m_lnumblocks; 

   basicvolinfo()
   {
       m_ullrawsize = m_uhsectorsz = m_lnumblocks = 0;
       m_breadonly = false;
   }

} SV_BASIC_VOLINFO;

bool IsValidMntEnt(struct mnttab *entry);
SV_ULONGLONG GetRawVolSize(const std::string &sVolume);
SVERROR GetRawVolumeSectorSize(const char *volName, unsigned int *psectorSize);
bool GetBasicVolInfo(const std::string &sDevName, SV_BASIC_VOLINFO *pbasicvolinfo);
unsigned long long GiveValidSize(const std::string &device, 
                                 const unsigned long long nsecs, 
                                 const unsigned long long secsz, 
                                 const E_VERIFYSIZEAT everifysizeat);
unsigned long long GetSizeThroughRead(int fd, const std::string &device);

std::string GetBlockVolumeName(const std::string &rdskname);

#define MOUNT_COMMAND "/usr/sbin/mount"
#define OPTION_TO_SPECIFY_FILESYSTEM "-F"
#define RETRIES_FOR_MOUNT 3

#define FSCK_COMMAND "/usr/sbin/fsck"
#define FSCK_FS_OPTION "-F"
#define FSCK_FULL_OPTION " -o full,nolog"

#ifdef _LP64
#define LIBEFI_DIRPATH "/lib/64"
#else
#define LIBEFI_DIRPATH "/lib"
#endif /* _LP64 */

#define LIBEFINAME "libefi.so"

#define FULLEFIIDX 7
#define READVTOCRVALFORP0 16

#define BIOSREADCMD "/usr/sbin/smbios"
/* not sure about prtconf */
/* #define PRTCONFCMD "/usr/sbin/prtconf" */

/* should we run prtdiag; for now (as of 5.5),
 * may not be needed since solaris 10 x86 is 
 * supported which has smbios */
/* #define PRTDIAGCMD "/usr/sbin/prtdiag" */
const char * const DeterministicVmCmds[] = {BIOSREADCMD /*, PRTDIAGCMD , PRTCONFCMD */ };

const char * const DeterministicVmPats[] = {VMWAREMFPAT, XENMFPAT, MICROSOFTMFPAT};

/* TODO: MICROSOFTPAT can mean hyperv or virtual pc; hence using hypervisor name as MICROSOFTNAME */
const char * const DeterministicHypervisors[] = {VMWARENAME, XENNAME, MICROSOFTNAME};

#define ZONEHYPERVISOR "solaris zone"
#define GLOBALZONEDIR "/proc/1"
bool IsNonGlobalZone(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool GetBasicVolInfoForP0(const int &fd, const std::string &sDevName, SV_BASIC_VOLINFO *pbasicvolinfo);

int SVgetmntent(FILE *fp, struct mnttab *mp,char*,int);

#define ROUTINGTABLECMD "/usr/bin/netstat -rn"

typedef std::map<std::string, svector_t> ZpoolsWithStorageDevices_t;
typedef ZpoolsWithStorageDevices_t::const_iterator ConstZpoolsWithStorageDevicesIter_t;
typedef ZpoolsWithStorageDevices_t::iterator ZpoolsWithStorageDevicesIter_t;
typedef std::pair<std::string, svector_t> ZpoolWithStorageDevices_t;

bool GetZpoolsWithStorageDevices(ZpoolsWithStorageDevices_t &zpoolswithstoragedevices);
void GetZpoolStorageDevices(std::stringstream &zpoolstream, ZpoolsWithStorageDevices_t &zpoolswithstoragedevices);
void PrintZpoolsWithStorageDevices(const ZpoolsWithStorageDevices_t &zpoolswithstoragedevices);

#endif /* PORTABLEHELPERS__MINORPORT__H_ */
