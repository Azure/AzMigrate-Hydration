//
// configurevxagentproxy.h: what the VxAgent uses to get local & remote settings
//
#ifndef CONFIGUREVXAGENTPROXY__H
#define CONFIGUREVXAGENTPROXY__H

#include <map>
#include "proxy.h"
#include "configurevxagent.h"
#include "localconfigurator.h"
#include "serializationtype.h"
#include "volumegroupsettings.h"

class ACE_Mutex;
struct InitialSettings;

#define HASH_LENGTH 16

class ConfigureVxAgentProxy : public ConfigureVxAgent {
public:
    //
    // constructors/destructors
    //
    ConfigureVxAgentProxy( ConfiguratorDeferredProcedureCall dpc, 
        ConfiguratorMediator& t, LocalConfigurator& cfg,
        ACE_Mutex& mutex, InitialSettings& settings, SNAPSHOT_REQUESTS & snaprequests,
        const SerializeType_t serializetype ) :
    m_serializeType(serializetype),
    m_dpc( dpc ), 
        m_transport( t ),
        m_localConfigurator( cfg ),
        m_lock( mutex ),
        m_settings( settings ),
        m_snapShotRequests( snaprequests )
    {
        memset(m_prevAgentHealthStatusHash, 0, sizeof m_prevAgentHealthStatusHash);
    }

public:
    //
    // ConfigureVxAgent interface
    //
    // getReplicationPairs();
    // getReplicationPairManager();
    // getVolumeGroups();
    // getVolumes();
    // getSnapshotManager();
    ConfigureVxTransport& getVxTransport();
    // getRetentionManager();
    // setHostInfo( hostname, ipaddress, os, agentVersion, driverVersion );
    // updateAgentLog( string );

    std::string getCacheDirectory() const;
    std::string getHostId() const;
    std::string getResourceId() const;
    std::string getSourceGroupId() const;
    int getMaxDifferentialPayload() const;
    SV_UINT getFastSyncReadBufferSize() const;
    std::string getLogPathname() const;
    SV_LOG_LEVEL getLogLevel() const;
    void setLogLevel(SV_LOG_LEVEL logLevel) const;
    SV_HOST_AGENT_TYPE getAgentType() const;
    std::string getDiffSourceExePathname() const;
    std::string getDataProtectionExePathname() const;

    /* Added by BSR for NEW SYNC Project */
    std::string getDataProtectionExeV2Pathname() const ;
    /* End of changed */

    /* Added by BSR for Fast Sync TBC */
    bool getMemoryBasedSyncApplyEnabled() const ;
    SV_ULONG getMaxMemoryCapForResync() const ;
    /* End of the change  */

