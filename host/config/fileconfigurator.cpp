//
// fileconfigurator.cpp: implements a configurator that reads from config file
//
#include <utility>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "fileconfigurator.h"
#include "inmageex.h"
#include "fileconfiguratordefs.h"
#include "platformfileconfiguratordefs.h"
#include "cfslocalname.h"
#include "defaultdirs.h"

#ifdef SV_WINDOWS
#include <VersionHelpers.h>
#endif

static const char SECTION_VXAGENT[] = "vxagent";
static const char SECTION_APPAGENT[] = "appagent";
static const char SECTION_DRIVER[] = "vxagent.filterdriver";
static const char SECTION_VXTRANSPORT[] = "vxagent.transport";
static const char SECTION_VXACCOUNT[] = "vxagent.account";
static const char SECTION_VXAGENT_VIRTUAL_VOLUMES[] = "vxagent.virtualvolumes";
static const char SECTION_REPLICATED_VOLUME_CAPACITY[] = "vxagent.replicatedvolumecapacity";
static const char SECTION_VOLUME_MOUNT_POINT[] = "vxagent.volumemountpoint";
static const char SECTION_VOLUME_FILE_SYSTEM[] = "vxagent.volumefilesystem";
static const char SECTION_APPLICATION[] = "application";
static const char SECTION_UPGRADE[] = "vxagent.upgrade";

static const char SECTION_PUSHINSTALLER[] = "PushInstaller";
static const char SECTION_PUSHINSTALLERTRANSPORT[] = "PushInstaller.transport";
static const char KEY_LOG_FOLDER[] = "LogFolder";
static const char KEY_PUSHTMP_FOLDER[] = "TmpFolder";
static const char KEY_PUSHINSTALL_TELEMETRY_FOLDER[] = "PushInstallTelemetryFolder";
static const char KEY_AGENT_TELEMETRY_FOLDER[] = "AgentTelemetryFolder";
static const char KEY_PUSH_SIGNATURE_CHECK[] = "SignatureVerificationChecks";
static const char KEY_PUSH_SIGNATURE_CHECK_ON_REMOTEMACHINE[] = "SignatureVerificationChecksOnRemoteMachine";
static const char KEY_PUSH_REPO_UA[] = "RepositoryPathUA";
static const char KEY_PUSH_REPO_PUSHCLIENT[] = "RepositoryPathPushclient";
static const char KEY_PUSH_REPO_PATCH[] = "RepositoryPathPatch";
static const char KEY_PUSH_REMOTEFOLDERWINDOWS[] = "RemoteFolderWindows";
static const char KEY_PUSH_REMOTEFOLDERUNIX[] = "RemoteFolderUnix";
static const char KEY_PUSH_OSSCRIPTPATH[] = "OsScriptPath";
static const char KEY_PUSH_VMWAREAPICMD[] = "VmWareApiCmd";
static const char KEY_PUSH_JOBTIMEOUT[] = "JobTimeoutInSecs";
static const char KEY_PUSH_VMWAREWRAPPER_TIMEOUT[] = "VmWareApiCmdTimeoutInSecs";
static const char KEY_PUSH_JOBFETCHINTERVAL[] = "JobFetchIntervalInSecs";
static const char KEY_PUSH_JOBRETRIES[] = "JobRetries";
static const char KEY_PUSH_JOBRETRYINTERVAL[] = "JobRetryIntervalInSecs";
static const char KEY_PUSH_CSJOBRETRIES[] = "CSJobRetries";
static const char KEY_PUSH_CSJOBRETRYINTERVAL[] = "CSJobRetryIntervalInSecs";
static const char KEY_DPP_USAGE_IN_PERCENTAGE[] = "DataPagePoolMemoryUsageInPercentage";
static const char KEY_DPP_ALIGNMENT_IN_MB[] = "DataPagedPoolAlignmentInMB";
static const char KEY_MAX_DPP_USAGE_IN_MB[] = "MaxDataPagedPoolSizeInMB";
static const char KEY_MIN_DPP_USAGE_IN_MB[] = "MinDataPagedPoolSizeInMB";

static const char SECTION_BACKUP_EXPRESS_CONFIGSTORE[] = "backupexpress.configstore";

static const char KEY_CACHE_DIRECTORY[] = "CacheDirectory";
static const char KEY_HOST_ID[] = "HostId";
static const char KEY_RESOURCE_ID[] = "ResourceId";
static const char KEY_SOURCE_GROUP_ID[] = "SourceGroupId";
static const char KEY_SYSTEM_UUID[] = "SystemUUID";
static const char KEY_HYPERVISOR_NAME[] = "HyperVisorName";

static const char KEY_FAILOVER_VM_DETECTION_ID[] = "FailoverVmDetectionId";
static const char KEY_IS_AZURE_VM[] = "IsAzureVm";
static const char KEY_SOURCE_CONTROL_PLANE[] = "SourceControlPlane";
static const char KEY_FAILOVER_VM_BIOS_ID[] = "FailoverVmBiosId";
static const char KEY_FAILOVER_TARGET_TYPE[] = "FailoverTargetType";


const std::string DEFAULT_UNREGISTER_LOG_FILE = "unregister.log";
#ifdef SV_WINDOWS
static std::string const DEFAULT_WIN_UNREGISTER_LOG_FILE_PATH = "C:\\ProgramData\\ASRSetupLogs\\";
#endif

// EvtCollForw settings - Start
static const char KEY_EVTCOLLFORW_AGENT_LOG_POST_LEVEL[] = "EvtCollForwAgentLogPostLevel";
static const int DEFAULT_EVTCOLLFORW_AGENT_LOG_POST_LEVEL = 3; // SV_LOG_ERROR
static const char KEY_SOURCE_AGENT_LOGS_TO_UPLOAD[] = "SourceAgentLogsToUpload";
static const char DEFAULT_SOURCE_AGENT_LOGS_TO_UPLOAD[] = "svagents.log,s2.log,appservice.log";
static const char KEY_EVTCOLLFORW_POLL_INTERVAL_IN_SECS[] = "EvtCollForwPollIntervalInSecs";
static const int DEFAULT_EVTCOLLFORW_POLL_INTERVAL_IN_SECS = 300; // 5 mins
static const char KEY_EVTCOLLFORW_PENALTY_TIME_IN_SECS[] = "EvtCollForwPenaltyTimeInSecs";
static const int DEFAULT_EVTCOLLFORW_PENALTY_TIME_IN_SECS = 3600; // 60 mins
static const char KEY_EVTCOLLFORW_MAX_STRIKES[] = "EvtCollForwMaxStrikes";
static const int DEFAULT_EVTCOLLFORW_MAX_STRIKES = 50;
static const char KEY_EVTCOLLFORW_PROC_SPAWN_INTERVAL[] = "EvtCollForwProcSpawnInterval";
static const int DEFAULT_EVTCOLLFORW_PROC_SPAWN_INTERVAL = 300; // 5 mins
static const char KEY_EVTCOLLFORW_MAX_MARS_UPLOAD_FILES_CNT[] = "EvtCollForwMaxMarsUploadFilesCnt";
static const int DEFAULT_EVTCOLLFORW_MAX_MARS_UPLOAD_FILES_CNT = 256;
static const char KEY_DEPRECATED_EVTCOLLFORW_CX_KEEP_ALIVE_INTERVAL_IN_SECS[] = "EvtCollForwCxKeepAliveIntervalInSecs";
static const int DEFAULT_DEPRECATED_EVTCOLLFORW_CX_KEEP_ALIVE_INTERVAL_IN_SECS = 90; // 90 secs = HEARTBEAT_INTERVAL_SECONDS / 2
static const char KEY_EVTCOLLFORW_CX_TRANSPORT_MAX_ATTEMPTS[] = "EvtCollForwCxTransMaxAttempts";
static const int DEFAULT_EVTCOLLFORW_CX_TRANSPORT_MAX_ATTEMPTS = 2;
static const char KEY_EVTCOLLFORW_UPLOAD_EVENT_LOG[] = "EvtCollForwUploadEventLog";
static const char DEFAULT_EVTCOLLFORW_UPLOAD_EVENT_LOG[] = "1";
static const char KEY_EVTCOLLFORW_IR_COMPLETED_FILES_MOVE_DIR[] = "EvtCollForwIRCompletedFilesMoveDir";
// EvtCollForw settings - End

// CsJobProcessor settings -- Start
static const char  KEY_CSJOBPROCESSOR_PROC_SPAWN_INTERVAL[] = "CsJobProcessorProcSpawnInterval";
static const int DEFAULT_CSJOBPROCESSOR_PROC_SPAWN_INTERVAL = 60; // 1 min
// CsJobProcessor settings -- End

static const char KEY_RCM_SETTINGS_PATH[] = "RcmSettingsPath";
#ifdef SV_WINDOWS
static const std::string RCM_SETTINGS_FILE_SUFFIX = "\\Config\\RCMInfo.conf";
#else
static const std::string RCM_SETTINGS_FILE_SUFFIX = "/config/RCMInfo.conf";
#endif
static const char KEY_PROXY_SETTINGS_PATH[] = "ProxySettingsPath";
static const char KEY_VM_PLATFORM[] = "VmPlatform";

static const char KEY_MAX_DIFF_SIZE[] = "MaxDiffSize";
static const char KEY_FASTSYNC_READ_BUFFER_SIZE[] = "FastSyncReadBufferSize";
static const char KEY_DIFF_SOURCE_DIRECTORY[] = "DiffSourceDirectory";
static const char KEY_DIFF_TARGET_DIRECTORY[] = "DiffTargetDirectory";
static const char KEY_PREFIX_DIFF_FILENAME[] = "PrefixDiffFilename";
static const char KEY_RESYNC_DIRECTORIES[] = "ResyncDirectories";
static const char KEY_SSL_CLIENT_FILE[] = "SslClientFile";
static const char KEY_SSL_KEY_PATHNAME[] = "SslKeyPathname";
static const char KEY_SSL_CERTIFICATE_PATHNAME[] = "SslCertificatePathname";
static const char KEY_CFS_LOCAL_NAME[] = "CfsLocalName";
static const char KEY_TCP_SENDWINDOW_SIZE[] = "TcpSendWindowSize";
static const char KEY_TCP_RECVWINDOW_SIZE[] = "TcpRecvWindowSize";
static const int DEFAULT_TCP_SENDWINDOW_SIZE = 1048576;
static const int DEFAULT_TCP_RECVWINDOW_SIZE = 1048576;
static const SV_UINT DEFAULT_FASTSYNC_READ_BUFFER_SIZE = 4 * 1048576; /* TODO: 4 MB ? */
static const char KEY_HTTP_IPADDRESS[] = "Hostname"; // does this need to be IP?
static const char KEY_CS_ADDRESS_FOR_AZURE_COMPONENTS[] = "CsAddressForAzureComponents"; 
static const char KEY_HTTPS[] = "Https";

static const char KEY_HTTP_PORT[] = "Port";
static const char KEY_LOG_PATHNAME[] = "LogPathname";
static const char KEY_LOG_LEVEL[] = "LogLevel";
static const SV_UINT DEFAULT_LOG_LEVEL = 3;
static const char KEY_LOG_MAX_NUM_COMPLETED_FILES[] = "LogMaxNumCompletedFiles";
static const char KEY_LOG_CUT_INTERVAL[] = "LogCutInterval";
static const char KEY_LOG_MAX_FILE_SIZE[] = "LogMaxFileSize";
static const char KEY_LOG_MAX_FILE_SIZE_TELEMETRY[] = "LogMaxFileSizeTelemetry";
static const char KEY_DIFF_SOURCE_EXE_PATHNAME[] = "DiffSourceExePathname";
static const char KEY_DATA_PROTECTION_EXE_PATHNAME[] = "DataProtectionExePathname";
static const char KEY_DATA_PROTECTION_EXEV2_PATHNAME[] = "DataProtectionExeV2Pathname";
static const char KEY_VSNAP_CONFIG_PATHNAME[] = "VsnapConfigPathname";
static const char KEY_VSNAP_PENDING_PATHNAME[] = "VsnapPendingPathname";
static const char KEY_OFFLOAD_SYNC_PATHNAME[] = "OffloadSyncPathname";
static const char KEY_OFFLOAD_SYNC_SOURCE_DIRECTORY[] = "OffloadSyncSourceDirectory";
static const char KEY_OFFLOAD_SYNC_CACHE_DIRECTORY[] = "OffloadSyncCacheDirectory";
static const char KEY_OFFLOAD_SYNC_FILENAME_PREFIX[] = "OffloadSyncFilenamePrefix";
static const char KEY_DIFF_TARGET_EXE_PATHNAME[] = "DiffTargetExePathname";
static const char KEY_DIFF_TARGET_SOURCE_DIRECTORY_PREFIX[] = "DiffTargetSourceDirectoryPrefix";
static const char KEY_DIFF_TARGET_CACHE_DIRECTORY_PREFIX[] = "DiffTargetCacheDirectoryPrefix";
static const char KEY_DIFF_TARGET_FILENAME_PREFIX[] = "DiffTargetFilenamePrefix";
static const char KEY_FAST_SYNC_EXE_PATHNAME[] = "FastSyncExePathname";
static const char KEY_FAST_SYNC_BLOCK_SIZE[] = "FastSyncBlockSize";
static const char KEY_FAST_SYNC_MAX_CHUNK_SIZE[] = "FastSyncMaxChunkSize";
static const char KEY_FAST_SYNC_MAX_CHUNK_SIZE_FOR_E2A[] = "FastSyncMaxChunkSizeE2A";
static const char KEY_CLUSTER_SYNC_CHUNK_SIZE[] = "ClusterSyncChunkSize";
static const char KEY_USE_CONFIGURED_HOSTNAME[] = "UseConfiguredHostname";
static const char KEY_USE_CONFIGURED_IP_ADDRESS[] = "UseConfiguredIpAddress";
static const char KEY_CONFIGURED_HOSTNAME[] = "ConfiguredHostname";
static const char KEY_CONFIGURED_IP_ADDRESS[] = "ConfiguredIpAddress";
static const char KEY_EXTERNAL_IP_ADDRESS[] = "ExternalIpAddress";
static const char KEY_FAST_SYNC_HASH_COMPARE_DATA_SIZE[] = "FastSyncHashCompareDataSize";
static const char DEFAULT_FAST_SYNC_HASH_COMPARE_DATA_SIZE[] = "1048576"; // 1 MB
static const char KEY_RESYNC_SOURCE_DIRECTORY_PATH[] = "ResyncSourceDirectoryPath";
static const char KEY_MAX_FASTSYNC_APPLY_THREADS[] = "MaxFastSyncApplyThreads";
static const char KEY_SYNC_BYTES_TO_APPLY_THRESHOLD[] = "SyncBytesToApplyThreshold";
static const char KEY_CHUNK_MODE[] = "ChunkMode";

static const char KEY_REMOTE_LOG_LEVEL[] = "RemoteLogLevel";

static const char KEY_MAX_OUTPOST_THREADS[] = "MaxOutpostThreads";
static const char KEY_VOLUME_CHUNK_SIZE[] = "VolumeChunkSize";
static const char KEY_REGISTER_SYSTEM_CACHE_VOLUMES[] = "RegisterSystemDrive";
static const char DEFAULT_REGISTER_SYSTEM_CACHE_VOLUMES[] = "1";
static const char KEY_TIME_STAMPS_ONLY_TAG[] = "TimeStampsOnlyTag";
static const char KEY_DEST_DIR[] = "DestDir";
static const char KEY_DAT_EXTENSION[] = "DatExtension";
static const char KEY_META_DATA_CONTINUATION_TAG[] = "MetaDataContinuationTag";
static const char KEY_META_DATA_CONTINUATION_END_TAG[] = "MetaDataContinuationEndTag";
static const char KEY_WRITE_ORDER_CONTINUATION_TAG[] = "WriteOrderContinuationTag";
static const char KEY_WRITE_ORDER_CONTINUATION_END_TAG[] = "WriteOrderContinuationEndTag";
static const char KEY_PRE_REMOTE_NAME[] = "PreRemoteName";
static const char KEY_FINAL_REMOTE_NAME[] = "FinalRemoteName";
static const char KEY_THROTTLE_WAIT_TIME[] = "ThrottleWaitTime";
static const char KEY_S2_DATA_WAIT_TIME[] = "S2DataWaitTIme";
static const char KEY_SENTINEL_EXIT_TIME[] = "SentinelExitTime";
static const char KEY_WAIT_FOR_DB_NOTIFY[] = "WaitForDBNotify";
static const char KEY_PROTECTED_VOLUMES[] = "ProtectedVolumes";
static const char KEY_INSTALL_PATH[] = "InstallDirectory";
static const char KEY_VMWAREBASED_PUSHINSTALL_ENABLED[] = "IsVmWareBasedPushInstallEnabled";
static const char KEY_CX_UPDATE_INTERVAL[] = "CxUpdateInterval";
static const char KEY_CX_CDPDISKUSAGE_UPDATE_INTERVAL[] = "CxCDPDiskUsageUpdateInterval";
static const char KEY_DELETE_CXCDPUPDATES[] = "DeleteCxCDPUpdates";
static const char KEY_DELETE_CXCDPUPDATES_INTERVAL[] = "DeleteCxCDPUpdatesInterval";
static const char KEY_MIN_FREE_FREE_DISKSPACE_PERCENT[] = "MinCacheFreeDiskSpacePercent";
static const char KEY_MIN_CACHE_FREE_DISKSPACE[] = "MinCacheFreeDiskSpace";
static const char KEY_CM_MIN_RESERVEDSPCE_PER_PAIR[] = "CMMinReservedSpacePerPair";
static const SV_ULONGLONG DEFAULT_CM_MIN_RESERVEDSPCE_PER_PAIR = 536870912L; //512 MB
static const char KEY_ID[] = "ID";
static const char KEY_IDLE_WAIT_TIME[] = "IdleWaitTime";
static const char KEY_VSNAP_ID[] = "VsnapId";
static const char KEY_VSNAP_WRITEDATA_LENGTH[] = "VsnapWriteDataLength";
static const char KEY_LOW_LAST_SNAPSHOT_ID[] = "LastSnapShotIdLo";
static const char KEY_HIGH_LAST_SNAPSHOT_ID[] = "LastSnapShotIdHi";
static const char KEY_SEQUENCECOUNT_VALUE_NAME[] = "SequenceCount";
static const char KEY_SEQUENCECOUNTINMSECS_VALUE_NAME[] = "SequenceCountInMsecs";
static const char KEY_RETENIONBUFSIZE_VALUE_NAME[] = "RetentionBufferSize";
static const char KEY_MAX_MEMORYUSAGEPERREPLICATION_VALUE_NAME[] = "MaxMemoryUsagePerReplication";
static const char KEY_MAX_INMEMORYCOMPRESSEDFILESIZE_VALUE_NAME[] = "MaxInMemoryCompressedFileSize";
static const SV_ULONG DEFAULT_INMEMORYCOMPRESSEDFILESIZE_VALUE = 4 * 1024 * 1024;
static const char KEY_MAX_INMEMORYUNCOMPRESSEDFILESIZE_VALUE_NAME[] = "MaxInMemoryUnCompressedFileSize";
static const SV_ULONG DEFAULT_INMEMORYUNCOMPRESSEDFILESIZE_VALUE = 8 * 1024 * 1024;
static const char KEY_COMPRESSIONCHUNKSIZE_VALUE_NAME[] = "CompressionChunkSize";
static const SV_ULONG DEFAULT_COMPRESSIONCHUNKSIZE_VALUE = 1048576;
static const char KEY_COMPRESSIONBUFSIZE_VALUE_NAME[] = "CompressionBufSize";
static const SV_ULONG DEFAULT_COMPRESSIONBUFSIZE_VALUE = 4194304;
static const char KEY_MAX_RUNSPERINVOCATION_VALUE_NAME[] = "MaxRunsPerInvocation";
static const char KEY_ENFORCE_STRICT_CONSISTENCY_GROUPS[] = "EnforceStrictConsistencyGroups";
static const char KEY_TRACK_EXPERIMENTAL_DEVICE_NUMBERS[] = "TrackExperimentalDeviceNumbers";
static const char KEY_HDLM_DLNKMGR_CMD[] = "HdlmDlnkMgrCmd";
static const char KEY_SCLI_PATH[] = "ScliPath";
static const char KEY_POWERMT_CMD[] = "PowermtCmd";
static const char KEY_VXDMP_CMD[] = "VxdmpCmd";
static const char KEY_VXDISK_CMD[] = "VxDiskCmd";
static const char KEY_LINUX_NUM_PARTITIONS[] = "LinuxNumberOfPartitions";
static const char KEY_VXVSET_CMD[] = "VxVsetCmd";
/* This can be comma separated */
static const char KEY_MPXIO_DRVNAMES[] = "MpxioDriverNames";
static const char KEY_SCDIDADM_CMD[] = "ScdidadmCmd";
static const char KEY_GREP_CMD[] = "GrepCmd";
static const char KEY_AWK_CMD[] = "AwkCmd";
static const char KEY_EMLXADM_PATH[] = "EmlxAdmPath";
static const char KEY_VM_CMDS[] = "VMCommands";
static const char KEY_VM_CMDS_FOR_PATS[] = "VMCommandsForPatterns";
static const char KEY_VM_PATS[] = "VMPatterns";
static const char KEY_USE_LINUX_DEVICE_TXT[] = "UseLinuxDeviceTxt";
static const char KEY_LINUX_DISK_DISC_CMD[] = "LinuxDiskDiscoveryCommand";
static const char KEY_FABRIC_WWN[] = "FabricWWN";
static const char KEY_DELAY_BETWEEN_APPSHUTDOWN_AND_TAGISSUE[] = "DelayBetweenAppShutdownAndTagIssue";
static const char KEY_MAX_WAITTIME_FOR_TAGARRIVAL[] = "MaxWaitTimeForTagArrival";
static const char KEY_IS_CX_PATCHED[] = "IsCXPatched";
static const char KEY_PROFILE_DEVICE_LIST[] = "ProfileDeviceList";
static const char KEY_GET_MAX_DIRECTSYNC_FLUSH_TO_RETN_SIZE[] = "MaxDirectSyncFlushToRetnSizeInMBs";
static const char KEY_GET_MAX_VACP_SERVICE_THREADS[] = "MaxVacpServerThreads";
static const char KEY_GET_VACP_TO_CX_DELAY[] = "VacpToCXCommDelay";
static const char KEY_VOLUME_RETRIES[] = "VolumeRetries";
static const char KEY_VOLUME_RETRY_DELAY[] = "VolumeRetryDelay";
static const char KEY_GET_ENFORCER_DELAY[] = "EnforcerDelay";
static const char KEY_DIRECTSYNC_PARTITIONS[] = "DirectSyncPartitions";
static const char KEY_DIRECTSYNC_PARTITIONSIZE[] = "DirectSyncPartitionSize";
static const char KEY_MEMORY_BASED_SYNC_APPLY_ENABLED[] = "MemoryBasedSyncApplyEnabled";
static const char KEY_MAX_MEMORY_CAP_FOR_RESYNC[] = "MaxMemoryCapForResync";
static const char KEY_RESYNC_COMPRESSION_BUFFER_SZIE[] = "ResyncCompressionBufferSize";
static const char KEY_RESYNC_COMPRESSION_CHUNK_SIZE[] = "ResyncCompressionChunkSize";
static const char KEY_MAX_SPACE_PER_CDP_DATA_FILE[] = "MaxSpacePerCdpDataFile";

static const char KEY_MAX_RETRY_ATTEMPTS_TO_SHUTDOWN_VXSERVICE[] = "MaxRetryAttemptsToShutdownVXAgentService";

static const char KEY_PREALLOCATE_CDP_DATA_FILE[] = "PreAllocateCdpDataFile";
static const char KEY_VSNAP_LOCAL_PERSISTENCE[] = "VsnapLocalPersistence";
static const char KEY_DI_CHECK[] = "DICheck";
static const char KEY_SVD_CHECK[] = "SVDCheck";
static const char KEY_DIRECT_TRANSFER[] = "DirectTransfer";
static const char KEY_ENABLE_DIFF_FILE_CHECKSUMS[] = "EnableDiffFileChecksums";
static const char KEY_COMPARE_IN_INITIAL_DIRECTSYNC[] = "CompareInInitialDirectSync";
static const char KEY_ENABLE_PROCESS_CLUSTER_PIPE[] = "EnableProcessClusterPipeline";
static const char KEY_MAX_HCDS[] = "MaxNumberOfHcdsAtCx";
static const char KEY_MAX_CLUSTER_BITMAPS[] = "MaxNumberOfClusterBitmapsAtCx";
static const char KEY_SECS_TO_WAITFORHCD[] = "SecsToWaitForHcdSend";
static const char KEY_SECS_TO_WAITFORCLUSTERBITMAP[] = "SecsToWaitForClusterBitmapSend";
static const char KEY_TS_CHECK[] = "TimeStampsCheck";
static const char KEY_DIRECTSYNC_BLOCKSIZE_INKB[] = "DirectSyncBlockSizeInKB";
static const char KEY_DI_VERIFY[] = "DIVerify";
static const char KEY_DPMEASURE_PERF[] = "DPMeasurePerf";
static const char KEY_DPPROFILE_SRCIO[] = "DPProfileSourceIo";
static const char KEY_DPPROFILE_VOLREAD[] = "DPProfileVolRead";
static const char KEY_DPPROFILE_VOLWRITE[] = "DPProfileVolWrite";
static const char KEY_DPPROFILE_CDPWRITE[] = "DPProfileCdpWrite";
static const char KEY_TARGET_CHECKSUMS_DIR[] = "TargetChecksumsDir";
static const char KEY_MAX_TIME_RANGE_PER_CDP_DATA_FILE[] = "GetMaxTimeRangePerCdpDataFile";
static const char KEY_REGISTER_CLUSTER_INFO[] = "RegisterClusterInfo";
static const char KEY_MONITOR_VOLUMES[] = "MonitorVolumes";
static const char KEY_REPORT_FULL_DEVICE_NAMES_ONLY[] = "ReportFullDeviceNamesOnly";
static const char KEY_AGENT_MODE[] = "AgentMode";
static const char KEY_PENDING_DATA_REPORTER_INTERVAL[] = "PendingDataReporterInterval";
static const char KEY_PENDING_VSNAP_REPORTER_INTERVAL[] = "PendingVsnapReporterInterval";
static const char KEY_NUMBER_OF_BATCH_REQUEST_TO_CX[] = "NumberOfBatchRequestsToCX";
static const char KEY_DONOT_RUN_DISKPART[] = "DoNotRunDiskPart";
static const char KEY_ARCHIVE_TOOL_PATH[] = "ArchiveToolPath";

