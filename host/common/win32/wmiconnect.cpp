#include "wmiconnect.h"

#include <boost/algorithm/string/predicate.hpp>
#include <sstream>

#include "portable.h"
#include "portablehelpersmajor.h"
#include "logger.h"

#include <ratio>
#include <chrono>
#include <ctime>

#include <atlbase.h>

#include "localconfigurator.h"

ACE_Recursive_Thread_Mutex WINWMIConnector::m_recursiveLock;

#pragma comment(lib, "wbemuuid.lib")


#define CLEANUP_ON_FAIL(hres) { if (FAILED(hres)) { goto Cleanup; } }

#define CLEANUP_ON_UNEXPECTED_TYPE(variant, expType) { if (V_VT(variant) != expType) { hres = E_FAIL; goto Cleanup; } }

WINWMIConnector::WINWMIConnector()
{
    m_WbemServices = NULL;
    m_WbemLocator = NULL;
}

SVERROR WINWMIConnector::ConnectToServer(const std::string& hostName, const std::string& objPath)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    AutoGuard guard(m_recursiveLock);

    SVERROR bRet = SVS_FALSE;
    HRESULT hres;

    SV_UINT uiMaxWmiConnectionTimeoutSec;
    try
    {
        LocalConfigurator lc;
        uiMaxWmiConnectionTimeoutSec = lc.getMaximumWMIConnectionTimeout();
    }
    catch (...)
    {
        // For instance, Azure PS doesn't have MT installed, which means the
        // local configurator creation would "throw" saying VX agent installation
        // registry key couldn't be found.
        uiMaxWmiConnectionTimeoutSec = LocalConfigurator::DEFAULT_MAX_WMI_CONNECTION_TIMEOUT_SEC;
    }

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        DebugPrintf(SV_LOG_ERROR, "CoInitializeEx failed. Error %08X\n", hres);
    }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&m_WbemLocator);
    if (FAILED(hres))
    {
        DebugPrintf(SV_LOG_ERROR, "CoCreateInstance failed. Error %08X\n", hres);
        return bRet;
    }

    DebugPrintf(SV_LOG_DEBUG, "CoCreateInstance succeeded\n");

    std::stringstream   ssNameSpace;

    ssNameSpace << "\\\\" << hostName << "\\" << objPath;

    m_objectNameSpace = ssNameSpace.str();

    std::wstring     wsObjNameSpace(m_objectNameSpace.begin(), m_objectNameSpace.end());

    DebugPrintf(SV_LOG_DEBUG, "Connecting to namespace %s\n", m_objectNameSpace.c_str());

    auto start = std::chrono::high_resolution_clock::now();

    hres = m_WbemLocator->ConnectServer(_bstr_t(wsObjNameSpace.c_str()),
                                            NULL,
                                            NULL,
                                            0,
                                            NULL,
                                            0,
                                            0,
                                            &m_WbemServices);

    auto end = std::chrono::high_resolution_clock::now();

    if (FAILED(hres))
    {
        if (hres == WBEM_E_INVALID_NAMESPACE)
        {
            DebugPrintf(SV_LOG_WARNING, "WMI provider %s did not exist. Error code = %08X\n", m_objectNameSpace.c_str(), hres);
            bRet = SVE_NOTIMPL;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "ConnectServer failed %08X\n", hres);
        }
        return bRet;
    }

    DebugPrintf(SV_LOG_DEBUG, "Connected to namespace %s\n", m_objectNameSpace.c_str());

    auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(end - start);

    int   ullElapsedSecs = (int) elapsedTime.count();

    if (ullElapsedSecs > uiMaxWmiConnectionTimeoutSec)
    {
        DebugPrintf(SV_LOG_ALWAYS, "WMIConnect: Connection to namespace %s took %d seconds\n", m_objectNameSpace.c_str(), ullElapsedSecs);
    }

    DebugPrintf(SV_LOG_DEBUG, "calling CoSetProxyBlanket\n");
    hres = CoSetProxyBlanket(m_WbemServices,
                                RPC_C_AUTHN_WINNT,
                                RPC_C_AUTHZ_NONE,
                                NULL,
                                RPC_C_AUTHN_LEVEL_CALL,
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL,
                                EOAC_NONE);

    if (FAILED(hres))
    {
        DebugPrintf(SV_LOG_ERROR, "CoSetProxyBlanket failed %08X\n", hres);
        return bRet;
    }

    DebugPrintf(SV_LOG_DEBUG, "CoSetProxyBlanket Succeeded\n");

    bRet = SVS_OK;
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}

