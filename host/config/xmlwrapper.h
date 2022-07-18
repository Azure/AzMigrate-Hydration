//
// xmlwrapper.h: simplifies xml's interface 
// so acts like a function<resp (req)>
//
#ifndef XMLWRAPPER_H
#define XMLWRAPPER_H

#include <boost/shared_ptr.hpp>
#include <string>

struct FunctionInfo;

class XmlWrapper {
public:
     void operator()(FunctionInfo &functioninfo);
};

#endif // XMLWRAPPER_H


