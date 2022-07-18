#include "registry.h"
#include "svtypes.h"
#include "portable.h"
#include "portablehelpers.h"
#ifndef _WINDOWS_
#include <Windows.h>
#endif
#include <atlbase.h>

#define MAX_REG_KEY_STR_LENGTH_INC_NULL  256

SVERROR createKey(const std::string& key)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, key.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        dwResult = cregkey.Create( HKEY_LOCAL_MACHINE, key.c_str() );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::Create failed for key %s with error %d\n", key.c_str(), dwResult);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Successfully created key %s in registry\n", key.c_str()) ;
            bRet = SVS_OK ;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "The key %s already present in registry\n", key.c_str());
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR deleteKey(const std::string& root, const std::string& subKey)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_ERROR, "The root %s is not present in the registry to delete subkey %s. Error %d\n", root.c_str(), subKey.c_str(), dwResult) ;
    }
    else
    {
        dwResult = cregkey.DeleteSubKey(subKey.c_str()) ;
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to remove the key %s under root %s from registry. Error %d\n", subKey.c_str(), root.c_str(), dwResult) ;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Successfully deleted root %s subkey %s in registry\n", root.c_str(), subKey.c_str());
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR deleteValue(const std::string& root, const std::string value)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_ERROR, "The root %s is not present in the registry to delete value %s. Error %d\n", root.c_str(), value.c_str(), dwResult) ;
    }
    else
    {
        dwResult = cregkey.DeleteValue(value.c_str()) ;
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to remove the value %s under root %s from registry. Error %d\n", value.c_str(), root.c_str(), dwResult) ;
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Successfully deleted root %s value %s in registry\n", root.c_str(), value.c_str());
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR getDwordValue(const std::string& root, const std::string value, SV_LONGLONG& data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        DWORD dwData ;
        dwResult = cregkey.QueryDWORDValue( value.c_str(), dwData );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::QueryDWORDValue for root %s value %s failed with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
            data = (SV_LONGLONG) dwData ;
            DebugPrintf(SV_LOG_DEBUG, "Successfully queried root %s value %s data %ld in registry\n", root.c_str(), value.c_str(), dwData);
            bRet = SVS_OK ;
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR setDwordValue(const std::string& root, const std::string& value, SV_LONGLONG data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        dwResult = cregkey.SetDWORDValue( value.c_str(), (DWORD) data );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::SetDWORDValue for root %s value %s failed with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Successfully set root %s value %s data %lld in registry\n", root.c_str(), value.c_str(), data);
            bRet = SVS_OK ;
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR getStringValue(const std::string& root, const std::string value, std::string& data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        char szResult[1024] ;
        DWORD length = 1024 ;
        dwResult = cregkey.QueryStringValue( value.c_str(), szResult, &length );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::QueryStringValue for root %s value %s failed with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
            data = szResult ;
            DebugPrintf(SV_LOG_DEBUG, "Successfully queried root %s value %s data %s in registry\n", root.c_str(), value.c_str(), data.c_str());
            bRet = SVS_OK ;
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}


SVERROR getStringValue(const std::string& root, const std::string value, std::string& data, USHORT accessMode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    SVERROR  bRet = SVE_FAIL;
    DWORD dwResult = ERROR_SUCCESS;
    CRegKey cregkey;
    dwResult = cregkey.Open(HKEY_LOCAL_MACHINE, root.c_str(), KEY_READ | accessMode);
    if (dwResult != ERROR_SUCCESS)
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        char szResult[1024];
        DWORD length = 1024;
        dwResult = cregkey.QueryStringValue(value.c_str(), szResult, &length);
        if (dwResult != ERROR_SUCCESS)
        {
            DebugPrintf(SV_LOG_DEBUG, "CRegKey::QueryStringValue for root %s value %s failed with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
            data = szResult;
            DebugPrintf(SV_LOG_DEBUG, "Successfully queried root %s value %s data %s in registry\n", root.c_str(), value.c_str(), data.c_str());
            bRet = SVS_OK;
        }
        cregkey.Close();
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}


SVERROR getStringValueEx(const std::string& root, const std::string value, std::string& data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
	
	HKEY hKey;
	DWORD dwResult = 0;
	dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,TEXT(root.c_str()),0,KEY_ALL_ACCESS|KEY_WOW64_64KEY,&hKey);
	if(dwResult != ERROR_SUCCESS)
	{
		DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
	}
	else
	{
		DWORD dwSize = 0;
		char* _data = NULL;
        dwResult = RegQueryValueEx(hKey, value.c_str(), NULL, NULL, NULL, &dwSize);
        if (ERROR_SUCCESS == dwResult)
		{
			_data = (char*)malloc(dwSize);
            dwResult = RegQueryValueEx(hKey, value.c_str(), NULL, NULL, (LPBYTE)_data, &dwSize);
			if(ERROR_SUCCESS == dwResult)
			{
				data = std::string(_data);
				free(_data);
				bRet = SVS_OK;
                DebugPrintf(SV_LOG_DEBUG, "Successfully queried root %s value %s data %s in registry\n", root.c_str(), value.c_str(), data.c_str());
			}
			else
			{
                DebugPrintf(SV_LOG_ERROR, "RegQueryValueEx for root %s value %s Failed. Error %ld\n", root.c_str(), value.c_str(), dwResult);
			}
		}
		else
		{
            DebugPrintf(SV_LOG_ERROR, "RegQueryValueEx for root %s value %s Failed. Error %ld \n", root.c_str(), value.c_str(), dwResult);
		}	
	}
	RegCloseKey(hKey);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR getMultiStringValue(const std::string& root, const std::string value, std::list<std::string>& data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        DWORD length ;
        dwResult = cregkey.QueryMultiStringValue( value.c_str(), NULL, &length );
        if( dwResult != ERROR_SUCCESS || length == 0 )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::QueryMultiStringValue failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
			char *buffer;
			buffer = new (std::nothrow) char[length];
			if( buffer != NULL )
			{
				dwResult = cregkey.QueryMultiStringValue( value.c_str(), buffer, &length );
				char *tempBuffer = buffer;
				while (*buffer != '\0') 
				{
					data.push_back(buffer) ;
					buffer = buffer + lstrlen(buffer);
					buffer++; 
				}
				if( tempBuffer != NULL )
				{
					delete[] tempBuffer;
				}
             
                DebugPrintf(SV_LOG_DEBUG, "Successfully MultiStringValue queried root %s value %s value %s in registry\n", root.c_str(), value.c_str(), buffer);
			}
            bRet = SVS_OK ;
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}


SVERROR getBinaryValue( const std::string& root, const std::string& value, std::vector<unsigned char>& data)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  retStatus = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
		DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
        DWORD length ;
        dwResult = cregkey.QueryBinaryValue( value.c_str(), NULL, &length );
        if( dwResult == ERROR_SUCCESS && length > 0 )
        {
			data.resize(length); 
			dwResult = cregkey.QueryBinaryValue( value.c_str(), &data[0], &length );
			if( dwResult == ERROR_SUCCESS )
			{
				retStatus = SVS_OK;
                DebugPrintf(SV_LOG_DEBUG, "Successfully queried root %s value %s in registry\n", root.c_str(), value.c_str());

			}
            else
            {
                DebugPrintf(SV_LOG_ERROR, "Get binary value failed for root %s value %s with error %d\n", root.c_str(), value.c_str(), dwResult);
            }
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "Get binary value failed for root %s value %s with error %d\n", root.c_str(), value.c_str(), dwResult);
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return retStatus ;
}

SVERROR setStringValue(const std::string& root, const std::string value, const std::string& data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
       dwResult = cregkey.SetStringValue ( value.c_str(), data.c_str() );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::SetStringValue failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Successfully set StringValue root %s value %s data %s in registry\n", root.c_str(), value.c_str(), data.c_str()) ;
            bRet = SVS_OK ;
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR setBinaryValue( const std::string& root, const std::string& value, std::string data)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR  retStatus = SVE_FAIL;

    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, root.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
		DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for root %s value %s with error %ld\n", root.c_str(), value.c_str(), dwResult);
    }
    else
    {
		std::vector<unsigned char> tdata( data.size() );
        DWORD length = data.size();
		for( int i = 0; i < data.size(); i++ )
		{
			tdata[i] = data.at(i);
		}
		 dwResult = cregkey.SetBinaryValue( value.c_str(), &tdata[0], length );
        if( dwResult != ERROR_SUCCESS )
        {
            DebugPrintf(SV_LOG_ERROR, "CRegKey::setBinaryValue failed for root %s value %s with error %d.\n", root.c_str(), value.c_str(), dwResult);
        }
		else
		{
            DebugPrintf(SV_LOG_DEBUG, "Successfully Set BinaryValue value for root %s value %s data %s in registry\n", root.c_str(), value.c_str(), data.c_str());
            retStatus = SVS_OK ;		
		}
        cregkey.Close() ;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return retStatus ;
}

SVERROR getSubKeysList(const std::string& key, std::list<std::string>& subkeys)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s %s\n", FUNCTION_NAME, key.c_str());
    SVERROR  bRet = SVE_FAIL ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open(HKEY_LOCAL_MACHINE, key.c_str());
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for key %s with error %ld\n", key.c_str(), dwResult);
    }
    else
    {
        char subKey[MAX_REG_KEY_STR_LENGTH_INC_NULL];
        DWORD subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
        RtlZeroMemory(subKey, MAX_REG_KEY_STR_LENGTH_INC_NULL);
        if( (dwResult = cregkey.EnumKey(0,  subKey, &subkeyLength)) == ERROR_MORE_DATA ||
            dwResult != ERROR_NO_MORE_ITEMS)
        {
            int index = 1 ;
            subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
            subkeys.push_back(subKey) ;
            while( (dwResult = cregkey.EnumKey(index,  subKey, &subkeyLength)) == ERROR_MORE_DATA || 
                dwResult != ERROR_NO_MORE_ITEMS)
            {
                index++ ;
                subkeys.push_back(subKey) ;
                RtlZeroMemory(subKey, MAX_REG_KEY_STR_LENGTH_INC_NULL);
                subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
            }
            bRet = SVS_OK ;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "EnumKey failed for key %s with error %d\n", key.c_str(), dwResult);
        }
        cregkey.Close() ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR getSubKeysListEx(const std::string& key, std::list<std::string>& subkeys, USHORT accessMode)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s %s\n", FUNCTION_NAME, key.c_str());
    SVERROR  bRet = SVE_FAIL;
    DWORD dwResult = ERROR_SUCCESS;
    CRegKey cregkey;
    
    dwResult = cregkey.Open(HKEY_LOCAL_MACHINE, key.c_str(), KEY_READ | accessMode);
    if (dwResult != ERROR_SUCCESS)
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for key %s with error %ld\n", key.c_str(), dwResult);
    }
    else
    {
        char subKey[MAX_REG_KEY_STR_LENGTH_INC_NULL];
        DWORD subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
        RtlZeroMemory(subKey, MAX_REG_KEY_STR_LENGTH_INC_NULL);
        if ((dwResult = cregkey.EnumKey(0, subKey, &subkeyLength)) == ERROR_MORE_DATA ||
            dwResult != ERROR_NO_MORE_ITEMS)
        {
            int index = 1;
            subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
            subkeys.push_back(subKey);
            while ((dwResult = cregkey.EnumKey(index, subKey, &subkeyLength)) == ERROR_MORE_DATA ||
                dwResult != ERROR_NO_MORE_ITEMS)
            {
                index++;
                subkeys.push_back(subKey);
                RtlZeroMemory(subKey, MAX_REG_KEY_STR_LENGTH_INC_NULL);
                subkeyLength = MAX_REG_KEY_STR_LENGTH_INC_NULL;
            }
            bRet = SVS_OK;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "EnumKey failed for key %s with error %d\n", key.c_str(), dwResult);
        }
        cregkey.Close();
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}


