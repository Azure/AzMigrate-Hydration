//
// Implements toString() function for SVERROR. Returns string rather than char*
//
#include <cassert>
#include <string>
#include <boost/lexical_cast.hpp>
#include "svtypes.h"
#include "hostagenthelpers_ported.h"    // ARRAYSIZE()

#undef SVSTATUS__H                      // Need to re-include this file (first include from svtypes.h)
#define SVSTATUS_GET_STRINGS
#include "svstatus.h"
static char const* s_rgSVStatusList[] = { SVSTATUS_LIST };
#undef SVSTATUS_GET_STRINGS

std::string toString( SVERROR const& e ) {
    std::string result;
    char const* pszShortName = "Unknown";

    switch( e.tag ) {
    default: assert( !"Unknown type of SVERROR" ); break;
    case SVERROR::SVERROR_SVSTATUS:
        if( e.error.SVStatus >= 0 && e.error.SVStatus < ARRAYSIZE( s_rgSVStatusList ) )
        {
            pszShortName = s_rgSVStatusList[ e.error.SVStatus ];
        }
        result = std::string("err ") + pszShortName;
        break;

    case SVERROR::SVERROR_ERRNO:
        result = std::string( "errno " ) + boost::lexical_cast<std::string>(e.error.unixErrno);
        break;

    case SVERROR::SVERROR_NTSTATUS:
        result = std::string( "status " ) + boost::lexical_cast<std::string>(e.error.ntstatus);
        break;

    case SVERROR::SVERROR_HRESULT:
        result = std::string( "hr " ) + boost::lexical_cast<std::string>( e.error.hr );
        break;
    }

    return result;
}