static const char KEY_UPDATED_UPGRADE_TO_CX[] = "UpdatedCX";
static const char KEY_UPGRADED_VERSION[] = "UpgradedVersion";
static const char KEY_UPGRADE_PHP[] = "UpgradePHP";
static const char KEY_UPDATE_UPGRADE_WAITTIME[] = "UpdateUpgradeWaitTime";
static const char KEY_RESYNC_STALE_FILE_CLEANUP_INTERVAL[] = "ResyncStaleFilesCleanupInterval";
static const char KEY_DEFAULT_RESYNC_STALE_FILE_CLEANUP_INTERVAL[] = "300";
static const char KEY_SHOULD_CLEANUP_CORRUPT_SYNC_FILE[] = "ShouldCleanupCorruptSyncFile";
static const char KEY_RESYNC_UPDATE_INTERVAL[] = "ResyncUpdateInterval";
static const char DEFAULT_RESYNC_UPDATE_INTERVAL[] = "300";
static const char KEY_IR_METRICS_REPORT_INTERVAL[] = "IRMetricsReportInterval";
static const char KEY_LOG_RESYNC_PROGRESS_INTERVAL[] = "LogResyncProgressInterval";
static const char KEY_RESYNC_SLOW_PROGRESS_THRESHOLD[] = "ResyncSlowProgressThreshold";
static const char KEY_RESYNC_NO_PROGRESS_THRESHOLD[] = "ResyncNoProgressThreshold";
static const char KEY_CONFIGSTORE_NAME[] = "ConfigStoreName";

static const SV_UINT DEFAULT_LENGTHFOR_FILESYSTEM_CLUSTERSQUERY = 1024 * 1024 * 1024; // 1 GB

static const char KEY_CONFIGSTORE_LOCATION[] = "ConfigStoreLocation";
static const char KEY_GUESTACCESS[] = "GuestAccess";
static const char KEY_S2_REINCARNATION_INTERVAL[] = "S2MaxThreadLifeTimeInSec";
static const char KEY_CURLVERBOSE[] = "CurlVerbose";
static const int DEFAULT_CURLVERBOSE = 0;
static const char KEY_SHOULD_S2_RENAME_DIFFERENTIAL_FILES[] = "ShouldS2RenameDifferentialFiles";
static const char KEY_SHOULD_PROFILE_DIRECT_SYNC[] = "ProfileDirectSync";
static const char KEY_S2_STRICTMODE[] = "S2StrictMode";
static const char KEY_REPEATING_ALERT_INTERVAL_IN_SECS[] = "RepeatingAlertIntervalInSeconds";
static const char KEY_ROLE[] = "Role";
static const char KEY_REGISTER_LABEL_ON_DISKS[] = "RegisterLayoutOnDisks";
static const char KEY_COMPARE_HCD[] = "CompareHcd";
static const char KEY_DIRECTSYNC_IO_BUFFER_COUNT[] = "DirectSyncIOBufferCount";
static const char KEY_PIPELINE_READWRITE_INDIRECTSYNC[] = "PipelineReadWriteInDirectSync";
static const char KEY_TRANSPORT_FLUSH_THRESHOLD_FORDIFF[] = "TransportFlushLimitForDiff";
static const char KEY_PROFILE_DIFFS[] = "ProfileDiffs";
static const char KEY_PROFILE_DIFFERENTIAL_RATE[] = "ProfileDifferentialRate";
static const char KEY_PROFILE_DIFFERENTIAL_RATE_INTERVAL[] = "ProfileDifferentialRateInterval";
static const char KEY_LENGTHFOR_FILESYSTEM_CLUSTERSQUERY[] = "LengthForFileSystemClustersQuery";
static const char KEY_WAIT_TIME_FOR_SRCLUN_VALIDITY[] = "WaitTimeForSourceLunValidity";
static const char KEY_SOURCE_READ_RETRIES[] = "SourceReadRetries";
static const char KEY_ZEROS_FOR_SOURCE_READ_FAILURES[] = "ZerosForSourceReadFailures";
static const char KEY_SOURCE_READ_RETRIES_INTERVAL[] = "SourceReadRetriesInterval";    /* TODO: add seconds ? */
static const char KEY_EXPECTED_MAX_DIFFFILE_SIZE[] = "MaxExpectedDiffFileSize";
static const char KEY_PENDING_CHANGES_UPDATE_INTERVAL[] = "PendingChangesUpdateInterval";
static const char KEY_SHOULD_ISSUE_SCSICMD[] = "IssueScsiCommand";
static const char KEY_CLUSSVC_RETRY_TIME_IN_SECONDS[] = "ClusSvcRetryTimeInSeconds";
static const char KEY_GET_SCSI_ID[] = "GetScsiId";
static const char KEY_CX_DATA[] = "CxData";
static const char KEY_ENABLE_FILE_XFERLOG[] = "EnableFileXferLog";
static const char KEY_METADATA_READ_BUFLEN[] = "MetadataReadBufferLength";
static const char KEY_MIRROR_RESYNC_EVENT_WAITTIME[] = "MirrorResyncEventWaitTime";
static const char KEY_ENABLE_VOLUMEMONITOR[] = "EnableVolumeMonitor";
static const char KEY_CM_ENABLE_SVD_CHECK[] = "CMEnableSVDCheck";
static const char KEY_CDPCLI_EXE_PATHNAME[] = "CdpcliExePathname";
static const char KEY_ENFORCER_ALERT_INTERVAL[] = "EnforcerAlertInterval";
static const int DEFAULT_ENFORCER_ALERT_INTERVAL = 120;
static const char KEY_DATAPROTECTION_EXIT_TIME[] = "DPExitTime";
static const char KEY_DEFAULT_SNAPSHOT_INTERVAL[] = "SnapshotInterval";
static const char KEY_DEFAULT_NOTIFY_DIFF_INTERVAL[] = "NotifyDiffInterval";
static const char KEY_VSNAPLOCKBOOKMARK[] = "VSNAPRetainBookmark";
static const char KEY_VALIDATE_INSTALLER_CHECKSUM[] = "ValidateInstallerChecksum";
static const char KEY_UPGRADE_DIRECTORY_CREATION_RETRY_COUNT[] = "UpgradeDirectoryCreationRetryCount";
static const char KEY_UPGRADE_DIRECTORY_DELETION_RETRY_COUNT[] = "UpgradeDirectoryDeletionRetryCount";
static const uint32_t DEFAULT_UPGRADE_DIRECTORY_CREATION_RETRY_COUNT = 3;
static const uint32_t DEFAULT_UPGRADE_DIRECTORY_DELETION_RETRY_COUNT = 3;
static const char KEY_UPGRADE_DIRECTORY_CREATION_RETRY_INTERVAL[] = "UpgradeDirectoryCreationRetryInterval";
static const char KEY_UPGRADE_DIRECTORY_DELETION_RETRY_INTERVAL[] = "UpgradeDirectoryDeletionRetryInterval";
static const uint32_t DEFAULT_UPGRADE_DIRECTORY_CREATION_RETRY_INTERVAL = 1;
static const uint32_t DEFAULT_UPGRADE_DIRECTORY_DELETION_RETRY_INTERVAL = 1;
static const char KEY_INSTALLER_DOWNLOAD_RETRY_COUNT[] = "InstallerDownloadRetryCount";
static const uint32_t DEFAULT_INSTALLER_DOWNLOAD_RETRY_COUNT = 1;
static const char KEY_INSTALLER_UNZIP_RETRY_COUNT[] = "InstallerUnzipRetryCount";
static const int DEFAULT_INSTALLER_UNZIP_RETRY_COUNT = 3;
static const char KEY_INSTALLER_UNZIP_RETRY_INTERVAL[] = "InstallerUnzipRetryInterval";
static const uint32_t DEFAULT_INSTALLER_UNZIP_RETRY_INTERVAL = 1;
static const char KEY_IS_CREDENTIAL_LESS_DISCOVERY[] = "IsCredentialLessDiscovery";
static const char KEY_SWITCH_APPLIANCE_STATE[] = "SwitchApplianceState";
static const char KEY_VACP_STATE[] = "VacpState";
static const char KEY_MIGRATION_STATE[] = "MigrationState";

static const char KEY_MAXDIFF_FS_RAW_SIZE[] = "MaxDifferenceBetweenFSandRawSize";

static const int DEFAULT_DATAPROTECTION_EXIT_TIME = 16200;

static const int DEFAULT_VSNAP_WRITEDATA_LENGTH = 67108864;
static const int DEFAULT_SNAPSHOT_INTERVAL = 86400;
static const unsigned int DEFAULT_WAITTIME_FOR_SRCVALIDITY = 180;
static const unsigned int DEFAULT_LINUX_NUM_PARTITIONS = 16;
static const int DEFAULT_NOTIFY_DIFF_INTERVAL = 60;
static const char KEY_REGISTER_HOST_INTERVAL[] = "RegisterHostInterval";
static const char KEY_TRANSPORT_ERROR_LOG_INTERVAL[] = "TransportErrorLogInterval";
static const char KEY_DISK_READ_ERROR_LOG_INTERVAL[] = "DiskReadErrorLogInterval";
static const char KEY_DISK_NOT_FOUND_ERROR_LOG_INTERVAL[] = "DiskNotFoundErrorLogInterval";
static const char KEY_SRC_TELEMETRY_POLL_INTERVAL[] = "SrcTelemetryPollInterval";
static const char KEY_SRC_TELEMETRY_START_DELAY[] = "SrcTelemetryStartDelay";
static const char KEY_MONITOR_HOST_INTERVAL[] = "MonitorHostInterval";
static const char KEY_RCM_DETAILS_POLL_INTERVAL[] = "RcmDetailsPollInterval";
static const char KEY_MONITOR_HOST_START_DELAY[] = "MonitorHostStartDelay";
static const char KEY_MONITOR_HOST_CMD_LIST[] = "MonitorHostCmdList";
static const char KEY_RECOVERY_CLEANUP_FILE_LIST[] = "RecoveryCleanupFileList";
static const char KEY_AZURE_SERVICES_ACCESS_CHECK_CMD[] = "AzureServicesAccessCheckCmd";
static const char KEY_AZURE_SERVICES[] = "AzureServices";
static const int DEFAULT_REGISTER_HOST_INTERVAL = 180;
static const int DEFAULT_TRANSPORT_ERROR_LOG_INTERVAL = 600;
static const int DEFAULT_DISK_READ_ERROR_LOG_INTERVAL = 600;
static const int DEFAULT_DISK_NOT_FOUND_ERROR_LOG_INTERVAL = 600;
static const int DEFAULT_SRC_TELEMETRY_POLL_INTERVAL = 14400; // 4 Hrs
static const int DEFAULT_SRC_TELEMETRY_START_DELAY = 900; // 15 min
static const int DEFAULT_MONITOR_HOST_INTERVAL = 43200; // 12 Hrs
static const int DEFAULT_RCM_DETAILS_POLL_INTERVAL = 300; // 5 min
static const int DEFAULT_MONITOR_HOST_START_DELAY = 3600; // in sec, 60 min
static const char KEY_INITIAL_SETTING_CALL_INTERVAL[] = "InitialSettingCallInterval";
static const char KEY_SETTINGS_CALL_INTERVAL[] = "SettingsCallInterval";    /* call interval for all settings in case of backup agent */
static const int DEFAULT_INITIAL_SETTING_CALL_INTERVAL = 90;
static const int DEFAULT_SETTINGS_CALL_INTERVAL = 60;
static const char KEY_MAX_SPARSE_SIZE[] = "SparseFileMaxSize";
static const char KEY_CONSISTENCY_OPTIONS[] = "ConsistencyOptions";
static const char KEY_APPCONSISTENT_EXCLUDED_VOLUMES[] = "AppConsistentExcludeVolumes";
static const int DEFAULT_DI_CHECK = 0;
static const int DEFAULT_SVD_CHECK = 0;
static const int DEFAULT_DIRECT_TRANSFER = 0;
static const int DEFAULT_ENABLE_DIFF_FILE_CHECKSUMS = 1; // make it zero in release and master branches by default
static const SV_UINT DEFAULT_MAX_HCDS = 1000;
static const SV_UINT DEFAULT_MAX_CLUSTER_BITMAPS = 1000; /* TODO: should this be thousand ? as already max hcds are 1000 */
static const SV_UINT DEFAULT_SECS_TO_WAITFORHCD = 15;
static const SV_UINT DEFAULT_SECS_TO_WAITFORCLUSTERBITMAP = 15;
static const int DEFAULT_TS_CHECK = 1;
static const SV_ULONG DEFAULT_DIRECTSYNC_BLOCKSIZE_INKB = 1024;
static const int DEFAULT_DI_VERIFY = 0;
static const int DEFAULT_MAX_SPARSE_SIZE = 0;
static const int DEFAULT_DPMEASURE_PERF = 1;
static const int DEFAULT_DPPROFILE_SRCIO = 0;
static const int DEFAULT_DPPROFILE_VOLREAD = 0;
static const int DEFAULT_DPPROFILE_VOLWRITE = 0;
static const int DEFAULT_DPPROFILE_CDPWRITE = 0;
static const int DEFAULT_VSNAPLOCKBOOKMARK = 0;
#ifdef SV_WINDOWS
static const int DEFAULT_DPUNBUFFEREDIO = 1;
#else
static const int DEFAULT_DPUNBUFFEREDIO = 0;
#endif

#ifdef SV_WINDOWS
static const int DEFAULT_CDP_COMPRESSION = 1;
#else
static const int DEFAULT_CDP_COMPRESSION = 0;
#endif

#ifdef SV_WINDOWS
static const int DEFAULT_VIRTUALVOLUME_COMPRESSION = 1;
static const int DEFAULT_MAXIMUM_DISK_INDEX = 128;
static const int DEFAULT_MAXIMUM_CONS_MISSING_DISK_INDEX = 20;
static const int DEFAULT_DISK_RECOVERY_WAIT_TIME_SEC = 0;
static const int DEFAULT_W2K8_DISK_RECOVERY_WAIT_TIME_SEC = 30;
#else
static const int DEFAULT_VIRTUALVOLUME_COMPRESSION = 0;
static const int DEFAULT_MAXIMUM_DISK_INDEX = 0;
static const int DEFAULT_MAXIMUM_CONS_MISSING_DISK_INDEX = 0;
static const int DEFAULT_DISK_RECOVERY_WAIT_TIME_SEC = 0;
#endif

static const char KEY_RENAME_FAILURE_RETRY_INTERVAL_IN_SEC[] = "RenameFailureRetryIntervalInSec";
static const char KEY_RENAME_FAILURE_RETRY_COUNT[] = "RenameFailureRetryCount";
static const int DEFAULT_RENAME_FAILURE_RETRY_INTERVAL_IN_SEC = 5;
static const int DEFAULT_RENAME_FAILURE_RETRY_COUNT = 3;

static const int DEFAULT_UNCOMPRESS_RETRIES = 3;
static const int DEFAULT_UNCOMPRESS_RETRY_INTERVAL = 30;
static const int DEFAULT_MIN_DISK_FREESPACE_FOR_UNCOMPRESSION = 134217728;//128MB
static const SV_ULONGLONG DEFAULT_MAX_CDPV3_COW_FILESIZE = 4294967295UL;//4 GB
static const int DEFAULT_IGNORE_CORRUPTED_DIFFS = 0; //false
static const SV_ULONGLONG DEFAULT_CLUSTER_SYNC_CHUNK_SIZE = 1024 * 1024 * 1024;
static const int DEFAULT_MAX_FASTSYNC_CHUNK_SIZE = 64 * 1024 * 1024;
static const int DEFAULT_MAX_FASTSYNC_CHUNK_SIZE_E2A = 256 * 1024 * 1024;

static const char KEY_ASYNCH_OPTIMIZATIONS[] = "DPAsynchOptimizations";
static const char KEY_ASYNCH_OPTIMIZATIONS_FOR_PHYSICALVOLUMES[] = "DPAsynchOptimizationsForPhysicalVolumes";
static const char KEY_USE_NEW_APPLY_ALGORITHM[] = "DPUseNewApplyAlgorithm";
static const char KEY_MAXASYNCH_IOS[] = "DPMaxAsynchIos";
static const char KEY_MAX_MEM_PER_DIFFSYNC_FILE[] = "DPMaxMemoryPerDiffSyncFile";
static const char KEY_MAX_MEM_PER_RESYNC_FILE[] = "DPMaxMemoryPerResyncFile";
static const char KEY_DPUNBUFFEREDIO[] = "DPUnBufferedIo";
static const char KEY_UNCOMPRESS_RETRIES[] = "DPUncompressRetries";
static const char KEY_UNCOMPRESS_RETRY_INTERVAL[] = "DPUncompressRetryInterval";
static const char KEY_MIN_DISK_FREESPACE_FOR_UNCOMPRESSION[] = "DPMinDiskFreeSpaceForUncompression";
static const char KEY_IGNORE_CORRUPTED_DIFFS[] = "DPIgnoreCorruptedDiffs";
static const char KEY_CDP_COMPRESSION[] = "CDPCompressionOn";
static const char KEY_VIRTUALVOLUME_COMPRESSION[] = "VirtualVolumeCompression";
static const char KEY_DPBM_ASYNCHIO[] = "DPBMAsynchIo";
static const char KEY_DPBM_ASYNCHIO_FOR_PHYSICALVOLUMES[] = "DPBMAsynchIoForPhysicalVolumes";
static const char KEY_DPBM_CACHING[] = "DPBMCaching";
static const char KEY_DPBM_CACHESIZE[] = "DPBMCacheSize";
static const char KEY_DPBM_UNBUFFEREDIO[] = "DPBMUnBufferedIo";
static const char KEY_DBBM_BLOCKSIZE[] = "DPBMBlockSize";
static const char KEY_DPBM_BLOCKSPERENTRY[] = "DPBMBlocksPerEntry";
static const char KEY_DPBM_COMPRESSION[] = "DPBMCompressionOn";
static const char KEY_DPBM_MAXIOS[] = "DPBMMaxIos";
static const char KEY_DPBM_MAXMEMFORIO[] = "DPBMMaxMemForIo";
static const char KEY_DPDELAY_BEFORE_EXIT_ONERROR[] = "DPDelayBeforeExitOnError";
static const int DEFAULT_DPDELAY_BEFORE_EXIT_ONERROR = 60;//60 seconds
static const char KEY_MAX_CDPV3_COW_FILESIZE[] = "DPMaxCdpv3CowFileSize";
static const char KEY_MAX_CDP_IO_SIZE[] = "DPMaxIOSize";
static const char KEY_MAX_CDP_SNAPSHOT_IO_SIZE[] = "CDPSnapshotIOSize";
static const char KEY_DP_CACHEVOLUMEHANDLE[] = "DPCacheVolumeHandle";
static const char KEY_DPMAX_RETENTIONFILE_TO_CACHE[] = "DPMaxRetentionFileToCache";
static const char KEY_HOST_AGENT_NAME[] = "HostAgentName";
static const char KEY_MAX_UNMOUNT_RETRIES[] = "MaxUnmountRetries";
static const char KEY_USE_FSAWARE_COPY[] = "FSAwareSnapshotCopy";
static const char KEY_MAXMEMFOR_READING_FSBITMAP[] = "MaxMemForReadingFSBitmap";
static const char KEY_ORACLE_DISCOVERY[] = "OracleDiscovery";
static const char KEY_MAXIMUM_DISK_INDEX[] = "MaximumDiskIndex";
static const char KEY_MAXIMUM_CONS_MISSING_DISK_INDEX[] = "MaxConsMissingDiskIndex";
static const char KEY_DISK_RECOVERY_WAIT_TIME_SEC[] = "DiskRecoveryWaitTimeSec";
static const char KEY_MAX_WMI_CONNECTION_TIMEOUT_SEC[] = "MaxWmiConnectionTimeout";
static const char KEY_MAX_SUPPORTED_PARTS_UEFI_BOOT[] = "MaximumSupportedPartitionsOnUefiBoot";

#ifdef SV_WINDOWS
static const int DEFAULT_CACHEVOLUMEHANDLE = 0;
#else
static const int DEFAULT_CACHEVOLUMEHANDLE = 1;
#endif

static const int DEFAULT_DPMAX_RETENTIONFILE_TO_CACHE = 100;
static const int DEFAULT_MAX_UNMOUNT_RETRIES = 2;

static const int DEFAULT_DPBM_ASYNCHIO = 1;
static const int DEFAULT_DPBM_CACHING = 1;
static const int DEFAULT_DPBM_CACHESIZE = 1048576;

#ifdef SV_WINDOWS
static const int DEFAULT_DPBM_UNBUFFEREDIO = 1;
#else
static const int DEFAULT_DPBM_UNBUFFEREDIO = 0;
#endif

static const int DEFAULT_DPBM_BLOCKSIZE = 4096;
static const int DEFAULT_DPBM_BLOCKSPERENTRY = 1024;

#ifdef SV_WINDOWS
static const int DEFAULT_DPBM_COMPRESSION = 1;
#else
static const int DEFAULT_DPBM_COMPRESSION = 0;
#endif

#ifdef SV_WINDOWS
static const char DEFAULT_AZURE_SERVICES[] = "WindowsAzureGuestAgent,WindowsAzureTelemetryService,RdAgent";
#else
static const char DEFAULT_AZURE_SERVICES[] = "";
#endif

static const int DEFAULT_DPBM_MAXIOS = 256;
static const int DEFAULT_DPBM_MAXMEMFORIO = 1048576;
static const SV_UINT DEFAULT_DP_MAX_IO_SIZE = 16777216;
static const SV_UINT DEFAULT_CDP_SNAPSHOT_IO_SIZE = 524288;


static const char KEY_TRANSPORT_MAX_BUFFER_SIZE[] = "TransportMaxBufferSize";
static const char KEY_TRANSPORT_RESPONSE_TIMEOUT_SECONDS[] = "TransportResponseTimeoutSeconds";
static const char KEY_TRANSPORT_LOW_SPEED_TIMEOUT_SECONDS[] = "TransportLowSpeedTimeoutSeconds";
static const char KEY_TRANSPORT_CONNECT_TIMEOUT_SECONDS[] = "TransportConnectTimeoutSeconds";
static const char KEY_TRANSPORT_WRITE_MODE[] = "TransportWriteMode";

// name of file for caching the initial settings
//
static std::string const CachedConfigFile("config.dat");
static std::string const CachedVolumesFile("volumes.dat");
static std::string const ConsistencySettingsCacheFile("ConsistencySettings.json");

// V2A RCM related config file names
static std::string const SourceConfigFile("SourceConfig.json");
static std::string const ResyncBatchCacheFile("ResyncBatch.json");

static const char KEY_MANUAL_RESYNC_START_THRESHOLD_IN_SECS[] = "ManualResyncStartThresholdInSecs";
static const SV_UINT DEFAULT_MANUAL_RESYNC_START_THRESHOLD_IN_SECS = 60 * 60;
static const char KEY_INITIAL_REPLICATION_START_THRESHOLD_IN_SECS[] = "InitialReplicationStartThresholdInSecs";
static const SV_UINT DEFAULT_INITIAL_REPLICATION_START_THRESHOLD_IN_SECS = 60 * 60 * 6;
static const char KEY_AUTO_RESYNC_START_THRESHOLD_IN_SECS[] = "AutoResyncStartThresholdInSecs";
static const SV_UINT DEFAULT_AUTO_RESYNC_START_THRESHOLD_IN_SECS = 60 * 60 * 24;
static std::string const VxProtectedDeviceDetailPeristentFile("vxprotdevdetails.dat");
static std::string const VxPlatformTypeForDriverPersistentFile("vxplatformtype.dat");
static std::string const CachedDisksLabelChecksumFile("diskslabelchecksum.dat");

static const char KEY_CACHEMGR_EXE_PATHNAME[] = "CacheMgrExePathname";
static const char CACHEMGR_EXE_BASENAME[] = "cachemgr";
static const char KEY_CACHEMGR_EXIT_TIME[] = "CacheMgrExitTime";
static const char KEY_MAX_DISKUSAGEPERREPLICATION_VALUE_NAME[] = "MaxDiskUsagePerReplication";
static const char KEY_NWTHREADSPERREPLICATION_VALUE_NAME[] = "NWThreadsPerReplication";
static const char KEY_IOTHREADSPERREPLICATION_VALUE_NAME[] = "IOThreadsPerReplication";
static const char KEY_CMRETRYDELAY_VALUE_NAME[] = "CMRetryDelayInSeconds";
static const char KEY_CMMAXRETRIES_VALUE_NAME[] = "CMMaxRetries";
static const char KEY_CMIDLEWAITTIME_VALUE_NAME[] = "CMIdleWaitTimeInSeconds";
static const char KEY_OUTOFORDERSEQ_VALUE_NAME[] = "AllowOutOfOrderSeq";
static const char KEY_OUTOFORDERTS_VALUE_NAME[] = "AllowOutOfOrderTS";
static const char KEY_IGNOREOUTOFORDER_VALUE_NAME[] = "DPIgnoreOOD";
static const char KEY_ALLOW_ROOTVOLUME_FOR_RETENTION_NAME[] = "AllowRootVolumeForRetention";

static const char KEY_CDPMGR_EXE_PATHNAME[] = "CdpMgrExePathname";

//Adding a new parameter for CDPMGR exit time as patch does not update the drscount.conf values.
//The new parameter is refferred.
static const char KEY_CDPMGR_EXIT_TIME_V2[] = "CDPMgrExitTimeV2";
static const int DEFAULT_CDPMGR_EXIT_TIME_V2 = 604800;

static const char KEY_CDP_READAHEAD_ENABLED[] = "CDPReadAheadEnabled";
static const char KEY_CDP_READAHEAD_THREADS[] = "CDPReadAheadThreadCount";
static const char KEY_CDP_READAHEAD_FILECOUNT[] = "CDPReadAheadFileCount";
static const char KEY_CDP_READAHEAD_LENGTH[] = "CDPReadAheadLength";


static const char MPXIODRVNAMES[] = "/scsi_vhci/,vhci";
static const char KEY_CDPMGR_UPDATECX_PER_TARGET_VOLUME[] = "CDPMgrUpdateCxPerTargetVolume";
static const char KEY_CDPMGR_EVENT_TIMERANGE_RECORDS_PER_BATCH[] = "CDPMgrEventTimeRangeRecordsPerBatch";
static const char KEY_CDPMGR_SEND_UPDATE_ATONCE[] = "CDPMgrSendUpdatesAtOnce";
static const char KEY_CDPMGR_DELETE_UNUSABLE_POINTS[] = "CDPMgrDeleteUnusablePoints";
static const char KEY_CDPMGR_DELETE_STALEFILES[] = "CDPMgrDeleteStaleFiles";

