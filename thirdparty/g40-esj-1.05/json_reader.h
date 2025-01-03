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


#ifndef JSON_READER_H
#define JSON_READER_H

#ifndef JSON_ADAPTER_H
#include "json_adapter.h"
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using boost::property_tree::ptree;


namespace JSON
{

    //-----------------------------------------------------------------------------
    // for deserializing
    class Reader : public Adapter
    {
        // the scanner
        Scanner* m_pScanner;
        // token stack. we need 1 look-ahead
        std::stack<Token> m_tokens;
        //
        Token m_token;

        //-----------------------------------------------------------------------------
        TokenType Next()
        {
            if (m_tokens.empty() == false)
            {
                m_token = m_tokens.top();
                m_tokens.pop();
            }
            else
            {
                m_token = m_pScanner->Scan();
            }
            return m_token.type;
        }

        //-----------------------------------------------------------------------------
        // need lookahead for some cases, especially arrays which can be empty or
        // "key" : string|number|bool
        bool _Peek(TokenType type)
        {
            Token ret = m_pScanner->Scan();
            m_tokens.push(ret);
            return ret.type == type;
        }

        //-----------------------------------------------------------------------------
        inline void throw_if(bool condition,std::string msg)
        {
            if (condition == true && !IsExceptionThrown())
            {
                SetExceptionThrown();

                Chordia::stringer sb;
                sb << "Error (" << m_pScanner->Line() << ':' << m_pScanner->Col() << ") " << msg;
                __PLATFORM_THROW(json_exception(sb.c_str()));
            }
        }

        //-----------------------------------------------------------------------------
        // do while (condition)
        bool loop_or_end(TokenType continue_type,TokenType end_type)
        {
            //
            TokenType next = Next();
            //
            // token type matches?
            if (next == end_type)
            {
                return false;
            }
            if (next == continue_type)
            {
                return true;
            }
            // no clean way to do this ....
            throw_if(true,"expecting ',' or ']'");
            // never reached
            return false;
        }


        void _UntilObjEnd()
        {
            uint64_t numofobj = 0;
            uint64_t numofarray = 0;

            // ignore the value
            TokenType next = Next();
            do
            {
                if (next == T_OBJ_END)
                {
                    if (numofobj > 0)
                        numofobj--;
                    else
                    {
                        return;
                    }
                }
                else if (next == T_OBJ_BEGIN)
                {
                    numofobj++;
                }
                else if (next == T_ARRAY_BEGIN)
                {
                    numofarray++;
                }
                else if (next == T_ARRAY_END)
                {
                    if (numofarray > 0)
                        numofarray--;
                    else
                    {
                        Chordia::stringer errmsg;
                        errmsg << "_UntilObjEnd: T_OBJ_END not found but reached end of array.";

                        throw_if(true, errmsg.c_str());
                    }
                }
                else if (next == T_EOF)
                {
                    // quit if EOF reached
                    Chordia::stringer errmsg;
                    errmsg << "_UntilObjEnd: T_OBJ_END not found til EOF.";

                    throw_if(true, errmsg.c_str());
                }

                next = Next();
            } while (true);

            return;
        }

        //-----------------------------------------------------------------------------
        // core type checking primitive
        virtual void GetNext(TokenType type)	
        {
            if (type == T_OBJ_END)
                return _UntilObjEnd();

            // get the next token
            TokenType next = Next();
            // token type matches?
            // is required to handle null strings?
            if ((type == T_STRING) && (next == T_NULL))
                next = T_STRING;

            if (next != type)
            {
                Chordia::stringer errmsg;
                errmsg << "GetNext: type mismatch - Expected type ";
                errmsg << TokenTypeStr[type];
                errmsg << " Next token type ";
                errmsg << TokenTypeStr[next];

                throw_if(true, errmsg.c_str());
            }
        }

        //-----------------------------------------------------------------------------
        // primitive to read type-checked key/value pair as in "count" : 123
        void GetNext(const std::string& key,TokenType type,std::string& value,bool more)
        {
            // expecting a string token for "key"
            GetNext(T_STRING);
            // checked key name matches parsed value
            // if no-match, skip the key/value pair
            while (key != m_token.text)
            {
                GetNext(T_COLON);

                // ingonre the value
                TokenType next = Next();
                while (next != T_COMMA)
                {
                    // quit if EOF reached
                    if (next == T_EOF)
                    {
                        Chordia::stringer errmsg;
                        errmsg << "GetNext: key \'" << key << "\' not found.";

                        throw_if(true, errmsg.c_str());
                    }

                    next = Next();
                }

                if (_Peek(T_OBJ_BEGIN))
                    Next();

                // get the next key
                GetNext(T_STRING);
            }

            // next token
            GetNext(T_COLON);
            // get the next token and match type
            GetNext(type);
            // conversions from text are performed one level further up the stack
            value = m_token.text;
            // this has to be one of ,]}
            if (more)
            {
                Next();
            }
        }

    public:

        //-----------------------------------------------------------------------------
        Reader(Scanner& scanner) : 
            m_pScanner(&scanner)
        {}

        //-----------------------------------------------------------------------------
        virtual bool storing() { return false; }

        //-----------------------------------------------------------------------------
        virtual bool more(TokenType type)
        { 
            return loop_or_end(T_COMMA, type);
        }

