///
/// @file portablehelpers.h
/// Define helper functions to work on volume parameters
///
#ifndef PORTABLEHELPERS__H
#define PORTABLEHELPERS__H

#include <iostream>
#include <string>
#include <vector>
#include <iterator>
#include <set>
#include <map>
#if defined SV_LINUX
#include <sys/sysmacros.h>
#endif
#include "pats.h"
typedef std::vector<std::string> svector_t;
typedef svector_t::const_iterator constsvectoriter_t;
typedef svector_t::iterator svectoriter_t;

typedef std::set<std::string> strset_t;

#include "portable.h"
#include "svtypes.h"
#include "inmdefines.h"
#include "inmquitfunction.h"
#include "svdparse.h"

#ifdef WIN32
#pragma warning( disable: 4312 )    // converting void* to int isn't 64-bit safe
#endif
#include <ace/OS.h>
#ifdef WIN32
#pragma warning( default: 4312 )    // converting void* to int isn't 64-bit safe
#endif

#include <ace/OS.h>

#include <ace/OS_NS_fcntl.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_errno.h>
#include <ace/Global_Macros.h>
#include <ace/config-lite.h>
#include <ace/Process.h>

#include <ace/Task.h>
#include <ace/Atomic_Op.h>
#include <ace/Mutex.h>
#include <ace/Message_Queue_T.h>
#include <ace/Message_Block.h>

#define VOL_BSIZE 512
#define YES "yes"
#define NO  "no"

#define MAX_ULL_VALUE 18446744073709551615ULL
#define MAX_FOURBYTE_VALUE 4294967295U

#define SV_NTFS_MAGIC_OFFSET  3
#define SV_MAGIC_NTFS         "NTFS    "
#define SV_MAGIC_SVFS         "SVFS0001"
#define SV_FSMAGIC_LEN        8

#define MAX_PORT_DIGITS 5

#ifndef MAXIPV4ADDRESSLEN
#define MAXIPV4ADDRESSLEN 15
#endif

#define LINE_NO     __LINE__
#define FILE_NAME   __FILE__

#if !defined(ARRAYSIZE)
#define ARRAYSIZE( a ) ( sizeof( a ) / sizeof( a[ 0 ] ) )
#endif

#if !defined(ALIGNED_SIZE)
#define ALIGNED_SIZE( offset, size ) (( ( (offset) + (size-1) )/(size) )*size)
#endif


#define VACP_E_SUCCESS 0
#define VACP_E_GENERIC_ERROR 1
#define VACP_E_INVALID_COMMAND_LINE 10001
#define VACP_E_INVOLFLT_DRIVER_NOT_INSTALLED 10002
#define VACP_E_DRIVER_IN_NON_DATA_MODE 10003
#define VACP_E_NO_PROTECTED_VOLUMES 10004
#define VACP_E_NON_FIXED_DISK 10005
#define VACP_E_ZERO_EFFECTED_VOLUMES 10006
#define VACP_E_VACP_SENT_REVOCATION_TAG 10007
#define VACP_E_VACP_MODULE_FAIL 0x80004005L
#define VACP_E_PRESCRIPT_FAILED 10009
#define VACP_E_POSTSCRIPT_FAILED 10010
#define VACP_PRESCRIPT_WAIT_TIME_EXCEEDED 10998
#define VACP_POSTSCRIPT_WAIT_TIME_EXCEEDED 10999

#define INM_ARRAY_SIZE(a)       (sizeof(a)/sizeof(a[0]))

const std::string external_ip_address = "external_ip_address";
const char HTTPS[] = "https://";
const char IMDS_URL[]						= "http://169.254.169.254/metadata/instance?api-version=2021-02-01";
const char IMDS_HEADERS[]					= "Metadata: true";
const char IMDS_COMPUTE_ENV[]				= "compute.azEnvironment";
const char IMDS_AZURESTACK_NAME[]			= "AzureStack";
const long HTTP_OK = 200L;