static const char KEY_AZURE_BLOBS_OPERATION_MAXIMUM_TIMEOUT[] = "AzureBlobOpsMaxTimeoutSec";
static const uint64_t DEFAULT_AZURE_BLOBS_OPERATION_MAXIMUM_TIMEOUT = 30;
static const char KEY_AZURE_BLOBS_OPERATION_MINIMUM_TIMEOUT[] = "AzureBlobOpsMinTimeoutSec";
static const uint64_t DEFAULT_AZURE_BLOBS_OPERATION_MINIMUM_TIMEOUT = 3;
static const char KEY_AZURE_BLOBS_OPERATION_TIMEOUT_RESET_INTERVAL[] = "AzureBlobOpsTimeoutResetIntervalHours";
static const unsigned int DEFAULT_AZURE_BLOBS_OPERATION_TIMEOUT_RESET_INTERVAL = 4;

static const SV_ULONG DEFAULT_EVENT_TIMERANGE_RECORDS_PER_BATCH = 512;

static const char KEY_MAX_FASTSYNC_PROCHCD_THREADS[] = "MaxFastSyncProcessChecksumThreads";
static const char KEY_MAX_CLUSTER_PROCESS_THREADS[] = "MaxFastSyncProcessClusterThreads";
static const int DEFAULT_MIN_FASTSYNC_PROCHCD_THREADS = 1;
static const int DEFAULT_MAX_FASTSYNC_PROCHCD_THREADS = 32;
static const int DEFAULT_MIN_CLUSTER_PROCESS_THREADS = 1;
static const int DEFAULT_MAX_CLUSTER_PROCESS_THREADS = 32;

static const char KEY_MAX_FASTSYNC_GENHCD_THREADS[] = "MaxFastSyncGenerateChecksumThreads";
static const int DEFAULT_MIN_FASTSYNC_GENHCD_THREADS = 1;
static const int DEFAULT_MAX_FASTSYNC_GENHCD_THREADS = 32;

static const char KEY_MAX_GENCLUSTER_BITMAP_THREADS[] = "MaxGenerateClusterBitmapThreads";
static const int DEFAULT_MIN_GENCLUSTER_BITMAP_THREADS = 1;
static const int DEFAULT_MAX_GENCLUSTER_BITMAP_THREADS = 32;

static const int DEFAULT_MIN_FASTSYNC_APPLY_THREADS = 4;
static const int DEFAULT_MAX_FASTSYNC_APPLY_THREADS = 32;
static const int SV_DEFAULT_RETRIES = 5;
static const int SV_DEFAULT_RETRY_DELAY = 5; // 5 secs

static const int DEFAULT_MIN_OUTPOST_THREADS = 4;
static const int DEFAULT_MAX_OUTPOST_THREADS = 32;

static const int DEFAULT_CACHEMGR_EXIT_TIME = 16200;
static const SV_ULONGLONG DEFAULT_MAXDISKUSAGE_PERREPLICATION = 536870912; // 512mb

static const char KEY_CM_LOCAL_LOG_SIZE[] = "CMLocalLogSize";
static const int DEFAULT_LOCAL_LOG_SIZE = 10485760; //10mb

static const int SV_DEFAULT_LOG_MAX_NUM_COMPLETED_FILES = 3;
static const int SV_DEFAULT_LOG_CUT_INTERVAL = 300; // 5 minutes
static const int SV_DEFAULT_LOG_MAX_FILE_SIZE = 10485760; // 10 MB
static const int SV_DEFAULT_LOG_MAX_FILE_SIZE_TELEMETRY = 1048576; // 1 MB
static const int DEFAULT_MIN_NW_THREADS = 4;
static const int DEFAULT_MAX_NW_THREADS = 32;

static const int DEFAULT_MIN_IO_THREADS = 4;
static const int DEFAULT_MAX_IO_THREADS = 32;

static const int CM_DEFAULT_RETRY_DELAY = 30; // 30 secs
static const int CM_DEFAULT_RETRIES = 10;
static const int CM_DEFAULT_IDLE_WAIT_TIME = 30; // 30 secs

static const SV_ULONG CM_DEFAULT_DPP_USAGE_IN_PERCENTAGE = 6;
static const SV_ULONG CM_MAX_DPP_USAGE_IN_PERCENTAGE = 10;
static const SV_ULONG CM_DEFAULT_MAX_DPP_USAGE_IN_MB = 4096;
static const SV_ULONG CM_DEFAULT_MIN_DPP_USAGE_IN_MB = 256;
static const SV_ULONG CM_DEFAULT_DPP_ALIGNMENT_IN_MB = 4;
static const unsigned long int DEFAULT_EXPECTED_MAX_DIFFFILE_SIZE = 10 * 1024 * 1024;

static const unsigned long DEFAULT_MIRROR_RESYNC_EVENT_WAITTIME = 65; /* 65 secs */

static const int SV_DEFAULT_OUTOFORDERSEQACTION = 0;
static const int SV_DEFAULT_OUTOFORDERTSACTION = 0;

static const char SCDIDADMCMD[] = "/usr/cluster/bin/scdidadm";

// Added by Ranjan for bug# 10404(XenServer Registration)
static const char KEY_MAX_WAITTIME_FOR_XENREGISTRATION[] = "MaxWaitTimeForXenRegistration";
static const char KEY_MAX_WAITTIME_FOR_LVACTIVATION[] = "MaxWaitTimeForLvActivation";
static const char KEY_MAX_WAITTIME_FOR_DISPLAYVMVDIINFO[] = "MaxWaitTimeForDisplayVmVdiInfo";
static const char KEY_MAX_WAITTIME_FOR_CXSTATUSUPDATE[] = "MaxWaitTimeForCxStatusUpdate";
//End of change
static const char KEY_AAPLICARION_FAILOVER_CHKDSK[] = "RunChkdsk";

static const char KEY_CMMVERIFYDIFFS_VALUE_NAME[] = "CMVerifyDiffs";

static const char KEY_CMPRESERVEBADDIFFS_VALUE_NAME[] = "CMPreserveBadDiffs";
static const char KEY_SIMULATESPARSE_VALUE_NAME[] = "CDPSimulateCoalesce";
static const char KEY_TRACKEXTRACOALESCEDFILES_VALUE_NAME[] = "CDPTrackExtraCoalesce";
static const char KEY_TRACKCOALESCEDFILES_VALUE_NAME[] = "CDPTrackCoalesce";
static const char KEY_FILTERDRIVERAVAILABLE_VALUE_NAME[] = "FilterDriverAvailable";
static const char KEY_VOLPACKDRIVERAVAILABLE_VALUE_NAME[] = "VolpackDriverAvailable";
static const char KEY_VSNAPDRIVERAVAILABLE_VALUE_NAME[] = "VsnapDriverAvailable";
static const char KEY_FULLBACKUP_SCHEDULE[] = "FullBackScheduleInSec";
static const char KEY_FULLBACKUP_ENABLED[] = "FullBackupRequired";
static const char KEY_SUCCESS_LAST_FULLBKPTIME[] = "LastSuccessfulFullBackupTimeGmt";
static const char KEY_SERIALIZER_TYPE[] = "SerializerType";
static const char KEY_START_SENTINAL[] = "StartSentinal";
static const char KEY_START_DATAPROTECTION[] = "StartDataProtection";
static const char KEY_START_CDP_MANAGER[] = "StartCDPManager";
static const char KEY_START_CACHE_MANAGER[] = "StartCacheManager";
static const char KEY_EDIT_CATCHE_PATH[] = "EditCachePath";
static const char KEY_CDP_POLICYCHECK_INTERVAL[] = "CDPPolicyCheckInterval";
static const char KEY_CDP_FREESPACECHECK_INTERVAL[] = "CDPFreeSpaceCheckInterval";
static const char KEY_CDP_LOWSPACE_TRIGGER_LOWER_THRESHOLD[] = "CDPLowSpaceTriggerLowerThreshold";
static const char KEY_CDP_LOWSPACE_TRIGGER_UPPER_THRESHOLD[] = "CDPLowSpaceTriggerUpperThreshold";
static const char KEY_CDP_LOWSPACE_TRIGGER_PERCENT[] = "CDPFreeSpaceTriggerPercent";
static const char KEY_CDP_RESERVED_RETENTION_SPACE_SIZE[] = "CDPReservedRetentionSpaceSize";
static const char KEY_DP_SECTOR_SIZE[] = "VxAlignmentSize";
static const char KEY_VOLPACK_SPARSE_ATTRIBUTE_ENABLED[] = "VolpackSparseAttribute";
static const char KEY_CDP_REDOLOG_MAX_FILE_SIZE[] = "CdpRedologMaxFileSize";
static const char KEY_FLUSH_AND_HOLD_WRITES_RETRY_ENABLED[] = "FlushAndHoldWritesRetryEnabled";
static const char KEY_FLUSH_AND_HOLD_RESUME_RETRY_ENABLED[] = "FlushAndHoldResumeRetryEnabled";
static const size_t DEFAULT_METADATA_READ_BUFFER_LENGTH = 1024 * 1024;

static const int DEFAULT_CDP_POLICYCHECK_INTERVAL = 60;
static const int DEFAULT_CDP_FREESPACECHECK_INTERVAL = 5;
static const SV_ULONGLONG DEFAULT_CDP_LOWSPACE_TRIGGER_LOWER_THRESHOLD = 1073741824; //1GB
static const SV_ULONGLONG DEFAULT_CDP_LOWSPACE_TRIGGER_UPPER_THRESHOLD = 21474836480ULL; //20GB
static const SV_ULONGLONG DEFAULT_CDP_LOWSPACE_TRIGGER_PERCENT = 5;
static const SV_ULONGLONG DEFAULT_CDP_RESERVED_RETENTION_SPACE_SIZE = 67108864; //64MB
static const SV_ULONGLONG DEFAULT_DP_SECTOR_SIZE = 512;
static const SV_UINT DEFAULT_CDP_REDOLOG_MAX_FILE_SIZE = 67108864;

static const char KEY_CDPCLI_SKIPCHECK_NAME[] = "CdpcliSkipCheck";

static const char KEY_GET_LAST_N_LINES_FROM_LOG[] = "GetLastNLines";

static const char KEY_MIN_AZURE_UPLOAD_SIZE[] = "MinAzureUploadSize";
static const int DEFAULT_MIN_AZURE_UPLOAD_SIZE = 134217728; // 128mb

static const char KEY_MIN_TIMEGAP_BETWEEN_AZURE_UPLOADS[] = "MinTimeGapAzureUpload";
static const unsigned int DEFAULT_MIN_TIMEGAP_AZURE_UPLOADS = 90; // 90 secs

static const char KEY_TIMEGAP_BETWEEN_FILEARRIVAL_CHECK[] = "TimeGapFileArrivalCheck";
static const unsigned int DEFAULT_TIMEGAP_FILEARRIVAL_CHECK = 15; // 15 secs

static const char KEY_MAX_AZURE_ATTEMPTS[] = "MaxAzureAttempts";
static const int DEFAULT_MAX_AZURE_ATTEMPTS = 10;
static const char KEY_AZURE_RETRY_DELAY_IN_SECS[] = "AzureRetryDelayInSecs";
static const int DEFAULT_AZURE_RETRY_DELAY_IN_SECS = 60;
static const char KEY_AZURE_IMPL_TYPE[] = "AzureImplType";

//For OMS Statistics and DR Metrics Collection
static const char KEY_OMS_STATS_COLLECTION_INTERVAL[] = "OmsStatsCollectionInterval";
static const int DEFAULT_OMS_STATS_COLL_INTERVAL = 60; //1 minute
static const char KEY_OMS_STATS_SENDING_INTERVAL_TO_PS[] = "OmsStatsSendingIntervalToPS";
static const int DEFAULT_OMS_STATS_SENDING_INTERVAL = 300;//5 minutes
static const char KEY_DR_METRICS_COLLECTION_INTERVAL[] = "DRMetricsCollectionInterval";
// The DR metric collection interval cannot be more than KEY_SENTINEL_EXIT_TIME (default 16200)
static const int DEFAULT_DR_METRICS_COLL_INTERVAL = 4*60*60;//4 hours

static const char KEY_SENTINEL_EXIT_TIME_V2[] = "SentinelExitTimeV2";
static const int DEFAULT_SENTINEL_EXIT_TIME_V2 = 2 * 24 * 60 * 60; // in secs, 2 days

static const char KEY_AZURE_BLOB_OPS_TIMEOUT[] = "AzureBlobOpsTimeout";
static const int DEFAULT_AZURE_BLOB_OPS_TIMEOUT = 10; // in secs

static const char KEY_AGENT_HEALTH_CHECK_INTERVAL[] = "AgentHealthCheckInterval";
static const int DEFAULT_AGENT_HEALTH_CHECK_INTERVAL = 300; // 5 min

static const char KEY_MARS_HEALTH_CHECK_INTERVAL[] = "MarsHealthCheckInterval";
static const int DEFAULT_MARS_HEALTH_CHECK_INTERVAL = 900; // 15 min

static const char KEY_MARS_SERVER_UNAVAILABLE_CHECK_INTERVAL[] = "MarsServerUnavailableCheckInterval";
static const int DEFAULT_MARS_SERVER_UNAVAILABLE_CHECK_INTERVAL = 900; // 15 min

static const char DEFAULT_UNIX_DRIVER_PERSISTENT_STORE_PATH[] = "/etc/vxagent/usr";

static const char KEY_CONSISTENCY_TAG_ISSUE_TIME_LIMIT[] = "ConsistencyTagIssueTimeLimit";
static const int DEFAULT_CONSISTENCY_TAG_ISSUE_TIME_LIMIT = 600; // 10 min

static const char KEY_RCM_REQUEST_TIMEOUT[] = "RcmRequestTimeout";
static const unsigned int DEFAULT_RCM_REQUEST_TIMEOUT = 180; // in sec, 3 min

static const char KEY_AZUREBLOB_FLUSH_THRESHOLD_FORDIFF[] = "AzureBlobFlushLimitForDiff";
static const int DEFAULT_AZUREBLOB_FLUSH_THRESHOLD_FORDIFF = 4 * 1024 * 1024; // 4 MB

static const char KEY_VACP_PARALLEL_MAX_RUN_TIME[] = "VacpExitTime";
static const int  DEFAULT_VACP_PARALLEL_MAX_RUN_TIME = 24 * 60 * 60; // in sec, 24 hours

/* 
 * Drain Barrier timeout
  * On Windows : default timeout is 3 min and max is 10 min
  * on Linux : default timeout is 40 sec and max is 60 sec
 */
static const char KEY_VACP_DRAINBARRIER_TIMEOUT[] = "VacpDrainBarrierTimeout";
#ifdef SV_WINDOWS
static const int  DEFAULT_VACP_DRAINBARRIER_TIMEOUT = 3 * 60 * 1000; // in msec, 3 min
#else
static const int  DEFAULT_VACP_DRAINBARRIER_TIMEOUT = 40 * 1000; // in msec, 40 sec
#endif
static const char KEY_VACP_TAG_COMMIT_MAX_TIMEOUT[] = "VacpTagCommitMaxTimeOut";
static const int  DEFAULT_VACP_TAG_COMMIT_MAX_TIMEOUT = 30 * 60 * 1000; // in msec, 30 min
static const char KEY_VACP_EXIT_WAIT_TIME[] = "VacpExitWaitTime";
static const int  DEFAULT_VACP_EXIT_WAIT_TIME = 2 * 60; // in sec, 2 min

static const char KEY_CONSISTENCY_LOG_PARSE_INTERVAL[] = "ConsistencyLogParseInterval";
static const int DEFAULT_CONSISTENCY_LOG_PARSE_INTERVAL = 300; // in sec, 5 min

static const char KEY_APP_CONSISTENCY_RETRY_ON_REBOOT_MAX_TIME[] = "AppConsistencyRetryOnRebootMaxTime";
static const int DEFAULT_APP_CONSISTENCY_RETRY_ON_REBOOT_MAX_TIME = 1800; // in sec, 30 min

/* churn-throughput CX session definitions start */
static const char KEY_MAX_DISK_CHURN_SUPPORTED_MBPS[] = "MaxDiskChurnSupportedMBps";
static const SV_ULONGLONG DEFAULT_MAX_DISK_CHURN_SUPPORTED_MBPS = 25; // in MBps

static const char KEY_MAX_VM_CHURN_SUPPORTED_MBPS[] = "MaxVMChurnSupportedMBps";
static const SV_ULONGLONG DEFAULT_MAX_VM_CHURN_SUPPORTED_MBPS = 50; // in MBps

static const char KEY_MAX_TIMEJUMP_FWD_ACCEPTABLE_IN_MS[] = "MaximumTimeJumpForwardAcceptableInMs";
static const SV_ULONGLONG DEFAULT_MAX_TIMEJUMP_FWD_ACCEPTABLE_IN_MS = 3 * 60 * 1000; // 3 min, in ms

static const char KEY_MAX_TIMEJUMP_BWD_ACCEPTABLE_IN_MS[] = "MaximumTimeJumpBackwardAcceptableInMs";
static const SV_ULONGLONG DEFAULT_MAX_TIMEJUMP_BWD_ACCEPTABLE_IN_MS = 60 * 60 * 1000; // 60 min in ms

static const char KEY_MIN_CONSECUTIVE_TAG_FAILURES[] = "MinConsecutiveTagFailures";
static const SV_ULONGLONG DEFAULT_MIN_CONSECUTIVE_TAG_FAILURES = 3;

static const char KEY_MAX_S2_LATENCY_BETWEEN_COMMITDB_AND_GETDB[] = "MaxS2LatencyBetweenCommitDBAndGetDb";
static const SV_ULONGLONG DEFAULT_MAX_S2_LATENCY_BETWEEN_COMMITDB_AND_GETDB = 8; // in ms

static const char KEY_CX_CLEAR_HEALTH_COUNT[] = "CxClearHealthCount";
static const SV_ULONGLONG DEFAULT_CX_CLEAR_HEALTH_COUNT = 4;

static const char KEY_MAX_WAIT_TIME_FOR_HEALTH_EVENT_COMMIT_FAILURE_IN_SEC[] = "MaxWaitTimeForHealthEventCommitFailureInSec";
static const SV_ULONGLONG DEFAULT_MAX_WAIT_TIME_FOR_HEALTH_EVENT_COMMIT_FAILURE_IN_SEC = 60; // in sec

/* churn-throughput CX session definitions end */

static const char KEY_MONITORING_CXPS_CLIENT_TIMEOUT_IN_SEC[] = "MonitoringCxpsClientTimeoutInSec";
static const SV_ULONGLONG DEFAULT_MONITORING_CXPS_CLIENT_TIMEOUT_IN_SEC = 300; // in sec

static const char KEY_PAUSE_PENDING_ACK_REPEAT_INTERVAL_IN_SECS[] = "PausePendingAckRepeatIntervalInSecs";
static const SV_ULONGLONG DEFAULT_PAUSE_PENDING_ACK_REPEAT_INTERVAL_IN_SECS = 75; // in sec (config.dat update interval + 15 sec threshold)

static const char KEY_CS_TYPE[] = "CSType";

static const int DEFAULT_UPGRADE_WAIT_TIME = 300;
static const char KEY_SETTINGS_FETCH_INTERVAL[] = "RcmSettingsFetchInterval";
static const int DEFAULT_SETTINGS_FETCH_INTERVAL = 90;  // in secs

static const char KEY_NUM_OF_RCM_JOB_WORKER_THREADS[] = "NumOfRcmJobWorkerThreads";
static const int DEFAULT_NUM_OF_RCM_JOB_WORKER_THREADS = 4;

static const char KEY_RCM_JOB_MAX_ALLOWED_TIME_INSECS[] = "RcmJobMaxAllowedTimeInSecs";
static const int DEFAULT_RCM_JOB_MAX_ALLOWED_TIME_INSECS = 60 * 60; //in secs, one hour

static const char KEY_CLOUD_PAIRING_STATUS[] = "CloudPairingStatus";

static const char KEY_PEER_REMOTE_RESYNCSTATE_PARSE_RETRY_COUNT[] = "PeerRemoteResyncStateParseRetryCount";
static const char DEFAULT_PEER_REMOTE_RESYNCSTATE_PARSE_RETRY_COUNT[] = "60";

static const char KEY_PEER_REMOTE_RESYNCSTATE_MONITOR_INTERVAL_IN_SEC[] = "PeerRemoteResyncStateMonitorInterval";
static const char DEFAULT_PEER_REMOTE_RESYNCSTATE_MONITOR_INTERVAL_IN_SEC[] = "60";

static const char KEY_MAX_RESYNC_BATCH_SIZE[] = "MaxResyncBatchSize";
static const char DEFAULT_MAX_RESYNC_BATCH_SIZE[] = "3";


#ifdef WIN32
static const int DEFAULT_AZURE_IMPL_TYPE = 0; // COM interface
#else
static const int DEFAULT_AZURE_IMPL_TYPE = 2; // unsupported interface
#endif

#ifdef SV_WINDOWS
static const bool DEFAULT_USE_FSAWARE_COPY = true;
#else
static const bool DEFAULT_USE_FSAWARE_COPY = false;
#endif
static const SV_ULONGLONG DEFAULT_MAXMEMFOR_READING_FSBITMAP = 1048576;

// Exposed default values for usage in other libraries that doesn't have
// to depend on the config file to be read successfully.
#ifdef SV_WINDOWS
const int FileConfigurator::DEFAULT_MAX_WMI_CONNECTION_TIMEOUT_SEC = 5;
const int DEFAULT_MAX_SUPPORTED_PARTS_UEFI_BOOT = 5;
#else
const int FileConfigurator::DEFAULT_MAX_WMI_CONNECTION_TIMEOUT_SEC = 0;
const int DEFAULT_MAX_SUPPORTED_PARTS_UEFI_BOOT = 0;
#endif

static const char KEY_LOG_CONTAINER_RENEWAL_RETRY_IN_SECS[] = "LogContainerRetryRenewalInSecs";
static const int DEFAULT_LOG_CONTAINER_RENEWAL_RETRY_IN_SECS = 600;

static const char KEY_HEALTHCOLLATOR_PATH[] = "HealthCollatorPath";

static const char KEY_ADDITIONAL_INSTALL_PATHS[] = "AdditionalInstallPaths";

FileConfiguratorMode FileConfigurator::s_initmode = FILE_CONFIGURATOR_MODE_VX_AGENT;

std::string FileConfigurator::s_telemetryFolderPathOnCsPrimeApplianceToAzure;
std::map<std::string, std::string> FileConfigurator::s_propertiesOnCsPrimeApplianceToAzure;
std::map<std::string, std::string> FileConfigurator::s_globalTunablesOnCsPrimeApplianceToAzure;
std::map<std::string, std::string> FileConfigurator::s_evtcollforwPropertiesOnAppliance;
boost::mutex FileConfigurator::s_lockRcmInputsOnCsPrimeApplianceToAzure;
boost::mutex FileConfigurator::s_lockEvtcollforwInputsOnAppliance;

ACE_Recursive_Thread_Mutex g_configRThreadMutex;
ACE_Recursive_Thread_Mutex g_configWriteRthreadMutex;

using namespace std;

void FormatVolumeNameForCx(std::string&);

#define SVLOG2SYSLOG(EVENT_MSG)                                         \
do {\
ACE_LOG_MSG->set_flags(ACE_Log_Msg::SYSLOG);                    \
ACE_LOG_MSG->open(ACE_TEXT("svagents"), ACE_Log_Msg::SYSLOG, NULL); \
ACE_DEBUG(EVENT_MSG);                                           \
} while (0)


FileConfiguratorMode FileConfigurator::getInitMode()
{
    return s_initmode;
}

void FileConfigurator::setInitMode(FileConfiguratorMode mode)
{
    s_initmode = mode;
    return;
}

FileConfigurator::FileConfigurator()
:m_lockInifile(ACE_TEXT_CHAR_TO_TCHAR((getConfigPathname() + ".lck").c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0)
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);

    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        if (m_inifile.open() < 0) {
            throw INMAGE_EX("couldn't open file");
        }
        ACE_Ini_ImpExp importer(m_inifile);
        // NOTE: this does not throw or return failure if file does not exist
        if (importer.import_config(ACE_TEXT_CHAR_TO_TCHAR(getConfigPathname().c_str())) < 0) {
            throw INMAGE_EX("couldn't import file")(getConfigPathname());
        }
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        initConfigParamsOnCsPrimeApplianceToAzureAgent();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        // No-op
    }
}

FileConfigurator::FileConfigurator(const std::string fileName)
:m_lockInifile(ACE_TEXT_CHAR_TO_TCHAR((fileName + ".lck").c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0)
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        if (m_inifile.open() < 0) {
            throw INMAGE_EX("couldn't open file");
        }
        ACE_Ini_ImpExp importer(m_inifile);
        // NOTE: this does not throw or return failure if file does not exist
        if (importer.import_config(ACE_TEXT_CHAR_TO_TCHAR(fileName.c_str())) < 0) {
            throw INMAGE_EX("couldn't import file")(fileName);
        }
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        initConfigParamsOnCsPrimeApplianceToAzureAgent();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        throw INMAGE_EX("invalid mode");
    }
}
std::string FileConfigurator::get(
    std::string const& section, std::string const& key) const
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);
    ACE_TString value;
    
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        ACE_Configuration_Section_Key sectionKey;
        if (m_inifile.open_section(m_inifile.root_section(), ACE_TEXT_CHAR_TO_TCHAR(section.c_str()), false, sectionKey) < 0) {
            throw INMAGE_EX("couldn't open section")(section);
        }
        if (m_inifile.get_string_value(sectionKey, ACE_TEXT_CHAR_TO_TCHAR(key.c_str()), value) < 0) {
            throw INMAGE_EX("couldn't read key value")(section)(key);
        }

        return std::string(ACE_TEXT_ALWAYS_CHAR(value.c_str()));
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getConfigParamOnCsPrimeApplianceToAzureAgent(section, key);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        throw INMAGE_EX("invalid mode");
    }
}

std::string FileConfigurator::getSMRInfoFromSMRConf(std::string& section, std::string& key)
{
    return get(section, key);
}

void FileConfigurator::getSection(std::string const& section, std::map<std::string, std::string> &namevaluepairs) const
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        ACE_Configuration_Section_Key sectionKey;
        if (m_inifile.open_section(m_inifile.root_section(), ACE_TEXT_CHAR_TO_TCHAR(section.c_str()), false, sectionKey) < 0) {
            throw INMAGE_EX("couldn't open section")(section);
        }

        int index = 0;
        int rval = -1;
        while (true)
        {
            ACE_TString name;
            ACE_Configuration::VALUETYPE type;
            rval = m_inifile.enumerate_values(sectionKey, index, name, type);
            if (0 == rval)
            {
                std::string sname(ACE_TEXT_ALWAYS_CHAR(name.c_str()));
                ACE_TString value;
                if (m_inifile.get_string_value(sectionKey, ACE_TEXT_CHAR_TO_TCHAR(sname.c_str()), value) < 0)
                {
                    value.clear();
                }
                namevaluepairs.insert(std::make_pair(sname, std::string(ACE_TEXT_ALWAYS_CHAR(value.c_str()))));
                index++;
            }
            else if (1 == rval)
            {
                break;
            }
            else if (-1 == rval)
            {
                throw INMAGE_EX("couldn't read section name value pairs")(section);
            }
            else
            {
                throw INMAGE_EX("couldn't read section name value pairs")(section);
            }
        }
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        getConfigParamOnCsPrimeApplianceToAzureAgent(section, namevaluepairs);
    }
}

