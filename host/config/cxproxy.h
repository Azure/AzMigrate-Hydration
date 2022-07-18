//
// cxproxy.h: turns procedure calls on the CX interface into calls on transport
//
#ifndef CONFIGURATORCXPROXY__H
#define CONFIGURATORCXPROXY__H

#include <string>
#include <cstring>
#include "proxy.h"
#include "snapshotmanagerproxy.h"
#include "configurecxagent.h"
#include "configurevxagentproxy.h"
#include "configurevolumemanagerproxy.h"
#include "datacacher.h"
#include "serializeinitialsettings.h"
#include "serializevolumegroupsettings.h"
#include "serializecdpsnapshotrequest.h"
#include "configureswitchagentproxy.h"
#include "serializeatconfigmanagersettings.h"
#include "serializeswitchinitialsettings.h"
#include "localconfigurator.h"
#include "serializationtype.h"

#define GETVXAGENTAPI "::getVxAgent"
#define HASH_LENGTH 16

/* Maybe CxProxy should be used through interface rather than directly.
   struct ICxInterface {
   boost::function<void ()> RegisterHost;
   boost::function<InitialSettings ()> getInitialSettings;
   boost::function<CxSnapshotManager ()> getSnapshotManager;
   };
 */
/*
   "RegisterHost", void(), 
   "getInitialSettings", InitialSettings(),
   "getSnapshotManager", CxSnapshotManager(),
   -> ICxInterface object
   -> Proxy<ICxInterface> specialization implementation
 */

class CxProxy : public ConfigureCxAgent {
    public:
        //
        // constructor/destructor
        //
        CxProxy(std::string hostId,
                ConfiguratorMediator t,LocalConfigurator& cfg,
                ACE_Mutex& mutex, InitialSettings& settings, SNAPSHOT_REQUESTS& snaprequests ,
                const SerializeType_t serializetype ) :
            m_serializeType(serializetype),
            m_transport( t ),
            m_hostId( hostId ),
            m_localConfigurator( cfg ),
            m_dpc((serializetype == PHPSerialize) ? GetBiosId(m_biosId) ? marshalCxCall(GETVXAGENTAPI, m_hostId, m_biosId) :
                                                                            marshalCxCall(GETVXAGENTAPI, m_hostId, "") : m_hostId),
            m_vxagent(m_dpc, t, cfg, mutex, settings, snaprequests , m_serializeType ),
            m_snapshotManager( marshalCxCall( m_dpc, "getSnapshotManager" ), t, m_serializeType ),
            m_volumeManager( marshalCxCall( m_dpc, "getVolumeManager" ), t, cfg, m_serializeType )
    {
        m_dataCacher.Init();
        memset(m_prevRegHostStaticHash, 0, sizeof m_prevRegHostStaticHash);
    }

    public:
        //
        // Object model
        //
        RegisterHostStatus_t RegisterHostStaticInfo(
                const bool &registerInAnyCase,
                std::string agentVersion,
                std::string driverVersion,
                std::string hostName,
                std::string ipAddress,
		        Object const & osinfo,
		        int compatibility,
		        std::string agentInstallPath,
                OS_VAL osVal,
		        std::string zone,
		        std::string sysVol,
		        std::string patchHistory,
                VolumeSummaries_t const& allVolumes,
		        std::string agentMode,
		        std::string prod_version , 
                NicInfos_t const& nicinfos,
                HypervisorInfo_t const& hypervinfo,
                NwwnPwwns_t const &npwwns,
                ENDIANNESS e,
		        CpuInfos_t const & cpuinfos,
                std::map<std::string, std::string> const& acnvpairs,
		        unsigned long long const & memory,
                const Options_t & miscInfo
                );

        RegisterHostStatus_t RegisterHostDynamicInfo( 
		        std::string agentLocalTime,
		        SV_ULONGLONG sysVolCapacity,
                SV_ULONGLONG sysVolFreeSpace,
		        SV_ULONGLONG insVolCapacity,
		        SV_ULONGLONG insVolFreeSpace,
                VolumeDynamicInfos_t const& volumeDynamicInfos
                );
        std::string GetRegisterHostStatusStr(const RegisterHostStatus_t rgs)
        {
			return NSRegisterHostStaticInfo::StrRegisterHostStatus[rgs];
        }