/******************************************************************************/
enum FILESYSTEM_TYPE
{
    TYPE_FAT,
    TYPE_NTFS,
    TYPE_ReFS,
    TYPE_EXFAT,
    TYPE_UNKNOWNFS
};

const char * const FileSystemTag[] = {"","NTFS    ","ReFS\0\0\0\0","EXFAT   "};
const char * const SvFileSystemMaskTag[] = {"","SVFS0001","SVFS0002","SVFS0003"};
/*****************************************************************************/


enum VOLUME_STATE { VOLUME_HIDDEN = 0, VOLUME_VISIBLE_RO = 1, VOLUME_VISIBLE_RW = 2, VOLUME_UNKNOWN = 3, VOLUME_DO_NOTHING = 4, VOLUME_HIDDEN_RO };
enum OS_VAL { OS_UNKNOWN = 0, SV_WIN_OS = 1, SV_LINUX_OS = 2, SV_SUN_OS = 3, SV_HPUX_OS = 4, SV_AIX_OS = 5 };

const char * const StrEndianness[] = {"unknown", "little endian", "big endian", "middle endian"};
#define INM_NOT_A_VOLPACK 1

const std::string SPARSE_PARTFILE_EXT = ".filepart";

const char NTFS[] = "ntfs";

const unsigned int LitttleEndianDataFormatFlags = INMAGE_MAKEFOURCC('D', 'R', 'T', 'L');

const char VMWAREPLATFORM[] = "VmWare";
const char AZUREPLATFORM[] = "Azure";
const char CSTYPE_CSPRIME[] = "CSPrime";

struct volpackproperties 
{
    SV_ULONGLONG m_logicalsize;
    SV_ULONGLONG m_sizeondisk;
    std::string m_sparsefilename;
    volpackproperties()
    {
        m_logicalsize = 0;
        m_sizeondisk = 0;
    }
};

enum NUMFORMAT
{
    E_DEC,
    E_OCT,
    E_HEX
};

#define PREUNDERSCORE "pre_"
#define PRE "pre"
#define UNDERSCORE "_"
#define CORRUPT "corrupt"
#define WILDCARDCHAR "*"

#define TRANSPORT_ERROR -11

enum AGENT_RESTART_STATUS
{
    AGENT_FRESH_START,
    AGENT_RESTART
};

typedef std::map<std::string, std::string> Options_t;
typedef Options_t::const_iterator ConstOptionsIter_t;
typedef Options_t::iterator OptionsIter_t;

#define CS_PARAM_RESYNC_REASON_CODE "resyncReasonCode"
#define CS_PARAM_DETECTION_TIME "detectionTime"
typedef std::map<std::string, std::string> ResyncReasonStamp;
#define STAMP_RESYNC_REASON(resyncReasonStamp, resyncReasonCode) \
{ \
    resyncReasonStamp[CS_PARAM_RESYNC_REASON_CODE] = resyncReasonCode; \
    resyncReasonStamp[CS_PARAM_DETECTION_TIME] = boost::lexical_cast<std::string>(GetTimeInSecSinceEpoch1970()); \
} \

inline void SAFE_CLOSEFILEHANDLE( ACE_HANDLE& h )
{
    if( ACE_INVALID_HANDLE != h )
    {
        ACE_OS::close( h );
    }
    h = ACE_INVALID_HANDLE;
}

struct IsPairKeyValEqual
{
    template <typename Pair>
    bool operator() (Pair const &lhs, Pair const &rhs) const
    {
        return (lhs.first == rhs.first) && (lhs.second == rhs.second);
    }
};

/// Returns true if boath input maps equal in size and also matches each key value pair, false otherwise.
template <typename Map>
bool CompareMap(Map const &left, Map const &right)
{
    return left.size() == right.size() &&
        std::equal(left.begin(), left.end(), right.begin(), IsPairKeyValEqual());
}