/*
 * FUNCTION NAME : FileConfigurator::getVolumesCachePathname
 *
 * DESCRIPTION : provides the path of local volume summaries store.
 *               This is updated normally by service for s2, dp
 *               to use
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : volumesCachePath
 *
 * NOTES :
 *
 *
 * return value : true on success, otw false
 *
 */

bool FileConfigurator::getVolumesCachePathname(std::string & volumesCachePath)
{
    bool bReturn = false;
    std::string configdir;

    if (getConfigDirname(configdir))
    {
        volumesCachePath = configdir + ACE_DIRECTORY_SEPARATOR_CHAR_A + CachedVolumesFile;
        bReturn = true;
    }

    return bReturn;
}


/*
 * FUNCTION NAME : FileConfigurator::getConfigCachePathname
 *
 * DESCRIPTION : provides the path of local persistent store.
 *               Initial settings are persistent in the path provided
 *               to be later used by cdpcli operations
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : cachePath
 *
 * NOTES :
 *
 *
 * return value : true on success, otw false
 *
 */

bool FileConfigurator::getConfigCachePathname(std::string & cachePath)
{
    bool bReturn = false;
    std::string configdir;

    if (getConfigDirname(configdir))
    {
        cachePath = configdir + ACE_DIRECTORY_SEPARATOR_CHAR_A + CachedConfigFile;
        bReturn = true;
    }

    return bReturn;
}

/*
 * FUNCTION NAME : FileConfigurator::getConfigDirname
 *
 * DESCRIPTION : provides the directory name of local persistent store.
 *               used as helper routine by getConfigCachePathname,
 *               getVolumesCachePathname
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : configdir
 *
 * NOTES :
 *
 *
 * return value : true on success, otw false
 *
 */

bool FileConfigurator::getConfigDirname(std::string & configdir)
{
    bool bReturn = false;
    char resolvedPath[PATH_MAX];

    if (NULL != ACE_OS::realpath(getConfigPathname().c_str(), resolvedPath))
    {
        configdir = dirname_r(resolvedPath);
        bReturn = true;
    }

    return bReturn;
}

bool FileConfigurator::getVxProtectedDeviceDetailCachePathname(std::string & cachePath)
{
    std::string configdir;

#ifdef SV_WINDOWS
    if (!getConfigDirname(configdir))
        return false;
#else
    configdir = DEFAULT_UNIX_DRIVER_PERSISTENT_STORE_PATH;
#endif

    cachePath = configdir + ACE_DIRECTORY_SEPARATOR_CHAR_A + VxProtectedDeviceDetailPeristentFile;
    return true;
}

bool FileConfigurator::getDeprecatedVxProtectedDeviceDetailCachePathname(std::string & cachePath)
{
    std::string configdir;

    if (!getConfigDirname(configdir))
        return false;

    cachePath = configdir + ACE_DIRECTORY_SEPARATOR_CHAR_A + VxProtectedDeviceDetailPeristentFile;
    return true;
}

bool FileConfigurator::getVxPlatformTypeForDriverPersistentFile(std::string &filePath)
{
    std::string configdir;

#ifdef SV_WINDOWS
    if (!getConfigDirname(configdir))
        return false;
#else
    configdir = DEFAULT_UNIX_DRIVER_PERSISTENT_STORE_PATH;
#endif

    filePath = configdir + ACE_DIRECTORY_SEPARATOR_CHAR_A + VxPlatformTypeForDriverPersistentFile;
    return true;
}

bool FileConfigurator::getNameValuesInSection(const std::string &section, std::map<std::string, std::string> &namevaluepairs) const
{
    bool bgot = true;

    try {
        getSection(section, namevaluepairs);
    }
    catch (ContextualException&) {
        namevaluepairs.clear();
        bgot = false;
    }
    return bgot;
}

std::string FileConfigurator::get(std::string const& section, std::string const& key, std::string const& defaultValue) const
{
    string result;
    try {
        result = get(section, key);
    }
    catch (ContextualException&) {
        result = defaultValue;
    }
    return result;
}

std::string FileConfigurator::get(std::string const& section, std::string const& key, int defaultValue) const
{
    string result;
    try {
        result = get(section, key);
    }
    catch (ContextualException&) {
        result = boost::lexical_cast<std::string>(defaultValue);
    }
    return result;
}

bool FileConfigurator::CopyDrconfFile(std::string const & SourceFile, std::string const & DestinationFile)const
{
    bool rv = true;
    const std::size_t buf_sz = 4096;
    try
    {
        char buf[buf_sz];
        //ACE_HANDLE infile=0, outfile=0;
        ACE_HANDLE infile = ACE_INVALID_HANDLE, outfile = ACE_INVALID_HANDLE;
        ACE_stat from_stat = { 0 };
        if (ACE_OS::lstat((const char*)SourceFile.c_str(), &from_stat) == 0
            && (infile = ACE_OS::open(SourceFile.c_str(), O_RDONLY)) != ACE_INVALID_HANDLE
            && (outfile = ACE_OS::open(DestinationFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC)) != ACE_INVALID_HANDLE)
        {
            ssize_t sz, sz_read = 1, sz_write;
            while (sz_read > 0
                && (sz_read = ACE_OS::read(infile, buf, buf_sz)) > 0)
            {
                sz_write = 0;
                do
                {
                    if ((sz = ACE_OS::write(outfile, buf + sz_write, sz_read - sz_write)) < 0)
                    {
                        sz_read = sz; // cause read loop termination
                        break;        //  and error to be thrown after closes
                    }
                    sz_write += sz;
                } while (sz_write < sz_read);
            }
            if (ACE_OS::close(infile) < 0)
                sz_read = -1;
            if (ACE_OS::close(outfile) < 0)
                sz_read = -1;
            if (sz_read < 0)
            {
                rv = false;
            }
        }
        else
        {
            rv = false;
            if (outfile != ACE_INVALID_HANDLE)
                ACE_OS::close(outfile);
            if (infile != ACE_INVALID_HANDLE)
                ACE_OS::close(infile);
        }
    }
    catch (...)
    {
        rv = false;
    }
    return rv;
}

std::string FileConfigurator::dirname_r(const std::string &name, const char &separator_char)
{
    std::string::size_type scp = name.rfind(separator_char);
    return (std::string::npos == scp) ? std::string(".") : name.substr(0, scp);
}

bool FileConfigurator::RenameDrconf(std::string const & drscoutcopyFile, std::string const & drscoutFile)const
{
    bool rv = true;
    int flags = -1;
#ifdef SV_WINDOWS
    flags = MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
#endif

    int retryCount = 0;
    SV_UINT totalRetryCount = getRenameFailureRetryCount();
    SV_UINT renameFailureRetryIntervalInSec = getRenameFailureRetryIntervalInSec();

    do {
        if (ACE_OS::rename(drscoutcopyFile.c_str(), drscoutFile.c_str()) < 0)
        {
            DebugPrintf(SV_LOG_ERROR, "%s : Rename of file %s to file %s failed with error : %d\n",
                FUNCTION_NAME, drscoutcopyFile.c_str(), drscoutFile.c_str(), ACE_OS::last_error());
            rv = false;
        }
        else {
            rv = true;
            break;
        }
        retryCount++;
        ACE_OS::sleep(renameFailureRetryIntervalInSec * retryCount);
    } while (retryCount < totalRetryCount && !rv);
    
    if (!rv)
    {
        ACE_OS::unlink(drscoutcopyFile.c_str());
    }
    return rv;
}


void FileConfigurator::set(
    std::string const& section, std::string const& key, std::string const& value) const
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);
    //copy drscout.conf to drscout.conf.copy
    std::string destinationFile = dirname_r(getConfigPathname().c_str());
    destinationFile += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    destinationFile += "drscout.conf.copy";
    if (!CopyDrconfFile(getConfigPathname().c_str(), destinationFile.c_str())){
        throw INMAGE_EX("couldn't copy file")(getConfigPathname());
    }
    //import drscout.conf.copy
    ACE_Ini_ImpExp importexport(m_inifile);
    // NOTE: this does not throw or return failure if file does not exist
    if (importexport.import_config(ACE_TEXT_CHAR_TO_TCHAR(destinationFile.c_str())) < 0){
        throw INMAGE_EX("couldn't import file")(destinationFile);
    }

    ACE_Configuration_Section_Key sectionKey;
    if (m_inifile.open_section(m_inifile.root_section(), ACE_TEXT_CHAR_TO_TCHAR(section.c_str()), true, sectionKey) < 0){
        throw INMAGE_EX("couldn't open section")(section);
    }
    if (m_inifile.set_string_value(sectionKey, ACE_TEXT_CHAR_TO_TCHAR(key.c_str()), ACE_TEXT_CHAR_TO_TCHAR(value.c_str())) < 0) {
        throw INMAGE_EX("couldn't write key string value")(section)(key);
    }
    //lock.UnlockFile();
    //export drscout.conf.copy
    if (importexport.export_config(ACE_TEXT_CHAR_TO_TCHAR(destinationFile.c_str())) < 0){
        throw INMAGE_EX("couldn't export file")(destinationFile);
    }
    //rename drscout.conf.copy to drscout.conf
    if (!RenameDrconf(destinationFile, getConfigPathname())){
        throw INMAGE_EX("couldn't rename file")(destinationFile);
    }
}

void FileConfigurator::remove(std::string const& section, std::string const& key) const
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    AutoGuardFileLock lock(m_lockInifile);
    //copy drscout.conf to drscout.conf.copy
    std::string destinationFile = dirname_r(getConfigPathname().c_str());
    destinationFile += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    destinationFile += "drscout.conf.copy";
    if (!CopyDrconfFile(getConfigPathname().c_str(), destinationFile.c_str())){
        throw INMAGE_EX("couldn't copy file")(getConfigPathname());
    }
    ACE_Ini_ImpExp importexport(m_inifile);
    // NOTE: this does not throw or return failure if file does not exist
    if (importexport.import_config(ACE_TEXT_CHAR_TO_TCHAR(destinationFile.c_str())) < 0){
        throw INMAGE_EX("couldn't import file")(destinationFile);
    }

    ACE_Configuration_Section_Key sectionKey;
    if (m_inifile.open_section(m_inifile.root_section(), ACE_TEXT_CHAR_TO_TCHAR(section.c_str()), true, sectionKey) < 0){
        throw INMAGE_EX("couldn't open section")(section);
    }
    if (m_inifile.remove_value(sectionKey, ACE_TEXT_CHAR_TO_TCHAR(key.c_str())) < 0) {
        throw INMAGE_EX("couldn't remove key value")(section)(key);
    }
    //lock.UnlockFile();
    if (importexport.export_config(ACE_TEXT_CHAR_TO_TCHAR(destinationFile.c_str())) < 0){
        throw INMAGE_EX("couldn't export file")(destinationFile);
    }
    //rename drscout.conf.copy to drscout.conf
    if (!RenameDrconf(destinationFile, getConfigPathname())){
        throw INMAGE_EX("couldn't rename file")(destinationFile);
    }
}

std::string FileConfigurator::getMTSupportedDataPlanes() const {
    return get(SECTION_VXAGENT, KEY_MT_SUPPORTED_DATAPLANES, DEFAULT_MT_SUPPORTED_DATAPLANES);
}

SV_ULONGLONG FileConfigurator::getMinAzureUploadSize() const {
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MIN_AZURE_UPLOAD_SIZE,
        DEFAULT_MIN_AZURE_UPLOAD_SIZE));
}

unsigned int FileConfigurator::getMinTimeGapBetweenAzureUploads() const {
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_MIN_TIMEGAP_BETWEEN_AZURE_UPLOADS,
        DEFAULT_MIN_TIMEGAP_AZURE_UPLOADS));
}

unsigned int FileConfigurator::getTimeGapBetweenFileArrivalCheck() const {
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_TIMEGAP_BETWEEN_FILEARRIVAL_CHECK,
        DEFAULT_TIMEGAP_FILEARRIVAL_CHECK));
}

unsigned int FileConfigurator::getMaxAzureAttempts() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_MAX_AZURE_ATTEMPTS,
        DEFAULT_MAX_AZURE_ATTEMPTS));
}

unsigned int FileConfigurator::getAzureRetryDelayInSecs() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT,
        KEY_AZURE_RETRY_DELAY_IN_SECS, DEFAULT_AZURE_RETRY_DELAY_IN_SECS));
}

unsigned int FileConfigurator::getAzureImplType() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT,
        KEY_AZURE_IMPL_TYPE, DEFAULT_AZURE_IMPL_TYPE));
}

std::string FileConfigurator::getCacheDirectory() const {
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_CACHE_DIRECTORY);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getInstallPath() + ACE_DIRECTORY_SEPARATOR_CHAR_A + Application_Data_Folder;
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

std::string FileConfigurator::getHostId() const {
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_HOST_ID);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return std::string();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return getEvtCollForwParam(KEY_HOST_ID);
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

std::string FileConfigurator::getResourceId() const {
    return get(SECTION_VXAGENT, KEY_RESOURCE_ID, std::string());
}

std::string FileConfigurator::getSourceGroupId() const {
    return get(SECTION_VXAGENT, KEY_SOURCE_GROUP_ID, std::string());
}
/*
* FUNCTION NAME :  FileConfigurator::getUnregisterAgentLogPath()
*
* DESCRIPTION : Provides the path of unregister log file in agent
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : logName
*
*/
std::string FileConfigurator::getUnregisterAgentLogPath() const {
    std::string logName;

#ifdef SV_WINDOWS
    logName = DEFAULT_WIN_UNREGISTER_LOG_FILE_PATH;
#else
    logName = getLogPathname();
#endif

    logName += DEFAULT_UNREGISTER_LOG_FILE;
    return logName;
}

// EvtCollForw settings - Start

SV_LOG_LEVEL FileConfigurator::getEvtCollForwAgentLogPostLevel() const {
    return static_cast<SV_LOG_LEVEL>(boost::lexical_cast<int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_AGENT_LOG_POST_LEVEL, DEFAULT_EVTCOLLFORW_AGENT_LOG_POST_LEVEL)));
}

std::vector<std::string> FileConfigurator::getSourceAgentLogsToUpload() const {
    std::vector<std::string> toRet;
    std::string sourceAgentLogsToUploadCsv = get(
        SECTION_VXAGENT, KEY_SOURCE_AGENT_LOGS_TO_UPLOAD, DEFAULT_SOURCE_AGENT_LOGS_TO_UPLOAD);
    boost::split(toRet, sourceAgentLogsToUploadCsv, boost::algorithm::is_any_of(","), boost::token_compress_on);

    return toRet;
}

int FileConfigurator::getEvtCollForwPollIntervalInSecs() const {
    return boost::lexical_cast<int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_POLL_INTERVAL_IN_SECS, DEFAULT_EVTCOLLFORW_POLL_INTERVAL_IN_SECS));
}

int FileConfigurator::getEvtCollForwPenaltyTimeInSecs() const {
    return boost::lexical_cast<int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_PENALTY_TIME_IN_SECS, DEFAULT_EVTCOLLFORW_PENALTY_TIME_IN_SECS));
}

int FileConfigurator::getEvtCollForwMaxStrikes() const {
    return boost::lexical_cast<int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_MAX_STRIKES, DEFAULT_EVTCOLLFORW_MAX_STRIKES));
}

unsigned int FileConfigurator::getEvtCollForwProcSpawnInterval() const {
    return boost::lexical_cast<unsigned int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_PROC_SPAWN_INTERVAL, DEFAULT_EVTCOLLFORW_PROC_SPAWN_INTERVAL));
}

unsigned int FileConfigurator::getEvtCollForwMaxMarsUploadFilesCnt() const {
    return boost::lexical_cast<unsigned int>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_MAX_MARS_UPLOAD_FILES_CNT, DEFAULT_EVTCOLLFORW_MAX_MARS_UPLOAD_FILES_CNT));
}

uint32_t FileConfigurator::getEvtCollForwCxTransportMaxAttempts() const {
    return boost::lexical_cast<uint32_t>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_CX_TRANSPORT_MAX_ATTEMPTS, DEFAULT_EVTCOLLFORW_CX_TRANSPORT_MAX_ATTEMPTS));
}

std::string FileConfigurator::getEvtCollForwIRCompletedFilesMoveDir() const {
    return get(SECTION_VXAGENT, KEY_EVTCOLLFORW_IR_COMPLETED_FILES_MOVE_DIR, std::string("completed"));
}

bool FileConfigurator::isEvtCollForwEventLogUploadEnabled() const {
    return boost::lexical_cast<bool>(
        get(SECTION_VXAGENT, KEY_EVTCOLLFORW_UPLOAD_EVENT_LOG, DEFAULT_EVTCOLLFORW_UPLOAD_EVENT_LOG));
}

// EvtCollForw settings - End

unsigned int FileConfigurator::getCsJobProcessorProcSpawnInterval() const {
    return boost::lexical_cast<unsigned int>(
        get(SECTION_VXAGENT, KEY_CSJOBPROCESSOR_PROC_SPAWN_INTERVAL, DEFAULT_CSJOBPROCESSOR_PROC_SPAWN_INTERVAL));
}

std::string FileConfigurator::getRcmSettingsPath() const {
    std::string rcmSettingsPath = get(SECTION_VXAGENT, KEY_RCM_SETTINGS_PATH, std::string());
    if (rcmSettingsPath.empty())
        rcmSettingsPath = securitylib::securityTopDir() + RCM_SETTINGS_FILE_SUFFIX;
    return rcmSettingsPath;
}

void FileConfigurator::setRcmSettingsPath(const std::string& rcmSettingsPath) const {
    set(SECTION_VXAGENT, KEY_RCM_SETTINGS_PATH, rcmSettingsPath);
}

unsigned int FileConfigurator::getRcmRequestTimeout() const {
    return boost::lexical_cast<unsigned int>(
        get(SECTION_VXAGENT, KEY_RCM_REQUEST_TIMEOUT, DEFAULT_RCM_REQUEST_TIMEOUT));
}


std::string FileConfigurator::getProxySettingsPath() const {
    return get(SECTION_VXAGENT, KEY_PROXY_SETTINGS_PATH, std::string());
}

std::string FileConfigurator::getVmPlatform() const {
    return get(SECTION_VXAGENT, KEY_VM_PLATFORM, std::string());
}

bool FileConfigurator::IsAzureToAzureReplication() const {
    std::string vmPlatform = get(SECTION_VXAGENT, KEY_VM_PLATFORM, std::string());
    return boost::iequals(vmPlatform, "Azure");
}
int FileConfigurator::getMaxDifferentialPayload() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_DIFF_SIZE));
}

SV_UINT FileConfigurator::getFastSyncReadBufferSize() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_FASTSYNC_READ_BUFFER_SIZE, DEFAULT_FASTSYNC_READ_BUFFER_SIZE));
}

SV_HOST_AGENT_TYPE FileConfigurator::getAgentType() const {
    throw INMAGE_EX("not impl");
}

std::string FileConfigurator::getLogPathname() const
{
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_LOG_PATHNAME);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getTelemetryFolderPathOnCsPrimeApplianceToAzure();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return getPSTelemetryFolderPathOnCsPrimeApplianceToAzure();
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

std::string FileConfigurator::getLogDir() const
{
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_DEST_DIR);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getTelemetryFolderPathOnCsPrimeApplianceToAzure();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return getPSInstallPathOnCsPrimeApplianceToAzure();
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

SV_LOG_LEVEL FileConfigurator::getLogLevel() const {
    return static_cast<SV_LOG_LEVEL>(boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_LOG_LEVEL, DEFAULT_LOG_LEVEL)));
}

SV_UINT FileConfigurator::getLogMaxCompletedFiles() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT,
        KEY_LOG_MAX_NUM_COMPLETED_FILES,
        SV_DEFAULT_LOG_MAX_NUM_COMPLETED_FILES));
}

SV_UINT FileConfigurator::getLogCutInterval() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT,
        KEY_LOG_CUT_INTERVAL,
        SV_DEFAULT_LOG_CUT_INTERVAL));
}
SV_UINT FileConfigurator::getLogMaxFileSize() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT,
        KEY_LOG_MAX_FILE_SIZE,
        SV_DEFAULT_LOG_MAX_FILE_SIZE));
}
SV_UINT FileConfigurator::getLogMaxFileSizeForTelemetry() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT,
        KEY_LOG_MAX_FILE_SIZE_TELEMETRY,
        SV_DEFAULT_LOG_MAX_FILE_SIZE_TELEMETRY));
}

void FileConfigurator::setLogLevel(SV_LOG_LEVEL logLevel) const {
    set(SECTION_VXAGENT, KEY_LOG_LEVEL, boost::lexical_cast<std::string>(logLevel));
}

std::string FileConfigurator::getDiffSourceExePathname() const {
    return get(SECTION_VXAGENT, KEY_DIFF_SOURCE_EXE_PATHNAME);
}

void FileConfigurator::setDiffSourceExePathname(const std::string& diffSourceExePathname) const {
    set(SECTION_VXAGENT, KEY_DIFF_SOURCE_EXE_PATHNAME, diffSourceExePathname);
}

std::string FileConfigurator::getDiffSourceDirectory() const {
    return get(SECTION_VXAGENT, KEY_DIFF_SOURCE_DIRECTORY);
}

std::string FileConfigurator::getDiffTargetDirectory() const {
    return get(SECTION_VXAGENT, KEY_DIFF_TARGET_DIRECTORY);
}

std::string FileConfigurator::getPrefixDiffFilename() const  {
    return get(SECTION_VXAGENT, KEY_PREFIX_DIFF_FILENAME);
}

std::string FileConfigurator::getResyncDirectories() const {
    // todo: don't think this belongs to local configurator
    return get(SECTION_VXAGENT, KEY_RESYNC_DIRECTORIES);
}

std::string FileConfigurator::getSslClientFile() const {
    return get(SECTION_VXTRANSPORT, KEY_SSL_CLIENT_FILE);
}

std::string FileConfigurator::getSslKeyPath() const {
    return get(SECTION_VXTRANSPORT, KEY_SSL_KEY_PATHNAME);
}

std::string FileConfigurator::getSslCertificatePath() const {
    return get(SECTION_VXTRANSPORT, KEY_SSL_CERTIFICATE_PATHNAME);
}

std::string FileConfigurator::getCfsLocalName() const {
    return get(SECTION_VXTRANSPORT, KEY_CFS_LOCAL_NAME, CFS_LOCAL_NAME_DEFAULT);
}

bool FileConfigurator::getAccountInfo(std::map<std::string, std::string> &namevaluepairs) const
{
    return getNameValuesInSection(SECTION_VXACCOUNT, namevaluepairs);
}

int FileConfigurator::getTcpSendWindowSize() const {
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TCP_SENDWINDOW_SIZE, DEFAULT_TCP_SENDWINDOW_SIZE));
}

int FileConfigurator::getTcpRecvWindowSize() const {
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TCP_RECVWINDOW_SIZE, DEFAULT_TCP_RECVWINDOW_SIZE));
}

HTTP_CONNECTION_SETTINGS FileConfigurator::getHttp() const {
    HTTP_CONNECTION_SETTINGS s;
    std::string ipaddr = get(SECTION_VXTRANSPORT, KEY_HTTP_IPADDRESS, "").c_str();
    inm_strcpy_s(s.ipAddress, ARRAYSIZE(s.ipAddress), ipaddr.c_str());
    s.port = boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_HTTP_PORT, "-1").c_str());
    return s;
}

void FileConfigurator::setCsAddressForAzureComponents(std::string const& address) const {
    set(SECTION_VXTRANSPORT, KEY_CS_ADDRESS_FOR_AZURE_COMPONENTS, address);
}

std::string FileConfigurator::getCsAddressForAzureComponents() const {
    return get(SECTION_VXTRANSPORT, KEY_CS_ADDRESS_FOR_AZURE_COMPONENTS, std::string());
}

bool FileConfigurator::IsHttps() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXTRANSPORT, KEY_HTTPS, "1"));
}


std::string FileConfigurator::getDataProtectionExePathname() const {
    return get(SECTION_VXAGENT, KEY_DATA_PROTECTION_EXE_PATHNAME);
}

void FileConfigurator::setDataProtectionExePathname(const std::string& dataProtectionExePathname) const {
    set(SECTION_VXAGENT, KEY_DATA_PROTECTION_EXE_PATHNAME, dataProtectionExePathname);
}

/* Added by BSR
 *  Project: New Sync
 * Start */

std::string FileConfigurator::getDataProtectionExeV2Pathname() const
{
    return get(SECTION_VXAGENT, KEY_DATA_PROTECTION_EXEV2_PATHNAME);
}

/*End*/
/** Added by BSR for upgrade issue **/
bool FileConfigurator::getUpdatedUpgradeToCX() const
{
    return boost::lexical_cast<bool>(get(SECTION_UPGRADE, KEY_UPDATED_UPGRADE_TO_CX, "1"));
}

void FileConfigurator::setUpdatedUpgradeToCX(const bool bUpdated) const
{
    set(SECTION_UPGRADE, KEY_UPDATED_UPGRADE_TO_CX, boost::lexical_cast<std::string>(bUpdated));
}

std::string FileConfigurator::getUpgradedVersion() const
{
    return get(SECTION_UPGRADE, KEY_UPGRADED_VERSION);
}

std::string FileConfigurator::getUpgradePHPPath() const
{
    return get(SECTION_UPGRADE, KEY_UPGRADE_PHP);
}

SV_ULONG FileConfigurator::getUpdateUpgradeWaitTimeSecs() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_UPGRADE, KEY_UPDATE_UPGRADE_WAITTIME, DEFAULT_UPGRADE_WAIT_TIME));
}
/** End of the change **/

SV_ULONG FileConfigurator::getResyncStaleFilesCleanupInterval() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_RESYNC_STALE_FILE_CLEANUP_INTERVAL, KEY_DEFAULT_RESYNC_STALE_FILE_CLEANUP_INTERVAL));
}

bool FileConfigurator::ShouldCleanupCorruptSyncFile() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SHOULD_CLEANUP_CORRUPT_SYNC_FILE, "1"));
}

SV_ULONG FileConfigurator::getResyncUpdateInterval() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_RESYNC_UPDATE_INTERVAL, DEFAULT_RESYNC_UPDATE_INTERVAL));
}

SV_ULONG FileConfigurator::getIRMetricsReportInterval() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_IR_METRICS_REPORT_INTERVAL, "7200"));
}

SV_ULONG FileConfigurator::getLogResyncProgressInterval() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_LOG_RESYNC_PROGRESS_INTERVAL, "900"));
}

