#ifndef INM_ERROR_ALERT_IMP_H
#define INM_ERROR_ALERT_IMP_H

#include <string>
#include <sstream>
#include <iomanip>
#include "inmalertimp.h"
#include "inmerroralertnumbers.h"

class InmErrorAlertImp : public InmAlertImp
{
protected:
    void SetDetails(const E_INM_ERROR_ALERT_NUMBER &n, const Parameters_t &parameters, const std::string &message) 
    { 
        std::stringstream id;
        id << "EA" << std::setw(4) << std::setfill('0') << n;
        InmAlertImp::SetDetails(id.str(), parameters, message);
    }
};

#endif /* INM_ERROR_ALERT_IMP_H */