SVERROR GetVolSize(ACE_HANDLE handle, unsigned long long *pSize);
SVERROR GetFsVolSize(const char *volName, unsigned long long *pfsSize);
SVERROR GetFsSectorSize(const char *volName, unsigned int *psectorSize);
bool GetVolumeSize(char *vol, SV_ULONGLONG* size);
bool CanReadDriverSize(char *vol, SV_ULONGLONG offset, unsigned int sectorSize);
bool GetDriverVolumeSize(char *vol, SV_ULONGLONG* size);
bool GetFileSystemVolumeSize(char *vol, SV_ULONGLONG* size, unsigned int* sectorSize);
bool GetVolumeRootPath(const std::string & path, std::string & strRoot);
SVERROR OpenVolume( ACE_HANDLE *pHandleVolume, const char *pch_VolumeName );
SVERROR OpenVolumeExtended( ACE_HANDLE *pHandleVolume, const char   *pch_VolumeName, int  OpenMode);
SVERROR OpenVolumeExclusive(ACE_HANDLE *pHandleVolume, std::string &outs_VolGuidName, char const *pch_VolumeName, char const* mountPoint, bool bUnmount, bool bUnbufferIO);
SVERROR CloseVolume( ACE_HANDLE handleVolume );
SVERROR CloseVolumeExclusive( ACE_HANDLE handleVolume, const char *pch_VolGuidName, const char *pch_VolumeName, char const* mountPoint );
SVERROR GetHardwareSectorSize(ACE_HANDLE handleVolume, unsigned int *pSectSize);
bool IsVolumeLocked(const char *pszVolume, const int fsTypeSize = 0, char * fsType = NULL);

SVERROR UnhideDrive_RO(const char * drive, char const* mountPoint,const char* fs = NULL );
SVERROR UnhideDrive_RW(const char * drive, char const* mountPoint,const char* fs = NULL );
//Bug #4044
SVERROR HideDrive(const char * volumeName, char const* mountPoint );
SVERROR HideDrive(const char * volumeName, char const* mountPoint, std::string& output, std::string& error,bool checkformultipaths = true);
SVERROR SetReadOnly(const char * drive);
SVERROR ResetReadOnly(const char * drive);
SVERROR StartFilter(const char * drive, void * VolumeGuid);
SVERROR StopFilter(const char * drive, void * VolumeGuid);
SVERROR SetReadOnly(const char * drive, void * VolumeGuid, bool isVirtualVolume );
SVERROR ResetReadOnly(const char * drive, void * VolumeGuid, bool isVirtualVolume );
SVERROR isReadOnly(const char * drive, bool & rvalue);
SVERROR  isVirtualVolumeReadOnly(void * VolumeGUID, bool & rvalue);
VOLUME_STATE GetVolumeState(const char * drive);

bool DeleteLocalFile(std::string const & name);
SVERROR FlushFileSystemBuffers(const char *Volume);

SVERROR SVGetFileSize( char const* pszPathname, SV_LONGLONG* pFileSize );
//SV_ULONG SVGetFileSize( char const* pszPathname, OPTIONAL SV_ULONG* pHighSize );

SVERROR SVMakeSureDirectoryPathExists( const char* pszDestPathname );
void StringToSVLongLong( SV_LONGLONG& num, char const* psz );
char* SVLongLongToString( char* psz, SV_LONGLONG num, size_t size );

SVERROR GetVolumeSize(ACE_HANDLE volumeHandle, unsigned long long* volumeSize);
SV_LONGLONG GetVolumeCapacity( const char *volName );
SVERROR BaseName( const char *pszFullName, char *pszBaseName, const int pszBaseNameLen);
SVERROR DirName( const char *pszFullName, char *pszDirName, const int pszDirNameLen);

std::string basename_r(const std::string &name, const char &separator_char = ACE_DIRECTORY_SEPARATOR_CHAR_A);
std::string dirname_r(const std::string &name, const char &separator_char = ACE_DIRECTORY_SEPARATOR_CHAR_A);
std::string PlatformBasedDirname(const std::string &name, const char &separator_char = ACE_DIRECTORY_SEPARATOR_CHAR_A);

