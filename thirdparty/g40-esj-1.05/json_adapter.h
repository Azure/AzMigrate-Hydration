/*

    Extremely simple JSON serialization for C++
        
    www.github.com/g40/esj

    Copyright (c) Jerry Evans, 2012-2014
    All rights reserved.

    Trivial example:

    Given a class with std::wstring members a,b,c implement
    a function called serialize() as in:

    void serialize(JSON::json_adapter& adapter)
    {
        JSON::Class root(adapter,"classname");
        JSON_E(adapter,a);
        JSON_E(adapter,b);
        JSON_T(adapter,c);	// Note 'T' type
    }

    That's all that is required. The adapter can be a reader or writer, the
    system is symmetric.

    Given the constraints of JSON/Javascript semantics the only container supported 
    out of the box is std::vector<> and there is obviously no pointer support as the 
    additional JSON overhead required would not interop with any JS consumers/producers.

    The MIT License (MIT)

    Copyright (c) 2012-2014 Jerry Evans

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.


*/



#ifndef JSON_ADAPTER_H
#define JSON_ADAPTER_H

//
#include <string>
#include <vector>
#include <stack>
#include <set>
#include <map>

// requires lexer for token types.
#include "json_lexer.h"
#include <boost/foreach.hpp>
#include <boost/type_traits.hpp>
#include <boost/property_tree/ptree.hpp>

using boost::property_tree::ptree;

namespace JSON
{

//-----------------------------------------------------------------------------
// Base class for serialization operations
class Adapter
{
    // scope count
    int _count;
    bool _is_exception_thrown;

    public:

        Adapter() : _count(0), _is_exception_thrown(false) {}

    // must be implemented
    virtual bool storing() = 0;

    // do some counting so we know if we are the root object
    int inc() 
    { 
        int ret = _count; _count++; return ret; 
    }
    //
    int dec() 
    { 
        return (--_count);
    }

    bool IsExceptionThrown() { return _is_exception_thrown; }
    void SetExceptionThrown() {
        _is_exception_thrown = true;
    }

    // mm. this is warty.
    virtual bool more(TokenType) { return true; }

    // we need some look-ahead
    virtual bool peek(TokenType) { return false; }

    // lexical elements like '{'
    virtual void serialize(TokenType) = 0;

    // support for fundamental types so we can stream vector<T>
    virtual void serialize(const std::string&) = 0;
    virtual void serialize(std::wstring&) = 0;
    virtual void serialize(std::string&) = 0;
    virtual void serialize(int&) = 0;
    virtual void serialize(unsigned int&) = 0;
    virtual void serialize(int64_t&) = 0;
    virtual void serialize(uint64_t&) = 0;
    virtual void serialize(double&) = 0;
    virtual void serialize(bool&) = 0;
    
