//
// configurator2.h: replacement for original configurator
//
#ifndef CONFIGURATOR2_H
#define CONFIGURATOR2_H

#include "svtypes.h"
#include "hostagenthelpers_ported.h"
#include "sigslot.h"

//#ifdef SV_UNIX
#include "localconfigurator.h"
//#endif 

#include "initialsettings.h"
#include "retentionsettings.h"
#include "cdpsnapshotrequest.h"
#include "volumegroupsettings.h"
#include "prismsettings.h"
#include "serializationtype.h"

struct AppConfigurator;

struct HTTP_CONNECTION_SETTINGS;
struct TRANSPORT_CONNECTION_SETTINGS;
struct VOLUME_GROUP_SETTINGS;
struct HOST_VOLUME_GROUP_SETTINGS;
struct ReplicationPairs ;
struct InitialSettings;
struct ViableCx;
struct ReachableCx;

enum ConfigSource { FETCH_SETTINGS_FROM_CX=0, USE_CACHE_SETTINGS_IF_CX_NOT_AVAILABLE=1 ,  USE_ONLY_CACHE_SETTINGS=2, USE_FILE_PROVIDED=3};

//struct CDP_SETTINGS;
struct TalConfigurator
{
    virtual HTTP_CONNECTION_SETTINGS getHttpSettings() = 0;
    // obsolete: not used by tal, so delete once using new configurator
    virtual TRANSPORT_CONNECTION_SETTINGS getTransportSettings(int volGrpId) = 0;
    virtual TRANSPORT_CONNECTION_SETTINGS getTransportSettings(const std::string &deviceName) = 0;

    virtual ~TalConfigurator() {}
};

struct VxAgentConfigurator
{
    virtual std::string getHostId() = 0;

    //
    // PR# 5554
    // cdpcli will use cache initial settings instead of being dependent
    // on cx being available. service gets initialsettings from configurator
    // and caches them locally to disk on
    // 1. service start
    // 2. on any change in the settings
    //
    virtual InitialSettings getInitialSettings() = 0;
    virtual sigslot::signal1<InitialSettings>& getInitialSettingsSignal() = 0;
    virtual sigslot::signal1<InitialSettings>& getConfigurationChangeSignal() = 0;
    virtual bool isProtectedVolume(const std::string & deviceName) = 0;

    virtual HOST_VOLUME_GROUP_SETTINGS getHostVolumeGroupSettings() = 0;
    virtual PRISM_SETTINGS getPrismSettings() = 0;
    virtual sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS>& getHostVolumeGroupSignal() = 0;
    virtual sigslot::signal1<PRISM_SETTINGS>& getPrismSettingsSignal() = 0;
	virtual sigslot::signal2<std::string, std::string>& getRegisterImmediatelySignal() = 0;
    virtual sigslot::signal0<>& getCdpSettingsSignal() = 0;
    virtual sigslot::signal2<LocalConfigurator&, std::string>& getCsAddressForAzureComponentsChangeSignal() = 0;


	virtual std::list<CsJob> getJobsToProcess() = 0;
	virtual void updateJobStatus(CsJob) =  0;

    //virtual SVERROR SetProtectedVolumes(const std::set<std::string> & protectedVolumes)=0;
    //virtual SVERROR GetProtectedVolumes(std::set<std::string>& protectedVolumes)=0;
    virtual int getVolumeAttribute(std::string driveName)= 0;  
    virtual std::string getInitialSyncDirectory(std::string deviceName ) = 0;
    virtual bool isTargetVolume(std::string deviceName) = 0;
    virtual bool isResyncing(std::string deviceName) = 0;
    virtual bool isInResyncStep1(std::string deviceName) = 0;
    virtual int getRpoThreshold(std::string deviceName) = 0;
    virtual SV_ULONGLONG getDiffsPendingInCX(std::string deviceName) = 0;
    virtual SV_ULONGLONG getCurrentRpo(std::string deviceName) = 0;
    virtual SV_ULONGLONG getApplyRate(std::string deviceName) = 0;
    virtual SV_ULONGLONG getResyncCounter(std::string deviceName) = 0;
    virtual std::string getSourceHostIdForTargetDevice(const std::string& deviceName) = 0;
    virtual std::string getSourceVolumeNameForTargetDevice(const std::string& deviceName) = 0;
    virtual VOLUME_SETTINGS::PAIR_STATE getPairState(const std::string & deviceName) = 0;
    virtual bool shouldOperationRun(const std::string & driveName, const std::string & operationName) = 0;
    virtual OS_VAL getSourceOSVal(const std::string & deviceName) = 0;
    virtual bool isSourceFullDevice(const std::string & deviceName) = 0;
    virtual SV_ULONG getOtherSiteCompartibilityNumber(std::string driveName) = 0;
    virtual SNAPSHOT_REQUESTS getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const = 0;
    virtual SNAPSHOT_REQUESTS getSnapshotRequests() = 0;
    virtual std::map<std::string, std::string> getReplicationPairInfo(const std::string & sourceHost) = 0;
    virtual std::map<std::string, std::string> getSourceVolumeDeviceNames(const std::string & targetHost) = 0;

