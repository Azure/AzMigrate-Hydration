
#ifndef CONFIGUREVXAGENT__H
#define CONFIGUREVXAGENT__H

#include <string>
#include <vector>
#include <map>
#include "sigslot.h"
#include "hostagenthelpers_ported.h"	// SV_HOST_AGENT_TYPE, SV_LOG_LEVEL
#include "portablehelpers.h"
#include "volumegroupsettings.h"
#include "initialsettings.h"
#include "cdpsnapshotrequest.h"
#include "atconfigmanagersettings.h"
#include "retentioninformation.h"
#include "serializationtype.h"
#include "inmalert.h"

#define RESYNC_REPORTED_BY_MT 0x80790000ul

class SourceAgentProtectionPairHealthIssues;
struct ConfigureVxTransport;

struct ConfigureLocalVxAgent {

    virtual std::string getCacheDirectory() const = 0;
    virtual std::string getHostId() const = 0;
    virtual std::string getResourceId() const = 0;
    virtual std::string getSourceGroupId() const = 0;
    virtual int getMaxDifferentialPayload() const = 0;
    virtual SV_UINT getFastSyncReadBufferSize() const = 0;
    virtual SV_HOST_AGENT_TYPE getAgentType() const = 0;
    virtual ~ConfigureLocalVxAgent() {}
    virtual std::string getLogPathname() const = 0;
    virtual SV_LOG_LEVEL getLogLevel() const = 0;
    virtual void setLogLevel(SV_LOG_LEVEL logLevel) const = 0;
    virtual std::string getDiffSourceExePathname() const = 0;
    virtual std::string getDataProtectionExePathname() const = 0;
    /* Added by BSR 
    *  Project : New Sync
    *  Start */
    virtual std::string getDataProtectionExeV2Pathname() const = 0 ;
    /* End */

