#include <windows.h>
#include <winioctl.h>

#include "hostagenthelpers.h"
#include "portablehelpersmajor.h"
#include "autoFS.h"
#include "globs.h"
#include "devicefilter.h"
#include <winioctl.h>

#include "VsnapUser.h"
#include "VVDevControl.h"

#include "inmsafecapis.h"

using namespace std;

const char *HiddenDrivesSemaphoreName = "InmageHiddenDrivesSemaphore";

HRESULT GetVisibleReadOnlyDrivesFromSVServer(char const *pszSVServerName,
                                             SV_INT HttpPort,
                                             char const *pszGetVisibleReadOnlyDrivesURL,
                                             char const *pszHostID,
                                             DWORD *pdwVisibleReadOnlyDrives )
{
    HRESULT hr = S_OK;
    char *pszGetBuffer = NULL;
    char *pszGetURL = NULL;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetVisibleReadOnlyDrivesFromSVServer()...\n" );

		const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];
        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVisibleReadOnlyDrivesFromSVServer()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pszGetURL, PSZGETURL_SIZE, pszGetVisibleReadOnlyDrivesURL );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, "?id=" );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, pszHostID );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING GetFromSVServer()...\n" );
        if( GetFromSVServer( pszSVServerName,
            HttpPort,
            pszGetURL,
            &pszGetBuffer ).failed() )
        {
            hr = E_FAIL;
        }

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetFromSVServer()... SVServerName: %s; URL: %s; hr = %08X\n", pszSVServerName, pszGetURL, hr );
            break;
        }

        *pdwVisibleReadOnlyDrives = ( DWORD ) atoi( pszGetBuffer );
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "Unhide Drives: %08X; URL: %s; hr = %08X\n", *pdwVisibleReadOnlyDrives, pszGetURL, hr );
    }
    while( FALSE );

    delete[] pszGetBuffer;
    delete[] pszGetURL;

    return( hr );
}

HRESULT GetVisibleReadWriteDrivesFromSVServer(char const * pszSVServerName,
                                              SV_INT HttpPort,
                                              char const * pszGetVisibleReadWriteDrivesURL,
                                              char const * pszHostID,
                                              DWORD *pdwVisibleReadWriteDrives )
{
    HRESULT hr = S_OK;
    char *pszGetBuffer = NULL;
    char *pszGetURL = NULL;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetVisibleReadWriteDrivesFromSVServer()...\n" );

		const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];
        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVisibleReadWriteDrivesFromSVServer()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pszGetURL, PSZGETURL_SIZE, pszGetVisibleReadWriteDrivesURL );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, "?id=" );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, pszHostID );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING GetFromSVServer()...\n" );
        if( GetFromSVServer( pszSVServerName,
            HttpPort,
            pszGetURL,
            &pszGetBuffer ).failed() )
        {
            hr = E_FAIL;
        }

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetFromSVServer()... SVServerName: %s; URL: %s; hr = %08X\n", pszSVServerName, pszGetURL, hr );
            break;
        }

        *pdwVisibleReadWriteDrives = ( DWORD ) atoi( pszGetBuffer );
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "Unhide Drives: %08X; URL: %s; hr = %08X\n", *pdwVisibleReadWriteDrives, pszGetURL, hr );
    }
    while( FALSE );

    delete[] pszGetBuffer;
    delete[] pszGetURL;

    return( hr );
}

///
/// clears bits for volumes that are no longer targets
/// 
HRESULT ClearVisibleRegistrySettingsForNonTargets( const char *pszHostAgentRegKey, DWORD targetDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;    
    DWORD dwResult = 0;

    do
    {        
        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVisibleReadOnlyDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Get the visible read only Drives in the registry
        //
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... key: %s; hr = %08X\n", pszHostAgentRegKey, hr );
            break;
        }

        // read only 
        DWORD visible;
        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READONLY_DRIVES, visible);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.GetValue() %s... hr = %08X\n",SV_VISIBLE_READONLY_DRIVES, hr );
            break;
        }           
        visible &= targetDrives;
        dwResult = cregkey.SetDWORDValue( SV_VISIBLE_READONLY_DRIVES, visible);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetDWORDValue() %s... hr = %08X\n",SV_VISIBLE_READONLY_DRIVES, hr );
            break;
        }           

        // read write        
        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READWRITE_DRIVES, visible);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.GetValue() %s... hr = %08X\n",SV_VISIBLE_READWRITE_DRIVES, hr );
            break;
        }  

        visible &= targetDrives;
        dwResult = cregkey.SetDWORDValue( SV_VISIBLE_READWRITE_DRIVES, visible);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetDWORDValue() %s... hr = %08X\n",SV_VISIBLE_READWRITE_DRIVES, hr );
            break;
        } 

    }
    while( FALSE );

    cregkey.Close();

    return( hr );
}


