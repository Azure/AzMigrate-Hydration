#ifndef PORTABLEHELPERS__MAJORPORT__H_
#define PORTABLEHELPERS__MAJORPORT__H_

#include <sys/statvfs.h>
#include "portablehelpers.h"

inline bool IS_INVALID_HANDLE_VALUE( ACE_HANDLE h )
{
    if ( ACE_INVALID_HANDLE == h )
	return true;
}


inline void SAFE_CLOSEHANDLE( ACE_HANDLE& h )
{
    if( !IS_INVALID_HANDLE_VALUE( h ) )
    {
		ACE_OS::close( h );
    }
}
SVERROR GetVolSize(ACE_HANDLE handle, unsigned long long *pSize);

#define PATCH_FILE	"/patch.log"

#define SV_UMOUNT           "umount "
#define INM_VACP_SUBPATH    "bin/vacp"

bool GetdevicefilefromMountpoint(const char* MntPnt,std::string &DeviceFile);
bool mountedvolume( const std::string& pathname, std::string& mountpoints );
bool containsMountedVolumes( const std::string& path, std::string& volumes );
bool GetMountPoints(const char * dev, std::vector<std::string> &mountPoints);
bool CanUnmountMountPointsIncludingMultipath(const char * dev, std::vector<std::string> &mountPoints,std::string & errormsg);

bool FormVolumeLabel(const std::string device, std::string &label);
bool addRWFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, 
                  const std::string& type,
				  std::vector<std::string>& fsents);
bool addROFSEntry(const std::string& device, const std::string& label, const std::string& mountpoint, 
                  const std::string& type,
				  std::vector<std::string>& fsents);
bool removeFSEntry(const std::string& device, const std::string& label, std::vector<std::string>& fsents);
std::string GetCommandPath(const std::string &cmd);
bool AddFSEntry(const char * dev,std::string  mountpoint, std::string type, std::string flags, bool remount);
bool AddCIFS_FSEntry(const char * dev, std::string  mountpoint, std::string type);
bool GetVolSizeWithOpenflag(const std::string vol, const int oflag, SV_ULONGLONG &refvolsize);
bool GetNumBlocksWithFd(int fd, const std::string vol, SV_ULONGLONG &NumBlks);
bool GetSectorSizeWithFd(int fd, const std::string vol, SV_ULONG &SectorSize);
bool  VsnapDeleteMntPoint(const char *MntPoint ) ;
bool ExecuteInmCommand(const std::string& fsmount, std::string& errormsg, int & ecode);
bool LinkExists(const std::string linkfile);
bool FileExists(const std::string file);
bool MountDevice(const std::string& device,const std::string& mountPoint,const std::string& fsType,bool bSetReadOnly, int &exitCode,
                 std::string &errMsg);

SV_ULONGLONG GetTotalFileSystemSpace(const struct statvfs64 &vfs);
SV_ULONGLONG GetFreeFileSystemSpace(const struct statvfs64 &vfs);

bool IsDeviceReadable(const int &fd, const std::string &devicename, 
                      const unsigned long long nsecs, 
                      const unsigned long long blksize,
                      const E_VERIFYSIZEAT everifysizeat);
bool MaskSignal(int signo);
std::string GetExecdName(void);
void set_resource_limits();
#define UNIX_PATH_SEPARATOR "/"
#define UNIX_HIDDENFILE_STARTER '.'
#define FOPEN_MODE_NOINHERIT ""

std::string GetDefaultGateways(const char &delim);
std::string GetNameServerAddresses(const char &delim);

int TerminateUsersAndUnmountMountPoint(const char * mntpnt,std::string& error);
bool RemoveVgWithAllLvsForVsnap(const std::string & device,std::string& output, std::string& error,bool needvgcleanup);

const char PIPE_CHAR = '|';
const char SPACE = ' ';
const char OR_EXTREGEX = '|';
const char WHITESPACE_REGEX[] = "[\t ]";
const char ZEROORMORE_REGEX = '*';
const char ONEORMORE_REGEX = '+';
const char STARTLINE_ANCHOR_REGEX = '^';

void GetAgentRepositoryAccess(int& accessError, SV_LONG& lastSuccessfulTs, std::string& errmsg);
long GetBookMarkSuccessTimeStamp() ;
void UpdateBookMarkSuccessTimeStamp() ;
long GetAgentHeartBeatTime() ;
long GetRepositoryAccessTime() ;
void UpdateAgentRepositoryAccess(int error, const std::string& errmsg) ;
void UpdateBackupAgentHeartBeatTime() ;
void UpdateRepositoryAccess() ;
std::string GetServiceStopReason() ;
void UpdateServiceStopReason(std::string& reason) ; 
unsigned int retrieveBusType(std::string volume) ;
#endif /* PORTABLEHELPERS__MAJORPORT__H_ */
