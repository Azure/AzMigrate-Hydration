#ifndef GLOBS__H
#define GLOBS__H

/* SV Systems Registry Value Names */
/* These were previously in hostagenthelpers.cpp */
/* Since the hostconfig project needed these, we moved them here */

const char SV_CONFIG_PATHNAME_VALUE_NAME[]      = "ConfigPathname";
const char SV_SERVER_NAME_VALUE_NAME[]			= "ServerName";
const char SV_SERVER_HTTP_PORT_VALUE_NAME[]		= "ServerHttpPort";
const char THROTTLE_BOOTSTRAP_URL[]             = "ThrottleBootstrap";
const char THROTTLE_SENTINEL_URL[]              = "ThrottleSentinel";
const char THROTTLE_OUTPOST_URL[]               = "ThrottleOutpost";
const char THROTTLE_AGENT_URL[]                 = "ThrottleAgent";
const char SV_UPDATE_STATUS_URL[]               = "update_status.php";        // BUGBUG: read from registry
const char SV_RENAME_AND_OR_DELETE_URL[]        = "rename_delete.php";        
const char SV_HOST_AGENT_TYPE_VALUE_NAME[]      = "HostAgentType";
const char SV_REGISTER_URL_VALUE_NAME[]         = "RegisterURL";
const char SV_PROTECTED_DRIVES_URL_VALUE_NAME[] = "GetProtectedDrivesURL";
const char SV_TARGET_DRIVES_URL_VALUE_NAME[]    = "GetTargetDrivesURL";
const char SV_HOST_ID_VALUE_NAME[]              = "HostID";
const char SV_AVAILABLE_DRIVES_VALUE_NAME[]     = "AvailableDrives";
const char SV_PROTECTED_DRIVES_VALUE_NAME[]     = "ProtectedDrives";
const char SV_RESYNC_DRIVES_VALUE_NAME[]        = "InitialSyncDrives";
const char SV_FILE_TRANSPORT_VALUE_NAME[]       = "FileTransport";
const char SV_SHOULD_RESYNC_VALUE_NAME[]        = "ShouldResync";
const char SV_DEBUG_LOG_LEVEL_VALUE_NAME[]      = "DebugLogLevel";
const char SV_UNCOMPRESS_EXE_VALUE_NAME[]       = "UncompressExe";
const char SV_VALUE_NAME[]                      = "SOFTWARE\\SV Systems";
const char SV_PROTECTED_VOLUME_SETTINGS_URL[]   = "GetProtectedVolumeSettingsURL";
//const char SV_SENTINEL_VALUE_NAME[]             = "SOFTWARE\\SV Systems\\Sentinel";
//const char SV_OUTPOSTAGENT_VALUE_NAME[]         = "SOFTWARE\\SV Systems\\OutpostAgent";
const char SV_FILEREP_AGENT_VALUE_NAME[]        = "SOFTWARE\\SV Systems\\FileReplicationAgent";
const char SV_VXAGENT_VALUE_NAME[]              = "SOFTWARE\\SV Systems\\VxAgent";
const char SV_INVOLFLT_PARAMETER[]              = "SYSTEM\\CurrentControlSet\\Services\\involflt\\Parameters";
const char SV_INDSKFLT_PARAMETER[]              = "SYSTEM\\CurrentControlSet\\Services\\indskflt\\Parameters"; // disk filter driver parameters
const char SV_RETENTIONSETTINGS_VALUE_NAME[]  = "SOFTWARE\\SV Systems\\VxAgent\\RetentionSettings";
const char SV_VIRTUALVOLUMES[]  = "SOFTWARE\\SV Systems\\VxAgent\\VirtualVolumes";
const char SV_UPDATE_AGENT_LOG_URL_VALUE_NAME[] = "UpdateAgentLogURL";
const char SV_GET_VOLUME_GROUP_SETTINGS_URL_VALUE_NAME[] = "GetVolumeGroupSettingsURL";
const char SV_UPDATE_PERMISSION_STATE_URL_VALUE_NAME[] = "UpdatePermissionStateUrl";
const char SV_HTTPS[]		= "Https";

const char SV_ROOT_TRANSPORT_KEY[] = "SOFTWARE\\SV Systems\\Transport";

const char SV_CSPRIME_APPLIANCE_TO_AZURE_PS_NAME[] = "SOFTWARE\\Microsoft\\Azure Site Recovery Process Server";
const char SV_CSPRIME_APPLIANCE_TO_AZURE_PS_INSTALL_PATH[] = "Install Location";
const char SV_CSPRIME_APPLIANCE_TO_AZURE_PS_LOG_FOLDER_PATH[] = "Log Folder Path";
const char SV_CSPRIME_APPLIANCE_TO_AZURE_PS_TELEMETRY_FOLDER_PATH[] = "Telemetry Folder Path";
/* END registry keys on the CS Prime appliance */

