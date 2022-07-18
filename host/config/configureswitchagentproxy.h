
#ifndef CONFIGURESWITCHAGENTPROXY__H
#define CONFIGURESWITCHAGENTPROXY__H


#include "proxy.h"
#include "configureswitchagent.h"
#include "switchinitialsettings.h"
#include "localconfigurator.h"

class ConfigureSwitchAgentProxy : public ConfigureSwitchAgent {
public:
    //
    // constructors/destructors
    //
    ConfigureSwitchAgentProxy( ConfiguratorDeferredProcedureCall dpc, 
        ConfiguratorTransport& t, LocalConfigurator& cfg,
        ACE_Mutex& mutex) :
        m_dpc( dpc ), 
        m_transport( t ),
        m_localConfigurator( cfg ),
        m_lock( mutex )
       
    {
    } 
   
   public:
	bool updateFrameRedirectOperationState(FrameRedirectOperation& frameRedirectOperation)const;
	bool sendDiscoveredLunInfoToCx(PhysicalLunDiscoveryInfoList &pLunDiscList)const;
	string getSerializedPersistSwitchInitialSettings(PersistSwitchInitialSettings & pSwInitSettings);
	PersistSwitchInitialSettings  getUnserializedPersistSwitchInitialSettings(string serialstr);
	bool getUseConfiguredHostname() const;
	std::string getConfiguredHostname() const;
	bool getUseConfiguredIpAddress() const;
	std::string getConfiguredIpAddress() const;
	std::string getInstallPath() const;
	std::string getFabricWorldWideName() const;
	bool sendEventNotifyInfoSettingsToCx(EventNotifyInfoSettings eventNotifyInfoSettings)const;
	bool reportSwitchAgentReboot () const;
	bool reportApplianceLunEvent(SwitchAgentExceptionsList&)const;
	bool reportPhysicalLunEvent(SwitchAgentExceptionsList&)const;
	bool reportIoErrorException(SwitchAgentExceptionsList&)const;


   private:
	LocalConfigurator& m_localConfigurator;
	ConfiguratorDeferredProcedureCall const m_dpc;
	ConfiguratorTransport const m_transport;
    ACE_Mutex& m_lock;
    typedef ACE_Guard<ACE_Mutex> AutoGuard;

   };

#endif	
