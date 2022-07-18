//
// volumegroupsettings.h: declare configurator interface to volume_group_settings
//
#ifndef VOLUME_GROUP_SETTINGS_H
#define VOLUME_GROUP_SETTINGS_H

#include <list>
#include <string>
#include <map>
#include "svtypes.h"
#include "portablehelpers.h"
#include "transport_settings.h"
#include "transportprotocols.h"
#include "compressmode.h"
#include "inmstrcmp.h"

struct VOLUME_SETTINGS;
struct VOLUME_GROUP_SETTINGS;

struct THROTTLE_SETTINGS
{
    bool throttleResync;
    bool throttleSource;
    bool throttleTarget;

    THROTTLE_SETTINGS();
    THROTTLE_SETTINGS(const THROTTLE_SETTINGS&);
    bool operator==( THROTTLE_SETTINGS const&) const;
    bool IsResyncThrottled(void) const ;
    bool IsSourceThrottled(void) const ;
    bool IsTargetThrottled(void) const ;
};

enum HOSTTYPE {HOSTTARGET=1, HOSTSOURCE=2};
enum CDP_DIRECTION { UNSPECIFIED, SOURCE, TARGET };
enum CDP_STATUS { PROTECTED, UNPROTECTED };
typedef std::pair<int, long long> JOB_ID_OFFSET;

typedef std::map<std::string,std::string> VolumeNameMountPointMap;
typedef std::pair<std::string,std::string> VolumeNameMountPointPair;
typedef std::map<std::string,std::string> VolumeNameFileSystemMap;
typedef std::pair<std::string,std::string> VolumeNameFileSystemPair;

int const INVALID_GROUP_SETTINGS_ID = -1;

struct SAN_VOLUME_INFO
{
    bool isSanVolume;
    std::string physicalDeviceName;
    std::string mirroredDeviceName;
    std::string virtualDevicename;
    std::string virtualName;
    std::string physicalLunId;


    SAN_VOLUME_INFO():isSanVolume(false) {}

    SAN_VOLUME_INFO& operator=( SAN_VOLUME_INFO const& rhs )
        {
            if(this == &rhs)
                return *this;
            isSanVolume = rhs.isSanVolume;
            physicalDeviceName = rhs.physicalDeviceName;
            virtualDevicename = rhs.virtualDevicename;
            mirroredDeviceName = rhs.mirroredDeviceName;
            virtualName = rhs.virtualName;
            physicalLunId = rhs.physicalLunId;
            return *this;
        }

    bool operator==(SAN_VOLUME_INFO const& rhs) const
        {
            return  isSanVolume == rhs.isSanVolume
                && physicalDeviceName == rhs.physicalDeviceName
                && mirroredDeviceName == rhs.mirroredDeviceName
                && virtualDevicename == rhs.virtualDevicename
                && virtualName == rhs.virtualName
                && physicalLunId == rhs.physicalLunId;
        }


};

/* asking statistics from AT by CX for source lun scsi id and pi pwwns; 
 * this is not inside volumesettings since this can be a potential 
 * separate structure */
struct ATLUN_STATS_REQUEST
{
    /* TODO: should we have other good numbers as per CX ? */
    enum ATLUN_STATS_REQUEST_TYPE  {ATLUN_STATS_NOREQUEST = 0, /* no request; also the state when previous request satisfied */
                                    ATLUN_STATS_LASTIO_TS};    /* add set of request; generic response can be string */
    ATLUN_STATS_REQUEST_TYPE type;
    std::string atLUNName;
    std::list<std::string> physicalInitiatorPWWNs;

    ATLUN_STATS_REQUEST();
    ~ATLUN_STATS_REQUEST();
    ATLUN_STATS_REQUEST & operator=(const ATLUN_STATS_REQUEST &rhs);
    bool operator==(const ATLUN_STATS_REQUEST &rhs) const;
};

/* SRM: start*/
/* Structure for getting FLUSH_AND_HOLD request settings*/
struct FLUSH_AND_HOLD_REQUEST
{
    enum FLUSH_AND_HOLD_REQUEST_TYPE {APPLICATION = 1,CRASH_CONSISTENCY};
    FLUSH_AND_HOLD_REQUEST_TYPE m_consistency_type;
    std::string m_timeto_pause_apply;
    SV_UINT m_apptype;
    std::string m_bookmark;

    FLUSH_AND_HOLD_REQUEST();
    ~FLUSH_AND_HOLD_REQUEST();
    FLUSH_AND_HOLD_REQUEST(const FLUSH_AND_HOLD_REQUEST&);
    bool operator==( FLUSH_AND_HOLD_REQUEST const&) const;
};

/* TODO: should have been static inside volume settings struct ? */
namespace NsVOLUME_SETTINGS
{
    /* name value pairs for options listed in order */
    const char NAME_RESYNC_COPY_MODE[] = "resync_copy_mode";
    const char VALUE_FULL[] = "full";
    const char VALUE_FILESYSTEM[] = "filesystem";

    const char NAME_PROTECTION_DIRECTION[] = "protection_direction";
    const char VALUE_FORWARD[] = "forward";
    const char VALUE_REVERSE[] = "reverse";

    const char NAME_TARGET_DATA_PLANE[] = "target_data_plane";
    const char VALUE_UNSUPPORTED_DATA_PLANE[] = "unsupported_data_plane";
    const char VALUE_AZURE_DATA_PLANE[] = "azure_data_plane";
    const char VALUE_INMAGE_DATA_PLANE[] = "inmage_data_plane";

    /* TODO: source and target should get other party's attributes like capacity etc, from end points */
    const char NAME_RAWSIZE[] = "rawsize";
    /* the value of this is a 64 bit number */
};

const char * const StrResyncCopyMode[] = {"Unknown", "Full", "FileSystem"};
const char * const StrProtectionDirection[] = {"Unknown", "Forward", "Reverse"};