SV_ULONG FileConfigurator::getResyncSlowProgressThreshold() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_RESYNC_SLOW_PROGRESS_THRESHOLD, "3600"));
}

SV_ULONG FileConfigurator::getResyncNoProgressThreshold() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_RESYNC_NO_PROGRESS_THRESHOLD, "7200"));
}

std::string FileConfigurator::getOffloadSyncPathname() const  {
    return get(SECTION_VXAGENT, KEY_OFFLOAD_SYNC_PATHNAME);
}

std::string FileConfigurator::getVsnapConfigPathname() const {
    std::string pathName = getLogPathname() + "vsnap";
    return get(SECTION_VXAGENT, KEY_VSNAP_CONFIG_PATHNAME, pathName);
}


std::string FileConfigurator::getVsnapPendingPathname() const {
    std::string pathName = getLogPathname() + "pendingvsnap";
    return get(SECTION_VXAGENT, KEY_VSNAP_PENDING_PATHNAME, pathName);
}


std::string FileConfigurator::getOffloadSyncSourceDirectory() const {
    return get(SECTION_VXAGENT, KEY_OFFLOAD_SYNC_SOURCE_DIRECTORY);
}


std::string FileConfigurator::getOffloadSyncCacheDirectory() const {
    return get(SECTION_VXAGENT, KEY_OFFLOAD_SYNC_CACHE_DIRECTORY);
}


std::string FileConfigurator::getOffloadSyncFilenamePrefix() const {
    return get(SECTION_VXAGENT, KEY_OFFLOAD_SYNC_FILENAME_PREFIX);
}

std::string FileConfigurator::getDiffTargetExePathname() const {
    return get(SECTION_VXAGENT, KEY_DIFF_TARGET_EXE_PATHNAME);
}

std::string FileConfigurator::getDiffTargetSourceDirectoryPrefix() const {
    return get(SECTION_VXAGENT, KEY_DIFF_TARGET_SOURCE_DIRECTORY_PREFIX);
}
std::string FileConfigurator::getDiffTargetCacheDirectoryPrefix() const {
    return get(SECTION_VXAGENT, KEY_DIFF_TARGET_CACHE_DIRECTORY_PREFIX);
}

void FileConfigurator::setDiffTargetCacheDirectoryPrefix(std::string const& value) const {
    set(SECTION_VXAGENT, KEY_DIFF_TARGET_CACHE_DIRECTORY_PREFIX, value);
}

std::string FileConfigurator::getDiffTargetFilenamePrefix() const {
    return get(SECTION_VXAGENT, KEY_DIFF_TARGET_FILENAME_PREFIX);
}

std::string FileConfigurator::getFastSyncExePathname() const {
    return get(SECTION_VXAGENT, KEY_FAST_SYNC_EXE_PATHNAME);
}

int FileConfigurator::getFastSyncBlockSize() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_FAST_SYNC_BLOCK_SIZE));
}

int FileConfigurator::getFastSyncMaxChunkSize() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_FAST_SYNC_MAX_CHUNK_SIZE, DEFAULT_MAX_FASTSYNC_CHUNK_SIZE));
}

int FileConfigurator::getFastSyncMaxChunkSizeForE2A() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_FAST_SYNC_MAX_CHUNK_SIZE_FOR_E2A, DEFAULT_MAX_FASTSYNC_CHUNK_SIZE_E2A));
}


bool FileConfigurator::getDICheck() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DI_CHECK, DEFAULT_DI_CHECK));
}

bool FileConfigurator::getSVDCheck() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SVD_CHECK, DEFAULT_SVD_CHECK));
}

bool FileConfigurator::getDirectTransfer() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DIRECT_TRANSFER, DEFAULT_DIRECT_TRANSFER));
}

bool FileConfigurator::getEnableDiffFileChecksums() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ENABLE_DIFF_FILE_CHECKSUMS, DEFAULT_ENABLE_DIFF_FILE_CHECKSUMS));
}

bool FileConfigurator::CompareInInitialDirectSync() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_COMPARE_IN_INITIAL_DIRECTSYNC, false));
}

bool FileConfigurator::IsProcessClusterPipeEnabled() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ENABLE_PROCESS_CLUSTER_PIPE, "1"));
}

bool FileConfigurator::allowRootVolumeForRetention() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ALLOW_ROOTVOLUME_FOR_RETENTION_NAME, 1));
}

SV_UINT FileConfigurator::getMaxHcdsAllowdAtCx() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAX_HCDS, DEFAULT_MAX_HCDS));
}

SV_UINT FileConfigurator::getMaxClusterBitmapsAllowdAtCx() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAX_CLUSTER_BITMAPS, DEFAULT_MAX_CLUSTER_BITMAPS));
}

SV_UINT FileConfigurator::getSecsToWaitForHcdSend() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_SECS_TO_WAITFORHCD, DEFAULT_SECS_TO_WAITFORHCD));
}

SV_UINT FileConfigurator::getSecsToWaitForClusterBitmapSend() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_SECS_TO_WAITFORCLUSTERBITMAP, DEFAULT_SECS_TO_WAITFORCLUSTERBITMAP));
}

bool FileConfigurator::getTSCheck() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_TS_CHECK, DEFAULT_TS_CHECK));
}

SV_ULONG FileConfigurator::getDirectSyncBlockSizeInKB() const {
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_DIRECTSYNC_BLOCKSIZE_INKB, DEFAULT_DIRECTSYNC_BLOCKSIZE_INKB));
}

bool FileConfigurator::getDIVerify() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DI_VERIFY, DEFAULT_DI_VERIFY));
}

bool FileConfigurator::isRetainBookmarkForVsnap() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VSNAPLOCKBOOKMARK, DEFAULT_VSNAPLOCKBOOKMARK));
}

SV_ULONGLONG FileConfigurator::getSparseFileMaxSize() const {
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_SPARSE_SIZE, DEFAULT_MAX_SPARSE_SIZE));
}

bool FileConfigurator::DPPrintPerfCounters() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPMEASURE_PERF, DEFAULT_DPMEASURE_PERF));
}

bool FileConfigurator::DPProfileSourceIo() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPPROFILE_SRCIO, DEFAULT_DPPROFILE_SRCIO));
}

std::string FileConfigurator::getApplicationConsistentExcludedVolumes() const
{
    return get(SECTION_VXAGENT, KEY_APPCONSISTENT_EXCLUDED_VOLUMES, "");
}

bool FileConfigurator::DPProfileVolRead() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPPROFILE_VOLREAD, DEFAULT_DPPROFILE_VOLREAD));
}

bool FileConfigurator::DPProfileVolWrite() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPPROFILE_VOLWRITE, DEFAULT_DPPROFILE_VOLWRITE));
}

bool FileConfigurator::DPProfileCdpWrite() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPPROFILE_CDPWRITE, DEFAULT_DPPROFILE_CDPWRITE));
}

bool FileConfigurator::IsCdpcliSkipCheck() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDPCLI_SKIPCHECK_NAME, 0));
}

std::string FileConfigurator::getTargetChecksumsDir() const {
    return get(SECTION_VXAGENT, KEY_TARGET_CHECKSUMS_DIR, getLogPathname());
}

int FileConfigurator::getUncompressRetries() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_UNCOMPRESS_RETRIES, DEFAULT_UNCOMPRESS_RETRIES));
}

int FileConfigurator::getUncompressRetryInterval() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_UNCOMPRESS_RETRY_INTERVAL, DEFAULT_UNCOMPRESS_RETRY_INTERVAL));
}

SV_ULONGLONG FileConfigurator::getMinDiskFreeSpaceForUncompression() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MIN_DISK_FREESPACE_FOR_UNCOMPRESSION,
        DEFAULT_MIN_DISK_FREESPACE_FOR_UNCOMPRESSION));
}

bool FileConfigurator::shouldIgnoreCorruptedDiffs() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_IGNORE_CORRUPTED_DIFFS, DEFAULT_IGNORE_CORRUPTED_DIFFS));
}

bool FileConfigurator::getUseConfiguredHostname() const {
    // TODO: handle `true' and `false' strings
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_USE_CONFIGURED_HOSTNAME));
}

void FileConfigurator::setUseConfiguredHostname(bool flag) const {
    set(SECTION_VXAGENT, KEY_USE_CONFIGURED_HOSTNAME, boost::lexical_cast<std::string>(flag));
}

bool FileConfigurator::getUseConfiguredIpAddress() const{
    // TODO: handle `true' and `false' strings
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_USE_CONFIGURED_IP_ADDRESS));
}

void FileConfigurator::setUseConfiguredIpAddress(bool flag) const{
    set(SECTION_VXAGENT, KEY_USE_CONFIGURED_IP_ADDRESS, boost::lexical_cast<std::string>(flag));
}

std::string FileConfigurator::getConfiguredHostname() const{
    return get(SECTION_VXAGENT, KEY_CONFIGURED_HOSTNAME);
}

void FileConfigurator::setConfiguredHostname(const std::string &hostName) const{
    set(SECTION_VXAGENT, KEY_CONFIGURED_HOSTNAME, hostName);
}

std::string FileConfigurator::getConfiguredIpAddress() const{
    return get(SECTION_VXAGENT, KEY_CONFIGURED_IP_ADDRESS);
}

void FileConfigurator::setConfiguredIpAddress(const std::string &ipAddress) const{
    set(SECTION_VXAGENT, KEY_CONFIGURED_IP_ADDRESS, ipAddress);
}

std::string FileConfigurator::getExternalIpAddress() const{
    return get(SECTION_VXAGENT, KEY_EXTERNAL_IP_ADDRESS, "");
}

int FileConfigurator::getFastSyncHashCompareDataSize() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_FAST_SYNC_HASH_COMPARE_DATA_SIZE, DEFAULT_FAST_SYNC_HASH_COMPARE_DATA_SIZE));
}

std::string FileConfigurator::getResyncSourceDirectoryPath() const {
    return get(SECTION_VXAGENT, KEY_RESYNC_SOURCE_DIRECTORY_PATH);
}


bool FileConfigurator::getChunkMode() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CHUNK_MODE));
}


unsigned int FileConfigurator::getMaxFastSyncApplyThreads() const
{
    SV_UINT maxFastsyncApplyThreads =
        boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_FASTSYNC_APPLY_THREADS, DEFAULT_MIN_FASTSYNC_APPLY_THREADS));

    if (maxFastsyncApplyThreads == 0)
    {
        maxFastsyncApplyThreads = DEFAULT_MIN_FASTSYNC_APPLY_THREADS;
    }

    //  protect against some upper thread number
    if (maxFastsyncApplyThreads > DEFAULT_MAX_FASTSYNC_APPLY_THREADS)
    {
        maxFastsyncApplyThreads = DEFAULT_MAX_FASTSYNC_APPLY_THREADS;
    }

    return maxFastsyncApplyThreads;
}

SV_UINT FileConfigurator::getMaxFastSyncProcessThreads() const
{
    SV_UINT maxFastSyncProcessThreads = boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_FASTSYNC_PROCHCD_THREADS, "2"));

    maxFastSyncProcessThreads =
        (maxFastSyncProcessThreads == 0) ? DEFAULT_MIN_FASTSYNC_PROCHCD_THREADS : maxFastSyncProcessThreads;

    maxFastSyncProcessThreads =
        (maxFastSyncProcessThreads > DEFAULT_MAX_FASTSYNC_PROCHCD_THREADS) ? DEFAULT_MAX_FASTSYNC_PROCHCD_THREADS : maxFastSyncProcessThreads;

    return maxFastSyncProcessThreads;
}

SV_UINT FileConfigurator::getMaxClusterProcessThreads() const
{
    SV_UINT maxClusterProcessThreads = boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_CLUSTER_PROCESS_THREADS, "1"));

    maxClusterProcessThreads =
        (maxClusterProcessThreads == 0) ? DEFAULT_MIN_CLUSTER_PROCESS_THREADS : maxClusterProcessThreads;

    maxClusterProcessThreads =
        (maxClusterProcessThreads > DEFAULT_MAX_CLUSTER_PROCESS_THREADS) ? DEFAULT_MAX_CLUSTER_PROCESS_THREADS : maxClusterProcessThreads;

    return maxClusterProcessThreads;
}

SV_UINT FileConfigurator::getMaxFastSyncGenerateHCDThreads() const
{
    SV_UINT maxFastSyncGenHCDThreads = boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_FASTSYNC_GENHCD_THREADS, "2"));

    maxFastSyncGenHCDThreads =
        (maxFastSyncGenHCDThreads == 0) ? DEFAULT_MIN_FASTSYNC_GENHCD_THREADS : maxFastSyncGenHCDThreads;

    maxFastSyncGenHCDThreads =
        (maxFastSyncGenHCDThreads > DEFAULT_MAX_FASTSYNC_GENHCD_THREADS) ? DEFAULT_MAX_FASTSYNC_GENHCD_THREADS : maxFastSyncGenHCDThreads;

    return maxFastSyncGenHCDThreads;
}

SV_UINT FileConfigurator::getMaxGenerateClusterBitmapThreads() const
{
    SV_UINT maxGenClusterBitmapThreads = boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_GENCLUSTER_BITMAP_THREADS, "1"));

    maxGenClusterBitmapThreads =
        (maxGenClusterBitmapThreads == 0) ? DEFAULT_MIN_GENCLUSTER_BITMAP_THREADS : maxGenClusterBitmapThreads;

    maxGenClusterBitmapThreads =
        (maxGenClusterBitmapThreads > DEFAULT_MAX_GENCLUSTER_BITMAP_THREADS) ? DEFAULT_MAX_GENCLUSTER_BITMAP_THREADS : maxGenClusterBitmapThreads;

    return maxGenClusterBitmapThreads;
}

int FileConfigurator::getSyncBytesToApplyThreshold(std::string const& vol) const {
    /* TODO: add in per volume support */
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SYNC_BYTES_TO_APPLY_THRESHOLD));
}

bool FileConfigurator::getHostType() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, "CDPDirection"));
}

// Recent get additions

int FileConfigurator::getMaxOutpostThreads() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MAX_OUTPOST_THREADS, DEFAULT_MIN_OUTPOST_THREADS));
}

int FileConfigurator::getVolumeChunkSize() const
{
    int iChunkSize = boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_VOLUME_CHUNK_SIZE));
    return iChunkSize;
}

bool FileConfigurator::getRegisterSystemDrive() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_REGISTER_SYSTEM_CACHE_VOLUMES, DEFAULT_REGISTER_SYSTEM_CACHE_VOLUMES));
}

// All set member functions

void FileConfigurator::setCacheDirectory(std::string const& value) const {
    set(SECTION_VXAGENT, KEY_CACHE_DIRECTORY, value);
}

void FileConfigurator::setHttp(HTTP_CONNECTION_SETTINGS s) const {
    set(SECTION_VXTRANSPORT, KEY_HTTP_IPADDRESS, s.ipAddress);
    set(SECTION_VXTRANSPORT, KEY_HTTP_PORT, boost::lexical_cast<std::string>(s.port));
}

void FileConfigurator::setHostName(std::string const& value) const {
    SVLOG2SYSLOG((LM_DEBUG, "CX ip is changed to %C", value.c_str()));
    set(SECTION_VXTRANSPORT, KEY_HTTP_IPADDRESS, value);
}

void FileConfigurator::setPort(int port) const {
    set(SECTION_VXTRANSPORT, KEY_HTTP_PORT, boost::lexical_cast<std::string>(port));
}

void FileConfigurator::setMaxOutpostThreads(int n) const {
    set(SECTION_VXAGENT, KEY_MAX_OUTPOST_THREADS, boost::lexical_cast<std::string>(n));
}

void FileConfigurator::setVolumeChunkSize(int n) const {
    set(SECTION_VXAGENT, KEY_VOLUME_CHUNK_SIZE, boost::lexical_cast<std::string>(n));
}

void FileConfigurator::setRegisterSystemDrive(bool flag) const {
    set(SECTION_VXAGENT, KEY_REGISTER_SYSTEM_CACHE_VOLUMES, boost::lexical_cast<std::string>(flag));
}

//Changed RemoteLog to RemoteLogLevel see Bug#6625 for details
int FileConfigurator::getRemoteLogLevel() const {
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_REMOTE_LOG_LEVEL, 0));
}

//Changed RemoteLog to RemoteLogLevel see Bug#6625 for details
void FileConfigurator::setRemoteLogLevel(int remoteLogLevel) const {
    set(SECTION_VXAGENT, KEY_REMOTE_LOG_LEVEL, boost::lexical_cast<std::string>(remoteLogLevel));
}

std::string FileConfigurator::getTimeStampsOnlyTag()const
{
    return get(SECTION_VXAGENT, KEY_TIME_STAMPS_ONLY_TAG);
}

std::string FileConfigurator::getDestDir()const
{
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_DEST_DIR);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getLogFolderPathOnCsPrimeApplianceToAzure();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return getPSTelemetryFolderPathOnCsPrimeApplianceToAzure();
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

std::string FileConfigurator::getDatExtension()const
{
    return get(SECTION_VXAGENT, KEY_DAT_EXTENSION);
}

std::string FileConfigurator::getMetaDataContinuationTag()const
{
    return get(SECTION_VXAGENT, KEY_META_DATA_CONTINUATION_TAG);
}

std::string FileConfigurator::getMetaDataContinuationEndTag()const
{
    return get(SECTION_VXAGENT, KEY_META_DATA_CONTINUATION_END_TAG);
}

std::string FileConfigurator::getWriteOrderContinuationTag()const
{
    return get(SECTION_VXAGENT, KEY_WRITE_ORDER_CONTINUATION_TAG);
}

std::string FileConfigurator::getWriteOrderContinuationEndTag()const
{
    return get(SECTION_VXAGENT, KEY_WRITE_ORDER_CONTINUATION_END_TAG);
}

std::string FileConfigurator::getPreRemoteName()const
{
    return get(SECTION_VXAGENT, KEY_PRE_REMOTE_NAME);
}

std::string FileConfigurator::getFinalRemoteName()const
{
    return get(SECTION_VXAGENT, KEY_FINAL_REMOTE_NAME);
}

int FileConfigurator::getThrottleWaitTime()const
{
    return boost::lexical_cast<int> (get(SECTION_VXAGENT, KEY_THROTTLE_WAIT_TIME));
}

int FileConfigurator::getSentinelExitTime() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SENTINEL_EXIT_TIME));
}

int FileConfigurator::getSentinelExitTimeV2() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SENTINEL_EXIT_TIME_V2, DEFAULT_SENTINEL_EXIT_TIME_V2));
}

int FileConfigurator::getS2DataWaitTime()const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_S2_DATA_WAIT_TIME));
}

int FileConfigurator::getWaitForDBNotify() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_WAIT_FOR_DB_NOTIFY));
}

int FileConfigurator::getAzureBlobOperationsTimeout() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_AZURE_BLOB_OPS_TIMEOUT, DEFAULT_AZURE_BLOB_OPS_TIMEOUT));
}

std::string FileConfigurator::getProtectedVolumes() const {
    return get(SECTION_VXAGENT, KEY_PROTECTED_VOLUMES);
}

void FileConfigurator::setProtectedVolumes(std::string protectedVolumes) const {
    set(SECTION_VXAGENT, KEY_PROTECTED_VOLUMES, protectedVolumes);
}

std::string FileConfigurator::getNotAllowedMountPointFileName() const
{
    std::string notallowedmntptfilename = getInstallPath();
    notallowedmntptfilename += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    notallowedmntptfilename += "etc";
    notallowedmntptfilename += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    notallowedmntptfilename += "notallowed_for_mountpoints.dat";
    return notallowedmntptfilename;
}

std::string FileConfigurator::getConsistencySettingsCachePath() const
{
    std::string consistencySettingsCachePath = getInstallPath();
#ifdef SV_WINDOWS
    consistencySettingsCachePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    consistencySettingsCachePath += Application_Data_Folder;
#endif
    consistencySettingsCachePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    consistencySettingsCachePath += "etc";
    consistencySettingsCachePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    consistencySettingsCachePath += ConsistencySettingsCacheFile;
    return consistencySettingsCachePath;
}

std::string FileConfigurator::getResyncBatchCachePath() const
{
    std::string resyncBatchCachePath;
    FileConfigurator::getConfigDirname(resyncBatchCachePath);
    resyncBatchCachePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    resyncBatchCachePath += ResyncBatchCacheFile;
    return resyncBatchCachePath;
}

std::string FileConfigurator::GetSourceAgentConfigPath() const
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    std::string sourceAgentConfigPath;

    if (getConfigDirname(sourceAgentConfigPath))
    {
        BOOST_ASSERT(!sourceAgentConfigPath.empty());
        boost::trim(sourceAgentConfigPath);
        if (!boost::ends_with(sourceAgentConfigPath, ACE_DIRECTORY_SEPARATOR_STR_A))
            sourceAgentConfigPath += ACE_DIRECTORY_SEPARATOR_STR_A;

        sourceAgentConfigPath += SourceConfigFile;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return sourceAgentConfigPath;
}

SV_UINT FileConfigurator::getManualResyncStartThresholdInSecs() const
{
    const SV_UINT delay = boost::lexical_cast<SV_UINT>(get
    (SECTION_VXAGENT, KEY_MANUAL_RESYNC_START_THRESHOLD_IN_SECS, DEFAULT_MANUAL_RESYNC_START_THRESHOLD_IN_SECS));

    return delay ? delay : DEFAULT_MANUAL_RESYNC_START_THRESHOLD_IN_SECS;
}

SV_UINT FileConfigurator::getInitialReplicationStartThresholdInSecs() const
{
    const SV_UINT delay = boost::lexical_cast<SV_UINT>(get
    (SECTION_VXAGENT, KEY_INITIAL_REPLICATION_START_THRESHOLD_IN_SECS, DEFAULT_INITIAL_REPLICATION_START_THRESHOLD_IN_SECS));

    return delay ? delay : DEFAULT_INITIAL_REPLICATION_START_THRESHOLD_IN_SECS;
}

SV_UINT FileConfigurator::getAutoResyncStartThresholdInSecs() const
{
    const SV_UINT delay = boost::lexical_cast<SV_UINT>(get
    (SECTION_VXAGENT, KEY_AUTO_RESYNC_START_THRESHOLD_IN_SECS, DEFAULT_AUTO_RESYNC_START_THRESHOLD_IN_SECS));

    return delay ? delay : DEFAULT_AUTO_RESYNC_START_THRESHOLD_IN_SECS;
}

std::string FileConfigurator::getInstallPath() const
{
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_INSTALL_PATH);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return getAgentInstallPathOnCsPrimeApplianceToAzure();
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return getPSInstallPathOnCsPrimeApplianceToAzure();
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}
std::string FileConfigurator::getHealthCollatorDirPath() const
{
    std::string strDefaultHealthCollatorPath = getInstallPath() + 
        ACE_DIRECTORY_SEPARATOR_CHAR_A + std::string("AgentHealth");

    return get(SECTION_VXAGENT, KEY_HEALTHCOLLATOR_PATH, strDefaultHealthCollatorPath);
}

std::string FileConfigurator::getAgentRole() const
{
    if (s_initmode == FILE_CONFIGURATOR_MODE_VX_AGENT)
    {
        return get(SECTION_VXAGENT, KEY_ROLE);
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE)
    {
        return MASTER_TARGET_ROLE;
    }
    else if (s_initmode == FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW)
    {
        return RCM_APPLIANCE_EVTCOLLFORW;
    }
    else
    {
        throw INMAGE_EX("Invalid init mode")(s_initmode);
    }
}

bool FileConfigurator::isMasterTarget() const
{
    return  (0 == strcmp(MASTER_TARGET_ROLE, getAgentRole().c_str()));
}

bool FileConfigurator::isMobilityAgent() const
{
    return  (0 == strcmp(MOBILITY_AGENT_ROLE, getAgentRole().c_str()));
}

bool FileConfigurator::shouldReportScsiIdAsDevName() const
{
    return isMasterTarget();
}


int FileConfigurator::getCxUpdateInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CX_UPDATE_INTERVAL, 60));
}

int FileConfigurator::getCxCDPDiskUsageUpdateInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CX_CDPDISKUSAGE_UPDATE_INTERVAL, 3600));
}

bool FileConfigurator::CanDeleteAllCxCDPPendingUpdates() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DELETE_CXCDPUPDATES, 0));
}

int FileConfigurator::getDeleteAllCxCDPPendingUpdatesInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DELETE_CXCDPUPDATES_INTERVAL, 0));
}

bool FileConfigurator::getIsCXPatched() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_IS_CX_PATCHED, 0));
}

std::string FileConfigurator::getProfileDeviceList() const
{
    return get(SECTION_VXAGENT, KEY_PROFILE_DEVICE_LIST, "");
}

unsigned int FileConfigurator::getMaxDirectSyncFlushToRetnSize() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_GET_MAX_DIRECTSYNC_FLUSH_TO_RETN_SIZE, 16));
}
unsigned int FileConfigurator::getMaxVacpServiceThreads() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_GET_MAX_VACP_SERVICE_THREADS, 2));
}
unsigned int FileConfigurator::getVacpToCxDelay() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_GET_VACP_TO_CX_DELAY, 300/* 5 mins*/));
}

int FileConfigurator::getVolumeRetries() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_VOLUME_RETRIES, SV_DEFAULT_RETRIES));
}

int FileConfigurator::getVolumeRetryDelay() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_VOLUME_RETRY_DELAY, SV_DEFAULT_RETRY_DELAY));
}

int FileConfigurator::getDirectSyncPartitions() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_DIRECTSYNC_PARTITIONS, 0));
}

SV_UINT FileConfigurator::getDirectSyncPartitionSize() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DIRECTSYNC_PARTITIONSIZE, 0));
}

int FileConfigurator::getEnforcerDelay() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_GET_ENFORCER_DELAY, 5));
}

void FileConfigurator::setCxUpdateInterval(int interval) const
{
    set(SECTION_VXAGENT, KEY_CX_UPDATE_INTERVAL, boost::lexical_cast<std::string>(interval));
}

SV_UINT FileConfigurator::getPendingDataReporterInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_PENDING_DATA_REPORTER_INTERVAL, "60"));
}

SV_UINT FileConfigurator::getPendingVsnapReporterInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_PENDING_VSNAP_REPORTER_INTERVAL, "180"));
}

SV_UINT FileConfigurator::getNumberOfBatchRequestToCX() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_NUMBER_OF_BATCH_REQUEST_TO_CX, "256"));
}

SV_UINT FileConfigurator::getDoNotRunDiskPart() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_DONOT_RUN_DISKPART, "0"));
}

std::string FileConfigurator::getArchiveToolPath() const
{
    std::string defaultArchiveTool = this->getInstallPath() + ACE_DIRECTORY_SEPARATOR_STR_A + "inmwinzip.exe";
    return get(SECTION_VXAGENT, KEY_ARCHIVE_TOOL_PATH, defaultArchiveTool.c_str());
}

