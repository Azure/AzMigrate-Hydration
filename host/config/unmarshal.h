//
// unmarshal.h: unmarshal subset of PHP serialization -> C++ structures 
//
#ifndef CONFIGURATORUNMARSHAL__H
#define CONFIGURATORUNMARSHAL__H

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <utility>
#include <map>
#include <list>
#include <vector>
#include <typeinfo>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_array.hpp>
#include "inmageex.h"

#ifdef TESTBED_DEBUG
#define TAB_SPACE 5
#define MAX_WORD_LEN 25
extern int tabs;
#endif

struct UnmarshalStream {
    UnmarshalStream(std::stringstream& in_) : in(in_), currentState(RAW_READ) {}
    std::istream& in;

    // The >> operator operates on the stream as is or for objects, filters
    // out the i:num; indices that separate the values of object fields
    enum StreamState { RAW_READ, FILTER_INDICES };
    StreamState currentState;
    std::vector<StreamState> states;
    void pushStreamState( StreamState s ) {
        states.push_back(currentState);
        currentState = s;
    }
    void popStreamState() {
        currentState = states.back();
        states.pop_back();
    }

    // A guard class for setting and unsetting stream states
    struct StateSetter{
        UnmarshalStream& str;
        StateSetter( UnmarshalStream& us, StreamState s ) : str( us ) {
            str.pushStreamState( s );
        }
        ~StateSetter() { str.popStreamState(); }
    };

    template <typename T>
    UnmarshalStream& operator >>( T& t );
};

template <typename T>
T unmarshal( UnmarshalStream& in );

// Treat enums as ints. Does no range checking.
template <bool b>
struct is_enum_tag {};

template <typename T>
T unmarshal( UnmarshalStream&, is_enum_tag<true> );
template <typename T>
T unmarshal( UnmarshalStream&, is_enum_tag<false> );

template <typename T>
T unmarshal( UnmarshalStream& s ) {
    return unmarshal<T>( s, is_enum_tag< boost::is_enum<T>::value >() );
}

template <typename T>
T unmarshal( UnmarshalStream& s, is_enum_tag<true> ) {
    return static_cast<T>( unmarshal<int>( s ) );
}

// If nothing else matches, try to use helper class
template <typename T>
struct UnmarshalHelper {
    static T unmarshalhelper( UnmarshalStream& in );
};

template <typename T>
T unmarshal( UnmarshalStream& in, is_enum_tag<false> ) {
    return UnmarshalHelper<T>::unmarshalhelper( in );
}

template <typename T>
T unmarshal( std::string s ) {
    std::stringstream str( s );
    UnmarshalStream ustr( str );	// starts in raw mode, no need to set
    T result;
    ustr >> result;
    return result;
}

template<> int unmarshal<int>( UnmarshalStream& str );
template<> unsigned short unmarshal<unsigned short>( UnmarshalStream& str );
template<> unsigned int unmarshal<unsigned int>( UnmarshalStream& str );
template<> long long unmarshal<long long>( UnmarshalStream& str );
template<> long long unmarshal<long long>( UnmarshalStream& str );
template<> double unmarshal<double>( UnmarshalStream& str );
template<> unsigned long unmarshal<unsigned long>( UnmarshalStream& str );
template<> unsigned long long unmarshal<unsigned long long>(UnmarshalStream& s);
template<> unsigned char unmarshal<unsigned char>(UnmarshalStream& s);

template<> std::string unmarshal<std::string>( UnmarshalStream& s );

template<> bool unmarshal<bool>( UnmarshalStream& str );

// Distinguish between arrays and non arrays because arrays cannot be assigned
template <bool b>
struct is_array_tag {};

template <typename T, unsigned N> // match func that returns array of N chars
char ( &ArrayCountFunc( T ( & )[ N ] ) )[ N ]; 

