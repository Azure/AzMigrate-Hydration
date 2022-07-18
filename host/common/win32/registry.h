#ifndef __REGISTRY__H
#define __REGISTRY__H

#include "svtypes.h"
#include <string>
#include <list>
#include <vector>

namespace REG_SV_SYSTEMS
{
	const char SV_SYSTEM_KEY_X32[] = "\\SV Systems";
	const char SV_SYSTEM_KEY_X64[] = "\\Wow6432Node\\SV Systems";
	const char SV_SYSTEM_KEY_VxAgent[] = "\\VxAgent";
	const char SOFTWARE_HIVE[] = "Software";

	const char VALUE_NAME_PROD_VERSION[] = "PROD_VERSION";
	const char VALUE_NAME_RECOVERY_INPROGRESS[] = "RecoveryInprogress";
	const char VALUE_NAME_NEW_HOSTID[] = "NewHostId";
	const char VALUE_NAME_TEST_FAILOVER[] = "TestFailover";
	const char VALUE_NAME_RECOVERED_ENV[] = "CloudEnv";
	const char VALUE_NAME_ENABLE_RDP[] = "EnableRDP";

	const char VALUE_NAME_HYPERVISOR[]  = "Hypervisor";
	const char VALUE_NAME_SYSTEM_UUID[] = "UUID";
};

template<typename TYPE>
inline bool HandleStatusCode(TYPE lStatus, TYPE successCode, TYPE failedCode, const std::string& expceptionMsg)
{
	if (successCode == lStatus) return true;
	else if (failedCode == lStatus) return false;
	else {
		std::stringstream expSteam; expSteam << expceptionMsg << ". Error " << lStatus;
		throw std::exception(expSteam.str().c_str());
	}
}
#define VERIFY_REG_STATUS(lStatus,errorMsg) HandleStatusCode(lStatus, ERROR_SUCCESS, ERROR_FILE_NOT_FOUND, errorMsg)

SVERROR createKey(const std::string& key) ;

SVERROR deleteKey(const std::string& root, const std::string& key) ;

SVERROR deleteValue(const std::string& root, const std::string value) ;

SVERROR getDwordValue(const std::string& root, const std::string value, SV_LONGLONG& data) ;

SVERROR setDwordValue(const std::string& root, const std::string& value, SV_LONGLONG data) ;

SVERROR getStringValue(const std::string& root, const std::string value, std::string& data) ;
SVERROR getStringValue(const std::string& root, const std::string value, std::string& data, SV_USHORT accessMode);
SVERROR getStringValueEx(const std::string& root, const std::string value, std::string& data) ;
SVERROR getMultiStringValue(const std::string& root, const std::string value, std::list<std::string>& data) ;
SVERROR getBinaryValue( const std::string& root, const std::string& key, std::vector<unsigned char>& binaryValue );

SVERROR setStringValue(const std::string& root, const std::string value, const std::string& data) ;
SVERROR setBinaryValue( const std::string& root, const std::string& key, std::string data);
SVERROR getSubKeysList(const std::string& key, std::list<std::string>& subkeys);
SVERROR getSubKeysListEx(const std::string& key, std::list<std::string>& subkeys, SV_USHORT accessMode);
bool isKeyAvailable(const std::string& key);

SV_LONG SetSVSystemDWORDValue(const std::string& valueName,
	SV_ULONG valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath = "");
SV_LONG GetSVSystemDWORDValue(const std::string& valueName,
	SV_ULONG &valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath = "");

SV_LONG SetSVSystemStringValue(const std::string& valueName,
	const std::string& valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath = "");
SV_LONG GetSVSystemStringValue(const std::string& valueName,
	std::string& valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath = "");

void TestRegistryFunctions() ;
#endif