///
/// Get the current set of visible read only drives from the registry.
///
HRESULT GetVisibleReadOnlyDrivesList( const char *pszHostAgentRegKey, DWORD *dwVisibleReadOnlyDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szUnhideDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetVisibleReadOnlyDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVisibleReadOnlyDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Get the visible read only Drives in the registry
        //
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr,
                "FAILED cregkey.Open()... key: %s; hr = %08X\n",
                pszHostAgentRegKey,
                hr );
            break;
        }

		inm_sprintf_s(szUnhideDrives, ARRAYSIZE(szUnhideDrives), "%Lu", dwVisibleReadOnlyDrives);
        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READONLY_DRIVES, *dwVisibleReadOnlyDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.GetValue()... hr = %08X\n", hr );
            break;
        }           
    }
    while( FALSE );

    cregkey.Close();

    return( hr );
}

///
/// Get the current set of visible read write drives from the registry.
///
HRESULT GetVisibleReadWriteDrivesList( const char *pszHostAgentRegKey, DWORD *dwVisibleReadWriteDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szUnhideDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetVisibleReadWriteDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVisibleReadWriteDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Get the visible read write Drives in the registry
        //
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr,
                "FAILED cregkey.Open()... key: %s; hr = %08X\n",
                pszHostAgentRegKey,
                hr );
            break;
        }

		inm_sprintf_s(szUnhideDrives, ARRAYSIZE(szUnhideDrives), "%Lu", dwVisibleReadWriteDrives);
        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READWRITE_DRIVES, *dwVisibleReadWriteDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.GetValue()... hr = %08X\n", hr );
            break;
        }           
    }
    while( FALSE );

    cregkey.Close();

    return( hr );
}

///
/// Persist the current set of Visible Read Only drives to the registry.
///
HRESULT UpdateVisibleReadOnlyDrivesList( const char *pszHostAgentRegKey, DWORD dwVisibleReadOnlyDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szUnhideDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED UpdateVisibleReadOnlyDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED UpdateVisibleReadOnlyDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Update the Unhide Drives in the registry
        //
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr,
                "FAILED cregkey.Open()... key: %s; hr = %08X\n",
                pszHostAgentRegKey,
                hr );
            break;
        }

		inm_sprintf_s(szUnhideDrives, ARRAYSIZE(szUnhideDrives), "%Lu", dwVisibleReadOnlyDrives);
        dwResult = cregkey.SetDWORDValue( SV_VISIBLE_READONLY_DRIVES, dwVisibleReadOnlyDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }           
    }
    while( FALSE );

    cregkey.Close();

    return( hr );
}

///
/// Persist the current set of Visible Read Write drives to the registry.
///
HRESULT UpdateVisibleReadWriteDrivesList( const char *pszHostAgentRegKey, DWORD dwVisibleReadWriteDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szUnhideDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED UpdateVisibleReadWriteDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED UpdateVisibleReadWriteDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Update the visible read write Drives in the registry
        //
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr,
                "FAILED cregkey.Open()... key: %s; hr = %08X\n",
                pszHostAgentRegKey,
                hr );
            break;
        }

		inm_sprintf_s(szUnhideDrives, ARRAYSIZE(szUnhideDrives), "%Lu", dwVisibleReadWriteDrives);
        dwResult = cregkey.SetDWORDValue( SV_VISIBLE_READWRITE_DRIVES, dwVisibleReadWriteDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }           
    }
    while( FALSE );

    cregkey.Close();

    return( hr );
}

SVERROR HideDrives(DWORD dwhideDrives, DWORD & dwSuccessDrives, DWORD & dwFailedDrives)
{
    SVERROR sve = SVS_OK;
    char ch[3];
    memset(ch,0,3);
    ch[1] = ':';
    ch[2] = '\0';
    int i=0;
    dwSuccessDrives = 0;
    dwFailedDrives  = 0;
    for( ; i < 26; i++ )
    {
        ch[0] = 'A';
        if( ( ( 1 << i ) & dwhideDrives ) )
        {
            ch[0] += i; 
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "[INFO]  Got hide request for drive() %s\n", ch );
            sve = HideDrive(ch,ch);
            if (sve.succeeded())
            {	
                dwSuccessDrives |= (1 << i);
            }
            else
            {
                dwFailedDrives |= (1 << i);
            }
        }
    }
    return sve;
}

