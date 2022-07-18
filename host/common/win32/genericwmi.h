#ifndef __GENERIC__WMI__H
#define __GENERIC__WMI__H

#include "wmiconnect.h"
#include <string>

class GenericWMI
{
public:
    // Forward decl for befriending
    class WmiRecordProcessor;
    GenericWMI(WmiRecordProcessor*);

    class WmiRecordProcessor
    {
    public:
        WmiRecordProcessor() : m_genericWmiPtr(NULL) { }

        virtual bool Process(IWbemClassObject *precordobj) = 0;
        virtual std::string GetErrMsg(void) = 0;
        virtual std::string GetTraceMsg(void) { return std::string(); }
        virtual ~WmiRecordProcessor() { }

    protected:
        GenericWMI* GetGenericWMI() { return m_genericWmiPtr; }

    private:
        GenericWMI* volatile m_genericWmiPtr;

        friend GenericWMI::GenericWMI(WmiRecordProcessor*);
    };

    SVERROR init();
    SVERROR init(const std::string &wmiProvider);

    void GetData(const std::string &classname);
    void GetDataUsingQuery(const std::string &query);

    SVERROR ExecuteMethod(
        IWbemClassObject *pClassOrInstanceObject,
        LPWSTR methodName,
        std::vector<WmiProperty> &inParamsList,
        std::vector<WmiProperty> &outParamsList);

private:
    bool m_init;
    WINWMIConnector m_wmiConnector;
    std::string m_wmiProvider;
    WmiRecordProcessor *m_pWmiRecordProcessor;
};

#endif