template <typename T,size_t N>
struct UnmarshalArrayHelper {
    static void unmarshalhelper( UnmarshalStream& in, T& t ) {
        UnmarshalStream::StateSetter setter( in, 
            UnmarshalStream::RAW_READ );
        //std::cout << "calling unmarshal array with " << N << std::endl;
        // TODO: handle arrays
    }
};

// NOTE: I null terminate all char arrays, treating them like strings.
template <size_t N>
struct UnmarshalArrayHelper<char [N],N> {
    static void unmarshalhelper( UnmarshalStream& str, char (&t)[N] ) {
        //std::cout << "calling unmarshal char array with " << N << std::endl;
        char ch, colon;
        str.in >> ch >> colon;
        if( 's' != ch || ':' != colon ) {
            //std::cout << "unmarshal char array: got " << ch << colon <<std::endl;
            throw INMAGE_EX("expected s: stream input")(ch)(colon);
        }
        int stringLength;
        str.in >> stringLength;
        //std::cout << "string was " << stringLength << " chars long" <<std::endl;
        if( stringLength < 0 ) {
            throw INMAGE_EX("got string with negative length")(stringLength);
        }
        if( stringLength >= N ) {
            throw INMAGE_EX("got string that was too long")(stringLength);
        }
        char colon2, quote1, quote2;
        str.in >> colon2 >> quote1;
        if( ':' != colon2 || '"' != quote1 ) {
            throw INMAGE_EX("expected beginning :\" ")(colon2)(quote1);
        }
        if( !str.in.read( t, stringLength ) ) {
            throw INMAGE_EX("expected more string data")(stringLength);
        }
        t[stringLength] = 0;
#ifdef TESTBED_DEBUG
		cout << " = " << t ;
#endif
        str.in >> quote2;
        if( '"' != quote2 ) {
            throw INMAGE_EX("expected ending quote character")(quote2);
        }
        char semicolon;
        str.in >> semicolon;
        if( ';' != semicolon ) {
            throw INMAGE_EX("expected terminating semicolon")(semicolon);
        }
    }
};

template <typename T>
UnmarshalStream& unmarshal( UnmarshalStream& in, T& t, is_array_tag<true> ) {
    UnmarshalArrayHelper<T,sizeof(ArrayCountFunc(t))>::unmarshalhelper( in, t );
    return in;
}

template <typename T>
T unmarshal( UnmarshalStream& in, int version );

template <bool b>
struct UnmarshalIsStructureTag {};

template <typename T>
T unmarshal( UnmarshalStream& str, UnmarshalIsStructureTag<true> ) {
    //std::cout << "unmarshal version tag true" << std::endl;
    char ch, colon1, colon2, brace;
    int cFields;
    str.in >> ch >> colon1 >> cFields >> colon2 >> brace;
    if( 'a' != ch || ':' != colon1 ) {
        //std::cout << "read " <<ch<<colon1<<cFields << colon2<<brace<<std::endl;
        throw INMAGE_EX("object expected `a:'")(ch)(colon1)(cFields)(colon2)
                       (brace);
    }
    if( cFields < 0 ) {
        throw INMAGE_EX("object array has negative element count")(cFields);
    }
    if( ':' != colon2 ) {
        throw INMAGE_EX("missing colon after a:count")(cFields)(colon2);
    }
    if( '{' != brace ) {
        throw INMAGE_EX("obj missing opening brace after a:count:")(brace);
    }

    char index[7];
    str.in.read( index, sizeof( index ) - 1 );
    index[sizeof(index)-1] = 0;
    if( 0 != strcmp( index, "i:0;i:" ) ) {
        //std::cout << "got " << index << std::endl;
        throw INMAGE_EX("expected i:0;i: at beginning of obj array")(index);
    }

    int version;
    char versionSemicolon;
    str.in >> version >> versionSemicolon;
    if( version < 0 ) {
        throw INMAGE_EX("object had negative version number")(version);
    }
    if( ';' != versionSemicolon ) {
        throw INMAGE_EX("semicolon expected after obj version number")
                       (versionSemicolon);
    }
    UnmarshalStream::StateSetter setter( str, 
        UnmarshalStream::FILTER_INDICES );
    T result = unmarshal<T>( str, version );

    char brace2;
    str.in >> brace2;
    if( '}' != brace2 ) {
        throw INMAGE_EX("didn't eat whole obj array, } missing")(brace2);
    }
    return result;
}