#ifdef SV_USES_LONGPATHS
std::wstring getLongPathName(const char* path);
#else
std::string getLongPathName(const char* path);
#endif /* SV_WINDOWS */

int sv_stat(const char *file, ACE_stat *);
int sv_stat(const wchar_t *file, ACE_stat *);
ACE_DIR *sv_opendir(const char *filename);

SVERROR ReplaceChar(char *pszInString, char inputChar, char outputChar);
SVERROR AppendChar(char *pszInString, char inputChar);

std::string GetVolumeDirName(std::string volName);
bool RegExpMatch(const char *srchptrn, const char *input);
int DoesRegExpRangeMatch(const char *srchptrn, char verify, char **ptrcurr);
SVERROR CleanupDirectory(const char *pszDirectory, const char *pszFilePattern);
bool sv_get_filecount_spaceusage(const char * directory, const char * file_pattern, 
                                 SV_ULONGLONG & filecount, SV_ULONGLONG & size_on_disk,SV_ULONGLONG& logicalsize);

bool endsWith( const char* str, const char* ending, bool caseSensitive = true );

bool GetFileSystemVolumeSize(char *vol, SV_ULONGLONG* size, unsigned int* sectorSize);
bool CanReadDriverSize(char *vol, SV_ULONGLONG offset, unsigned int sectorSize);
int mkdirRecursive( char const* pathname, mode_t mode = ACE_DEFAULT_DIR_PERMS);
void GetConsoleYesOrNoInput(char & ch);

void ReplaceChar(std::string & str, char inputChar, char outputChar);
void FormatVolumeName(std::string& sName,bool bUnicodeFormat=true);
void UnformatVolumeNameToDriveLetter(std::string&);

/// \brief registers this host with CS.
/// 
/// \returns 0 on success, -1 on failure
int RegisterHost(const std::string & onDemandRegistrationRequestId = std::string(), ///< on demand registration request id
                 const std::string & disksLayoutOption = std::string(),             ///< disks layout file option
                 QuitFunction_t qf = 0);                                            ///< quit function to use when retrying

SVERROR GetServiceRequestDataSize( unsigned long *);
SVERROR GetDirtyBlockThresholdSize( unsigned long *,char *);

void FormatNameToUNC(std::string&);
void FormatDeviceName(std::string&);

void ToStandardFileName(std::string&);

bool IsUNCFormat(const std::string&);
void FormatVolumeNameForMountPoint(std::string&);
void FormatVolumeNameForCxReporting(std::string&);

bool IsVolumeNameInGuidFormat(const std::string&);
bool IsDrive(const std::string&);
bool IsMountPoint(const std::string&);
bool FormatVolumeNameToGuid(std::string&);
void ExtractGuidFromVolumeName(std::string& sVolumeNameAsGuid);
bool IsVolumeMounted(const std::string volume,std::string& sMountPoint, std::string & mode);
bool IsValidDevfile(std::string devname);
dev_t GetDevNumber(std::string devname);
bool MountVolume(const std::string&,const std::string&,const std::string&,bool);
std::string ToUpper(const std::string&);
std::string ToLower(const std::string&);
bool IsSparseVolume(std::string Vol) ;
bool IsDirectoryExisting(const std::string&);
std::string getLocalTime();
std::string getTimeZone();
bool executeAceProcess(std::string &,std::string);

bool IsLeafDevice(const std::string&);
bool GetDeviceNameFromSymLink(std::string &linkName);
bool GetDeviceNameForMountPt(const std::string & mtpt, std::string & devName);
bool IsProcessRunning(const std::string &, uint32_t numExpectedInstances = 1);

SVERROR PostToSVServer( const char *pszSVServerName,
                        SV_INT HttpPort,
                        const char *pszPostURL,
                        char *pchPostBuffer,
                        SV_ULONG dwPostBufferLength,
                        const bool& pHttps);


