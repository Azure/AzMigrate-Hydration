//
// talwrapper.cpp: simplifies tal's interface
//
// so acts like a function<string (string)>
//
#include <iostream>
#include <string>
#include <map>
#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <cassert>
#include <curl/curl.h>
#include "unmarshal.h"
#include "serializer.h"
#include "apiwrapper.h"
#include "svtypesstub.h"        // toString(sverror)
#include "inmageex.h"
#include <ace/Guard_T.h>
#include "ace/Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include <ace/Manual_Event.h>
#include "localconfigurator.h"

using namespace std;

ApiWrapper::ApiWrapper( const SerializeType_t serializetype ) :
m_serializeType(serializetype)
{
}

ApiWrapper::ApiWrapper( std::string const& destination_, const int &port_, const SerializeType_t serializetype ) :
m_serializeType(serializetype)
{
    if (PHPSerialize == m_serializeType)
    {
        m_tal.SetDestIPAndPort( destination_, port_ );
    }
    else
    {
        /* TODO: throw error ? */
    }
}

void ApiWrapper::operator()(Serializer &serializer)
{
    if (PHPSerialize == m_serializeType)
    {
        serializer.m_SerializedResponse = m_tal( serializer.m_SerializedRequest );
    }
    else if (Xmlize == m_serializeType)
    {
        m_xml( serializer.m_FunctionInfo );
    }
    else
    {
        /* TODO: throw error ? */
    }
}


