#include "service.h"
#ifndef _WINDOWS_
#include <Windows.h>
#endif
#include <winbase.h>
#include <winsvc.h>
#include <tchar.h>
#include <sstream>
#include "portable.h"
#include "portablehelpers.h"

SVERROR isServiceInstalled(const std::string& serviceName, InmServiceStatus& status)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    status = INM_SVCSTAT_NONE ;
    SVERROR bRet = SVS_FALSE ;
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM != NULL) 
    {
        SC_HANDLE hService = ::OpenService(hSCM, serviceName.c_str(),SERVICE_ALL_ACCESS);
        bRet = SVS_OK ;
		if (hService != NULL) 
        {
            DebugPrintf(SV_LOG_DEBUG, "Service %s is installed on the machine\n", serviceName.c_str()) ;
			status  = INM_SVCSTAT_INSTALLED ;
			::CloseServiceHandle(hService);
		}
        else
        {
            status = INM_SVCSTAT_NOTINSTALLED ;
            DebugPrintf(SV_LOG_DEBUG, "Service %s is not installed on the machine\n", serviceName.c_str()) ;
        }
		::CloseServiceHandle(hSCM);
	}
    else
    {
        DebugPrintf(SV_LOG_ERROR, "::OpenSCManager Failed %d\n", GetLastError()) ;
        throw "::OpenSCManager Failed\n" ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR getServiceStatus(const std::string& serviceName, InmServiceStatus& status)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR bRet = SVS_FALSE ;
    DWORD dwBytesNeeded = 0 ;
    status = INM_SVCSTAT_NONE ;
    SERVICE_STATUS_PROCESS ssStatus;
	SC_HANDLE hSCM = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM != NULL)
    {
        SC_HANDLE hService = ::OpenService(hSCM, serviceName.c_str(),SERVICE_ALL_ACCESS);
		if (hService != NULL) 
        {
            status = INM_SVCSTAT_INSTALLED ;
            DebugPrintf(SV_LOG_DEBUG, "Service %s is installed on the machine\n", serviceName.c_str()) ;
            if (!QueryServiceStatusEx(hService,                     
                                    SC_STATUS_PROCESS_INFO,
                                    (LPBYTE) &ssStatus,
                                    sizeof(SERVICE_STATUS_PROCESS),
                                    &dwBytesNeeded ) )
            {
                DebugPrintf(SV_LOG_ERROR, "QueryServiceStatusEx failed %d\n", GetLastError());
            }
            else
            {
                bRet = SVS_OK ;
                switch(ssStatus.dwCurrentState)
                {
                    case SERVICE_STOPPED :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is stopped on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_STOPPED ;
                        break ;
                    case SERVICE_START_PENDING :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is start pending on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_START_PENDING ;
                        break ;
                    case SERVICE_STOP_PENDING :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is stop pending on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_STOP_PENDING ;
                        break ;
                    case SERVICE_RUNNING :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is running on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_RUNNING ;
                        break ;
                    case SERVICE_CONTINUE_PENDING :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is continue pending on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_CONTINUE_PENDING ; 
                        break ;
                    case SERVICE_PAUSE_PENDING :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is pause pending on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_PAUSE_PENDING ;
                        break ;
                    case SERVICE_PAUSED :
                        DebugPrintf(SV_LOG_DEBUG, "Service %s is paused on the machine\n", serviceName.c_str()) ;
                        status = INM_SVCSTAT_PAUSED ;
                        break ;
                    default:
                        status = INM_SVCSTAT_NONE ;
                }
            }
            ::CloseServiceHandle(hService);
		}
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "Service %s is not installed on the machine\n", serviceName.c_str()) ;
            status = INM_SVCSTAT_NOTINSTALLED ;
        }
		::CloseServiceHandle(hSCM);
	}
    else
    {
        DebugPrintf(SV_LOG_ERROR, "::OpenSCManager Failed %d\n", GetLastError()) ;
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR StopSvc(const std::string& svcName)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR bRet = SVS_FALSE ;
    SC_HANDLE schService;        
    SERVICE_STATUS_PROCESS  serviceStatusProcess;
    DWORD actualSize = 0;
    SERVICE_STATUS serviceStatus ;
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager) 
    {
        schService = OpenService(schSCManager, svcName.c_str(), SERVICE_ALL_ACCESS); 
        if (NULL != schService) 
		{ 
            if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_RUNNING == serviceStatusProcess.dwCurrentState) 
			{
                if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) 
                {
                    do 
                    {                   
                        Sleep(1000); 
                    } while (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_STOPPED != serviceStatusProcess.dwCurrentState);
                    bRet  =  SVS_OK ;
                }
				else
				{
					DebugPrintf(SV_LOG_ERROR, "Unable to stop the service %s. Error %ld\n", svcName.c_str(), GetLastError()) ;
                    bRet = SVS_FALSE ;
				}
            } 
			else
			{
				DebugPrintf(SV_LOG_INFO,"\n Service %s is not running ",svcName.c_str());
				bRet = SVS_OK;
			}
            CloseServiceHandle(schService); 
        }
    }
    CloseServiceHandle(schSCManager);     
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR StartSvc(const std::string& svcName)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SC_HANDLE schService;        
    SVERROR bRet = SVS_FALSE ;
    SERVICE_STATUS_PROCESS  serviceStatusProcess;
    DWORD actualSize = 0;

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager) 
    {
        schService = OpenService(schSCManager, svcName.c_str(), SERVICE_ALL_ACCESS); 
        if (NULL != schService) 
        { 
            if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_RUNNING != serviceStatusProcess.dwCurrentState) 
            {
                if( !StartService(schService, 0, NULL) )
                {
                    DebugPrintf(SV_LOG_ERROR, "Unable to start the service %s. Error %ld\n", svcName.c_str(), GetLastError()) ;
                    bRet = SVS_FALSE ; 
                }
                else
                {
					DebugPrintf(SV_LOG_INFO, "Waiting for the service %s to start ...\n", svcName.c_str()) ;
					int nSec = 0;
					bRet = SVS_OK;
                    do 
                    {
						if(120 == nSec)
						{
							DebugPrintf(SV_LOG_ERROR, "Service %s is not responding.\n", svcName.c_str()) ;
							bRet = SVS_FALSE;
							break;
						}
                        Sleep(1000); 
						nSec++;
                    } while (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_RUNNING != serviceStatusProcess.dwCurrentState); 
                }
            }
			else
			{
				if(SERVICE_RUNNING == serviceStatusProcess.dwCurrentState)
				{
					DebugPrintf(SV_LOG_INFO, "%s is already in running state\n", svcName.c_str());
					bRet = SVS_OK;
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "Failed in QueryServiceStatusEx, Unable to start the service %s. Error %ld\n", svcName.c_str(), GetLastError()); 
				}
			}            
        }
		CloseServiceHandle(schService); 
    }
    CloseServiceHandle(schSCManager);     
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}
SVERROR StrService(const std::string& svcName)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
	SVERROR bRet = SVS_OK;
	DebugPrintf(SV_LOG_INFO, "Going to Start the Service %s\n", svcName.c_str());
	if( StartSvc(svcName) == SVS_FALSE)
	{
		DebugPrintf(SV_LOG_ERROR, "Failed to start the service %s.\n", svcName.c_str()) ;
		bRet = SVS_FALSE ; 
	}
	else if(FindDependentServicesAndStart(svcName.c_str()) == SVS_FALSE)
	{
		DebugPrintf(SV_LOG_ERROR, "Failed while starting the dependent service of the service %s.\n", svcName.c_str()) ;
	}
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
	return bRet;
}

