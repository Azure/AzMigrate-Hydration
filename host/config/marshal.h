//
// marshal.h: marshal C++ structures -> subset of PHP arrays
//
#ifndef CONFIGURATORMARSHAL__H
#define CONFIGURATORMARSHAL__H

#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "cxarg.h"
#include "svenum.h"

std::ostream& operator <<( std::ostream& o, CxArgObj<int> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned short> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<long> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned long> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<bool> b );
std::ostream& operator <<( std::ostream& o, CxArgObj<float> f );
std::ostream& operator <<( std::ostream& o, CxArgObj<double> d );
std::ostream& operator <<( std::ostream& o, CxArgObj<std::string> s );
std::ostream& operator <<( std::ostream& o, CxArgObj<char const*> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<char *> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<long long> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned long long> x );
std::ostream& operator <<( std::ostream& o, CxArgObj<unsigned char> x );

//Do not define a wchar_t or wstring marshaler. We use UTF-8 as our standard.
//std::ostream& operator <<( std::ostream& o, CxArgObj<std::wstring> s );

template <typename T>
std::ostream& operator <<( std::ostream& o, CxArgObj<std::vector<T> > vec ) {
    o << "a:" << static_cast<unsigned long>( vec.value.size() ) << ":{";
    int index = 0;
    for( typename std::vector<T>::const_iterator i( vec.value.begin() ); i != vec.value.end(); ++i, ++index ) {
        o << "i:" << index << ";" << cxArg( *i );
    }
    return o << '}';
}

template <typename T>
std::ostream& operator <<( std::ostream& o, CxArgObj<std::list<T> > arg ) {
    o << "a:" << static_cast<unsigned long>( arg.value.size() ) << ":{";
    int index = 0;
    for( typename std::list<T>::const_iterator i( arg.value.begin() ); i != arg.value.end(); ++i, ++index ) {
        o << "i:" << index << ";" << cxArg( *i );
    }
    return o << '}';
}


template <typename T1, typename T2,typename T3>
std::ostream& operator <<( std::ostream& o, CxArgObj<std::map<T1,T2,T3> > arg ) {
    o << "a:" << static_cast<unsigned long>( arg.value.size() ) << ":{";
    for( typename std::map<T1,T2,T3>::const_iterator i( arg.value.begin() ); i != arg.value.end(); ++i ) {
        o << cxArg( (*i).first ) << cxArg( (*i).second );
    }
    return o << '}';
}

template <size_t N>
std::ostream& operator <<( std::ostream& o, CxArgObj<char [N]> arg ) {
    return o << "s:" << static_cast<unsigned>( strlen( arg.value ) ) << ":\"" << arg.value << "\";";
}

// no <false> declared; we only handle basic types + enums
template <typename T>
std::ostream& streamCxArg(std::ostream& o,CxArgObj<T> arg,is_enum_tag2<true>) {
    return o << "i:" << arg.value << ";";
}

template <typename T>
std::ostream& operator <<( std::ostream& o, CxArgObj<T> arg ) {
    //return o << "i:" << arg.value << ";";
    return streamCxArg( o, arg, is_enum_tag2< boost::is_enum<T>::value >() );
}

#endif // CONFIGURATORMARSHAL__H

