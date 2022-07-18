//
// rpc.h: provides basic remote procedure call from VX->CX and vice versa
//
#ifndef CONFIGURATORRPC__H
#define CONFIGURATORRPC__H

#include <sstream>
#include "marshal.h"

std::string marshalCxCall( const std::string &functionName );

template <typename T1>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1 )
{
    std::ostringstream s;
    s << "a:" << 2 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' << functionName << '"' << ";i:1;" << cxArg( arg1 ) << "}";
    return s.str();
}

template <typename T1, typename T2>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2 )
{
    std::ostringstream s;
    s << "a:" << 3 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3 )
{
    std::ostringstream s;
    s << "a:" << 4 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4 )
{
    std::ostringstream s;
    s << "a:" << 5 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5  )
{
    std::ostringstream s;
    s << "a:" << 6 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6  )
{
    std::ostringstream s;
    s << "a:" << 7 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7  )
{
    std::ostringstream s;
    s << "a:" << 8 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8  )
{
    std::ostringstream s;
    s << "a:" << 9 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9  )
{
    std::ostringstream s;
    s << "a:" << 10 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10  )
{
    std::ostringstream s;
    s << "a:" << 11 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11  )
{
    std::ostringstream s;
    s << "a:" << 12 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12  )
{
    std::ostringstream s;
    s << "a:" << 13 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13)
{
    std::ostringstream s;
    s << "a:" << 14 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14)
{
    std::ostringstream s;
    s << "a:" << 15 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "}";
    return s.str();
}
template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15)
{
    std::ostringstream s;
    s << "a:" << 16 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16)
{
    std::ostringstream s;
    s << "a:" << 17 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17)
{
    std::ostringstream s;
    s << "a:" << 18 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) << "}";
    return s.str();
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18)
{
    std::ostringstream s;
    s << "a:" << 19 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) 
	  << "i:18;" << cxArg( arg18 ) << "}";
    return s.str();
}
template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19)
{
    std::ostringstream s;
    s << "a:" << 20 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) 
	  << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 ) << "}";
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20)
{
    std::ostringstream s;
    s << "a:" << 21 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) 
	  << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 ) << "i:20;" << cxArg( arg20 ) << "}";
	  
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20, 
          typename T21>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20, 
    const T21 &arg21)
{
    std::ostringstream s;
    s << "a:" << 22 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) 
	  << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 ) 
      << "i:20;" << cxArg( arg20 ) << "i:21;" << cxArg( arg21 ) << "}";
	  
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20, 
          typename T21, typename T22>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20, 
    const T21 &arg21, const T22 &arg22)
{
    std::ostringstream s;
    s << "a:" << 23 << ":{i:0;s:" 
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"' 
      << functionName << '"' << ";i:1;" << cxArg( arg1 ) 
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 ) 
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 ) 
	  << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 ) 
	  << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
	  << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 ) 
	  << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 ) 
	  << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 ) 
      << "i:20;" << cxArg( arg20 ) << "i:21;" << cxArg( arg21 ) 
      << "i:22;" << cxArg( arg22 ) << "}";
	  
    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23)
{
    std::ostringstream s;
    s << "a:" << 24 << ":{i:0;s:"
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"'
      << functionName << '"' << ";i:1;" << cxArg( arg1 )
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 )
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 )
          << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 )
          << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
          << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 )
          << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 )
          << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 )
      << "i:20;" << cxArg( arg20 ) << "i:21;" << cxArg( arg21 )
      << "i:22;" << cxArg( arg22 ) << "i:23;" << cxArg( arg23 )
      << "}";

    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23, typename T24>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23, const T24 &arg24)
{
    std::ostringstream s;
    s << "a:" << 25 << ":{i:0;s:"
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"'
      << functionName << '"' << ";i:1;" << cxArg( arg1 )
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 )
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 )
          << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 )
          << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
          << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 )
          << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 )
          << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 )
      << "i:20;" << cxArg( arg20 ) << "i:21;" << cxArg( arg21 )
      << "i:22;" << cxArg( arg22 ) << "i:23;" << cxArg( arg23 )
      << "i:24;" << cxArg( arg24 ) << "}";

    return s.str();
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23, typename T24, typename T25>
std::string marshalCxCall( const std::string &functionName, const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 &arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23, const T24 &arg24, const T25 &arg25)
{
    std::ostringstream s;
    s << "a:" << 26 << ":{i:0;s:"
      << static_cast<unsigned long>( functionName.length() ) << ':' << '"'
      << functionName << '"' << ";i:1;" << cxArg( arg1 )
      << "i:2;" << cxArg( arg2 ) << "i:3;" << cxArg( arg3 )
      << "i:4;" << cxArg( arg4 ) << "i:5;" << cxArg( arg5 )
      << "i:6;" << cxArg( arg6 ) << "i:7;" << cxArg( arg7 )
      << "i:8;" << cxArg( arg8 ) << "i:9;" << cxArg( arg9 )
          << "i:10;" << cxArg( arg10 ) << "i:11;" << cxArg( arg11 )
          << "i:12;" << cxArg( arg12 ) << "i:13;" << cxArg( arg13 )
          << "i:14;" << cxArg( arg14 ) << "i:15;" << cxArg( arg15 )
          << "i:16;" << cxArg( arg16 ) << "i:17;" << cxArg( arg17 )
          << "i:18;" << cxArg( arg18 ) << "i:19;" << cxArg( arg19 )
      << "i:20;" << cxArg( arg20 ) << "i:21;" << cxArg( arg21 )
      << "i:22;" << cxArg( arg22 ) << "i:23;" << cxArg( arg23 )
      << "i:24;" << cxArg( arg24 ) << "i:25;" << cxArg( arg25 )
      << "}";

    return s.str();
}


#endif // CONFIGURATORRPC__H

