//
// Windows support for fileconfigurator. Uses registry key SV Systems\VxAgent\ConfigPathname
//
#include "../fileconfigurator.h"
#include <string>
#include <atlbase.h>
#include "globs.h"
#include "inmageex.h"

std::string FileConfigurator::getConfigPathname() {
    CRegKey reg;
    LONG rc = reg.Open( HKEY_LOCAL_MACHINE, SV_VXAGENT_VALUE_NAME, KEY_READ );
    if( ERROR_SUCCESS != rc ) {
        throw INMAGE_EX( "Couldn't open vxagent registry key" )( rc )( SV_VXAGENT_VALUE_NAME );
    }

    char szPathname[ MAX_PATH ] = "";
    ULONG cch = sizeof( szPathname );
    rc = reg.QueryStringValue( SV_CONFIG_PATHNAME_VALUE_NAME, szPathname, &cch );
    if( ERROR_SUCCESS != rc ) {
        throw INMAGE_EX( "Couldn't read config pathname from registry" )( rc )( SV_VXAGENT_VALUE_NAME )( SV_CONFIG_PATHNAME_VALUE_NAME );
    }
    return std::string( szPathname );
}

