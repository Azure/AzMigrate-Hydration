
#ifndef CONFIGURESWITCHAGENT__H
#define CONFIGURESWITCHAGENT__H

#include <string>
#include <vector>
#include "sigslot.h"
#include "hostagenthelpers_ported.h"
#include "portablehelpers.h"
#include "switchinitialsettings.h"

using namespace std;




struct ConfigureLocalSwitchAgent 
{
	virtual bool getUseConfiguredHostname()const = 0;
	virtual std::string getConfiguredHostname()const = 0;
	virtual bool getUseConfiguredIpAddress()const = 0;
	virtual std::string getConfiguredIpAddress()const = 0;
	virtual std::string getInstallPath()const = 0;
	virtual std::string getFabricWorldWideName()const = 0;
};

struct ConfigureSwitchAgent : ConfigureLocalSwitchAgent {
	virtual bool updateFrameRedirectOperationState(FrameRedirectOperation& frameRedirectOperation)const = 0;
	virtual bool sendDiscoveredLunInfoToCx(PhysicalLunDiscoveryInfoList &pLunDiscList)const = 0;
	virtual string getSerializedPersistSwitchInitialSettings(PersistSwitchInitialSettings & pSwInitSettings) = 0;
	virtual PersistSwitchInitialSettings  getUnserializedPersistSwitchInitialSettings(string serialstr) = 0;
	virtual bool sendEventNotifyInfoSettingsToCx(EventNotifyInfoSettings eventNotifyInfoSettings)const = 0;
	virtual bool reportSwitchAgentReboot()const = 0;
        virtual bool reportApplianceLunEvent(SwitchAgentExceptionsList&)const = 0;
        virtual bool reportPhysicalLunEvent(SwitchAgentExceptionsList&)const = 0;
        virtual bool reportIoErrorException(SwitchAgentExceptionsList&)const = 0;


	virtual ~ConfigureSwitchAgent() {}
};

#endif // CONFIGUREVXAGENT__H