	virtual int getS2StrictMode() const = 0 ;
	virtual unsigned int getRepeatingAlertIntervalInSeconds() const = 0 ;
    virtual void insertRole(std::map<std::string, std::string> &m) const = 0;
    virtual int getAgentHealthCheckInterval() const = 0;
    virtual std::string getHealthCollatorDirPath() const = 0;
	virtual int getMarsHealthCheckInterval() const = 0;
	virtual int getMarsServerUnavailableCheckInterval() const = 0;
	virtual int getRegisterHostInterval() const = 0 ;
    virtual int getTransportErrorLogInterval() const = 0;
    virtual int getDiskReadErrorLogInterval() const = 0;
    virtual int getDiskNotFoundErrorLogInterval() const = 0;
    virtual int getMonitorHostStartDelay() const = 0;
    virtual int getMonitorHostInterval() const = 0;
    virtual std::string getMonitorHostCmdList() const = 0;
	virtual bool registerLabelOnDisks() const = 0 ;
	virtual bool compareHcd() const = 0 ;
	virtual unsigned int DirectSyncIOBufferCount() const = 0 ;
	virtual SV_ULONGLONG getVxAlignmentSize() const = 0 ;
    virtual bool pipelineReadWriteInDirectSync() const = 0;
	virtual bool getShouldS2RenameDiffs() const = 0 ;
	virtual bool ShouldProfileDirectSync(void) const = 0 ;
    virtual long getTransportFlushThresholdForDiff() const = 0 ;
    virtual int getProfileDiffs() const = 0 ;
    virtual std::string ProfileDifferentialRate() const = 0 ;
    virtual SV_UINT ProfileDifferentialRateInterval() const = 0 ;
	virtual SV_UINT getLengthForFileSystemClustersQuery() const = 0 ;
    virtual bool getAccountInfo(std::map<std::string, std::string> &namevaluepairs) const = 0;
	virtual unsigned int getWaitTimeForSrcLunsValidity() const = 0 ;
    virtual unsigned int getSourceReadRetries() const = 0 ;
    virtual bool getZerosForSourceReadFailures() const = 0 ;
    virtual unsigned int getSourceReadRetriesInterval() const = 0 ;
	virtual unsigned long int getExpectedMaxDiffFileSize() const = 0 ;
	virtual unsigned int getPendingChangesUpdateInterval() const = 0 ;
    virtual bool shouldIssueScsiCmd() const = 0 ;
	virtual std::string getCxData() const = 0 ;
	virtual int getLogFileXfer() const = 0 ;
	virtual size_t getMetadataReadBufLen() const = 0 ;
    virtual unsigned long getMirrorResyncEventWaitTime() const = 0;
    virtual int getEnableVolumeMonitor() const = 0 ;
    virtual bool getDICheck() const = 0;
    virtual bool getSVDCheck() const = 0;
    virtual bool getDirectTransfer() const = 0;
	virtual bool CompareInInitialDirectSync() const = 0 ;
	virtual bool IsProcessClusterPipeEnabled() const = 0 ;
	virtual SV_UINT getMaxHcdsAllowdAtCx() const = 0 ;
	virtual SV_UINT getMaxClusterBitmapsAllowdAtCx() const = 0 ;
	virtual SV_UINT getSecsToWaitForHcdSend() const = 0 ;
	virtual SV_UINT getSecsToWaitForClusterBitmapSend() const = 0 ;
	virtual bool getTSCheck() const = 0 ;
	virtual SV_ULONG getDirectSyncBlockSizeInKB() const = 0 ;
	virtual std::string getTargetChecksumsDir() const = 0 ;
    virtual std::string getOffloadSyncPathname() const = 0;
    virtual std::string getOffloadSyncSourceDirectory() const = 0;
    virtual std::string getOffloadSyncCacheDirectory() const = 0;
    virtual std::string getOffloadSyncFilenamePrefix() const = 0;
    virtual std::string getDiffTargetExePathname() const = 0;
    virtual std::string getDiffTargetSourceDirectoryPrefix() const = 0;
    virtual std::string getDiffTargetCacheDirectoryPrefix() const = 0;
    virtual std::string getDiffTargetFilenamePrefix() const = 0;
    virtual std::string getFastSyncExePathname() const = 0;
    virtual int getFastSyncBlockSize() const = 0;
    virtual int getFastSyncMaxChunkSize() const = 0;
	virtual int getFastSyncMaxChunkSizeForE2A() const = 0;
    virtual bool getUseConfiguredHostname() const = 0;
    virtual void setUseConfiguredHostname(bool flag) const = 0;
    virtual bool getUseConfiguredIpAddress() const = 0;
    virtual void setUseConfiguredIpAddress(bool flag) const = 0;
    virtual std::string getConfiguredHostname() const = 0;
    virtual void setConfiguredHostname(const std::string &hostName) const = 0;
    virtual std::string getConfiguredIpAddress() const = 0;
    virtual bool isMobilityAgent() const = 0;
    virtual bool isMasterTarget() const = 0;
    virtual void setConfiguredIpAddress(const std::string &ipAddress) const = 0;
    virtual std::string getExternalIpAddress() const = 0;
    virtual int getFastSyncHashCompareDataSize() const = 0;
    virtual std::string getResyncSourceDirectoryPath() const = 0;
    virtual unsigned int getMaxFastSyncApplyThreads() const = 0;
    virtual int getSyncBytesToApplyThreshold( std::string const& vol) const = 0;
    virtual bool getChunkMode() const = 0;
    virtual bool getHostType() const = 0;
    virtual int getMaxOutpostThreads() const = 0;
    virtual int getVolumeChunkSize() const = 0;
    virtual bool getRegisterSystemDrive() const = 0;
    virtual void setCacheDirectory(std::string const& value) const = 0;
    virtual void setDiffTargetCacheDirectoryPrefix(std::string const& value) const = 0;
    virtual void setHttp(HTTP_CONNECTION_SETTINGS s) const = 0;
    virtual void setHostName(std::string const& value) const = 0;
    virtual void setPort(int port) const = 0;
    virtual void setMaxOutpostThreads(int n) const = 0;
    virtual void setVolumeChunkSize(int n) const = 0;
    virtual void setRegisterSystemDrive(bool flag) const=0;
    virtual int getRemoteLogLevel() const = 0;
    virtual void setRemoteLogLevel(int remoteLogLevel) const = 0;
    virtual std::string getTimeStampsOnlyTag()const = 0;
    virtual std::string getDestDir() const = 0;
    virtual std::string getDatExtension() const = 0;
    virtual std::string getMetaDataContinuationTag() const = 0;
    virtual std::string getMetaDataContinuationEndTag() const = 0;
    virtual std::string getWriteOrderContinuationTag() const = 0;
    virtual std::string getWriteOrderContinuationEndTag() const = 0;
    virtual std::string getPreRemoteName() const = 0;
    virtual std::string getFinalRemoteName() const = 0;
    virtual int getThrottleWaitTime()const = 0;
    virtual int getSentinelExitTime()const = 0;
    virtual int getS2DataWaitTime()const = 0;
    virtual int getWaitForDBNotify() const = 0;
    virtual std::string getProtectedVolumes() const = 0;
    virtual void setProtectedVolumes(std::string protectedVolumes) const = 0;
    virtual std::string getInstallPath() const = 0;
	virtual std::string getAgentRole() const = 0;
    virtual int getCxUpdateInterval() const = 0;
    virtual bool getIsCXPatched() const = 0;
    virtual std::string getProfileDeviceList() const = 0;
    virtual int getEnforcerDelay() const = 0;
    virtual int getVolumeRetries() const = 0;
    virtual int getVolumeRetryDelay() const = 0;
    virtual void setCxUpdateInterval(int interval) const = 0;
    virtual SV_ULONG getMinCacheFreeDiskSpacePercent() const = 0;
    virtual void setMinCacheFreeDiskSpacePercent(SV_ULONG percent) const = 0;
    virtual SV_ULONGLONG getMinCacheFreeDiskSpace() const = 0;
    virtual void setMinCacheFreeDiskSpace(SV_ULONG space) const = 0;
	virtual SV_ULONGLONG getCMMinReservedSpacePerPair() const = 0;
    virtual int getVirtualVolumesId() const = 0;
    virtual void  setVirtualVolumesId(int id) const = 0;
    virtual std::string getVirtualVolumesPath(std::string key) const = 0;
    virtual void setVirtualVolumesPath(std::string key, std::string value) const = 0;
    virtual int getIdleWaitTime() const = 0;
    virtual unsigned long long getVsnapId() const = 0;
	virtual void setVsnapId(unsigned long long vsnapId)const = 0;
    virtual unsigned long long getLowLastSnapshotId() const = 0;
    virtual void setLowLastSnapshotId(unsigned long long snapId) const = 0;
    virtual unsigned long long getHighLastSnapshotId() const = 0;
    virtual void setHighLastSnapshotId(unsigned long long snapId) const = 0;
    virtual SV_ULONG getMaxRunsPerInvocation() const = 0;
    virtual SV_ULONG getMaxMemoryUsagePerReplication() const = 0;
    virtual SV_ULONG getMaxInMemoryCompressedFileSize() const = 0;
    virtual SV_ULONG getMaxInMemoryUnCompressedFileSize() const = 0;
    virtual SV_ULONG getCompressionChunkSize() const = 0;
    virtual SV_ULONG getCompressionBufSize() const = 0;
    virtual SV_ULONG getSequenceCount() const = 0;
    virtual SV_ULONG getSequenceCountInMsecs() const = 0;
    virtual bool enforceStrictConsistencyGroups() const = 0;
    virtual SV_ULONG getRetentionBufferSize() const = 0;
    