struct VOLUME_SETTINGS
{
    enum SECURE_MODE { SECURE_MODE_NONE, SECURE_MODE_SECURE };

    //Adding new Fast Sync Algorithm by BSR for Target Based checksum ( SYNC_FAST_TBC )
    enum SYNC_MODE { SYNC_DIFF, SYNC_OFFLOAD, SYNC_FAST, SYNC_SNAPSHOT, SYNC_DIFF_SNAPSHOT, SYNC_QUASIDIFF, SYNC_FAST_TBC , SYNC_DIRECT };
    enum RESYNCREQ_CAUSE { RESYNCREQUIRED_BYSOURCE, RESYNCREQUIRED_BYUSER, RESYNCREQUIRED_BYTARGET };
    enum TARGET_DATA_PLANE { UNSUPPORTED_DATA_PLANE, INMAGE_DATA_PLANE, AZURE_DATA_PLANE };

    //Bug #6298
    //added new states here for Bug 6890
    enum PAIR_STATE { UNKNOWN = -1, DELETE_PENDING = 24, CLEANUP_DONE = 25, CLEANUP_FAILED = -CLEANUP_DONE,
                      PAUSE = 26, PAIR_PROGRESS = 0,
                      LOG_CLEANUP_COMPLETE = 32, LOG_CLEANUP_FAILED = -LOG_CLEANUP_COMPLETE,
                      UNLOCK_COMPLETE = 34, UNLOCK_FAILED = -UNLOCK_COMPLETE,
                      CACHE_CLEANUP_COMPLETE = 36, CACHE_CLEANUP_FAILED = -CACHE_CLEANUP_COMPLETE,
                      VNSAP_CLEANUP_COMPLETE  = 38, VNSAP_CLEANUP_FAILED = -VNSAP_CLEANUP_COMPLETE,
                      PENDING_FILES_CLEANUP_COMPLETE = 40, PENDING_FILES_CLEANUP_FAILED = -PENDING_FILES_CLEANUP_COMPLETE,
                      PAUSE_PENDING = 41, RESTART_RESYNC_CLEANUP = 42, PAUSE_TACK_PENDING = 48, PAUSE_TACK_COMPLETED = 50, RESUME_PENDING = 51, 
                      FLUSH_AND_HOLD_WRITES_PENDING = 71, FLUSH_AND_HOLD_WRITES = 72,FLUSH_AND_HOLD_RESUME_PENDING=73}; /*SRM: Added three new states */

    enum RESYNC_COPYMODE { RESYNC_COPYMODE_UNKNOWN, RESYNC_COPYMODE_FULL, RESYNC_COPYMODE_FILESYSTEM };
    enum PROTECTION_DIRECTION { PROTECTION_DIRECTION_UNKNOWN, PROTECTION_DIRECTION_FORWARD, PROTECTION_DIRECTION_REVERSE };

    typedef std::list<VOLUME_SETTINGS> endpoints_t;
    typedef endpoints_t::iterator endpoints_iterator;
    typedef endpoints_t::const_iterator endpoints_constiterator;
    typedef std::map<std::string, std::string> options_t;
    
    std::string deviceName;
    std::string mountPoint;
    std::string fstype;
    std::string hostname;
    std::string hostId;
    SECURE_MODE secureMode;
    SECURE_MODE sourceToCXSecureMode;
    SYNC_MODE syncMode;
    TRANSPORT_PROTOCOL transportProtocol;
    int visibility;
    SV_ULONGLONG sourceCapacity;
    std::string resyncDirectory;        // not in use yet
    int rpoThreshold;

    SV_ULONGLONG srcResyncStarttime;
    SV_ULONGLONG srcResyncEndtime;
    SV_ULONG OtherSiteCompatibilityNum;

    //to hold resync required info
    bool resyncRequiredFlag ;
    RESYNCREQ_CAUSE resyncRequiredCause;
    SV_ULONGLONG resyncRequiredTimestamp;
    OS_VAL sourceOSVal;
    COMPRESS_MODE compressionEnable;
    /** Fast Sync TBC - BSR **/
    std::string jobId ;
    /** End of the change **/
    SAN_VOLUME_INFO sanVolumeInfo;
    RESYNCREQ_CAUSE DefaultResyncRequiredCause()  {return RESYNCREQUIRED_BYSOURCE;}

    endpoints_t endpoints;

    //Bug #6298
    PAIR_STATE pairState;
    //added new string to get the requested string from CX for the bug 6890
    std::string cleanup_action;
    SV_ULONGLONG diffsPendingInCX;
    SV_ULONGLONG currentRPO;
    SV_ULONGLONG applyRate;
    std::string maintenance_action;
    SV_ULONGLONG srcResyncStartSeq;
    SV_ULONGLONG srcResyncEndSeq;
    THROTTLE_SETTINGS throttleSettings;
    SV_ULONGLONG resyncCounter;
    SV_ULONGLONG sourceRawSize;
    ATLUN_STATS_REQUEST atLunStatsRequest; /* s2 will answer the request as soon as it starts;
                                            * if request is satisfied again settings will change.
                                            * cx sets ATLUN_STATS_REQUEST_TYPE to no request.
                                            * and s2 quits and comes up not requiring to send any
                                            * response */
    SV_ULONGLONG srcStartOffset;

    int devicetype;/* To identify the volume type */    
    options_t options;

    bool isFabricVolume()const {
        return !sanVolumeInfo.virtualName.empty(); }
    VOLUME_SETTINGS();
    ~VOLUME_SETTINGS();
    bool operator==(VOLUME_SETTINGS const&) const;
    bool strictCompare(VOLUME_SETTINGS const & ) const;
    bool shouldOperationRun(const std::string & operationName);
    VOLUME_SETTINGS(const VOLUME_SETTINGS&);
    VOLUME_SETTINGS& operator=(const VOLUME_SETTINGS&);