	int getS2StrictMode() const ;
	unsigned int getRepeatingAlertIntervalInSeconds() const ;
	void insertRole(std::map<std::string, std::string> &m) const ;
	bool registerLabelOnDisks() const ;
	bool compareHcd() const ;
	unsigned int DirectSyncIOBufferCount() const ;
    bool pipelineReadWriteInDirectSync() const;
	SV_ULONGLONG getVxAlignmentSize() const ;
	bool getShouldS2RenameDiffs() const ;
	bool ShouldProfileDirectSync(void) const ;
    long getTransportFlushThresholdForDiff()  const ;
    int getProfileDiffs() const ;
    std::string ProfileDifferentialRate() const ;
    SV_UINT ProfileDifferentialRateInterval() const ;
	SV_UINT getLengthForFileSystemClustersQuery() const ;
    bool getAccountInfo(std::map<std::string, std::string> &namevaluepairs) const ;
	unsigned int getWaitTimeForSrcLunsValidity() const ;
    unsigned int getSourceReadRetries() const ;
    bool getZerosForSourceReadFailures() const ;
    unsigned int getSourceReadRetriesInterval() const ;
	unsigned long int getExpectedMaxDiffFileSize() const ;
	unsigned int getPendingChangesUpdateInterval() const ;
	bool shouldIssueScsiCmd() const ;
	std::string getCxData() const ;
	int getLogFileXfer() const ;
    size_t getMetadataReadBufLen() const ;
    unsigned long getMirrorResyncEventWaitTime() const ;
    int getEnableVolumeMonitor() const ;
    std::string getOffloadSyncPathname() const;
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
    bool getUseConfiguredHostname() const;
    void setUseConfiguredHostname(bool flag) const;
    bool getUseConfiguredIpAddress() const;
    void setUseConfiguredIpAddress(bool flag) const;
    std::string getConfiguredHostname() const;
    void setConfiguredHostname( std::string const & hostName) const;
    std::string getConfiguredIpAddress() const;
    bool isMobilityAgent() const;
    bool isMasterTarget() const;
    void setConfiguredIpAddress(std::string const & ipAddress) const;
    std::string getExternalIpAddress() const;
    int getFastSyncHashCompareDataSize() const;
    std::string getResyncSourceDirectoryPath() const;
    unsigned int getMaxFastSyncApplyThreads() const;
    JOB_ID_OFFSET getVolumeCheckpoint( std::string const & drivename ) const;
    int getSyncBytesToApplyThreshold( std::string const& vol ) const;
    bool getChunkMode() const;
    bool getHostType() const;
    int getMaxOutpostThreads() const;
    int getVolumeChunkSize() const;
    bool getRegisterSystemDrive() const;
    int getAgentHealthCheckInterval() const;
    std::string getHealthCollatorDirPath() const;
    int getMarsHealthCheckInterval() const;
	int getMarsServerUnavailableCheckInterval() const;
    int getRegisterHostInterval() const;
    int getTransportErrorLogInterval() const;
    int getDiskReadErrorLogInterval() const;
    int getDiskNotFoundErrorLogInterval() const;
    int getMonitorHostStartDelay() const;
    int getMonitorHostInterval() const;
    std::string getMonitorHostCmdList() const;
    void setCacheDirectory(std::string const& value) const;
    void setDiffTargetCacheDirectoryPrefix(std::string const& value) const;
    void setHttp(HTTP_CONNECTION_SETTINGS s) const;
    void setHostName(std::string const& value) const;
    void setPort(int port) const;
    void setMaxOutpostThreads(int n) const;
    void setVolumeChunkSize(int n) const;
    void setRegisterSystemDrive(bool flag) const;
    int getRemoteLogLevel() const;
    void setRemoteLogLevel(int remoteLogLevel) const;
    std::string getProtectedVolumes() const;
    void setProtectedVolumes(std::string protectedVolumes) const;
    std::string getInstallPath() const;
	std::string getAgentRole() const;
    int getCxUpdateInterval() const;
    bool getIsCXPatched() const;
    std::string getProfileDeviceList() const;
    int getVolumeRetries() const;
    int getVolumeRetryDelay() const;
    int getEnforcerDelay() const;
    void setCxUpdateInterval(int interval) const;
    SV_ULONG getMaxRunsPerInvocation() const;
    SV_ULONG getMaxMemoryUsagePerReplication() const;
    SV_ULONG getMaxInMemoryCompressedFileSize() const;
    SV_ULONG getMaxInMemoryUnCompressedFileSize() const;
    SV_ULONG getCompressionChunkSize() const;
    SV_ULONG getCompressionBufSize() const;
    SV_ULONG getSequenceCount() const;
    SV_ULONG getSequenceCountInMsecs() const;
    bool enforceStrictConsistencyGroups() const;
    SV_ULONG getRetentionBufferSize() const;
    SV_ULONG getMinCacheFreeDiskSpacePercent() const;
    void setMinCacheFreeDiskSpacePercent(SV_ULONG percent) const;
    SV_ULONGLONG getMinCacheFreeDiskSpace() const;
	SV_ULONGLONG getCMMinReservedSpacePerPair() const;
    void setMinCacheFreeDiskSpace(SV_ULONG space) const;
    int getVirtualVolumesId() const;
    void  setVirtualVolumesId(int id) const;
    std::string getVirtualVolumesPath(std::string key) const;
    void setVirtualVolumesPath(std::string key, std::string value) const;
    int getIdleWaitTime() const;
    unsigned long long getVsnapId() const;
	void setVsnapId(unsigned long long vsnapId)const;
    unsigned long long getLowLastSnapshotId() const;
    void setLowLastSnapshotId(unsigned long long snapId) const;
    unsigned long long getHighLastSnapshotId() const;
    void setHighLastSnapshotId(unsigned long long snapId) const;
    std::string getTestValue() const;

