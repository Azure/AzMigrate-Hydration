#ifndef MONITORING_PARAMS_H
#define MONITORING_PARAMS_H

#include "json_reader.h"
#include "json_writer.h"

#define MoitoringParamsFileName "MoitoringParams.json"

class MoitoringParams
{
public:
    std::string LastRebootTime;
    std::string AgentVersion;

public:
    void serialize(JSON::Adapter& adapter)
    {
        JSON::Class root(adapter, "MoitoringParams", false);
        JSON_E(adapter, LastRebootTime);
        JSON_T(adapter, AgentVersion);
    }
};

#endif //END MONITORING_PARAMS_H