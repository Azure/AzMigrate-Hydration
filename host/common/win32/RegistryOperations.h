#include "portablehelpersmajor.h"

class RegistryInfo {
public:
    static DWORD QueryServiceStartType(std::string rootKey, std::string szDriverName);
    static DWORD SetServiceStartType(std::string rootKey, std::string szDriverName, DWORD dwStartType);
    static DWORD GetSubKeysList(const std::string& key, std::list<std::string>& subkeys);
    static DWORD GetSubKeysList(const std::string& key, std::list<std::string>& subkeys, const std::string pattern);
    static DWORD SetDriversStartType(std::map<std::string, DWORD> DriversMap);
    static DWORD GetDriversStartType(std::map<std::string, DWORD>& DriversMap);
};

namespace SourceRegistry{
    const char  SERVICES_KEY_VALUE[] = "Services";
    const char  REG_PATH_SEPARATOR[] = "\\";
    const char  DIR_PATH_SEPARATOR[] = "\\";
    const char  START_TYPE[] = "Start";
}