HRESULT GetAutoFSInfoFromRegistry( const char *pszSVRootKey, SV_AUTO_FS_PARAMETERS *pszGetAutoFSDrivesURL )
{
    HRESULT hr = S_OK;
    char szValue[ 256 ];
    CRegKey cregkey;    
    DWORD dwCount = 0;
    DWORD dwNumDrives = 0;
    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetAutoFSDrivesURL()...\n" );

        if( ( NULL == pszSVRootKey ) ||
            ( NULL == pszGetAutoFSDrivesURL ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetAutoFSDrivesURL()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // 
        //
        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, pszSVRootKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }   

        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_GET_VISIBLE_READONLY_DRIVES_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strncpy_s( pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL, ARRAYSIZE(pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL), szValue,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';           
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_GET_VISIBLE_READONLY_DRIVES_URL, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strncpy_s( pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL, ARRAYSIZE(pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL), DEFAULT_SV_GET_VISIBLE_READONLY_DRIVES_URL,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szGetVisibleReadOnlyDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';   
        }


        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_GET_VISIBLE_READWRITE_DRIVES_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strncpy_s( pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL, ARRAYSIZE(pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL), szValue,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';           
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_GET_VISIBLE_READWRITE_DRIVES_URL, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strncpy_s( pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL, ARRAYSIZE(pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL), DEFAULT_SV_GET_VISIBLE_READWRITE_DRIVES_URL,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szGetVisibleReadWriteDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';   
        }

        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_UPDATE_SHOULD_RESYNC_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strncpy_s( pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive, ARRAYSIZE(pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive), szValue,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive[INTERNET_MAX_URL_LENGTH] = '\0';
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_UPDATE_SHOULD_RESYNC_URL, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strncpy_s( pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive, ARRAYSIZE(pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive), DEFAULT_SV_UPDATE_SHOULD_RESYNC_URL,INTERNET_MAX_URL_LENGTH );
            pszGetAutoFSDrivesURL->szUpdateShouldResyncDrive[INTERNET_MAX_URL_LENGTH] = '\0';   
        }   
        //unhide drives
        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READONLY_DRIVES, dwNumDrives );
        if( ERROR_SUCCESS == dwResult )
        {
            pszGetAutoFSDrivesURL->dwVisibleReadOnlyDrives = dwNumDrives;          
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_VISIBLE_READONLY_DRIVES, HRESULT_FROM_WIN32( GetLastError() ) );
            pszGetAutoFSDrivesURL->dwVisibleReadOnlyDrives = 0;    
        }

        dwResult = cregkey.QueryDWORDValue( SV_VISIBLE_READWRITE_DRIVES, dwNumDrives );
        if( ERROR_SUCCESS == dwResult )
        {
            pszGetAutoFSDrivesURL->dwVisibleReadWriteDrives = dwNumDrives;          
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_VISIBLE_READWRITE_DRIVES, HRESULT_FROM_WIN32( GetLastError() ) );
            pszGetAutoFSDrivesURL->dwVisibleReadWriteDrives = 0;    
        }


    }
    while( FALSE );

    cregkey.Close();

    return( hr );


}

//
// A function to get the target rollback volumes and the associated retention logs per URL.
//

HRESULT GetTargetRollbackVolumes( const char *pszSVServerName,
                                 SV_INT HttpPort,
                                 const char *pszGetTargetRollbackVolumesURL,
                                 const char *pszHostID,
                                 char **pszGetBuff )
{
    HRESULT hr = S_OK;
    char *pszGetURL = NULL;
    char *pszGetBuffer = NULL;

    do
    {
        //__asm int 3;
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetTargetRollbackVolumes()...\n" );
		
		const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];
        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetOperandDrivesFromSVServerInBuffer()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pszGetURL, PSZGETURL_SIZE, pszGetTargetRollbackVolumesURL );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, "?id=" );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, pszHostID );


        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING GetFromSVServer()...\n" );
        DebugPrintf("pszSVServerName = %s \n",pszSVServerName);
        DebugPrintf("HttpPort = %d \n",HttpPort);
        DebugPrintf("pszGetURL = %s \n",pszGetURL);

        if( GetFromSVServer( pszSVServerName, HttpPort, pszGetURL, &pszGetBuffer ).failed() )
        {
            hr = E_FAIL;
        }
        DebugPrintf("pszGetBuffer = %s \n",pszGetBuffer);
        DebugPrintf( "RETURNED FROM GetFromSVServer()...\n" );

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetFromSVServer()... SVServerName: %s; URL: %s; hr = %08X\n", pszSVServerName, pszGetURL, hr );
            break;
        }  

        *pszGetBuff = pszGetBuffer ;
    }while( FALSE );

    delete[] pszGetURL;
    return( hr );
}

bool OpenVVControlDevice(HANDLE & CtrlDevice)
{
	// PR#10815: Long Path support
    CtrlDevice = SVCreateFile ( VV_CONTROL_DOS_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        NULL, NULL);

    if(INVALID_HANDLE_VALUE == CtrlDevice)
        return false;
    else
        return true;
}


void LockSemaphoreObject(const char *SemName, HANDLE & SemObject)
{
    SemObject = CreateSemaphore(NULL, 1, 1, SemName);
    if(!SemObject)
    {
        SemObject = INVALID_HANDLE_VALUE;
        return;
    }

    DWORD Result;

    Result = WaitForSingleObject(SemObject, INFINITE);

    switch(Result)
    {
    case WAIT_OBJECT_0:
        break;
    case WAIT_TIMEOUT:
    case WAIT_ABANDONED:
        SemObject = INVALID_HANDLE_VALUE;
        break;
    }
}

void UnLockSemaphoreObject(HANDLE SemObject)
{
    if(INVALID_HANDLE_VALUE == SemObject)
        return;

    ReleaseSemaphore(SemObject, 1, NULL);
}
