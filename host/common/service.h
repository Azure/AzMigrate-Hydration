#ifndef __SERVICE__H
#define __SERVICE__H

#include <string>
#include <list>
#include <vector>
#include "svtypes.h"

enum InmServiceStartType
{
    INM_SVCTYPE_NONE,
    INM_SVCTYPE_MANUAL,
    INM_SVCTYPE_DISABLED,
    INM_SVCTYPE_AUTO
} ;

enum InmServiceStatus
{

    INM_SVCSTAT_NONE,
    INM_SVCSTAT_ACCESSDENIED,
    INM_SVCSTAT_NOTINSTALLED,
    INM_SVCSTAT_INSTALLED,
    INM_SVCSTAT_STOPPED,
    INM_SVCSTAT_START_PENDING ,
    INM_SVCSTAT_STOP_PENDING, 
    INM_SVCSTAT_RUNNING, 
    INM_SVCSTAT_CONTINUE_PENDING, 
    INM_SVCSTAT_PAUSE_PENDING, 
    INM_SVCSTAT_PAUSED 
} ;

struct InmService
{
	SV_UINT m_pid ;
    std::string m_serviceName ;
    std::string m_displayName ;
    InmServiceStartType m_svcStartType ;
	InmServiceStatus m_svcStatus  ;
	std::string m_binPathName ;
	std::string m_logonUser ;
    std::string m_dependencies ;
    std::string m_discription ;

    InmService(const std::string& servicename) ;
	InmService(const InmServiceStatus &status);
    std::string statusAsStr() const ;
    std::string typeAsStr() const ;
    std::list<std::pair<std::string, std::string> > getProperties() ;
} ;

SVERROR isServiceInstalled(const std::string& serviceName, InmServiceStatus& status) ;
SVERROR getServiceStatus(const std::string& serviceName, InmServiceStatus& status) ;
SVERROR StrService(const std::string& svcName);
SVERROR StpService(const std::string& svcName) ;
SVERROR StartSvc(const std::string& svcName) ;
SVERROR StopSvc(const std::string& svcName);
SVERROR ReStartSvc(const std::string& svcName);
SVERROR FindDependentServicesAndStop(const std::string& svcName, std::vector<std::string>& vecDependentSvcNames);
SVERROR FindDependentServicesAndStart(const std::string& svcName);
void TestServiceFunctions() ;
SVERROR QuerySvcConfig(InmService& svc);
SVERROR changeServiceType(const std::string& svcName, InmServiceStartType serviceType);
bool getStartServiceType(const std::string& svcName, InmServiceStartType &serviceType , std::string &serviceStartUpType); 
#endif