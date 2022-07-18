//
// rpcconfigurator.h: declare concrete configurator
//
#ifndef RPCCONFIGURATOR_H
#define RPCCONFIGURATOR_H

#include <ace/Thread_Manager.h>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/Event.h>

#include "configurator2.h"
#include "cxproxy.h"
#include "retentionsettings.h"
#include "cdpsnapshotrequest.h"
//#ifdef SV_FABRIC
#include "atconfigmanagersettings.h"
//#endif
#include "apiwrapper.h"
#include "prismsettings.h"
#include "serializationtype.h"
#include "sourcesystemconfigsettings.h"

class RpcConfigurator : public Configurator 
{
public:
    //
    // constructor/destructor
    //
    // PR#5554
    // if useCachedSettings is true, get the initial settings from local persistent store
    RpcConfigurator(const SerializeType_t serializetype, 
        ConfigSource configSource = FETCH_SETTINGS_FROM_CX,
        const std::string& cachepath="");

    RpcConfigurator( const SerializeType_t serializetype, std::string const& ipAddress, int port, 
        std::string const& hostId);

    //#15949 - Constructor which takes file as an argument. Used for getting volume settings
    //from a given file.
    RpcConfigurator(const SerializeType_t serializetype, std::string const& fileName);                     
    // todo: create another constructor that takes a ConfigureCxAgent object
    ~RpcConfigurator();

public:
    // 
    // VxAgentConfigurator interface
    //
    virtual std::string getHostId();

    // PR#5554
    // getInitialSettings and getInitialSettingsSignal have been added
    // for service. service persist the initial settings to a local store on
    //  1. service start
    //  2. on any change in the initial settings
    // cdpcli will use the local cached settings instead of relying on CX
    //
    virtual InitialSettings getInitialSettings();
    virtual sigslot::signal1<InitialSettings>& getInitialSettingsSignal();
    virtual sigslot::signal1<InitialSettings>& getConfigurationChangeSignal();

	virtual std::list<CsJob> getJobsToProcess();
	virtual void updateJobStatus(CsJob);

    virtual HOST_VOLUME_GROUP_SETTINGS getHostVolumeGroupSettings();
    virtual PRISM_SETTINGS getPrismSettings();
    virtual sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS>& getHostVolumeGroupSignal();
    virtual sigslot::signal1<PRISM_SETTINGS>& getPrismSettingsSignal();
	virtual sigslot::signal2<std::string, std::string>& getRegisterImmediatelySignal();

    virtual sigslot::signal0<>& getCdpSettingsSignal();
    virtual sigslot::signal2<LocalConfigurator&, std::string>& getCsAddressForAzureComponentsChangeSignal();


    //virtual bool shouldThrottle() const;
    virtual int getVolumeAttribute(std::string driveName);
    //virtual bool CheckThrottle(std::string deviceName,THRESHOLD_AGENT thresholdAgent,
    //    THROTTLE_STATUS* isThrottled);
    virtual std::string getInitialSyncDirectory(std::string driveName);
    virtual bool isProtectedVolume(const std::string & deviceName);
    virtual bool isTargetVolume(std::string deviceName);
    virtual bool isResyncing(std::string driveName);
    virtual bool isInResyncStep1(std::string driveName);
    virtual int getRpoThreshold(std::string deviceName);
    virtual SV_ULONGLONG getDiffsPendingInCX(std::string deviceName);
    virtual SV_ULONGLONG getCurrentRpo(std::string deviceName);
    virtual SV_ULONGLONG getApplyRate(std::string deviceName);
    virtual SV_ULONGLONG getResyncCounter(std::string deviceName);
    virtual std::string getSourceHostIdForTargetDevice(const std::string& deviceName);
    virtual std::string getSourceVolumeNameForTargetDevice(const std::string& deviceName);
    virtual VOLUME_SETTINGS::PAIR_STATE getPairState(const std::string & deviceName);
    virtual bool shouldOperationRun(const std::string & driveName, const std::string & operationName);
    virtual OS_VAL getSourceOSVal(const std::string &deviceName);
    virtual bool isSourceFullDevice(const std::string & deviceName);
    virtual SV_ULONG getOtherSiteCompartibilityNumber(std::string driveName);
    virtual SNAPSHOT_REQUESTS getSnapshotRequests(const std::string & vol, const svector_t & bookmarks) const;
    virtual SNAPSHOT_REQUESTS getSnapshotRequests();
    virtual std::map<std::string, std::string> getReplicationPairInfo(const std::string & sourceHost);
    virtual std::map<std::string, std::string> getSourceVolumeDeviceNames(const std::string & targetHost);

    //
    // Routines to fetch remote (failover) cx information
    //
    virtual bool remoteCxsReachable();
    virtual bool getViableCxs(ViableCxs & viablecxs);
    virtual SV_ULONG getFailoverTimeout();
    virtual sigslot::signal1<ReachableCx>& getFailoverCxSignal();
    virtual bool gettargetvolumes(std::vector<std::string>&);

    virtual SV_ULONGLONG getResyncStartTimeStamp(const std::string &deviceName);
    virtual SV_ULONGLONG getResyncEndTimeStamp(const std::string &deviceName) ;
    virtual SV_ULONG getResyncStartTimeStampSeq(const std::string &deviceName);
    virtual SV_ULONG getResyncEndTimeStampSeq(const std::string &deviceName);
    virtual TRANSPORT_PROTOCOL getProtocol(const std::string &deviceName);
    virtual bool getSecureMode(const std::string &deviceName);
    virtual bool isInQuasiState(const std::string &deviceName);
    virtual bool isResyncRequiredFlag(const std::string &deviceName);
    virtual SV_ULONGLONG getResyncRequiredTimestamp(const std::string &deviceName);
    virtual VOLUME_SETTINGS::RESYNCREQ_CAUSE getResyncRequiredCause(const std::string &deviceName);