    /* NOTE: for options, call the function once
     * cache the return, since we cannot store
     * anything in VOLUME_SETTINGS, every time
     * search in options is done */
    RESYNC_COPYMODE GetResyncCopyMode(void) const;
    PROTECTION_DIRECTION GetProtectionDirection(void) const;
    SV_ULONGLONG GetRawSize(void) const;
    SV_ULONGLONG GetEndpointRawSize(void) const;
    std::string GetEndpointDeviceName(void) const;
    TARGET_DATA_PLANE getTargetDataPlane(void) const;
    int GetEndpointDeviceType(void) const;
    std::string GetEndpointHostId(void) const;
    std::string GetEndpointHostName(void) const;
    std::string GetResyncJobId(void) const { return jobId; }
    int GetDeviceType(void) const { return devicetype; }

    bool isAzureDataPlane() const { return getTargetDataPlane() == AZURE_DATA_PLANE; }
    bool isInMageDataPlane() const { return getTargetDataPlane() == INMAGE_DATA_PLANE; }
    bool isUnSupportedDataPlane() const { return getTargetDataPlane() == UNSUPPORTED_DATA_PLANE; }
};

struct VOLUME_GROUP_SETTINGS
{
    typedef std::map<std::string,VOLUME_SETTINGS> volumes_t;
    typedef volumes_t::iterator volumes_iterator;
    typedef volumes_t::const_iterator volumes_constiterator;
    int id;
    CDP_DIRECTION direction;
    CDP_STATUS status;
    volumes_t volumes;
    TRANSPORT_CONNECTION_SETTINGS transportSettings;
    VOLUME_GROUP_SETTINGS();
    ~VOLUME_GROUP_SETTINGS();
    bool operator==(VOLUME_GROUP_SETTINGS const&) const;
    bool strictCompare(VOLUME_GROUP_SETTINGS const&) const;
    bool strictVolumesCompare(VOLUME_GROUP_SETTINGS const&) const;
    VOLUME_GROUP_SETTINGS(const VOLUME_GROUP_SETTINGS&);
    VOLUME_GROUP_SETTINGS& operator=(const VOLUME_GROUP_SETTINGS&);
    volumes_t::iterator find_by_guid(const std::string devicename);
    volumes_t::iterator find_by_name(const std::string devicename);
};

struct HOST_VOLUME_GROUP_SETTINGS
{
    typedef std::list<VOLUME_GROUP_SETTINGS> volumeGroups_t;    // not a map<groupId,VGS> because source volumes don't have a volume group id
    typedef volumeGroups_t::iterator volumeGroups_iterator;
    typedef volumeGroups_t::const_iterator volumeGroups_constiterator;
    volumeGroups_t volumeGroups;
    HOST_VOLUME_GROUP_SETTINGS();
    ~HOST_VOLUME_GROUP_SETTINGS();
    HOST_VOLUME_GROUP_SETTINGS(const HOST_VOLUME_GROUP_SETTINGS&);
    HOST_VOLUME_GROUP_SETTINGS& operator=(const HOST_VOLUME_GROUP_SETTINGS&);
    bool operator==(HOST_VOLUME_GROUP_SETTINGS const&) const;
    bool strictCompare(HOST_VOLUME_GROUP_SETTINGS const&) const;
    int getVolumeAttributeChangeSettings(std::string driveName);
    std::string getSyncDirectory(std::string drivename);
    bool isProtectedVolume(const std::string & deviceName);
    bool isTargetVolume(std::string driveName);
    bool isResyncing(std::string driveName);
    bool isInResyncStep1(std::string driveName);
    bool isSourceFullDevice(const std::string & deviceName);
    int getRpoThreshold(std::string driveName);
    SV_ULONGLONG getDiffsPendingInCX(std::string driveName);
    SV_ULONGLONG getCurrentRpo(std::string driveName);
    SV_ULONGLONG getApplyRate(std::string driveName);
    SV_ULONGLONG getResyncCounter(const std::string& deviceName);
    std::string getSourceHostIdForTargetDevice(const std::string& deviceName);
    std::string getSourceVolumeNameForTargetDevice(const std::string& deviceName);
    VOLUME_SETTINGS::PAIR_STATE getPairState(const std::string & driveName);
    OS_VAL getSourceOSVal(const std::string & driveName);
    SV_ULONG getOtherSiteCompartibilityNumber(std::string driveName);
    std::string getVolumeMountPoint(const std::string& deviceName);
    std::string getSourceFileSystem(const std::string& deviceName);
    SV_ULONGLONG getSourceCapacity(const std::string & deviceName);
    SV_ULONGLONG getSourceRawSize(const std::string & deviceName);
    std::map<std::string, std::string> getReplicationPairInfo(const std::string & sourceHost);
    std::map<std::string, std::string> getSourceVolumeDeviceNames(const std::string & targetHost);
    void getVolumeNameAndMountPointForAll(VolumeNameMountPointMap &volumeNameMountPointMap );
    void getVolumeNameAndFileSystemForAll(VolumeNameFileSystemMap &volumeNameFileSystemMap );
    TRANSPORT_CONNECTION_SETTINGS getTransportSettings(const std::string &deviceName);
    TRANSPORT_CONNECTION_SETTINGS getTransportSettings(int volGrpId);
    bool gettargetvolumes(std::vector<std::string> &);

