#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <string>
#include <sstream>
#include <atlbase.h>
#include <Security.h>
#include "host.h"
#include "genericwmi.h"
#include "portable.h"
#include "logger.h"

GenericWMI::GenericWMI(WmiRecordProcessor *pwrp)
    : m_pWmiRecordProcessor(pwrp),
    m_init(false)
{
    if (m_pWmiRecordProcessor != NULL)
    {
        BOOST_ASSERT(m_pWmiRecordProcessor->m_genericWmiPtr == NULL);
        m_pWmiRecordProcessor->m_genericWmiPtr = this;
    }
}

/*
* FUNCTION NAME     : init
*
* DESCRIPTION       : Calls init method with default wmi provider
*
* INPUT PARAMETERS  : NONE
*
* OUTPUT PARAMETERS : NONE
*
* return value      : Returns SVS_OK if init method returns true
*                     otherwise returns SVS_FALSE
*
*/
SVERROR GenericWMI::init()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVERROR  bRet = SVS_FALSE;
    bRet = init("root\\CIMV2");

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bRet;
}

/*
* FUNCTION NAME     : init
*
* DESCRIPTION       : Uses given wmi provider to connect to the server
*
* INPUT PARAMETERS  : wmiProvider
*
* OUTPUT PARAMETERS : NONE
*
* return value      : Returns SVS_OK if connection to wmiprovider is successful 
*                     otherwise returns SVS_FALSE
*
*/
SVERROR GenericWMI::init(const std::string &wmiProvider)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVERROR  bRet = SVS_FALSE;

    if( m_init == false )
    {
        if (m_pWmiRecordProcessor)
        {
            DebugPrintf(SV_LOG_DEBUG, "Connecting to wmi provider %s\n", wmiProvider.c_str());

            // Connect to local server
            SVERROR  ret = m_wmiConnector.ConnectToServer(".", wmiProvider);
            if (ret == 0)
            {
                m_init = true;
                bRet = SVS_OK;

                DebugPrintf(SV_LOG_DEBUG, "Connected to wmi provider %s\n", wmiProvider.c_str());
            }
            else
            {
                DebugPrintf(SV_LOG_WARNING, "Connection to WMI provider failed hr = %08x\n", wmiProvider.c_str(), ret);
            }
        }
        else
        {
            DebugPrintf( SV_LOG_ERROR, "Failed to initialize generic wmi as no record processor given\n");
        }
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "Already initialized\n");
        bRet = SVS_OK;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return bRet;
}

void GenericWMI::GetData(const std::string &classname)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    std::string query = "SELECT * FROM " + classname;
    GetDataUsingQuery(query);
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}

void GenericWMI::GetDataUsingQuery(const std::string &query)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    std::string data;

    if (init() != SVS_OK)
    {
        DebugPrintf(SV_LOG_ERROR, "%s intialization failed\n", FUNCTION_NAME);
        return;
    }

    DebugPrintf(SV_LOG_DEBUG, "intialization succeded\n");

    CComPtr<IEnumWbemClassObject>   pEnumerator;

    if (m_wmiConnector.execQuery(query, &pEnumerator) == SVS_OK)
    {
        DebugPrintf(SV_LOG_DEBUG, "Executed query %s successfully\n", query.c_str());

        int numRecords = 0;
        while (pEnumerator)
        {
            ULONG uReturn = 0;
            CComPtr<IWbemClassObject>   pclsObj;

            HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
            if (0 == uReturn)
            {
                DebugPrintf(SV_LOG_DEBUG, "%s: No more records exist. Total records received %d\n", FUNCTION_NAME, numRecords);
                break;
            }

            numRecords++;
            DebugPrintf(SV_LOG_DEBUG, "%s: Processing record %d\n", FUNCTION_NAME, numRecords);

            if (!m_pWmiRecordProcessor->Process(pclsObj))
            {
                /* Not a major error */
                DebugPrintf(SV_LOG_ERROR, "Failed to process a wmi record for query %s with error %s\n", query.c_str(), m_pWmiRecordProcessor->GetErrMsg().c_str());
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "Successfully processed a wmi record. %s\n", m_pWmiRecordProcessor->GetTraceMsg().c_str());
            }
        }
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}

SVERROR GenericWMI::ExecuteMethod(
    IWbemClassObject            *pClassOrInstanceObject,
    LPWSTR                      methodName,
    std::vector<WmiProperty>    &inParamsList,
    std::vector<WmiProperty>    &outParamsList)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    SVERROR ret = SVE_FAIL;

    HRESULT hr = m_wmiConnector.execMethod(
                        pClassOrInstanceObject,
                        methodName,
                        inParamsList,
                        outParamsList);

    if (FAILED(hr))
    {
        DebugPrintf(SV_LOG_ERROR,
            "Failed to invoke WMI method: %S for object: %p with %d input and %d output parameters. hr = %#x.\n",
            methodName, pClassOrInstanceObject, inParamsList.size(), outParamsList.size(), hr);

        goto Cleanup;
    }

    DebugPrintf(SV_LOG_DEBUG, "Executed WMI method %s successfully\n", methodName);
    ret = SVS_OK;

Cleanup:
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return ret;
}