	virtual VOLUME_SETTINGS::TARGET_DATA_PLANE getTargetDataPlane(const std::string & devicename);
	virtual SV_ULONGLONG GetEndpointRawSize(const std::string & devicename) const;
	virtual std::string GetEndpointDeviceName(const std::string & devicename) const;
	virtual std::string GetEndpointHostId(const std::string & devicename) const;
	virtual std::string GetEndpointHostName(const std::string & devicename) const;
	virtual std::string GetResyncJobId(const std::string & devicename) const;
	virtual VolumeSummary::Devicetype GetDeviceType(const std::string & devicename) const;

	virtual TRANSPORT_CONNECTION_SETTINGS getCSTransportSettings(void);
	virtual VOLUME_SETTINGS::SECURE_MODE getCSTransportSecureMode(void);
	virtual TRANSPORT_PROTOCOL getCSTransportProtocol(void);
    virtual std::string getRepositoryLocation() const;
    virtual ESERIALIZE_TYPE getSerializerType() const;


public:
    //
    // TalConfigurator interface
    //
    virtual HTTP_CONNECTION_SETTINGS getHttpSettings();
    virtual void updateHttpSettings(const HTTP_CONNECTION_SETTINGS&);
    virtual TRANSPORT_CONNECTION_SETTINGS getTransportSettings(int volGrpId);
    virtual TRANSPORT_CONNECTION_SETTINGS getTransportSettings(const std::string &deviceName);

public:
    //
    // RetentionConfigurator interface
    //
    virtual CDP_SETTINGS getCdpSettings(const std::string & volname);
    virtual std::string getCdpDbName(const std::string & volname);
    virtual bool containsRetentionFiles(const std::string & volname) const;
public:
    //
    // Configurator interface
    //
    void Start();
    void Stop();
    ConfigureVxAgent& getVxAgent();
    ConfigureReplicationPair& getReplicationPair( int id );
    ConfigureReplicationPairManager& getReplicationPairManager();
    ConfigureVolumeManager& getVolumeManager();
    ConfigureVolumeGroups& getVolumeGroups();
    ConfigureVolume& getVolume( std::string deviceName );
    ConfigureSnapshotManager& getSnapshotManager();
    ConfigureSnapshot& getSnapshot( std::string deviceName );
    ConfigureClusterManager& getClusterManager();
    ConfigureVxTransport& getVxTransport();
    ConfigureRetentionManager& getRetentionManager();
    ConfigureServiceManager& getServiceManager();
    ConfigureService& getService( std::string serviceName );
    ConfigureCxAgent& getCxAgent();
    void GetCurrentSettings();
    void GetCurrentSettingsAndOperate();
	bool AnyPendingEvents() ;
	bool BackupAgentPause(CDP_DIRECTION direction, const std::string& devicename)  ;
	bool BackupAgentPauseTrack(CDP_DIRECTION direction, const std::string& devicename)  ;
    /*
    public:
    inline SerializeType_t GetSerializeType(void)
    {
    return m_serializeType;
    } 
    */

protected:
    SerializeType_t m_serializeType;
    LocalConfigurator m_localConfigurator;	// must be before m_cx
    ApiWrapper m_apiWrapper;
    CxProxy m_cx;
    ACE_Thread_Manager m_threadManager;
    static ACE_THR_FUNC_RETURN ThreadFunc( void* arg );
    ACE_THR_FUNC_RETURN run();
    void getCurrentInitialSettings();
    // PR# 5554
    // get the settings from local persistent store
    InitialSettings getCachedInitialSettings(const std::string& cachepathfile);
    // #15949
    InitialSettings getInitialSettingsFromFile();
    void updateSettings(const InitialSettings &);

protected:
    //
    // State
    //
    InitialSettings m_settings;
    time_t m_lastTimeCacheUpdated;
    SNAPSHOT_REQUESTS m_snapShotRequests;
    sigslot::signal1<HOST_VOLUME_GROUP_SETTINGS> m_HostVolumeGroupSettingsSignal;
    sigslot::signal1<PRISM_SETTINGS> m_PrismSettingsSignal;
	sigslot::signal2<std::string, std::string> m_RegisterImmediatelySignal;
    sigslot::signal1<InitialSettings> m_InitialSettingsSignal;
    sigslot::signal1<InitialSettings> m_ConfigurationChangeSignal;
    sigslot::signal1<ReachableCx> m_FailoverCxSignal;
    sigslot::signal0<> m_CdpsettingsChangeSignal;
    sigslot::signal2<LocalConfigurator&, std::string> m_CsAddressForAzureComponentsChangeSignal;


private:
    bool isCxReachable(const std::string& ip, const SV_UINT& port);


private:
    mutable ACE_Mutex m_lockSettings;
    typedef ACE_Guard<ACE_Mutex> AutoGuard;
    ACE_Event* m_QuitEvent;
    // PR# 5554
    // m_useCachedSettings is set to true if we want to fetch
    // the initialsettings from local store
    //exclusively use cached settings
    ConfigSource m_configSource;
    ACE_Time_Value m_lastTimeSettingsFetchedFromCx;

    ReachableCx m_ReachableCx;
    bool m_started;
    //15949: Store the volume settings filename provided.
    std::string m_fileName;
};

#endif // RPCCONFIGURATOR_H