    SV_ULONGLONG getResyncStartTimeStamp(const std::string &deviceName);
    SV_ULONGLONG getResyncEndTimeStamp(const std::string &deviceName) ;
    SV_ULONG getResyncStartTimeStampSeq(const std::string &deviceName);
    SV_ULONG getResyncEndTimeStampSeq(const std::string &deviceName);
    TRANSPORT_PROTOCOL getProtocol(const std::string &deviceName);
    bool getSecureMode(const std::string &deviceName);
    bool isInQuasiState(const std::string &deviceName);
    bool isResyncRequiredFlag(const std::string &deviceName);
    SV_ULONGLONG getResyncRequiredTimestamp(const std::string &deviceName);
    VOLUME_SETTINGS::RESYNCREQ_CAUSE getResyncRequiredCause(const std::string &deviceName);
    VOLUME_SETTINGS::TARGET_DATA_PLANE getTargetDataPlane(const std::string & devicename);
    SV_ULONGLONG GetEndpointRawSize(const std::string & devicename) const;
    std::string GetEndpointDeviceName(const std::string & devicename) const;
    std::string GetEndpointHostId(const std::string & devicename) const;
    std::string GetEndpointHostName(const std::string & devicename) const;
    std::string GetResyncJobId(const std::string & devicename) const;
    int GetDeviceType(const std::string & devicename) const;
};

struct NwwnPwwn
{
    std::string m_Nwwn;
    std::string m_Pwwn;
};
typedef NwwnPwwn NwwnPwwn_t;
typedef std::vector<NwwnPwwn> NwwnPwwns_t;


typedef std::string Name_t;
typedef std::string Value_t;
typedef std::map<Name_t, Value_t> Attributes_t;
struct Object
{
    Attributes_t m_Attributes;
    void Print(const SV_LOG_LEVEL LogLevel = SV_LOG_DEBUG);
    void Reset(void);
};
typedef std::vector<Object> Objects_t;
typedef Objects_t::iterator ObjectsIter_t;
typedef Objects_t::const_iterator ConstObjectsIter_t;

typedef Attributes_t::iterator AttributesIter_t;
typedef Attributes_t::const_iterator ConstAttributesIter_t;

struct VolumeSummary;
typedef std::vector<VolumeSummary> VolumeSummaries_t;
typedef VolumeSummaries_t::const_iterator ConstVolumeSummariesIter;
typedef VolumeSummaries_t::iterator VolumeSummariesIter;

/*
 * TODO:
 * 1. Function that checks bounds 
 */
const char * const StrDeviceType[] = {
    "Disk", "Vsnap Mounted", "Vsnap UnMounted",
    "Unknown", "Partition", "Logical Volume",
    "Volpack", "Custom", "Extended Partition",
    "Disk Partition", "Docker Disk"
};

const char * const StrVendor[] = {
    "Unknown", "Native", "DevMapper", "Mpxio", "EmcPowerpath", 
    "Hdlm", "Sun Cluster DID", "Sun Cluster Global", 
    "Veritas DMP", "Solaris Volume Manager", 
    "Veritas Volume Manager", "LVM", 
    "InMage Volpack", "InMage Vsnap", "Custom", "ASM",
    "ZFS", "DevMapper Docker"
};

const char * const StrWCState[] = {"Do not know", "Disabled", "Enabled"};

const char * const StrFormatLabel[] = {"Unknown", "SMI", "EFI", "MBR", "GPT", "RAW"};

const char * const StrNameBasedOn[] = {"signature", "scsiid", "scsihctl"};

namespace NsVolumeAttributes
{
    const char INTERFACE_TYPE[] = "interface_type";
    const char SCSI_BUS[] = "scsi_bus";
    const char SCSI_LOGICAL_UNIT[] = "scsi_logical_unit";
    const char SCSI_PORT[] = "scsi_port";
    const char SCSI_TARGET_ID[] = "scsi_target_id";
    const char STORAGE_TYPE[] = "storage_type";                        /* to intimate basic / dynamic disks for windows */
    const char TOTAL_CYLINDERS[] = "total_cylinders";
    const char TOTAL_HEADS[] = "total_heads";
    const char TOTAL_SECTORS[] = "total_sectors";
    const char SECTORSPERTRACK[] = "sectors_per_track";
    const char HAS_BOOTABLE_PARTITION[] = "has_bootable_partition";    /* true or false */
    const char MANUFACTURER[] = "manufacturer";
    const char MODEL[] = "model";
    const char HAS_UEFI[] = "has_uefi";
    const char MEDIA_TYPE[] = "media_type";
    const char IS_SHARED[] = "is_shared";
    const char DEVICE_NAME[] = "display_name";                          /* Windows full disk case: holds \\.\physicaldrive<n>. For unix, holds disk name. 
                                                                           NOTE: key name is display_name, because CS needs it that way. Windows agent code 
                                                                           uses this as device name to open and read */
    const char NAME_BASED_ON[] = "name_based_on";                       /* For Master Target, device name is created using disk scsi id. 
                                                                        For Source, device name is created using disk signature*/

    /* values */
    const char DYNAMIC[] = "dynamic";
    const char BASIC[] = "basic";

    const char SIGNATURE[] = "signature";
    const char SCSIID[] = "scsiid";
    const char SCSIHCTL[] = "scsihctl";
    const char PERSISTENT_NAME_MAP[] = "persistent_name_map";
    const char SERIAL_NUMBER[] = "serial_number";
    const char IS_RESOURCE_DISK[] = "is_resource_disk";             /* for Azure VM resource disk identification */
    const char IS_BEK_VOLUME[] = "is_bek_volume";
    const char IS_ISCSI_DISK[] = "is_iscsi_disk";
    const char IS_NVME_DISK[] = "is_nvme_disk";

    const char ENCRYPTION[] = "encryption";
    const char BITLOCKER_PROT_STATUS[] = "bitlocker_prot_status";
    const char BITLOCKER_CONV_STATUS[] = "bitlocker_conv_status";

    const char MSFT_UNIQUEID[] = "msft_uniqueid";       /*WMI class MSFT_PhysicalDisk UniqueId*/

