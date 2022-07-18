//
// rpc.h: provides basic remote procedure call from VX->CX and vice versa
//
#ifndef CONFIGURATORXML__H
#define CONFIGURATORXML__H

#include "InmFunctionInfo.h"

template <typename T1>
ParameterGroup marshalXmlCall( const T1 &arg1 )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    
    return pg;
}

template <typename T1, typename T2>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2 )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    
    return pg;
}

template <typename T1, typename T2, typename T3>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3 )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4 )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12  )
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    
    return pg;
}
template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    
    return pg;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    
    return pg;
}
template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20, 
          typename T21>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20, 
    const T21 &arg21)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    insertintoreqresp(pg, cxArg ( arg21 ), "21");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
		  typename T11, typename T12, typename T13, typename T14, typename T15,
		  typename T16, typename T17, typename T18, typename T19, typename T20, 
          typename T21, typename T22>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
	const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20, 
    const T21 &arg21, const T22 &arg22)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    insertintoreqresp(pg, cxArg ( arg21 ), "21");
    insertintoreqresp(pg, cxArg ( arg22 ), "22");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    insertintoreqresp(pg, cxArg ( arg21 ), "21");
    insertintoreqresp(pg, cxArg ( arg22 ), "22");
    insertintoreqresp(pg, cxArg ( arg23 ), "23");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23, typename T24>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23, const T24 &arg24)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    insertintoreqresp(pg, cxArg ( arg21 ), "21");
    insertintoreqresp(pg, cxArg ( arg22 ), "22");
    insertintoreqresp(pg, cxArg ( arg23 ), "23");
    insertintoreqresp(pg, cxArg ( arg24 ), "24");
    
    return pg;
}


template <typename T1, typename T2, typename T3, typename T4, typename T5,
          typename T6, typename T7, typename T8, typename T9, typename T10,
                  typename T11, typename T12, typename T13, typename T14, typename T15,
                  typename T16, typename T17, typename T18, typename T19, typename T20,
          typename T21, typename T22, typename T23, typename T24, typename T25>
ParameterGroup marshalXmlCall( const T1 &arg1, const T2 &arg2, const T3 &arg3,
    const T4 & arg4, const T5 &arg5, const T6 &arg6, const T7 &arg7, const T8 &arg8, const T9 &arg9, const T10 &arg10, const T11 &arg11, const T12 &arg12,
        const T13 &arg13, const T14 &arg14, const T15 &arg15, const T16 &arg16, const T17 &arg17, const T18 &arg18, const T19 &arg19, const T20 &arg20,
    const T21 &arg21, const T22 &arg22, const T23 &arg23, const T24 &arg24, const T25 &arg25)
{
    ParameterGroup pg;
    
    insertintoreqresp(pg, cxArg ( arg1 ), "1");
    insertintoreqresp(pg, cxArg ( arg2 ), "2");
    insertintoreqresp(pg, cxArg ( arg3 ), "3");
    insertintoreqresp(pg, cxArg ( arg4 ), "4");
    insertintoreqresp(pg, cxArg ( arg5 ), "5");
    insertintoreqresp(pg, cxArg ( arg6 ), "6");
    insertintoreqresp(pg, cxArg ( arg7 ), "7");
    insertintoreqresp(pg, cxArg ( arg8 ), "8");
    insertintoreqresp(pg, cxArg ( arg9 ), "9");
    insertintoreqresp(pg, cxArg ( arg10 ), "10");
    insertintoreqresp(pg, cxArg ( arg11 ), "11");
    insertintoreqresp(pg, cxArg ( arg12 ), "12");
    insertintoreqresp(pg, cxArg ( arg13 ), "13");
    insertintoreqresp(pg, cxArg ( arg14 ), "14");
    insertintoreqresp(pg, cxArg ( arg15 ), "15");
    insertintoreqresp(pg, cxArg ( arg16 ), "16");
    insertintoreqresp(pg, cxArg ( arg17 ), "17");
    insertintoreqresp(pg, cxArg ( arg18 ), "18");
    insertintoreqresp(pg, cxArg ( arg19 ), "19");
    insertintoreqresp(pg, cxArg ( arg20 ), "20");
    insertintoreqresp(pg, cxArg ( arg21 ), "21");
    insertintoreqresp(pg, cxArg ( arg22 ), "22");
    insertintoreqresp(pg, cxArg ( arg23 ), "23");
    insertintoreqresp(pg, cxArg ( arg24 ), "24");
    insertintoreqresp(pg, cxArg ( arg25 ), "25");
    
    return pg;
}


#endif // CONFIGURATORXML__H

