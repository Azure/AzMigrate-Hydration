#ifndef INM_ALERT_H
#define INM_ALERT_H

#include <string>
#include <map>

class InmAlert
{
public:
    typedef std::map<std::string, std::string> Parameters_t;

public:
    virtual std::string GetID(void) const = 0;
    virtual Parameters_t GetParameters(void) const = 0;
    virtual std::string GetMessage(void) const = 0;
    virtual ~InmAlert() {}
};

#endif /* INM_ALERT_H */
