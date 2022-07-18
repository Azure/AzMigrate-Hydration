/*
+------------------------------------------------------------------------------------+
Copyright(c) Microsoft Corp. 2015
+------------------------------------------------------------------------------------+
File		:	WmiConnect.h

Description	:   Warpper class for WMI connection

History		:   29-4-2015 (Venu Sivanadham) - Created
+------------------------------------------------------------------------------------+
*/
#ifndef AZURE_RECOVERY_WMI_CONNECT_H
#define AZURE_RECOVERY_WMI_CONNECT_H

#include <string>
#include <list>

#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#include <ace/Thread_Manager.h>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>

namespace AzureRecovery
{

typedef std::list<std::list<std::string> > ResultSet ;

class WINWMIConnector

{
    IWbemServices* m_WbemServices ;

    IWbemLocator* m_WbemLocator ;

    std::string m_objectNameSpace ;

    typedef ACE_Guard<ACE_Recursive_Thread_Mutex> AutoGuard;

    static ACE_Recursive_Thread_Mutex m_lock ;

public:

    ~WINWMIConnector() ;

    WINWMIConnector() ;

    bool ConnectToServer(const std::string& hostName, const std::string& objectPath) ;

	bool execQuery(const std::string& query, IEnumWbemClassObject** pEnumerator);

	IWbemServices* GetNamespace() const;
} ;

} // namespace AzureRecovery

#endif //~AZURE_RECOVERY_WMI_CONNECT_H

