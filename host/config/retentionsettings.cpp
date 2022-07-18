// Copyright (c) 2006 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File       : retentionsettings.cpp
//
// Description:
//

#include "retentionsettings.h"


#if 0
//
// Serialization support
//

template<>
CDP_SPACEPOLICY unmarshal<CDP_SPACEPOLICY>(
    UnmarshalStream& in, int version )
{
    CDP_SPACEPOLICY s;
    in >> s.bphysicalsize >> s.size;
    return s;
}

std::ostream& operator<<(std::ostream& o, CxArgObj<CDP_SPACEPOLICY> v) {
    CDP_SPACEPOLICY const& x = v.value;
    o << "a:3:{i:0;i:0;"
      << "i:1;" << cxArg( x.bphysicalsize )
      << "i:2;" << cxArg( x.size )
      << "}";
   return o;
}

template<>
CDP_TIMEPOLICY unmarshal<CDP_TIMEPOLICY>(
    UnmarshalStream& in, int version )
{
    CDP_TIMEPOLICY s;
    in >> s.m_time >> s.m_whentoenforce;
    return s;
}

std::ostream& operator<<(std::ostream& o, CxArgObj<CDP_TIMEPOLICY> v) {
    CDP_TIMEPOLICY const& x = v.value;
    o << "a:3:{i:0;i:0;"
      << "i:1;" << cxArg( x.m_time )
      << "i:2;" << cxArg( x.m_whentoenforce )
      << "}";
    return o;
}

template<>
CDP_SETTINGS unmarshal<CDP_SETTINGS>(
    UnmarshalStream& in, int version ) 
{
    CDP_SETTINGS s;
    in >> s.m_state >> s.m_type >> s.m_metadir >> s.m_dbname >> s.m_altmetadir
       >> s.m_datadirs >> s.m_compress >> s.m_deleteoption >> s.m_spacepolicy
       >> s.m_timepolicy;
    return s;
}

std::ostream& operator<<(std::ostream& o, CxArgObj<CDP_SETTINGS> v) {
    CDP_SETTINGS const& x = v.value;
    o << "a:11:{i:0;i:0;"
      << "i:1;" << cxArg( x.m_state )
      << "i:2;" << cxArg( x.m_type )
      << "i:3;" << cxArg( x.m_metadir )
      << "i:4;" << cxArg( x.m_dbname )
      << "i:5;" << cxArg( x.m_altmetadir )
      << "i:6;" << cxArg( x.m_datadirs )
      << "i:7;" << cxArg( x.m_compress )
      << "i:8;" << cxArg( x.m_deleteoption )
      << "i:9;" << cxArg( x.m_spacepolicy )
      << "i:10;" << cxArg( x.m_timepolicy )
      << "}";
    return o;
}

#endif