    //Added by BSR for bitmap resync
    virtual SV_ULONG getResyncStaleFilesCleanupInterval() const = 0;
    virtual bool ShouldCleanupCorruptSyncFile() const = 0;
    virtual SV_ULONG getResyncUpdateInterval() const = 0 ;
    //End of the change
    virtual SV_ULONG getIRMetricsReportInterval() const = 0;
    virtual SV_ULONG getLogResyncProgressInterval() const = 0;
    virtual SV_ULONG getResyncSlowProgressThreshold() const = 0;
    virtual SV_ULONG getResyncNoProgressThreshold() const = 0;

    // APIs For parameters required for Application Failover
    virtual int getDelayBetweenAppShutdownAndTagIssue() const = 0;
    virtual int getMaxWaitTimeForTagArrival() const = 0;
    virtual void setDelayBetweenAppShutdownAndTagIssue(int delayBetweenAppShutdownAndTagIssue) const = 0;	
    virtual void setMaxWaitTimeForTagArrival(int noOfSecondsToWaitForTagArrival) const = 0;
    //Added by Ranjan bug#10404 (Xen Registration)
    virtual int getMaxWaitTimeForXenRegistration() const = 0;
    virtual int getMaxWaitTimeForLvActivation() const = 0;
	virtual int getMaxWaitTimeForDisplayVmVdiInfo() const = 0;
	virtual int getMaxWaitTimeForCxStatusUpdate() const = 0;
    //end of change

