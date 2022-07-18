//
// Implements toString() function for SVERROR
//
#include <assert.h>
#include <stdlib.h>
#include "svtypes.h"
#include "svtypesstub.h"
#include "hostagenthelpers_ported.h"    // GetThreadsafeErrorStringBuffer(), ARRAYSIZE()

#undef SVSTATUS__H                      // Need to re-include this file (first include from svtypes.h)
#define SVSTATUS_GET_STRINGS
#include "svstatus.h"
static char const* s_rgSVStatusList[] = { SVSTATUS_LIST };
#undef SVSTATUS_GET_STRINGS


char const* SVERROR::toString(char * pszError, const SV_UINT errorLen)
{
    char const* pszShortName = "Unknown";
    SV_UINT errlen = errorLen;
	if( pszError == NULL )
	{
		pszError  = GetThreadsafeErrorStringBuffer() ;
        errlen = THREAD_SAFE_ERROR_STRING_BUFFER_LENGTH;
	}
    switch( tag ) {
    default: assert( !"Unknown type of SVERROR" ); break;
    case SVERROR_SVSTATUS:
        if( error.SVStatus >= 0 && error.SVStatus < ARRAYSIZE( s_rgSVStatusList ) )
        {
            pszShortName = s_rgSVStatusList[ error.SVStatus ];
        }
        inm_sprintf_s(pszError, errlen, "err %s", pszShortName);
        break;

    case SVERROR_ERRNO:
        inm_sprintf_s(pszError, errlen, "errno %d", error.unixErrno);
        break;

    case SVERROR_NTSTATUS:
        inm_sprintf_s(pszError, errlen, "status %d", error.ntstatus);
        break;

    case SVERROR_HRESULT:
        inm_sprintf_s(pszError, errlen, "hr %08X", error.hr);
        break;
    }

    return( pszError );
}