	bool getDICheck() const ;
    bool getSVDCheck() const;
    bool getDirectTransfer() const;
	bool CompareInInitialDirectSync() const ;
	bool IsProcessClusterPipeEnabled() const ;
	SV_UINT getMaxHcdsAllowdAtCx() const ;
	SV_UINT getMaxClusterBitmapsAllowdAtCx() const ;
	SV_UINT getSecsToWaitForHcdSend() const ;
	SV_UINT getSecsToWaitForClusterBitmapSend() const ;
	bool getTSCheck() const ;
	SV_ULONG getDirectSyncBlockSizeInKB() const ;
    std::string getTargetChecksumsDir() const ;

    // APIs For parameters required for Application Failover
    int getDelayBetweenAppShutdownAndTagIssue() const;
    int getMaxWaitTimeForTagArrival() const;

    //Added by ranjan bug#10404 (Xen Registration)
    int getMaxWaitTimeForXenRegistration() const;
    int getMaxWaitTimeForLvActivation() const;
	int getMaxWaitTimeForDisplayVmVdiInfo() const;
	int getMaxWaitTimeForCxStatusUpdate() const;
    //End of change

    void setDelayBetweenAppShutdownAndTagIssue(int delayBetweenAppShutdownAndTagIssue) const;	
    void setMaxWaitTimeForTagArrival(int noOfSecondsToWaitForTagArrival) const;

    // vx actions
    bool updateAgentLog(std::string const& timestamp,std::string const& agentInfo,
        std::string const& loglevel,std::string const& errorString ) const;
    void renameClusterGroup( std::string const& clusterName,                   
        std::string const& oldGroup, std::string const& newGroup ) const;
    void deleteVolumesFromCluster( std::string const& groupName,
        std::vector<std::string> const& volumes ) const;
    void deleteClusterNode( std::string const& clusterName ) const;
    void addVolumesToCluster( std::string const& groupName,
        std::vector<std::string> const& volumes,
        std::string const& groupGuid ) const;
    /*Added by BSR for FastSync TBC */
    int setResyncProgress( std::string deviceName, long long offset, long long bytes, bool matched, std::string jobId, std::string const oldName, std::string newName, std::string deleteName1, std::string deleteName2, std::string agent ) const;

    int setResyncProgressFastsync( std::string const &sourceHostId,
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
        std::string deleteName2 ) const ;

    JOB_ID_OFFSET getVolumeCheckpointFastsync( std::string const &sourceHostId,
        std::string const & sourceDeviceName,
        std::string const &destHostId, 
        std::string const &destDeviceName ) const ;

    int setResyncTransitionStepOneToTwo( std::string const &sourceHostId,
        std::string const & sourceDeviceName,
        std::string const &destHostId, 
        std::string const &destDeviceName,
        const std::string& jobId,
        std::string const &syncNoMoreFile  ) const ;