SVERROR StpService(const std::string& svcName)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
	SVERROR bRet = SVS_OK;
	DebugPrintf(SV_LOG_INFO, "Going to stop the service %s\n", svcName.c_str());
	std::vector<std::string> vecDependentSvcNames;
	if( FindDependentServicesAndStop(svcName.c_str(), vecDependentSvcNames) == SVS_FALSE )
	{
		DebugPrintf(SV_LOG_ERROR, "Failed while stopping the dependent service of the service %s.\n", svcName.c_str()) ;
	}
	if( StopSvc(svcName) == SVS_FALSE)
	{
		DebugPrintf(SV_LOG_ERROR, "Failed to stop the service %s.\n", svcName.c_str()) ;
		bRet = SVS_FALSE;
	}
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
	return bRet;
}

SVERROR ReStartSvc(const std::string& svcName)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SC_HANDLE schService;        
    SVERROR bRet = SVS_FALSE ;
    SERVICE_STATUS_PROCESS  serviceStatusProcess;
    DWORD actualSize = 0;
	std::vector<std::string> vecDependentSvcNames;

    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager) 
    {
        schService = OpenService(schSCManager, svcName.c_str(), SERVICE_ALL_ACCESS); 
        if (NULL != schService) 
        { 
            if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_RUNNING != serviceStatusProcess.dwCurrentState) 
            {
				DebugPrintf(SV_LOG_INFO, "%s is in stopped state, Going to Start the Service\n", svcName.c_str());
				if( StartSvc(svcName) == SVS_FALSE)
                {
                    DebugPrintf(SV_LOG_ERROR, "Failed to start the service %s.\n", svcName.c_str()) ;
                    bRet = SVS_FALSE ; 
                }
                else
                {
                    bRet = SVS_OK; 
                }
				if(FindDependentServicesAndStart(svcName.c_str()) == SVS_FALSE)
				{
					DebugPrintf(SV_LOG_ERROR, "Failed while starting the dependent service of the service %s.\n", svcName.c_str()) ;
				}
			}
			else
			{
				if(SERVICE_RUNNING == serviceStatusProcess.dwCurrentState)
				{
					DebugPrintf(SV_LOG_INFO, "%s is in running state, Going to Re-Start the Server\n", svcName.c_str());
					if( FindDependentServicesAndStop(svcName.c_str(), vecDependentSvcNames) == SVS_FALSE )
					{
						DebugPrintf(SV_LOG_ERROR, "Failed while stopping the dependent service of the service %s.\n", svcName.c_str()) ;
					}
					if( StopSvc(svcName) == SVS_FALSE)
					{
						DebugPrintf(SV_LOG_ERROR, "Failed to stop the service %s.\n", svcName.c_str()) ;
						bRet = SVS_FALSE;
					}
					if( StartSvc(svcName) == SVS_OK)
					{
						//Start all the dependent services which are stopped during Restart
						if(!vecDependentSvcNames.empty())
						{
							std::reverse(vecDependentSvcNames.begin(),vecDependentSvcNames.end());
							std::vector<std::string>::iterator iter = vecDependentSvcNames.begin();
							while(iter != vecDependentSvcNames.end())
							{
								if( StartSvc(*iter) == SVS_FALSE)
								{
									DebugPrintf(SV_LOG_ERROR, "Failed to start the service %s.\n", (*iter).c_str()) ;
								}
								iter++;
							}
						}
						bRet = SVS_OK;
					}
					else
					{
						DebugPrintf(SV_LOG_ERROR, "Failed to start the service %s.\n", svcName.c_str()) ;
						bRet = SVS_FALSE;
					}
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "Failed in QueryServiceStatusEx, Unable to start the service %s. Error %ld\n", svcName.c_str(), GetLastError()); 
					bRet = SVS_FALSE ;
				}
			}			
		}
		else
		{
			DebugPrintf(SV_LOG_ERROR, "Failed in getting the handle  on the service %s. Error %ld\n", svcName.c_str(), GetLastError()); 
			bRet = SVS_FALSE ;
		}
		CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

