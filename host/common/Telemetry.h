#ifndef MOBILITY_SERVICE_TELEMETRY_H
#define MOBILITY_SERVICE_TELEMETRY_H

#include "json_writer.h"

#define ADD_INT_TO_MAP(obj, key, val)                                               \
            {                                                                       \
                std::string strKey(key);                                            \
                try {                                                               \
                    std::string strVal = boost::lexical_cast<std::string>((val));   \
                    obj.Map.erase(key);                                             \
                    obj.Map[key] = strVal;                                          \
                }                                                                   \
                catch (const boost::bad_lexical_cast &e)                            \
                {                                                                   \
                    DebugPrintf(SV_LOG_ERROR, "%s : bad cast for %s. %s",           \
                    FUNCTION_NAME, strKey.c_str(),  e.what());                      \
                }                                                                   \
            }

#define ADD_STRING_TO_MAP(obj, key, val)                      \
        if ((val).length())                                   \
            obj.Map[key] = val;

#ifdef SV_UNIX
#define SOURCE_TELEMETRY_BASE_MESSAGE_TYPE   0x4000000000000000
#else
#define SOURCE_TELEMETRY_BASE_MESSAGE_TYPE   0x0
#endif

namespace AgentTelemetry {

    const std::string MESSAGETYPE("MessageType");
    const std::string MESSAGE("Message");
    const std::string TRUNCSTART("TruncStart");
    const std::string TRUNCEND("TruncEnd");

    const uint32_t  MESSAGE_TYPE_LOG_TRUNCATE = 2;

    class LogTruncStatus
    {
    public:
        std::map<std::string, std::string> Map;

        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "LogTruncStatus", false);

            JSON_T(adapter, Map);
        }
    };
}
#endif
