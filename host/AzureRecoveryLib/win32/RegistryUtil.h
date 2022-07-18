/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		: RegistryUtil.h

Description	: Delcarations for windows registry utility functions.

History		: 1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_REGISTRY_UTILS_H
#define AZURE_RECOVERY_REGISTRY_UTILS_H

#include <list>
#include <map>

#include <Windows.h>
#include "WinConstants.h"
#include "../config/HostInfoDefs.h"
#include "../config/HostInfoConfig.h"
#include "../common/utils.h"
#include "OSVersion.h"

#define VERIFY_REG_STATUS(lStatus,errorMsg) HandleStatusCode(lStatus, ERROR_SUCCESS, ERROR_FILE_NOT_FOUND, errorMsg)

namespace AzureRecovery
{
	LONG LoadRegistryHive(const std::string& hivePath, const std::string& hiveName);

	LONG UnloadRegistryHive(const std::string& hiveName);

	LONG RegSetServiceStartType(const std::string& systemHive,
		const std::map<std::string, DWORD> serviceStartTypes,
		bool ignoreIfNotFound = false);

	LONG RegChangeServiceStartType(const std::string& systemHive,
		const std::string& controlSet,
		const std::string& serviceName,
		DWORD dwSvcStartType,
		DWORD &oldDwSvcStartType,
		bool ignoreIfNotFound = false);

	LONG RegUpdateSystemHiveDWORDValue(const std::string& systemHive,
		const std::string& controlSet,
		const std::string& relativeKeyPath,
		const std::string& valueName,
		DWORD & dwOldValue, 
		DWORD dwNewValue,
		bool igonreIfNotFound = false);

    LONG RegAddOrUpdateSystemHiveDWORDValue(const std::string& systemHive,
        const std::string& controlSet,
        const std::string& relativeKeyPath,
        const std::string& valueName,
        DWORD & dwOldValue,
        DWORD dwNewValue,
        bool igonreIfNotFound = false);

	LONG RegGetSystemHiveDWORDValue(const std::string& systemHive,
		const std::string& controlSet,
		const std::string& relativeKeyPath,
		const std::string& valueName,
		DWORD & dwOldValue);

	bool RegKeyExists(HKEY hKeyParent, const std::string& keyPath);
		
	LONG RegSetDWORDValues(HKEY hKey, std::map<std::string, DWORD>& value_map);

	LONG RegSetStringValues(HKEY hKey, std::map<std::string, std::string>& value_map);

	LONG RegGetStringValue(HKEY hKey, const std::string& valueName, std::string& valueData);

	LONG RegGetSubKeys(HKEY hBaseKey, std::vector<std::string>& subKeys, const std::string& keyContains = "");

	LONG RegGetControlSets(const std::string& SystemHiveName, std::vector<std::string>& controlSets, const std::string& controlSetPreference = "");

	LONG RegGetAgentInstallPath(const std::string& SoftwareHive, std::string& agentInstallPath);

	LONG RegGetOSCurrentVersion(const std::string& SoftwareHive, std::string& osVersion);

	LONG RegAddBootUpScript(const std::string& BootupScriptRegKeyPath,
		const std::string& Script,
		const std::string& ScriptParams,
		const std::string& SrcSystemVolName,
		const OSVersion& osVersion,
		bool IsPowerShellScript);

	LONG RegGetSVSystemDWORDValue(const std::string& valueName,
		DWORD& valueData,
		const std::string& relativeKeyPath = "");

	LONG RegSetSVSystemDWORDValue(const std::string& valueName, 
		DWORD valueData, 
		const std::string& relativeKeyPath = "");

	LONG RegSetSVSystemStringValue(const std::string& valueName, 
		const std::string& valueData, 
		bool isMultiString = false, 
		const std::string& relativeKeyPath = "");

	LONG RegGetSVSystemStringValue(const std::string& valueName,
		std::string& valueData,
		const std::string& relativeKeyPath = "");

	LONG RegGetCurrentControlSetValue(const std::string& systemHiveName, DWORD & dwValue);

    LONG RegGetLastKnownGoodControlSetValue(const std::string& systemHiveName, DWORD & dwValue);
}
#endif //~AZURE_RECOVERY_REGISTRY_UTILS_H