    const char SCSI_UUID[] = "scsi_uuid";  // SCSI UUID from page 83
};


struct VolumeSummary
{
    typedef enum vendor
    {
        UNKNOWN_VENDOR, NATIVE, DEVMAPPER, 
        MPXIO, EMC, HDLM, DEVDID, 
        DEVGLOBAL, VXDMP, SVM, 
        VXVM, LVM, INMVOLPACK,
        INMVSNAP, CUSTOMVENDOR, ASM, ZFS,
        DEVMAPPERDOCKER

    } Vendor;

    /* TODO: names can be changed to OS_DISK/FDISK_PARTITION/...;
     * for now to go ahead, using this name */
    typedef enum devicetype
    {
        DISK, VSNAP_MOUNTED, VSNAP_UNMOUNTED, 
        UNKNOWN_DEVICETYPE, PARTITION, LOGICAL_VOLUME, 
        VOLPACK, CUSTOM_DEVICE, EXTENDED_PARTITION, 
        DISK_PARTITION, DOCKER_DISK

    } Devicetype;

    typedef enum writecache
    {
        WRITE_CACHE_DONTKNOW,
        WRITE_CACHE_DISABLED,
        WRITE_CACHE_ENABLED

    } WriteCacheState;

    typedef enum formatlabel
    {
        LABEL_UNKNOWN,
        SMI,
        EFI,
        MBR,
        GPT,
        LABEL_RAW

    } FormatLabel;

    typedef enum nameBasedOn
    {
        SIGNATURE,
        SCSIID,
        SCSIHCTL,
        PERSISTENT_NAME_MAP    
    } NameBasedOn;

    // UINT32
    typedef enum bitlockerProtectionStatus
    {
        // The volume is not encrypted, partially encrypted, or the volume's
        // encryption key for the volume is available in the clear on the hard disk. 
        PROTECTION_OFF = 0,

        // The volume is fully encrypted and the encryption key for the volume
        // is not available in the clear on the hard disk.
        PROTECTION_ON = 1,

        // The volume protection status cannot be determined. One potential cause
        // is that the volume is in a locked state.
        PROTECTION_UNKNOWN = 2,

        PROTECTION_COUNT
    } BitlockerProtectionStatus;

    // UINT32
    typedef enum bitlockerConversionStatus
    {
        // For a standard hard drive(HDD), the volume is fully decrypted. For a
        // hardware encrypted hard drive(EHDD), the volume is perpetually unlocked.
        CONVERSION_FULLY_DECRYPTED = 0,

        // For a standard hard drive(HDD), the volume is fully encrypted. For a
        // hardware encrypted hard drive(EHDD), the volume is not perpetually unlocked.
        CONVERSION_FULLY_ENCRYPTED = 1,

        // The volume is partially encrypted.
        CONVERSION_ENCRYPTION_IN_PROGRESS = 2,

        // The volume is partially encrypted.
        CONVERSION_DECRYPTION_IN_PROGRESS = 3,

        // The volume has been paused during the encryption progress. The volume
        // is partially encrypted.
        CONVERSION_ENCRYPTION_PAUSED = 4,

        // The volume has been paused during the decryption progress. The volume
        // is partially encrypted.
        CONVERSION_DECRYPTION_PAUSED = 5,

        CONVERSION_COUNT
    } BitlockerConversionStatus;

    VolumeSummary()
        {
            type = UNKNOWN_DEVICETYPE;
            vendor = UNKNOWN_VENDOR;
            isMounted = false; 
            isSystemVolume = false;
            isCacheVolume = false;
            capacity = 0;
            locked = false;
            physicalOffset = 0;
            sectorSize = 0;
            containPagefile = false;
            isStartingAtBlockZero = false;
            volumeGroupVendor = UNKNOWN_VENDOR;
            isMultipath = false;
            rawcapacity = 0;
            writeCacheState = WRITE_CACHE_DONTKNOW;
            formatLabel = LABEL_UNKNOWN;
        }

    /*
     * some properties like system volume, cachevolume, locked, have to be propagated back to parents
     * and from parents to children by volumeinfocollector
     */
    std::string id;                     /* scsi id for disk if found. if not found empty (FIXME: ID for IDE/PATA/SATA/others ?)*/
    std::string name;                   /* device name */
    Devicetype type;                    /* disk, partition or lv */
    Vendor vendor;                      /* vendor type (if can be determined). For lvs too. Need to know if string is needed ? */
    std::string fileSystem;             /* target unmounting, cdpcli and CX should consider file system among all matching scsi ids */
    std::string mountPoint;             /* same as above */
    bool isMounted;                     /* same as above */
    bool isSystemVolume;                /* same as above. Also swap, boot and dump devices to be system volumes */
    bool isCacheVolume;                 /* same as above */
    /*TODO: make capacity and freeSpace as unsigned quantities after 
     * checking impact */
    SV_LONGLONG capacity;               /* same as above */
    bool locked;                        /* same as above */
    SV_LONGLONG physicalOffset;         /* TODO: Need to find purpose */
    int sectorSize;                     /* TODO: Need to find purpose */
    std::string volumeLabel;            /* Used only in windows */
    bool containPagefile;               /* Used only in windows */
    bool isStartingAtBlockZero;         /* Although previously added for partitions, disks should not make this true */
    std::string volumeGroup;            /* disk groups. need to check if these can be multiple */
    Vendor volumeGroupVendor;           /* vendor of the volume group; needed since across 2 vendors, volume group can be same */
    bool isMultipath;                   /* true if this is multipath */
    SV_LONGLONG rawcapacity;            /* already present */
    WriteCacheState writeCacheState;    /* write cache state */
    FormatLabel formatLabel;            /* format label: efi or smi or unknown */
    Attributes_t attributes;            /* attributes specific to device */
    VolumeSummaries_t children;         /* children */
};

