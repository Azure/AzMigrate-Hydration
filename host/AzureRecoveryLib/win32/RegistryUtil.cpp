/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	RegistryUtil.cpp

Description	:   Registry utility functions.

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "RegistryUtil.h"
#include "WmiRecordProcessors.h"
#include "../common/utils.h"
#include "../common/Trace.h"
#include "../Status.h"

#include <sstream>
#include <list>
#include <atlbase.h>

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace AzureRecovery
{

/*
Method      : LoadRegistryHive

Description : Loads the given registry file 'hivePath, and assigns the the hiveName
              to access its contents using registy APIs.

Parameters  : [in] hivePath: regstry hive file path
              [in] hiveName: key name for the registry hive that beeing loaded.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG LoadRegistryHive(const std::string& hivePath, const std::string& hiveName)
{
    TRACE_FUNC_BEGIN;

    BOOST_ASSERT(!hivePath.empty());
    BOOST_ASSERT(!hiveName.empty());

    LONG lRegRet = ERROR_SUCCESS;
    do
    {
        //
        // Verify that there is no hive already loaded with hiveName. If loaded then uload it.
        // 
        HKEY hKeyHive = NULL;
        lRegRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, hiveName.c_str(), 0, KEY_ALL_ACCESS, &hKeyHive);
        if (ERROR_SUCCESS == lRegRet)
        {
            CloseHandle(hKeyHive); hKeyHive = NULL;

            lRegRet = RegUnLoadKey(HKEY_LOCAL_MACHINE, hiveName.c_str());
            if (ERROR_SUCCESS != lRegRet)
            {
                TRACE_ERROR("Could not unload existing registry hive %s. Error %lu\n",
                            hiveName.c_str(),
                            lRegRet);

                break;
            }
        }

        //
        // Load the registry hive
        //
        TRACE_INFO("Loading the Hive %s in registry\n", hiveName.c_str());
        lRegRet = RegLoadKey(HKEY_LOCAL_MACHINE, hiveName.c_str(), hivePath.c_str());
        if (ERROR_SUCCESS != lRegRet)
        {
            TRACE_ERROR("Could not load the registry hive %s from the path %s. Error %lu\n",
                        hiveName.c_str(),
                        hivePath.c_str(),
                        lRegRet);

            SetLastErrorMsg("Loading registry hive %s failed with error %lu.",
                hivePath.c_str(),
                lRegRet);

            // Check if the hive path is valid and accessible, if not 
            // then the volume might not be accessible or does not have valid OS installation.
            if (!FileExists(hivePath))
            {
                SetRecoveryErrorCode(E_RECOVERY_VOL_UNEXPECTED_FORMAT);
            }

            break;
        }
        
    } while (false);

    TRACE_FUNC_END;
    return lRegRet;
}

/*
Method      : UnloadRegistryHive

Description : Unloads the registry hive which is attached with the hiveName.

Parameters  : [in] hiveName: Uloading registry hive name.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG UnloadRegistryHive(const std::string& hiveName)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!hiveName.empty());

    LONG lRegRet = RegUnLoadKey(HKEY_LOCAL_MACHINE, hiveName.c_str());
    if (ERROR_SUCCESS != lRegRet) // TODO: consider the file not found return code aslo as success
    {
        TRACE_ERROR("Could not uload the registry hive %s. RegUnLoadKey error %lu\n",
                    hiveName.c_str(),
                    lRegRet);
    }

    TRACE_FUNC_END;
    return lRegRet;
}

/*
Method      : RegGetStringValue

Description : Reads the string value data

Parameters  : [in]  hKey      : Valied registry hey handle
              [in]  valueName : Name of the string registry value
              [out] valueData : Filled with the string value data.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetStringValue(HKEY hKey, const std::string& valueName, std::string& valueData)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;
    BOOST_ASSERT(NULL != hKey);

    DWORD ccData = MAX_PATH + 1;
    TCHAR* pcData = NULL;

    CRegKey key;
    key.Attach(hKey);

    do
    {
        pcData = (TCHAR*)malloc(ccData * sizeof(TCHAR));
        if (NULL == pcData)
        {
            lRetStatus = ERROR_OUTOFMEMORY;
            TRACE_ERROR("Out of memory\n");
            break;
        }

        SecureZeroMemory(pcData, ccData * sizeof(TCHAR));

        lRetStatus = key.QueryStringValue(valueName.c_str(), pcData, &ccData);
        if (ERROR_MORE_DATA == lRetStatus)
        {
            free(pcData);

            pcData = (TCHAR*)malloc(ccData * sizeof(TCHAR));
            if (NULL == pcData)
            {
                lRetStatus = ERROR_OUTOFMEMORY;
                TRACE_ERROR("Out of memory\n");
                break;
            }

            SecureZeroMemory(pcData, ccData * sizeof(TCHAR));

            lRetStatus = key.QueryStringValue(valueName.c_str(), pcData, &ccData);
        }

        if (ERROR_SUCCESS == lRetStatus)
            valueData = pcData;

    } while (false);

    if (pcData) free(pcData);

    // Calling Detach to avoid closing hKey handle by CRegKey class destuctor.
    key.Detach();

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegSetStringValues

Description : Sets the list of string values under the registry key hKey

Parameters  : [in] hKey     : Valid registry key handle
              [in] value_map: Map of string value->data entries. 

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegSetStringValues(HKEY hKey, std::map<std::string, std::string>& value_map)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;
    BOOST_ASSERT(NULL != hKey);

    if (value_map.empty()) 
        return lRetStatus;

    CRegKey regKey;
    regKey.Attach(hKey);

    do 
    {
        auto iterMap = value_map.begin();
        for (; iterMap != value_map.end(); iterMap++)
        {
            if (iterMap->first.empty())
                continue;

            lRetStatus = regKey.SetStringValue(iterMap->first.c_str(), iterMap->second.c_str());
            if (ERROR_SUCCESS != lRetStatus)
            {
                TRACE_ERROR("RegSetStringValue error for %s = %s. Error code %ld\n",
                            iterMap->first.c_str(),
                            iterMap->second.c_str(),
                            lRetStatus );
                break;
            }
        }
    } while (false);

    regKey.Detach();

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegSetDWORDValues

Description : Sets list of dword values under the regisry key hKey

Parameters  : [in] hKey     : Valid registry key handle
              [in] value_map: Map of value-name->dword entries. 

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegSetDWORDValues(HKEY hKey, std::map<std::string, DWORD>& value_map)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;
    BOOST_ASSERT(NULL != hKey);

    if (value_map.empty())
        return lRetStatus;

    CRegKey regKey;
    regKey.Attach(hKey);

    do
    {
        auto iterMap = value_map.begin();
        for (; iterMap != value_map.end(); iterMap++)
        {
            if (iterMap->first.empty())
                continue;

            lRetStatus = regKey.SetDWORDValue(iterMap->first.c_str(), iterMap->second);
            if (ERROR_SUCCESS != lRetStatus)
            {
                TRACE_ERROR("RegSetDWORDValue error for %s = %lu. Error code %ld\n",
                            iterMap->first.c_str(),
                            iterMap->second,
                            lRetStatus);
                break;
            }
        }
    } while (false);

    regKey.Detach();

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetSubKeys

Description : Retries list of sub-keys under given base registry key

Parameters  : [in] hBaseKey : Valid registry key handle
              [out] subkeys : Filled with list of subkeys under the base key
              [in, optional] keyContrains: a patterns that matches the sub-key names.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetSubKeys(HKEY hBaseKey, std::vector<std::string>& subKeys, const std::string& keyContains)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    BOOST_ASSERT(NULL != hBaseKey);

    do {
        TCHAR    achClass[MAX_PATH] = { 0 };  // buffer for class name 
        DWORD    cchClassName = MAX_PATH;  // size of class string 
        DWORD    cSubKeys = 0;         // number of subkeys 
        DWORD    cbMaxSubKey;          // longest subkey size 
        DWORD    cchMaxClass;          // longest class string 
        DWORD    cValues;              // number of values for key 
        DWORD    cchMaxValue;          // longest value name 
        DWORD    cbMaxValueData;       // longest value data 
        DWORD    cbSecurityDescriptor; // size of security descriptor 
        FILETIME ftLastWriteTime;      // last write time 

        // Get the sub-key count. 
        lRetStatus = RegQueryInfoKey(
            hBaseKey,                // key handle 
            achClass,                // buffer for class name 
            &cchClassName,           // size of class string 
            NULL,                    // reserved 
            &cSubKeys,               // number of subkeys 
            &cbMaxSubKey,            // longest subkey size 
            &cchMaxClass,            // longest class string 
            &cValues,                // number of values for this key 
            &cchMaxValue,            // longest value name 
            &cbMaxValueData,         // longest value data 
            &cbSecurityDescriptor,   // security descriptor 
            &ftLastWriteTime);       // last write time 

        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("RegQueryInfoKey failed with error %ld\n", lRetStatus);
            break;
        }

        // Enumerate the subkeys
        if (cSubKeys)
        {
            for (DWORD iSubKey = 0; iSubKey < cSubKeys; iSubKey++)
            {
                TCHAR    achKey[MAX_PATH] = { 0 };
                DWORD    cbName = MAX_PATH;

                lRetStatus = RegEnumKeyEx(hBaseKey, iSubKey,
                    achKey,
                    &cbName,
                    NULL,
                    NULL,
                    NULL,
                    &ftLastWriteTime);
                if (lRetStatus == ERROR_SUCCESS)
                {
                    if (keyContains.empty() ||
                        std::string(achKey).find(keyContains) != std::string::npos
                        )
                        subKeys.push_back(achKey);
                    else
                        TRACE_INFO("Skipping key [%s] as it does not contain the pattern %s\n",
                                   achKey,
                                   keyContains.c_str());
                }
                else
                {
                    TRACE_ERROR("RegEnumKeyEx failed with error %ld\n", lRetStatus);
                }
            }
        }
        else
        {
            TRACE_WARNING("No subkeys found under the base key\n");
        }

    } while (false);

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetControlSets

Description : Retrieves the list of control sets under a give system hive

Parameters  : [in] SystemHiveName: Name of the system hive from where the control sets should retrieve.
              [in] controlSetPreference: Preferred Control Set
              [out] controlSets  : Filled with list of control set names.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetControlSets(
    const std::string& SystemHiveName,
    std::vector<std::string>& controlSets,
    const std::string& controlSetPreference)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!SystemHiveName.empty());

    LONG retStatus = ERROR_SUCCESS;
    HKEY hKeySysHive = NULL;
    controlSets.clear();

    do
    {
        retStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, SystemHiveName.c_str(), 0, KEY_READ, &hKeySysHive);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not open the reg key %s. RegOpenKeyEx error %ld\n",
                        SystemHiveName.c_str(),
                        retStatus);
            break;
        }

        DWORD preferredControlSet = 0;
        std::stringstream controlSetNum;
        std::string controlSetkeyContains = "ControlSet";

        if (boost::equals(controlSetPreference, std::string(RegistryConstants::VALUE_NAME_LASTKNOWNGOOD)))
        {
            retStatus = RegGetLastKnownGoodControlSetValue(RegistryConstants::VM_SYSTEM_HIVE_NAME, preferredControlSet);
            if (ERROR_SUCCESS != retStatus)
            {
                TRACE_ERROR("Could not get last known good control set value.");
            }
        }
        else if (boost::equals(controlSetPreference, std::string(RegistryConstants::VALUE_NAME_CURRENT)))
        {
            retStatus = RegGetCurrentControlSetValue(RegistryConstants::VM_SYSTEM_HIVE_NAME, preferredControlSet);
            if (ERROR_SUCCESS != retStatus)
            {
                TRACE_ERROR("Could not get current control set value.");
            }
        }

        if (retStatus == ERROR_SUCCESS &&
            !controlSetPreference.empty())
        {
            controlSetNum << RegistryConstants::CONTROL_SET_PREFIX
                << std::setfill('0')
                << std::setw(3)
                << preferredControlSet;

            controlSetkeyContains = controlSetNum.str();

            TRACE_INFO("%s ControlSet is %s\n", controlSetPreference.c_str(), controlSetkeyContains.c_str());
        }

        retStatus = RegGetSubKeys(hKeySysHive, controlSets, controlSetkeyContains);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not get ControlSet subkeys under %s\n",
                SystemHiveName.c_str());
            break;
        }
        
        if (controlSets.empty())
        {
            TRACE_ERROR("No ControlSet subkeys found under the reg key %s.\n",
                        SystemHiveName.c_str());
            retStatus = ERROR_FILE_NOT_FOUND;
            break;
        }

    } while (false);

    if (hKeySysHive) RegCloseKey(hKeySysHive);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegUpdateSystemHiveDWORDValue

Description : Updates the DWORD registry value with given value, and creates a backup registry value for existing key.

Parameters  : [in] systemHive: Name of the system hive under which the entry will be updated.
              [in] controlSet: Name of the control set under systemHive.
              [in] relativeKeyPath: relative key path
              [in] valueName: Registry DWORD value name that need to update
              [out] dwOldValue: Old DWORD value data of valueName.On failure or if key does not exist then the value is unchanged.
              [in] dwNewValue: New DWORD value data to be updated.
              [in, options] ignoreIfNotFound: if true, then retrun code will not be failure if specified relativepath key not found.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegUpdateSystemHiveDWORDValue(const std::string& systemHive,
    const std::string& controlSet,
    const std::string& relativeKeyPath,
    const std::string& valueName,
    DWORD & dwOldValue,
    DWORD dwNewValue,
    bool igonreIfNotFound)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!systemHive.empty());
    BOOST_ASSERT(!controlSet.empty());
    BOOST_ASSERT(!relativeKeyPath.empty());
    BOOST_ASSERT(!valueName.empty());

    LONG retStatus = ERROR_SUCCESS;

    std::string keyPath = systemHive +
        DIRECOTRY_SEPERATOR +
        controlSet +
        (boost::starts_with(relativeKeyPath, DIRECOTRY_SEPERATOR) ? "" : DIRECOTRY_SEPERATOR) +
        relativeKeyPath;

    do{
        TRACE_INFO("Registry path %s\n", keyPath.c_str());

        CRegKey svcKey;
        retStatus = svcKey.Open(HKEY_LOCAL_MACHINE, keyPath.c_str(), KEY_ALL_ACCESS);
        if (ERROR_SUCCESS != retStatus)
        {
            //
            // If ignoreIfNotFound is set to true then ignore the ERROR_FILE_NOT_FOUND error for Open call
            //
            if (ERROR_FILE_NOT_FOUND == retStatus &&
                igonreIfNotFound)
            {
                TRACE_WARNING("Registry entry %s not found\n", keyPath.c_str());
                retStatus = ERROR_SUCCESS;
                break;
            }

            TRACE_ERROR("Could not open the registry key. Error %ld\n", retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = svcKey.QueryDWORDValue(valueName.c_str(), dwOldValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not query the  DWORD value %s under %s. Error %ld\n", 
                valueName.c_str(),
                keyPath.c_str(),
                retStatus);

            break;
        }

        //
        // If the existing DWORD value and New DWORD values are same then no action is needed
        //
        if (dwOldValue == dwNewValue)
        {
            TRACE_INFO("Existing DWORD value is same as New DWORD value. Nothing to change.\n");
            break;
        }

        //
        // Update new value now
        //
        retStatus = svcKey.SetDWORDValue(valueName.c_str(), dwNewValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not change the DWORD value %s to %lu. Error %ld\n", 
                valueName.c_str(), 
                dwNewValue,
                retStatus);

            break;
        }

        TRACE_INFO("The DWORD registry value changed from %lu to %lu successfuly under %s.\n",
            dwOldValue,
            dwNewValue,
            keyPath.c_str());

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegUpdateSystemHiveDWORDValue

Description : Updates the DWORD registry value with given value, and creates a backup registry value for existing key.

Parameters  : [in] systemHive: Name of the system hive under which the entry will be updated.
              [in] controlSet: Name of the control set under systemHive.
              [in] relativeKeyPath: relative key path
              [in] valueName: Registry DWORD value name that need to update
              [out] dwOldValue: Old DWORD value data of valueName.On failure or if key does not exist then the value is unchanged.
              [in] dwNewValue: New DWORD value data to be updated.
              [in, options] ignoreIfNotFound: if true, then retrun code will not be failure if specified relativepath key not found.

Return code : ERROR_SUCCESS -> on success
win32 error   -> on failure.

*/
LONG RegAddOrUpdateSystemHiveDWORDValue(const std::string& systemHive,
    const std::string& controlSet,
    const std::string& relativeKeyPath,
    const std::string& valueName,
    DWORD & dwOldValue,
    DWORD dwNewValue,
    bool igonreIfNotFound)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!systemHive.empty());
    BOOST_ASSERT(!controlSet.empty());
    BOOST_ASSERT(!relativeKeyPath.empty());
    BOOST_ASSERT(!valueName.empty());

    LONG retStatus = ERROR_SUCCESS;

    std::string keyPath = systemHive +
        DIRECOTRY_SEPERATOR +
        controlSet +
        (boost::starts_with(relativeKeyPath, DIRECOTRY_SEPERATOR) ? "" : DIRECOTRY_SEPERATOR) +
        relativeKeyPath;

    do {
        TRACE_INFO("Registry path %s\n", keyPath.c_str());

        CRegKey svcKey;
        retStatus = svcKey.Open(HKEY_LOCAL_MACHINE, keyPath.c_str(), KEY_ALL_ACCESS);
        if (ERROR_SUCCESS != retStatus)
        {
            //
            // If ignoreIfNotFound is set to true then ignore the ERROR_FILE_NOT_FOUND error for Open call
            //
            if (ERROR_FILE_NOT_FOUND == retStatus &&
                igonreIfNotFound)
            {
                TRACE_WARNING("Registry entry %s not found\n", keyPath.c_str());
                retStatus = ERROR_SUCCESS;
                break;
            }

            TRACE_ERROR("Could not open the registry key. Error %ld\n", retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = svcKey.QueryDWORDValue(valueName.c_str(), dwOldValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_WARNING("Could not query the  DWORD value %s under %s. Error %ld\n",
                valueName.c_str(),
                keyPath.c_str(),
                retStatus);
        }
        else if (dwOldValue == dwNewValue)
        {
            TRACE_INFO("Existing DWORD value is same as New DWORD value. Nothing to change.\n");
            break;
        }

        //
        //Add or Update Value.
        //
        retStatus = svcKey.SetDWORDValue(valueName.c_str(), dwNewValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not change the DWORD value %s to %lu. Error %ld\n",
                valueName.c_str(),
                dwNewValue,
                retStatus);

            break;
        }

        TRACE_INFO("The DWORD registry value updated successfuly to %lu under %s.\n",
            dwNewValue,
            keyPath.c_str());

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegGetSystemHiveDWORDValue

Description : Gets the DWORD registry value with given value-name.

Parameters  : [in] systemHive: Name of the system hive.
              [in] controlSet: Name of the control set under systemHive.
              [in] relativeKeyPath: relative key path
              [in] valueName: Registry DWORD value name that need to retrieve
              [out] dwValue: DWORD value data of valueName. On failure or if key does not exist then the value is unchanged.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetSystemHiveDWORDValue(const std::string& systemHive,
    const std::string& controlSet,
    const std::string& relativeKeyPath,
    const std::string& valueName,
    DWORD & dwValue)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!systemHive.empty());
    BOOST_ASSERT(!controlSet.empty());
    BOOST_ASSERT(!relativeKeyPath.empty());
    BOOST_ASSERT(!valueName.empty());

    LONG retStatus = ERROR_SUCCESS;

    std::string keyPath = systemHive +
        DIRECOTRY_SEPERATOR +
        controlSet +
        (boost::starts_with(relativeKeyPath, DIRECOTRY_SEPERATOR) ? "" : DIRECOTRY_SEPERATOR) +
        relativeKeyPath;

    do{
        TRACE_INFO("Registry path %s\n", keyPath.c_str());

        CRegKey svcKey;
        retStatus = svcKey.Open(HKEY_LOCAL_MACHINE, keyPath.c_str(), KEY_ALL_ACCESS);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not open the registry key. Error %ld\n", retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = svcKey.QueryDWORDValue(valueName.c_str(), dwValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not query the  DWORD value %s under %s. Error %ld\n", 
                valueName.c_str(),
                keyPath.c_str(),
                retStatus);

            break;
        }

        TRACE_INFO("The DWORD registry value is %lu.\n", dwValue);

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegKeyExists

