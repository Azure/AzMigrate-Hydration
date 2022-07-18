#ifndef INM_NOTICE_ALERT_IMP_H
#define INM_NOTICE_ALERT_IMP_H

#include <string>
#include <sstream>
#include <iomanip>
#include "inmalertimp.h"
#include "inmnoticealertnumbers.h"

class InmNoticeAlertImp : public InmAlertImp
{
protected:
    void SetDetails(const E_INM_NOTICE_ALERT_NUMBER &n, const Parameters_t &parameters, const std::string &message) 
    { 
        std::stringstream id;
        id << "AA" << std::setw(4) << std::setfill('0') << n;
        InmAlertImp::SetDetails(id.str(), parameters, message);
    }
};

#endif /* INM_NOTICE_ALERT_IMP_H */