struct VolumeDynamicInfo;
typedef std::vector<VolumeDynamicInfo> VolumeDynamicInfos_t;
typedef VolumeDynamicInfos_t::const_iterator ConstVolumeDynamicInfosIter;
typedef VolumeDynamicInfos_t::iterator VolumeDynamicInfosIter;

struct VolumeDynamicInfo
{
    VolumeDynamicInfo()
        {
            freeSpace = 0;
        }

    /* NOTE: below 2 members id and name 
     * should be same as that of VolumeSummary; 
     * since these two update same relation in 
     * configuration store; This is for now;
     * This has to be changed to maintain a
     * map */
    std::string id;                        /* scsi id for disk if found. if not found empty (FIXME: ID for IDE/PATA/SATA/others ?)*/
    std::string name;
    SV_LONGLONG freeSpace;
    VolumeDynamicInfos_t children;         /* children */
};

struct VgInfo
{
    VolumeSummary::Vendor Vendor;
    std::string Name;

    VgInfo()
        : Vendor(VolumeSummary::UNKNOWN_VENDOR)
        {
        }
 
    VgInfo(const VolumeSummary::Vendor &vendor, const std::string &name)
        : Vendor(vendor), 
          Name(name)
        {
        }
};
typedef std::map<std::string, VgInfo> DeviceVgInfos;
typedef DeviceVgInfos::const_iterator ConstDeviceVgInfosIter;
typedef DeviceVgInfos::iterator DeviceVgInfosIter;

const char * const StrHypervisorState[] = {"Do not know", "Not present", "Present"};

typedef struct HypervisorInfo
{
    typedef enum hypervisorstate
    {
        HypervisorStateUnknown,
        HypervisorNotPresent,
        HyperVisorPresent

    } HypervisorState;

    HypervisorState state;
    std::string name;
    std::string version;

    HypervisorInfo()
        {
            state = HypervisorStateUnknown;
        }

} HypervisorInfo_t;


/* map from hardware address of nic to its attributes */
typedef std::map<std::string, Objects_t> NicInfos_t;
typedef NicInfos_t::iterator NicInfosIter_t;
typedef NicInfos_t::const_iterator ConstNicInfosIter_t;

/* map from cpu id (unique inside system; also changes across reboot)
 * to cpu info */
typedef std::map<std::string, Object> CpuInfos_t;
typedef CpuInfos_t::iterator CpuInfosIter_t;
typedef CpuInfos_t::const_iterator ConstCpuInfosIter_t;

namespace NSCpuInfo
{
    const char NAME[] = "name";
    const char ARCHITECTURE[] = "architecture";
    const char MANUFACTURER[] = "manufacturer";
    const char NUM_CORES[] = "number_of_cores";
    const char NUM_LOGICAL_PROCESSORS[] = "number_of_logical_processors";
};

namespace NSNicInfo
{
    const char DELIM = ',';
    const char NAME[] = "name";
    const char MANUFACTURER[] = "manufacturer";
    const char MAX_SPEED[] = "max_speed";
    const char IS_DHCP_ENABLED[] = "is_dhcp_enabled";
    const char IP_ADDRESSES[] = "ip_addresses";                            /* comma separated */
    const char IP_TYPES[] = "ip_types"; /* comma separated values whether ip is physical or virtual, have 1-1 mapping with ips */
    const char IP_SUBNET_MASKS[] = "ip_subnet_masks"; /* comma separated; have one to one mapping with IP addresses */
    const char DEFAULT_IP_GATEWAYS[] = "default_ip_gateways";            /* comma separated */
    const char DNS_SERVER_ADDRESSES[] = "dns_server_addresses";            /* comma separated */
    const char DNS_HOST_NAME[] = "dns_host_name";
    const char INDEX[] = "index";

    /* values */
    const char IP_TYPE_PHYSICAL[] = "physical";
    const char IP_TYPE_UNKNOWN[] = "unknown";
};

namespace NSOsInfo
{
    const char NAME[] = "name";
    const char DISTRO_NAME[] = "distroname";
    const char CAPTION[] = "caption";
    const char MAJOR_VERSION[] = "major_version";
    const char MINOR_VERSION[] = "minor_version";
    const char BUILD[] = "build";
    const char SYSTEMTYPE[] = "systemtype";
    const char SYSTEMDIR[] = "systemdirectory";
    const char SYSTEMDRIVE[] = "systemdrive";
    const char SYSTEMDRIVE_DISKEXTENTS[] = "systemdrivediskextents";
    const char LASTBOOTUPTIME[] = "LastBootUpTime";
};

struct ClusterVolumeInfo {
    std::string deviceName;
    std::string deviceId;
    ClusterVolumeInfo() {}
    ClusterVolumeInfo (const ClusterVolumeInfo& rhs)
        {
            deviceName = rhs.deviceName;
            deviceId = rhs.deviceId;
        }
    ClusterVolumeInfo& operator =(const ClusterVolumeInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            deviceName = rhs.deviceName;
            deviceId = rhs.deviceId;
            return *this;
        }
    bool operator==( ClusterVolumeInfo const& rhs ) const
        {
            return( deviceName == rhs.deviceName && deviceId == rhs.deviceId );
        }
};

typedef std::vector<ClusterVolumeInfo> ClusterVolumeInfos_t;

struct SanVolumeSummary {
    std::string deviceName;
    std::string TargetNodeWWN;
    std::string TargetPortWWN;
    SV_ULONG        TargetPortFcId;

    std::string InitiatorNodeWWN;
    std::string InitiatorPortWWN;
    SV_ULONG        InitiatorPortFcId;

    SV_ULONGLONG fcLun;
    std::string luid;
    SV_LONGLONG capacity;
    SV_ULONG blocksize;
    std::string vendorName;
    std::string model;
    std::string revision;
    SV_ULONGLONG startOffset;
    SV_ULONGLONG partitionLength;
    std::string partitionType;

