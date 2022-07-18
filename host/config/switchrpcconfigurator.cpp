//
// switchrpcconfigurator.cpp: define concrete configurator
//
#include <iostream>	//debugging
#include <ace/Guard_T.h>
#include <ace/Mutex.h>
#include "switchrpcconfigurator.h"
#include "localconfigurator.h"
#include "talwrapper.h"
#include "inmageex.h"
#include "switchinitialsettings.h"

using namespace std;
SwitchRpcConfigurator& GetSwitchRpcConfigurator() {
    static ACE_Mutex lock;
    ACE_Guard<ACE_Mutex> guard( lock );

    static SwitchRpcConfigurator* configurator = NULL;
    if( configurator != NULL ) {
        return *configurator;
    }

    //LocalConfigurator localConfigurator;
    configurator = new SwitchRpcConfigurator( );
    return *configurator;
}

SwitchRpcConfigurator::SwitchRpcConfigurator() :
	m_cx( m_localConfigurator.getHostId(), m_tal, m_localConfigurator, m_lockSettings)
{
	
}

SwitchRpcConfigurator::~SwitchRpcConfigurator()
{}


SwitchInitialSettings SwitchRpcConfigurator::getCurrentSwitchSetting()
{
	SwitchInitialSettings CurrentSwitchSettings;
	CurrentSwitchSettings = m_cx.getSwitchInitialSettings();		 
        return CurrentSwitchSettings;
}



SwitchInitialSettings SwitchRpcConfigurator::getCurrentVtOperationListOnReboot()
{
	return m_cx.getSwitchInitialSettingsOnReboot();		 
}

SwitchCxProxy& SwitchRpcConfigurator::getCxAgent() { 
    return m_cx;
}

ConfigureSwitchAgent& SwitchRpcConfigurator::GetSwitchAgent()
{
	return m_cx.getSwitchAgent();
}