SVERROR FindDependentServicesAndStop(const std::string& svcName, std::vector<std::string>& vecDependentSvcNames)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
	SC_HANDLE schService = NULL;
	LPENUM_SERVICE_STATUS serviceStatus = NULL;
	BOOL bFlag = TRUE;

	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if (schSCManager) 
    {
		DWORD bufferSize;
		schService = OpenService(schSCManager, svcName.c_str(), SERVICE_STOP|SERVICE_QUERY_STATUS |SERVICE_ENUMERATE_DEPENDENTS);
		if(schService)
		{
			DWORD dwRequriedBuffer;
			DWORD dwServicesReturned = NULL;
			//Enumerate dependent services.
			EnumDependentServices( schService, SERVICE_STATE_ALL, serviceStatus, 0, &dwRequriedBuffer, &dwServicesReturned);
			if(GetLastError() == ERROR_MORE_DATA)
			{
				serviceStatus = (LPENUM_SERVICE_STATUS)new BYTE[1024+1];
				bufferSize = dwRequriedBuffer;
				if(EnumDependentServices( schService, SERVICE_STATE_ALL, (LPENUM_SERVICE_STATUS)serviceStatus, bufferSize , &dwRequriedBuffer, &dwServicesReturned))
				{
					for(DWORD dwCount = 0; dwCount < dwServicesReturned; dwCount++)
					{
						if((serviceStatus[dwCount].ServiceStatus.dwCurrentState) == SERVICE_STOPPED)
						{
							DebugPrintf(SV_LOG_DEBUG, " %s Service is in stopped state\n", serviceStatus[dwCount].lpServiceName);
							continue;
						}
						FindDependentServicesAndStop(serviceStatus[dwCount].lpServiceName, vecDependentSvcNames);						
						//push the already stopped dependent service into vector(vecStoppedServicesNames).
						vecDependentSvcNames.push_back(serviceStatus[dwCount].lpServiceName);
						if(StopSvc(serviceStatus[dwCount].lpServiceName) == SVS_FALSE)
						{
							bFlag = FALSE;
							break;
						}

					}
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "Failed to enumerate dependent services Error:[%d]\n",GetLastError());
					bFlag = FALSE;
				}
			}
			if(serviceStatus)
			{
				delete []serviceStatus;
			}
		}
		CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
	if(FALSE == bFlag)
		return SVS_FALSE;
	return SVS_OK;
}