    //Added by BSR for parallelising HCD Processing
    virtual SV_UINT getMaxFastSyncProcessThreads() const = 0 ;
    virtual SV_UINT getMaxFastSyncGenerateHCDThreads() const = 0 ;
    virtual SV_UINT getMaxClusterProcessThreads() const = 0 ;
    //End of the change
    virtual SV_UINT getMaxGenerateClusterBitmapThreads() const = 0 ;
    virtual bool IsVsnapDriverAvailable() const = 0;
	virtual bool IsVolpackDriverAvailable() const = 0;
    virtual ESERIALIZE_TYPE getSerializerType() const = 0;
	virtual void setSerializerType(ESERIALIZE_TYPE) const = 0 ;
	virtual void setHostId(const std::string&) const = 0;
	virtual void setResourceId(const std::string&) const = 0;
    virtual SV_UINT getSentinalStartStatus() const = 0;
    virtual SV_UINT getDataProtectionStartStatus() const = 0;
    virtual SV_UINT getCDPManagerStartStatus() const = 0;
    virtual SV_UINT getCacheManagerStartStatus() const = 0;
    virtual bool canEditCatchePath() const = 0;

    virtual int getMonitoringCxpsClientTimeoutInSec() const = 0;

	virtual std::string getMTSupportedDataPlanes() const = 0;
	virtual SV_ULONGLONG getMinAzureUploadSize() const = 0;
	virtual unsigned int getMinTimeGapBetweenAzureUploads() const = 0;
	virtual unsigned int getTimeGapBetweenFileArrivalCheck() const = 0;
	virtual unsigned int getMaxAzureAttempts() const = 0;
	virtual unsigned int getAzureRetryDelayInSecs() const = 0;
	virtual unsigned int getAzureImplType() const = 0;
};

struct ConfigureVxAgent : ConfigureLocalVxAgent {

    virtual ~ConfigureVxAgent() {}

    // drill down
    // getReplicationPairs();
    // getReplicationPairManager();
    // getVolumeGroups();
    // getVolumes();
    // getVolumeManager();
    // getSnapshotManager();
    virtual ConfigureVxTransport& getVxTransport() = 0;
    // getRetentionManager();
    // setHostInfo( hostname, ipaddress, os, agentVersion, driverVersion );
    // updateAgentLog( string );
    // LOG_LEVEL getDebugLogLevel();

    virtual JOB_ID_OFFSET getVolumeCheckpoint( std::string const & drivename ) const = 0;

    // callbacks
    enum ThrottledAgent { ThrottleResync, ThrottleDiffs, ThrottleTarget };
    sigslot::signal3<ThrottledAgent,std::string const&,bool> throttle;

    // vx actions
    virtual bool updateAgentLog( std::string const& timestamp,std::string const& loglevel,
        std::string const& agentInfo,std::string const& errorString ) const = 0;
    virtual void renameClusterGroup( std::string const& clusterName, 
        std::string const& oldGroup, std::string const& newGroup) const = 0;
    virtual void deleteVolumesFromCluster( std::string const& groupName, 
        std::vector<std::string> const& volumes ) const = 0;
    virtual void deleteClusterNode( std::string const& clusterName ) const = 0;
    virtual void addVolumesToCluster( std::string const& groupName,
        std::vector<std::string> const& volumes, 
        std::string const& groupGuid ) const = 0;
    /* Added by BSR for Fast Sync TBC */
    virtual int setResyncProgress( std::string deviceName, long long offset, long long bytes, bool matched, std::string jobId, std::string const oldName, std::string newName, std::string deleteName1, std::string deleteName2, std::string agent ) const = 0;