template <typename T>
T unmarshal( UnmarshalStream& in, UnmarshalIsStructureTag<false> ) {
    //std::cout << "unmarshal version tag false" << std::endl;
    return unmarshal<T>( in );
}

template <typename T>
struct UnmarshalIsStructure {
    static const bool Value = false;
};

template <typename T>
UnmarshalStream& unmarshal( UnmarshalStream& in, T& t, is_array_tag<false> ) {
    //std::cout << "unmarshal for nonarray " << typeid(T).name() << ", has version is " << (UnmarshalIsStructure<T>::Value ? "true" : "false") << std::endl;
    t = unmarshal<T>( in, UnmarshalIsStructureTag< UnmarshalIsStructure<T>::Value >() );
    return in;
}

template <typename T>
UnmarshalStream& UnmarshalStream::operator >>( T& t ) {
    if( FILTER_INDICES == currentState ) {
        char i0, colon, semicolon;
        int index;
        in >> i0 >> colon >> index >> semicolon;
        if( 'i' != i0 || ':' != colon || index < 0 || ';' != semicolon ) {
            throw INMAGE_EX("UnmarshalStream>>: expected array index but not found")(i0)(colon)(index)(semicolon);
        }
        //std::cout << "Eating index " << index << std::endl;
    }
    return unmarshal<T>( *this, t, is_array_tag<boost::is_array<T>::value>() );
}

// Declare this type of partial template specialization to unmarshal template arguments
template<typename T0,typename T1>
struct UnmarshalHelper<std::pair<T0,T1> > {
    static std::pair<T0,T1> unmarshalhelper( UnmarshalStream& str ) {
        UnmarshalStream::StateSetter setter( str, 
            UnmarshalStream::RAW_READ );
        // todo: enclose this in an a:2:{..} array?
		char ch, col1, col2, brace;
        int count;
        str.in >> ch >> col1 >> count >> col2 >> brace;
        if('a' != ch || ':' != col1 || count!=2 || ':' != col2 || '{' != brace) {
            throw INMAGE_EX("unmarshal pair expected valid `a:count:{'")(ch)(col1)(count)(col2)(brace);
        }
        T0 t0;
        T1 t1;
		
		char i0, colon, semicolon;
        int index;
        
		str.in >> i0 >> colon >> index >> semicolon;
        if( 'i' != i0 || ':' != colon || index != 0 || ';' != semicolon ) {
			throw INMAGE_EX("loop index i:number; expected")(i0)(colon)(index)(semicolon);
        }
#ifdef TESTBED_DEBUG
		cout <<  endl;
		cout.width(tabs*3+25);
		cout << " Key";
#endif
        str >> t0;

		str.in >> i0 >> colon >> index >> semicolon;
        if( 'i' != i0 || ':' != colon || index != 1 || ';' != semicolon ) {
			throw INMAGE_EX("loop index i:number; expected")(i0)(colon)(index)(semicolon);
        }

		str >> t1; // TODO: actual unmarshal
        char brace2;
        str.in >> brace2;
        if( '}' != brace2 ) {
            throw INMAGE_EX("pair longer than array count indicated")(brace2);
        }
		return std::make_pair( t0, t1 );

    }
};

