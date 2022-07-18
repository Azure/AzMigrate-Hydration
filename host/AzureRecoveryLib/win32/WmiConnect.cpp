/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiConnect.cpp

Description	:   Warpper class for WMI connection

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include "WmiConnect.h"
#include "../common/Trace.h"

#include <sstream>
#pragma comment(lib, "wbemuuid.lib")

namespace AzureRecovery
{

ACE_Recursive_Thread_Mutex WINWMIConnector::m_lock ;

WINWMIConnector::WINWMIConnector()

{
    m_WbemServices = NULL ;
	m_WbemLocator = NULL ;
}

IWbemServices* WINWMIConnector::GetNamespace() const
{
	AutoGuard guard(m_lock);
	return m_WbemServices;
}

bool WINWMIConnector::ConnectToServer(const std::string& hostName, const std::string& objPath)

{
	AutoGuard guard(m_lock) ;
    TRACE_FUNC_BEGIN;

    bool bRet = false ;
    HRESULT hres ;

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &m_WbemLocator);
    if( FAILED(hres) )
    {
        TRACE_ERROR("CoCreateInstance failed. Error %08X\n", hres) ;
    }
    else
    {

        TRACE("CoCreateInstance is successful\n") ;

        m_objectNameSpace = "" ;
        m_objectNameSpace += "\\\\" ;
        m_objectNameSpace += hostName ;
        m_objectNameSpace += "\\" ;
        m_objectNameSpace += objPath ;        

        hres = m_WbemLocator->ConnectServer(_bstr_t(m_objectNameSpace.c_str()),
                                            NULL,
                                            NULL,
                                            0,
                                            NULL,
                                            0,
                                            0,
                                            &m_WbemServices) ;
        if( FAILED(hres) )
        {
			TRACE_ERROR("ConnectServer failed %08X\n", hres);
        }
        else
        {
            hres = CoSetProxyBlanket(m_WbemServices,
                                    RPC_C_AUTHN_WINNT,
                                    RPC_C_AUTHZ_NONE,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_CALL,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    NULL,
                                    EOAC_NONE);
            if( FAILED(hres) )
            {
				TRACE_ERROR("CoSetProxyBlanket failed %08X\n", hres);
            }
            else
            {
                bRet = true ;
            }
        }
    }

	TRACE_FUNC_END;
    return bRet ;
}



bool WINWMIConnector::execQuery(const std::string& query, IEnumWbemClassObject** pEnumerator)
{
    AutoGuard guard(m_lock) ;

	TRACE_FUNC_BEGIN;

    bool bRet = false ;
    HRESULT hres ;

    hres = m_WbemServices->ExecQuery(bstr_t("WQL"),
                            bstr_t(query.c_str()),
                            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
                            NULL, pEnumerator) ;
    if( FAILED(hres) )
    {
		TRACE_ERROR("Query %s is failed. Error %08X\n", query.c_str(), hres);
		TRACE_ERROR("Namespace : %s\n", m_objectNameSpace.c_str());
    }
    else
    {
        bRet = true ;
    }

	TRACE_FUNC_END;

    return bRet ;
}



WINWMIConnector::~WINWMIConnector()
{
    if( m_WbemServices != NULL )
    {
        m_WbemServices->Release() ;
        m_WbemServices = NULL ;
    }

    if( m_WbemLocator != NULL )
    {
        m_WbemLocator->Release() ;
         m_WbemLocator = NULL ;
    }
}

}

