//
// xmlunmarshal.cpp: marshal C++ structures -> request/response object
//
#include "xmlunmarshal.h"

/* TODO: code is common for all xml unmarshall routines 
 *       Hence there can be only one routine */
void extractfromreqresp( const ParameterGroup& o, int &x, const std::string &name )
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for int name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            x = boost::lexical_cast<int>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected interger. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        /* TODO: should version be handled here ? 
         *       This can happen if xml store is old
         *       and does not have an entry for this 
         *       name.
         *       currently not doing anything ? 
         *       Is unmarshall the only place to 
         *       check version information ? 
         *       should version be a member of of all structures ? */
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, unsigned short &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for unsigned short name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        /* TODO: how do we verify that value is not signed ? */
        try
        {
            x = boost::lexical_cast<unsigned short>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected unsigned short. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, unsigned &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for unsigned int name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        /* TODO: how do we verify that value is not signed ? */
        try
        {
            x = boost::lexical_cast<unsigned int>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected unsigned integer. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, long &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for long name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            x = boost::lexical_cast<long>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
             std::stringstream stream;
            stream<<"For "<< name << "expected long. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, unsigned long &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for unsigned long name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        /* TODO: how do we verify that value is not signed ? */
        try
        {
            x = boost::lexical_cast<unsigned long>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected unsigned long. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, bool &b , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for bool name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        /* TODO: Need further understanding like nonzeros to 1 and 0 to false */
        try
        {
            b = boost::lexical_cast<bool>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected bool. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, float &f , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for float name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            f = boost::lexical_cast<float>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected float. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, double &d , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for double name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            d = boost::lexical_cast<double>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected double. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, std::string &s , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for string name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        s = i->second.value;
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, long long &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for long long name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            x = boost::lexical_cast<long long>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected long long. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, unsigned long long &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for unsigned long long name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        /* TODO: how do we verify that value is not signed ? */
        try
        {
            x = boost::lexical_cast<unsigned long long>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected unsigned long long. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}


void extractfromreqresp( const ParameterGroup& o, unsigned char &x , const std::string &name)
{
    #ifdef DEBUG_XMLIZE
    std::cout << "extractfromreqresp called for unsigned char name:" << name << "\n";
    #endif
    Parameters_t::const_iterator i = o.m_Params.find(name);
    if (i != o.m_Params.end())
    {
        try
        {
            x = boost::lexical_cast<unsigned char>(i->second.value);
        }
        catch(boost::bad_lexical_cast&) {
            std::stringstream stream;
            stream<<"For "<< name << "expected unsigned char. But got " << i->second.value;
            throw INMAGE_EX("EXCEPTION : ")(stream.str());
        }
    }
    else
    {
        throw INMAGE_EX("could not find expected parameter name:")(name);
    }
}