SVERROR FindDependentServicesAndStart(const std::string& svcName)
{
	DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
	SC_HANDLE schService = NULL;
	LPENUM_SERVICE_STATUS serviceStatus = NULL;
	BOOL bFlag = TRUE;

	SC_HANDLE schSCManager = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
	if (schSCManager) 
    {
		DWORD bufferSize;
		schService = OpenService(schSCManager, svcName.c_str(), SERVICE_START |SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS);
		if(schService)
		{
			DWORD dwRequriedBuffer;
			DWORD dwServicesReturned = NULL;
			//Enumerate dependent services.
			EnumDependentServices( schService, SERVICE_STATE_ALL, serviceStatus, 0, &dwRequriedBuffer, &dwServicesReturned);
			if(GetLastError() == ERROR_MORE_DATA)
			{
				serviceStatus = (LPENUM_SERVICE_STATUS)new BYTE[1024+1];
				bufferSize = dwRequriedBuffer;
				if(EnumDependentServices( schService, SERVICE_STATE_ALL, (LPENUM_SERVICE_STATUS)serviceStatus, bufferSize , &dwRequriedBuffer, &dwServicesReturned))
				{
					for(DWORD dwCount = 0; dwCount < dwServicesReturned; dwCount++)
					{
						FindDependentServicesAndStart(serviceStatus[dwCount].lpServiceName);
						if((serviceStatus[dwCount].ServiceStatus.dwCurrentState) == SERVICE_RUNNING)
						{
							DebugPrintf(SV_LOG_DEBUG, " %s Service is in running state\n", serviceStatus[dwCount].lpServiceName);
							continue;
						}
						if(StartSvc(serviceStatus[dwCount].lpServiceName) == SVS_FALSE)
						{
							bFlag = FALSE;
							break;
						}

					}
				}
				else
				{
					DebugPrintf(SV_LOG_ERROR, "Failed to enumerate dependent services Error:[%d]\n",GetLastError());
					bFlag = FALSE;
				}
			}
			if(serviceStatus)
			{
				delete []serviceStatus;
			}
		}
		CloseServiceHandle(schService);
	}
	CloseServiceHandle(schSCManager);
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
	if(FALSE == bFlag)
		return SVS_FALSE;
	return SVS_OK;
}