Description : Verifies does the keyPath exists.

Parameters  : [in] hKeyParent: parent key, for example: HKEY_LOCAL_MACHINE
              [in] keyPath: Relative key path from the parent key.

Return code : true  -> if key exists.
              false -> if key does not exists.
              exception will be thrown if a failure happens on accessing specified registry key.
*/
bool RegKeyExists(HKEY hKeyParent, const std::string& keyPath)
{
    TRACE_FUNC_BEGIN;
    bool bExist = false;

    BOOST_ASSERT(!keyPath.empty());
    BOOST_ASSERT(hKeyParent != NULL);

    TRACE_INFO("Opening %s key.\n", keyPath.c_str());

    CRegKey regKey;
    LONG lRet = regKey.Open(hKeyParent, keyPath.c_str(), KEY_READ);

    bExist = VERIFY_REG_STATUS(lRet, "Registry Key open failure");

    TRACE_FUNC_END;
    return bExist;
}

/*
Method      : RegChangeServiceStartType

Description : Sets the service related registry entries to reflect its start type as dwSvcStartType under
              the specified control set.

Parameters  : [in] systemHive: Name of the system hive undewhich the service start type will be updated.
              [in] controlSet: Name of the control set under systemHive.
              [in] serviceName: Name of the service
              [in] dwSvcStartType: Dword value indicating a valid service start type
              [out] oldDwSvcStartType: set with last service start type dword value.
              [in, options] ignoreIfNotFound: if true, then retrun code will not be failure if specified service
                  related service entries not found.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegChangeServiceStartType( const std::string& systemHive,
                                const std::string& controlSet,
                                const std::string& serviceName,
                                DWORD dwSvcStartType,
                                DWORD &oldDwSvcStartType,
                                bool ignoreIfNotFound)
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!systemHive.empty());
    BOOST_ASSERT(!controlSet.empty());
    BOOST_ASSERT(!serviceName.empty());

    LONG retStatus = ERROR_SUCCESS;
    
    std::string svcPath = systemHive + 
                          DIRECOTRY_SEPERATOR +
                          controlSet +
                          RegistryConstants::SERVICES +
                          serviceName;

    do{
        TRACE_INFO("Service registry path %s\n", svcPath.c_str());

        CRegKey svcKey;
        retStatus = svcKey.Open(HKEY_LOCAL_MACHINE, svcPath.c_str(), KEY_ALL_ACCESS);
        if (ERROR_SUCCESS != retStatus)
        {
            //
            // If ignoreIfNotFound is set to true then ignore the ERROR_FILE_NOT_FOUND error for Open call
            //
            if (ERROR_FILE_NOT_FOUND == retStatus && 
                ignoreIfNotFound)
            {
                TRACE_WARNING("Registry entry not found for the service: %s\n", svcPath.c_str());
                retStatus = ERROR_SUCCESS;
                break;
            }

            TRACE_ERROR("Could not open the service registry key. Error %ld\n", retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = svcKey.QueryDWORDValue(RegistryConstants::SERVICE_START_VALUE_NAME, oldDwSvcStartType);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not query the Start DWORD value for the service. Error %ld\n", retStatus);
            break;
        }

        //
        // Set the new Start DWORD value if its different than existing value.
        //
        if (oldDwSvcStartType == dwSvcStartType)
        {
            TRACE_INFO("The current service Start type value is same as the requested start type.\
                        Nothing to change.\n");
            break;
        }

        retStatus = svcKey.SetDWORDValue(RegistryConstants::SERVICE_START_VALUE_NAME, dwSvcStartType);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not change the Start DWORD value for the service. Error %ld\n", retStatus);
            break;
        }

        TRACE_INFO("The Service Start type value changed from %lu to %lu successfuly.\n",
                   oldDwSvcStartType,
                   dwSvcStartType );

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegSetServiceStartType

Description : Sets the list if services start types to specified values in all control sets under 
              the given systemHive.

Parameters  : [in] systemHive: Name of the system hive
              [in] serviceStarTypes: List of service name -> service start type dword values
              [in, opt] ignoreIfNotFound: Ignore if the registry entries of a service is not found.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegSetServiceStartType(const std::string& systemHive,
                            const std::map<std::string, DWORD> serviceStartTypes,
                            bool ignoreIfNotFound
                            )
{
    TRACE_FUNC_BEGIN;
    BOOST_ASSERT(!systemHive.empty());

    LONG lRetStatus = ERROR_SUCCESS;

    std::stringstream   ssError;

    do{
        if (serviceStartTypes.empty())
        {
            TRACE_INFO("Empty service list. Nothing to do.\n");
            break;
        }

        std::vector<std::string> contolSets;
        lRetStatus = RegGetControlSets(systemHive, contolSets);
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not get ControlSets from Source VM system hive.\n");
            break;
        }

        std::map<std::string, DWORD>::const_iterator iterSvc = serviceStartTypes.begin();
        for (;
            iterSvc != serviceStartTypes.end();
            iterSvc++)
        {

            TRACE("Changing service start type for : %s\n", iterSvc->first.c_str());

            for (size_t iCtrlSet = 0; iCtrlSet < contolSets.size(); iCtrlSet++)
            {
                DWORD dwOldSvcStartType = 0;

                lRetStatus = RegChangeServiceStartType(systemHive,
                                                     contolSets[iCtrlSet],
                                                     iterSvc->first,
                                                     iterSvc->second,
                                                     dwOldSvcStartType,
                                                     ignoreIfNotFound);

                if (ERROR_SUCCESS != lRetStatus)
                {
                    ssError.str("");
                    ssError << "Could not set the service start type on controlset " << 
                            contolSets[iCtrlSet] << "error: " << lRetStatus << std::endl;

                    TRACE_WARNING(ssError.str().c_str());
                }
            }
        }
    } while (false);

    TRACE_FUNC_END;
    return ERROR_SUCCESS;
}

/*
Method      : RegGetAgentInstallPath

Description : Reads the Source-Agent install path from its registry entries under the
              specified software registry hive.

Parameters  : [in] SoftwareHive: Name of the sotware hive
              [out] agentInstallPath: Filled with agent install path on success

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetAgentInstallPath(const std::string& SoftwareHive, std::string& agentInstallPath)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;
    BOOST_ASSERT(!SoftwareHive.empty());

    do
    {
        CRegKey svSystemsKey;
        
        std::string strSvSystemsKey = SoftwareHive + RegistryConstants::INMAGE_INSTALL_PATH_X32;

        lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, strSvSystemsKey.c_str(), KEY_READ);
        if (ERROR_FILE_NOT_FOUND == lRetStatus)
        {
            // Try 64bit path
            strSvSystemsKey = SoftwareHive + RegistryConstants::INMAGE_INSTALL_PATH_X64;

            lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, strSvSystemsKey.c_str(), KEY_READ);
        }
        
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not open registry key %s. Error %ld\n", strSvSystemsKey.c_str(), lRetStatus);
            break;
        }

        lRetStatus = RegGetStringValue(svSystemsKey, RegistryConstants::VALUE_INMAGE_INSTALL_DIR, agentInstallPath);
        
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not read %s value under the key %s. Error %ld\n",
                        RegistryConstants::VALUE_INMAGE_INSTALL_DIR, 
                        strSvSystemsKey.c_str(), 
                        lRetStatus);
            break;
        }

    } while (false);

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetOSCurrentVersion

Description : Reads the windows OS version information from software registry hive.

Parameters  : [in] SoftwareHive: Name of the software hive
              [out] osVersion: filled with string representation of OS version info.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetOSCurrentVersion(const std::string& SoftwareHive, std::string& osVersion)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    do{
        std::string strCurrVerPath = SoftwareHive + RegistryConstants::WIN_CURRENT_VERSION_KEY;

        CRegKey winCurrVerkey;
        lRetStatus = winCurrVerkey.Open(HKEY_LOCAL_MACHINE, strCurrVerPath.c_str(), KEY_READ);
        
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not open registry key %s. Error %ld\n",
                        strCurrVerPath.c_str(), 
                        lRetStatus);

            break;
        }

        lRetStatus = RegGetStringValue(winCurrVerkey, RegistryConstants::VALUE_CURRENT_VERSION, osVersion);
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not read %s value under the key %s. Error %ld\n",
                        RegistryConstants::VALUE_CURRENT_VERSION,
                        strCurrVerPath.c_str(),
                        lRetStatus);

            break;
        }

    } while (false);

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegAddBootUpScript

Description : Adds the required registry values to setup boot up script.

Parameters  : [in] BootupScriptRegKeyPath : Path of the bootup script registry key.
              [in] Script : Script command
              [in] ScriptParams : Script command parameters
              [in] SrcSystemVolName : Source system volume name
              [in] osVersion : OS version information
              [in] IsPowerShellScript : Is the bootup script is a powershell script.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegAddBootUpScript(const std::string& BootupScriptRegKeyPath,
                        const std::string& Script,
                        const std::string& ScriptParams,
                        const std::string& SrcSystemVolName,
                        const OSVersion& osVersion,
                        bool IsPowerShellScript)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;
    BOOST_ASSERT(!BootupScriptRegKeyPath.empty());
    BOOST_ASSERT(!Script.empty());

    do
    {		
        CRegKey bootupScriptKey;

        //
        // Set script and its parameters under BootupScriptRegKeyPath key
        //
        lRetStatus = bootupScriptKey.Create( HKEY_LOCAL_MACHINE,
                                             BootupScriptRegKeyPath.c_str(), 
                                             0,
                                             REG_OPTION_NON_VOLATILE );
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not create reg key %s. Error %ld\n",
                         BootupScriptRegKeyPath.c_str(),
                         lRetStatus );
            
            break;
        }

        std::map<std::string, std::string> mapStringValues;

        mapStringValues[RegistryConstants::VALUE_NAME_SOM_ID] = "Local";
        mapStringValues[RegistryConstants::VALUE_NAME_GPO_ID] = "LocalGPO";
        mapStringValues[RegistryConstants::VALUE_NAME_GPONAME] = "Local Group Policy";
        mapStringValues[RegistryConstants::VALUE_NAME_DISAPLAYNAME] = "Local Group Policy";
        mapStringValues[RegistryConstants::VALUE_NAME_FILESYSPATH] = SrcSystemVolName +
            std::string("Windows\\System32\\GroupPolicy\\Machine");

        lRetStatus = RegSetStringValues(bootupScriptKey, mapStringValues);
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not set all string values under the registry key %s.\n",
                        BootupScriptRegKeyPath.c_str());

            break;
        }

        if (osVersion >= OSVersion(6, 1))
        {
            //
            // Below key is required if the OS version is greater than or equla to 6.1
            //                                                (Windows Server 2008 R2)
            //
            std::map<std::string, DWORD> mapDWordValues;
            mapDWordValues[RegistryConstants::VALUE_NAME_SCRIPT_ORDER] = 1;

            lRetStatus = RegSetDWORDValues(bootupScriptKey, mapDWordValues);
            if (ERROR_SUCCESS != lRetStatus)
            {
                TRACE_ERROR("Could not set all DWORD values under the registry key %s.\n",
                    BootupScriptRegKeyPath.c_str());

                break;
            }
        }
        bootupScriptKey.Flush();
        bootupScriptKey.Close();
        mapStringValues.clear();

        //
        // Set script and its parameters under BootupScriptRegKeyPath\0 subkey
        //
        std::string BootupScriptRegKeySubPath = BootupScriptRegKeyPath + "\\0";
        
        lRetStatus = bootupScriptKey.Create(HKEY_LOCAL_MACHINE,
                                            BootupScriptRegKeySubPath.c_str(),
                                            0,
                                            REG_OPTION_NON_VOLATILE);
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not create reg key %s. Error %ld\n",
                BootupScriptRegKeySubPath.c_str(),
                lRetStatus);

            break;
        }

        mapStringValues[RegistryConstants::VALUE_NAME_SCRIPT] = Script;
        mapStringValues[RegistryConstants::VALUE_NAME_SCRIPT_PARAMS] = ScriptParams;
        lRetStatus = RegSetStringValues(bootupScriptKey, mapStringValues);
        if (ERROR_SUCCESS != lRetStatus)
        {
            TRACE_ERROR("Could not set all string values under the registry key %s.\n",
                BootupScriptRegKeyPath.c_str());

            break;
        }

        if (osVersion >= OSVersion(6,1))
        {
            //
            // Below key is required if the OS version is greater than or equla to 6.1
            //                                                (Windows Server 2008 R2)
            //
            std::map<std::string, DWORD> mapDWordValues;
            mapDWordValues[RegistryConstants::VALUE_NAME_IS_POWERSHELL] = IsPowerShellScript ? 1 : 0;

            lRetStatus = RegSetDWORDValues(bootupScriptKey, mapDWordValues);
            if (ERROR_SUCCESS != lRetStatus)
            {
                TRACE_ERROR("Could not set all DWORD values under the registry key %s.\n",
                            BootupScriptRegKeySubPath.c_str());

                break;
            }
        }

        bootupScriptKey.Flush();

    } while (false);

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetSVSystemDWORDValue

Description : Wrapper function to get DWORD value under "SV Systems" registry key.

Parameters  : [in] valueName : Name of the registry value.
              [in, out] valueData : Filled with DWORD value of valueName.
              [in, Optional] relativeKeyPath : Relative key path under "SV Systems" key. Empty string
                   indicate that the valueName needs to update directly under "SV Systems" key.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetSVSystemDWORDValue(const std::string& valueName,
    DWORD& valueData,
    const std::string& relativeKeyPath)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    std::stringstream errorStream;

    do
    {
        CRegKey svSystemsKey;

        std::string svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
            + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X32)
            + (relativeKeyPath.empty() ? "" : relativeKeyPath);

        lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        if (ERROR_FILE_NOT_FOUND == lRetStatus)
        {
            svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X64)
                + (relativeKeyPath.empty() ? "" : relativeKeyPath);

            lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        }

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not open " << svSystemsPath
                << ". Error " << lRetStatus;

            break;
        }

        lRetStatus = svSystemsKey.QueryDWORDValue(valueName.c_str(), valueData);

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not get "
                << valueName
                << " value under the key " << svSystemsPath
                << ". Error " << lRetStatus;
        }

    } while (false);

    if (ERROR_SUCCESS != lRetStatus)
    {
        SetLastErrorMsg(errorStream.str());

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegSetSVSystemDWORDValue

Description : Wrapper function to set DWORD value under "SV Systems" registry key.

Parameters  : [in] valueName : Name of the registry value.
              [in] valueData : Data to be set to the registry value.
              [in, Optional] relativeKeyPath : Relative key path under "SV Systems" key. Empty string
                             indicate that the valueName needs to update directly under "SV Systems" key.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegSetSVSystemDWORDValue(const std::string& valueName, 
    DWORD valueData, 
    const std::string& relativeKeyPath)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    std::stringstream errorStream;

    do
    {
        CRegKey svSystemsKey;

        std::string svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
            + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X32)
            + (relativeKeyPath.empty() ? "" : relativeKeyPath);

        lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        if (ERROR_FILE_NOT_FOUND == lRetStatus)
        {
            svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X64)
                + (relativeKeyPath.empty() ? "" : relativeKeyPath);

            lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        }

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not open " << svSystemsPath
                << ". Error " << lRetStatus;

            break;
        }

        lRetStatus = svSystemsKey.SetDWORDValue(valueName.c_str(), valueData);

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not set "
                << valueName << " = " << valueData
                << " value under the key " << svSystemsPath
                << ". Error " << lRetStatus;
        }

    } while (false);

    if (ERROR_SUCCESS != lRetStatus)
    {
        SetLastErrorMsg(errorStream.str());

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegSetSVSystemStringValue

Description : Wrapper function to set String/MultiString value under "SV Systems" registry key.

Parameters  : [in] valueName : Name of the registry value.
              [in] valueData : Data to be set to the registry value.
              [in, Optional] isMultiString : Pass true if the registry value is multustring, othewise false
              [in, Optional] relativeKeyPath : Relative key path under "SV Systems" key. Empty string
                             indicate that the valueName needs to update directly under "SV Systems" key.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegSetSVSystemStringValue(const std::string& valueName, 
    const std::string& valueData, 
    bool isMultiString, 
    const std::string& relativeKeyPath)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    std::stringstream errorStream;

    do
    {
        CRegKey svSystemsKey;

        std::string svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
            + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X32)
            + (relativeKeyPath.empty() ? "" : relativeKeyPath);

        lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        if (ERROR_FILE_NOT_FOUND == lRetStatus)
        {
            svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X64)
                + (relativeKeyPath.empty() ? "" : relativeKeyPath);

            lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        }

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not open " << svSystemsPath
                << ". Error " << lRetStatus;
            break;
        }

        lRetStatus = isMultiString ? 
            svSystemsKey.SetMultiStringValue(valueName.c_str(), valueData.c_str()) :
            svSystemsKey.SetStringValue(valueName.c_str(), valueData.c_str() );

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not set "
                << valueName << " = " << valueData
                << " value under the key " << svSystemsPath
                << ". Error " << lRetStatus;
        }

    } while (false);

    if (ERROR_SUCCESS != lRetStatus)
    {
        SetLastErrorMsg(errorStream.str());

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetSVSystemStringValue

Description : Wrapper function to Get String value under "SV Systems" registry key.

Parameters  : [in] valueName : Name of the registry value.
              [out] valueData : Registry value data will be filled in it on success.
              [in, Optional] relativeKeyPath : Relative key path under "SV Systems" key. Empty string
                   indicate that the valueName needs to update directly under "SV Systems" key.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetSVSystemStringValue(const std::string& valueName,
    std::string& valueData,
    const std::string& relativeKeyPath)
{
    TRACE_FUNC_BEGIN;
    LONG lRetStatus = ERROR_SUCCESS;

    std::stringstream errorStream;

    do
    {
        CRegKey svSystemsKey;

        std::string svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
            + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X32)
            + (relativeKeyPath.empty() ? "" : relativeKeyPath);

        lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        if (ERROR_FILE_NOT_FOUND == lRetStatus)
        {
            svSystemsPath = RegistryConstants::VM_SOFTWARE_HIVE_NAME
                + std::string(RegistryConstants::INMAGE_SV_SYSTEMS_X64)
                + (relativeKeyPath.empty() ? "" : relativeKeyPath);

            lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
        }

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not open " << svSystemsPath
                << ". Error " << lRetStatus;
            break;
        }

        lRetStatus = RegGetStringValue(svSystemsKey.m_hKey,valueName,valueData);

        if (ERROR_SUCCESS != lRetStatus)
        {
            errorStream << "Could not get "
                << valueName
                << " value under the key " << svSystemsPath
                << ". Error " << lRetStatus;
        }

    } while (false);

    if (ERROR_SUCCESS != lRetStatus)
    {
        SetLastErrorMsg(errorStream.str());

        TRACE_ERROR("%s\n", errorStream.str().c_str());
    }

    TRACE_FUNC_END;
    return lRetStatus;
}

/*
Method      : RegGetCurrentControlSetValue

Description : Gets the current control.

Parameters  : [in] systemHiveName : Name of the system hive.
              [out] dwValue : Filled with current controlset number.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetCurrentControlSetValue(const std::string& systemHiveName, DWORD & dwValue)
{
    LONG retStatus = ERROR_SUCCESS;

    std::string keyPath = systemHiveName +
        DIRECOTRY_SEPERATOR + RegistryConstants::SYSTEM_SELECT_KEY;

    do{
        TRACE_INFO("Registry path %s\n", keyPath.c_str());

        CRegKey selectKey;
        retStatus = selectKey.Open(HKEY_LOCAL_MACHINE, keyPath.c_str(), KEY_READ);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not open the registry key %s. Error %ld\n", keyPath.c_str(), retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = selectKey.QueryDWORDValue(RegistryConstants::VALUE_NAME_CURRENT, dwValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not query the Current DWORD value under %s. Error %ld\n",
                keyPath.c_str(),
                retStatus);

            break;
        }

        TRACE_INFO("The Current Control set is %lu.\n", dwValue);

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

/*
Method      : RegGetLastKnownGoodControlSetValue

Description : Gets the LastKnownGood control number.

Parameters  : [in] systemHiveName : Name of the system hive.
              [out] dwValue : Filled with current controlset number.

Return code : ERROR_SUCCESS -> on success
              win32 error   -> on failure.

*/
LONG RegGetLastKnownGoodControlSetValue(const std::string& systemHiveName, DWORD & dwValue)
{
    LONG retStatus = ERROR_SUCCESS;

    std::string keyPath = systemHiveName +
        DIRECOTRY_SEPERATOR + RegistryConstants::SYSTEM_SELECT_KEY;

    do {
        TRACE_INFO("Registry path %s\n", keyPath.c_str());

        CRegKey selectKey;
        retStatus = selectKey.Open(HKEY_LOCAL_MACHINE, keyPath.c_str(), KEY_READ);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not open the registry key %s. Error %ld\n", keyPath.c_str(), retStatus);
            break;
        }

        //
        // Get the existing DWORD value
        //
        retStatus = selectKey.QueryDWORDValue(RegistryConstants::VALUE_NAME_LASTKNOWNGOOD, dwValue);
        if (ERROR_SUCCESS != retStatus)
        {
            TRACE_ERROR("Could not query the last known good DWORD value under %s. Error %ld\n",
                keyPath.c_str(),
                retStatus);

            break;
        }

        TRACE_INFO("The last known good set is %lu.\n", dwValue);

    } while (false);

    TRACE_FUNC_END;
    return retStatus;
}

}
