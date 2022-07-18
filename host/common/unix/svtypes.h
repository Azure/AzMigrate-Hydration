/// @file svtypes.h
/// Defines all the types used by SV software.
///
#ifndef SVTYPES__H
#define SVTYPES__H

#include <climits>
#include <errno.h>
#include "svstatus.h"

typedef unsigned char SV_UCHAR;
typedef SV_UCHAR	*SV_PUCHAR;
typedef unsigned short SV_USHORT;
typedef SV_USHORT	*SV_PUSHORT;
typedef int SV_INT;
typedef unsigned int SV_UINT;
typedef long SV_LONG;
typedef unsigned long SV_ULONG;
typedef long long SV_LONGLONG;
typedef unsigned long long SV_ULONGLONG;
typedef wchar_t SV_WCHAR;

#define LLSPEC "%lld"
#define ULLSPEC "%llu"

#ifndef MAX_PATH
#define MAX_PATH          260
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif


typedef SV_USHORT   SV_EVENT_TYPE;
typedef SV_LONGLONG SV_OFFSET_TYPE;

#define SV_UINT_MAX UINT_MAX

typedef union _SV_LARGE_INTEGER {
  struct {
    SV_ULONG LowPart;
    SV_LONG HighPart;
  } u;
  SV_LONGLONG QuadPart;
} SV_LARGE_INTEGER;


typedef union _SV_ULARGE_INTEGER {
    struct {
        SV_UINT LowPart;
        SV_UINT HighPart;
    } u;
    SV_ULONGLONG QuadPart;
} SV_ULARGE_INTEGER;


typedef struct _SV_TIME {

	SV_USHORT wYear;
	SV_USHORT wMonth;
	SV_USHORT wDay;
	SV_USHORT wHour;
	SV_USHORT wMinute;
	SV_USHORT wSecond;
	SV_USHORT wMilliseconds;
	SV_USHORT wMicroseconds;
	SV_USHORT wHundrecNanoseconds;
} SV_TIME;

typedef struct tagGUID {
  SV_UINT   Data1;
  SV_USHORT  Data2;
  SV_USHORT  Data3;
  SV_UCHAR   Data4[8];
} SV_GUID;

///
/// Discriminated union of error information. Avoiding dynamic dispatch because of the overhead.
///
struct SVERROR
{
    enum SVERROR_TYPE { SVERROR_SVSTATUS, SVERROR_ERRNO, SVERROR_NTSTATUS, SVERROR_HRESULT };
    SVERROR_TYPE tag;
    union tagError {
        SVSTATUS SVStatus;
        SV_INT unixErrno;      // not named `errno' due to macro
        SV_ULONG ntstatus;
        SV_LONG hr;
    } error;

    SVERROR() : tag( SVERROR_SVSTATUS ) { error.SVStatus = SVS_OK; }
    SVERROR( SVSTATUS theError ) : tag( SVERROR_SVSTATUS ) { error.SVStatus = theError; }
    SVERROR( SV_LONG hr ) : tag( SVERROR_HRESULT ) { error.hr = hr; }
    SVERROR( SV_ULONG dwError ) : tag( SVERROR_NTSTATUS ) { error.ntstatus = dwError; }
    SVERROR( SV_INT unixErrno ) : tag( SVERROR_ERRNO ) { error.unixErrno = unixErrno; }

    ~SVERROR() {}

    bool failed()
    {
        if( SVERROR_SVSTATUS == tag )
        {
            return( !( SVS_OK == error.SVStatus || SVS_FALSE == error.SVStatus ) );
        }
        else if( SVERROR_ERRNO == tag )
        {
            return( 0 != error.unixErrno );
        }
        else if( SVERROR_NTSTATUS == tag )
        {
            return( 0 != error.ntstatus );
        }
        return( error.hr < 0 );
    }
    bool succeeded()        { return( !failed() ); }

    SV_LONG const&  operator=( SV_LONG hr )         { tag = SVERROR_HRESULT;   return( error.hr = hr ); }
    SV_ULONG const& operator=( SV_ULONG dwError )   { tag = SVERROR_NTSTATUS;  return( error.ntstatus = dwError ); }
    SV_INT const&  operator=( SV_INT unixErrno )    { tag = SVERROR_ERRNO;     return( error.unixErrno = unixErrno ); }
    SVSTATUS const&  operator=( SVSTATUS SVStatus )  { tag = SVERROR_SVSTATUS; return( error.SVStatus = SVStatus ); }

    bool operator==( const SVERROR& svhr )
    {
        if( SVERROR_SVSTATUS == tag )
        {
            return( error.SVStatus == svhr.error.SVStatus );
        }
        else if( SVERROR_ERRNO == tag )
        {
            return( error.unixErrno == svhr.error.unixErrno);
        }
        else if( SVERROR_NTSTATUS == tag )
        {
            return( error.ntstatus == svhr.error.ntstatus);
        }
        return( error.hr == svhr.error.hr );
    }

	bool operator!=( const SVERROR& svhr )
    {
		return !(operator==(svhr));
	}

    char const* toString(char * pszError = NULL, const SV_UINT errorLen = 0);
};

#endif // SVTYPES__H