/** Added by BSR - Fixing 5288 Issue **/
bool resolvePathAndVerify( std::string& pathName ) ;
/** End of the change **/


// Bug# 5527
bool SVUnlink( const std::string& filename );
// End of the change

void FirstCharToUpperForWindows(std::string & volume);

bool IsSparseFile(const std::string fileName);

// Bug # 6133 

void TruncateTrailingSlash(char *str);
size_t WriteMemoryCallbackFileReplication(void *ptr, size_t size, size_t nmemb, void *data);
//Modified forceSecure parameter from default to normal,all the callers has to pass this parameter based on configaration paramter
//Renamed Http port paramter to Port, so that callers can specify any port. 
SVERROR postToCx(const char *pszSVServerName,
                 SV_INT Port,
                 const char *pszPostURL,
                 const char* pszBuffer,
                 char** ppszBuffer,bool useSecure,
                 SV_ULONG suze=0,
                 SV_ULONG retryCount = 3
                 ) ; 

#ifdef SV_WINDOWS
#define FUNCTION_NAME __FUNCTION__
#else
#define FUNCTION_NAME __func__
#endif /* SV_WINDOWS */

#define EQUALS "="

void setdirectmode(unsigned long int &access);
void setdirectmode(int &mode);
void setsharemode_for_all(mode_t &share);
void setumask();

bool GetInstallVolCapacityAndFreeSpace(const std::string &installPath,
                                       unsigned long long &insVolFreeSpace,
                                       unsigned long long &insVolCapacity);
bool GetSysVolCapacityAndFreeSpace(std::string &sysVol,
                                   unsigned long long &sysVolFreeSpace,
                                   unsigned long long &sysVolCapacity);
bool GetSysVolCapacityAndFreeSpace(std::string &sysVol,
                                   std::string &sysDir,
                                   unsigned long long &sysVolFreeSpace, 
                                   unsigned long long &sysVolCapacity);
bool Tokenize(const std::string& input, std::vector<std::string>& outtoks,const std::string& separators);
int comparevolnames(const char *vol1, const char *vol2);
bool sameVolumeCheck(const std::string & vol1, const std::string & vol2);
int posix_fadvise_wrapper(ACE_HANDLE fd, SV_ULONGLONG offset, SV_ULONGLONG len, int advice);
bool IsColon( std::string::value_type ch );
bool IsQuestion( std::string::value_type ch );
bool IsReportingRealNameToCx(void);
bool GetLinkNameIfRealDeviceName(const std::string sVolName, std::string &sLinkName);
std::string GetRawVolumeName(const std::string &dskname);
bool ParseSparseFileNameAndVolPackName(const std::string lineinconffile, std::string &sparsefilename, std::string &volpackname);
bool IsCacheVolume(std::string const & volume);
bool IsValidMountPoint(const std::string & volume,std::string & errmsg);
bool IsInstallPathVolume(std::string const & volume);
bool IsPreAdded(const std::string &filename, const char &delim, std::string &prefile);
bool IsVirtual(std::string &hypervisorname, std::string &hypervisorvers);
bool IsAnyCmdSucceeded(const std::vector<std::string> &cmds, std::string &cmd);
bool HasPatternInCmd(const std::vector<std::string> cmds, 
                     Pats &pats, std::string &pat);
bool RunInmCommand(
                   const std::string& cmd,
                   std::string &outputmsg,
                   std::string& errormsg,
                   int & ecode
                   );
bool HasPatternInWMIDB(const std::vector<std::string> &wmis, 
                       Pats &pats);