const char SV_INITIALSYNC_OFFSET_LOW_VALUE_NAME[]                 = "InitialSync Offset Low";
const char SV_INITIALSYNC_OFFSET_HIGH_VALUE_NAME[]                = "InitialSync Offset High";
const char SV_REPORT_RESYNC_REQUIRED_URL_VALUE_NAME[]             = "ReportResyncRequiredURL";
const char SV_UPDATE_STATUS_URL_VALUE_NAME[]                      = "UpdateStatusURL";
const char SV_CACHE_DIRECTORY_VALUE_NAME[]                        = "CacheDirectory";
const char SV_VOLUME_CHUNK_SIZE_VALUE_NAME[]                      = "VolumeChunkSize";
const char SV_MAX_RESYNC_THREADS_VALUE_NAME[]                     = "MaxResyncThreads";
const char SV_MIN_CACHE_FREE_DISK_SPACE_PERCENT[]                 = "MinCacheFreeDiskSpacePercent";
const char SV_MIN_CACHE_FREE_DISK_SPACE[]                         = "MinCacheFreeDiskSpace";
const char SV_MAX_OUTPOST_THREADS_VALUE_NAME[]                    = "MaxOutpostThreads";
const char SV_BYTES_APPLIED_THRESHOLD_VALUE_NAME[]                = "BytesAppliedThreshold";
const char SV_SYNC_BYTES_APPLIED_THRESHOLD_VALUE_NAME[]           = "SyncBytesAppliedThreshold";
const char SV_MAX_SENTINEL_PAYLOAD_VALUE_NAME[]                   = "MaxSentinelPayload";
const char SV_REGISTER_SYSTEM_DRIVE[]                             = "RegisterSystemDrive";
const char SV_GET_FILE_REPLICATION_CONFIGURATION_URL_VALUE_NAME[] = "GetFileReplicationConfigurationURL";
const char SV_UPDATE_FILE_REPLICATION_STATUS_URL_VALUE_NAME[]     = "UpdateFileReplicationStatusURL";
const char SV_INMSYNC_EXE_VALUE_NAME[]                            = "InmsyncExe";
const char SV_SHELL_COMMAND_VALUE_NAME[]                          = "ShellCommand";
const char SV_UNREGISTER_URL_VALUE_NAME[]                         = "UnregisterURL";
const char SV_ENABLE_FR_LOG_FILE_DELETION[]                       = "EnableFrLogFileDeletion";
const char SV_ENABLE_FR_DELETE_OPTIONS[]                          = "EnableDeleteOptions";
const char SV_USE_CONFIGURED_IP[]                                 = "UseConfiguredIP";
const char SV_CONFIGURED_IP[]                                     = "ConfiguredIP";
const char SV_USE_CONFIGURED_HOSTNAME[]                           = "UseConfiguredHostname";
const char SV_CONFIGURED_HOSTNAME[]                               = "ConfiguredHostname";
const char SV_FR_SEND_SIGNAL_EXE[]                                = "SendSignalExe";
const char SV_MAX_FR_LOG_PAYLOAD_VALUE_NAME[]                     = "MaxFrLogPayload";
const char SV_INMSYNC_TIMEOUT_SECONDS_VALUE_NAME[]                = "InmsyncTimeoutSeconds";
const char SV_REGISTER_CLUSTER_INFO_URL_VALUE_NAME[]              = "RegisterClusterInfoURL";
const char SV_SENTINEL_EXE_VALUE_NAME[]                           = "SentinelExe";
const char SV_GET_RESYNC_DRIVES_URL_VALUE_NAME[]                  = "GetResyncDrivesURL";
const char SV_SOURCE_DIRECTORY_PREFIX_VALUE_NAME[]                = "SourceDirectoryPrefix";
//const char SV_DESTINATION_DIRECTORY_PREFIX_VALUE_NAME[]           = "DestinationDirectoryPrefix";
const char SV_COMPLETED_DIFF_FILENAME_PREFIX_VALUE_NAME[]         = "FilenamePrefix";
const char SV_INITIAL_SYNC_COMPLETE_VALUE_NAME[]                  = "InitialSyncComplete";
const char SV_SNAPSHOT_URL_VALUE_NAME[]                           = "SnapshotURL";
const char SV_DATA_PROTECTION_EXE_VALUE_NAME[]                    = "DataProtectionExe";
const char SV_HIGH_LAST_SNAPSHOT_ID[]							  = "LastSnapShotIdHi";
const char SV_LOW_LAST_SNAPSHOT_ID[]							  = "LastSnapShotIdLo";
const char SV_SERVICE_REQUEST_DATA_SIZE[]                         = "ServiceRequestDataSizeLimit";
const char SV_VOLUME_DATA_NOTIFY_LIMIT[]                          = "VolumeDataNotifyLimitInKB";                   
const char SV_DISK_DATA_NOTIFY_LIMIT[]                            = "DeviceDataNotifyLimitInKB";     //Disk data notify limit in KB             
//Added for FX-Upgrade case
const char SV_FX_UPGRADED[]                          = "IsFxUpgraded";



