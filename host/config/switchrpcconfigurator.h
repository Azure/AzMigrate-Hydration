//
// rpcconfigurator.h: declare concrete configurator
//
#ifndef SWITCHRPCCONFIGURATOR__H
#define SWITCHRPCCONFIGURATOR__H

#include <ace/Thread_Manager.h>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include <ace/Event.h>

#include "cxproxy.h"
#include "localconfigurator.h"
#include "switchinitialsettings.h"
#include "talwrapper.h"



class SwitchRpcConfigurator // : 
//    public RpcConfigurator 
{
public:

    SwitchRpcConfigurator();
    ~SwitchRpcConfigurator();

public:
	ConfigureSwitchAgent& GetSwitchAgent();
	SwitchCxProxy& getCxAgent();
	SwitchInitialSettings getCurrentSwitchSetting();	
	SwitchInitialSettings getCurrentVtOperationListOnReboot();

protected:
	LocalConfigurator m_localConfigurator;	// must be before m_cx
	TalWrapper m_tal;
	SwitchCxProxy m_cx;
private:
    mutable ACE_Mutex m_lockSettings;
};

SwitchRpcConfigurator& GetSwitchRpcConfigurator();


#endif // RPCCONFIGURATOR__H

