//
// configuratorrpc.cpp: basic remote procedure call from VX->CX and vice versa
//
#include "configuratorrpc.h"

std::string marshalCxCall( const std::string &functionName ) {
    std::ostringstream s;
    s << "a:" << 1 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' << functionName << '"' << ';'
      << "}";
    return s.str();
}

