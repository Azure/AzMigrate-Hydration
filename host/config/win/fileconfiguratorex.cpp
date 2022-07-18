//
// Windows support for fileconfigurator. Uses registry key SV Systems\VxAgent\ConfigPathname
//
#include "../fileconfigurator.h"
#include <string>
#include <atlbase.h>
#include "globs.h"
#include "inmageex.h"
#include <string>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>

using namespace boost::property_tree;

extern ACE_Recursive_Thread_Mutex g_configRThreadMutex ;

static bool getRegKeyStringValue(const char *key,
    const char *param,
    std::string &outKeyValue,
    std::string &strError,
    const REGSAM &samDesired)
{
    bool retVal = true;
    CRegKey reg;
    LONG rc = reg.Open(HKEY_LOCAL_MACHINE, key, samDesired);
    if (ERROR_SUCCESS != rc) {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed to open registry key " << key << ", error=" << rc;
        strError = ss.str();
        retVal = false;
    }

    std::vector<char> szPathname(MAX_PATH + 1, '\0');
    ULONG cch = szPathname.size();
    rc = reg.QueryStringValue(param, &szPathname[0], &cch);
    if (ERROR_SUCCESS != rc)
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed to read " << param << " in registry key " << key << ", error=" << rc;
        strError = ss.str();
        retVal = false;
    }
    outKeyValue = std::string(&szPathname[0]);

    return retVal;
}

static std::string getConfigPathOnSource()
{
    std::string regKeyStringValue, strError;
    if (!getRegKeyStringValue(SV_VXAGENT_VALUE_NAME,
        SV_CONFIG_PATHNAME_VALUE_NAME,
        regKeyStringValue,
        strError,
        KEY_READ))
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed: " << strError;
        throw INMAGE_EX(ss.str().c_str());
    }

    return regKeyStringValue;
}

std::string FileConfigurator::getConfigPathname()
{
    AutoGuardRThreadMutex mutex(g_configRThreadMutex);
    static std::string strConfigPathName ;
    if(strConfigPathName.empty())
    {
        switch (s_initmode)
        {
        case FILE_CONFIGURATOR_MODE_VX_AGENT:
            strConfigPathName = getConfigPathOnSource();
            break;

        case FILE_CONFIGURATOR_MODE_CS_PRIME_APPLAINCE_TO_AZURE:
        case FILE_CONFIGURATOR_MODE_CS_PRIME_APPLIANCE_EVTCOLLFORW:
            // Keep strConfigPathName blank - dummy initializer of FileConfigurator on RCM prime appliance
            break;

        default:
            throw INMAGE_EX("Invalid init mode")(s_initmode);
        }
    }
 
    return strConfigPathName ;
}

void FileConfigurator::initConfigParamsOnCsPrimeApplianceToAzureAgent()
{
    boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
    std::map<std::string, std::string>::const_iterator itr =
        s_propertiesOnCsPrimeApplianceToAzure.find(CsPrimeApplianceToAzureAgentProperties::LOCAL_PREFERENCES_PATH);
    if (itr != s_propertiesOnCsPrimeApplianceToAzure.end())
    {
        try {
            if (boost::filesystem::exists(itr->second))
            {
                std::ifstream localconffile(itr->second);
                read_json(localconffile, m_pt_local_confparams);
            }
        }
        catch (std::exception& e)
        {
            throw INMAGE_EX("Init config params failed.")(itr->second)(e.what());
        }
    }
}

std::string FileConfigurator::getAgentInstallPathOnCsPrimeApplianceToAzure() const
{
    boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
    std::map<std::string, std::string>::const_iterator itr =
        s_propertiesOnCsPrimeApplianceToAzure.find(CsPrimeApplianceToAzureAgentProperties::SERVICE_INSTALL_PATH);
    if (itr == s_propertiesOnCsPrimeApplianceToAzure.end())
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed: "
            << CsPrimeApplianceToAzureAgentProperties::SERVICE_INSTALL_PATH << " not found";
        throw INMAGE_EX(ss.str().c_str());
    }
    return itr->second;
}

std::string FileConfigurator::getPSInstallPathOnCsPrimeApplianceToAzure() const
{
    std::string regKeyStringValue, strError;
    if (!getRegKeyStringValue(SV_CSPRIME_APPLIANCE_TO_AZURE_PS_NAME,
        SV_CSPRIME_APPLIANCE_TO_AZURE_PS_INSTALL_PATH,
        regKeyStringValue,
        strError,
        KEY_READ | KEY_WOW64_64KEY))
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed: " << strError;
        throw INMAGE_EX(ss.str().c_str());
    }

    return regKeyStringValue;
}

// To do: sadewang
// Remove this if its not used anywhere
std::string FileConfigurator::getPSTelemetryFolderPathOnCsPrimeApplianceToAzure() const
{
    std::string regKeyStringValue, strError;
    if (!getRegKeyStringValue(SV_CSPRIME_APPLIANCE_TO_AZURE_PS_NAME,
        SV_CSPRIME_APPLIANCE_TO_AZURE_PS_TELEMETRY_FOLDER_PATH,
        regKeyStringValue,
        strError,
        KEY_READ | KEY_WOW64_64KEY))
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed: " << strError;
        throw INMAGE_EX(ss.str().c_str());
    }

    return regKeyStringValue;
}

std::string FileConfigurator::getLogFolderPathOnCsPrimeApplianceToAzure() const
{

    std::string regKeyStringValue, strError;
    if (!getRegKeyStringValue(SV_CSPRIME_APPLIANCE_TO_AZURE_PS_NAME,
        SV_CSPRIME_APPLIANCE_TO_AZURE_PS_LOG_FOLDER_PATH,
        regKeyStringValue,
        strError,
        KEY_READ | KEY_WOW64_64KEY))
    {
        std::stringstream ss;
        ss << FUNCTION_NAME << " failed: " << strError;
        throw INMAGE_EX(ss.str().c_str());
    }

    return regKeyStringValue;
}

std::string FileConfigurator::getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, const std::string& key) const
{
    std::string param;
    bool IsKeyFound = false;

    try {
        param = m_pt_local_confparams.get<std::string>(key);
        IsKeyFound = true;
    }
    catch (const ptree_error &e)
    {
        IsKeyFound = false;
    }

    if (!IsKeyFound)
    {
        boost::mutex::scoped_lock guard(s_lockRcmInputsOnCsPrimeApplianceToAzure);
        std::map<std::string, std::string>::const_iterator itr =
            s_globalTunablesOnCsPrimeApplianceToAzure.find(key);
        if (itr == s_globalTunablesOnCsPrimeApplianceToAzure.end())
        {
            throw INMAGE_EX("config param no found.")(key);
        }
        param = itr->second;
    }

    return param;
}

void FileConfigurator::getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, std::map<std::string, std::string>& map) const
{
    throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getEvtCollForwParam(const std::string key) const
{
    boost::mutex::scoped_lock guard(s_lockEvtcollforwInputsOnAppliance);
    std::map<std::string, std::string>::const_iterator itr =
        s_evtcollforwPropertiesOnAppliance.find(key);
    if (itr == s_evtcollforwPropertiesOnAppliance.end())
    {
        throw INMAGE_EX("config param no found.")(key);
    }
    return itr->second;
}