SVERROR WINWMIConnector::execQuery(const std::string& query, IEnumWbemClassObject** pEnumerator)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    AutoGuard guard(m_recursiveLock);

    SVERROR bRet = SVS_FALSE;
    HRESULT hres;

    DebugPrintf(SV_LOG_DEBUG, "Namespace: %s Executing Query: %s\n", m_objectNameSpace.c_str(), query.c_str());

    hres = m_WbemServices->ExecQuery(bstr_t("WQL"),
        bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        pEnumerator);

    if (FAILED(hres))
    {
        DebugPrintf(SV_LOG_ERROR, "Query %s is failed. Error %08X\n", query.c_str(), hres);
        DebugPrintf(SV_LOG_ERROR, "Namespace : %s\n", m_objectNameSpace.c_str());
    }
    else
    {
        bRet = SVS_OK;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return bRet;
}

HRESULT WINWMIConnector::getClassPath(IWbemClassObject *pObject, std::wstring &classPath)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    AutoGuard guard(m_recursiveLock);

    HRESULT hres;

    CComVariant serverName;
    CComVariant nameSpace;
    CComVariant className;

    BOOST_ASSERT(pObject != NULL);

    hres = pObject->Get(L"__Server", 0, &serverName, NULL, NULL);
    CLEANUP_ON_FAIL(hres);
    CLEANUP_ON_UNEXPECTED_TYPE(&serverName, VT_BSTR);

    hres = pObject->Get(L"__Namespace", 0, &nameSpace, NULL, NULL);
    CLEANUP_ON_FAIL(hres);
    CLEANUP_ON_UNEXPECTED_TYPE(&nameSpace, VT_BSTR);

    hres = pObject->Get(L"__Class", 0, &className, NULL, NULL);
    CLEANUP_ON_FAIL(hres);
    CLEANUP_ON_UNEXPECTED_TYPE(&className, VT_BSTR);

    try
    {
        classPath = L"\\\\";
        classPath += V_BSTR(&serverName);
        classPath += L'\\';
        classPath += V_BSTR(&nameSpace);
        classPath += L':';
        classPath += V_BSTR(&className);
    }
    catch (std::bad_alloc)
    {
        hres = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return hres;
}

HRESULT WINWMIConnector::execMethod(
    LPWSTR classOrInstancePath,
    LPWSTR methodName,
    std::vector<WmiProperty> &inParamsList,
    std::vector<WmiProperty> &outParamsList)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    AutoGuard guard(m_recursiveLock);

    BOOST_ASSERT(classOrInstancePath != NULL);
    BOOST_ASSERT(wcslen(classOrInstancePath) != 0);

    HRESULT hres;
    CComPtr<IWbemClassObject>   pClassOrInstanceObject;

    hres = m_WbemServices->GetObject(
        classOrInstancePath,
        0,
        NULL,
        &pClassOrInstanceObject,
        NULL);

    CLEANUP_ON_FAIL(hres);

    hres = this->execMethod(pClassOrInstanceObject, methodName, inParamsList, outParamsList);
    CLEANUP_ON_FAIL(hres);

Cleanup:
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return hres;
}

