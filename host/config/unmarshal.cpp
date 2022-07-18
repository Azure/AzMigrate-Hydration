//
// unmarshal.cpp: unmarshal subset of PHP serialization -> C++ structures 
//
#include "unmarshal.h"
#include <boost/lexical_cast.hpp>

using namespace std;

template<>
int unmarshal<int>( UnmarshalStream& str ) {
    char i0, colon, semicolon;
    int value;
    str.in >> i0 >> colon >> value >> semicolon;
    if( 'i' != i0 || ':' != colon || ';' != semicolon ) {
        throw INMAGE_EX("expected i:number; got ")(i0)(colon)(value)(semicolon);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << value << endl;
#endif
    return value;
}

template<>
unsigned int unmarshal<unsigned int>( UnmarshalStream& str ) {
    char i0, colon, semicolon;
    unsigned int value;
    str.in >> i0 >> colon >> value >> semicolon;
    if( 'i' != i0 || ':' != colon || ';' != semicolon ) {
        throw INMAGE_EX("expected i:number; got ")(i0)(colon)(value)(semicolon);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << value << endl;
#endif
    return value;
}

template<>
unsigned short unmarshal<unsigned short>( UnmarshalStream& str ) {
    char i0, colon, semicolon;
    unsigned short value;
    str.in >> i0 >> colon >> value >> semicolon;
    if( 'i' != i0 || ':' != colon || ';' != semicolon ) {
        throw INMAGE_EX("expected i:number; got ")(i0)(colon)(value)(semicolon);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << value << endl;
#endif
    return value;
}

template<>
long long unmarshal<long long>( UnmarshalStream& str ) {
    UnmarshalStream::StateSetter setter( str,
            UnmarshalStream::RAW_READ );
    string stringValue;
    str >> stringValue;
#ifdef TESTBED_DEBUG
	cout << " = " << boost::lexical_cast<long long>( stringValue ) << endl;
#endif
    return boost::lexical_cast<long long>( stringValue );
}

template<>
double unmarshal<double>( UnmarshalStream& str ) {
    char i0, colon, semicolon;
    double value;
    str.in >> i0 >> colon >> value >> semicolon;
    if( 'd' != i0 || ':' != colon || ';' != semicolon ) {
        throw INMAGE_EX("expected d:decimal; got ")(i0)(colon)(value)(semicolon);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << value << endl;
#endif
    return value;
}
/*
template<>
uint64_t unmarshal<uint64_t>( UnmarshalStream& str ) {
    UnmarshalStream::StateSetter setter( str,
            UnmarshalStream::RAW_READ );
    string stringValue;
    str >> stringValue;
#ifdef TESTBED_DEBUG
	cout << " = " << boost::lexical_cast<uint64_t>( stringValue ) << endl;
#endif
    return boost::lexical_cast<uint64_t>( stringValue );
}
*/

template<>
unsigned long unmarshal<unsigned long>( UnmarshalStream& str ) {
    UnmarshalStream::StateSetter setter( str,
            UnmarshalStream::RAW_READ );
    string stringValue;
    str >> stringValue;
    return boost::lexical_cast<unsigned long>( stringValue );
}


template<>
unsigned long long unmarshal<unsigned long long>( UnmarshalStream& str ) {
    UnmarshalStream::StateSetter setter( str,
            UnmarshalStream::RAW_READ );
    string stringValue;
    str >> stringValue;
    return boost::lexical_cast<unsigned long long>( stringValue );
}

template<> unsigned char unmarshal<unsigned char>( UnmarshalStream& str ) {
    UnmarshalStream::StateSetter setter( str,
            UnmarshalStream::RAW_READ );
	int intValue;
	str >> intValue;
#ifdef TESTBED_DEBUG
	cout << " = " << static_cast<unsigned char>( intValue ) << endl;
#endif
	return static_cast<unsigned char>( intValue );
}

template<>
string unmarshal<string>( UnmarshalStream& s ) {
    char ch, colon;
    s.in >> ch >> colon;
    if( 's' != ch || ':' != colon ) {
        //cout << "unmarshal string : got " << ch << colon << endl;
        throw INMAGE_EX("expected s: got ")(ch)(colon);
    }
    int stringLength;
    s.in >> stringLength;
    //cout << "string was " << stringLength << " chars long" << endl;
    if( stringLength < 0 ) {
        throw INMAGE_EX("got string with negative length")(stringLength);
    }
    char colon2, quote1, quote2;
    s.in >> colon2 >> quote1;
    if( ':' != colon2 || '"' != quote1 ) {
        throw INMAGE_EX("expected beginning :\" ")(colon2)(quote1);
    }
    string result;
    result.reserve( stringLength );
    // Eswar - converted string::size_type to int in for loop
    for( int i=0; s.in.good() && i<stringLength; ++i ) {
        result.push_back( s.in.get() );
    }

    s.in >> quote2;
    if( '"' != quote2 ) {
        throw INMAGE_EX("expected ending quote character")(quote2);
    }
    char semicolon;
    s.in >> semicolon;
    if( ';' != semicolon ) {
        throw INMAGE_EX("expected terminating semicolon")(semicolon);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << result << endl;
#endif
    return result;  // how come I don't get warning when this line is deleted?
}

template<>
bool unmarshal<bool>( UnmarshalStream& str ) {
    bool result;
    char b0, colon, semi;
    str.in >> b0 >> colon >> result >> semi;
    if( 'b' != b0 || ':' != colon || !(0==result || 1==result) ||';' != semi ) {
        throw INMAGE_EX("expected b:[01];")(b0)(colon)(result)(semi);
    }
#ifdef TESTBED_DEBUG
	cout << " = " << result << endl;
#endif
    return result;
}