    SanVolumeSummary()
        {
            startOffset = 0;
            partitionLength = 0;
            capacity = 0;
            blocksize = 0;
            fcLun = 0;
        }
    SanVolumeSummary& operator=( SanVolumeSummary const& rhs )
        {
            deviceName = rhs.deviceName;
            startOffset = rhs.startOffset;
            partitionLength = rhs.partitionLength;
            partitionType = rhs.partitionType;
            TargetNodeWWN = rhs.TargetNodeWWN;
            TargetPortWWN = rhs.TargetPortWWN;
            TargetPortFcId = rhs.TargetPortFcId;

            InitiatorNodeWWN = rhs.InitiatorNodeWWN;
            InitiatorPortWWN = rhs.InitiatorPortWWN;
            InitiatorPortFcId = rhs.InitiatorPortFcId;

            fcLun = rhs.fcLun;
            capacity = rhs.capacity;
            blocksize = rhs.blocksize;
            TargetPortFcId = rhs.TargetPortFcId;
            vendorName = rhs.vendorName;
            model = rhs.model;
            revision = rhs.revision;
            return *this;
        }
    bool operator==(SanVolumeSummary const& rhs)
        {
            return(deviceName == rhs.deviceName &&
                   startOffset == rhs.startOffset &&
                   partitionLength == rhs.partitionLength &&
                   partitionType == rhs.partitionType &&
                   TargetNodeWWN == rhs.TargetNodeWWN &&
                   TargetPortWWN == rhs.TargetPortWWN &&
                   TargetPortFcId == rhs.TargetPortFcId &&

                   InitiatorNodeWWN == rhs.InitiatorNodeWWN &&
                   InitiatorPortWWN == rhs.InitiatorPortWWN &&
                   InitiatorPortFcId == rhs.InitiatorPortFcId &&

                   fcLun == rhs.fcLun &&
                   capacity == rhs.capacity &&
                   blocksize == rhs.blocksize &&
                   vendorName == rhs.vendorName &&
                   model == rhs.model &&
                   revision == rhs.revision &&
                   TargetPortFcId == rhs.TargetPortFcId);
        }
};

struct ClusterResourceDependencyInfo {
    std::string name;
    std::string id;
    ClusterResourceDependencyInfo() {}
    ClusterResourceDependencyInfo (const ClusterResourceDependencyInfo& rhs)
        {
            name = rhs.name;
            id = rhs.id;
        }
    ClusterResourceDependencyInfo& operator =(const ClusterResourceDependencyInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            name = rhs.name;
            id = rhs.id;
            return *this;
        }
    bool operator==( ClusterResourceDependencyInfo const& rhs ) const
        {
            return(id == rhs.id && name == rhs.name);
        }
};

typedef std::vector<ClusterResourceDependencyInfo> ClusterResourceDependencyInfos_t;

struct ClusterResourceInfo {
    std::string name;
    std::string id;
    ClusterVolumeInfos_t volumes;
    ClusterResourceDependencyInfos_t dependencies;
    ClusterResourceInfo() {}
    ClusterResourceInfo (const ClusterResourceInfo& rhs)
        {
            name = rhs.name;
            id = rhs.id;
            volumes = rhs.volumes;
            dependencies = rhs.dependencies;
        }

    ClusterResourceInfo& operator =(const ClusterResourceInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            name = rhs.name;
            id = rhs.id;
            volumes = rhs.volumes;
            dependencies = rhs.dependencies;
            return *this;
        }
    bool operator==( ClusterResourceInfo const& rhs ) const
        {
            return( name == rhs.name && id == rhs.id && volumes == rhs.volumes && dependencies == rhs.dependencies );
        }
};

typedef std::vector<ClusterResourceInfo> ClusterResourceInfos_t;

struct ClusterGroupInfo {
    std::string name;
    std::string id;
    std::string sqlVirtualServerName;
    std::string exchangeVirtualServerName;
    ClusterResourceInfos_t resources;
    ClusterGroupInfo() {}
    ClusterGroupInfo (const ClusterGroupInfo& rhs)
        {
            name = rhs.name;
            id = rhs.id;
            sqlVirtualServerName = rhs.sqlVirtualServerName;
            exchangeVirtualServerName = rhs.exchangeVirtualServerName;
            resources = rhs.resources;
        }

    ClusterGroupInfo& operator =(const ClusterGroupInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            name = rhs.name;
            id = rhs.id;
            sqlVirtualServerName = rhs.sqlVirtualServerName;
            exchangeVirtualServerName = rhs.exchangeVirtualServerName;
            resources = rhs.resources;
            return *this;
        }
    bool operator==( ClusterGroupInfo const& rhs ) const
        {
            return( name == rhs.name && id == rhs.id && resources == rhs.resources
                    && sqlVirtualServerName == rhs.sqlVirtualServerName && exchangeVirtualServerName == rhs.exchangeVirtualServerName );
        }
};

typedef std::vector<ClusterGroupInfo> ClusterGroupInfos_t;

struct ClusterAvailableVolume {
    std::string name;
    std::string id;
    ClusterAvailableVolume() {}
    ClusterAvailableVolume (const ClusterAvailableVolume& rhs)
        {
            name = rhs.name;
            id = rhs.id;
        }

    ClusterAvailableVolume& operator =(const ClusterAvailableVolume& rhs)
        {
            if ( this == &rhs)
                return *this;
            name = rhs.name;
            id = rhs.id;
            return *this;
        }
    bool operator==( ClusterAvailableVolume const& rhs ) const
        {
            return( name == rhs.name && id == rhs.id );
        }
};

typedef std::vector<ClusterAvailableVolume> ClusterAvailableVolumes_t;