        //-----------------------------------------------------------------------------
        virtual bool peek(TokenType type) 
        { 
            return _Peek(type);
        }

        //-----------------------------------------------------------------------------
        // read and match a delimiter {}[],:
        virtual void serialize(TokenType delimiter) 
        {
            GetNext(delimiter);
        }

        //-----------------------------------------------------------------------------
        // primitive to read type-checked key/value pair
        virtual void serialize(const std::string& key,std::string& value,bool more)
        {
            GetNext(key,T_STRING,value,more);
        }


        //-----------------------------------------------------------------------------
        virtual void serialize(const std::string& key,std::wstring& value,bool more)
        {
            std::string ss;
            GetNext(key,T_STRING,ss,more);
            value = Chordia::n2w(ss);
        }
        
        virtual void serialize(const std::string& key,int& value,bool more)
        {
            std::string ss;
            GetNext(key,T_NUMBER,ss,more);
            value = static_cast<int>(Chordia::toInt(ss));
        }
        
        virtual void serialize(const std::string& key,unsigned int& value,bool more)
        {
            std::string ss;
            GetNext(key,T_NUMBER,ss,more);
            value = static_cast<unsigned int>(Chordia::toInt(ss));
        }

        virtual void serialize(const std::string& key,int64_t& value,bool more)
        {
            std::string ss;
            GetNext(key,T_NUMBER,ss,more);
            value = static_cast<int64_t>(Chordia::toInt(ss));
        }

        virtual void serialize(const std::string& key, uint64_t& value, bool more)
        {
            std::string ss;
            GetNext(key, T_NUMBER, ss, more);
            value = static_cast<uint64_t>(Chordia::toInt(ss));
        }

        virtual void serialize(const std::string& key,unsigned char& value,bool more)
        {
            std::string ss;
            GetNext(key,T_NUMBER,ss,more);
            value = static_cast<unsigned char>(Chordia::toInt(ss));
        }

        virtual void serialize(const std::string& key,double& value,bool more)
        {
            std::string ss;
            GetNext(key,T_NUMBER,ss,more);
            value = Chordia::toDouble(ss);
        }

        // 
        virtual void serialize(const std::string& key,bool& value,bool more)
        {
            std::string ss;
            GetNext(key,T_BOOLEAN,ss,more);
            value = (ss == "true");
        }

        // read a const value. does nothing with the parameter
        virtual void serialize(const std::string& key)	
        {
            GetNext(T_STRING);

            // added following code to debug which key is coming out of order
            std::string parsedKey = m_token.text;
            if (parsedKey != key)
            {
                Chordia::stringer errmsg;
                errmsg << "GetNext: key mismatch - Expected key ";
                errmsg << key;
                errmsg << " Next key ";
                errmsg << parsedKey;

                throw_if(true, errmsg.c_str());
            }
        }

        // read a quoted singleton value
        virtual void serialize(std::string& value)	
        {
            GetNext(T_STRING);
            // key matches parsed value
            value = m_token.text;
        }

        // strings will be quoted
        virtual void serialize(std::wstring& ret)	
        {
            //
            GetNext(T_STRING);
            // do conversion
            ret = Chordia::n2w(m_token.text);
        }

        virtual void serialize(int& value)	
        {
            GetNext(T_NUMBER);
            // key matches parsed value
            value = static_cast<char>(Chordia::toInt(m_token.text));
        }

        virtual void serialize(unsigned int& value)	
        {
            GetNext(T_NUMBER);
            // key matches parsed value
            value = static_cast<unsigned int>(Chordia::toInt(m_token.text));
        }

        virtual void serialize(int64_t& value)	
        {
            GetNext(T_NUMBER);
            // key matches parsed value
            value = static_cast<int64_t>(Chordia::toInt(m_token.text));
        }

        virtual void serialize(uint64_t& value)
        {
            GetNext(T_NUMBER);
            // key matches parsed value
            value = static_cast<uint64_t>(Chordia::toInt(m_token.text));
        }


        virtual void serialize(double& value)	
        {
            GetNext(T_NUMBER);
            // key matches parsed value
            value = Chordia::toDouble(m_token.text);
        }
        
        virtual void serialize(bool& value)	
        {
            GetNext(T_BOOLEAN);
            value = (m_token.text == "true");
        }
    };

    //-----------------------------------------------------------------------------
    // demonstrate the JSON reader
    template <typename serializable_type, typename source_type = StringSource>
    class consumer
    {
    public:
        static serializable_type convert(const std::string& json)
        {
            // create an instance
            serializable_type target;
            // set up the reader chain
            source_type ss(json.c_str(),json.size());
            // scanner
            JSON::Scanner scanner(ss);
            // and JSON reader
            JSON::Reader reader(scanner);
            // re-hydrate ...
            target.serialize(reader);
            // return the new instance
            return target;
        }

        static serializable_type convert(const std::string& json, bool unordered)
        {
            if (!unordered)
                return convert(json);

            // create an instance
            serializable_type target;
            ptree node;
            std::stringstream ssJson(json);
            boost::property_tree::read_json(ssJson, node);
            // re-hydrate ...
            target.serialize(node);
            // return the new instance
            return target;
        }
    };
}

#endif
