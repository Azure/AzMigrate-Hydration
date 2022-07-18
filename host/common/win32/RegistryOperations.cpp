#include "portablehelpersmajor.h"

#include <Windows.h>
#include <list>
#include <string>
#include <atlbase.h>
#include <sstream>
#include <map>

#include <boost/scope_exit.hpp>
#include <boost/algorithm/string.hpp>

#include "RegistryOperations.h"

DWORD RegistryInfo::GetDriversStartType(std::map<std::string, DWORD>& DriversMap)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    std::stringstream ssControlKey;
    ssControlKey << "SYSTEM\\" << "CurrentControlSet";

    for (auto driverIt : DriversMap) {
        DWORD dwStartType = QueryServiceStartType(ssControlKey.str(), driverIt.first);
        DriversMap[driverIt.first] = dwStartType;
        DebugPrintf(SV_LOG_ALWAYS, "Driver %s StartType: %d\n", driverIt.first.c_str(), dwStartType);
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return ERROR_SUCCESS;
}

DWORD RegistryInfo::SetDriversStartType(std::map<std::string, DWORD> DriversMap)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    // Get all Control Sets
    std::list<std::string>  controlSetSubKeys;

    DWORD dwStatus = GetSubKeysList("SYSTEM", controlSetSubKeys, "ControlSet");
    if (ERROR_SUCCESS != dwStatus) {
        DebugPrintf(SV_LOG_ERROR, "Failed to Get Subkeys for ControlSet\n");
        return dwStatus;
    }

    for (auto driverIt : DriversMap) {
        for (auto controlIt : controlSetSubKeys) {
            std::stringstream ssControlKey;
            ssControlKey << "SYSTEM\\" << controlIt.c_str();

            DebugPrintf(SV_LOG_INFO, "Setting in ControlSet %s  for driver %s dwStartType: %d\n",
                                       ssControlKey.str().c_str(),
                                        driverIt.first.c_str(),
                                        driverIt.second);

            SetServiceStartType(ssControlKey.str(), driverIt.first, driverIt.second);
        }
    }
    return dwStatus;
}

DWORD RegistryInfo::GetSubKeysList(const std::string& key, std::list<std::string>& subkeys)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);
    return GetSubKeysList(key, subkeys, "");
}

DWORD RegistryInfo::GetSubKeysList(const std::string& key, std::list<std::string>& subkeys, const std::string pattern)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    DWORD   dwResult = ERROR_SUCCESS;
    CRegKey cregkey;
    std::vector<char>    subKeyBuffer(MAX_PATH + 1);


    DebugPrintf(SV_LOG_DEBUG, "KeyName: %s\n", key.c_str());

    dwResult = cregkey.Open(HKEY_LOCAL_MACHINE, key.c_str(), KEY_READ);
    if (dwResult != ERROR_SUCCESS)
    {
        DebugPrintf(SV_LOG_ERROR, "Error: CRegKey::Open failed for key: %s with error %ld\n", key.c_str(), dwResult);
        return dwResult;
    }

    DWORD subkeyLength = MAX_PATH;
    memset(&subKeyBuffer[0], '\0', MAX_PATH + 1);

    int index = 0;

    while (ERROR_SUCCESS == (dwResult = RegEnumKey(cregkey, index, &subKeyBuffer[0], subkeyLength)))
    {
        ++index;
        subkeyLength = MAX_PATH;

        DebugPrintf(SV_LOG_DEBUG, "Subkey: %s\n", &subKeyBuffer[0]);

        if (!pattern.empty() && !boost::starts_with(&subKeyBuffer[0], pattern)){
            DebugPrintf(SV_LOG_DEBUG, "Skipping Subkey: %s\n", &subKeyBuffer[0]);
            std::fill(subKeyBuffer.begin(), subKeyBuffer.end(), 0);
            continue;
        }

        DebugPrintf(SV_LOG_DEBUG, "Pushing Subkey: %s\n", &subKeyBuffer[0]);
        subkeys.push_back(&subKeyBuffer[0]);

        std::fill(subKeyBuffer.begin(), subKeyBuffer.end(), 0);
    }

    if (ERROR_NO_MORE_ITEMS == dwResult) {
        dwResult = ERROR_SUCCESS;
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "EnumKey failed with error %d\n", dwResult);
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return dwResult;
}