bool GetBiosId(std::string& biosId) ;
bool GetBaseBoardId(std::string& baseboardId) ;
bool IsVmDeterministic(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsVmFromDeterministics(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsXenVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsVMFromCpuInfo(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsOpenVzVM(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsNativeVm(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers);
bool IsValidIP(const std::string &ip);
bool IsVolPackDevice(char const * deviceFile,std::string & sparsefile,bool & multisparse);
bool containsVolpackFiles(const std::string & volumename);
void GetFirstInpFromFile(const std::string &filename, NUMFORMAT nfmt, SV_LONGLONG *contents);
void RemoveChar(std::string &nwwn, const char &c);
void GetFirstStringFromFile(const std::string &filename, std::string &str);
void GetFirstLineFromFile(const std::string &filename, std::string &str);
void RemoveLastMatchingChar(std::string &s, const char &c);
bool GetSparseFilePath(std::string volumename,std::string & filename);
void PrintString(const std::string &s);
void Trim(std::string& s, const std::string & trim_chars = " \n\b\t\a\r\xc") ;
bool isnotxdigit(const char &c);
bool IsxString(const std::string &s);
bool IsNotxString(const std::string &s);
std::string GetUpperCase(const std::string &str);
const char *GetStartOfLastNumber(const char *cs);
std::string RemoveSuffix(const std::string &s, const std::string &suffix);
bool AreAllDigits(const char *p);
SVERROR getFileContent( std::string filePath, std::string& content );
bool MaskRequiredSignals(void);
//bool IsNewSparseFileFormat(const std::string vol_name);
void PrintStatuses(const Statuses_t &ss, SV_LOG_LEVEL lvl);
void GetStatuses(const Statuses_t &ss, std::string &s);
std::list<std::string> getFirstLevelDirPathNameList(const std::string& baseDirPathName);
bool isFileExited(const std::string& fileNamePath);
void pruneFiles(const std::string& dirName, const std::string& date, bool recursive, bool bFirst = true);
std::string getSourceConfigFilesTimeFormat(SV_ULONGLONG timeValue);
void pruneDirs(const std::string& repoPath, const std::list<std::string>&);
bool RestoreNtfsOrFatSignature(const std::string & volumename);
int getvolpackproperties(const std::string & volpack_device,volpackproperties & );
void removeStringSpaces( std::string& s);
E_INM_TRISTATE DoesFileExist(const std::string &file, std::string &errmsg);
bool DoesFileExist(const std::string &file);
bool InmFtruncate(const std::string &file, std::string &errmsg);
bool InmRenameAndRemoveFile(const std::string &file, std::string &errmsg);
bool InmFopen(FILE **fpp, const std::string &name, const std::string &mode, std::string &errmsg);
bool InmFclose(FILE **fpp, const std::string &name, std::string &errmsg);
bool InmFprintf(FILE *fp, const std::string &name, std::string &errmsg, const bool &brestorefilepointer, const long &writepos, const char *fmt, ...);
bool SyncFile(const std::string &file, std::string &errmsg);
bool InmCopyTextFile(const std::string &src, const std::string &dest, const bool &busetemporary, std::string &errmsg);
bool InmRemoveFile(const std::string &file, std::string &errmsg);
void TrimChar(char *str, const char &c);
bool InmRename(const std::string &oldfile, const std::string &newfile, std::string &errmsg);
SVERROR WriteStringIntoFile(const std::string& fileName,const std::string& str) ;
void FormatVacpErrorCode( std::stringstream& stream, SV_ULONG& exitCode ) ;
bool enableDiableFileSystemRedirection(const bool bDisableRedirect= true);
std::string Escapexml(const std::string& value) ;
/* TODO: smbios gives manufacturer in all these below but 
* not using; may need to use later */
/* paterns */
#define HYPERVMANUFACTURER      "Microsoft Corporation"
#define HYPERVMODEL             "Virtual Machine"

#define VMWAREPAT "VMware"
#define XENPAT    "Xen"
#define MICROSOFTPAT "Microsoft"

#define VMWAREMFPAT "Manufacturer: VMware"
#define MICROSOFTCORPMFPAT "Manufacturer: Microsoft Corporation"
#define MICROSOFTMFPAT "Manufacturer: Microsoft"
#define VIRTUALBOXMFPAT "Manufacturer: innotek GmbH"
#define XENMFPAT "Manufacturer: Xen"

/* hypervisor names */
#define XENNAME                 "xen"
#define HYPERVNAME              "Microsoft HyperV"
#define VMWARENAME              "VMware"
#define VIRTUALPCNAME           "virtualpc"
#define VIRTUALBOXNAME          "virtualbox"
#define MICROSOFTNAME           "Microsoft"
#define PHYSICALMACHINE         "Physical Machine"

#define NELEMS(ARR) ((sizeof (ARR)) / (sizeof (ARR[0])))
#define NBOOLS 2

#define MAX_NETWORK_TRIES 3
#define TYPICAL_NADAPTERS 4

#define DSKDIR "/dsk/"
#define RDSKDIR "/rdsk/"
#define DMPDIR "/dmp/"
#define RDMPDIR "/rdmp/"

#define STRBOOL(B) ((B) ? "true" : "false")

typedef enum eVerifySizeAt
{
    E_ATMORETHANSIZE,
    E_ATLESSTHANSIZE

} E_VERIFYSIZEAT;

#define NBITSINBYTE 8

typedef struct _Bpb {
    unsigned char  BS_jmpBoot[3];  /* Jump instruction to the boot code */
    unsigned char  BS_OEMName[8];  /* The name of the system that formatted the volume */
    unsigned short BPB_BytsPerSec; /* How many bytes in a sector (should be 512) */
    unsigned char  BPB_SecPerClus; /* How many sectors are in a cluster (FAT-12 should be 1) */
    unsigned short BPB_RsvdSecCnt; /* Number of sectors that are reserved (FAT-12 should be 1) */
    unsigned char  BPB_NumFATs;    /* The number of FAT tables on the disk (should be 2) */
    unsigned short BPB_RootEntCnt; /* Maximum number of directory entries in the root directory */
    unsigned short BPB_TotSec16;   /* FAT-12 total number of sectors on the disk */
    unsigned char  BPB_Media;      /* Code for media type {fixed, removable, etc.} */
    unsigned short BPB_FATSz16;    /* FAT-12 number of sectors that each FAT table occupies (should be 9) */
    unsigned short BPB_SecPerTrk;  /* Number of sectors in one cylindrical track */
    unsigned short BPB_NumHeads;   /* Number of heads for this volume (2 heads on a 1.4Mb 3.5 inch floppy) */
    unsigned long  BPB_HiddSec;    /* Number of preceding hidden sectors (0 for non-partitioned media) */
    unsigned long  BPB_TotSec32;   /* FAT-32 number of sectors on the disk (0 for FAT-12) */
    unsigned char  BS_DrvNum;      /* A drive number for the media (OS specific) */
    unsigned char  BS_Reserved1;   /* Reserved space for Windows NT (when formatting, set to 0) */
    unsigned char  BS_BootSig;     /* Indicates that the following three fields are present (0x29) */
    unsigned long  BS_VolID;       /* Volume serial number (for tracking this disk) */
    unsigned char  BS_VolLab[11];  /* Volume label (matches label in the root directory, or "NO NAME    ") */
    unsigned char  BS_FilSysType[8];/* Deceptive FAT type Label that may or may not indicate the FAT type */
} Bpb;

void InmHexStrToUll(const std::string &s, unsigned long long &ull);

typedef int (*DeQ_t)( ACE_Message_Queue<ACE_MT_SYNCH> * Q, ACE_Message_Block **mb, ACE_Time_Value &absolutewaittime, ACE_Time_Value *pwaittime);
int DeQWithRecordedWaitTime( ACE_Message_Queue<ACE_MT_SYNCH> * Q, ACE_Message_Block **mb, ACE_Time_Value &absolutewaittime, ACE_Time_Value *pwaittime);
int DirectDeQ( ACE_Message_Queue<ACE_MT_SYNCH> * Q, ACE_Message_Block **mb, ACE_Time_Value &absolutewaittime, ACE_Time_Value *pwaittime);

ACE_Message_Block * DeQ( ACE_Message_Queue<ACE_MT_SYNCH> * Q, const int &waitsecs, const bool &getwaittime = false, ACE_Time_Value *pwaittime = 0);
bool EnQ(ACE_Message_Queue<ACE_MT_SYNCH> *Q, ACE_Message_Block *mb, const int &waitsecs, QuitFunction_t &qf);
int RemoveDirectories(const std::string &parentdirectory, const std::set<std::string> &exceptionlist);
bool IsFileORVolumeExisting(const std::string& sName);

typedef int (*CompareMemory_t)(const void *s1, const void *s2, size_t n);

/* return values same as memcmp */
int comparememory(const void *s1, const void *s2, size_t n);

/* Alloc page aligned memory. Windows version uses C++ new. valloc is used for unix */
char* AllocatePageAlignedMemory(const size_t &length);

/* Free the memory allocated by AllocatePageAligned() */
void FreePageAlignedMemory(char *readdata);

/* return that there is a change;
 * do not compare, return values 
 * are same as that of memcmp */
int alwaysreturnunequal(const void *s1, const void *s2, size_t n);
bool GetMD5Hash(const std::string &fileNamePath, unsigned char *hash);
std::string GetBytesInArrayAsHexString(const unsigned char *a, const size_t &length);
bool ConvertToSVTime(SV_ULONGLONG ts, SV_TIME &svTime);

void PrintOptions(const Options_t &options);

template <class KEY, class VALUE, class COMP>
 void InsertIntoStream(const std::map<KEY, VALUE, COMP> &m, std::ostream& o)
 {
    o << "map (key --> value):\n";
    for (typename std::map<KEY, VALUE, COMP>::const_iterator it = m.begin(); it != m.end(); it++)
        o << it->first << " --> " << it->second << '\n';
 }

 std::string getSystemTimeInUtc(void);
 bool ConvertTimeToString(SV_ULONGLONG ts, std::string & display);

class CollectExistingFiles
{
public:
    std::vector<std::string>::size_type size(void)
    {
        return m_ExistingFiles.size();
    }

    std::string operator[](const std::vector<std::string>::size_type &i)
    {
        return m_ExistingFiles[i];
    }

    void operator()(const std::string& file)
    {
        /* TODO: consider using DoesFileExist version that gives exists, does not exist and cannot say */
        if (DoesFileExist(file))
            m_ExistingFiles.push_back(file);
    }

private:
    std::vector<std::string> m_ExistingFiles;
};

void replace_nonsupported_chars(std::string & filename,
          const std::string & chars_to_replace = "\\/:*?\" <>|", char replaced_char = '_');

void RecordFilterDrvVolumeBitmapDelMsg(const std::string &msg);

std::string GetIOSState(std::ios &stream);

bool IsRecoveryInProgress();

bool IsItTestFailoverVM();

std::string GetRecoveryScriptCmd();

std::string GenerateUuid();

std::string GetSystemUUID();

std::string GetChassisAssetTag();

uint64_t GetTimeInMilliSecSinceEpoch1970();

uint64_t GetTimeInSecSinceEpoch1970();

uint64_t GetTimeInSecSinceEpoch1601();

uint64_t GetTimeInSecSinceAd0001();

uint64_t GetSecsBetweenEpoch1970AndEpoch1601();

bool PersistPlatformTypeForDriver();

int GetDeviceNameTypeToReport(const std::string& hypervisorName = std::string(), const bool isMigrateToRcm = false);

bool IsAzureVirtualMachine();

std::string GetImdsMetadata();

bool IsAzureStackVirtualMachine();

bool IsAgentRunningOnAzureVm();

std::string GetCxIpAddress();

bool DeleteProtectedDeviceDetails();
std::string  InmGetFormattedSize(unsigned long long ullSize);

bool IsUEFIBoot(void);

void ExtractCacheStorageNameFromBlobContainerSasUrl(const std::string& blobContainerSasUri, std::string& cacheStorageAccountName);

#endif //PORTABLEHELPERS__H