HRESULT WINWMIConnector::execMethod(
    IWbemClassObject *pClassOrInstanceObject,
    LPWSTR methodName,
    std::vector<WmiProperty> &inParamsList,
    std::vector<WmiProperty> &outParamsList)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    AutoGuard guard(m_recursiveLock);

    HRESULT hres;
    CComPtr<IWbemClassObject>   pClassObj;
    CComPtr<IWbemClassObject>   pInParamsSignature;
    CComPtr<IWbemClassObject>   pInParams;
    CComPtr<IWbemClassObject>   pOutParams;

    CComVariant                 objPath;
    std::wstring classPath;

    BOOST_ASSERT(pClassOrInstanceObject != NULL);
    BOOST_ASSERT(methodName != NULL);
    BOOST_ASSERT(wcslen(methodName) != 0);

    // TODO-SanKumar-1807: The class path could be retrieved by parsing the
    // object path too (cheaper option).
    hres = this->getClassPath(pClassOrInstanceObject, classPath);
    CLEANUP_ON_FAIL(hres);

    hres = pClassOrInstanceObject->Get(L"__Path", 0, &objPath, NULL, NULL);
    CLEANUP_ON_FAIL(hres);
    CLEANUP_ON_UNEXPECTED_TYPE(&objPath, VT_BSTR);

    if (boost::iequals(classPath, objPath.bstrVal))
    {
        // This is a class object. Reuse the class object argument itself.

        hres = pClassOrInstanceObject->GetMethod(methodName, 0, &pInParamsSignature, NULL);
        CLEANUP_ON_FAIL(hres);
    }
    else
    {
        // This is an instance object, so get its class object.

        hres = m_WbemServices->GetObject(
            bstr_t(classPath.c_str()),
            0,
            NULL,
            &pClassObj,
            NULL);

        CLEANUP_ON_FAIL(hres);

        hres = pClassObj->GetMethod(methodName, 0, &pInParamsSignature, NULL);
        CLEANUP_ON_FAIL(hres);
    }

    if (pInParamsSignature == NULL && !inParamsList.empty())
    {
        // The systems says no args but input arguments are passed by the caller.
        hres = E_INVALIDARG;
        CLEANUP_ON_FAIL(hres);
    }

    if (pInParamsSignature != NULL)
    {
        hres = pInParamsSignature->SpawnInstance(0, &pInParams);
        CLEANUP_ON_FAIL(hres);

        for (WmiProperty &currInParam : inParamsList)
        {
            hres = pInParams->Put(currInParam.Name.c_str(), 0, &currInParam.Value, NULL);
            CLEANUP_ON_FAIL(hres);
        }
    }

    hres = m_WbemServices->ExecMethod(
        objPath.bstrVal, /*strObjPath*/
        methodName, /*strMethodName*/
        0, /*Flags*/
        NULL, /*pCtx*/
        pInParams, /*pInParams*/
        &pOutParams, /*ppOutParams*/
        NULL); /*ppCallResult*/

    CLEANUP_ON_FAIL(hres);

    for (WmiProperty &currOutParam : outParamsList)
    {
        hres = pOutParams->Get(currOutParam.Name.c_str(), 0, &currOutParam.Value, NULL, NULL);
        CLEANUP_ON_FAIL(hres);

        // The property value is mismatching against the expected type.
        // But the variant is fully initialized and so the cleanup will be
        // taken up by the destructor of the WmiProperty class.
        CLEANUP_ON_UNEXPECTED_TYPE(&currOutParam.Value, currOutParam.Type);
    }

Cleanup:
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return hres;
}

WINWMIConnector::~WINWMIConnector()
{
    SafeReleaseComPtr(m_WbemServices);
    SafeReleaseComPtr(m_WbemLocator);
}

template<>
HRESULT WmiProperty::SetValue<INT32>(INT32 value)
{
    if (this->Type != VT_I4)
        return ERROR_UNSUPPORTED_TYPE;

    V_VT(&this->Value) = VT_I4;
    V_I4(&this->Value) = value;
    return S_OK;
}