		/// \brief returns true if host static info registered atleast once
		bool IsHostStaticInfoRegisteredAtleastOnce(void);

//#ifdef SV_FABRIC
        void SanRegisterHost(
                std::string agentVersion,
                std::string driverVersion,
                std::string hostName,
                std::string ipAddress,
                std::string osInfo,
                int compatibility,
                std::string agentInstallPath, 
                OS_VAL osVal,
                std::vector<SanInitiatorSummary> const& initiators, 
                std::vector<SanVolumeSummary> const& allVolumes ) ;
//#endif



        void RegisterClusterInfo(
                std::string clusterName,
                std::string groupName,
                std::string groupId,
                std::string action,
                std::string newGroupName,
                std::vector<ClusterVolumeInfo> const& clusterVolumes,
                std::string fieldName,
                std::string fieldValue );

        void UnregisterHost() ;

        InitialSettings getInitialSettings();

		std::list<CsJob> getJobsToProcess();
		void updateJobStatus(CsJob);

		SNAPSHOT_REQUESTS getSnapshotRequestFromCx(const std::string & cxVolName = "") const;

        ConfigureSnapshotManager& getSnapshotManager() { return m_snapshotManager; }
        ConfigureVxAgent& getVxAgent() { return m_vxagent; }

        ConfigureVxTransport& getVxTransport() { return getVxAgent().getVxTransport(); }
        ConfigureVolumeManager& getVolumeManager() { return m_volumeManager; }

        /* combine stub and proxy?
           void doWork() {
           char const* data[] = { "one", "two", "three" };
        // transport.post( make_stub(this).onStubCall( data[0..2] ) );
        // post request
        // if response is remote->local call, do transport.post( make_stub(this).OnStubCall( response ) );
        // else x = make_stub(this).call<InitialSettings>( transport.getRequest() ); operate on (x).
        // stub.bind( this ); s = transport.getDpc(); stub.dispatch( s );[return type?] -> marshal and return to cx.
        // ..maybe this: post any requests on same thread; poll for cx server calls into vx, which block if no data.
        }*/
        ConfiguratorMediator m_transport; // In Qt style, allow slot to be public 
    private:
        CxProxy();
        void setApiAndFirstArg(const char *apiname,
                               const char *dpc, 
                               const char **apitocall,
                               const char **firstargtoapi) const;

        void logRegisteredVolumeSummaries(VolumeSummaries_t const& vs);
        SerializeType_t m_serializeType;
        std::string m_hostId;
        std::string m_biosId;
        LocalConfigurator& m_localConfigurator;
        ConfiguratorDeferredProcedureCall m_dpc;
        CxSnapshotManager m_snapshotManager;
        ConfigureVxAgentProxy m_vxagent;
        ConfigureVolumeManagerProxy m_volumeManager;
        unsigned char m_prevRegHostStaticHash[HASH_LENGTH];
        DataCacher m_dataCacher;
};


//#ifdef SV_FABRIC

class SwitchCxProxy 
{
    public:
        //
        // constructor/destructor
        //
        SwitchCxProxy(std::string hostId,ConfiguratorTransport t,LocalConfigurator& cfg,
                ACE_Mutex& mutex) :
            m_transport( t ),
            m_hostId( hostId ),
            m_localConfigurator( cfg ),
            m_dpc( marshalCxCall( GETVXAGENTAPI, m_hostId ) ),
            m_switchagent(m_dpc,t,cfg,mutex)
    {
    }

    public:
        void RegisterSwitchAgent(
                SwitchAgentInfo & swInfo ) ;
        void NewRegisterSwitchAgent(
                SwitchSummary & swSummary ) ;

	void UnregisterSwitchAgent() ;

	void UpdateSATimestamp(
		string timeZone,
		string localTime );

	SwitchInitialSettings getSwitchInitialSettings() ;

        SwitchInitialSettings getSwitchInitialSettingsOnReboot() ;

        ConfigureSwitchAgent& getSwitchAgent() { return m_switchagent;}

        ConfiguratorTransport m_transport;
    private:
        SwitchCxProxy();
        std::string m_hostId;
        std::string m_biosId;
        LocalConfigurator& m_localConfigurator;
        ConfiguratorDeferredProcedureCall m_dpc;
        ConfigureSwitchAgentProxy m_switchagent;
};
//#endif

#endif // CONFIGURATORCXPROXY__H

