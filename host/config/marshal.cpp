//
// marshal.cpp: marshal C++ structures -> subset of PHP arrays
//
// PHP's Serialization Format
// Type     Serialized                  Example 
// NULL     N;                          N; 
// Integer  i:$data;                    i:123; 
// Double   d:$data;                    d:1.23; 
// Float    d:$data;                    d:1.23; 
// Boolean  b:$bool_value;              b:1; 
// String   s:$data_length:"$data";     s:5:"Hello" 
// Array    a:$key_count:{$key;$value}  a:1:{i:1;i:2} 
//          $value can be any data type 
//          if $key is a string, it is serialized as an int if it looks like one
//
//
#include "marshal.h"

#ifdef SV_WINDOWS
#include <yvals.h>
//#else

#endif
#include <svtypes.h>
#include <sstream>

std::ostream& operator <<( std::ostream& o, CxArgObj<int> x ) {
    return o << "i:" << x.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned short> x ) {
    return o << "i:" << x.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned> x ) {
    return o << "i:" << x.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<long> x ) {
    return o << "i:" << x.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned long> x ) {
    // PR#5554
    // We send unsigned long as if they were strings to match with the
    // format that is being used by CX
    std::stringstream s;
    s << x.value;
    std::string const& stringVal = s.str();
    return o << "s:" << (unsigned int)stringVal.size() << ":\"" << stringVal << "\";";
    //return o << "i:" << x.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<SV_LONGLONG> x ) {
    // We send long long's as if they were strings because PHP on 32 bit arch
    // cannot hold 64 bit integers
    std::stringstream s;
    s << x.value;
    std::string const& stringVal = s.str();
    return o << "s:" << (unsigned int)stringVal.size() << ":\"" << stringVal << "\";";
}

std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned long long> x ) {
    std::stringstream s;
    s << x.value;
    std::string const& stringVal = s.str();
    return o << "s:" << (unsigned int)stringVal.size() << ":\"" << stringVal << "\";";
    
}

std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned char> x ) {
    return o << cxArg( static_cast<unsigned>( x.value ) );
}

std::ostream& operator <<( std::ostream& o, CxArgObj<bool> b ) {
    return o << "b:" << b.value << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<float> f ) {
    return o << "d:" << f.value << ';';
}

// sending d:3 or d:1.334E-05 seems to work fine
std::ostream& operator <<( std::ostream& o, CxArgObj<double> d ) {
    return o << "d:" << d.value << ';';
}

// BUGBUG: if s has NUL characters, << truncates the string
std::ostream& operator <<( std::ostream& o, CxArgObj<std::string> s ) {
    return o << "s:" << static_cast<unsigned long>( s.value.length() ) << ':' << '"' << s.value << '"' << ';';
}

std::ostream& operator <<( std::ostream& o, CxArgObj<char const*> x ) {
    return o << cxArg( std::string( x.value ) );
}