struct ClusterInfo {
    std::string name;
    std::string id;
    ClusterGroupInfos_t groups;
    ClusterAvailableVolumes_t availableVolumes;
    ClusterInfo() {}
    ClusterInfo (const ClusterInfo& rhs)
        {
            name = rhs.name;
            id = rhs.id;
            groups = rhs.groups;
            availableVolumes = rhs.availableVolumes;
        }

    ClusterInfo& operator =(const ClusterInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            name = rhs.name;
            id = rhs.id;
            groups = rhs.groups;
            availableVolumes = rhs.availableVolumes;
            return *this;
        }
    bool operator==( ClusterInfo const& rhs ) const
        {
            return( name == rhs.name && id == rhs.id && groups == rhs.groups && availableVolumes == rhs.availableVolumes);
        }
};


struct VDIInfo
{
    std::string uuid;
    std::string vdi_description;
    std::string vdi_devicenumber;
    bool vdi_availability;
    VDIInfo() {}
    VDIInfo (const VDIInfo& rhs)
        {
            uuid = rhs.uuid;
            vdi_description = rhs.vdi_description;
            vdi_devicenumber = rhs.vdi_devicenumber;
            vdi_availability = rhs.vdi_availability;
        }

    VDIInfo& operator =(const VDIInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            uuid = rhs.uuid;
            vdi_description = rhs.vdi_description;
            vdi_devicenumber = rhs.vdi_devicenumber;
            vdi_availability = rhs.vdi_availability;
            return *this;
        }

    bool operator==( VDIInfo const& rhs ) const
        {
            return( uuid == rhs.uuid
                    && vdi_description == rhs.vdi_description
                    && vdi_devicenumber == rhs.vdi_devicenumber
                    && vdi_availability == rhs.vdi_availability);
        }
};

typedef std::list<VDIInfo> VDIInfos_t;

struct VMInfo
{
    std::string uuid;
    std::string residenton;
    std::string powerstate;
    VMInfo() {}
    VMInfo (const VMInfo& rhs)
        {
            uuid = rhs.uuid;
            residenton = rhs.residenton;
            powerstate = rhs.powerstate;
        }

    VMInfo& operator =(const VMInfo& rhs)
        {
            if ( this == &rhs)
                return *this;
            uuid = rhs.uuid;
            residenton = rhs.residenton;
            powerstate = rhs.powerstate;
            return *this;
        }

    bool operator==( VMInfo const& rhs ) const
        {
            return( uuid == rhs.uuid
                    && residenton == rhs.residenton
                    && powerstate == rhs.powerstate );
        }
};

typedef std::list<VMInfo> VMInfos_t;

struct PoolInfo
{
    std::string uuid;
    std::string name;
    PoolInfo() {}
    PoolInfo(const PoolInfo& rhs)
        {
            uuid = rhs.uuid;
            name = rhs.name;
        }
    PoolInfo& operator =(const PoolInfo& rhs)
        {
            if( this == &rhs )
                return *this;
            uuid = rhs.uuid;
            name = rhs.name;
            return *this;
        }
    bool operator ==(PoolInfo const& rhs) const
        {
            return(uuid == rhs.uuid    && name == rhs.name);
        }
};

typedef std::list<PoolInfo> PoolInfos_t;

struct SanInitiatorSummary
{
    std::string NodeWWN;
    std::string PortWWN;
    std::string ModelName;
    std::string ModelDesc;
    std::string SerialNum;
    bool target_mode_enabled;
    bool state;
};

struct ResyncTimeSettings
{
    ResyncTimeSettings() : time(0), seqNo(0) {}
    SV_ULONGLONG     time;
    SV_ULONGLONG    seqNo;
};

struct StorageFailover {
    std::string results;

    StorageFailover() {}
    StorageFailover(StorageFailover const& rhs) {
        init(rhs);
    }

    StorageFailover& operator=(StorageFailover const& rhs) {
        if (this == &rhs) {
            return *this;
        }
        init(rhs);
        return *this;
    }

    bool operator==(StorageFailover const& rhs ) const {
        return (results == rhs.results);
    }

    void init(StorageFailover const& rhs) {
        results = rhs.results;
    }
};


struct VolumeStats
{
    SV_ULONGLONG totalChangesPending;
    SV_ULONGLONG oldestChangeTimeStamp;
    SV_ULONGLONG driverCurrentTimeStamp;

    VolumeStats()
        {
            totalChangesPending = oldestChangeTimeStamp = driverCurrentTimeStamp = 0;
        }
};

typedef std::map<std::string, VolumeStats> VolumesStats_t; /* map from volume name to its stats */

struct disk_extent
{
    std::string disk_id;
    SV_LONGLONG offset;
    SV_LONGLONG length;

    disk_extent ()
    {
        offset = length = 0;
    }

    disk_extent (const std::string& _disk_id, SV_LONGLONG _offset, SV_LONGLONG _lenght)
    {
        disk_id = _disk_id;
        offset = _offset;
        length = _lenght;
    }

    disk_extent(disk_extent const& rhs)
    {
        disk_id = rhs.disk_id;
        offset = rhs.offset;
        length = rhs.length;
    }

    disk_extent& operator = (disk_extent const& rhs)
    {
        if (this == &rhs)
            return *this;

        disk_id = rhs.disk_id;
        offset = rhs.offset;
        length = rhs.length;

        return *this;
    }

    bool operator == (disk_extent const& rhs) const
    {
        return ( 
                 disk_id == rhs.disk_id &&
                 offset == rhs.offset &&
                 length == rhs.length
               );
    }
};

typedef std::vector<disk_extent> disk_extents_t;
typedef disk_extents_t::iterator disk_extents_iter_t;


struct MTRegistrationDetails
{
    std::string certData;
    std::string registryData;
};

#endif // VOLUME_GROUP_SETTINGS_H
