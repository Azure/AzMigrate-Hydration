
#include "../fileconfigurator.h"

static const char FILE_CONFIGURATOR_CONFIG_PATHNAME[] = "/home/svsystems/etc/drscout.conf";
 
std::string FileConfigurator::getConfigPathname() {
    return FILE_CONFIGURATOR_CONFIG_PATHNAME;
}