bool isKeyAvailable(const std::string& key)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    bool  bRet = false ;
    DWORD dwResult = ERROR_SUCCESS ;
    CRegKey cregkey ;
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, key.c_str() );
    if( dwResult != ERROR_SUCCESS )
    {
        DebugPrintf(SV_LOG_WARNING, "CRegKey::Open failed for %s with error %ld\n", key.c_str(), dwResult);
    }
    else
    {
        cregkey.Close() ;
        bRet = true;    
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SV_LONG SetSVSystemDWORDValue(const std::string& valueName,
	SV_ULONG valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
	
	SV_LONG lRetStatus = ERROR_SUCCESS;
	std::stringstream errorStream;
	do
	{
		CRegKey svSystemsKey;

		std::string svSystemsPath = hiveRoot
			+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X32)
			+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

		lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		if (ERROR_FILE_NOT_FOUND == lRetStatus)
		{
			svSystemsPath = hiveRoot
				+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X64)
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
		DebugPrintf(SV_LOG_ERROR, "%s\n", errorStream.str().c_str());
	}

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return lRetStatus;
}

SV_LONG GetSVSystemDWORDValue(const std::string& valueName,
	SV_ULONG& valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	SV_LONG lRetStatus = ERROR_SUCCESS;
	std::stringstream errorStream;
	do
	{
		CRegKey svSystemsKey;

		std::string svSystemsPath = hiveRoot
			+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X32)
			+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

		lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		if (ERROR_FILE_NOT_FOUND == lRetStatus)
		{
			svSystemsPath = hiveRoot
				+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X64)
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
				<< " value under the key " << svSystemsPath;
		}

	} while (false);

	if (ERROR_SUCCESS != lRetStatus)
	{
		DebugPrintf(SV_LOG_WARNING, "%s\n", errorStream.str().c_str());
	}

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return lRetStatus;
}

