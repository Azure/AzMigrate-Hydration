
#include "../fileconfigurator.h"
#include <fstream>
#include <string>
#include "globs.h"	

static const char FILE_CONFIGURATOR_CONFIG_PATHNAME[] = "/usr/local/drscout.conf";
std::string FileConfigurator::getConfigPathname() {
#ifdef SV_CYGWIN
	
	static char configPath[256] = {'\0'};
	if (configPath[0] != '\0')
		return configPath;
	else {
		string regKeyPathName = string ("/proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/SV\ Systems/VxAgent/") + SV_CONFIG_PATHNAME_VALUE_NAME ;
		ifstream fstr(regKeyPathName.c_str());
		int i=0;
		while (configPath[i++] = fstr.get());

		configPath[i-1] = '\0';
		fstr.close();
    		return configPath; 
	}
	
#else
        char resolvedPath[PATH_MAX] ;
        if(NULL != ACE_OS::realpath(FILE_CONFIGURATOR_CONFIG_PATHNAME, resolvedPath))
        {
            return resolvedPath;
        }

    	return FILE_CONFIGURATOR_CONFIG_PATHNAME;
#endif 
}

std::string FileConfigurator::getAgentInstallPathOnCsPrimeApplianceToAzure() const
{
    throw INMAGE_EX("not implemented")(s_initmode);
}

void FileConfigurator::initConfigParamsOnCsPrimeApplianceToAzureAgent()
{
	throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, const std::string& key) const
{
	throw INMAGE_EX("not implemented")(s_initmode);
}

void FileConfigurator::getConfigParamOnCsPrimeApplianceToAzureAgent(const std::string& section, std::map<std::string, std::string>& map) const
{
	throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getPSInstallPathOnCsPrimeApplianceToAzure() const
{
	throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getLogFolderPathOnCsPrimeApplianceToAzure() const
{
	throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getEvtCollForwParam(const std::string key) const
{
    throw INMAGE_EX("not implemented")(s_initmode);
}

std::string FileConfigurator::getPSTelemetryFolderPathOnCsPrimeApplianceToAzure() const
{
    throw INMAGE_EX("not implemented")(s_initmode);
}