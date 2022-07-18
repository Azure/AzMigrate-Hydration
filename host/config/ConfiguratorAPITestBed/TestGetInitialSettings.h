#ifndef TESTGETINITIALSETTINGS
#define TESTGETINITIALSETTINGS


#include "rpcconfigurator.h"
#include "serializationtype.h"
class TestGetInitialSettings : public RpcConfigurator
{

public:
    TestGetInitialSettings(ConfiguratorTransport t, SerializeType_t serializeType) : RpcConfigurator( serializeType) {}
 void Print();
 void PrintCdpsettings();
 void PrintHostVolumeGroupSettings();
 void PrintPrismSettings();
 void PrintTransportConnectionSettings(const TRANSPORT_CONNECTION_SETTINGS &tcs);
 void PrintATLUNStateRequest(const VOLUME_SETTINGS &vs);
 void PrintOptions(const VOLUME_SETTINGS &vs);
 LocalConfigurator& getLocalConfigurator();
};

#endif