    ResyncTimeSettings getResyncStartTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId ) ;

    int sendResyncStartTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId, 
        const SV_ULONGLONG & ts, 
        const SV_ULONG & seq ) ;

    ResyncTimeSettings getResyncEndTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId )  ;

    int sendResyncEndTimeStampFastsync( const std::string & sourceHostId, 
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId, 
        const SV_ULONGLONG & ts, 
        const SV_ULONG & seq ) ;
    /* End of the change */
	/*
	*  Added Following functions to fix overshoot issue using bitmap
	*  By Suresh
	*/
	int SetFastSyncUpdateFullSyncBytes(std::string const &sourceHostId,
		std::string const &sourceDeviceName,
		std::string const &destHostId,
		std::string const &destDeviceName,
		SV_ULONGLONG fullSyncBytesSent,
		std::string jobId) const;
	int SetFastSyncUpdateProgressBytes(std::string const &sourceHostId,
		std::string const &sourceDeviceName,
		std::string const &destHostId,
		std::string const &destDeviceName,
		SV_ULONGLONG bytesApplied,
		SV_ULONGLONG bytesMatched,
		std::string jobId) const;
    int SetFastSyncUpdateProgressBytesWithStats(std::string const &sourceHostId,
        std::string const &sourceDeviceName,
        std::string const &destHostId,
        std::string const &destDeviceName,
        const SV_ULONGLONG bytesApplied,
        const SV_ULONGLONG bytesMatched,
        const std::string &jobId,
        const std::string &stats) const;
    int SetFastSyncUpdateMatchedBytes( std::string const& sourceHostId,
        std::string const& sourceDeviceName,
        std::string const& destHostId,
        std::string const& destDeviceName,
        SV_ULONGLONG bytesMatched,
		int during_resyncTransition,
        std::string jobId) const ;
    int SetFastSyncFullyUnusedBytes( std::string const& sourceHostId,
        std::string const& sourceDeviceName,
        std::string const& destHostId,
        std::string const& destDeviceName,
        SV_ULONGLONG bytesunused,
        std::string jobId) const ;

    SV_ULONG getResyncStaleFilesCleanupInterval() const;
    bool ShouldCleanupCorruptSyncFile() const;
    SV_ULONG getResyncUpdateInterval() const ;	
    SV_ULONG getIRMetricsReportInterval() const;
    SV_ULONG getLogResyncProgressInterval() const;
    SV_ULONG getResyncSlowProgressThreshold() const;
    SV_ULONG getResyncNoProgressThreshold() const;
    /* End of the change */

    int setLastResyncOffsetForDirectSync(const std::string & sourceHostId,
        const std::string& sourceDeviceName,
        const std::string& destHostId,
        const std::string& destDeviceName,
        const std::string & jobId,
        const SV_ULONGLONG &offset,
		const SV_ULONGLONG &filesystemunusedbytes);

    std::string getInitialSyncDirectories() const;
    std::string getTargetReplicationJobId( std::string deviceName ) const;
    bool shouldThrottleResync(std::string const& deviceName, const std::string &endpointdeviceName, const int &grpid) const;
    bool shouldThrottleSource(std::string const& deviceName) const;
    bool shouldThrottleTarget(std::string const& deviceName) const;
    bool setTargetResyncRequired(const std::string &deviceName, const std::string &errStr, const ResyncReasonStamp &resyncReasonStamp, const long errorcode = RESYNC_REPORTED_BY_MT) const;
    bool setSourceResyncRequired(const std::string &deviceName, const std::string &errStr, const ResyncReasonStamp &resyncReasonStamp) const;
    bool setPrismResyncRequired( std::string sourceLunId,std::string errStr ) const;
	bool setXsPoolSourceResyncRequired( std::string deviceName,std::string errStr ) const;
	bool pauseReplicationFromHost( std::string deviceName,std::string errStr ) const;
	bool resumeReplicationFromHost( std::string deviceName,std::string errStr ) const;
	bool canClearDifferentials( std::string deviceName ) const;
    bool updateVolumeAttribute(NOTIFY_TYPE notifyType,const std::string &deviceName,VOLUME_STATE volumeState,const std::string &mountPoint= "") const;
    bool updateLogAvailable(std::string deviceName,std::string dateFrom, std::string dateTo, unsigned long long spaceOccupied, unsigned long long freeSpaceLeft,unsigned long long dateFromUtc, unsigned long long dateToUtc, std::string diffs) const ;
    bool updateRetentionTag(std::string deviceName,std::string tagTimeStamp, std::string appName, std::string userTag, std::string actionTag, unsigned short accuracy,std::string identifier,unsigned short verifier,std::string comment) const;
	bool updateCDPInformationV2(const HostCDPInformation& cdpmap)const;
	bool updateCdpDiskUsage(const HostCDPRetentionDiskUsage&,const HostCDPTargetDiskUsage&) const ;


	// Bug #6298
	bool updateReplicationStateStatus(const std::string& deviceName, VOLUME_SETTINGS::PAIR_STATE state)const;
	bool updateCleanUpActionStatus(const std::string& deviceName, const std::string & cleanupstr)const;

	bool updateRestartResyncCleanupStatus(const std::string& deviceName, bool& success, const std::string& error_message) const;

    std::string getTimeStampsOnlyTag()const;
    std::string getDestDir() const;
    std::string getDatExtension() const;
    std::string getMetaDataContinuationTag() const;
    std::string getMetaDataContinuationEndTag() const;
    std::string getWriteOrderContinuationTag() const;
    std::string getWriteOrderContinuationEndTag() const;
    std::string getPreRemoteName() const;
    std::string getFinalRemoteName() const;
    int getThrottleWaitTime()const;
    int getS2DataWaitTime()const;
    int getSentinelExitTime()const;
    int getWaitForDBNotify() const;
    int getCurrentVolumeAttribute(std::string deviceName) const;
    // SNAPSHOT_REQUESTS getSnapshotRequestFromCx() const;
    bool notifyCxOnSnapshotStatus(const std::string &snapId, int timeval,const SV_ULONGLONG & VsnapId, const std::string & errMessage, int status) const; 
    bool notifyCxOnSnapshotProgress(const std::string &snapId, int percentage, int rpoint) const;
	std::vector<bool> notifyCxOnSnapshotCreation(std::vector<VsnapPersistInfo> const & vsnapInfo) const;
	std::vector<bool> notifyCxOnSnapshotDeletion(std::vector<VsnapDeleteInfo> const & vsnapDeleteInfo) const;
    bool isSnapshotAborted(const std::string & snapshotId) const;
    int makeSnapshotActive(const std::string & snapshotId) const;
    // SNAPSHOT_REQUESTS getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const;

    virtual ResyncTimeSettings getResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const std::string &hostType = "source");
    virtual int sendResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const std::string &hostType = "source");
    virtual int updateMirrorState(const std::string &sourceLunID, 
			                      const PRISM_VOLUME_INFO::MIRROR_STATE mirrorStateRequested,
                                  const PRISM_VOLUME_INFO::MIRROR_ERROR errcode,
                                  const std::string &errorString);
    virtual int sendLastIOTimeStampOnATLUN(const std::string &sourceVolumeName,
                                           const SV_ULONGLONG ts);
    virtual int updateVolumesPendingChanges(const VolumesStats_t &vss, const std::list<std::string>& statsNotAvailableVolumes);
    virtual ResyncTimeSettings getResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const std::string &hostType = "source");
    virtual int sendResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts, const SV_ULONG & seq, const std::string &hostType = "source");

    int sendEndQuasiStateRequest(const std::string & volname) const;
    bool sendAlertToCx(const std::string & timeval, const std::string & errLevel, const std::string & agentType, SV_ALERT_TYPE alertType, SV_ALERT_MODULE alertingModule, const InmAlert &alert) const;
    void ReportAgentHealthStatus(const SourceAgentProtectionPairHealthIssues &healthIssues);
	void ReportMarsAgentHealthStatus(const std::list<long>& marsHealth);
    void sendDebugMsgToLocalHostLog(SV_LOG_LEVEL LogLevel, const std::string& szDebugString) const ;
    VsnapRemountVolumes getVsnapRemountVolumes() const;
    int notifyCxDiffsDrained(const std::string &driveName, const SV_ULONGLONG& bytesAppliedPerSecond) const;
    bool deleteVirtualSnapshot(std::string targetVolume,unsigned long long vsnapid , int status, std::string message) const;
    bool cdpStopReplication(const std::string & volname, const std::string & cleanupaction) const ;
	bool setPauseReplicationStatus(const std::string& deviceName, int hosttype, const std::string & respstr) const;

    std::string getVolumeMountPoint(const std::string & volname) const;
    std::string getSourceFileSystem(const std::string & volname) const;
    SV_ULONGLONG getSourceCapacity(const std::string & volname) const;
	SV_ULONGLONG getSourceRawSize(const std::string & volname) const;
    void getVolumeNameAndMountPointForAll(VolumeNameMountPointMap & volumeNameMountPointMap) const;
    void getVolumeNameAndFileSystemForAll(VolumeNameFileSystemMap &volumeNameFileSystemMap )const;
    std::string getVirtualServerName(std::string fieldName) const;
    void registerClusterInfo(const std::string & action, const ClusterInfo & clusterInfo ) const;
    //void registerXsInfo(const std::string & action, const ClusterInfo & xsInfo,const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const;
	
	//Added by ranjan (changing from void to bool for return value of registerXsInfo)
	bool registerXsInfo(const std::string & action, const ClusterInfo & xsInfo,const VMInfos_t & vmInfos, const VDIInfos_t & vdiInfos, int isMaster) const;