    // key/value pairs
    virtual void serialize(const std::string&,std::wstring&,bool) = 0;
    virtual void serialize(const std::string&,std::string&,bool) = 0;
    virtual void serialize(const std::string&,int&,bool) = 0;
    virtual void serialize(const std::string&,unsigned int&,bool) = 0;
    virtual void serialize(const std::string&,int64_t&,bool) = 0;
    virtual void serialize(const std::string&, uint64_t&, bool) = 0;
    virtual void serialize(const std::string&,unsigned char&,bool) = 0;
    virtual void serialize(const std::string&,double&,bool) = 0;
    virtual void serialize(const std::string&,bool&,bool) = 0;

};

//-----------------------------------------------------------------------------
#define DBMSGV(arg)

#ifndef _ASTRING
#define _ASTRING(arg) (arg)
#endif
#ifndef _USTRING
#define _USTRING(arg) L##arg
#endif

//-----------------------------------------------------------------------------
// Some helper macros for brevity
#define JSON_C(json_adapter,class_type_name)  JSON::Class __cls(json_adapter,_ASTRING(#class_type_name))
// Serializable element with more elements to follow
#define JSON_E(json_adapter,class_member)  JSON::stream(json_adapter,_ASTRING(#class_member),class_member,true)
// Key-value pair of Serializable element with more elements to follow. 
// This doesn't support conditional serialization or repeated use of a member variable
// TODO-1805: throw compiler exception if used incorrectly
#define JSON_KV_E(json_adapter,key,element) JSON::stream(json_adapter, key, element, true)
// Serializable terminator, keep things simple for the JSON writer. No stream manipulations required
#define JSON_T(json_adapter,class_member)  JSON::stream(json_adapter,_ASTRING(#class_member),class_member,false)
// Key-value pair of Serializable element with terminator.
// This doesn't support conditional serialization or repeated use of a member variable
// TODO-1805: throw compiler exception if used incorrectly
#define JSON_KV_T(json_adapter,key,element) JSON::stream(json_adapter, key, element, false)
// Deprecated
// TODO-1804: remove 
#define JSON_E_KV_PRODUCER_ONLY JSON_KV_E
#define JSON_T_KV_PRODUCER_ONLY JSON_KV_T


//-----------------------------------------------------------------------------
// for literals
inline void stream(Adapter& adapter,TokenType type)
{
    //
    adapter.serialize(type);
}

//-----------------------------------------------------------------------------
inline void stream(Adapter& adapter,const std::string& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
inline void stream(Adapter& adapter,std::string& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
// fundamental types so we can directly serialize vectors of string/wstring/int/bool ...
inline void stream(Adapter& adapter,std::wstring& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
inline void stream(Adapter& adapter,int& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
inline void stream(Adapter& adapter,int64_t& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
inline void	stream(Adapter& adapter,double& value)
{
    //
    adapter.serialize(value);
}

//-----------------------------------------------------------------------------
inline void	stream(Adapter& adapter,bool& value)
{
    //
    adapter.serialize(value);
}


//-----------------------------------------------------------------------------
// string
inline void	stream(Adapter& adapter,const std::string& key,std::string& value,bool more)
{
    //
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// wstring
inline void stream(Adapter& adapter,const std::string& key,std::wstring& value,bool more)
{
    //
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// doubles
inline void	stream(Adapter& adapter,const std::string& key,double& value,bool more)
{
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// int
inline void	stream(Adapter& adapter,const std::string& key,int& value,bool more)
{
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// int
inline void	stream(Adapter& adapter,const std::string& key,unsigned int& value,bool more)
{
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// int64_t
inline void	stream(Adapter& adapter,const std::string& key,int64_t& value,bool more)
{
    adapter.serialize(key,value,more);
}

// uint64_t
inline void	stream(Adapter& adapter, const std::string& key, uint64_t& value, bool more)
{
    adapter.serialize(key, value, more);
}

//-----------------------------------------------------------------------------
inline void stream(Adapter& adapter, const std::string& key, unsigned char& value, bool more) 
{
    adapter.serialize(key,value,more);
}

//-----------------------------------------------------------------------------
// bool
inline void	stream(Adapter& adapter,const std::string& key,bool& value,bool more)
{
    adapter.serialize(key,value,more);
}


//-----------------------------------------------------------------------------
// arbitrary type T. must implement serialize with the correct signature
template <typename T>
inline void stream(Adapter& adapter,T& arg)
{
    //
    arg.serialize(adapter);
}

//-----------------------------------------------------------------------------
// serialize a single instance of T. 
// Highlights an asymmetry in JSON (or more likely Javascript ...)
template <typename T>
inline void stream(Adapter& adapter,const std::string& key,T& value,bool more)
{
    adapter.serialize(key);
    adapter.serialize(T_COLON);
    adapter.serialize(T_OBJ_BEGIN);
    // handle the value
    value.serialize(adapter);
    adapter.serialize(T_OBJ_END);
    if (more)
    {
        // handle the trailing comma if this is part of a sequence
        adapter.serialize(T_COMMA);
    }
}

//-----------------------------------------------------------------------------
// this is the serializer for std::set<std::string>
inline void stream(Adapter& adapter, const std::string& key, std::set<std::string>& value, bool more)
{
    typedef std::set<std::string>::iterator iterator_t;
    if (adapter.storing())
    {
        adapter.serialize(key);
        adapter.serialize(T_COLON);
        adapter.serialize(T_OBJ_BEGIN);
        iterator_t it = value.begin();
        if (it != value.end())
        {
            // VC2012 cannot disambiguate the type of T when a vector of bool is used.
            std::string t = *it;
            stream(adapter, t);
            for (++it; it != value.end(); ++it)
            {
                adapter.serialize(T_COMMA);
                t = *it;
                stream(adapter, t);
            }
        }
        adapter.serialize(T_OBJ_END);

        if (more)
        {
            adapter.serialize(T_COMMA);
        }
    }
    else
    {
        // expecting "name" ':'
        adapter.serialize(key);
        //
        adapter.serialize(T_COLON);
        // '['
        adapter.serialize(T_OBJ_BEGIN);
        // cope with empty arrays so we need look-ahead here
        if (adapter.peek(T_OBJ_END))
        {
            // '}'
            adapter.serialize(T_OBJ_END);
        }
        else
        {
            do
            {
                // create a new instance
                std::string t;
                // read off adapter
                stream(adapter, t);

                // push back into map
                value.insert(t);
                // keep going if we have a ',', end if '}'
            } while (adapter.more(T_OBJ_END));
        }
        // 
        if (more)
        {
            // there will be more ...
            adapter.serialize(T_COMMA);
        }
    }
}

//-----------------------------------------------------------------------------
// this is the serializer for dictionary
inline void stream(Adapter& adapter, const std::string& key, std::map<std::string, std::string>& value, bool more)
{
    typedef std::map<std::string, std::string>::iterator iterator_t;
    if (adapter.storing())
    {
        adapter.serialize(key);
        adapter.serialize(T_COLON);
        adapter.serialize(T_OBJ_BEGIN);
        iterator_t it = value.begin();
        if (it != value.end())
        {
            // VC2012 cannot disambiguate the type of T when a vector of bool is used.
            std::string t1 = it->first;
            std::string t2 = it->second;
            stream(adapter, t1);
            adapter.serialize(T_COLON);
            stream(adapter, t2);
            for (++it; it != value.end(); ++it)
            {
                adapter.serialize(T_COMMA);
                t1 = it->first;
                t2 = it->second;
                stream(adapter, t1);
                adapter.serialize(T_COLON);
                stream(adapter, t2);
            }
        }
        adapter.serialize(T_OBJ_END);

        if (more)
        {
            adapter.serialize(T_COMMA);
        }
    }
    else
    {
        // expecting "name" ':'
        adapter.serialize(key);
        //
        adapter.serialize(T_COLON);
        // '['
        adapter.serialize(T_OBJ_BEGIN);
        // cope with empty arrays so we need look-ahead here
        if (adapter.peek(T_OBJ_END))
        {
            // '}'
            adapter.serialize(T_OBJ_END);
        }
        else
        {
            do
            {
                // create a new instance
                std::string t1, t2;
                // read off adapter
                stream(adapter, t1);
                adapter.serialize(T_COLON);
                stream(adapter, t2);

                // push back into map
                value.insert(std::make_pair(t1, t2));
                // keep going if we have a ',', end if '}'
            } while (adapter.more(T_OBJ_END));
        }
        // 
        if (more)
        {
            // there will be more ...
            adapter.serialize(T_COMMA);
        }
    }
}

//-----------------------------------------------------------------------------
// this is the serializer for dictionary of class T objects
template <typename T>
inline void stream(Adapter& adapter, const std::string& key, std::map<std::string, T>& value, bool more)
{
    typedef typename::std::map<std::string, T>::iterator iterator_t;
    if (adapter.storing())
    {
        adapter.serialize(key);
        adapter.serialize(T_COLON);
        adapter.serialize(T_OBJ_BEGIN);
        iterator_t it = value.begin();
        if (it != value.end())
        {
            // VC2012 cannot disambiguate the type of T when a vector of bool is used.
            std::string t1 = it->first;
            T t2 = it->second;
            stream(adapter, t1);
            adapter.serialize(T_COLON);
            adapter.serialize(T_OBJ_BEGIN);
            stream(adapter, t2);
            adapter.serialize(T_OBJ_END);
            for (++it; it != value.end(); ++it)
            {
                adapter.serialize(T_COMMA);
                t1 = it->first;
                t2 = it->second;
                stream(adapter, t1);
                adapter.serialize(T_COLON);
                adapter.serialize(T_OBJ_BEGIN);
                stream(adapter, t2);
                adapter.serialize(T_OBJ_END);
            }
        }
        adapter.serialize(T_OBJ_END);

        if (more)
        {
            adapter.serialize(T_COMMA);
        }
    }
    else
    {
        // expecting "name" ':'
        adapter.serialize(key);
        //
        adapter.serialize(T_COLON);
        // '['
        adapter.serialize(T_OBJ_BEGIN);
        // cope with empty arrays so we need look-ahead here
        if (adapter.peek(T_OBJ_END))
        {
            // '}'
            adapter.serialize(T_OBJ_END);
        }
        else
        {
            do
            {
                // create a new instance
                std::string t1;
                T t2;
                // read off adapter
                stream(adapter, t1);
                adapter.serialize(T_COLON);
                adapter.serialize(T_OBJ_BEGIN);
                stream(adapter, t2);
                adapter.serialize(T_OBJ_END);

                // push back into map
                value.insert(std::make_pair(t1, t2));
                // keep going if we have a ',', end if '}'
            } while (adapter.more(T_OBJ_END));
        }
        // 
        if (more)
        {
            // there will be more ...
            adapter.serialize(T_COMMA);
        }
    }
}

//-----------------------------------------------------------------------------
// this means we get (with quotes removed for clarity) vectors of string etc
// written out as:
// array : [ e0, e1, ..., en ]
// rather than
// array : [ { e0 }, { e1 }, ..., { eN } ]
// 
template <typename T>
inline void stream_primitives(Adapter& adapter,const std::string& key,std::vector<T>& value,bool more)
{
    typedef typename::std::vector<T>::iterator iterator_t;
    if (adapter.storing())
    {
        adapter.serialize(key);
        adapter.serialize(T_COLON);
        adapter.serialize(T_ARRAY_BEGIN);
        iterator_t it = value.begin();
        if (it != value.end())
        {
            // VC2012 cannot disambiguate the type of T when a vector of bool is used.
            T t = (*it);
            stream(adapter,t);
            for (++it; it != value.end(); ++it)
            {
                adapter.serialize(T_COMMA);
                t = (*it);
                stream(adapter,t);
            }
        }
        adapter.serialize(T_ARRAY_END);
        if (more)
        {
            adapter.serialize(T_COMMA);
        }
    }
    else
    {
        // expecting "name" ':'
        adapter.serialize(key);
        //
        adapter.serialize(T_COLON);
        // '['
        adapter.serialize(T_ARRAY_BEGIN);
        // cope with empty arrays so we need look-ahead here
        if (adapter.peek(T_ARRAY_END))
        {
            // ']'
            adapter.serialize(T_ARRAY_END);
        }
        else
        {
            do 
            {
                // create a new instance
                T t;
                // read off adapter
                stream(adapter,t);
                // push back into vector
                value.push_back(t);
                // keep going if we have a ',', end if ']'
            } while (adapter.more(T_ARRAY_END));
        }
        // 
        if (more)
        {
            // there will be more ...
            adapter.serialize(T_COMMA);
        }
    }
}

//-----------------------------------------------------------------------------
// this is the serializer for any non-primitive types including your own ...
template <typename T>
inline void stream_classes(Adapter& adapter, const std::string& key, std::vector<T>& value, bool more)
{
    typedef typename::std::vector<T>::iterator iterator_t;
    if (adapter.storing())
    {
        adapter.serialize(key);
        adapter.serialize(T_COLON);
        adapter.serialize(T_ARRAY_BEGIN);
        iterator_t it = value.begin();
        if (it != value.end())
        {
            // VC2012 cannot disambiguate the type of T when a vector of bool is used.
            T t = (*it);
            adapter.serialize(T_OBJ_BEGIN); // <-- achtung baby. avoid when JSONizing strings or numbers (!)
            stream(adapter, t);
            adapter.serialize(T_OBJ_END);
            for (++it; it != value.end(); ++it)
            {
                adapter.serialize(T_COMMA);
                t = (*it);
                adapter.serialize(T_OBJ_BEGIN);
                stream(adapter, t);
                adapter.serialize(T_OBJ_END);
            }
        }
        adapter.serialize(T_ARRAY_END);
        if (more)
        {
            adapter.serialize(T_COMMA);
        }
    }
    else
    {
        // expecting "name" ':'
        adapter.serialize(key);
        //
        adapter.serialize(T_COLON);
        // '['
        adapter.serialize(T_ARRAY_BEGIN);
        // cope with empty arrays so we need look-ahead here
        if (adapter.peek(T_ARRAY_END))
        {
            // ']'
            adapter.serialize(T_ARRAY_END);
        }
        else
        {
            do
            {
                // create a new instance
                T t;
                adapter.serialize(T_OBJ_BEGIN);
                // read off adapter
                stream(adapter, t);

                adapter.serialize(T_OBJ_END);
                // push back into vector
                value.push_back(t);
                // keep going if we have a ',', end if ']'
            } while (adapter.more(T_ARRAY_END));
        }
        // 
        if (more)
        {
            // there will be more ...
            adapter.serialize(T_COMMA);
        }
    }
}

    // each C++ class which can be serialized needs to declare a 
    // single Class instance in the top-most scope of the 
    // serialize() function. This is to ensure correct JSON class
    // delimiters are written to/read from the stream
    class Class
    {
        // adapter instance
        Adapter* m_pAdapter;
        bool    m_includeClassName;

    public:

        Class(Adapter& adapter,const std::string& type_name, bool includeClassName=true) 
                : m_pAdapter(&adapter),
                m_includeClassName(includeClassName)
        {
            // monitor the nesting level - this could be checked at runtime
            // to ensure nesting is not excessively deep to protect stack resources.
            int count = m_pAdapter->inc();
            //
            //DBMSG(type_name << " " << count);
            // is this the opening scope?
            if (count == 0)
            {
                m_pAdapter->serialize(T_OBJ_BEGIN);
            }
        
            //if (m_includeClassName) 
            //{
            //    // emit/check "type_name"
            //    m_pAdapter->serialize(type_name);
            //    
            //    m_pAdapter->serialize(T_COLON);

            //    // constructor emits opening scope
            //    m_pAdapter->serialize(T_OBJ_BEGIN);
            //}
        }

        // this is a bit nasty as it causes a double exception
        // if you get a type mismatch or somesuch in the lexer
        ~Class()
        {
            // destructor emits closing scope(s)
            // if (m_includeClassName) 
            //    m_pAdapter->serialize(T_OBJ_END);

            // is this the final scope?
            int count = m_pAdapter->dec();
            if (count == 0)
            {
                if (m_pAdapter->storing())
                    m_pAdapter->serialize(T_OBJ_END);
            }
        }
    };

//-----------------------------------------------------------------------------
// serialize a vector of T.
template <typename T>
inline void stream(Adapter& adapter,const std::string& key,std::vector<T>& value,bool more)
{
    // https://github.com/g40/esj/pull/2/files

    // yack. Now there is a reason why this approach was chosen. The usual (and in my
    // opinion better) way to do this is to have a number of suitably overloaded
    // template functions. However so doing can be ambiguous given that we *only*
    // want C++ strings and primitive bool/int/float. The resultant explosion of
    // interfaces makes things far less clear
    bool json_primitive = (boost::is_same<T,std::wstring>::value || 
                        boost::is_same<T,std::string>::value ||
                        boost::is_integral<T>::value ||
                        boost::is_floating_point<T>::value);
    // but we _can_ make the implementation a little easier to follow by doing this:
    if (json_primitive)
    {
        stream_primitives<T>(adapter,key,value,more);
    }
    else
    {
        stream_classes<T>(adapter,key,value,more);
    }
        //
    // https://github.com/g40/esj/pull/2/files
}

//-----------------------------------------------------------------------------
// the boost::property_tree::ptree based read-only serialization to support any order
// of the class members and optional members

// single instance class or a primitive type
#define JSON_CL(node, class_member)  JSON::stream_class(node, _ASTRING(#class_member), class_member)
#define JSON_P_KEYNAME(node, key, class_member)  JSON::stream_primitive(node, key, class_member)
#define JSON_P(node, class_member)  JSON_P_KEYNAME(node, _ASTRING(#class_member), class_member)

// for vectors of class or primitive type
#define JSON_VCL_KEYNAME(node, key, class_member)  JSON::stream_classes(node, key, class_member)
#define JSON_VCL(node, class_member)  JSON_VCL_KEYNAME(node, _ASTRING(#class_member), class_member)
#define JSON_VP(node, class_member)  JSON::stream_primitives(node, _ASTRING(#class_member), class_member)

#define JSON_KV_P_KEYNAME(node, key, class_member) JSON::stream_kv(node, key, class_member)
#define JSON_KV_P(node, class_member) JSON_KV_P_KEYNAME(node, _ASTRING(#class_member), class_member)
#define JSON_KV_CL_KEYNAME(node, key, class_member) JSON::stream_kv_class(node, key, class_member)
#define JSON_KV_CL(node, class_member) JSON_KV_CL_KEYNAME(node, _ASTRING(#class_member), class_member)

#define   CONT_IF_CHILD_PRESENT(node, key, child) \
    child = node.get_child_optional(key); \
    if (!child) return;

// serialize primitive types
template <typename T>
inline void stream_primitive(ptree& node, const std::string& key, T& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);
     
    value = opChild.get().get_value<T>();
}

// serialize class types
template <typename T>
inline void stream_class(ptree& node, const std::string& key, T& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);
    
    value.serialize(opChild.get());
}

// serialize array of primitives
template <typename T>
inline void stream_primitives(ptree& node, const std::string& key, std::vector<T>& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);

    BOOST_FOREACH(ptree::value_type &v, opChild.get()) 
    {
        T primitive = v.second.get_value<T>();
        value.push_back(primitive);
    }
}

// serialize array of classes
template <typename T>
inline void stream_classes(ptree& node, const std::string& key, std::vector<T>& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);

    BOOST_FOREACH(ptree::value_type &v, opChild.get())
    {
        T cls;
        cls.serialize(v.second);
        value.push_back(cls);
    }
}

// serialize dictionary
inline void stream_kv(ptree& node, const std::string& key, std::map<std::string, std::string>& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);

    BOOST_FOREACH(ptree::value_type &v, opChild.get())
    {
        value.insert(std::make_pair(v.first, v.second.data()));
    }
}

template <typename T>
inline void stream_kv_class(ptree& node, const std::string& key, std::map < std::string, T>& value)
{
    boost::optional<ptree &> opChild;
    CONT_IF_CHILD_PRESENT(node, key, opChild);

    BOOST_FOREACH(ptree::value_type &v, opChild.get())
    {
        T cls;
        cls.serialize(v.second);
        value.insert(std::make_pair(v.first, cls));
    }
}

}   // JSON


#endif