SVERROR StopService(const std::string& svcName, SV_UINT timeout)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR bRet = SVS_FALSE ;
    SC_HANDLE schService;        
    SERVICE_STATUS_PROCESS  serviceStatusProcess;
    DWORD actualSize = 0;
    SERVICE_STATUS serviceStatus ;
    SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager) 
    {
        schService = OpenService(schSCManager, svcName.c_str(), SERVICE_ALL_ACCESS); 
        if (NULL != schService) { 
            if (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_RUNNING == serviceStatusProcess.dwCurrentState) {
                if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus)) 
                {
                    SV_UINT waitTime = 0 ;
                    do 
                    {   if( timeout != 0 )
                        {
                            waitTime++ ;
                            if( waitTime > timeout )
                            {
                                break ;
                            }
                        }
                        Sleep(1000); 
                    } while (QueryServiceStatusEx(schService, SC_STATUS_PROCESS_INFO, (LPBYTE)&serviceStatusProcess, sizeof(serviceStatusProcess), &actualSize) && SERVICE_STOPPED != serviceStatusProcess.dwCurrentState);
                    if( QueryServiceStatusEx(schService, 
                            SC_STATUS_PROCESS_INFO, 
                            (LPBYTE)&serviceStatusProcess, 
                            sizeof(serviceStatusProcess), &actualSize) && 
                                SERVICE_STOPPED == serviceStatusProcess.dwCurrentState)
                    {
                        bRet  =  SVS_OK ;
                    }
                }
            } 
			else
			{
				DebugPrintf(SV_LOG_INFO,"\n Service %s is not running ",svcName.c_str());
				bRet = SVS_OK;
			}
            CloseServiceHandle(schService); 
        }
    }
    CloseServiceHandle(schSCManager);     
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

bool getStartServiceType(const std::string& svcName, InmServiceStartType &serviceType , std::string &serviceStartUpType)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    bool bRet = false ;
	LPQUERY_SERVICE_CONFIG lpqscBuf; 
	char szStartType[255];

	// Get a handle to the SCM database. 
	schSCManager = OpenSCManager( 
		NULL,                    // local computer
		NULL,                    // ServicesActive database 
		SC_MANAGER_ALL_ACCESS);  // full access rights 
 
	if(schSCManager)
	{
		schService = OpenService( 
				schSCManager,            // SCM database 
				svcName.c_str(),               // name of service 
				SERVICE_ALL_ACCESS);  
		if (NULL != schService )
		{ 
	        lpqscBuf = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LPTR, 4096); 
			DWORD dwBytesNeeded;
			if (! QueryServiceConfig( schService, lpqscBuf, 4096, &dwBytesNeeded) ) 
			{
				DebugPrintf(SV_LOG_ERROR,"QueryServiceConfig() failed with Error code :%d \n",GetLastError());
			}
			else 
			{
				//Get the current services startup type
				switch(lpqscBuf->dwStartType)
				{
					case SERVICE_AUTO_START:
						serviceType = INM_SVCTYPE_AUTO ; 
						serviceStartUpType = "Automatic";
						bRet = true;
						break; 
					case SERVICE_DEMAND_START:
						serviceType = INM_SVCTYPE_MANUAL ; 
						serviceStartUpType = "Manual";
						bRet = true;
						break; 
					case SERVICE_DISABLED :
						serviceType = INM_SVCTYPE_DISABLED ; 
						serviceStartUpType = "Disabled";
						bRet = true;
						break; 
				}
			}
			if ( lpqscBuf != NULL ) 
			{
				LocalFree (lpqscBuf); 
			}
			CloseServiceHandle(schService); 
		}
		else
		{
			DebugPrintf(SV_LOG_ERROR,"OpenService() failed with Error code :%d \n ",GetLastError());
		}
		CloseServiceHandle(schSCManager);
	}
	else 
	{
		DebugPrintf(SV_LOG_ERROR,"OpenSCManager() failed with Error code :%d \n ",GetLastError());
	}
	DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
	return bRet ;
}

