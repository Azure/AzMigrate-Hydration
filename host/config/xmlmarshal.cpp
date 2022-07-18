//
// xmlmarshal.cpp: marshal C++ structures -> request/response object
//

#include "xmlmarshal.h"

/* TODO: code is common for all xml marshall routines 
 *       Hence there can be only one routine */
void insertintoreqresp( ParameterGroup& o, CxArgObj<int> x, const std::string &name )
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for int name:" << name << "\n";
    #endif

    const std::string TYPE = "int";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned short> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for unsigned short name:" << name << "\n";
    #endif

    const std::string TYPE = "int";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for unsigned int name:" << name << "\n";
    #endif

    const std::string TYPE = "int";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


/* TODO: long and unsigned long both have to be string; 
 * in both serialize and xmlize since long is 64 bit 
 * on gcc running on 64 bit machines */
void insertintoreqresp( ParameterGroup& o, CxArgObj<long> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for long name:" << name << "\n";
    #endif

    const std::string TYPE = "int";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned long> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for long name:" << name << "\n";
    #endif

    const std::string TYPE = "string";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<bool> b , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for bool name:" << name << "\n";
    #endif

    const std::string TYPE = "bool";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << b.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<float> f , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for float name:" << name << "\n";
    #endif

    const std::string TYPE = "double";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << f.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<double> d , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for double name:" << name << "\n";
    #endif

    const std::string TYPE = "double";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << d.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<std::string> s , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for string name:" << name << "\n";
    #endif

    const std::string TYPE = "string";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << s.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<char const*> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for char const * name:" << name << "\n";
    #endif
    insertintoreqresp(o, std::string(x.value), name);
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<long long> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for long long name:" << name << "\n";
    #endif

    const std::string TYPE = "string";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned long long> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for unsigned long long name:" << name << "\n";
    #endif

    const std::string TYPE = "string";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


void insertintoreqresp( ParameterGroup& o, CxArgObj<unsigned char> x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "insertintoreqresp called for unsigned char name:" << name << "\n";
    #endif

    const std::string TYPE = "unsigned char";
    if (name.empty()) {
        throw INMAGE_EX("name should not be empty to xmlize:")(TYPE);
    }

    std::stringstream ss;
    ss << x.value;
    ValueType v(TYPE, ss.str());
    o.m_Params.insert(std::make_pair(name, v));
}