SV_ULONG FileConfigurator::getMinCacheFreeDiskSpacePercent() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MIN_FREE_FREE_DISKSPACE_PERCENT));
}

int FileConfigurator::getLocalLogSize() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CM_LOCAL_LOG_SIZE, DEFAULT_LOCAL_LOG_SIZE));
}

void FileConfigurator::setMinCacheFreeDiskSpacePercent(SV_ULONG percent) const
{
    set(SECTION_VXAGENT, KEY_MIN_FREE_FREE_DISKSPACE_PERCENT, boost::lexical_cast<std::string>(percent));
}

SV_ULONGLONG FileConfigurator::getMinCacheFreeDiskSpace() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MIN_CACHE_FREE_DISKSPACE));
}

void FileConfigurator::setMinCacheFreeDiskSpace(SV_ULONG space) const
{
    set(SECTION_VXAGENT, KEY_MIN_CACHE_FREE_DISKSPACE, boost::lexical_cast<std::string>(space));
}

SV_ULONGLONG FileConfigurator::getCMMinReservedSpacePerPair() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_CM_MIN_RESERVEDSPCE_PER_PAIR,
        DEFAULT_CM_MIN_RESERVEDSPCE_PER_PAIR));
}

int FileConfigurator::getVirtualVolumesId() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT_VIRTUAL_VOLUMES, KEY_ID, "0"));
}

void  FileConfigurator::setVirtualVolumesId(int id) const
{
    set(SECTION_VXAGENT_VIRTUAL_VOLUMES, KEY_ID, boost::lexical_cast<std::string>(id));
}

std::string FileConfigurator::getVirtualVolumesPath(string key) const
{
    return get(SECTION_VXAGENT_VIRTUAL_VOLUMES, key, "");
}

void FileConfigurator::setVirtualVolumesPath(string key, string value) const
{
    set(SECTION_VXAGENT_VIRTUAL_VOLUMES, key, value);
}

int FileConfigurator::getIdleWaitTime() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_IDLE_WAIT_TIME));
}

unsigned long long FileConfigurator::getVsnapId() const
{
    return boost::lexical_cast<unsigned long long>(get(SECTION_VXAGENT, KEY_VSNAP_ID, 0));
}

SV_ULONGLONG FileConfigurator::getVsnapWriteDataLength() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_VSNAP_WRITEDATA_LENGTH, DEFAULT_VSNAP_WRITEDATA_LENGTH));
}

void FileConfigurator::setVsnapId(unsigned long long snapId) const
{
    set(SECTION_VXAGENT, KEY_VSNAP_ID, boost::lexical_cast<std::string>(snapId));
}

unsigned long long FileConfigurator::getLowLastSnapshotId() const
{
    return boost::lexical_cast<unsigned long long>(get(SECTION_VXAGENT, KEY_LOW_LAST_SNAPSHOT_ID));
}

void FileConfigurator::setLowLastSnapshotId(unsigned long long snapId) const
{
    set(SECTION_VXAGENT, KEY_LOW_LAST_SNAPSHOT_ID, boost::lexical_cast<std::string>(snapId));
}

unsigned long long FileConfigurator::getHighLastSnapshotId() const
{
    return boost::lexical_cast<unsigned long long>(get(SECTION_VXAGENT, KEY_HIGH_LAST_SNAPSHOT_ID));
}

void FileConfigurator::setHighLastSnapshotId(unsigned long long snapId) const
{
    set(SECTION_VXAGENT, KEY_HIGH_LAST_SNAPSHOT_ID, boost::lexical_cast<std::string>(snapId));
}

SV_ULONG FileConfigurator::getMaxMemoryUsagePerReplication() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAX_MEMORYUSAGEPERREPLICATION_VALUE_NAME));
}

SV_ULONG FileConfigurator::getMaxRunsPerInvocation() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAX_RUNSPERINVOCATION_VALUE_NAME));
}

SV_ULONG FileConfigurator::getMaxInMemoryCompressedFileSize() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAX_INMEMORYCOMPRESSEDFILESIZE_VALUE_NAME, DEFAULT_INMEMORYCOMPRESSEDFILESIZE_VALUE));
}


SV_ULONG FileConfigurator::getMaxInMemoryUnCompressedFileSize() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAX_INMEMORYUNCOMPRESSEDFILESIZE_VALUE_NAME, DEFAULT_INMEMORYUNCOMPRESSEDFILESIZE_VALUE));
}

SV_ULONG FileConfigurator::getCompressionChunkSize() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_COMPRESSIONCHUNKSIZE_VALUE_NAME, DEFAULT_COMPRESSIONCHUNKSIZE_VALUE));
}

SV_ULONG FileConfigurator::getCompressionBufSize() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_COMPRESSIONBUFSIZE_VALUE_NAME, DEFAULT_COMPRESSIONBUFSIZE_VALUE));
}

SV_ULONG FileConfigurator::getSequenceCount() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_SEQUENCECOUNT_VALUE_NAME));
}

SV_ULONG FileConfigurator::getSequenceCountInMsecs() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_SEQUENCECOUNTINMSECS_VALUE_NAME));
}

SV_ULONG FileConfigurator::getRetentionBufferSize() const
{
    string defaultValue = "1048576"; // 1 MB

    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_RETENIONBUFSIZE_VALUE_NAME, defaultValue));
}

int FileConfigurator::getCdpPolicyCheckInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CDP_POLICYCHECK_INTERVAL, DEFAULT_CDP_POLICYCHECK_INTERVAL));
}

int FileConfigurator::getCdpFreeSpaceCheckInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CDP_FREESPACECHECK_INTERVAL, DEFAULT_CDP_FREESPACECHECK_INTERVAL));
}

SV_ULONGLONG FileConfigurator::getCdpLowSpaceTriggerPercentage() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_CDP_LOWSPACE_TRIGGER_PERCENT, DEFAULT_CDP_LOWSPACE_TRIGGER_PERCENT));
}

SV_ULONGLONG FileConfigurator::getCdpLowSpaceTriggerLowerThreshold() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_CDP_LOWSPACE_TRIGGER_LOWER_THRESHOLD, DEFAULT_CDP_LOWSPACE_TRIGGER_LOWER_THRESHOLD));
}

SV_ULONGLONG FileConfigurator::getCdpLowSpaceTriggerUpperThreshold() const
{
    string result;
    try {
        result = get(SECTION_VXAGENT, KEY_CDP_LOWSPACE_TRIGGER_UPPER_THRESHOLD);
    }
    catch (ContextualException&) {
        result = boost::lexical_cast<std::string>(DEFAULT_CDP_LOWSPACE_TRIGGER_UPPER_THRESHOLD);
    }
    return boost::lexical_cast<SV_ULONGLONG>(result);
}

SV_ULONGLONG FileConfigurator::getSizeOfReservedRetentionSpace() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_CDP_RESERVED_RETENTION_SPACE_SIZE, DEFAULT_CDP_RESERVED_RETENTION_SPACE_SIZE));
}

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
void FormatVolumeNameForCx(std::string& volumeName)
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

    if (':' == volumeName[len - 2]) {
        --idx;
    }

    if (idx < len) {
        volumeName.erase(idx);
    }
}

bool FileConfigurator::enforceStrictConsistencyGroups() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ENFORCE_STRICT_CONSISTENCY_GROUPS));
}

bool FileConfigurator::trackExperimentalDeviceNumbers() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_TRACK_EXPERIMENTAL_DEVICE_NUMBERS, "0"));
}

bool FileConfigurator::useLinuxDeviceTxt() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_USE_LINUX_DEVICE_TXT, "0"));
}

std::string FileConfigurator::getLinuxDiskDiscoveryCommand() const
{
    return get(SECTION_VXAGENT, KEY_LINUX_DISK_DISC_CMD, "sfdisk -luS 2> /dev/null | grep -e '/dev/' | grep -v Empty");
}

std::string FileConfigurator::getHdlmDlnkmgr() const
{
    return get(SECTION_VXAGENT, KEY_HDLM_DLNKMGR_CMD, "/opt/DynamicLinkManager/bin/dlnkmgr");

}

std::string FileConfigurator::getScliPath() const
{
    /* generally /opt/QLogic_Corporation/SANsurferCLI
     * but link from /usr/sbin is there */
    return get(SECTION_VXAGENT, KEY_SCLI_PATH, "/opt/QLogic_Corporation/SANsurferCLI");
}

std::string FileConfigurator::getPowermtCmd() const
{
    return get(SECTION_VXAGENT, KEY_POWERMT_CMD, POWERMTCMD);
}

std::string FileConfigurator::getGrepCmd() const
{
    return get(SECTION_VXAGENT, KEY_GREP_CMD, GREPCMD);
}

std::string FileConfigurator::getAwkCmd() const
{
    return get(SECTION_VXAGENT, KEY_AWK_CMD, AWKCMD);
}

std::string FileConfigurator::getVxDmpCmd() const
{
    return get(SECTION_VXAGENT, KEY_VXDMP_CMD, VXDMPCMD);
}

std::string FileConfigurator::getVxDiskCmd() const
{
    return get(SECTION_VXAGENT, KEY_VXDISK_CMD, VXDISKCMD);
}

unsigned int FileConfigurator::getLinuxNumPartitions() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_LINUX_NUM_PARTITIONS, DEFAULT_LINUX_NUM_PARTITIONS));
}

std::string FileConfigurator::getVxVsetCmd() const
{
    return get(SECTION_VXAGENT, KEY_VXVSET_CMD, VXVSETCMD);
}

std::string FileConfigurator::getMpxioDrvNames() const
{
    return get(SECTION_VXAGENT, KEY_MPXIO_DRVNAMES, MPXIODRVNAMES);
}

std::string FileConfigurator::getScdidadmCmd() const
{
    return get(SECTION_VXAGENT, KEY_SCDIDADM_CMD, SCDIDADMCMD);
}

std::string FileConfigurator::getEmlxAdmPath() const
{
    return get(SECTION_VXAGENT, KEY_EMLXADM_PATH, "/opt/EMLXemlxu/bin");
}

std::string FileConfigurator::getVMCmds() const
{
    return get(SECTION_VXAGENT, KEY_VM_CMDS, DEFAULT_VM_CMDS);
}

std::string FileConfigurator::getVMCmdsForPats() const
{
    return get(SECTION_VXAGENT, KEY_VM_CMDS_FOR_PATS, DEFAULT_VM_CMDS_FOR_PATS);
}

std::string FileConfigurator::getVMPats() const
{
    return get(SECTION_VXAGENT, KEY_VM_PATS, DEFAULT_VM_PATS);
}

std::string FileConfigurator::getFabricWorldWideName() const
{
    return get(SECTION_VXAGENT, KEY_FABRIC_WWN);

}
int FileConfigurator::getDelayBetweenAppShutdownAndTagIssue() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_DELAY_BETWEEN_APPSHUTDOWN_AND_TAGISSUE, 60));
}

int FileConfigurator::getMaxWaitTimeForTagArrival() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_TAGARRIVAL, 300));
}

int FileConfigurator::getMaxRetryAttemptsToShutdownVXAgent() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_RETRY_ATTEMPTS_TO_SHUTDOWN_VXSERVICE, 2));
}

bool FileConfigurator::getApplicationFailoverChkDskEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_APPLICATION, KEY_AAPLICARION_FAILOVER_CHKDSK, 0));
}
//Xen Registration by Ranjan

int FileConfigurator::getMaxWaitTimeForXenRegistration() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_XENREGISTRATION, 180));
}

int FileConfigurator::getMaxWaitTimeForLvActivation() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_LVACTIVATION, 21600));
}

int FileConfigurator::getMaxWaitTimeForDisplayVmVdiInfo() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_DISPLAYVMVDIINFO, 180));
}

int FileConfigurator::getMaxWaitTimeForCxStatusUpdate() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_CXSTATUSUPDATE, 300));
}
//end of change

void FileConfigurator::setDelayBetweenAppShutdownAndTagIssue(int delayBetweenAppShutdownAndTagIssue) const
{
    set(SECTION_APPLICATION, KEY_DELAY_BETWEEN_APPSHUTDOWN_AND_TAGISSUE, boost::lexical_cast<std::string>(delayBetweenAppShutdownAndTagIssue));
}

void FileConfigurator::setMaxWaitTimeForTagArrival(int noOfSecondsToWaitForTagArrival) const
{
    set(SECTION_APPLICATION, KEY_MAX_WAITTIME_FOR_TAGARRIVAL, boost::lexical_cast<std::string>(noOfSecondsToWaitForTagArrival));
}

/*Added by BSR for Fast Sync TBC*/
bool FileConfigurator::getMemoryBasedSyncApplyEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_MEMORY_BASED_SYNC_APPLY_ENABLED, 0));
}

SV_ULONG FileConfigurator::getMaxMemoryCapForResync() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAX_MEMORY_CAP_FOR_RESYNC));
}

/*End of the change */

unsigned long long FileConfigurator::GetMaxSpacePerCdpDataFile() const
{
    return boost::lexical_cast<unsigned long long>(get(SECTION_VXAGENT, KEY_MAX_SPACE_PER_CDP_DATA_FILE, 67108864));
}

bool FileConfigurator::isCdpDataFilePreAllocationEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_PREALLOCATE_CDP_DATA_FILE, "0"));
}
bool FileConfigurator::isVsnapLocalPersistenceEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VSNAP_LOCAL_PERSISTENCE, "1"));
}
unsigned long long FileConfigurator::GetMaxTimeRangePerCdpDataFile() const
{
    return boost::lexical_cast<unsigned long long>(get(SECTION_VXAGENT, KEY_MAX_TIME_RANGE_PER_CDP_DATA_FILE, "36000000000"));
}

bool FileConfigurator::registerClusterInfoEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_REGISTER_CLUSTER_INFO, "1"));
}

bool FileConfigurator::monitorVolumesEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_MONITOR_VOLUMES, "1"));
}

bool FileConfigurator::reportFullDeviceNamesOnly() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_REPORT_FULL_DEVICE_NAMES_ONLY, "0"));
}

std::string FileConfigurator::getAgentMode() const
{
    return get(SECTION_VXAGENT, KEY_AGENT_MODE);

}

void FileConfigurator::setAgentMode(std::string mode) const
{
    set(SECTION_VXAGENT, KEY_AGENT_MODE, mode);
}

int FileConfigurator::getTransportMaxBufferSize() const
{
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TRANSPORT_MAX_BUFFER_SIZE, "1048576"));
}

int FileConfigurator::getTransportConnectTimeoutSeconds() const
{
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TRANSPORT_CONNECT_TIMEOUT_SECONDS, "0"));
}

int FileConfigurator::getTransportResponseTimeoutSeconds() const
{
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TRANSPORT_RESPONSE_TIMEOUT_SECONDS, "300"));
}

int FileConfigurator::getTransportLowSpeedTimeoutSeconds() const
{
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TRANSPORT_LOW_SPEED_TIMEOUT_SECONDS, "120"));
}

int FileConfigurator::getTransportWriteMode() const
{
    return boost::lexical_cast<int>(get(SECTION_VXTRANSPORT, KEY_TRANSPORT_WRITE_MODE, "1"));
}

/*
 * FUNCTION NAME : FileConfigurator::get_curl_verbose
 *
 * DESCRIPTION : Fetches the value of CURL_VERBOSE option.
 *               if this is set to true, all transfers
 *               through curl are spewed to console in verbose
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : bool - value of CURL_VERBOSE
 *
 */
bool FileConfigurator::get_curl_verbose() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXTRANSPORT, KEY_CURLVERBOSE, DEFAULT_CURLVERBOSE));
}

bool FileConfigurator::ignoreCurlPartialFileErrors() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXTRANSPORT, "IgnoreCurlPartialFileErrors", false));
}

std::string FileConfigurator::getCacheMgrExePathname() const
{
    return get(SECTION_VXAGENT, KEY_CACHEMGR_EXE_PATHNAME);
}

std::string FileConfigurator::getCdpcliExePathname() const
{
    return get(SECTION_VXAGENT, KEY_CDPCLI_EXE_PATHNAME);
}

std::string FileConfigurator::getCdpMgrExePathname() const
{
    return get(SECTION_VXAGENT, KEY_CDPMGR_EXE_PATHNAME);
}

/*
 * FUNCTION NAME : FileConfigurator::getCDPMgrExitTime
 *
 * DESCRIPTION : Fetches the value of CDPMgr exit time in seconds.
 *               cdpmgr exits after running for the specified time
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : cdpmgr exit time interval
 *
 */
int FileConfigurator::getCDPMgrExitTime() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CDPMGR_EXIT_TIME_V2, DEFAULT_CDPMGR_EXIT_TIME_V2));
}

// to speed up rollback operation, metadata can be read ahead by multiple read ahead threads
// KEY_CDP_READAHEAD_ENABLED - configurable based on which read ahead is enabled/disabled
// KEY_CDP_READAHEAD_THREADS - configurable representing number of read ahead threads
// KEY_CDP_READAHEAD_FILECOUNT - no. of files to read ahead

bool FileConfigurator::isCDPReadAheadCacheEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDP_READAHEAD_ENABLED, 1));
}

unsigned int FileConfigurator::CDPReadAheadThreads() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_CDP_READAHEAD_THREADS, 4));
}

unsigned int FileConfigurator::CDPReadAheadFileCount() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_CDP_READAHEAD_FILECOUNT, 4));
}

unsigned int FileConfigurator::CDPReadAheadLength() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_CDP_READAHEAD_LENGTH, 4194304));
}

/*
 * FUNCTION NAME : FileConfigurator::getCacheMgrExitTime
 *
 * DESCRIPTION : Fetches the value of CacheMgr exit time in seconds.
 *               cachemgr exits after running for the specified time
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : cacheMgr exit time interval
 *
 */
int FileConfigurator::getCacheMgrExitTime() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CACHEMGR_EXIT_TIME, DEFAULT_CACHEMGR_EXIT_TIME));
}

/*
 * FUNCTION NAME : FileConfigurator::getMaxDiskUsagePerReplication
 *
 * DESCRIPTION : Fetches the maximum allowed disk usage per replication.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : max. allowed disk usage in bytes per replication
 *
 */
SV_ULONGLONG FileConfigurator::getMaxDiskUsagePerReplication() const
{
    SV_ULONGLONG maxdiskusage =
        boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT,
        KEY_MAX_DISKUSAGEPERREPLICATION_VALUE_NAME, DEFAULT_MAXDISKUSAGE_PERREPLICATION));

    if (maxdiskusage == 0)
    {
        maxdiskusage = DEFAULT_MAXDISKUSAGE_PERREPLICATION;
    }

    return maxdiskusage;
}

/*
 * FUNCTION NAME : FileConfigurator::getNWThreadsPerReplication
 *
 * DESCRIPTION : Fetches the allowed parallel downloads per replication.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : max. allowed parallel downloads per replication
 *
 */
SV_UINT FileConfigurator::getNWThreadsPerReplication() const
{
    SV_UINT nwThreads = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_NWTHREADSPERREPLICATION_VALUE_NAME, DEFAULT_MIN_NW_THREADS));
    if (nwThreads == 0)
    {
        nwThreads = DEFAULT_MIN_NW_THREADS;
    }

    if (nwThreads > DEFAULT_MAX_NW_THREADS)
    {
        nwThreads = DEFAULT_MAX_NW_THREADS;
    }

    return nwThreads;
}

/*
 * FUNCTION NAME : FileConfigurator::getIOThreadsPerReplication
 *
 * DESCRIPTION : Fetches the allowed parallel cache writes per replication.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : max allowed parallel cache writes per replicationn
 *
 */
SV_UINT FileConfigurator::getIOThreadsPerReplication() const
{
    SV_UINT ioThreads = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_IOTHREADSPERREPLICATION_VALUE_NAME, DEFAULT_MIN_IO_THREADS));
    if (ioThreads == 0)
    {
        ioThreads = DEFAULT_MIN_IO_THREADS;
    }

    if (ioThreads > DEFAULT_MAX_IO_THREADS)
    {
        ioThreads = DEFAULT_MAX_IO_THREADS;
    }

    return ioThreads;
}

/*
 * FUNCTION NAME : FileConfigurator::getCMRetryDelayInSeconds
 *
 * DESCRIPTION : Fetches the delay value between two attempts by cache manager
 *               on encountering an error.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value :   delay value between two attempts by cache manager
 *
 */
SV_UINT FileConfigurator::getCMRetryDelayInSeconds() const
{
    SV_UINT delay = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_CMRETRYDELAY_VALUE_NAME, CM_DEFAULT_RETRY_DELAY));

    if (delay == 0)
    {
        delay = CM_DEFAULT_RETRY_DELAY;
    }

    return delay;
}

/*
 * FUNCTION NAME : FileConfigurator::getCMMaxRetries
 *
 * DESCRIPTION : Fetches the delay value between two attempts by cache manager
 *               on encountering an error.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value :   delay value between two attempts by cache manager
 *
 */
SV_UINT FileConfigurator::getCMMaxRetries() const
{
    SV_UINT retries = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_CMMAXRETRIES_VALUE_NAME, CM_DEFAULT_RETRIES));
    if (retries == 0)
    {
        retries = CM_DEFAULT_RETRIES;
    }

    return retries;
}

/*
 * FUNCTION NAME : FileConfigurator::getCMIdleWaitTimeInSeconds
 *
 * DESCRIPTION : Fetches the idle wait time between two retries by cache manager.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value :   idle wait time(in seconds) between two retries by cache manager
 *
 */
SV_UINT FileConfigurator::getCMIdleWaitTimeInSeconds() const
{
    SV_UINT idleWaitTime = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_CMIDLEWAITTIME_VALUE_NAME, CM_DEFAULT_IDLE_WAIT_TIME));
    if (idleWaitTime == 0)
    {
        idleWaitTime = CM_DEFAULT_IDLE_WAIT_TIME;
    }

    return idleWaitTime;
}

/*
* FUNCTION NAME : FileConfigurator::getDriverDppRamUsageInPercent
*
* DESCRIPTION 
*       Defines percentage of RAM to be used by driver as data paged pool.
*       Fetches the data paged pool in percentage of Total RAM.
*       It is configured in KEY_DPP_USAGE_IN_PERCENTAGE.
*       By default it is configured to 6%.
*       Maximum allowed is defined by CM_MAX_DPP_USAGE_IN_PERCENTAGE.
*       If configuration doesn't contain it returns
*                                   CM_DEFAULT_DPP_USAGE_IN_PERCENTAGE.
*       This is only applicable for Windows.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value :   Data Paged Pool in percentage of RAM to be used by driver.
*
*/
SV_ULONG    FileConfigurator::getDriverDppRamUsageInPercent() const
{
    SV_ULONG ulDppUsageInPercent = boost::lexical_cast<SV_ULONG>(get
        (SECTION_DRIVER, KEY_DPP_USAGE_IN_PERCENTAGE, CM_DEFAULT_DPP_USAGE_IN_PERCENTAGE));

    if ((ulDppUsageInPercent <= 0) || (ulDppUsageInPercent >= CM_MAX_DPP_USAGE_IN_PERCENTAGE))
    {
        ulDppUsageInPercent = CM_DEFAULT_DPP_USAGE_IN_PERCENTAGE;
    }
    return ulDppUsageInPercent;
}

/*
* FUNCTION NAME : FileConfigurator::getDriverMinDppUsageInMB
*
* DESCRIPTION
*       Minimum Data paged pool in MB to be used by driver.
*       Fetches the minimum data paged pool size in MB.
*       It is configured in KEY_MIN_DPP_USAGE_IN_MB in conf file.
*       By default it is configured to CM_DEFAULT_MIN_DPP_USAGE_IN_MB.
*       If this configuration is missing CM_DEFAULT_MIN_DPP_USAGE_IN_MB 
*               is returned.
*       If minimum configured is less than CM_DEFAULT_MIN_DPP_USAGE_IN_MB, 
*                   CM_DEFAULT_MIN_DPP_USAGE_IN_MB is returned.
*       This is only applicable for Windows.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : Minumum Data Paged Pool to be used by driver.
*
*/
SV_ULONG    FileConfigurator::getDriverMinDppUsageInMB() const
{
    SV_ULONG ulMinDppUsageInMB = boost::lexical_cast<SV_ULONG>(get
        (SECTION_DRIVER, KEY_MIN_DPP_USAGE_IN_MB, CM_DEFAULT_MIN_DPP_USAGE_IN_MB));

    if (ulMinDppUsageInMB < CM_DEFAULT_MIN_DPP_USAGE_IN_MB)
    {
        ulMinDppUsageInMB = CM_DEFAULT_MIN_DPP_USAGE_IN_MB;
    }

    return ulMinDppUsageInMB;
}


/*
* FUNCTION NAME : FileConfigurator::getDriverMaxDppUsageInMB
*
* DESCRIPTION
*       Maximum Data paged pool in MB to be used by driver.
*       Fetches the maximum data paged pool size in MB.
*       It is configured in KEY_MAX_DPP_USAGE_IN_MB in conf file.
*       By default it is configured to CM_DEFAULT_MAX_DPP_USAGE_IN_MB.
*       If this configuration is missing CM_DEFAULT_MAX_DPP_USAGE_IN_MB
*               is returned.
*       If maximum configured is greater than than CM_DEFAULT_MAX_DPP_USAGE_IN_MB,
*                   CM_DEFAULT_MAX_DPP_USAGE_IN_MB is returned.
*       This is only applicable for Windows.
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : Max Data Paged Pool to be used by driver.
*
*/
SV_ULONG    FileConfigurator::getDriverMaxDppUsageInMB() const
{
    SV_ULONG ulMaxDppUsageInMB = boost::lexical_cast<SV_ULONG>(get
        (SECTION_DRIVER, KEY_MAX_DPP_USAGE_IN_MB, CM_DEFAULT_MAX_DPP_USAGE_IN_MB));
    if (ulMaxDppUsageInMB > CM_DEFAULT_MAX_DPP_USAGE_IN_MB)
    {
        ulMaxDppUsageInMB = CM_DEFAULT_MAX_DPP_USAGE_IN_MB;
    }
    return ulMaxDppUsageInMB;
}

/*
* FUNCTION NAME : FileConfigurator::getDriverDppAlignmentInMB
*
* DESCRIPTION
*       Data paged pool alignment in MB to be used by driver.
*       Fetches the data paged pool alignment in MB.
*       It is configured in KEY_DPP_ALIGNMENT_IN_MB in conf file.
*       By default it is configured to CM_DEFAULT_DPP_ALIGNMENT_IN_MB.
*
*       This is only applicable for Windows.
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : Data Paged Pool Alignment to be used by driver.
*
*/
SV_ULONG    FileConfigurator::getDriverDppAlignmentInMB() const
{
    SV_ULONG ulDppAlignmentInMB = boost::lexical_cast<SV_ULONG>(get
        (SECTION_DRIVER, KEY_DPP_ALIGNMENT_IN_MB, CM_DEFAULT_DPP_ALIGNMENT_IN_MB));

    if (0 <= ulDppAlignmentInMB) {
        ulDppAlignmentInMB = CM_DEFAULT_DPP_ALIGNMENT_IN_MB;
    }

    return ulDppAlignmentInMB;
}

