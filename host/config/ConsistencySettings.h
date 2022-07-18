#ifndef CONSISTENCYSETTINGS_H
#define CONSISTENCYSETTINGS_H

#include "json_reader.h"
#include "json_writer.h"

#define OPT_CRASH_CONSISTENCY   "-cc"
#define OPT_MASTER_NODE         "-mn"
#define OPT_CLIENT_NODE         "-cn"
#define OPT_PRESCRIPT           "-prescript "
#define OPT_SPACE_SEPARATOR     " "

namespace CS
{
    class ConsistencySettings
    {
    public:
        ConsistencySettings() : m_AppConsistencyInterval(0), m_CrashConsistencyInterval(0) {}

        /// \brief Command options
        std::string m_CmdOptions;

        /// \brief Application consistency bookmark interval in sec granularity
        uint64_t m_AppConsistencyInterval;

        /// \brief Crash consistency bookmark interval in sec granularity
        uint64_t m_CrashConsistencyInterval;

    public:
        void serialize(JSON::Adapter& adapter)
        {
            JSON::Class root(adapter, "ConsistencySettings", false);
            JSON_E(adapter, m_CmdOptions);
            JSON_E(adapter, m_AppConsistencyInterval);
            JSON_T(adapter, m_CrashConsistencyInterval);
        }
    };
}

#endif // CONSISTENCYSETTINGS_H