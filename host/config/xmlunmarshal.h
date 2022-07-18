//
// xmlunmarshal.h: marshal C++ structures -> request/response object
//
#ifndef CONFIGURATORUNXMLMARSHAL__H
#define CONFIGURATORUNXMLMARSHAL__H

#include <map>
#include <list>
#include <vector>
#include <string>
#include <utility>
#include <sstream>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/lexical_cast.hpp>
#include "InmFunctionInfo.h"
#include "svenum.h"
#include "cxarg.h"
#include "inmageex.h"

#ifdef DEBUG_XMLIZE
#include <iostream>
#endif

/* TODO: code is common for all xml unmarshall routines 
 *       Hence there can be only one routine */
void extractfromreqresp( const ParameterGroup& o, int &x, const std::string &name );

void extractfromreqresp( const ParameterGroup& o, unsigned short &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, unsigned &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, long &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, unsigned long &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, bool &b , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, float &f , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, double &d , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, std::string &s , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, long long &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, unsigned long long &x , const std::string &name);

void extractfromreqresp( const ParameterGroup& o, unsigned char &x , const std::string &name);

//Do not define a wchar_t or wstring marshaler. We use UTF-8 as our standard.
//ParameterGroup& extractfromreqresp( const ParameterGroup& o, CxArgObj<std::wstring> s );

template<typename T>
 void extractfromreqresp( const ParameterGroup& o, std::vector<T> &vec , const std::string &name = std::string() ) {
    const ParameterGroup *ppg = &o;
    if (!name.empty()) {
        ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);
        if (i != o.m_ParamGroups.end()) {
            const ParameterGroup &pg = i->second;
            ppg = &pg;
        }
        else {
            throw INMAGE_EX("could not find expected parameter name:")(name);
        }
    }

    unsigned long nelems = ppg->m_Params.empty() ? ppg->m_ParamGroups.size() : ppg->m_Params.size();
    for ( unsigned long index = 0; index < nelems; index++) {
        std::string elemname(name);
        std::stringstream ssindex;
        ssindex  << index;
        elemname += '[';
        elemname += ssindex.str();
        elemname += ']';
        T t;
        extractfromreqresp(*ppg, t, elemname);
        vec.push_back(t);
    }
}


template<typename T>
 void extractfromreqresp( const ParameterGroup& o, std::list<T> &l , const std::string &name = std::string() ) {
    const ParameterGroup *ppg = &o;
    if (!name.empty()) {
        ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);
        if (i != o.m_ParamGroups.end()) {
            const ParameterGroup &pg = i->second;
            ppg = &pg;
        }
        else {
            throw INMAGE_EX("could not find expected parameter name:")(name);
        }
    }

    unsigned long nelems = ppg->m_Params.empty() ? ppg->m_ParamGroups.size() : ppg->m_Params.size();
    for ( unsigned long index = 0; index < nelems; index++) {
        std::string elemname(name);
        std::stringstream ssindex;
        ssindex  << index;
        elemname += '[';
        elemname += ssindex.str();
        elemname += ']';
        T t;
        extractfromreqresp(*ppg, t, elemname);
        l.push_back(t);
    }
}

template<typename KEY, typename VALUE, typename COMP>
 void extractfromreqresp( const ParameterGroup& o, std::map<KEY, VALUE, COMP> &m , const std::string &name = std::string() ) {
    const ParameterGroup *ppg = &o;
    if (!name.empty()) {
        ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);
        if (i != o.m_ParamGroups.end()) {
            const ParameterGroup &pg = i->second;
            ppg = &pg;
        }
        else {
            throw INMAGE_EX("could not find expected parameter name:")(name);
        }
    }

    if (!ppg->m_Params.empty())
    {
        Parameters_t::const_iterator it = ppg->m_Params.begin();
        for ( /* empty */ ; it != ppg->m_Params.end(); it++)
        {
            VALUE value;
            extractfromreqresp(*ppg, value, it->first);
            try {
                KEY key = boost::lexical_cast<KEY>(it->first);
                m.insert(std::make_pair(key, value));
            }
            catch(boost::bad_lexical_cast&) {
                throw INMAGE_EX("expected map, but found cound not convert key to map key")(it->first);
            }
        }
    }
    else
    {
        ParameterGroups_t::const_iterator it = ppg->m_ParamGroups.begin();
        for ( /* empty */ ; it != ppg->m_ParamGroups.end(); it++)
        {
            VALUE value;
            extractfromreqresp(*ppg, value, it->first);
            try {
                KEY key = boost::lexical_cast<KEY>(it->first);
                m.insert(std::make_pair(key, value));
            }
            catch(boost::bad_lexical_cast&) {
                throw INMAGE_EX("expected map, but found cound not convert key to map key")(it->first);
            }
        }
    }
}

