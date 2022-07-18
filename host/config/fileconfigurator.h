//
// fileconfigurator.h: defines configurator that fetches values from a config
// file.  Does not look for file changes, it assumes it is static information.
// Thus signals will never fire.
//
#ifndef FILECONFIGURATOR_H
#define FILECONFIGURATOR_H

#include <map>
#include <string>
#include <ace/File_Lock.h>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>
//#include <ace/Process_Semaphore.h>

#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>

#include "svsemaphore.h"

#include <boost/property_tree/ptree.hpp>

#include "configurevxagent.h"
#include "configurevxtransport.h"

/// In Caspian V2 release, we introduced PaaS MT
/// with this for E2E protection, we use InMage CDP based protection
/// for E2A, MT acts as a proxy using Azure Protection Service
/// The below key is to tell the scenarios supported by this MT

static const char KEY_MT_SUPPORTED_DATAPLANES[] = "MT_SUPPORTED_DATAPLANES";

#ifdef WIN32
static std::string const DEFAULT_MT_SUPPORTED_DATAPLANES("INMAGE_DATA_PLANE,AZURE_DATA_PLANE");
static const std::string SVAGENTS_LINK = "svagents.exe";
static const std::string SVAGENTS_CS = "svagentsCS.exe";
static const std::string SVAGENTS_RCM = "svagentsRCM.exe";
static const std::string S2_CS = "s2CS.exe";
static const std::string S2_RCM = "s2RCM.exe";
static const std::string DATAPROTECTION_CS = "dataprotection.exe";
static const std::string DATAPROTECTION_RCM = "DataProtectionSyncRcm.exe";
#else
static std::string const DEFAULT_MT_SUPPORTED_DATAPLANES("INMAGE_DATA_PLANE");
static const std::string SVAGENTS_LINK = "svagents";
static const std::string SVAGENTS_CS = "svagentsV2A";
static const std::string SVAGENTS_RCM = "svagentsA2A";
static const std::string S2_CS = "s2V2A";
static const std::string S2_RCM = "s2A2A";
static const std::string DATAPROTECTION_CS = "dataprotection";
static const std::string DATAPROTECTION_RCM = "DataProtectionSyncRcm";
#endif

#ifdef SV_UNIX
static std::string const DEFAULT_CLEAN_SHUTDOWN_STATUS_FILE = "/etc/vxagent/involflt/common/CleanShutdown";
#endif


#define MASTER_TARGET_ROLE          "MasterTarget"
#define MOBILITY_AGENT_ROLE         "Agent"
#define RCM_APPLIANCE_EVTCOLLFORW   "RcmApplianceEvtcollforw"

const std::string FileConfiguratorModes[] = {
    "FILE_CONFIGURATOR_MODE_VX_AGENT",
    "FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE",
    "FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW"
};

enum FileConfiguratorMode {
	FILE_CONFIGURATOR_MODE_VX_AGENT,
	FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE,
    FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW
};

namespace SwitchAppliance {
    enum State {
        NONE = 0,
        SWITCH_APPLIANCE_SUCCEEDED,
        PREPARE_SWITCH_APPLIANCE_START,
        PREPARE_SWITCH_APPLIANCE_SUCCEEDED,
        SWITCH_APPLIANCE_START,
        VERIFY_CLIENT_FAILED,
        VERIFY_CLIENT_SUCCEEDED,
        REGISTER_SOURCE_AGENT_FAILED,
        REGISTER_SOURCE_AGENT_SUCCEEDED,
        GET_NON_CACHED_SETTINGS_FAILED,
        INCONSISTENT
    };

    const int JobWaitTimeOut = 30;   //minutes
    const int JobTimeOut = 600;      //seconds
    const int PollTime = 30;         //seconds
    const int WaitTime = 5;          //seconds
}

namespace VacpConf {
    enum State {
        NONE=0,
        VACP_STARTED,
        VACP_STOPPED,
        VACP_START_REQUESTED,
        VACP_STOP_REQUESTED
    };

    const int MaxRetryCount = 3;
    const int RequestTimeOut = 150;  //seconds
    const int PollTime = 30;         //seconds
    const int WaitTime = 5;          //seconds
};

namespace Migration {
    enum State {
        NONE=0,
        UNKNOWN,
        REGISTRATION_PENDING,
        REGISTRATION_FAILURE,
        REGISTRATION_SUCCESS,
        FINAL_REPLICATION_CYCLE_POLICY_FAILURE,
        FINAL_REPLICATION_CYCLE_POLICY_SUCCESS, 
        RESUME_REPLICATION_POLICY_FAILURE,
        RESUME_REPLICATION_POLICY_SUCCESS,
        MIGRATION_SWITCHED_TO_RCM,
        MIGRATION_SWITCHED_TO_RCM_PENDING,
        MIGRATION_ROLLBACK_PENDING,
        MIGRATION_ROLLBACK_IN_PROGRESS,
        MIGRATION_ROLLBACK_SUCCESS,
        MIGRATION_FAILED,
        MIGRATION_SUCCESS
    };

    const int MaxRCMSettingPollRetryCount = 60;
    const int RCMSettingPollSleepTime = 10; // seconds
    const int MaxRetryCount = 3;
    const int SleepTime = 10;            //seconds
    const int ServiceStartTimeOut = 120; //seconds
    const int ServiceStopTimeOut = 210;  //seconds
}