//#ifdef SV_FABRIC
    FabricAgentInitialSettings getFabricServiceSetting()const;
    PrismAgentInitialSettings getPrismServiceSetting()const;
    FabricAgentInitialSettings getFabricServiceSettingOnReboot()const;
    bool updateApplianceLunState(ATLunOperations& atLun)const;
    bool updatePrismApplianceLunState(PrismATLunOperations& atLun)const;
    bool updateDeviceLunState(DeviceLunOperations& deviceLunOperationState)const;
    bool updateGroupInfoListState(AccessCtrlGroupInfo & groupInfo)const;
    bool updateBindingDeviceDiscoveryState(DiscoverBindingDeviceSetting& discoverBindingDeviceSetting)const;
    std::list<TargetModeOperation> getTargetModeSettings() const;
    void updateTargetModeStatus(std::list<TargetModeOperation> tmLunList)const;
    void updateTargetModeStatusOnReboot(std::list<TargetModeOperation> tmLunList)const;
    void SanRegisterInitiators( std::vector<SanInitiatorSummary> const& initiators)const;
    std::string getAgentMode() const;
    void setAgentMode( std::string mode ) const;
//#endif

    //
    // PR # 6277
    // Routines to sync up retention information with remote cx once failover is done
    // 
    HostRetentionWindow getHostRetentionWindow() const;
    bool updateRetentionInfo(const HostRetentionInformation& hRetInfo) const;

    // Routine to update pending data size in cache dir to CX. See # 6899 for details
	bool updatePendingDataInfo(const std::map<std::string,SV_ULONGLONG>& pendingDataInfo) const;

    //Added by BSR for parallelising HCD Processing
    SV_UINT getMaxFastSyncProcessThreads() const ;
    SV_UINT getMaxFastSyncGenerateHCDThreads() const ;
    SV_UINT getMaxClusterProcessThreads() const ;
    //End of the change
    SV_UINT getMaxGenerateClusterBitmapThreads() const ;

    virtual StorageFailover failoverStorage(std::string const& action, bool migration) const;    
   
    virtual void registerIScsiInitiatorName(std::string const& initiatorName) const;
   
	virtual bool IsVsnapDriverAvailable() const;
	virtual bool IsVolpackDriverAvailable() const;
    virtual bool EnableVolumeUnprovisioningPolicy(std::list <std::string> &source) const;
	virtual bool MonitorEvents () const;
    virtual ESERIALIZE_TYPE getSerializerType() const;
	virtual void setSerializerType(ESERIALIZE_TYPE) const;
	virtual void setHostId(const std::string& hostId) const ;
	virtual void setResourceId(const std::string& resourceId) const ;
    virtual SV_UINT getSentinalStartStatus() const;
    virtual SV_UINT getDataProtectionStartStatus() const;
    virtual SV_UINT getCDPManagerStartStatus() const;
    virtual SV_UINT getCacheManagerStartStatus() const;
    virtual bool canEditCatchePath() const;

    virtual int getMonitoringCxpsClientTimeoutInSec() const;

	virtual SV_UINT updateFlushAndHoldWritesPendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const;
	virtual SV_UINT updateFlushAndHoldResumePendingStatus(std::string volumename,bool status,std::string errmsg, SV_INT error_num) const;
	virtual FLUSH_AND_HOLD_REQUEST getFlushAndHoldRequestSettings(std::string volumename) const;
	virtual bool AnyPendingEvents() const ;
	virtual bool BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename) const ;
	virtual bool BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename)const ;

	//
	// routines for MT azure data plane support

	std::string getMTSupportedDataPlanes() const;
	SV_ULONGLONG getMinAzureUploadSize() const;
	unsigned int getMinTimeGapBetweenAzureUploads() const;
	unsigned int getTimeGapBetweenFileArrivalCheck() const;
	unsigned int getMaxAzureAttempts() const;
	unsigned int getAzureRetryDelayInSecs()const;
	unsigned int getAzureImplType() const;

private:
    SerializeType_t m_serializeType;
    LocalConfigurator& m_localConfigurator;
    ConfiguratorDeferredProcedureCall const m_dpc;
    ConfiguratorMediator const m_transport;
    ACE_Mutex& m_lock;
    InitialSettings& m_settings;
    SNAPSHOT_REQUESTS& m_snapShotRequests;
    unsigned char m_prevAgentHealthStatusHash[HASH_LENGTH];

    typedef ACE_Guard<ACE_Mutex> AutoGuard;
private:
    void setApiAndFirstArg(const char *apiname,
                           const char *dpc, 
                           const char **apitocall,
                           const char **firstargtoapi) const;
};

#endif // CONFIGUREVXAGENTPROXY__H