    virtual SV_ULONGLONG getResyncStartTimeStamp(const std::string &deviceName) = 0;
    virtual SV_ULONGLONG getResyncEndTimeStamp(const std::string &deviceName) = 0 ;
    virtual SV_ULONG getResyncStartTimeStampSeq(const std::string &deviceName) = 0;
    virtual SV_ULONG getResyncEndTimeStampSeq(const std::string &deviceName) = 0;
    virtual TRANSPORT_PROTOCOL getProtocol(const std::string &deviceName) = 0;
    virtual bool getSecureMode(const std::string &deviceName) = 0;
    virtual bool isInQuasiState(const std::string &deviceName) = 0;
    virtual bool isResyncRequiredFlag(const std::string &deviceName) = 0;
    virtual SV_ULONGLONG getResyncRequiredTimestamp(const std::string &deviceName) = 0;
    virtual VOLUME_SETTINGS::RESYNCREQ_CAUSE getResyncRequiredCause(const std::string &deviceName) = 0;
	virtual VOLUME_SETTINGS::TARGET_DATA_PLANE getTargetDataPlane(const std::string & devicename) = 0;
	virtual SV_ULONGLONG GetEndpointRawSize(const std::string & devicename) const = 0;
	virtual std::string GetEndpointDeviceName(const std::string & devicename) const = 0;
	virtual std::string GetEndpointHostId(const std::string & devicename) const = 0;
	virtual std::string GetEndpointHostName(const std::string & devicename) const = 0;
	virtual std::string GetResyncJobId(const std::string & devicename) const = 0;
	virtual VolumeSummary::Devicetype GetDeviceType(const std::string & devicename) const = 0;

    //
    // Routines to fetch remote (failover) cx information
    //
    virtual bool remoteCxsReachable() = 0;
    virtual bool getViableCxs(std::list<ViableCx> & viablecxs) = 0;
    virtual SV_ULONG getFailoverTimeout() = 0;
    virtual sigslot::signal1<ReachableCx>& getFailoverCxSignal() = 0;

	virtual TRANSPORT_CONNECTION_SETTINGS getCSTransportSettings(void) = 0;
	virtual VOLUME_SETTINGS::SECURE_MODE getCSTransportSecureMode(void) = 0;
	virtual TRANSPORT_PROTOCOL getCSTransportProtocol(void) = 0;
    virtual ESERIALIZE_TYPE getSerializerType() const = 0;
    virtual std::string getRepositoryLocation() const = 0;


    virtual ~VxAgentConfigurator() {}
};

struct RetentionConfigurator
{
    virtual CDP_SETTINGS getCdpSettings(const std::string &volname) = 0;
    virtual std::string getCdpDbName(const std::string &volname) = 0;
    virtual bool containsRetentionFiles(const std::string & volname) const = 0;

