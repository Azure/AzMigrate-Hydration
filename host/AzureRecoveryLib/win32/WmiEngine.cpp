/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiEngine.cpp

Description	:   Warpper class for iterating WMI queries results

History		:   1-6-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#include <Windows.h>

#include "WmiEngine.h"
#include "../common/Trace.h"

namespace AzureRecovery
{

/*
Method      : CWmiEngine::CWmiEngine

Description : CWmiEngine constructor

Parameters  : [in] pwrp : Record Process object pointer
              [in] wmiProvider: Namespace of the wmi provider

Return      : None

*/
CWmiEngine::CWmiEngine(CWmiRecordProcessor *pwrp, const std::string& wmiProvider)
: m_pWmiRecordProcessor(pwrp),
m_wmiProvider(wmiProvider),
m_init(false)
{
}

/*
Method      : CWmiEngine::~CWmiEngine

Description : Destructor

Parameters  :

Return      :

*/
CWmiEngine::~CWmiEngine()
{
}

/*
Method      : CWmiEngine::init

Description : Initializes the CWmiEngine object by establishing a connection to wmi

Parameters  : None

Return      : true initialization is successful, otherwise false.

*/
bool CWmiEngine::init()
{
	TRACE_FUNC_BEGIN;
	bool  bRet = false;
	if (m_init == false)
	{
		if (m_pWmiRecordProcessor)
		{
			bRet = m_wmiConnector.ConnectToServer(".", m_wmiProvider);
			if ( bRet )
			{
				m_init = true;
				TRACE("Connection to WMI provider %s is successful\n",
					  m_wmiProvider.c_str());
			}
			else
			{
				TRACE_ERROR("Connection to WMI provider %s is failed\n",
					        m_wmiProvider.c_str());
			}
		}
		else
		{
			TRACE_ERROR("Failed to initialize generic wmi as no record processor given\n");
		}
	}
	else
	{
		TRACE("WMI SQL Interface already initialized\n");
		bRet = true;
	}

	TRACE_FUNC_END;
	return bRet;
}

/*
Method      : CWmiEngine::ProcessQuery

Description : Executes the query and calls the record processor Process() method for each
              query result entry.

Parameters  : [in] query_string: wmi query

Return      : true if all the record processes successfuly, otherwise false.

*/
bool CWmiEngine::ProcessQuery(const std::string &query_string)
{
	TRACE_FUNC_BEGIN;
	bool bProcess = true;
	std::string data;

	bProcess = init();
	if (!bProcess)
	{
		TRACE_ERROR("%s failed for the query %s.\n", __FUNCTION__, query_string.c_str());
		return bProcess;
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	bProcess = m_wmiConnector.execQuery(query_string, &pEnumerator);
	if ( bProcess )
	{
		IWbemClassObject *pclsObj = NULL;
		ULONG uReturn = 0;

		while (pEnumerator && bProcess)
		{
			HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

			if (0 == uReturn)
			{
				break;
			}

			if (!m_pWmiRecordProcessor->Process(pclsObj, m_wmiConnector.GetNamespace()))
			{
				TRACE_ERROR("Failed to process a wmi record for the query [%s].\nError: %s\n",
					         query_string.c_str(), 
					         m_pWmiRecordProcessor->GetErrMsg().c_str()
					         );

				bProcess = false;
			}
			pclsObj && pclsObj->Release();
		}
		pEnumerator->Release();
	}

	TRACE_FUNC_END;
	return bProcess;
}

}
