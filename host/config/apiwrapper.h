//
// apiwrapper.h: simplifies api's interface 
// so acts like a function<string (string)>
//
#ifndef APIWRAPPER_H
#define APIWRAPPER_H

#include <boost/shared_ptr.hpp>
#include <string>
#include "serializationtype.h"
#include "talwrapper.h"
#include "xmlwrapper.h"

class Serializer;

class ApiWrapper {
public:
    ApiWrapper( const SerializeType_t serializetype );
    ApiWrapper( std::string const& destination_, const int &port_, const SerializeType_t serializetype );
    void operator()(Serializer &serializer);
private:
    SerializeType_t m_serializeType;
    TalWrapper m_tal;
    XmlWrapper m_xml;
};

#endif // APIWRAPPER_H