namespace CsPrimeApplianceToAzureAgentProperties {
    const std::string LOCAL_PREFERENCES_PATH = "LocalPreferencesFilePath";
    const std::string SERVICE_INSTALL_PATH = "ServiceInstallPath";
}

class FileConfigurator :
    public ConfigureLocalVxAgent,
    public ConfigureVxTransport
{
public:
    static std::string getConfigPathname();
    //
    // constructor/destructor
    //
    FileConfigurator();
    FileConfigurator(const std::string fileName);

    // PR# 5554
    // cdpcli will use cached initial settings instead of being dependent
    // on cx being available. The stttings are persisted locally
    // in path provided by getConfigCachePathname routine
    // getConfigDirname is used as a helper routine by
    // getConfigCachePathname for getting the directory name.
    // service is responsible for persistence of the settings
    // on service start and on any change in the initial settings
    //
    static bool getConfigDirname(std::string & configdir);
    static bool getConfigCachePathname(std::string & cachePath);
    static bool getVolumesCachePathname(std::string & volumesCachePath);
    static bool getVxProtectedDeviceDetailCachePathname(std::string & cachePath);
    static bool getDeprecatedVxProtectedDeviceDetailCachePathname(std::string & cachePath);
    static bool getVxPlatformTypeForDriverPersistentFile(std::string &filePath);
public:
    //
    // ConfigureLocalVxAgent interface
    //

    std::string getMTSupportedDataPlanes() const;
    SV_ULONGLONG getMinAzureUploadSize() const;
    unsigned int getMinTimeGapBetweenAzureUploads() const;
    unsigned int getTimeGapBetweenFileArrivalCheck() const;
    unsigned int getMaxAzureAttempts() const;
    unsigned int getAzureRetryDelayInSecs() const;
    unsigned int getAzureImplType() const;

    virtual std::string getLogPathname() const;
    std::string getLogDir() const;
    std::string getCacheDirectory() const;
    std::string getHostId() const;
    std::string getResourceId() const;
    std::string getSourceGroupId() const;
    std::string getUnregisterAgentLogPath() const;
    std::string getFailoverVmDetectionId() const;
    bool getIsAzureVm() const;
    std::string getSourceControlPlane() const;
    std::string getFailoverVmBiosId() const;
    std::string getFailoverTargetType() const;

    // EvtCollForw settings - Start
    SV_LOG_LEVEL getEvtCollForwAgentLogPostLevel() const;
    std::vector<std::string> getSourceAgentLogsToUpload() const;
    int getEvtCollForwPollIntervalInSecs() const;
    int getEvtCollForwPenaltyTimeInSecs() const;
    int getEvtCollForwMaxStrikes() const;
    unsigned int getEvtCollForwProcSpawnInterval() const;
    unsigned int getEvtCollForwMaxMarsUploadFilesCnt() const;
    uint32_t getEvtCollForwCxTransportMaxAttempts() const;
    std::string getEvtCollForwIRCompletedFilesMoveDir() const;
    bool isEvtCollForwEventLogUploadEnabled() const;
    // EvtCollForw settings - End

    // CsJobProcessor settings -- Start
    unsigned int getCsJobProcessorProcSpawnInterval() const;
    // CsJobProcessor settings -- End

    std::string getRcmSettingsPath() const;
    void setRcmSettingsPath(const std::string& rcmSettingsPath) const;
    unsigned int getRcmRequestTimeout() const;
    std::string getProxySettingsPath() const;
    std::string getVmPlatform() const;
    bool IsAzureToAzureReplication() const;

    int getMaxDifferentialPayload() const;
    SV_UINT getFastSyncReadBufferSize() const;
    SV_LOG_LEVEL getLogLevel() const;
    SV_UINT getLogMaxCompletedFiles() const;
    SV_UINT getLogCutInterval() const;
    SV_UINT getLogMaxFileSize() const;
    SV_UINT getLogMaxFileSizeForTelemetry() const;

    void setLogLevel(SV_LOG_LEVEL logLevel) const;
    SV_HOST_AGENT_TYPE getAgentType() const;
    std::string getDiffSourceExePathname() const;
    void setDiffSourceExePathname(const std::string& diffSourceExePathname) const;
    std::string getDataProtectionExePathname() const;
    void setDataProtectionExePathname(const std::string& dataProtectionExePathname) const;

    /* Added by BSR
     *  Project: New Sync
     *  Start  */
    std::string getDataProtectionExeV2Pathname() const;
    /* End */

    int getS2StrictMode() const;
    unsigned int getRepeatingAlertIntervalInSeconds() const;
    void insertRole(std::map<std::string, std::string> &m) const;
    bool registerLabelOnDisks() const;
    bool compareHcd() const;

    /* honoured only if pipeline is enabled in direct sync */
    unsigned int DirectSyncIOBufferCount() const;

    bool pipelineReadWriteInDirectSync() const;
    bool getShouldS2RenameDiffs() const;
    bool ShouldProfileDirectSync(void) const;
    long getTransportFlushThresholdForDiff() const;
    long getAzureBlobFlushThresholdForDiff() const;
    long getVacpParallelMaxRunTime() const;
    long getVacpDrainBarrierTimeout() const;
    long getVacpTagCommitMaxTimeOut() const;
    uint32_t getVacpExitWaitTime() const;
    uint32_t getConsistencyLogParseInterval() const;
    uint32_t getAppConsistencyRetryOnRebootMaxTime() const;
    int getProfileDiffs() const;
    std::string ProfileDifferentialRate() const;
    SV_UINT ProfileDifferentialRateInterval() const;
    SV_UINT getLengthForFileSystemClustersQuery() const;
    unsigned int getWaitTimeForSrcLunsValidity() const;
    unsigned int getSourceReadRetries() const;
    bool getZerosForSourceReadFailures() const;
    unsigned int getSourceReadRetriesInterval() const;
    unsigned long int getExpectedMaxDiffFileSize() const;
    unsigned int getPendingChangesUpdateInterval() const;
    bool shouldIssueScsiCmd() const;
    unsigned getClusSvcRetryTimeInSeconds() const;
    bool getScsiId() const;
    std::string getCxData() const;
    int getLogFileXfer() const;
    size_t getMetadataReadBufLen() const;
    unsigned long getMirrorResyncEventWaitTime() const;
    int getEnableVolumeMonitor() const;
    bool isCMSVDCheckEnabled() const;

    /*Added by BSR for Fastsync TBC */
    bool getMemoryBasedSyncApplyEnabled() const;
    SV_ULONG getMaxMemoryCapForResync() const;
    /* End of the change */

    /**Added by BSR for Upgrade Issue **/
    bool getUpdatedUpgradeToCX() const;
    std::string getUpgradedVersion() const;
    std::string getUpgradePHPPath() const;
    SV_ULONG getUpdateUpgradeWaitTimeSecs() const;

    void setUpdatedUpgradeToCX(const bool bUpdated) const;
    /** End of the change **/
    SV_ULONG getResyncStaleFilesCleanupInterval() const;
    bool ShouldCleanupCorruptSyncFile() const;
    SV_ULONG getResyncUpdateInterval() const;
    SV_ULONG getIRMetricsReportInterval() const;
    SV_ULONG getLogResyncProgressInterval() const;
    SV_ULONG getResyncSlowProgressThreshold() const;
    SV_ULONG getResyncNoProgressThreshold() const;

    std::string getOffloadSyncPathname() const;
    std::string getVsnapConfigPathname() const;
    std::string getVsnapPendingPathname() const;
    bool getDICheck() const;
    bool getSVDCheck() const;
    bool getDirectTransfer() const;
    bool getEnableDiffFileChecksums() const;
    bool CompareInInitialDirectSync() const;
    bool IsProcessClusterPipeEnabled() const;
    SV_UINT getMaxHcdsAllowdAtCx() const;
    SV_UINT getMaxClusterBitmapsAllowdAtCx() const;
    SV_UINT getSecsToWaitForHcdSend() const;
    SV_UINT getSecsToWaitForClusterBitmapSend() const;
    bool getTSCheck() const;
    SV_ULONG getDirectSyncBlockSizeInKB() const;
    bool getDIVerify() const;
    /*to lock/unlock bookmark for vsnap*/
    bool isRetainBookmarkForVsnap() const;
    bool DPPrintPerfCounters() const;
    bool DPProfileSourceIo() const;
    bool DPProfileVolRead() const;
    bool DPProfileVolWrite() const;
    bool DPProfileCdpWrite() const;

    std::string getApplicationConsistentExcludedVolumes() const;
    std::string getTargetChecksumsDir() const;
    std::string getOffloadSyncSourceDirectory() const;
    std::string getOffloadSyncCacheDirectory() const;
    std::string getOffloadSyncFilenamePrefix() const;
    std::string getDiffTargetExePathname() const;
    std::string getDiffTargetSourceDirectoryPrefix() const;
    std::string getDiffTargetCacheDirectoryPrefix() const;
    std::string getDiffTargetFilenamePrefix() const;
    std::string getFastSyncExePathname() const;
    int getFastSyncBlockSize() const;
    int getFastSyncMaxChunkSize() const;
    int getFastSyncMaxChunkSizeForE2A() const;
    int getUncompressRetries() const;
    int getUncompressRetryInterval() const;
    SV_ULONGLONG getMinDiskFreeSpaceForUncompression() const;
    bool shouldIgnoreCorruptedDiffs() const;
    bool getUseConfiguredHostname() const;
    void setUseConfiguredHostname(bool flag) const;
    bool getUseConfiguredIpAddress() const;
    void setUseConfiguredIpAddress(bool flag) const;
    std::string getConfiguredHostname() const;
    void setConfiguredHostname(const std::string &hostName) const;
    std::string getConfiguredIpAddress() const;
    void setConfiguredIpAddress(const std::string &ipAddress) const;
    std::string getExternalIpAddress() const;
    int getFastSyncHashCompareDataSize() const;
    std::string getResyncSourceDirectoryPath() const;
    unsigned int getMaxFastSyncApplyThreads() const;
    int getSyncBytesToApplyThreshold(std::string const& vol) const;
    bool getChunkMode() const;
    bool getHostType() const;
    std::string getFabricWorldWideName() const;
    std::string getConsistencyOptions() const;
    int getMaxOutpostThreads() const;
    int getVolumeChunkSize() const;
    bool getRegisterSystemDrive() const;
    void setCacheDirectory(std::string const& value) const;
    void setDiffTargetCacheDirectoryPrefix(std::string const& value) const;
    void setHttp(HTTP_CONNECTION_SETTINGS s) const;
    void setHostName(std::string const& value) const;
    void setPort(int port) const;
    void setMaxOutpostThreads(int n) const;
    void setVolumeChunkSize(int n) const;
    void setRegisterSystemDrive(bool flag) const;

    //Added for fabric Logging requirement see Bug#6625 for details
    int getRemoteLogLevel() const;
    void setRemoteLogLevel(int remoteLogLevel) const;
    bool RenameDrconf(std::string const & oldName, std::string const & newName)const;
    bool CopyDrconfFile(std::string const & SourceFile, std::string const & DestinationFile)const;
    std::string getTimeStampsOnlyTag()const;
    std::string getDestDir()const;
    std::string getDatExtension()const;
    std::string getMetaDataContinuationTag()const;
    std::string getMetaDataContinuationEndTag()const;
    std::string getWriteOrderContinuationTag()const;
    std::string getWriteOrderContinuationEndTag()const;
    std::string getPreRemoteName()const;
    std::string getFinalRemoteName()const;
    int getThrottleWaitTime()const;
    int getS2DataWaitTime()const;
    int getSentinelExitTime() const;
    int getSentinelExitTimeV2() const;
    int getWaitForDBNotify() const;
    int getAzureBlobOperationsTimeout() const;
    std::string getProtectedVolumes() const;
    void setProtectedVolumes(std::string protectedVolumes) const;
    std::string getInstallPath() const;
    std::string getAgentInstallPathOnCsPrimeApplianceToAzure() const;
    std::string getPSInstallPathOnCsPrimeApplianceToAzure() const;
    std::string getPSTelemetryFolderPathOnCsPrimeApplianceToAzure() const;
    std::string getLogFolderPathOnCsPrimeApplianceToAzure() const; // TODO: rename to getPSLogFolderPathOnCsPrimeApplianceToAzure
    std::string getAgentRole() const;
    bool isMasterTarget() const;
    bool isMobilityAgent() const;
    bool shouldReportScsiIdAsDevName() const;

    int getCxUpdateInterval() const;
    int getCxCDPDiskUsageUpdateInterval() const;
    bool CanDeleteAllCxCDPPendingUpdates() const;
    int getDeleteAllCxCDPPendingUpdatesInterval() const;
    bool getIsCXPatched() const;
    std::string getProfileDeviceList() const;
    unsigned int getMaxDirectSyncFlushToRetnSize() const;
    unsigned int getMaxVacpServiceThreads() const;
    unsigned int getVacpToCxDelay() const;
    int getVolumeRetryDelay() const;
    int getVolumeRetries() const;
    int getDirectSyncPartitions() const;
    SV_UINT getDirectSyncPartitionSize() const;
    int getEnforcerDelay() const;
    void setCxUpdateInterval(int interval) const;
    SV_ULONG getMinCacheFreeDiskSpacePercent() const;
    void setMinCacheFreeDiskSpacePercent(SV_ULONG percent) const;
    SV_ULONGLONG getMinCacheFreeDiskSpace() const;
    void setMinCacheFreeDiskSpace(SV_ULONG space) const;
    SV_ULONGLONG getCMMinReservedSpacePerPair() const;
    int getVirtualVolumesId() const;
    void  setVirtualVolumesId(int id) const;
    std::string getVirtualVolumesPath(std::string key) const;
    void setVirtualVolumesPath(std::string key, std::string value) const;
    int getIdleWaitTime() const;
    unsigned long long getVsnapId() const;
    SV_ULONGLONG getVsnapWriteDataLength() const;
    void setVsnapId(unsigned long long snapId) const;
    unsigned long long getLowLastSnapshotId() const;
    void setLowLastSnapshotId(unsigned long long snapId) const;
    unsigned long long getHighLastSnapshotId() const;
    void setHighLastSnapshotId(unsigned long long snapId) const;
    SV_ULONG getMaxRunsPerInvocation() const;
    SV_ULONG getMaxMemoryUsagePerReplication() const;
    SV_ULONG getMaxInMemoryCompressedFileSize() const;
    SV_ULONG getMaxInMemoryUnCompressedFileSize() const;
    SV_ULONG getCompressionChunkSize() const;
    SV_ULONG getCompressionBufSize() const;
    SV_ULONG getSequenceCount() const;
    SV_ULONG getSequenceCountInMsecs() const;
    SV_ULONG getRetentionBufferSize() const;
    int getLocalLogSize() const;
    bool trackExperimentalDeviceNumbers() const;
    std::string getHdlmDlnkmgr() const;
    std::string getScliPath() const;
    std::string getPowermtCmd() const;
    std::string getGrepCmd() const;
    std::string getAwkCmd() const;
    std::string getVxDmpCmd() const;
    std::string getVxDiskCmd() const;
    std::string getVxVsetCmd() const;
    std::string getScdidadmCmd() const;
    std::string getVxDmpPath() const;
    unsigned int getLinuxNumPartitions() const;
    std::string getMpxioDrvNames() const;
    std::string getEmlxAdmPath() const;
    std::string getVMCmds() const;
    std::string getVMCmdsForPats() const;
    std::string getVMPats() const;
    bool useLinuxDeviceTxt() const;
    std::string getLinuxDiskDiscoveryCommand() const;
    int getCdpPolicyCheckInterval() const;
    int getCdpFreeSpaceCheckInterval() const;

    SV_UINT getPendingDataReporterInterval() const;

    bool enforceStrictConsistencyGroups() const;
    int getDelayBetweenAppShutdownAndTagIssue() const;
    int getMaxWaitTimeForTagArrival() const;
    bool getApplicationFailoverChkDskEnabled() const;
    //Added by Ranjan bug#10404( Xen Registration)
    int getMaxWaitTimeForXenRegistration() const;
    int getMaxWaitTimeForLvActivation() const;
    int getMaxWaitTimeForDisplayVmVdiInfo() const;
    int getMaxWaitTimeForCxStatusUpdate() const;
    //End of change
    void setDelayBetweenAppShutdownAndTagIssue(int delayBetweenAppShutdownAndTagIssue) const;
    void setMaxWaitTimeForTagArrival(int noOfSecondsToWaitForTagArrival) const;
    SV_UINT getPendingVsnapReporterInterval() const;
    SV_UINT getNumberOfBatchRequestToCX() const;

    int getMaxRetryAttemptsToShutdownVXAgent() const;

    unsigned long long GetMaxSpacePerCdpDataFile() const;
    bool isCdpDataFilePreAllocationEnabled() const;
    bool isVsnapLocalPersistenceEnabled() const;
    unsigned long long GetMaxTimeRangePerCdpDataFile() const;
    std::string getAgentMode() const;
    void setAgentMode(std::string mode) const;

    bool registerClusterInfoEnabled() const;
    bool monitorVolumesEnabled() const;
    bool reportFullDeviceNamesOnly() const;
    std::string getNotAllowedMountPointFileName()const;
    std::string getConsistencySettingsCachePath() const;
    std::string getResyncBatchCachePath() const;
    std::string GetSourceAgentConfigPath() const;

    SV_UINT getManualResyncStartThresholdInSecs() const;
    SV_UINT getInitialReplicationStartThresholdInSecs() const;
    SV_UINT getAutoResyncStartThresholdInSecs() const;
    std::string getCacheMgrExePathname() const;
    std::string getCdpcliExePathname() const;
    int getCacheMgrExitTime() const;
    SV_ULONGLONG getMaxDiskUsagePerReplication() const;
    SV_UINT getNWThreadsPerReplication() const;
    SV_UINT getIOThreadsPerReplication() const;
    SV_UINT getCMRetryDelayInSeconds() const;
    SV_UINT getCMMaxRetries() const;
    SV_UINT getCMIdleWaitTimeInSeconds() const;
    SV_UINT AllowOutOfOrderSeq() const;
    SV_UINT AllowOutOfOrderTS() const;
    SV_UINT IgnoreOutOfOrder() const;
    int getEnforcerAlertInterval() const;
    int getDataprotectionExitTime() const;
    int getNotifyCxDiffsInterval() const;
    int getSnapshotInterval() const;
    int getAgentHealthCheckInterval() const;
    int getMarsHealthCheckInterval() const;
    int getMarsServerUnavailableCheckInterval() const;
    int getRegisterHostInterval() const;
    int getTransportErrorLogInterval() const;
    int getDiskReadErrorLogInterval() const;
    int getDiskNotFoundErrorLogInterval() const;
    int getSrcTelemetryPollInterval() const;
    int getSrcTelemetryStartDelay() const;
    int getMonitorHostStartDelay() const;
    int getMonitorHostInterval() const;
    int getRcmDetailsPollInterval() const;
    std::string getMonitorHostCmdList() const;
    std::string getRecoveryCleanupFileList() const;
    void setRecoveryCleanupFileList(const std::string &cleanupList) const;
    void addToRecoveryCleanupFileList(const std::string &cleanupFile) const;
    std::string getAzureServicesAccessCheckCmd() const;
    std::string getAzureServices() const;
    int getInitialSettingCallInterval() const;
    int getSettingsCallInterval() const;
    bool allowRootVolumeForRetention() const;
    bool IsCdpcliSkipCheck() const;
    int getConsistencyTagIssueTimeLimit() const;

    SV_ULONG    getDriverDppRamUsageInPercent() const;
    SV_ULONG    getDriverMinDppUsageInMB() const;
    SV_ULONG    getDriverMaxDppUsageInMB() const;
    SV_ULONG    getDriverDppAlignmentInMB() const;

    //Added by BSR for parallelising HCD Process threads
    SV_UINT getMaxFastSyncProcessThreads() const;
    SV_UINT getMaxClusterProcessThreads() const;
    SV_UINT getMaxFastSyncGenerateHCDThreads() const;
    //End of the change

    SV_UINT getMaxGenerateClusterBitmapThreads() const;
    bool AsyncOpEnabled() const;
    bool AsyncOpEnabledForPhysicalVolumes() const;
    bool useNewApplyAlgorithm() const;
    SV_ULONG MaxAsyncIos() const;
    SV_ULONGLONG getMaxMemoryForDiffSyncFile() const;
    SV_ULONGLONG getMaxMemoryForResyncFile() const;
    bool useUnBufferedIo() const;
    bool CDPCompressionEnabled() const;
    void SetCDPCompression(bool compress) const;
    SV_UINT GetCDPCompression() const;
    bool VirtualVolumeCompressionEnabled() const;
    void SetVirtualVolumeCompression(bool compression) const;
    bool DPBMAsynchIo() const;
    bool DPBMAsynchIoForPhysicalVolumes() const;
    bool DPBMCachingEnabled() const;
    unsigned int DPBMCacheSize() const;
    bool DPBMUnBufferedIo() const;
    unsigned int DPBMBlockSize() const;
    unsigned int DPBMBlocksPerEntry() const;
    bool DPBMCompressionEnabled() const;
    unsigned int DPBMMaxIos() const;
    unsigned int DPBMMaxMemForIo() const;
    unsigned int DPDelayBeforeExitOnError() const;

    SV_UINT GetMaximumDiskIndex() const;
    SV_UINT GetMaximumConsMissingDiskIndex() const;
    SV_UINT GetDiskRecoveryWaitTime() const;


    SV_UINT getMaximumWMIConnectionTimeout() const;
    SV_UINT getMaxSupportedPartitionsCountUEFIBoot() const;

    std::string getCdpMgrExePathname() const;
    int getCDPMgrExitTime() const;

    bool isCDPReadAheadCacheEnabled() const;
    unsigned int CDPReadAheadThreads() const;
    unsigned int CDPReadAheadFileCount() const;
    unsigned CDPReadAheadLength() const;


    bool getCMVerifyDiff() const;
    bool getCMPreserveBadDiff() const;
    SV_ULONGLONG getMaxCdpv3CowFileSize() const;
    bool SimulateSparse() const;
    bool TrackExtraCoalescedFiles() const;
    bool TrackCoalescedFiles() const;
    void setRepositoryName(const std::string& repositoryName) const;
    std::string getRepositoryName() const;
    void unsetRepositoryLocation();
    void setRepositoryLocation(const std::string& repoLocation) const;
    //end of change
    ESERIALIZE_TYPE getSerializerType() const;
    void setSerializerType(ESERIALIZE_TYPE) const;
    void setHostId(const std::string& hostId) const;
    void setResourceId(const std::string& resourceId) const;
    void setSourceGroupId(const std::string& sourceGroupId) const;
    void setFailoverVmDetectionId(const std::string& failoverVmId) const;
    void setIsAzureVm(bool bAzureVM) const;
    void setSourceControlPlane(const std::string& sourceControlPlane) const;
    void setFailoverVmBiosId(const std::string& failoverVmBiosId) const;
    void setFailoverTargetType(const std::string& failoverTargetType) const;

    bool getSystemUUID(std::string& uuid) const;
    bool getHypervisorName(std::string& hypervisor) const;
    void setSystemUUID(const std::string& uuid) const;
    void setHypervisorName(const std::string& uuid) const;
    SV_UINT getSentinalStartStatus() const;
    SV_UINT getDataProtectionStartStatus() const;
    SV_UINT getCDPManagerStartStatus() const;
    SV_UINT getCacheManagerStartStatus() const;
    bool canEditCatchePath() const;
    SV_ULONGLONG getSizeOfReservedRetentionSpace() const;
    SV_UINT getCdpMaxIOSize() const;
    SV_UINT getCdpMaxSnapshotIOSize() const;

    bool getCDPMgrUpdateCxPerTargetVolume() const;
    SV_ULONG getCDPMgrEventTimeRangeRecordsPerBatch() const;
    bool getCDPMgrSendUpdatesAtOnce() const;
    bool getCDPMgrDeleteUnusableRecoveryPoints() const;
    bool getCDPMgrDeleteStaleFiles() const;

    /* make it allocatable type size_t instead
     * of long long */
    SV_ULONGLONG getVxAlignmentSize() const;

    SV_UINT getVolpackSparseAttribute() const;
    bool DPCacheVolumeHandle() const;
    SV_UINT DPMaxRetentionFileHandlesToCache() const;
    std::string getHostAgentName() const;
    SV_UINT getMaxUnmountRetries() const;
    bool shouldDiscoverOracle() const;
    bool useFSAwareSnapshotCopy() const;
    SV_ULONGLONG getMaxMemForReadingFSBitmap() const;
    SV_ULONGLONG getCdpLowSpaceTriggerPercentage() const;
    SV_ULONGLONG getCdpLowSpaceTriggerLowerThreshold() const;
    SV_ULONGLONG getCdpLowSpaceTriggerUpperThreshold() const;
    SV_UINT getCdpRedoLogMaxFileSize() const;
    bool IsFlushAndHoldWritesRetryEnabled() const;
    bool IsFlushAndHoldResumeRetryEnabled() const;

    SV_UINT GetFullBackupInterval() const;
    void SetFullBackupInterval(SV_UINT intervalInSec);
    void SetFullBackupRequired(bool required = true);
    void SetLastFullBackupTimeInGmt(SV_ULONGLONG gmt);
    SV_ULONGLONG GetLastFullBackupTimeInGmt() const;
    bool IsFullBackupRequired() const;

    SV_LONGLONG MaxDifferenceBetweenFSandRawSize() const;

    /* churn-throughput CX session definitions start */
    SV_ULONGLONG getMaxDiskChurnSupportedMBps() const;
    SV_ULONGLONG getMaxVMChurnSupportedMBps() const;
    SV_ULONGLONG getMaximumTimeJumpForwardAcceptableInMs() const;
    SV_ULONGLONG getMaximumTimeJumpBackwardAcceptableInMs() const;
    SV_ULONGLONG getMinConsecutiveTagFailures() const;
    SV_ULONGLONG getMaxS2LatencyBetweenCommitDBAndGetDB() const;
    SV_ULONGLONG getMaxWaitTimeForHealthEventCommitFailureInSec() const;

    // This counter is used as follows
    // Whenever a health CX is reported by driver
    // It is assigned a weight (ref count)
    // In every health reporting these events are reported
    // and ref count is decremented.
    // Once ref count reaches 0 it is cleared.
    // This is used to compensate the issue that
    // Cx Notification doesnt comes when cx event is cleared.
    SV_ULONGLONG getCxClearHealthCount() const;

    /* churn-throughput CX session definitions end */

    int getMonitoringCxpsClientTimeoutInSec() const;
    SV_ULONGLONG getPausePendingAckRepeatIntervalInSecs() const;
    SV_UINT getLogContainerRenewalRetryTimeInSecs() const;

    /*Rename failure configurations*/
    SV_UINT getRenameFailureRetryIntervalInSec() const;
    SV_UINT getRenameFailureRetryCount() const;

public:
    //
    // ConfigureVxTransport
    //
    HTTP_CONNECTION_SETTINGS getHttp() const;

    std::string getDiffSourceDirectory() const;
    std::string getDiffTargetDirectory() const;
    std::string getPrefixDiffFilename() const;
    std::string getResyncDirectories() const;
    std::string getSslClientFile() const;
    std::string getSslKeyPath() const;
    std::string getSslCertificatePath() const;
    std::string getCfsLocalName() const;
    int getTcpSendWindowSize() const;
    int getTcpRecvWindowSize() const;
    bool get_curl_verbose() const;
    bool ignoreCurlPartialFileErrors() const;
    int getTransportMaxBufferSize() const;
    int getTransportConnectTimeoutSeconds() const;
    int getTransportResponseTimeoutSeconds() const;
    int getTransportLowSpeedTimeoutSeconds() const;
    int getTransportWriteMode() const;
    bool IsFilterDriverAvailable() const;
    bool IsVolpackDriverAvailable() const;
    bool IsVsnapDriverAvailable() const;
    bool IsHttps() const;


    SV_ULONGLONG getSparseFileMaxSize()const;
    SV_UINT getDoNotRunDiskPart() const;
    std::string getArchiveToolPath() const;
    SV_UINT getS2ReIncarnationInterval() const;
    std::string getRepositoryLocation() const;
    bool isGuestAccess() const;
    void setGuestAccess(bool isGuestAccess);
    void setHttps(const bool &isHttps);
public:
    //
    // For reading vxagent.account section
    //
    bool getAccountInfo(std::map<std::string, std::string> &namevaluepairs) const;

protected:
    //
    // Helper methods
    //
    static std::string dirname_r(const std::string &name, const char &separator_char = ACE_DIRECTORY_SEPARATOR_CHAR_A);
    std::string get(std::string const& section, std::string const& key) const;
    std::string get(std::string const& section, std::string const& key, std::string const& defaultValue) const;
    std::string get(std::string const& section, std::string const& key, int defaultValue) const;
    void getSection(std::string const& section, std::map<std::string, std::string> &namevaluepairs) const;
    bool getNameValuesInSection(const std::string &section, std::map<std::string, std::string> &namevaluepairs) const;
    void set(std::string const& section, std::string const& key, std::string const& value) const;
    void remove(std::string const& section, std::string const& key) const;

public:
    void setCsAddressForAzureComponents(std::string const& address) const;
    std::string getCsAddressForAzureComponents() const;

private:
    //
    // State
    //
    mutable ACE_Configuration_Heap m_inifile; // ACE not totally const-correct
    //Replaced ACE_Process_Semaphore with SVSemaphore to fix Bug #4124
    //Replaced SVSemaphore with ACE_File_Lock to fix Bug #10321
    mutable ACE_File_Lock m_lockInifile;
    typedef ACE_Guard<ACE_File_Lock> AutoGuardFileLock;
    typedef ACE_Guard<ACE_Recursive_Thread_Mutex> AutoGuardRThreadMutex;
public:
    std::string getSMRInfoFromSMRConf(std::string& section, std::string& key);

public:

    std::string pushInstallPath() const;
    bool isVmWarePushInstallationEnabled() const;
    std::string pushLogFolder() const;
    std::string pushTmpFolder() const;

    HTTP_CONNECTION_SETTINGS pushHttpSettings() const;
    bool pushHttps() const;
    std::string pushHostid() const;

    int pushLogLevel() const;
    void pushLogLevel(int level);

    bool pushSignVerificationEnabled() const;
    bool pushSignVerificationEnabledOnRemoteMachine() const;

    std::string pushInstallTelemetryLogsPath() const;
    std::string agentTelemetryLogsPath() const;

    std::string pushLocalRepositoryPathUA() const;
    std::string pushLocalRepositoryPathPushclient() const;
    std::string pushLocalRepositoryPathPatch() const;
    std::string pushRemoteRootFolderWindows() const;
    std::string pushRemoteRootFolderUnix() const;
    std::string pushOsScriptPath() const;
    std::string pushVmwareApiWrapperCmd() const;

    int pushJobTimeoutSecs() const;
    int pushVmwareApiWrapperCmdTimeOutSecs() const;
    int pushJobFetchIntervalInSecs() const;
    int pushJobRetries() const;
    int pushJobRetryIntervalInsecs() const;
    int pushJobCSRetries() const;
    int pushJobCSRetryIntervalInsecs() const;

    int getLastNLinesCountToReadFromLogOrBuffer() const;

    //For OMS Statistics and DR Metrics Collection
    int getOMSStatsCollInterval() const;
    int getOMSStatsSendingIntervalToPS() const;
    int getDRMetricsCollInterval() const;

    // get Config Server type
    std::string getCSType() const;
    // set Config Server type
    void setCSType(std::string csType) const;

    // get the interval in seconds at which the replicaiton settings to be fetched
    uint32_t    getReplicationSettingsFetchInterval() const;

    // get the number job worker threads to use
    uint32_t getRcmJobWorkerThreadCount() const;
    uint32_t getRcmJobMaxAllowedTimeInSec() const;

	// get the cloud pairing status 
	std::string getCloudPairingStatus() const;

	// set the cloud pairing status 
	void setCloudPairingStatus(const std::string& status) const;

    uint32_t getPeerRemoteResyncStateParseRetryCount() const;

    uint32_t getPeerRemoteResyncStateMonitorInterval() const;

    uint32_t getMaxResyncBatchSize() const;

    bool validateAgentInstallerChecksum() const;
    uint32_t getUpdateDirectoryCreationRetryCount() const;
    uint32_t getUpdateDirectoryCreationRetryInterval() const;
    uint32_t getUpdateDirectoryDeletionRetryCount() const;
    uint32_t getUpdateDirectoryDeletionRetryInterval() const;
    uint32_t getInstallerDownloadRetryCount() const;
    int getInstallerUnzipRetryCount() const;
    uint32_t getInstallerUnzipRetryInterval() const;
    bool getIsCredentialLessDiscovery() const;
    void setIsCredentialLessDiscovery(const bool& isCredentialLessDiscovery) const;
    void setSwitchApplianceState(SwitchAppliance::State state) const;
    SwitchAppliance::State getSwitchApplianceState() const;
    void setVacpState(VacpConf::State state) const;
    VacpConf::State getVacpState() const;
    void setMigrationState(Migration::State state) const;
    Migration::State getMigrationState() const;

    std::string getAdditionalInstallPaths() const;
public:
    // Exposed default values for usage in other libraries that doesn't have
    // to depend on the config file to be read successfully.
    static const int DEFAULT_MAX_WMI_CONNECTION_TIMEOUT_SEC;

    // get the init mode
    static FileConfiguratorMode getInitMode();

    static void setGlobalEvtCollForwParams(
        const std::map<std::string, std::string> & evtcollforwPropertiesOnAppliance)
    {
        boost::mutex::scoped_lock guard(s_lockEvtcollforwInputsOnAppliance);
        s_evtcollforwPropertiesOnAppliance = evtcollforwPropertiesOnAppliance;
    }

	/* START CS Prime appliance Replication To Azure Agent*/
private:
	// indicates if the fileconfigurator is used with VxAgent or CS Prime appliance mode
	static FileConfiguratorMode	s_initmode;
    static std::string s_telemetryFolderPathOnCsPrimeApplianceToAzure;
    static std::map<std::string, std::string> s_globalTunablesOnCsPrimeApplianceToAzure;
    static std::map<std::string, std::string> s_propertiesOnCsPrimeApplianceToAzure;
    static std::map<std::string, std::string> s_evtcollforwPropertiesOnAppliance;
    static boost::mutex s_lockRcmInputsOnCsPrimeApplianceToAzure;
    static boost::mutex s_lockEvtcollforwInputsOnAppliance;

protected:
	// set the init mode
	static void setInitMode(FileConfiguratorMode mode);

    // initializes the in memory copy of the config params
    void initConfigParamsOnCsPrimeApplianceToAzureAgent();

    static void setTelemetryFolderPathOnCsPrimeApplianceToAzure(
        const std::string & telemetryFolderPathOnCsPrimeApplianceToAzure)
    {
        boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
        s_telemetryFolderPathOnCsPrimeApplianceToAzure = telemetryFolderPathOnCsPrimeApplianceToAzure;
    }

    static std::string getTelemetryFolderPathOnCsPrimeApplianceToAzure()
    {
        boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
        return s_telemetryFolderPathOnCsPrimeApplianceToAzure;
    }

    static void setGlobalTunablesOnCsPrimeApplianceToAzure(
        const std::map<std::string, std::string> & globalTunablesOnCsPrimeApplianceToAzure)
    {
        boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
        s_globalTunablesOnCsPrimeApplianceToAzure = globalTunablesOnCsPrimeApplianceToAzure;
    }

    static void setPropertiesOnCsPrimeApplianceToAzure(
        const std::map<std::string, std::string> & propertiesOnCsPrimeApplianceToAzure)
    {
        boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
        s_propertiesOnCsPrimeApplianceToAzure = propertiesOnCsPrimeApplianceToAzure;
    }

private:
    // Retrieves MachineIdentifier in config on CS Prime appliance
    // Note: "MachineIdentifier" is only available in main config in "ConfigPath" on CS Prime appliance.
    //std::string getMachineIdentifierOnCsPrimeApplianceToAzureAgent() const; // TODO: remove as not required

	// gets a config param on CS Prime appliance
	std::string getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, const std::string& key) const;

	// gets a config param section on CS Prime appliance
	void getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, std::map<std::string, std::string> &map) const;

    // gets a config param from evtcollforw properties
    std::string getEvtCollForwParam(const std::string key) const;

	// in memory copy of the local conf params
	boost::property_tree::ptree	m_pt_local_confparams;

	/* END CS Prime appliance Replication To Azure Agent*/

public:
    // section for s2 timeout failure configurables
    SV_ULONGLONG getAzureBlobOperationMaximumTimeout() const;
    SV_ULONGLONG getAzureBlobOperationMinimumTimeout() const;
    SV_UINT getAzureBlobOperationTimeoutResetInterval() const;

    //section for New Health Model's Health Collator directory path
    std::string getHealthCollatorDirPath() const;

};

bool PersistVirtualVolumes(std::string path, const char* volumename);
bool IncrementAndGetVsnapId(unsigned long long &VsnapId);

#endif // FILECONFIGURATOR_H