    virtual int setResyncProgressFastsync( std::string const &sourceHostId,
        std::string const &sourcedeviceName, 
        std::string const &destHostId,
        std::string const &destDeviceName,
        long long offset, 
        long long bytes, 
        bool matched, 
        std::string jobId, 
        std::string const oldName, 
        std::string newName, 
        std::string deleteName1, 
        std::string deleteName2 ) const = 0;
   
   /*
   *
   * Added following two functions for fixing overshoot problem using BitMap 
   * By Suresh
   * Change Start
   */
    virtual int SetFastSyncUpdateProgressBytes(std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        SV_ULONGLONG bytesApplied,
        SV_ULONGLONG bytesMatched,
        std::string jobId) const = 0;
    virtual int SetFastSyncUpdateProgressBytesWithStats(std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        const SV_ULONGLONG bytesApplied,
        const SV_ULONGLONG bytesMatched,
        const std::string &jobId,
        const std::string &stats) const = 0;
    virtual int SetFastSyncUpdateMatchedBytes( std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        SV_ULONGLONG bytesMatched,
		int during_resyncTransition,
        std::string jobId) const = 0;
    virtual int SetFastSyncFullyUnusedBytes( std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        SV_ULONGLONG bytesunused,
        std::string jobId) const = 0;
    virtual int SetFastSyncUpdateFullSyncBytes(std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        SV_ULONGLONG fullSyncBytesSent,
        std::string jobId) const = 0;
	//Change End

    virtual JOB_ID_OFFSET getVolumeCheckpointFastsync( std::string const &sourceHostId,
        std::string const & sourceDeviceName,
        std::string const &destHostId, 
        std::string const &destDeviceName ) const = 0;

    virtual int setResyncTransitionStepOneToTwo( std::string const &sourceHostId,
        std::string const & sourceDeviceName,
        std::string const &destHostId, 
        std::string const &destDeviceName,
        const std::string& jobId,
        std::string const &syncNoMoreFile ) const = 0 ;