SVERROR changeServiceType(const std::string& svcName, InmServiceStartType serviceType)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;

    SC_HANDLE schSCManager;
    SC_HANDLE schService;
    SVERROR bRet = SVS_FALSE ;
    DWORD dwServiceType ;
    bool isProperType = true  ;
    switch( serviceType )
    {
        case INM_SVCTYPE_MANUAL :
            dwServiceType = SERVICE_DEMAND_START ;
            break ;
        case INM_SVCTYPE_DISABLED :
            dwServiceType = SERVICE_DISABLED ;
            break ;
        case INM_SVCTYPE_AUTO :
            dwServiceType = SERVICE_AUTO_START ;
            break ;
        default :
            isProperType = false ;
    }
    if( isProperType )
    {
    // Get a handle to the SCM database. 
    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if(schSCManager)
	{
		schService = OpenService( 
				schSCManager,            // SCM database 
				svcName.c_str(),               // name of service 
				SERVICE_CHANGE_CONFIG);  // need change config access 
 
		if (NULL != schService )
		{ 
	        
			// Change the service start type.
		    if (! ChangeServiceConfig(
		        schService,        // handle of service 
		        SERVICE_NO_CHANGE, // service type: no change 
		            dwServiceType,  // service start type 
		        SERVICE_NO_CHANGE, // error control: no change 
			    NULL,              // binary path: no change 
		        NULL,              // load order group: no change 
				NULL,              // tag ID: no change 
				NULL,              // dependencies: no change 
				NULL,              // account name: no change 
				NULL,              // password: no change 
				NULL) )            // display name: no change
			{
				DebugPrintf(SV_LOG_ERROR, "ChangeServiceConfig failed (%d)\n", GetLastError());
			}
			else
			{
				DebugPrintf(SV_LOG_ERROR, "ChangeServiceConfig succeeded\n");
				bRet = SVS_OK;
			}
		}
		else
		{
			DebugPrintf(SV_LOG_ERROR,"openservice() failed with Error code :%d\n",GetLastError());
		}
	}
    else 
	{
		DebugPrintf(SV_LOG_DEBUG,"OpenSCManager() failed with Error code :%d\n",GetLastError());
	}
    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return bRet ;
}

void TestServiceFunctions()
{
    InmServiceStatus status_;
    isServiceInstalled("AGT", status_); 
    isServiceInstalled("AppMgmt", status_);
    
    getServiceStatus("AppMgmt", status_) ;
}


