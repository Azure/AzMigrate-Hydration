//
// configurevolumemanagerproxy.h: CX representation of volume manager
//
#ifndef CONFIGUREVOLUMEMANAGERPROXY__H
#define CONFIGUREVOLUMEMANAGERPROXY__H

#include "proxy.h"
#include "configurevolumemanager.h"
#include "localconfigurator.h"

class ConfigureVolumeManagerProxy : public ConfigureVolumeManager {
public:
    //
    // constructors/destructors
    //
    ConfigureVolumeManagerProxy( ConfiguratorDeferredProcedureCall dpc, 
        ConfiguratorMediator& t, LocalConfigurator& cfg,
        const SerializeType_t serializetype ) :
        m_serializeType(serializetype),
        m_dpc( dpc ), 
        m_transport( t ),
        m_localConfigurator( cfg )
    {
    }

public:
    //
    // No interesting interfaces yet
    //

private:
    SerializeType_t m_serializeType;
    LocalConfigurator& m_localConfigurator;
    ConfiguratorDeferredProcedureCall const m_dpc;
    ConfiguratorMediator const m_transport;
};

#endif // CONFIGUREVOLUMEMANAGERPROXY__H

