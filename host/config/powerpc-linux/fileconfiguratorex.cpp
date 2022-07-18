
#include "../fileconfigurator.h"
#include <fstream>
#include <string>
#include "globs.h"

static const char FILE_CONFIGURATOR_CONFIG_PATHNAME[] = "/etc/drscout.conf";
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
    	return FILE_CONFIGURATOR_CONFIG_PATHNAME;
#endif
}

