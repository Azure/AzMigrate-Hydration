//
// xmlmarshal.h: marshal C++ structures -> request/response object
//
#ifndef CONFIGURATORXMLMARSHAL__H
#define CONFIGURATORXMLMARSHAL__H

#include <map>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/remove_const.hpp>
#include "InmFunctionInfo.h"
#include "cxarg.h"
#include "svenum.h"
#include "inmageex.h"

#ifdef DEBUG_XMLIZE
#include <iostream>
#endif

/* TODO: code is common for all xml marshall routines 
 *       Hence there can be only one routine */
void insertintoreqresp( ParameterGroup& o, CxArgObj<int> x, const std::string &name );

void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned short> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<long> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned long> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<bool> b , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<float> f , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<double> d , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<std::string> s , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<char const*> x , const std::string &name);

/* TODO: This is not defined in serial marshall;
 *       Hence not defining this here too */
void insertintoreqresp( ParameterGroup& o, CxArgObj<char *> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<long long> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned long long> x , const std::string &name);

void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned char> x , const std::string &name);

template <typename T>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<std::vector<T> > vec , const std::string &name = std::string() ) {
    ParameterGroup *ppg = &o;
    if (!name.empty()) {
        std::pair<ParameterGroupsIter_t, bool> pgpair = o.m_ParamGroups.insert(std::make_pair(name, ParameterGroup()));
        ParameterGroupsIter_t &pgpairiter = pgpair.first;
        ParameterGroup &pg = pgpairiter->second;
        ppg = &pg;
    }

    typename std::vector<T>::size_type index = 0;
    for( typename std::vector<T>::const_iterator i( vec.value.begin() ); i != vec.value.end(); ++i, ++index ) {
        std::string elemname(name);
        std::stringstream ssindex;
        ssindex  << index;
        elemname += '[';
        elemname += ssindex.str();
        elemname += ']';
        insertintoreqresp(*ppg, cxArg ( *i ), elemname);
    }
}


template <typename T>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<std::list<T> > l , const std::string &name = std::string() ) {
    ParameterGroup *ppg = &o;
    if (!name.empty()) {
        std::pair<ParameterGroupsIter_t, bool> pgpair = o.m_ParamGroups.insert(std::make_pair(name, ParameterGroup()));
        ParameterGroupsIter_t &pgpairiter = pgpair.first;
        ParameterGroup &pg = pgpairiter->second;
        ppg = &pg;
    }

    typename std::list<T>::size_type index = 0;
    for( typename std::list<T>::const_iterator i( l.value.begin() ); i != l.value.end(); ++i, ++index ) {
        std::string elemname(name);
        std::stringstream ssindex;
        ssindex  << index;
        elemname += '[';
        elemname += ssindex.str();
        elemname += ']';
        insertintoreqresp(*ppg, cxArg ( *i ), elemname);
    }
}


template <typename KEY, typename VALUE, typename COMP>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<std::map<KEY, VALUE, COMP> > m , const std::string &name = std::string() ) {
    ParameterGroup *ppg = &o;
    if (!name.empty()) {
        std::pair<ParameterGroupsIter_t, bool> pgpair = o.m_ParamGroups.insert(std::make_pair(name, ParameterGroup()));
        ParameterGroupsIter_t &pgpairiter = pgpair.first;
        ParameterGroup &pg = pgpairiter->second;
        ppg = &pg;
    }

    for( typename std::map<KEY, VALUE, COMP>::const_iterator i( m.value.begin() ); i != m.value.end(); ++i )
    {
        std::stringstream ss;
        ss << i->first;
        insertintoreqresp(*ppg, cxArg ( i->second ), ss.str());
    }
}


template <typename T0, typename T1>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<std::pair<T0, T1> > p , const std::string &name = std::string() ) {
    ParameterGroup *ppg = &o;
    if (!name.empty()) {
        std::pair<ParameterGroupsIter_t, bool> pgpair = o.m_ParamGroups.insert(std::make_pair(name, ParameterGroup()));
        ParameterGroupsIter_t &pgpairiter = pgpair.first;
        ParameterGroup &pg = pgpairiter->second;
        ppg = &pg;
    }

    std::stringstream ss;
    ss << p.value.first;
    insertintoreqresp(*ppg, cxArg ( p.value.second ), ss.str());
}


template <size_t N>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<char [N]> arg , const std::string &name) {
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for char[] name:" << name << "\n";
    #endif
    insertintoreqresp(o, std::string(arg.value), name);
}


// no <false> declared; we only handle basic types + enums
template <typename T>
 void streamXmlArg(ParameterGroup& o,CxArgObj<T> arg,is_enum_tag2<true>, const std::string &name) {
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for enum name:" << name << "\n";
    #endif

    const std::string TYPE = "int";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << arg.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


template <typename T>
 void insertintoreqresp( ParameterGroup& o, CxArgObj<T> arg , const std::string &name) {
    streamXmlArg( o, arg, is_enum_tag2< boost::is_enum<T>::value >(), name );
}

#endif // CONFIGURATORXMLMARSHAL__H

