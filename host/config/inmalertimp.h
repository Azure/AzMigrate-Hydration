#ifndef INM_ALERT_IMP_H
#define INM_ALERT_IMP_H

#include "inmalert.h"

class InmAlertImp : public InmAlert
{
public:
    std::string GetID(void) const { return m_ID; }
    Parameters_t GetParameters(void) const { return m_Parameters; }
    std::string GetMessage(void) const { return m_Message; }

protected:
    void SetDetails(const std::string &id, const Parameters_t &parameters, const std::string &message) 
    { 
        m_ID = id;
        m_Parameters = parameters;
        m_Message = message;
    }

private:
    std::string m_ID;
    Parameters_t m_Parameters;
    std::string m_Message;
};

#endif /* INM_ALERT_IMP_H */