// HACK: temp hack to simulate volume groups
const char SV_FILES_APPLIED_THRESHOLD_VALUE_NAME[]                = "FilesAppliedThreshold";
const char SV_TIME_STAMP_APPLIED_THRESHOLD_VALUE_NAME[]           = "TimestampAppliedThreshold";

const char SV_SNAPSHOT_OFFSET_HIGH[]            = "Snapshot Offset High";
const char SV_SNAPSHOT_OFFSET_LOW[]             = "Snapshot Offset Low";
const char SV_MAXIMUM_BITMAP_BUFFER_MEMORY[]    = "MaximumBitmapBufferMemory";
const char SV_BITMAP8K_GRANULARITY_SIZE[]       = "Bitmap8KGranularitySize";
const char SV_BITMAP16K_GRANULARITY_SIZE[]      = "Bitmap16KGranularitySize";
const char SV_BITMAP32K_GRANULARITY_SIZE[]      = "Bitmap32KGranularitySize";
const char SV_VOLUME_BITMAP_GRANULARITY[]       = "VolumeBitmapGranularity";

const char SV_FR_ENABLE_ENCRYPTION[] = "EnableEncryption";
const char SV_FR_REGISTER_HOST_INTERVEL[] = "RegisterHostIntervel";


const char SV_AGENT_INSTALL_LOCATON_VALUE_NAME[] = "InstallDirectory";

const unsigned DEFAULT_FILES_APPLIED_THRESHOLD = 56;       // max files to apply in a given run
const int DEFAULT_TIME_STAMP_APPLIED_THRESHOLD = 56;  // max timestamp files to apply in a given run

const int MAX_RESYNC_THREADS = 50;
const int DEFAULT_RESYNC_THREADS = 8;
const int MAX_OUTPOST_THREADS = 25;
const int DEFAULT_OUTPOST_THREADS = 4;
const unsigned DEFAULT_VOLUME_CHUNK_SIZE = 64*1024*1024;
const unsigned DEFAULT_SERVICE_REQUEST_DATA_SIZE = 4096 ;
const unsigned DEFAULT_BYTES_APPLIED_THRESHOLD = DEFAULT_FILES_APPLIED_THRESHOLD * DEFAULT_VOLUME_CHUNK_SIZE;  // allow max full chunks 
const int DEFAULT_SYNC_BYTES_APPLIED_THRESHOLD = 0;                                            // no limit for default

const unsigned DEFAULT_MIN_CACHE_FREE_DISK_SPACE_PERCENT = 25;
const unsigned DEFAULT_MIN_CACHE_FREE_DISK_SPACE = 0x40000000; // 1GB
const int DEFAULT_VOLUME_DATA_NOTIFY_LIMIT = 512;




const char RESYNC_SOURCE_DIRECTORY_PATH_NAME[] = "ResyncSourceDirectoryPath";
// fast sync hash 
const int FAST_SYNC_MAX_CHUNK_SIZE_DEFAULT = 1024 * 1024 * 64;  // default value to use 64MB
const int FAST_SYNC_HASH_COMPARE_DATA_SIZE_DEFAULT = 1024 * 1024; // default value to use 512KB

const char FAST_SYNC_HASH_COMPARE_DATA_SIZE_NAME[] = "FastSyncHashCompareDataSize";
const char FAST_SYNC_MAX_CHUNK_SIZE_NAME[] = "FastSyncMaxChunkSize";

//CxUpdateInterval
const unsigned long DEFAULT_CX_UPDATE_INTERVAL = 60;        //default interval
const char SV_CX_UPDATE_INTERVAL[]          = "CxUpdateInterval" ;

int const MAX_PLEXES        = 20;

int const MAX_JOBID_LENGTH = 56;

// Bug#3 fix: this is set to 8 meg now.
const int VOLUME_READ_CHUNK_SIZE =  8 * 1024*1024; // 8MB
#ifdef SV_WINDOWS
const int PAGE_SIZE = 4*1024;

#define DEFAULT_GET_VOLUME_CHECKPOINT_URL TEXT("get_volume_checkpoint.php")
#define GET_VOLUME_CHECKPOINT_URL_VALUE_NAME TEXT("GetVolumeCheckpointURL")

// Changefstag releated globals
#define GET_UNLOCK_STATUS_URL TEXT("unlock_status.php")
#define GET_UNLOCK_REQUEST_NOTIFY TEXT("unlock_request_notify.php")
#endif
const char SV_SETTINGS_POLLING_INTERVAL[]					= "SettingsPollingInterval";
const int DEFAULT_SETTINGS_POLLING_INTERVAL = 30;

#endif // GLOBS__H