SVERROR QuerySvcConfig(InmService& svc)
{
    SC_HANDLE schSCManager = NULL ;
    SC_HANDLE schService  = NULL ;
    LPQUERY_SERVICE_CONFIG lpsc = NULL ; 
    LPSERVICE_DESCRIPTION lpsd = NULL ;
    DWORD dwBytesNeeded = 0 ;
    DWORD cbBufSize = 0 ;
    DWORD dwError = 0 ; 
    SVERROR bRet = SVS_FALSE ;
    svc.m_svcStartType = INM_SVCTYPE_NONE ;
    svc.m_svcStatus = INM_SVCSTAT_NONE ;

    schSCManager = OpenSCManager( 
        NULL,                    // local computer
        NULL,                    // ServicesActive database 
        SC_MANAGER_ALL_ACCESS);  // full access rights 
 
    if (NULL != schSCManager) 
    {
        DebugPrintf(SV_LOG_DEBUG, "Opened scmanager\n");
        schService = OpenService( 
            schSCManager,          // SCM database 
            svc.m_serviceName.c_str(),       // name of service 
            SERVICE_QUERY_CONFIG); // need query config access 
     
        if (schService != NULL)
        {
            DebugPrintf(SV_LOG_DEBUG, "Opened handle to service %s\n", svc.m_serviceName.c_str());
            if( !QueryServiceConfig( 
                schService, 
                NULL, 
                0, 
                &dwBytesNeeded))
            {
                dwError = GetLastError();
                if( ERROR_INSUFFICIENT_BUFFER == dwError )
                {
                    cbBufSize = dwBytesNeeded;
                    lpsc = (LPQUERY_SERVICE_CONFIG) LocalAlloc(LMEM_FIXED, cbBufSize);
                }
          
                if( QueryServiceConfig( 
                    schService, 
                    lpsc, 
                    cbBufSize, 
                    &dwBytesNeeded) ) 
                {

                    if( !QueryServiceConfig2( 
                        schService, 
                        SERVICE_CONFIG_DESCRIPTION,
                        NULL, 
                        0, 
                        &dwBytesNeeded))
                    {
                        dwError = GetLastError();
                        if( ERROR_INSUFFICIENT_BUFFER == dwError )
                        {
                            cbBufSize = dwBytesNeeded;
                            lpsd = (LPSERVICE_DESCRIPTION) LocalAlloc(LMEM_FIXED, cbBufSize);
                        }
                     
                        if (QueryServiceConfig2( 
                            schService, 
                            SERVICE_CONFIG_DESCRIPTION,
                            (LPBYTE) lpsd, 
                            cbBufSize, 
                            &dwBytesNeeded) ) 
                        {
                            switch( lpsc->dwStartType )
                            {
                                case SERVICE_AUTO_START :
                                    svc.m_svcStartType = INM_SVCTYPE_AUTO ;
                                    break ;
                                case SERVICE_BOOT_START :
                                    svc.m_svcStartType  = INM_SVCTYPE_AUTO ;
                                    break ;
                                case SERVICE_DEMAND_START :
                                    svc.m_svcStartType = INM_SVCTYPE_MANUAL ;
                                    break ;
                                case SERVICE_DISABLED :
                                    svc.m_svcStartType = INM_SVCTYPE_DISABLED ;
                                    break ;
                                case SERVICE_SYSTEM_START :
                                    svc.m_svcStartType = INM_SVCTYPE_AUTO ;
                                    break ;
                                default :
                                    svc.m_svcStartType = INM_SVCTYPE_NONE ;
                                    break ;
                            }

                            svc.m_binPathName = lpsc->lpBinaryPathName ;
                            svc.m_logonUser = lpsc->lpServiceStartName ;
                            svc.m_displayName = lpsc->lpDisplayName ;
                            if (lpsd->lpDescription != NULL && lstrcmp(lpsd->lpDescription, TEXT("")) != 0)
                            {
                                svc.m_discription = lpsd->lpDescription ; 
                            }
                            if (lpsc->lpDependencies != NULL && lstrcmp(lpsc->lpDependencies, TEXT("")) != 0)
                            {
                                svc.m_dependencies = lpsc->lpDependencies ;
                            }
                            getServiceStatus(svc.m_serviceName, svc.m_svcStatus) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcName        : %s\n", svc.m_serviceName.c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "svcStartUpType : %s\n", svc.typeAsStr().c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcStatus      : %s\n", svc.statusAsStr().c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcDescription : %s\n", svc.m_discription.c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcLogonUser   : %s\n", svc.m_logonUser.c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcBinPath     : %s\n", svc.m_binPathName.c_str()) ;
                            DebugPrintf(SV_LOG_DEBUG, "SvcDepedencies : %s\n", svc.m_dependencies.c_str()) ;
                            bRet = SVS_OK ;
                        }
                    }
                }
            }
            CloseServiceHandle(schService);
        }
        else
        {
            DWORD err = GetLastError();
			std::stringstream sserr;
			sserr << err;
            DebugPrintf(SV_LOG_DEBUG, "Failed to open handle to service %s with error %s\n", svc.m_serviceName.c_str(), sserr.str().c_str());
            svc.m_svcStatus = INM_SVCSTAT_NOTINSTALLED ;
        }
        CloseServiceHandle(schSCManager);
    }
    else
    {
        DWORD err = GetLastError();
		std::stringstream sserr;
		sserr << err;
        DebugPrintf(SV_LOG_ERROR, "Failed to open scmanager with error %s\n", sserr.str().c_str());
    }
    if( lpsc != NULL )
    {
        LocalFree(lpsc); 
    }
    if( lpsd != NULL )
    {
        LocalFree(lpsd);
    }
    return bRet ;
}