    virtual ResyncTimeSettings getResyncStartTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId ) = 0 ;

    virtual int sendResyncStartTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId, 
        const SV_ULONGLONG & ts, 
        const SV_ULONG & seq ) = 0;

    virtual ResyncTimeSettings getResyncEndTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId ) = 0 ;

    virtual int sendResyncEndTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId, 
        const SV_ULONGLONG & ts, 
        const SV_ULONG & seq ) = 0;

    /* End of the change */

    /* Added by BSR for NEW SYNC Project 
    virtual void updateMatchedBytes( std::string jobid, long long bytes ) const = 0 ;
    virtual void updateSyncedBytes( std::string jobid, long long bytes ) const = 0 ;
    End of the change */ 
    virtual int setLastResyncOffsetForDirectSync(const std::string & sourceHostId,
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId,
        const SV_ULONGLONG &offset,
		const SV_ULONGLONG &filesystemunusedbytes) = 0;

    virtual void ReportAgentHealthStatus(const SourceAgentProtectionPairHealthIssues& agentHealth) = 0;
	virtual void ReportMarsAgentHealthStatus(const std::list<long>& marsHealth) = 0;
    virtual std::string getInitialSyncDirectories() const = 0;
    virtual std::string getTargetReplicationJobId( std::string deviceName ) const = 0;
    virtual bool shouldThrottleResync(std::string const& deviceName, const std::string &endpointdeviceName, const int &grpid) const=0;
    virtual bool shouldThrottleSource(std::string const& deviceName) const=0;
    virtual bool shouldThrottleTarget(std::string const& deviceName) const=0;
	virtual bool setTargetResyncRequired(const std::string &deviceName, const std::string &errStr, const ResyncReasonStamp &resyncReasonStamp, const long errorcode = RESYNC_REPORTED_BY_MT) const = 0;
    virtual bool setSourceResyncRequired(const std::string &deviceName, const std::string &errStr, const ResyncReasonStamp &resyncReasonStamp) const = 0;
    virtual bool setPrismResyncRequired( std::string sourceLunId,std::string errStr ) const = 0;
	virtual bool setXsPoolSourceResyncRequired( std::string deviceName,std::string errStr ) const = 0;
	virtual bool pauseReplicationFromHost( std::string deviceName,std::string errStr ) const = 0;
	virtual bool resumeReplicationFromHost( std::string deviceName,std::string errStr ) const = 0;
    virtual bool canClearDifferentials( std::string deviceName ) const = 0;
    virtual bool updateVolumeAttribute(NOTIFY_TYPE notifyType, const std::string & deviceName,VOLUME_STATE volumeState,const std::string & mountPoint = "" ) const =0;
    virtual bool updateLogAvailable(std::string deviceName,std::string dateFrom, std::string dateTo, unsigned long long spaceOccupied, unsigned long long freeSpaceLeft,unsigned long long dateFromUtc, unsigned long long dateToUtc, std::string diffs) const = 0;
	virtual bool updateRetentionTag(std::string deviceName,std::string tagTimeStamp, std::string appName, std::string userTag, std::string actionTag, unsigned short accuracy,std::string identifier,unsigned short verifier,std::string comment) const =0;
	virtual bool updateCDPInformationV2(const HostCDPInformation& cdpmap) const =0;
	virtual bool updateCdpDiskUsage(const HostCDPRetentionDiskUsage&,const HostCDPTargetDiskUsage& ) const = 0;
	//Bug #6298  
	virtual bool updateReplicationStateStatus(const std::string& deviceName, VOLUME_SETTINGS::PAIR_STATE state) const = 0;

	virtual bool updateCleanUpActionStatus(const std::string& deviceName, const std::string & cleanupstr) const = 0;
	virtual bool setPauseReplicationStatus(const std::string& deviceName, int hosttype, const std::string & respstr) const = 0;

	virtual bool updateRestartResyncCleanupStatus(const std::string& deviceName, bool& success, const std::string& error_message) const = 0;

    virtual int getCurrentVolumeAttribute(std::string deviceName) const = 0;
    // virtual SNAPSHOT_REQUESTS getSnapshotRequestFromCx() const = 0;
    virtual bool notifyCxOnSnapshotStatus(const std::string &snapId, int timeval,const SV_ULONGLONG &VsnapId, const std::string &errMessage, int status) const = 0; 
    virtual bool notifyCxOnSnapshotProgress(const std::string &snapId, int percentage, int rpoint) const = 0;
	virtual std::vector<bool> notifyCxOnSnapshotCreation(std::vector<VsnapPersistInfo> const & vsnapInfo) const = 0;
	virtual std::vector<bool> notifyCxOnSnapshotDeletion(std::vector<VsnapDeleteInfo> const & vsnapDeleteInfo) const = 0;
    virtual bool isSnapshotAborted(const std::string & snapshotId) const = 0;
    virtual int makeSnapshotActive(const std::string & snapshotId) const = 0;
    // virtual SNAPSHOT_REQUESTS getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const = 0;

    virtual ResyncTimeSettings getResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const std::string &hostType = "source") = 0;
    virtual int sendResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const std::string &hostType = "source") = 0;
    virtual int updateMirrorState(const std::string &sourceLunID, 
			                      const PRISM_VOLUME_INFO::MIRROR_STATE mirrorStateRequested,
                                  const PRISM_VOLUME_INFO::MIRROR_ERROR errcode,
                                  const std::string &errorString) = 0;
    virtual int sendLastIOTimeStampOnATLUN(const std::string &sourceVolumeName,
                                           const SV_ULONGLONG ts) = 0;
    virtual int updateVolumesPendingChanges(const VolumesStats_t &vss, const std::list<std::string>& statsNotAvailableVolumes) = 0;
    virtual ResyncTimeSettings getResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const std::string &hostType = "source") = 0;
    virtual int sendResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const std::string &hostType = "source") = 0;

    virtual int sendEndQuasiStateRequest(const std::string & volname) const = 0;
    virtual bool sendAlertToCx(const std::string & timeval, const std::string & errLevel, const std::string & agentType, SV_ALERT_TYPE alertType, SV_ALERT_MODULE alertingModule, const InmAlert &alert) const = 0;
    virtual void sendDebugMsgToLocalHostLog(SV_LOG_LEVEL LogLevel, const std::string& szDebugString) const = 0 ;
    virtual VsnapRemountVolumes getVsnapRemountVolumes() const = 0;
    virtual int notifyCxDiffsDrained(const std::string &driveName, const SV_ULONGLONG& bytesAppliedPerSecond) const = 0;
    virtual bool deleteVirtualSnapshot(std::string targetVolume,unsigned long long vsnapid ,int status, std::string message) const = 0;
    virtual bool cdpStopReplication(const std::string & volname, const std::string & cleanupaction) const = 0;
    virtual std::string getVolumeMountPoint(const std::string & volname) const = 0;
    virtual std::string getSourceFileSystem(const std::string & volname) const = 0;
    virtual SV_ULONGLONG getSourceCapacity(const std::string & volname) const = 0;
	virtual SV_ULONGLONG getSourceRawSize(const std::string & volname) const = 0;
    virtual void getVolumeNameAndMountPointForAll(VolumeNameMountPointMap & volumeNameMountPointMap) const = 0;
    virtual void getVolumeNameAndFileSystemForAll(VolumeNameFileSystemMap &volumeNameFileSystemMap )const = 0;
    virtual std::string getVirtualServerName(std::string fieldName) const = 0;
    virtual void registerClusterInfo(const std::string & action, const ClusterInfo & clusterInfo ) const = 0;	
    //virtual void registerXsInfo(const std::string & action, const ClusterInfo & xsInfo,const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const = 0;

	//Added by ranjan (changing from void to bool for return value of registerXsInfo)
	virtual bool registerXsInfo(const std::string & action, const ClusterInfo & xsInfo,const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const = 0;
	
//#ifdef SV_FABRIC
    virtual FabricAgentInitialSettings getFabricServiceSetting()const= 0;
    virtual PrismAgentInitialSettings getPrismServiceSetting()const= 0;
    virtual FabricAgentInitialSettings getFabricServiceSettingOnReboot()const= 0;
    virtual bool updateApplianceLunState(ATLunOperations& atLun)const= 0;
    virtual bool updatePrismApplianceLunState(PrismATLunOperations& atLun)const= 0;
    virtual bool updateDeviceLunState(DeviceLunOperations& deviceLunOperationState)const = 0;
    virtual bool updateGroupInfoListState(AccessCtrlGroupInfo & groupInfo)const = 0;
    virtual bool updateBindingDeviceDiscoveryState(DiscoverBindingDeviceSetting& discoverBindingDeviceSetting)const= 0;
    virtual std::list<TargetModeOperation> getTargetModeSettings() const= 0;
    virtual void updateTargetModeStatus(std::list<TargetModeOperation> tmLunList)const= 0;
    virtual void updateTargetModeStatusOnReboot(std::list<TargetModeOperation> tmLunList)const= 0;
    virtual void SanRegisterInitiators( std::vector<SanInitiatorSummary> const& initiators)const = 0;
    virtual std::string getAgentMode() const = 0;
    virtual void setAgentMode( std::string mode ) const = 0;
//#endif

    //
    // PR # 6277
    // Routines to sync up retention information with remote cx once failover is done
    // 
	virtual HostRetentionWindow getHostRetentionWindow() const = 0;
	virtual bool updateRetentionInfo(const HostRetentionInformation& hRetInfo) const = 0;

    // Routine to update pending data size in cache dir to CX. See # 6899 for details
	virtual bool updatePendingDataInfo(const std::map<std::string,SV_ULONGLONG>& pendingDataInfo) const = 0;

    virtual StorageFailover failoverStorage(std::string const& action, bool migration) const = 0;
    
    virtual void registerIScsiInitiatorName(std::string const& initiatorName) const = 0;
   
	virtual bool EnableVolumeUnprovisioningPolicy(std::list <std::string> &sourceList) const = 0;
	virtual bool MonitorEvents () const = 0;

	virtual SV_UINT updateFlushAndHoldWritesPendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const = 0;
	virtual SV_UINT updateFlushAndHoldResumePendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const = 0;
	virtual FLUSH_AND_HOLD_REQUEST getFlushAndHoldRequestSettings(std::string volumename) const = 0;
	virtual bool AnyPendingEvents() const = 0 ;
	virtual bool BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename) const = 0 ;
	virtual bool BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename)const = 0 ;
};

#endif // CONFIGUREVXAGENT__H

