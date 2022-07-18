#include "service.h"
SVERROR isServiceInstalled(const std::string& serviceName, InmServiceStatus& status)
{
	return SVS_FALSE;
}
SVERROR getServiceStatus(const std::string& serviceName, InmServiceStatus& status)
{
	return SVS_FALSE;
}
SVERROR StopService(const std::string& svcName, SV_UINT  timeout)
{
	return SVS_FALSE;
}
SVERROR StartSvc(const std::string& svcName)
{
	return SVS_FALSE;
}
SVERROR QuerySvcConfig(InmService& svc)
{
	return SVS_FALSE;
}
SVERROR changeServiceType(const std::string& svcName, InmServiceStartType serviceType)
{
	return SVS_FALSE;
}