    virtual ~RetentionConfigurator() {}
};
struct AppConfigurator : 
    public TalConfigurator, 
    public VxAgentConfigurator
    // public ThrottleConfigurator removed
{
    virtual void Stop() = 0;
    virtual SVERROR ConfiguratorRegisterHost() = 0;
    virtual std::string getCacheDirectory() const = 0;
    virtual SV_ULONG getFastSyncMaxChunkSize() const = 0;
	virtual SV_ULONG getFastSyncMaxChunkSizeForE2A() const = 0;
    virtual SV_ULONG getFastSyncHashCompareDataSize() const = 0;
    virtual std::string getResyncSourceDirectoryPath() const = 0;
    virtual unsigned getMaxFastSyncApplyThreads() const = 0;
    virtual SV_ULONG getMinCacheFreeDiskSpacePercent() const = 0;
    virtual SV_ULONGLONG getMinCacheFreeDiskSpace() const = 0;
    virtual SV_ULONGLONG getCMMinReservedSpacePerPair() const = 0;
    virtual bool GetRetentionPolicies(std::string vol, CDP_SETTINGS& policy) = 0;
    virtual bool sendResyncStartTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts) = 0;
    virtual bool sendResyncEndTimeStamp(const std::string & volname, const std::string & jobId, const SV_ULONGLONG & ts) = 0;
    virtual bool SendEndQuasiStateRequest(const std::string & volname) = 0;

    virtual std::string getInstallPath() const = 0;

    //get cxupdateinterval
    virtual SV_ULONG getCxUpdateInterval() const =0;

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

    virtual SNAPSHOT_REQUESTS getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) = 0 ;
    virtual bool NotifyCxOnSnapshotProgress(const CDPMessage * msg)= 0;
    virtual SV_UINT makeSnapshotActive(const std::string & snapshotId) = 0;
    virtual bool isSnapshotAborted(const std::string & snapshotId) =0;
    virtual bool IsVsnapDriverAvailable() const = 0;
    virtual bool IsVolpackDriverAvailable() const = 0;

};

struct ConfigureVxAgent;
struct ConfigureReplicationPair;
struct ConfigureReplicationPairManager;
struct ConfigureVolumeManager;
struct ConfigureVolumeGroups;
struct ConfigureVolume;
struct ConfigureSnapshotManager;
struct ConfigureSnapshot;
struct ConfigureClusterManager;
struct ConfigureVxTransport;
struct ConfigureRetentionManager;
struct ConfigureServiceManager;
struct ConfigureService;
struct ConfigureCxAgent;

struct Configurator : public VxAgentConfigurator, public TalConfigurator, public RetentionConfigurator {
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual ConfigureVxAgent& getVxAgent() = 0;
    virtual ConfigureReplicationPair& getReplicationPair( int id ) = 0;
    virtual ConfigureReplicationPairManager& getReplicationPairManager() = 0;
    virtual ConfigureVolumeManager& getVolumeManager() = 0;
    virtual ConfigureVolumeGroups& getVolumeGroups() = 0;
    virtual ConfigureVolume& getVolume( std::string deviceName ) = 0;
    virtual ConfigureSnapshotManager& getSnapshotManager() = 0;
    virtual ConfigureSnapshot& getSnapshot( std::string deviceName ) = 0;
    virtual ConfigureClusterManager& getClusterManager() = 0;
    virtual ConfigureVxTransport& getVxTransport() = 0;
    virtual ConfigureRetentionManager& getRetentionManager() = 0;
    virtual ConfigureServiceManager& getServiceManager() = 0;
    virtual ConfigureService& getService( std::string serviceName ) = 0;
    virtual ConfigureCxAgent& getCxAgent() = 0;
    virtual void GetCurrentSettings() = 0;
    /* This has to get current settings and then
    * do operations based on that */
    virtual void GetCurrentSettingsAndOperate() = 0;
	virtual bool AnyPendingEvents() = 0 ;
	virtual bool BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename) = 0 ;
	virtual bool BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename) = 0 ;
    static bool instanceFlag;
    Configurator(); 
    virtual ~Configurator();
};


bool InitializeConfigurator(Configurator** configurator, 
                            ConfigSource configSource, 
                            const SerializeType_t serializetype,
                            const std::string& cachepath="");

bool InitializeConfigurator(Configurator** configurator, 
                            std::string const& ip, 
                            int port, 
                            std::string const& hostId,
                            const SerializeType_t serializetype = PHPSerialize );

//
// Function: GetConfigurator
// output: previously created configurator instance
// return value: true if configurator instance exists
//               false otherwise
//

bool GetConfigurator(Configurator** configurator);

#endif // CONFIGURATOR2_H


