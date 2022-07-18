//
// snapshotmanagerproxy.h: marshal calls of snapshotmanager proxy -> transport
//
#ifndef CONFIGURATORSNAPSHOTMANAGERPROXY__H
#define CONFIGURATORSNAPSHOTMANAGERPROXY__H

#include "proxy.h"
#include "configuresnapshotmanager.h"
#include "serializationtype.h"

class CxSnapshotManager : public ConfigureSnapshotManager {
public:
    //
    // constructor/destructor
    //
    CxSnapshotManager( 
        ConfiguratorDeferredProcedureCall dpc, ConfiguratorMediator& t , 
        const SerializeType_t serializetype ) : 
        m_serializeType(serializetype) , m_dpc( dpc ), m_transport( t ) {}
public:
    //
    // Object model
    //
    int getSnapshotCount();

    // todo: return cached list of actual snapshots managed
    Range getSnapshots() {
        return Range();
    }

private:
    CxSnapshotManager();
    SerializeType_t m_serializeType;
    ConfiguratorDeferredProcedureCall const m_dpc;
    ConfiguratorMediator const  m_transport;
};

#endif // CONFIGURATORSNAPSHOTMANAGERPROXY__H