/*
 * FUNCTION NAME : FileConfigurator::AllowOutOfOrderSeq
 *
 * DESCRIPTION : Fetches the action to be taken on recieving an out of
 *               order sequence differential file.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : action to be taken on recieving an out of order sequence
 *
 */
SV_UINT FileConfigurator::AllowOutOfOrderSeq() const
{
    SV_UINT action = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_OUTOFORDERSEQ_VALUE_NAME, SV_DEFAULT_OUTOFORDERSEQACTION));

    return action;
}

/*
 * FUNCTION NAME : FileConfigurator::AllownOutOfOrderTS
 *
 * DESCRIPTION : Fetches the action to be taken on recieving an out of
 *               order timestamp differential file.
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : action to be taken on recieving an out of order timestamp
 *
 */
SV_UINT FileConfigurator::AllowOutOfOrderTS() const
{
    SV_UINT action = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_OUTOFORDERTS_VALUE_NAME, SV_DEFAULT_OUTOFORDERTSACTION));

    return action;
}


/*
 * FUNCTION NAME : FileConfigurator::IgnoreOutOfOrder
 *
 * DESCRIPTION : Fetches the action to be taken on recieving an out of
 *               order timestamp differential file.If this is set, resync
 *               required will not be set
 *
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : action to be taken on recieving an out of order timestamp
 *
 */
SV_UINT FileConfigurator::IgnoreOutOfOrder() const
{
    SV_UINT action = boost::lexical_cast<SV_UINT>(get
        (SECTION_VXAGENT, KEY_IGNOREOUTOFORDER_VALUE_NAME, 0));

    return action;
}

bool FileConfigurator::isCMSVDCheckEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CM_ENABLE_SVD_CHECK, "0"));
}

bool FileConfigurator::getShouldS2RenameDiffs() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SHOULD_S2_RENAME_DIFFERENTIAL_FILES, true));
}

bool FileConfigurator::ShouldProfileDirectSync(void) const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SHOULD_PROFILE_DIRECT_SYNC, false));
}

int FileConfigurator::getS2StrictMode() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_S2_STRICTMODE, "0"));
}

unsigned int FileConfigurator::getRepeatingAlertIntervalInSeconds() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_REPEATING_ALERT_INTERVAL_IN_SECS, "1800"));
}

void FileConfigurator::insertRole(std::map<std::string, std::string> &m) const
{
    std::string role = get(SECTION_VXAGENT, KEY_ROLE, std::string());
    if (!role.empty())
        m.insert(std::make_pair(KEY_ROLE, role));
}

bool FileConfigurator::registerLabelOnDisks() const
{
    /* TODO: should default be not to register ? */
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_REGISTER_LABEL_ON_DISKS, true));
}

bool FileConfigurator::compareHcd() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_COMPARE_HCD, true));
}

unsigned FileConfigurator::DirectSyncIOBufferCount() const
{
    /* 2 is default because there are two stages in direct sync pipeline (read and write) */
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DIRECTSYNC_IO_BUFFER_COUNT, "2"));
}

bool FileConfigurator::pipelineReadWriteInDirectSync() const
{
    return boost::lexical_cast<bool> (get(SECTION_VXAGENT, KEY_PIPELINE_READWRITE_INDIRECTSYNC, true));
}

long FileConfigurator::getTransportFlushThresholdForDiff() const
{
    return boost::lexical_cast<long>(get(SECTION_VXAGENT, KEY_TRANSPORT_FLUSH_THRESHOLD_FORDIFF, 1048576));
}

long FileConfigurator::getAzureBlobFlushThresholdForDiff() const
{
    return boost::lexical_cast<long>(get(SECTION_VXAGENT, KEY_AZUREBLOB_FLUSH_THRESHOLD_FORDIFF, DEFAULT_AZUREBLOB_FLUSH_THRESHOLD_FORDIFF));
}

long FileConfigurator::getVacpParallelMaxRunTime() const
{
    return boost::lexical_cast<long>(get(SECTION_VXAGENT, KEY_VACP_PARALLEL_MAX_RUN_TIME, DEFAULT_VACP_PARALLEL_MAX_RUN_TIME));
}

long FileConfigurator::getVacpDrainBarrierTimeout() const
{
    return boost::lexical_cast<long>(get(SECTION_VXAGENT, KEY_VACP_DRAINBARRIER_TIMEOUT, DEFAULT_VACP_DRAINBARRIER_TIMEOUT));
}

long FileConfigurator::getVacpTagCommitMaxTimeOut() const
{
    return boost::lexical_cast<long>(get(SECTION_VXAGENT, KEY_VACP_TAG_COMMIT_MAX_TIMEOUT, DEFAULT_VACP_TAG_COMMIT_MAX_TIMEOUT));
}

uint32_t FileConfigurator::getVacpExitWaitTime() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_VACP_EXIT_WAIT_TIME, DEFAULT_VACP_EXIT_WAIT_TIME));
}

uint32_t FileConfigurator::getConsistencyLogParseInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_CONSISTENCY_LOG_PARSE_INTERVAL, DEFAULT_CONSISTENCY_LOG_PARSE_INTERVAL));
}

uint32_t FileConfigurator::getAppConsistencyRetryOnRebootMaxTime() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_APP_CONSISTENCY_RETRY_ON_REBOOT_MAX_TIME, DEFAULT_APP_CONSISTENCY_RETRY_ON_REBOOT_MAX_TIME));
}

int FileConfigurator::getProfileDiffs() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_PROFILE_DIFFS, "0"));
}

std::string FileConfigurator::ProfileDifferentialRate() const
{
    return get(SECTION_VXAGENT, KEY_PROFILE_DIFFERENTIAL_RATE, PROFILE_DIFFERENTIAL_RATE_OPTION_NO);
}

SV_UINT FileConfigurator::ProfileDifferentialRateInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_PROFILE_DIFFERENTIAL_RATE_INTERVAL, 600));
}

SV_UINT FileConfigurator::getLengthForFileSystemClustersQuery() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_LENGTHFOR_FILESYSTEM_CLUSTERSQUERY, DEFAULT_LENGTHFOR_FILESYSTEM_CLUSTERSQUERY));
}

unsigned int FileConfigurator::getWaitTimeForSrcLunsValidity() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_WAIT_TIME_FOR_SRCLUN_VALIDITY, DEFAULT_WAITTIME_FOR_SRCVALIDITY));
}

unsigned int FileConfigurator::getSourceReadRetries() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_SOURCE_READ_RETRIES, "30"));
}


bool FileConfigurator::getZerosForSourceReadFailures() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ZEROS_FOR_SOURCE_READ_FAILURES, false));
}


unsigned int FileConfigurator::getSourceReadRetriesInterval() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_SOURCE_READ_RETRIES_INTERVAL, "30"));
}

unsigned long int FileConfigurator::getExpectedMaxDiffFileSize() const
{
    return boost::lexical_cast<unsigned long int>(get(SECTION_VXAGENT, KEY_EXPECTED_MAX_DIFFFILE_SIZE, DEFAULT_EXPECTED_MAX_DIFFFILE_SIZE));
}

unsigned int FileConfigurator::getPendingChangesUpdateInterval() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_PENDING_CHANGES_UPDATE_INTERVAL, "300"));
}

bool FileConfigurator::shouldIssueScsiCmd() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SHOULD_ISSUE_SCSICMD, "1"));
}

unsigned FileConfigurator::getClusSvcRetryTimeInSeconds() const
{
    /* 6 mins should handle almost all cases because on internal setups, the
    *  time is generally 2 to 3 mins for cluster service to be online after svagents */
    return boost::lexical_cast<unsigned>(get(SECTION_VXAGENT, KEY_CLUSSVC_RETRY_TIME_IN_SECONDS, "360"));
}

bool FileConfigurator::getScsiId() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_GET_SCSI_ID, "1"));
}

std::string FileConfigurator::getCxData() const
{
    return get(SECTION_VXAGENT, KEY_CX_DATA, "");
}

int FileConfigurator::getLogFileXfer() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_ENABLE_FILE_XFERLOG, "1"));
}

size_t FileConfigurator::getMetadataReadBufLen() const
{
    return boost::lexical_cast<size_t>(get(SECTION_VXAGENT, KEY_METADATA_READ_BUFLEN, DEFAULT_METADATA_READ_BUFFER_LENGTH));
}

unsigned long FileConfigurator::getMirrorResyncEventWaitTime() const
{
    return boost::lexical_cast<unsigned long>(get(SECTION_VXAGENT, KEY_MIRROR_RESYNC_EVENT_WAITTIME, DEFAULT_MIRROR_RESYNC_EVENT_WAITTIME));
}

int FileConfigurator::getEnableVolumeMonitor() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_ENABLE_VOLUMEMONITOR, "1"));
}

/*
 * FUNCTION NAME : FileConfigurator::getEnforcerAlertInterval
 *
 * DESCRIPTION : fetches the value of time interval between any two alerts
 *               in seconds.
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : time interval between any two alerts
 *
 */
int FileConfigurator::getEnforcerAlertInterval() const
{
    return boost::lexical_cast<int>(get
        (SECTION_VXAGENT, KEY_ENFORCER_ALERT_INTERVAL, DEFAULT_ENFORCER_ALERT_INTERVAL));
}

/*
 * FUNCTION NAME : FileConfigurator::getDataprotectionExitTime
 *
 * DESCRIPTION : fetches the value of Dataprotection exit time in seconds.
 *               dataprotection exits
 *                (i) after running for the specified time  OR
 *                (ii) on quit request from service OR
 *                (iii) change in replication pair settings.
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : dataprotection exit time interval
 *
 */
int FileConfigurator::getDataprotectionExitTime() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DATAPROTECTION_EXIT_TIME, DEFAULT_DATAPROTECTION_EXIT_TIME));
}

/*
 * FUNCTION NAME : FileConfigurator::getSnapshotInterval
 *
 * DESCRIPTION : fetches the snapshot interval in seconds
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : snapshot interval
 *
 */

int FileConfigurator::getSnapshotInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DEFAULT_SNAPSHOT_INTERVAL, DEFAULT_SNAPSHOT_INTERVAL));
}
/*
 * FUNCTION NAME : FileConfigurator::getNotifyCxDiffsInterval
 *
 * DESCRIPTION : gets the interval on which the cx is notified of the diffs drained
 *
 *
 * INPUT PARAMETERS : none
 *
 * OUTPUT PARAMETERS : none
 *
 * NOTES :
 *
 *
 * return value : notify cx diffs interval
 *
 */

int FileConfigurator::getNotifyCxDiffsInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DEFAULT_NOTIFY_DIFF_INTERVAL, DEFAULT_NOTIFY_DIFF_INTERVAL));
}
bool FileConfigurator::AsyncOpEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ASYNCH_OPTIMIZATIONS, 1));
}

bool FileConfigurator::AsyncOpEnabledForPhysicalVolumes() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_ASYNCH_OPTIMIZATIONS_FOR_PHYSICALVOLUMES, 1));
}



bool FileConfigurator::useNewApplyAlgorithm() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_USE_NEW_APPLY_ALGORITHM, 1));
}

SV_ULONG FileConfigurator::MaxAsyncIos() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_MAXASYNCH_IOS, 256));
}

bool FileConfigurator::getCDPMgrUpdateCxPerTargetVolume() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDPMGR_UPDATECX_PER_TARGET_VOLUME, 0));
}

SV_ULONG FileConfigurator::getCDPMgrEventTimeRangeRecordsPerBatch() const
{
    return boost::lexical_cast<SV_ULONG>(get(SECTION_VXAGENT, KEY_CDPMGR_EVENT_TIMERANGE_RECORDS_PER_BATCH, DEFAULT_EVENT_TIMERANGE_RECORDS_PER_BATCH));
}

bool FileConfigurator::getCDPMgrSendUpdatesAtOnce() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDPMGR_SEND_UPDATE_ATONCE, 0));
}

bool FileConfigurator::getCDPMgrDeleteUnusableRecoveryPoints() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDPMGR_DELETE_UNUSABLE_POINTS, 1));
}

bool FileConfigurator::getCDPMgrDeleteStaleFiles() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDPMGR_DELETE_STALEFILES, 1));
}

int FileConfigurator::getAgentHealthCheckInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_AGENT_HEALTH_CHECK_INTERVAL, DEFAULT_AGENT_HEALTH_CHECK_INTERVAL));
}

int FileConfigurator::getMarsHealthCheckInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MARS_HEALTH_CHECK_INTERVAL, DEFAULT_MARS_HEALTH_CHECK_INTERVAL));
}

int FileConfigurator::getMarsServerUnavailableCheckInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MARS_SERVER_UNAVAILABLE_CHECK_INTERVAL, DEFAULT_MARS_SERVER_UNAVAILABLE_CHECK_INTERVAL));
}

int FileConfigurator::getRegisterHostInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_REGISTER_HOST_INTERVAL, DEFAULT_REGISTER_HOST_INTERVAL));
}

int FileConfigurator::getTransportErrorLogInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_TRANSPORT_ERROR_LOG_INTERVAL, DEFAULT_TRANSPORT_ERROR_LOG_INTERVAL));
}

int FileConfigurator::getDiskReadErrorLogInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DISK_READ_ERROR_LOG_INTERVAL, DEFAULT_DISK_READ_ERROR_LOG_INTERVAL));
}

int FileConfigurator::getDiskNotFoundErrorLogInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DISK_NOT_FOUND_ERROR_LOG_INTERVAL, DEFAULT_DISK_NOT_FOUND_ERROR_LOG_INTERVAL));
}
int FileConfigurator::getSrcTelemetryPollInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SRC_TELEMETRY_POLL_INTERVAL, DEFAULT_SRC_TELEMETRY_POLL_INTERVAL));
}
int FileConfigurator::getSrcTelemetryStartDelay() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SRC_TELEMETRY_START_DELAY, DEFAULT_SRC_TELEMETRY_START_DELAY));
}
int FileConfigurator::getMonitorHostInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MONITOR_HOST_INTERVAL, DEFAULT_MONITOR_HOST_INTERVAL));
}

int FileConfigurator::getRcmDetailsPollInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_RCM_DETAILS_POLL_INTERVAL, DEFAULT_RCM_DETAILS_POLL_INTERVAL));
}

int FileConfigurator::getMonitorHostStartDelay() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_MONITOR_HOST_START_DELAY, DEFAULT_MONITOR_HOST_START_DELAY));
}

int FileConfigurator::getConsistencyTagIssueTimeLimit() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_CONSISTENCY_TAG_ISSUE_TIME_LIMIT, DEFAULT_CONSISTENCY_TAG_ISSUE_TIME_LIMIT));
}

std::string FileConfigurator::getMonitorHostCmdList() const
{
    return get(SECTION_VXAGENT, KEY_MONITOR_HOST_CMD_LIST, "");
}

std::string FileConfigurator::getAzureServices() const
{
    return get(SECTION_VXAGENT, KEY_AZURE_SERVICES, DEFAULT_AZURE_SERVICES);
}

std::string FileConfigurator::getAzureServicesAccessCheckCmd() const
{
    return get(SECTION_VXAGENT, KEY_AZURE_SERVICES_ACCESS_CHECK_CMD, "");
}

std::string FileConfigurator::getRecoveryCleanupFileList() const
{
    return get(SECTION_VXAGENT, KEY_RECOVERY_CLEANUP_FILE_LIST, "");
}

void FileConfigurator::setRecoveryCleanupFileList(const std::string &cleanupList) const
{
    set(SECTION_VXAGENT, KEY_RECOVERY_CLEANUP_FILE_LIST, cleanupList);
}

void FileConfigurator::addToRecoveryCleanupFileList(const std::string &cleanupFile) const
{
    std::string cleanupList = getRecoveryCleanupFileList();

    boost::char_separator<char> delm(",");
    boost::tokenizer < boost::char_separator<char> > strtokens(cleanupList, delm);
    for (boost::tokenizer< boost::char_separator<char> > ::iterator it = strtokens.begin(); it != strtokens.end(); ++it)
    {
        std::string filename = *it;
        boost::trim(filename);
        if (filename == cleanupFile)
            return;
    }
    if (!cleanupList.empty())
        cleanupList += ",";
    cleanupList += cleanupFile;
    setRecoveryCleanupFileList(cleanupList);
}

int FileConfigurator::getInitialSettingCallInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_INITIAL_SETTING_CALL_INTERVAL, DEFAULT_INITIAL_SETTING_CALL_INTERVAL));
}

int FileConfigurator::getSettingsCallInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_SETTINGS_CALL_INTERVAL, DEFAULT_SETTINGS_CALL_INTERVAL));
}

SV_ULONGLONG FileConfigurator::getMaxMemoryForDiffSyncFile() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_MEM_PER_DIFFSYNC_FILE, 33554432));
}

SV_ULONGLONG FileConfigurator::getMaxMemoryForResyncFile() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_MEM_PER_RESYNC_FILE, 33554432));
}

bool FileConfigurator::DPCacheVolumeHandle() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DP_CACHEVOLUMEHANDLE, DEFAULT_CACHEVOLUMEHANDLE));
}

SV_UINT FileConfigurator::DPMaxRetentionFileHandlesToCache() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_DPMAX_RETENTIONFILE_TO_CACHE, DEFAULT_DPMAX_RETENTIONFILE_TO_CACHE));
}

std::string FileConfigurator::getHostAgentName() const
{
    return get(SECTION_VXAGENT, KEY_HOST_AGENT_NAME, "svagents");
}

bool FileConfigurator::useUnBufferedIo() const
{
    // On Solaris, O_DIRECT is not supported
    // for direct i/o we need to open the rawio routines
    // so, we just return 0 in case of solaris
#ifdef SV_SUN
    return 0;
#endif
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPUNBUFFEREDIO, 1));
}

bool FileConfigurator::getCMVerifyDiff() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CMMVERIFYDIFFS_VALUE_NAME, 1));
}

bool FileConfigurator::getCMPreserveBadDiff() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CMPRESERVEBADDIFFS_VALUE_NAME, 0));
}

SV_ULONGLONG FileConfigurator::getMaxCdpv3CowFileSize() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_CDPV3_COW_FILESIZE,
        DEFAULT_MAX_CDPV3_COW_FILESIZE));
}

bool FileConfigurator::SimulateSparse() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_SIMULATESPARSE_VALUE_NAME, 0));
}

bool FileConfigurator::TrackExtraCoalescedFiles() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_TRACKEXTRACOALESCEDFILES_VALUE_NAME, 0));
}

bool FileConfigurator::TrackCoalescedFiles() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_TRACKCOALESCEDFILES_VALUE_NAME, 1));
}

void FileConfigurator::setRepositoryLocation(const std::string& repoLocation) const
{
    set(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_CONFIGSTORE_LOCATION, repoLocation);
}

void FileConfigurator::setRepositoryName(const std::string& repositoryName) const
{
    set(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_CONFIGSTORE_NAME, repositoryName);
}

std::string FileConfigurator::getRepositoryName() const
{
    std::string configstorePath = get(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_CONFIGSTORE_NAME);
    return configstorePath;
}
void FileConfigurator::unsetRepositoryLocation()
{
    remove(SECTION_BACKUP_EXPRESS_CONFIGSTORE,
        KEY_CONFIGSTORE_LOCATION);
}

std::string FileConfigurator::getRepositoryLocation() const
{
    std::string configstorePath = get(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_CONFIGSTORE_LOCATION);
    if (configstorePath == "")
    {
        throw INMAGE_EX("couldn't read key value")(SECTION_BACKUP_EXPRESS_CONFIGSTORE)(KEY_CONFIGSTORE_NAME);
    }

    if (configstorePath.length() > 1 && configstorePath[configstorePath.length() - 1] == '\\')
    {
        configstorePath.erase(configstorePath.length() - 1);
    }

    if (configstorePath.length() == 2 && configstorePath[1] == ':')
    {
        configstorePath.erase(configstorePath.length() - 1);
    }

    return configstorePath;
}
bool FileConfigurator::CDPCompressionEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_CDP_COMPRESSION, DEFAULT_CDP_COMPRESSION));
}

void FileConfigurator::SetCDPCompression(bool compress) const
{
    if (compress)
    {
        set(SECTION_VXAGENT, KEY_CDP_COMPRESSION, "1");
    }
    else
    {
        set(SECTION_VXAGENT, KEY_CDP_COMPRESSION, "0");
    }
}

SV_UINT FileConfigurator::GetCDPCompression() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_CDP_COMPRESSION));
}

SV_UINT FileConfigurator::GetDiskRecoveryWaitTime() const
{
    SV_UINT     uiDiskRecoveryTimeSec = DEFAULT_DISK_RECOVERY_WAIT_TIME_SEC;
#ifdef SV_WINDOWS
    if (IsWindowsVistaOrGreater() && !IsWindows7OrGreater()) {
        uiDiskRecoveryTimeSec = DEFAULT_W2K8_DISK_RECOVERY_WAIT_TIME_SEC;
    }
#endif
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_DISK_RECOVERY_WAIT_TIME_SEC, uiDiskRecoveryTimeSec));
}

SV_UINT FileConfigurator::getMaximumWMIConnectionTimeout() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAX_WMI_CONNECTION_TIMEOUT_SEC, DEFAULT_MAX_WMI_CONNECTION_TIMEOUT_SEC));
}

SV_UINT FileConfigurator::getMaxSupportedPartitionsCountUEFIBoot() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAX_SUPPORTED_PARTS_UEFI_BOOT, DEFAULT_MAX_SUPPORTED_PARTS_UEFI_BOOT));
}

SV_UINT FileConfigurator::GetMaximumDiskIndex() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAXIMUM_DISK_INDEX, DEFAULT_MAXIMUM_DISK_INDEX));
}

SV_UINT FileConfigurator::GetMaximumConsMissingDiskIndex() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAXIMUM_CONS_MISSING_DISK_INDEX, DEFAULT_MAXIMUM_CONS_MISSING_DISK_INDEX));
}

bool FileConfigurator::VirtualVolumeCompressionEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VIRTUALVOLUME_COMPRESSION, DEFAULT_VIRTUALVOLUME_COMPRESSION));
}

void FileConfigurator::SetVirtualVolumeCompression(bool compression) const
{
    if (compression)
    {
        set(SECTION_VXAGENT, KEY_VIRTUALVOLUME_COMPRESSION, "1");
    }
    else
    {
        set(SECTION_VXAGENT, KEY_VIRTUALVOLUME_COMPRESSION, "0");
    }
}
bool FileConfigurator::DPBMAsynchIo() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPBM_ASYNCHIO, DEFAULT_DPBM_ASYNCHIO));
}

bool FileConfigurator::DPBMAsynchIoForPhysicalVolumes() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPBM_ASYNCHIO_FOR_PHYSICALVOLUMES, 1));
}

bool FileConfigurator::DPBMCachingEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPBM_CACHING, DEFAULT_DPBM_CACHING));
}

unsigned int FileConfigurator::DPBMCacheSize() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DPBM_CACHESIZE, DEFAULT_DPBM_CACHESIZE));
}

bool FileConfigurator::DPBMUnBufferedIo() const
{
    // On Solaris, O_DIRECT is not supported
    // for direct i/o we need to open the rawio routines
    // so, we just return 0 in case of solaris
#ifdef SV_SUN
    return 0;
#endif
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPBM_UNBUFFEREDIO, DEFAULT_DPBM_UNBUFFEREDIO));
}

unsigned int FileConfigurator::DPBMBlockSize() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DBBM_BLOCKSIZE, DEFAULT_DPBM_BLOCKSIZE));
}

unsigned int FileConfigurator::DPBMBlocksPerEntry() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DPBM_BLOCKSPERENTRY, DEFAULT_DPBM_BLOCKSPERENTRY));
}

bool FileConfigurator::DPBMCompressionEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_DPBM_COMPRESSION, DEFAULT_DPBM_COMPRESSION));
}

unsigned int FileConfigurator::DPBMMaxIos() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DPBM_MAXIOS, DEFAULT_DPBM_MAXIOS));
}

unsigned int FileConfigurator::DPBMMaxMemForIo() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DPBM_MAXMEMFORIO, DEFAULT_DPBM_MAXMEMFORIO));
}

unsigned int FileConfigurator::DPDelayBeforeExitOnError() const
{
    return boost::lexical_cast<unsigned int>(get(SECTION_VXAGENT, KEY_DPDELAY_BEFORE_EXIT_ONERROR, DEFAULT_DPDELAY_BEFORE_EXIT_ONERROR));
}



bool FileConfigurator::IsFilterDriverAvailable() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_FILTERDRIVERAVAILABLE_VALUE_NAME, "0"));
}

bool FileConfigurator::IsVolpackDriverAvailable() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VOLPACKDRIVERAVAILABLE_VALUE_NAME, 0));
}


SV_UINT FileConfigurator::getCdpMaxIOSize() const {
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_CDP_IO_SIZE, DEFAULT_DP_MAX_IO_SIZE));
}

SV_UINT FileConfigurator::getCdpMaxSnapshotIOSize() const {
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_CDP_SNAPSHOT_IO_SIZE, DEFAULT_CDP_SNAPSHOT_IO_SIZE));
}

bool FileConfigurator::IsVsnapDriverAvailable() const {
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VSNAPDRIVERAVAILABLE_VALUE_NAME, 0));
}

SV_UINT FileConfigurator::GetFullBackupInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_FULLBACKUP_SCHEDULE, 24 * 60 * 60));
}

bool FileConfigurator::IsFullBackupRequired() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_FULLBACKUP_ENABLED, false));
}
SV_ULONGLONG FileConfigurator::GetLastFullBackupTimeInGmt() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_SUCCESS_LAST_FULLBKPTIME, 0));
}

void FileConfigurator::SetLastFullBackupTimeInGmt(SV_ULONGLONG gmt)
{
    set(SECTION_VXAGENT, KEY_SUCCESS_LAST_FULLBKPTIME, boost::lexical_cast<std::string>(gmt));
}
SV_ULONGLONG FileConfigurator::getVxAlignmentSize() const {
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_DP_SECTOR_SIZE, DEFAULT_DP_SECTOR_SIZE));
}
SV_UINT FileConfigurator::getVolpackSparseAttribute() const {
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_VOLPACK_SPARSE_ATTRIBUTE_ENABLED, 2));
}

SV_UINT FileConfigurator::getMaxUnmountRetries() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_MAX_UNMOUNT_RETRIES, DEFAULT_MAX_UNMOUNT_RETRIES));
}