InmService::InmService(const std::string& servicename)
{
    m_serviceName = servicename ;
}

InmService::InmService(const InmServiceStatus &status)
{
	m_svcStatus = status;
}
std::string InmService::statusAsStr() const 
{
    switch( m_svcStatus )
    {
        case INM_SVCSTAT_NONE :
            return "Unknown" ;
            break ;
        case INM_SVCSTAT_NOTINSTALLED :
            return "Not Installed" ;
            break ;
        case INM_SVCSTAT_INSTALLED :
            return "Installed" ;
            break ;
        case INM_SVCSTAT_STOPPED :
            return "Not Running" ;
            break ;
        case INM_SVCSTAT_START_PENDING :
            return "Start Pending" ;
            break ;
        case INM_SVCSTAT_STOP_PENDING :
            return "Stop Pending" ;
            break ;
        case INM_SVCSTAT_RUNNING :
            return "Running" ;
            break ;
        case INM_SVCSTAT_CONTINUE_PENDING :
            return "Continue Pending" ;
            break ;
        case INM_SVCSTAT_PAUSE_PENDING :
            return "Pause Pending" ;
            break ;
        case INM_SVCSTAT_PAUSED :
            return "Paused" ;                
            break ;
        default :
            return "Unknown" ;
            break ;
    
    }
}
std::string InmService::typeAsStr() const
{
    switch( m_svcStartType )
    {
    case INM_SVCTYPE_NONE :
        return "Unknown" ;
        break ;
    case INM_SVCTYPE_MANUAL :
        return "Manual" ;
        break ;
    case INM_SVCTYPE_DISABLED :
        return "Disabled" ;
        break ;
    case INM_SVCTYPE_AUTO :
        return "Automatic" ;
        break ;
    default:
        return "Unknown" ;
        break ;
    }
}

std::list<std::pair<std::string, std::string> > InmService::getProperties()
{
    std::list<std::pair<std::string, std::string> > properties ;
    properties.push_back(std::make_pair("Name", this->m_serviceName)) ;
    properties.push_back(std::make_pair("Status", this->statusAsStr())) ;
    if( !(this->m_svcStatus == INM_SVCSTAT_NOTINSTALLED || 
        this->m_svcStatus == INM_SVCSTAT_ACCESSDENIED || 
        this->m_svcStatus == INM_SVCSTAT_NONE) )
    {
        properties.push_back(std::make_pair("Display Name", this->m_displayName)) ;
        properties.push_back(std::make_pair("Discription", this->m_discription)) ;
        properties.push_back(std::make_pair("Startup Type", this->typeAsStr())) ;
        properties.push_back(std::make_pair("Log On User", this->m_logonUser)) ;
        if( ! this->m_dependencies.empty() )
        {
            properties.push_back(std::make_pair("Dependencies", this->m_dependencies)) ;
        }
    }
    return properties ;
}