DWORD RegistryInfo::QueryServiceStartType(std::string rootKey, std::string szDriverName)
{
    LONG    lStatus;
    DWORD   lpType = REG_DWORD;
    DWORD   cbData = sizeof(DWORD);
    DWORD   dwStartType;

    std::vector<CHAR>   szImagePath(MAX_PATH);
    std::vector<CHAR>   szSystemRoot(MAX_PATH);

    PVOID               OldValue = NULL;
    CRegKey             svcKey;

    std::stringstream   ssRegName;

    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    ssRegName << rootKey << "\\" << SourceRegistry::SERVICES_KEY_VALUE << SourceRegistry::REG_PATH_SEPARATOR << szDriverName;
    DebugPrintf(SV_LOG_INFO, "Validating ControlSet %s\n", ssRegName.str().c_str());

    lStatus = svcKey.Open(HKEY_LOCAL_MACHINE, ssRegName.str().c_str());
    if (ERROR_SUCCESS != lStatus) {
        DebugPrintf(SV_LOG_ERROR, "Error: failed to open registry %s error=%d\n", ssRegName.str().c_str(), lStatus);
        return lStatus;
    }

    lStatus = svcKey.QueryDWORDValue(SourceRegistry::START_TYPE, dwStartType);
    if (ERROR_SUCCESS != lStatus) {
        DebugPrintf(SV_LOG_ERROR, "Error: failed to query Start Type registry %s error=%d\n", ssRegName.str().c_str(), lStatus);
        return lStatus;
    }

    DebugPrintf(SV_LOG_INFO, "%s StartType: %d\n", ssRegName.str().c_str(), dwStartType);

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);

    return dwStartType;
}

DWORD RegistryInfo::SetServiceStartType(std::string rootKey, std::string szDriverName, DWORD dwStartType)
{
    LONG    lStatus;
    DWORD   lpType = REG_DWORD;
    DWORD   cbData = sizeof(DWORD);
    DWORD   dwCurrentStartType;

    std::vector<CHAR>   szImagePath(MAX_PATH);
    std::vector<CHAR>   szSystemRoot(MAX_PATH);

    ULONG               lPathSize;
    PVOID               OldValue = NULL;
    CRegKey             svcKey;

    std::stringstream   ssRegName;

    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", FUNCTION_NAME);

    ssRegName << rootKey << "\\" << SourceRegistry::SERVICES_KEY_VALUE << SourceRegistry::REG_PATH_SEPARATOR << szDriverName;
    DebugPrintf(SV_LOG_INFO, "Validating ControlSet %s\n", ssRegName.str().c_str());

    lStatus = svcKey.Open(HKEY_LOCAL_MACHINE, ssRegName.str().c_str());
    if (ERROR_SUCCESS != lStatus) {
        DebugPrintf(SV_LOG_ERROR, "Error: failed to open registry %s error=%d\n", ssRegName.str().c_str(), lStatus);
        return lStatus;
    }

    lStatus = svcKey.QueryDWORDValue(SourceRegistry::START_TYPE, dwCurrentStartType);
    if (ERROR_SUCCESS != lStatus) {
        DebugPrintf(SV_LOG_ERROR, "Error: failed to query Start Type registry %s error=%d\n", ssRegName.str().c_str(), lStatus);
        return lStatus;
    }

    if (dwCurrentStartType == dwStartType) {
        DebugPrintf(SV_LOG_INFO, "Driver %s Start Type is %d already skipping\n", ssRegName.str().c_str(), dwStartType);
        return dwStartType;
    }

    lStatus = svcKey.SetDWORDValue(SourceRegistry::START_TYPE, dwStartType);
    if (ERROR_SUCCESS != lStatus) {
        DebugPrintf(SV_LOG_ERROR, "Error: failed to set Start Type registry %s error=%d\n", ssRegName.str().c_str(), lStatus);
        return lStatus;
    }

    DebugPrintf(SV_LOG_ALWAYS, "Driver %s Updated Start Type current: %d changed: %d\n", ssRegName.str().c_str(), dwCurrentStartType, dwStartType);
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", FUNCTION_NAME);
    return dwStartType;
}