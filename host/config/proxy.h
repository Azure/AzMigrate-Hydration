//
// proxy.h: turns procedure calls on the CX interface into calls on transport
//
#ifndef CONFIGURATORPROXY__H
#define CONFIGURATORPROXY__H

#include <string>
#include <boost/function.hpp>
#include "configuratorrpc.h"
#include "unmarshal.h"

class Serializer;

typedef std::string ConfiguratorDeferredProcedureCall;
typedef boost::function<void (Serializer &)> ConfiguratorMediator;
typedef boost::function<std::string (std::string)> ConfiguratorTransport;

/* todo: create macro to automatically create proxy
#define DECLARE_PROXY1(InterfaceName,name0,Signature0) \
    struct InterfaceName {\
        boost::function<Signature0> name0;\
    };\
    template <>\
    struct Proxy<InterfaceName> : InterfaceName {\
        Proxy() {\
        name0 = boost::bind( unmarshal<boost::function<Signature0>::function_return_type::type>( transport( marshalCxCall( #name0, "inmage_get_configurator" ) ) ) );\
        }\
        boost::function<string (string)> transport;\
    };
*/
/*
template <>
struct Stub<InterfaceName> : InterfaceName {
    Stub();
    template <typename T>
    void bind( T* t ) {
        name0 = boost::bind( t, name0 );
        map[ #name0 ] = name0;
    }
};
*/


#endif // CONFIGURATORPROXY__H


