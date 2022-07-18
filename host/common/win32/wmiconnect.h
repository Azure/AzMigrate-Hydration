#ifndef WMI__CONNECT__H

#define WMI__CONNECT__H

#include <string>
#include <vector>
#include <list>

#define _WIN32_DCOM

#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

#include <ace/Thread_Manager.h>
#include <ace/Guard_T.h>
#include <ace/Mutex.h>

#include "svtypes.h"

typedef std::list<std::list<std::string> > ResultSet;

#define SafeReleaseComPtr(ptr) { if (ptr != NULL) { ptr->Release(); ptr = NULL; } }

struct WmiProperty final
{
    std::wstring Name;
    VARIANT Value;
    VARTYPE Type;

    WmiProperty(const std::wstring &name, VARTYPE variantType)
        : Name(name), Type(variantType)
    {
        VariantInit(&Value);
    }

    template<class DataType>
    HRESULT SetValue(DataType value);

    ~WmiProperty()
    {
        VariantClear(&Value);
    }
};

class WINWMIConnector
{
private:
    IWbemServices * m_WbemServices;
    IWbemLocator* m_WbemLocator;
    std::string m_objectNameSpace;

    typedef ACE_Guard<ACE_Recursive_Thread_Mutex> AutoGuard;

    static ACE_Recursive_Thread_Mutex m_recursiveLock;

    HRESULT getClassPath(IWbemClassObject *pObject, std::wstring &classPath);

public:
    ~WINWMIConnector();
    WINWMIConnector();

    SVERROR ConnectToServer(const std::string& hostName, const std::string& objectPath);

    SVERROR execQuery(const std::string& query, IEnumWbemClassObject** pEnumerator);

    HRESULT execMethod(
        LPWSTR classOrInstancePath,
        LPWSTR methodName,
        std::vector<WmiProperty> &inParamsList,
        std::vector<WmiProperty> &outParamsList);

    HRESULT execMethod(
        IWbemClassObject *pClassOrInstanceObject,
        LPWSTR methodName,
        std::vector<WmiProperty> &inParamsList,
        std::vector<WmiProperty> &outParamsList);
};

#endif