bool FileConfigurator::shouldDiscoverOracle() const
{
    return boost::lexical_cast<bool>(get(SECTION_APPAGENT, KEY_ORACLE_DISCOVERY, 0));
}
bool FileConfigurator::useFSAwareSnapshotCopy() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_USE_FSAWARE_COPY, DEFAULT_USE_FSAWARE_COPY));
}

SV_ULONGLONG FileConfigurator::getMaxMemForReadingFSBitmap() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAXMEMFOR_READING_FSBITMAP, DEFAULT_MAXMEMFOR_READING_FSBITMAP));
}

SV_UINT FileConfigurator::getCdpRedoLogMaxFileSize() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_CDP_REDOLOG_MAX_FILE_SIZE, DEFAULT_CDP_REDOLOG_MAX_FILE_SIZE));
}

bool FileConfigurator::IsFlushAndHoldWritesRetryEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_FLUSH_AND_HOLD_WRITES_RETRY_ENABLED, 1));
}

bool FileConfigurator::IsFlushAndHoldResumeRetryEnabled() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_FLUSH_AND_HOLD_RESUME_RETRY_ENABLED, 1));
}

SV_LONGLONG FileConfigurator::MaxDifferenceBetweenFSandRawSize() const
{
    return boost::lexical_cast<SV_LONGLONG>(get(SECTION_VXAGENT, KEY_MAXDIFF_FS_RAW_SIZE, 0));
}

SV_UINT FileConfigurator::getRenameFailureRetryIntervalInSec() const
{
    return DEFAULT_RENAME_FAILURE_RETRY_INTERVAL_IN_SEC;
}

SV_UINT FileConfigurator::getRenameFailureRetryCount() const
{
    return DEFAULT_RENAME_FAILURE_RETRY_COUNT;
}

/*This function makes an entry into the Regsitry about the creation of virtual volumes*/
bool PersistVirtualVolumes(std::string path, const char* volumename)
{
    SVERROR svError = SVS_OK;
    string RegData = path;
    RegData = path + "--";
    RegData += volumename;

    ACE_Guard<ACE_Recursive_Thread_Mutex> autoGuardMutex(g_configWriteRthreadMutex);

    std::string configWriteLockPath;
    FileConfigurator::getConfigDirname(configWriteLockPath);
    configWriteLockPath = configWriteLockPath + ACE_DIRECTORY_SEPARATOR_CHAR_A + "drscout.conf.write.lck";
    ACE_File_Lock configWriteFLock(ACE_TEXT_CHAR_TO_TCHAR(configWriteLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    ACE_Guard<ACE_File_Lock> autoGuardFileLock(configWriteFLock);

    FileConfigurator localConfigurator;
    int counter = localConfigurator.getVirtualVolumesId();
    char regName[26];

    inm_sprintf_s(regName, ARRAYSIZE(regName), "VirVolume%d", counter);


    counter = counter + 1;
    inm_sprintf_s(regName, ARRAYSIZE(regName), "VirVolume%d", counter);

    localConfigurator.setVirtualVolumesPath(regName, RegData.c_str());
    localConfigurator.setVirtualVolumesId(counter);
    return true;
}

//
// FUNCTION NAME : IncrementAndGetVsnapId
//
// DESCRIPTION : Helper routine to get a unique number for creating vsnap devicename
//
// INPUT PARAMETERS : None 
//
// OUTPUT PARAMETERS :  Unique number
//
// NOTES : This is used by GetUniqueVsnapId
//
// return value : True on generating the unique number
//
//
bool IncrementAndGetVsnapId(unsigned long long &VsnapId)
{
    bool rv = true;

    ACE_Guard<ACE_Recursive_Thread_Mutex> autoGuardMutex(g_configWriteRthreadMutex);

    std::string configWriteLockPath;
    FileConfigurator::getConfigDirname(configWriteLockPath);
    configWriteLockPath = configWriteLockPath + ACE_DIRECTORY_SEPARATOR_CHAR_A + "drscout.conf.write.lck";
    ACE_File_Lock configWriteFLock(ACE_TEXT_CHAR_TO_TCHAR(configWriteLockPath.c_str()), O_CREAT | O_RDWR, SV_LOCK_PERMISSIONS, 0);
    ACE_Guard<ACE_File_Lock> autoGuardFileLock(configWriteFLock);

    SV_ULONGLONG			LowLastId;
    SV_ULONGLONG			HighLastId;
    FileConfigurator localConfigurator;

    try {
        VsnapId = localConfigurator.getVsnapId();
    }
    catch (ContextualException &ex) {
        VsnapId = 0;
        return false;
    }

    if (!VsnapId)
    {

        try {
            LowLastId = localConfigurator.getLowLastSnapshotId();
            HighLastId = localConfigurator.getHighLastSnapshotId();
        }
        catch (ContextualException &ex) {
            VsnapId = 0;
            return false;
        }
        VsnapId = HighLastId;
        VsnapId = VsnapId << 32;
        VsnapId += LowLastId;
    }

    VsnapId++;

    try {
        localConfigurator.setVsnapId(VsnapId);
    }
    catch (ContextualException &ex) {
        VsnapId = 0;
        return false;
    }
    return rv;
}

bool FileConfigurator::getSystemUUID(std::string& uuid) const
{
    uuid = get(SECTION_VXAGENT, KEY_SYSTEM_UUID, std::string());
    return !uuid.empty();
}
bool FileConfigurator::getHypervisorName(std::string& hypervisor) const
{
    hypervisor = get(SECTION_VXAGENT, KEY_HYPERVISOR_NAME, std::string());
    return !hypervisor.empty();
}
void FileConfigurator::setHostId(const std::string& hostId) const
{
    set(SECTION_VXAGENT, KEY_HOST_ID, hostId);
}
void FileConfigurator::setSystemUUID(const std::string& uuid) const
{
    set(SECTION_VXAGENT, KEY_SYSTEM_UUID, uuid);
}
void FileConfigurator::setHypervisorName(const std::string& hypervisor) const
{
    set(SECTION_VXAGENT, KEY_HYPERVISOR_NAME, hypervisor);
}

void FileConfigurator::setResourceId(const std::string& resourceId) const
{
    set(SECTION_VXAGENT, KEY_RESOURCE_ID, resourceId);
}

void FileConfigurator::setSourceGroupId(const std::string& sourceGroupId) const
{
    set(SECTION_VXAGENT, KEY_SOURCE_GROUP_ID, sourceGroupId);
}

std::string FileConfigurator::getFailoverVmDetectionId() const
{
    return get(SECTION_VXAGENT, KEY_FAILOVER_VM_DETECTION_ID, std::string());
}

void FileConfigurator::setFailoverVmDetectionId(const std::string& failoverVmId) const
{
    set(SECTION_VXAGENT, KEY_FAILOVER_VM_DETECTION_ID, failoverVmId);
}

bool FileConfigurator::getIsAzureVm() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_IS_AZURE_VM, 0));
}

void FileConfigurator::setIsAzureVm(bool bAzureVM) const
{
    set(SECTION_VXAGENT, KEY_IS_AZURE_VM, boost::lexical_cast<std::string>(bAzureVM));
}

std::string FileConfigurator::getSourceControlPlane() const
{
    return get(SECTION_VXAGENT, KEY_SOURCE_CONTROL_PLANE, std::string());
}

void FileConfigurator::setSourceControlPlane(const std::string& sourceControlPlane) const
{
    set(SECTION_VXAGENT, KEY_SOURCE_CONTROL_PLANE, sourceControlPlane);
}

std::string FileConfigurator::getFailoverVmBiosId() const
{
    return get(SECTION_VXAGENT, KEY_FAILOVER_VM_BIOS_ID, std::string());
}

void FileConfigurator::setFailoverVmBiosId(const std::string& failoverVmBiosId) const
{
    set(SECTION_VXAGENT, KEY_FAILOVER_VM_BIOS_ID, failoverVmBiosId);
}

std::string FileConfigurator::getFailoverTargetType() const
{
    return get(SECTION_VXAGENT, KEY_FAILOVER_TARGET_TYPE, std::string());
}

void FileConfigurator::setFailoverTargetType(const std::string& failoverTargetType) const
{
    set(SECTION_VXAGENT, KEY_FAILOVER_TARGET_TYPE, failoverTargetType);
}

void FileConfigurator::setSerializerType(ESERIALIZE_TYPE serializeType) const
{
    switch (serializeType)
    {
    case PHPSerialize:
        set(SECTION_VXTRANSPORT, KEY_SERIALIZER_TYPE, "PHP");
        break;
    case Xmlize:
        set(SECTION_VXTRANSPORT, KEY_SERIALIZER_TYPE, "XML");
        break;
    default:
        set(SECTION_VXTRANSPORT, KEY_SERIALIZER_TYPE, "PHP");
        break;
    }
}

ESERIALIZE_TYPE FileConfigurator::getSerializerType() const
{
    std::string typeStr = get(SECTION_VXTRANSPORT, KEY_SERIALIZER_TYPE, "PHP");
    ESERIALIZE_TYPE stype = PHPSerialize;
    if (strcmpi(typeStr.c_str(), "XML") == 0)
    {
        stype = Xmlize;
    }

    return stype;
}

SV_UINT FileConfigurator::getSentinalStartStatus() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_START_SENTINAL, 1));
}

SV_UINT FileConfigurator::getDataProtectionStartStatus() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_START_DATAPROTECTION, 1));
}

SV_UINT FileConfigurator::getCDPManagerStartStatus() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_START_CDP_MANAGER, 1));
}

SV_UINT FileConfigurator::getCacheManagerStartStatus() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_START_CACHE_MANAGER, 1));
}

bool FileConfigurator::canEditCatchePath() const
{
    bool bRet = true;
    int value = boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_EDIT_CATCHE_PATH, 1));
    if (value == 0)
    {
        bRet = false;
    }
    return bRet;
}
std::string FileConfigurator::getConsistencyOptions() const
{
    return boost::lexical_cast<std::string>(get(SECTION_VXAGENT, KEY_CONSISTENCY_OPTIONS, "-a all"));
}


SV_UINT FileConfigurator::getS2ReIncarnationInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXAGENT, KEY_S2_REINCARNATION_INTERVAL, 0));
}
bool FileConfigurator::isGuestAccess() const
{
    return boost::lexical_cast<bool> (get(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_GUESTACCESS, 0));
}

void FileConfigurator::setGuestAccess(bool isGuestAccess)
{
    set(SECTION_BACKUP_EXPRESS_CONFIGSTORE, KEY_GUESTACCESS, boost::lexical_cast<std::string>(isGuestAccess));
}

void FileConfigurator::SetFullBackupInterval(SV_UINT intervalInSec)
{
    set(SECTION_VXAGENT, KEY_FULLBACKUP_SCHEDULE, boost::lexical_cast<std::string>(intervalInSec));

}
void FileConfigurator::SetFullBackupRequired(bool required)
{
    set(SECTION_VXAGENT, KEY_FULLBACKUP_ENABLED, boost::lexical_cast<std::string>(required));
}
void FileConfigurator::setHttps(const bool &isHttps)
{
    set(SECTION_VXTRANSPORT, KEY_HTTPS, boost::lexical_cast<std::string>(isHttps));
}


std::string FileConfigurator::pushInstallPath() const
{
    return get(SECTION_PUSHINSTALLER, KEY_INSTALL_PATH);
}

bool FileConfigurator::isVmWarePushInstallationEnabled() const
{
    return boost::lexical_cast<bool> (get(SECTION_PUSHINSTALLER, KEY_VMWAREBASED_PUSHINSTALL_ENABLED, "0"));
}

std::string FileConfigurator::pushLogFolder() const
{
    return get(SECTION_PUSHINSTALLER, KEY_LOG_FOLDER, "");
}

std::string FileConfigurator::pushTmpFolder() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSHTMP_FOLDER, "");
}

HTTP_CONNECTION_SETTINGS FileConfigurator::pushHttpSettings() const
{
    HTTP_CONNECTION_SETTINGS s;
    std::string ipaddr = get(SECTION_PUSHINSTALLERTRANSPORT, KEY_HTTP_IPADDRESS).c_str();
    inm_strcpy_s(s.ipAddress, ARRAYSIZE(s.ipAddress), ipaddr.c_str());
    s.port = boost::lexical_cast<int>(get(SECTION_PUSHINSTALLERTRANSPORT, KEY_HTTP_PORT).c_str());
    return s;
}

bool FileConfigurator::pushHttps() const
{
    return boost::lexical_cast<bool> (get(SECTION_PUSHINSTALLERTRANSPORT, KEY_HTTPS, "0"));
}

std::string FileConfigurator::pushHostid() const
{
    return get(SECTION_PUSHINSTALLER, KEY_HOST_ID, "");
}

int FileConfigurator::pushLogLevel() const
{
    return static_cast<SV_LOG_LEVEL>(boost::lexical_cast<int>(get(SECTION_PUSHINSTALLER, KEY_LOG_LEVEL, "3")));
}
void FileConfigurator::pushLogLevel(int level)
{
    set(SECTION_PUSHINSTALLER, KEY_LOG_LEVEL, boost::lexical_cast<std::string>(level));
}

bool FileConfigurator::pushSignVerificationEnabled() const
{
    return boost::lexical_cast<bool> (get(SECTION_PUSHINSTALLERTRANSPORT, KEY_PUSH_SIGNATURE_CHECK, "1"));
}

bool FileConfigurator::pushSignVerificationEnabledOnRemoteMachine() const
{
    return boost::lexical_cast<bool> (get(SECTION_PUSHINSTALLERTRANSPORT, KEY_PUSH_SIGNATURE_CHECK_ON_REMOTEMACHINE, "0"));
}

std::string FileConfigurator::pushInstallTelemetryLogsPath() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSHINSTALL_TELEMETRY_FOLDER, "");
}

std::string FileConfigurator::agentTelemetryLogsPath() const
{
    return get(SECTION_PUSHINSTALLER, KEY_AGENT_TELEMETRY_FOLDER, "");
}

std::string FileConfigurator::pushLocalRepositoryPathUA() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_REPO_UA, "");
}

std::string FileConfigurator::pushLocalRepositoryPathPushclient() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_REPO_PUSHCLIENT, "");
}

std::string FileConfigurator::pushLocalRepositoryPathPatch() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_REPO_PATCH, "");
}

std::string FileConfigurator::pushRemoteRootFolderWindows() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_REMOTEFOLDERWINDOWS, "");

}

std::string FileConfigurator::pushRemoteRootFolderUnix() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_REMOTEFOLDERUNIX, "");
}


std::string FileConfigurator::pushOsScriptPath() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_OSSCRIPTPATH, "");
}

std::string FileConfigurator::pushVmwareApiWrapperCmd() const
{
    return get(SECTION_PUSHINSTALLER, KEY_PUSH_VMWAREAPICMD, "");
}

int FileConfigurator::pushJobTimeoutSecs() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_JOBTIMEOUT, 1800));
}

int FileConfigurator::pushVmwareApiWrapperCmdTimeOutSecs() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_VMWAREWRAPPER_TIMEOUT, 0));
}

int FileConfigurator::pushJobFetchIntervalInSecs() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_JOBFETCHINTERVAL, 60));
}

int FileConfigurator::pushJobRetries() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_JOBRETRIES, 10));
}

int FileConfigurator::pushJobRetryIntervalInsecs() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_JOBRETRYINTERVAL, 30));
}

int FileConfigurator::pushJobCSRetries() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_CSJOBRETRIES, 15));
}

int FileConfigurator::pushJobCSRetryIntervalInsecs() const
{
    return boost::lexical_cast<int> (get(SECTION_PUSHINSTALLER, KEY_PUSH_CSJOBRETRYINTERVAL, 60));
}
int FileConfigurator::getLastNLinesCountToReadFromLogOrBuffer() const
{
    return boost::lexical_cast<int>(get(SECTION_APPLICATION, KEY_GET_LAST_N_LINES_FROM_LOG, 8));
}

//For OMS Statistics and DR Metrics Collection
int FileConfigurator::getOMSStatsCollInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_OMS_STATS_COLLECTION_INTERVAL, DEFAULT_OMS_STATS_COLL_INTERVAL));
}

int FileConfigurator::getOMSStatsSendingIntervalToPS() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_OMS_STATS_SENDING_INTERVAL_TO_PS, DEFAULT_OMS_STATS_SENDING_INTERVAL));
}

int FileConfigurator::getDRMetricsCollInterval() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT, KEY_DR_METRICS_COLLECTION_INTERVAL, DEFAULT_DR_METRICS_COLL_INTERVAL));
}

/* churn-throughput CX session definitions start */
SV_ULONGLONG FileConfigurator::getMaxDiskChurnSupportedMBps() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_DISK_CHURN_SUPPORTED_MBPS,
        DEFAULT_MAX_DISK_CHURN_SUPPORTED_MBPS));
}

SV_ULONGLONG FileConfigurator::getMaxVMChurnSupportedMBps() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_VM_CHURN_SUPPORTED_MBPS,
        DEFAULT_MAX_VM_CHURN_SUPPORTED_MBPS));
}
SV_ULONGLONG FileConfigurator::getMaximumTimeJumpForwardAcceptableInMs() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_TIMEJUMP_FWD_ACCEPTABLE_IN_MS,
        DEFAULT_MAX_TIMEJUMP_FWD_ACCEPTABLE_IN_MS));
}
SV_ULONGLONG FileConfigurator::getMaximumTimeJumpBackwardAcceptableInMs() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_TIMEJUMP_BWD_ACCEPTABLE_IN_MS,
        DEFAULT_MAX_TIMEJUMP_BWD_ACCEPTABLE_IN_MS));
}
SV_ULONGLONG FileConfigurator::getMinConsecutiveTagFailures() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MIN_CONSECUTIVE_TAG_FAILURES,
        DEFAULT_MIN_CONSECUTIVE_TAG_FAILURES));
}

SV_ULONGLONG FileConfigurator::getMaxS2LatencyBetweenCommitDBAndGetDB() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_S2_LATENCY_BETWEEN_COMMITDB_AND_GETDB,
        DEFAULT_MAX_S2_LATENCY_BETWEEN_COMMITDB_AND_GETDB));
}

SV_ULONGLONG FileConfigurator::getCxClearHealthCount() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_CX_CLEAR_HEALTH_COUNT,
        DEFAULT_CX_CLEAR_HEALTH_COUNT));
}

SV_ULONGLONG FileConfigurator::getMaxWaitTimeForHealthEventCommitFailureInSec() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT, KEY_MAX_WAIT_TIME_FOR_HEALTH_EVENT_COMMIT_FAILURE_IN_SEC,
        DEFAULT_MAX_WAIT_TIME_FOR_HEALTH_EVENT_COMMIT_FAILURE_IN_SEC));
}

/* churn-throughput CX session definitions end */

int FileConfigurator::getMonitoringCxpsClientTimeoutInSec() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_MONITORING_CXPS_CLIENT_TIMEOUT_IN_SEC,
        DEFAULT_MONITORING_CXPS_CLIENT_TIMEOUT_IN_SEC));
}

SV_ULONGLONG FileConfigurator::getPausePendingAckRepeatIntervalInSecs() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXAGENT,
        KEY_PAUSE_PENDING_ACK_REPEAT_INTERVAL_IN_SECS,
        DEFAULT_PAUSE_PENDING_ACK_REPEAT_INTERVAL_IN_SECS));
}

void FileConfigurator::setCSType(std::string csType) const
{
    if (csType == "CSLegacy" || csType == "CSPrime")
        return set(SECTION_VXAGENT, KEY_CS_TYPE, csType);
    else
    {
        DebugPrintf(SV_LOG_ERROR, "%s : Invalid value of CSType : %s\n",
            FUNCTION_NAME, csType.c_str());
        throw INMAGE_EX("Invalid value of CSType : " + csType);
    }
}

std::string FileConfigurator::getCSType() const
{
    return get(SECTION_VXAGENT, KEY_CS_TYPE, "");
}

// get the interval in seconds at which the replicaiton settings to be fetched
uint32_t    FileConfigurator::getReplicationSettingsFetchInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_SETTINGS_FETCH_INTERVAL, DEFAULT_SETTINGS_FETCH_INTERVAL));
}

// get the number job worker threads to use
uint32_t FileConfigurator::getRcmJobWorkerThreadCount() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_NUM_OF_RCM_JOB_WORKER_THREADS, DEFAULT_NUM_OF_RCM_JOB_WORKER_THREADS));
}

// get the number job worker threads to use
uint32_t FileConfigurator::getRcmJobMaxAllowedTimeInSec() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_RCM_JOB_MAX_ALLOWED_TIME_INSECS, DEFAULT_RCM_JOB_MAX_ALLOWED_TIME_INSECS));
}

std::string FileConfigurator::getCloudPairingStatus() const
{
    return get(SECTION_VXAGENT, KEY_CLOUD_PAIRING_STATUS, "");
}

void FileConfigurator::setCloudPairingStatus(const std::string& status) const
{
    return set(SECTION_VXAGENT, KEY_CLOUD_PAIRING_STATUS, status);
}

uint32_t FileConfigurator::getPeerRemoteResyncStateParseRetryCount() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_PEER_REMOTE_RESYNCSTATE_PARSE_RETRY_COUNT, DEFAULT_PEER_REMOTE_RESYNCSTATE_PARSE_RETRY_COUNT));
}

uint32_t FileConfigurator::getPeerRemoteResyncStateMonitorInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_PEER_REMOTE_RESYNCSTATE_MONITOR_INTERVAL_IN_SEC, DEFAULT_PEER_REMOTE_RESYNCSTATE_MONITOR_INTERVAL_IN_SEC));
}

uint32_t FileConfigurator::getMaxResyncBatchSize() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT, KEY_MAX_RESYNC_BATCH_SIZE, DEFAULT_MAX_RESYNC_BATCH_SIZE));
}

SV_ULONGLONG FileConfigurator::getAzureBlobOperationMaximumTimeout() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXTRANSPORT,
        KEY_AZURE_BLOBS_OPERATION_MAXIMUM_TIMEOUT,
        DEFAULT_AZURE_BLOBS_OPERATION_MAXIMUM_TIMEOUT));
}

SV_ULONGLONG FileConfigurator::getAzureBlobOperationMinimumTimeout() const
{
    return boost::lexical_cast<SV_ULONGLONG>(get(SECTION_VXTRANSPORT,
        KEY_AZURE_BLOBS_OPERATION_MINIMUM_TIMEOUT,
        DEFAULT_AZURE_BLOBS_OPERATION_MINIMUM_TIMEOUT
    ));
}

SV_UINT FileConfigurator::getAzureBlobOperationTimeoutResetInterval() const
{
    return boost::lexical_cast<SV_UINT>(get(SECTION_VXTRANSPORT,
        KEY_AZURE_BLOBS_OPERATION_TIMEOUT_RESET_INTERVAL,
        DEFAULT_AZURE_BLOBS_OPERATION_TIMEOUT_RESET_INTERVAL
    ));
}

SV_UINT FileConfigurator::getLogContainerRenewalRetryTimeInSecs() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_LOG_CONTAINER_RENEWAL_RETRY_IN_SECS,
        DEFAULT_LOG_CONTAINER_RENEWAL_RETRY_IN_SECS));
}

bool FileConfigurator::validateAgentInstallerChecksum() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT, KEY_VALIDATE_INSTALLER_CHECKSUM, 1));
}

uint32_t FileConfigurator::getUpdateDirectoryCreationRetryCount() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_UPGRADE_DIRECTORY_CREATION_RETRY_COUNT,
        DEFAULT_UPGRADE_DIRECTORY_CREATION_RETRY_COUNT));
}

uint32_t FileConfigurator::getUpdateDirectoryCreationRetryInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_UPGRADE_DIRECTORY_CREATION_RETRY_INTERVAL,
        DEFAULT_UPGRADE_DIRECTORY_CREATION_RETRY_INTERVAL));
}

uint32_t FileConfigurator::getUpdateDirectoryDeletionRetryCount() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_UPGRADE_DIRECTORY_DELETION_RETRY_COUNT,
        DEFAULT_UPGRADE_DIRECTORY_DELETION_RETRY_COUNT));
}

uint32_t FileConfigurator::getUpdateDirectoryDeletionRetryInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_UPGRADE_DIRECTORY_DELETION_RETRY_INTERVAL,
        DEFAULT_UPGRADE_DIRECTORY_DELETION_RETRY_INTERVAL));
}

uint32_t FileConfigurator::getInstallerDownloadRetryCount() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_INSTALLER_DOWNLOAD_RETRY_COUNT,
        DEFAULT_INSTALLER_DOWNLOAD_RETRY_COUNT));
}

int FileConfigurator::getInstallerUnzipRetryCount() const
{
    return boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_INSTALLER_UNZIP_RETRY_COUNT,
        DEFAULT_INSTALLER_UNZIP_RETRY_COUNT));
}

uint32_t FileConfigurator::getInstallerUnzipRetryInterval() const
{
    return boost::lexical_cast<uint32_t>(get(SECTION_VXAGENT,
        KEY_INSTALLER_UNZIP_RETRY_INTERVAL,
        DEFAULT_INSTALLER_UNZIP_RETRY_INTERVAL));
}

bool FileConfigurator::getIsCredentialLessDiscovery() const
{
    return boost::lexical_cast<bool>(get(SECTION_VXAGENT,
        KEY_IS_CREDENTIAL_LESS_DISCOVERY, "0"));
}

void FileConfigurator::setIsCredentialLessDiscovery(const bool& isCredentialLessDiscovery) const 
{
    set(SECTION_VXAGENT, KEY_IS_CREDENTIAL_LESS_DISCOVERY, 
        boost::lexical_cast<std::string>(isCredentialLessDiscovery));
}

void FileConfigurator::setSwitchApplianceState(SwitchAppliance::State state) const
{
    return set(SECTION_VXAGENT, KEY_SWITCH_APPLIANCE_STATE, boost::lexical_cast<std::string>(state));
}

SwitchAppliance::State FileConfigurator::getSwitchApplianceState() const
{
    return (SwitchAppliance::State)boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_SWITCH_APPLIANCE_STATE, 0));
}

void FileConfigurator::setVacpState(VacpConf::State state) const
{
    return set(SECTION_VXAGENT, KEY_VACP_STATE, boost::lexical_cast<std::string>(state));
}

VacpConf::State FileConfigurator::getVacpState() const
{
    return (VacpConf::State)boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_VACP_STATE, 0));
}

void FileConfigurator::setMigrationState(Migration::State state) const
{
    return set(SECTION_VXAGENT, KEY_MIGRATION_STATE, boost::lexical_cast<std::string>(state));
}

Migration::State FileConfigurator::getMigrationState() const
{
    return (Migration::State)boost::lexical_cast<int>(get(SECTION_VXAGENT,
        KEY_MIGRATION_STATE, 0));
}

std::string FileConfigurator::getAdditionalInstallPaths() const
{
    return get(SECTION_VXAGENT, KEY_ADDITIONAL_INSTALL_PATHS, "");
}