template<typename T0,typename T1,typename T2>
struct UnmarshalHelper< std::map<T0,T1,T2> > {
    static std::map<T0,T1,T2> unmarshalhelper( UnmarshalStream& str ) {
        UnmarshalStream::StateSetter setter( str, 
            UnmarshalStream::RAW_READ );
        //std::cout << "calling unmarshal on map<T0,T1>" << std::endl;
        char ch, col1, col2, brace;
        int count;
        str.in >> ch >> col1 >> count >> col2 >> brace;
        if( 'a' != ch || ':' != col1 || count < 0 || ':' != col2||'{' !=brace ){
            throw INMAGE_EX("expect valid `a:count:{'")(ch)(col1)(count)(col2)(brace);
        }
        //std::cout << "map has " << count << " elements" << std::endl;
        std::map<T0,T1,T2> myMap;
        for( int i = 0; i < count; ++i ) {
            T0 key;
#ifdef TESTBED_DEBUG
			cout << endl;
			cout.width(tabs*3+25);
			cout << " Key";
#endif           
			str >> key;
			
            //std::cout << "unmarshal map read key: " << key << std::endl;
            T1 value;
            str >> value;
            //std::cout << "unmarshal map read value: " << value << std::endl;
            myMap[ key ] = value;
        }
        char brace2;
        str.in >> brace2;
        if( '}' != brace2 ) {
            throw INMAGE_EX("map longer than array count indicated")(brace2);
        }
        //std::cout << "unmarshal map done" << std::endl;
        return myMap;
    }
};

template <typename T>
struct UnmarshalHelper< std::list<T> > {
    static std::list<T> unmarshalhelper( UnmarshalStream& str ) {
        UnmarshalStream::StateSetter setter( str, 
            UnmarshalStream::RAW_READ );
        //std::cout << "Unmarshaling list of " << typeid(T).name() << std::endl;

        char ch, col1, col2, brace;
        int count;
        str.in >> ch >> col1 >> count >> col2 >> brace;
        if('a' != ch || ':' != col1 || count<0 || ':' != col2 || '{' != brace) {
            throw INMAGE_EX("unmarshal list expected valid `a:count:{'")(ch)(col1)(count)(col2)(brace);
        }
        //std::cout << "list has " << count << " elements" << std::endl;
        std::list<T> myList;
        for( int i = 0; i < count; ++i ) {
            char i0, colon, semicolon;
            int index;
            str.in >> i0 >> colon >> index >> semicolon;
            if( 'i' != i0 || ':' != colon || index != i || ';' != semicolon ) {
                throw INMAGE_EX("loop index i:number; expected")(i0)(colon)(index)(semicolon);
            }
            T element;
            str >> element;
            myList.push_back( element );
        }
        char brace2;
        str.in >> brace2;
        if( '}' != brace2 ) {
            throw INMAGE_EX("list longer than array count indicated")(brace2);
        }
        //std::cout << "Done unmarshaling list of "<<typeid(T).name()<< std::endl;
        return myList;
    }
};

template <typename T>
struct UnmarshalHelper< std::vector<T> > {
    static std::vector<T> unmarshalhelper( UnmarshalStream& str ) {
        UnmarshalStream::StateSetter setter( str, 
            UnmarshalStream::RAW_READ );
		char ch, col1, col2, brace;
        int count;
        str.in >> ch >> col1 >> count >> col2 >> brace;
        if('a' != ch || ':' != col1 || count<0 || ':' != col2 || '{' != brace) {
            throw INMAGE_EX("unmarshal list expected valid `a:count:{'")(ch)(col1)(count)(col2)(brace);
        }
        std::vector<T> myVector;
        for( int i = 0; i < count; ++i ) {
            char i0, colon, semicolon;
            int index;
            str.in >> i0 >> colon >> index >> semicolon;
            if( 'i' != i0 || ':' != colon || index != i || ';' != semicolon ) {
                throw INMAGE_EX("loop index i:number; expected")(i0)(colon)(index)(semicolon);
            }
            T element;
            str >> element;
            myVector.push_back( element );
        }
        char brace2;
        str.in >> brace2;
        if( '}' != brace2 ) {
            throw INMAGE_EX("list longer than array count indicated")(brace2);
        }
        return myVector;
    }
};

#endif // CONFIGURATORUNMARSHAL__H