template<typename T0, typename T1>
 void extractfromreqresp( const ParameterGroup& o, std::pair<T0, T1> &p , const std::string &name = std::string() ) {
    const ParameterGroup *ppg = &o;
    if (!name.empty()) {
        ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);
        if (i != o.m_ParamGroups.end()) {
            const ParameterGroup &pg = i->second;
            ppg = &pg;
        }
        else {
            throw INMAGE_EX("could not find expected parameter name:")(name);
        }
    }

    if (!ppg->m_Params.empty())
    {
        /* TODO: add error checking for size not more than one ? */
        Parameters_t::const_iterator it = ppg->m_Params.begin();
        T1 t1;
        extractfromreqresp(*ppg, t1, it->first);
        try {
            T0 t0 = boost::lexical_cast<T0>(it->first);
            p = std::make_pair(t0, t1);
        }
        catch(boost::bad_lexical_cast&) {
            throw INMAGE_EX("expected map, but found cound not convert t0 to pair t0")(it->first);
        }
    }
    else
    {
        /* TODO: add error checking for size not more than one ? */
        ParameterGroups_t::const_iterator it = ppg->m_ParamGroups.begin();
        T1 t1;
        extractfromreqresp(*ppg, t1, it->first);
        try {
            T0 t0 = boost::lexical_cast<T0>(it->first);
            p = std::make_pair(t0, t1);
        }
        catch(boost::bad_lexical_cast&) {
            throw INMAGE_EX("expected map, but found cound not convert key to map key")(it->first);
        }
    }
}

/* TODO: keeping this incase alternate implementation is needed ? 
template<typename T1, typename T2, typename T3>
 void extractfromreqresp( const ParameterGroup& o, std::map<T1,T2,T3> &m , const std::string &name) {
    ParameterGroups_t::const_iterator i = o.m_ParamGroups.find(name);
    if (i != o.m_ParamGroups.end()) {
        const ParameterGroup &pg = i->second;
        // TODO: error check of % 2 to be added; % 2 should be zero 
        unsigned long nelems = (pg.m_ParamGroups.size() + pg.m_Params.size());
        if (nelems & 1ul)
        {
            // TODO: throw proper error message; remove name parameter
            throw INMAGE_EX("expected map, but found odd number of elements for")(name);
        }
        unsigned long nmapelems = nelems / 2;
        for ( unsigned long index = 0; index < nmapelems; index++) {
            std::string elemname(name);
            std::stringstream ssindex;
            ssindex  << index;
            elemname += '[';
            elemname += ssindex.str();
            elemname += ']';
            std::string keyname = elemname;
            keyname += ".key";
            T1 key;
            extractfromreqresp(pg, key, keyname);
            std::string valuename = elemname;
            valuename += ".value";
            T2 value;
            extractfromreqresp(pg, value, valuename);
            m.insert(std::make_pair(key, value));
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}
*/


/* TODO: add error checking if size later */
template <size_t N>
 void extractfromreqresp( const ParameterGroup& o, char (&x)[N], const std::string &name )
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for char[] name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        std::string::size_type nchars = i->second.value.length();
        if (nchars >= N)
        {
            throw INMAGE_EX("got string that was too long")(nchars);
        }
        std::string::size_type n = 0;
        for ( /* empty */ ; n < nchars; n++)
        {
            x[n] = i->second.value[n];
        }
        x[n] = '\0';
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


template <typename T>
 void extractenumfromreqresp( const ParameterGroup &o, T &x, is_enum_tag2<true>, const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractenumfromreqresp called for enum name:" << name << "\n";
    #endif
    int i;
    extractfromreqresp( o, i, name);
    x = static_cast<T>(i);
}



template <typename T>
 void extractfromreqresp( const ParameterGroup& o, T &x, const std::string &name )
{
    extractenumfromreqresp(o, x, is_enum_tag2< boost::is_enum<T>::value >(), name);
}

#endif // CONFIGURATORUNXMLMARSHAL__H