SV_LONG SetSVSystemStringValue(const std::string& valueName,
	const std::string& valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	SV_LONG lRetStatus = ERROR_SUCCESS;
	std::stringstream errorStream;
	do
	{
		CRegKey svSystemsKey;

		std::string svSystemsPath = hiveRoot
			+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X32)
			+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

		lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		if (ERROR_FILE_NOT_FOUND == lRetStatus)
		{
			svSystemsPath = hiveRoot
				+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X64)
				+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

			lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		}

		if (ERROR_SUCCESS != lRetStatus)
		{
			errorStream << "Could not open " << svSystemsPath
				<< ". Error " << lRetStatus;

			break;
		}

		lRetStatus = svSystemsKey.SetStringValue(valueName.c_str(), valueData.c_str());

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
		DebugPrintf(SV_LOG_ERROR, "%s\n", errorStream.str().c_str());
	}

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return lRetStatus;
}

LONG GetStringValue(HKEY hKey, const std::string& valueName, std::string& valueData)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
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
			DebugPrintf(SV_LOG_ERROR, "Out of memory\n");
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
				DebugPrintf(SV_LOG_ERROR, "Out of memory\n");
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

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return lRetStatus;
}

SV_LONG GetSVSystemStringValue(const std::string& valueName,
	std::string& valueData,
	const std::string& hiveRoot,
	const std::string& relativeKeyPath)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

	LONG lRetStatus = ERROR_SUCCESS;
	std::stringstream errorStream;
	do
	{
		CRegKey svSystemsKey;

		std::string svSystemsPath = hiveRoot
			+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X32)
			+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

		lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		if (ERROR_FILE_NOT_FOUND == lRetStatus)
		{
			svSystemsPath = hiveRoot
				+ std::string(REG_SV_SYSTEMS::SV_SYSTEM_KEY_X64)
				+ (relativeKeyPath.empty() ? "" : relativeKeyPath);

			lRetStatus = svSystemsKey.Open(HKEY_LOCAL_MACHINE, svSystemsPath.c_str());
		}

		if (ERROR_SUCCESS != lRetStatus)
		{
			errorStream << "Could not open " << svSystemsPath
				<< ". Error " << lRetStatus;

			break;
		}

		lRetStatus = GetStringValue(svSystemsKey.m_hKey, valueName, valueData);
		if (ERROR_SUCCESS != lRetStatus)
		{
			errorStream << "Could not get "
				<< valueName
				<< " value under the key " << svSystemsPath;
		}

	} while (false);

	if (ERROR_SUCCESS != lRetStatus)
	{
		DebugPrintf(SV_LOG_WARNING, "%s\n", errorStream.str().c_str());
	}

	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
	return lRetStatus;
}

#include <iostream>

void TestRegistryFunctions()
{
    std::cout<<"Testing the registry functions\n" ;
    std::cout<<"Creating a key in the registry\n" ;
    std::string root, keyName ;
    std::list<std::string> subkeys ;
    getSubKeysList("SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}", subkeys) ;
    std::list<std::string>::const_iterator subkeysIter = subkeys.begin() ;
    while( subkeysIter != subkeys.end() )
    {
        std::cout<<*subkeysIter <<std::endl ;
        subkeysIter++ ;
    }

    createKey("SOFTWARE\\TEST") ;
    setDwordValue("SOFTWARE\\TEST", "value", 100) ;
    SV_LONGLONG value ;
    getDwordValue("SOFTWARE\\TEST", "value", value) ;
    setStringValue("SOFTWARE\\TEST", "string", "string") ;
    std::string valueStr ;
    getStringValue("SOFTWARE\\TEST", "string", valueStr) ;
    deleteValue("SOFTWARE\\TEST", "value") ;
    deleteValue("SOFTWARE\\TEST", "string") ;
    deleteKey("SOFTWARE", "TEST") ;
}