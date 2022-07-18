
#ifdef SV_WINDOWS
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif /* _WIN32_WINNT */
#define NOMINMAX 
#include <windows.h>
#include <winioctl.h>
#include <windef.h>
#include "winsock2.h"
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#endif /* SV_WINDOWS */
#include <boost/filesystem.hpp>
#include <boost/chrono.hpp>
#include <ace/ACE.h>
#include <ace/OS.h>
#include <ace/OS_NS_fcntl.h>
#include <ace/INET_Addr.h>
#include <ace/OS_NS_unistd.h>
#include <ace/OS_NS_errno.h>
#include <ace/SOCK.h>
#include <ace/SOCK_Acceptor.h>
#include <ace/Global_Macros.h>
#include <ace/Process_Manager.h>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <io.h>                     // _findfirst etc.
#include <curl/curl.h>
#include <locale>
#include <iostream>
#include <iomanip>
//#include <ntddvol.h>
#include <strstream>
#include <curl/easy.h>
#include "hostagenthelpers.h"
#include "inm_md5.h"
#include "defines.h"
#include "autoFS.h"
#include "globs.h"
#include "devicefilter.h"

//For bugfix bug26
#include "svconfig.h"
#include "logger.h"
// For sending compatibility number
#include "compatibility.h"
#include "portablehelpers.h"
#include "portablehelpersmajor.h"
#include "version.h"
#include "host.h"
#include "inmsafecapis.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "createpaths.h"

using namespace std;
using namespace boost::chrono;

SSH_Configuration ssh_config;
/* All globals here previously, were moved to globs.h */

extern char g_szEmptyString[] = "";
char s_szConfiguredIP[ 40 ] = "";
char s_szConfiguredHostname[ BUFSIZ ] = "";
#define FILTER_NAME _T( "involflt" )
#define FILTER_EXTENSION _T( ".sys" )
#define BUFSIZE 250
#define OSINFOLENGTH 500

char const * const NTFS_FILE_SYSTEM_TYPE = "NTFS";
char const * const FAT_FILE_SYSTEM_TYPE = "FAT";

bool bEnableEncryption = false;
int RegisterHostIntervel = 0;


///
/// Returns the value for specified header field (ala WinInet's HttpQueryInfo). 
/// Caller must free memory.
///
HRESULT SVHttpQueryInfo( HINTERNET hRequest,
                        DWORD dwInfoLevel,
                        DWORD* pdwIndex,
                        LPSTR* ppszResult )
{
    HRESULT hr = S_OK;
    assert( NULL != hRequest );
    assert( 0 != dwInfoLevel );

    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( "ENTERED SVHttpQueryInfo()...\n" );

    if( NULL == ppszResult )
    {
        hr = E_INVALIDARG;
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "FAILED SVHttpQueryInfo, hr=%08X()...\n", hr );
        return( hr );
    }

    DWORD cbAllocated = 1024;
    LPSTR pszHeaderInfo = NULL;

    do
    {
        delete [] pszHeaderInfo;
        pszHeaderInfo = new char[ cbAllocated ];

        if( NULL == pszHeaderInfo )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED new(), hr=%08X()...\n", hr );
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING HttpQueryInfo() ...\n" );

        DWORD dwIndex = 0;
        if( !HttpQueryInfo( hRequest,
            dwInfoLevel,
            pszHeaderInfo,
            &cbAllocated,
            &dwIndex ) )
        {
            delete [] pszHeaderInfo;
            pszHeaderInfo = NULL;

            DWORD dwError = GetLastError();
            if( ERROR_INSUFFICIENT_BUFFER != dwError )
            {
                hr = HRESULT_FROM_WIN32( dwError );
                //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                //DebugPrintf( hr, "WARNING HttpQueryInfo((, hr=%08X()...\n", hr );
                break;
            }
        }
        else
        {
            if( NULL != pdwIndex )
            {
                *pdwIndex = dwIndex;
            }

            break;
        }
    } while( TRUE );

    *ppszResult = pszHeaderInfo;

    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( SV_LOG_DEBUG, "LEAVING SVHttpQueryInfo((, hr=%08X()...\n", hr );

    return( hr );
}

///
/// Returns the response of an HTTP request. Only needs to be called once to return the complete response.
/// dwContentLength may be zero, in which case the response is read till end of connection.
/// Caller must free the resulting buffer. pchRead may be NULL.
///
HRESULT SVInternetReadFile( HINTERNET hFile,
                           char** ppszResponse,
                           DWORD dwContentLength,
                           LPDWORD pcbRead )
{
    HRESULT hr = S_OK;

    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( "ENTERING SVInternetReadFile()...\n" );

    assert( NULL != hFile );
    if( NULL == ppszResponse )
    {
        hr = E_INVALIDARG;
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "FAILED SVInternetReadFile()..., hr = %08X\n", hr );
        return( hr );
    }

    int cbRemaining = dwContentLength;
    DWORD cbBuffer = 8*1024;
    DWORD cbTotalRead = 0;
    LPSTR pszBuffer = new char[ cbBuffer+1 ];

    if( NULL == pszBuffer )
    {
        hr = E_OUTOFMEMORY;
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "FAILED SVInternetReadFile(), new..., hr = %08X\n", hr );
        return( hr );
    }

    do
    {
        DWORD dwToRead = (0 == dwContentLength) ? cbBuffer - cbTotalRead : cbRemaining;
        DWORD dwRead = 0;

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING InternetReadFile()...\n" );

        if( !InternetReadFile( hFile, pszBuffer + cbTotalRead, dwToRead, &dwRead ) )
        {
            DWORD dwError = GetLastError();
            if( ERROR_INSUFFICIENT_BUFFER == dwError )
            {
                DWORD cbNewBuffer = cbBuffer * 2;
                char* pszNewBuffer = new char[ cbNewBuffer+1 ];
                if( NULL == pszNewBuffer )
                {
                    hr = E_OUTOFMEMORY;
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( "FAILED SVInternetReadFile, new()...\n" );
                    break;
                }

                inm_memcpy_s( pszNewBuffer, cbNewBuffer+1, pszBuffer, cbBuffer );
                delete [] pszBuffer;
                pszBuffer = pszNewBuffer;
                cbBuffer = cbNewBuffer;
            }
            else
            {
                hr = HRESULT_FROM_WIN32( dwError );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED InternetReadFile(), new..., hr = %08X\n", hr );
                break;
            }
        }
        else
        {
            cbTotalRead += dwRead;
            if( 0 != dwContentLength )
            {
                cbRemaining -= dwRead;
            }

            if( 0 == dwRead )
            {
                pszBuffer[ cbTotalRead ] = '\0';
                break;
            }

            //
            // If still bytes to read, but didn't get ERROR_INSUFFICIENT_BUFFER, keep reading
            //
            if( cbTotalRead == cbBuffer )
            {
                DWORD cbNewBuffer = cbBuffer * 2;
                char* pszNewBuffer = new char[ cbNewBuffer+1 ];
                if( NULL == pszNewBuffer )
                {
                    hr = E_OUTOFMEMORY;
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED SVInternetReadFile(), new..., hr = %08X\n", hr );
                    break;
                }

                inm_memcpy_s( pszNewBuffer, cbNewBuffer+1, pszBuffer, cbBuffer );
                delete [] pszBuffer;
                pszBuffer = pszNewBuffer;
                cbBuffer = cbNewBuffer;
            }
        }
    } while( TRUE );

    *ppszResponse = pszBuffer;
    if( NULL != pcbRead )
    {
        *pcbRead = cbTotalRead;
    }
    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( SV_LOG_DEBUG, "LEAVING SVInternetReadFile()..., hr = %08X\n", hr );

    return( hr );
}

void getInstallVolumeParameters(string installPath, SV_ULONGLONG* insVolCapacity, SV_ULONGLONG* insVolFreeSpace)
{
    // Install Directory Capacity & FreeSpace
    // From InstallPath extract the volume letter.
        char installVolume[5];
        memset( installVolume, 0, 5);
        installVolume[0] = installPath[0];
        inm_strcat_s(installVolume, ARRAYSIZE(installVolume), ":\\");
        
        ULARGE_INTEGER uliQuota = {0};
        ULARGE_INTEGER uliTotalCapacity = {0};
        ULARGE_INTEGER uliFreeSpace = {0};

        // PR#10815: Long Path support        
        if( !SVGetDiskFreeSpaceEx( installVolume, &uliQuota, &uliTotalCapacity, &uliFreeSpace ) )
        {
            if(insVolCapacity)
                *insVolCapacity  = 0;
            if(insVolFreeSpace)
                *insVolFreeSpace = 0;        
                DebugPrintf(SV_LOG_WARNING, "FAILED: Unable to get free space on Install Volume %s\n", installVolume);
        }

        if(insVolFreeSpace)
                *insVolFreeSpace = (unsigned long long)uliFreeSpace.QuadPart;
        if(insVolCapacity)
                *insVolCapacity = (unsigned long long)uliTotalCapacity.QuadPart;
}

///
/// Performs an HTTP GET from the SV Server. Caller must free memory.
/// 
SVERROR GetFromSVServer( const char *pszSVServerName,
                        SV_INT HttpPort,
                        const char *pszGetURL,
                        char **ppszGetBuffer )
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    SVERROR rc;
    HRESULT hr = S_OK;
    DWORD dwQueryResponseLength = 0;
    char *pszQueryResponse = NULL;
    HINTERNET hinternetSession = NULL;
    HINTERNET hinternet = NULL;
    HINTERNET hinternetRequest = NULL;
    char const* SV_ROOT_KEY = "SOFTWARE\\SV Systems";
    SV_USHORT usiServerHttpPort = INTERNET_DEFAULT_HTTP_PORT;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetFromSVServer()...\n" );

        if( ( NULL == pszSVServerName ) ||
            ( NULL == pszGetURL ) ||
            ( NULL == ppszGetBuffer ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetFromSVServer()... hr = %08X\n", hr );
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING InternetOpen()...\n" );

        hinternetSession = InternetOpen( "SVHostAgent",
            0, //INTERNET_OPEN_TYPE_DIRECT,
            NULL,
            NULL,
            INTERNET_FLAG_DONT_CACHE );
        if( NULL == hinternetSession )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED InternetOpen()... hr = %08X\n", hr );
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING InternetConnect()... SVServerName: %s\n", pszSVServerName );

        hinternet = InternetConnect( hinternetSession,
            pszSVServerName,
            HttpPort,
            "",
            NULL,
            INTERNET_SERVICE_HTTP,
            INTERNET_FLAG_DONT_CACHE,
            0 );
        if( NULL == hinternet )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED InternetConnect()... hr = %08X\n", hr );
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING HttpOpenRequest()... URL: %s\n", pszGetURL );

        hinternetRequest = HttpOpenRequest( hinternet,
            "GET",
            pszGetURL,
            NULL, // Defaults to HTTP 1.0
            NULL,
            NULL, // Accepts only text/*"
            INTERNET_FLAG_DONT_CACHE, //BUGBUG: No SSL
            0 );

        if( NULL == hinternetRequest )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED InternetConnect()... hr = %08X\n", hr );
            printf( "HttpOpenRequest Failed: hr: %x\n", hr );
            break;
        }

        DWORD dwTimeout = 30 * 1000;
        if(!InternetSetOption(hinternetRequest, INTERNET_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout))) {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "CALLING Failed Setting Timeout()...\n" );
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING HttpSendRequest()...\n" );

        if( !HttpSendRequest( hinternetRequest,
            NULL,
            0,
            NULL,
            0 ) )
        {
            DWORD dwError = GetLastError();
            hr = HRESULT_FROM_WIN32( dwError );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED HttpSendRequest()... hr = %08X\n", hr );
            break;
        }

        DWORD dwIndex = 0;
        DWORD dwError = 0;

        hr = SVHttpQueryInfo( hinternetRequest,
            HTTP_QUERY_STATUS_CODE,
            NULL,
            &pszQueryResponse );

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SVHttpQueryInfo() for HTTP status... hr = %08X\n", hr );
            break;
        }       

        if( HTTP_STATUS_OK != atoi( pszQueryResponse ) )
        {
            hr = E_FAIL;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED HTTP_STATUS: %d; hr = %08X\n", atoi( pszQueryResponse ), hr );
            break;
        }
        delete [] pszQueryResponse;
        pszQueryResponse = NULL;

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING HttpQueryInfo()...\n" );

        //
        // Use Content-Length if present; otherwise, we read till end of connection
        //
        hr = SVHttpQueryInfo( hinternetRequest,
            HTTP_QUERY_CONTENT_LENGTH,
            NULL,
            &pszQueryResponse );

        DWORD dwContentLength = 0;      
        if( SUCCEEDED( hr ) )
        {
            dwContentLength = atoi( pszQueryResponse );
        }
        delete [] pszQueryResponse;
        pszQueryResponse = NULL;


        DWORD dwBytesRead = 0;
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );

        hr = SVInternetReadFile( hinternetRequest, ppszGetBuffer, dwContentLength, &dwBytesRead );

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SVInternetReadFile()... hr = %08X\n", hr );
            break;
        }
    }
    while( FALSE );

    if( ( NULL != hinternetRequest ) && !InternetCloseHandle( hinternetRequest ) )
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( HRESULT_FROM_WIN32( GetLastError() ), "FAILED InternetCloseHandle()...\n" );
    }
    if( ( NULL != hinternet ) && !InternetCloseHandle( hinternet ) )
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( HRESULT_FROM_WIN32( GetLastError() ), "FAILED InternetCloseHandle()...\n" );
    }
    if( ( NULL != hinternetSession ) && !InternetCloseHandle( hinternetSession ) )
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( HRESULT_FROM_WIN32( GetLastError() ), "FAILED InternetCloseHandle()...\n" );
    }

    rc = hr;
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return( rc );
}

SVERROR SettingStandByCxValuesInRegistry(const char *pszServerName,int value)
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szServerkey[260];
    DWORD dwResult = ERROR_SUCCESS;
    const char* SV_ROOT_KEY = "SOFTWARE\\SV Systems";
  //  dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,SV_ROOT_KEY );
    do {
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,SV_ROOT_KEY );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }
        inm_strcpy_s(szServerkey, ARRAYSIZE(szServerkey), "ServerName");
        dwResult = cregkey.SetStringValue(szServerkey,pszServerName);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf(SV_LOG_ERROR, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }
            
        dwResult = cregkey.SetDWORDValue( "ServerHttpPort", value);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED cregkey.SetValue(), hr = %08X\n", hr );
            break;
        }
    }while(false);    
      cregkey.Close();        
      DebugPrintf(SV_LOG_INFO,"Retrun value from SettingStandByCxValuesInRegistry hr = %08X\n", hr); 
      return( hr );
}





/*
Registration of FX is doesnt involve some of the operations mentioned in RegistreHost 
Function that is defined just above. 
Ideally we should be having common operations in seperate functions and Agent 
specific operations in agent specific functions or same function that bypasses unrequired
operations.
TODO: For the time being a seperate function is written that doesnt consists of operations
that are not required by FX. But this needs to be refractored.
*/
SVERROR RegisterFX( const char *pszRegKey,
                     const char *pszSVServerName,
                     SV_INT HttpPort,
                     const char *pszURL,
                     const char *installPath,
                     char *pszHostID,
                     SV_ULONG *pdwHostIDSize,bool https)     // BUGBUG: no reason for this to be out parameter
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    const std::string strInstallPath = std::string(installPath) + "\\"; //Windows Specific code: FX-Windows-4.1
    const std::string patchfile = strInstallPath + "patch.log";
    SVERROR svError = S_OK;
    CRegKey cregkey, cregkeyCacheDir, cregkeyRoot;
    char szFileSystemType[MAX_DRIVES] [ BUFSIZ ] = {0};
    char szIpAddress[ 4*4 ];
    char insVolCapacityBuf[20], insVolFreeSpaceBuf[20], compatibilityNoBuf[20];
    SV_ULONGLONG insVolCapacity = 0, insVolFreeSpace = 0;
    //
    // Gather hostname, IP address, unique id, mounted volume list and send it to SV box
    //
    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED RegisterHost()...\n" );

        if( ( NULL == pszRegKey ) ||
            ( NULL == pszSVServerName )||
            ( NULL == pszURL ) )
        {
            svError = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED RegisterHost(), err=%s\n", svError.toString() );
            break;
        }

        std::string strHostName;
        if (0 != s_szConfiguredHostname[0])
        {
            strHostName = std::string(s_szConfiguredHostname);
        }
        else
        {
            strHostName = Host::GetInstance().GetHostName();
        }

        // ip address
        std::string szIpAddress = Host::GetInstance().GetIPAddress();

        //
        // Determine what our host guid id is
        //
        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Create( HKEY_LOCAL_MACHINE, pszRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            svError = dwResult;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED cregkey.Create()... RegKey: %s, err=%s\n", pszRegKey, svError.toString() );
            break;
        }

        HRESULT hr2 = GetHostId( pszHostID, *pdwHostIDSize );
        if( FAILED( hr2 ) )
        {
            svError = hr2;
            DebugPrintf( SV_LOG_ERROR, "FAILED Couldn't get host id, err=%s\n", svError.toString());
            break;
        }


        if( ERROR_SUCCESS != dwResult )
        {
            svError = dwResult;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED cregkey.SetValue(), err=%s\n", svError.toString() );
            break;
        }
        std::string strOSInfo;    
        GetOperatingSystemInfo(strOSInfo);
        DebugPrintf( SV_LOG_DEBUG,"[HostAgentHelpers::RegisterHost()] OS Info = %s\n",
                                                            strOSInfo.c_str());

// osVal is added to identify osType 1 for windows and  2 for UNIX
char osVal[2];
#ifdef SV_WINDOWS
inm_strcpy_s(osVal, ARRAYSIZE(osVal), "1");
#else
inm_strcpy_s(osVal, ARRAYSIZE(osVal), "2");
#endif          
        // Patch History
        // Read patch.log which is present in installation directory 
        // Prepare patch_history with fields seperated by ',' & records seperated by '|'    

        int patchsize = 0;
        char *patch_history;

        long begin,end;
        ifstream myfile (patchfile.c_str());
        if(!myfile.fail())
        {
            begin = myfile.tellg();
            myfile.seekg (0, ios::end);
            end = myfile.tellg();
            myfile.close();
        }
        else
        {
            DebugPrintf(SV_LOG_WARNING, "Could not open patch file: %s", patchfile.c_str());
            patch_history = new char[1];
            patch_history[0] = '\0';
            patchsize = 0;
        }

        fstream filestrm;
        filestrm.open (patchfile.c_str(), fstream::in); // | fstream::out | fstream::app);
        if(!filestrm.fail())
        {
            int k=0;
            int deletedflag = 0;
        
            char chr = ' ';
            INM_SAFE_ARITHMETIC(patchsize = InmSafeInt<long>::Type(end) - begin, INMAGE_EX(end)(begin))

            if(patchsize)
            {
                patch_history = new char[patchsize];
                memset(patch_history, 0,patchsize);
                DebugPrintf(SV_LOG_DEBUG," Opened patch file of length %ld characters\n", patchsize);

                while((chr=filestrm.get())!= EOF)
                {
                    if( chr == '#')
                    {
                        deletedflag = 1;
                        continue;
                    }

                    if(deletedflag && chr == '\n')
                    {
                        deletedflag = 0;
                        continue;
                    }
                    
                    if(deletedflag)
                        continue;

                if (chr == '\n')
                        patch_history[k++] = '|';
                    else
                        patch_history[k++] = chr;                
                }
                patch_history[k] = '\0';
                patchsize = k;
                filestrm.close();
                DebugPrintf(SV_LOG_DEBUG," patch_history = %s\n", patch_history );
            }
            else
            {
                patch_history = new char[1];
                patch_history[0] = '\0';
                patchsize = 0;
            }
        }

        string installDir, zone, postBuffer;
        zone = getTimeZone();
        getInstallVolumeParameters(strInstallPath, &insVolCapacity, &insVolFreeSpace);
        inm_sprintf_s(insVolCapacityBuf, ARRAYSIZE(insVolCapacityBuf), "%llu", insVolCapacity);
        inm_sprintf_s(insVolFreeSpaceBuf, ARRAYSIZE(insVolFreeSpaceBuf), "%llu", insVolFreeSpace);
        inm_sprintf_s(compatibilityNoBuf, ARRAYSIZE(compatibilityNoBuf), "%lu", CurrentCompatibilityNum());

        postBuffer += "hostname=" + strHostName
            + "&ip=" + szIpAddress
            + "&id=" + string(pszHostID)
            + "&os=" + strOSInfo
            + "&av=" + string(GetProductVersion())
            + "&installpath=" + strInstallPath
            + "&compatibilityNo=" + string(compatibilityNoBuf)
            + "&osVal=" + string(osVal)
            + "&invc=" + string(insVolCapacityBuf)
            + "&infs=" + string(insVolFreeSpaceBuf)
            + "&dttm=" + getLocalTime()
            + "&patch_history=" + string(patch_history)
            + ((zone[0] == '+')? "&tmzn=%2B" : "&tmzn=-" ) ;// Following [HTTP:PostMethod] standard. 
                                                            // Replace '+' with '%2B' escape sequence 
        postBuffer += zone.substr(1);                  
        std::string prod_version = PROD_VERSION;
        postBuffer = postBuffer + "&prodVerson=" + prod_version;
        DebugPrintf(SV_LOG_DEBUG,"[HostAgentHelpers::RegisterHost()] postBuffer = %s\n",postBuffer.c_str());
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        
        svError = postToCx( pszSVServerName,
                        HttpPort,
                        pszURL,
                        (char*)postBuffer.c_str(),
                        NULL,https);

    

        if( svError.failed() )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED PostToSVServer()... SVServerName: %s; URL: %s, err=%s\n",
                pszSVServerName,
                pszURL,
                svError.toString() );
            break;
        }
    }
    while( FALSE );

    cregkey.Close();
    cregkeyRoot.Close();
    cregkeyCacheDir.Close();
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return( svError );
}


/// HostId is always located under SOFTWARE\SV Systems. One HostId per machine, not per agent.
/// We do not create this on demand, it is created by the installer.
///
HRESULT GetHostId( char* pszValue, DWORD cch )
{
    HRESULT hr = S_OK;
    DWORD dwCount = cch;

    assert( NULL != pszValue && cch >= 36 );

    CRegKey reg;

    do
    {
        if( ERROR_SUCCESS != reg.Open( HKEY_LOCAL_MACHINE, _T( "SOFTWARE\\SV Systems\\" ), KEY_READ ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }

        DWORD dwResult = reg.QueryStringValue( SV_HOST_ID_VALUE_NAME, pszValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        }
    } while( false );

    return( hr );
}


bool GetSslPaths(char const * rootKey, std::string& strSslKeyPath, std::string& strSslCertPath)
{    
    char value[256];

    CRegKey cregkey;    
    
    DWORD count = 0; 


    if (NULL == rootKey) {
        return false; 
    }

    DWORD rc = cregkey.Open(HKEY_LOCAL_MACHINE, rootKey);
    if (ERROR_SUCCESS != rc) {        
        DebugPrintf(SV_LOG_ERROR, "FAILED GetSslInfo open key %s: %d\n", rootKey, rc);
        return false;
    }   

    count = sizeof(value);
    rc = cregkey.QueryStringValue(SV_SSL_KEY_PATH, value, &count);
    if (ERROR_SUCCESS != rc) {
        DebugPrintf(SV_LOG_ERROR, "FAILED GetSslInfo query %s: %d\n", SV_SSL_KEY_PATH, rc);
        cregkey.Close();
        return false;
    }
    strSslKeyPath = std::string(value);
    
    count = sizeof(value);
    rc = cregkey.QueryStringValue(SV_SSL_CERT_PATH, value, &count);
    if (ERROR_SUCCESS != rc) {
        DebugPrintf(SV_LOG_ERROR, "FAILED GetSslInfo query %s: %d\n", SV_SSL_CERT_PATH, rc);
        cregkey.Close();
        return false;
    }
    strSslCertPath = std::string(value);

    cregkey.Close();

    return true;
}

/*
/// The following HRESULT SVFtpGetFile(char *lptstrSourceFile, char *lptstrDestDirectory)
/// function was moved to ncftptransport.cpp, and renamed to NcFtpGetFile().
//
// The following SVFtpGetFile() is for the fix of BUG#111 - to use ncftpget 
// instead of Wininet's FtpGetFile() call to get the differential files from 
// the TB box. 
// However, this function is more generic in that it does Ftpget on any file 
// specified at the source site, and copy it to the specified destination directory.
//
// The major reasons for this fix are:
// (1)ncftpget has timeout support, and 
// (2)Wininet's FtpGetfile() uses the IE's cache (IE5.Content area) 
// to cach all the _diff's files and does not do any cleanups afterward. 
// And additionally, there does not seem to be some straight forward and 
// safe ways for us to use to do the cleanup's.
// I.e., there does not seem to have some obvious programmable and/or manual 
// operations that we could use safely. Additionally, we have not been able 
// to identify all the possible side-effects associated with direct 
// manual deleting of these cached files.
// 
HRESULT SVFtpGetFile(char *lptstrSourceFile, char *lptstrDestDirectory)
*/

///
/// Ask the SV Server what drives on the host should be replicated. All other drives are
/// not currently selected for replication.
///
HRESULT GetProtectedDrivesFromSVServer(char const * pszSVServerName,
                                       SV_INT HttpPort,
                                       char const * pszGetProtectedDrivesURL,
                                       char const * pszHostID,
                                       DWORD *pdwProtectedDrives )
{
    HRESULT hr = S_OK;
    char *pszGetBuffer = NULL;
    char *pszGetURL = NULL;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetProtectedDrivesFromSVServer()...\n" );

        const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];
        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetProtectedDrivesFromSVServer()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pszGetURL, PSZGETURL_SIZE, pszGetProtectedDrivesURL );
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
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "Protected Drives as string : %s; URL: %s; hr = %08X\n", pszGetBuffer, pszGetURL, hr );
        *pdwProtectedDrives = ( DWORD ) atoi( pszGetBuffer );
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "Protected Drives: %08X; URL: %s; hr = %08X\n", *pdwProtectedDrives, pszGetURL, hr );
    }
    while( FALSE );

    delete[] pszGetBuffer;
    delete[] pszGetURL;

    return( hr );
}

bool GetProtectedVolumeSettingsFromSVServer(char const * pszSVServerName,
                                       SV_INT HttpPort,
                                       char const * pszGetProtectedVolumeSettingsURL,
                                       char const * pszHostID,
                                       char ** pszProtectedVolumeSettings)
{
    HRESULT hr = S_OK;
    //char *pszGetBuffer = NULL;
    char *pszGetURL = NULL;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetProtectedDrivesFromSVServer()...\n" );

        const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];

        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetProtectedDrivesFromSVServer()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pszGetURL, PSZGETURL_SIZE, pszGetProtectedVolumeSettingsURL );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, "?id=" );
        inm_strcat_s( pszGetURL, PSZGETURL_SIZE, pszHostID );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING GetFromSVServer()...\n" );
        if( GetFromSVServer( pszSVServerName,
            HttpPort,
            pszGetURL,
            pszProtectedVolumeSettings ).failed() )
        {
            hr = E_FAIL;
        }

        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetFromSVServer()... SVServerName: %s; URL: %s; hr = %08X\n", pszSVServerName, pszGetURL, hr );
            break;
        }
    }
    while( FALSE );

//    delete[] pszGetBuffer;
    delete[] pszGetURL;

    return( hr==S_OK?true:false );
}

///
/// Persist the current set of protected drives to the registry.
///
HRESULT UpdateProtectedDrivesList( char *pszHostAgentRegKey, DWORD dwProtectedDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szProtectedDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED UpdateProtectedDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED UpdateProtectedDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Update the Protected Drives Key in the registry
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

        inm_sprintf_s(szProtectedDrives, ARRAYSIZE(szProtectedDrives), "%Lu", dwProtectedDrives);
        dwResult = cregkey.SetDWORDValue( "ProtectedDrives", dwProtectedDrives);
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
/// Persist the current set of target drives to the registry.
///
HRESULT UpdateTargetDrivesList( char *pszHostAgentRegKey, DWORD dwTargetDrives )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szProtectedDrives[ 64 ];
    DWORD dwResult = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED UpdateProtectedDrivesList()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED UpdateProtectedDrivesList()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Update the Protected Drives Key in the registry
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

        inm_sprintf_s(szProtectedDrives, ARRAYSIZE(szProtectedDrives), "%Lu", dwTargetDrives);
        dwResult = cregkey.SetDWORDValue( "TargetDrives", dwTargetDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }           
    }
    while( false );

    cregkey.Close();

    return( hr );
}

/// 
/// Returns true if the set of protected drives (as reported by the SV Server) has changed.
/// 
BOOL ProtectedDriveMaskChanged( char *pszHostAgentRegKey, DWORD dwCurrentProtectedDriveMask )
{
    HRESULT hr = S_OK;
    BOOL bRetVal = FALSE;
    CRegKey cregkey;
    DWORD dwProtectedDriveMask = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED ProtectedDriveMaskChanged()...\n" );

        if( NULL == pszHostAgentRegKey )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED ProtectedDriveMaskChanged()... hr = %08X\n", hr );
            break;
        }

        USES_CONVERSION;
        //
        // Determine what our host guid id is
        //
        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        /*
        char szValue[ 64 ];
        DWORD dwCount = sizeof( szValue );
        dwResult = cregkey.QueryValue( szValue, "ProtectedDrives", &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
        hr = HRESULT_FROM_WIN32( dwResult );
        break;
        }

        dwProtectedDriveMask = ( DWORD ) atoi( szValue );
        */
        dwResult = cregkey.QueryDWORDValue( "ProtectedDrives", dwProtectedDriveMask);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
            break;
        }

        if( dwCurrentProtectedDriveMask != dwProtectedDriveMask )
        {
            bRetVal = TRUE;
        }
    }
    while( FALSE );

    if( FAILED( hr ) )
    {
        bRetVal = FALSE;
    }

    cregkey.Close();

    return( bRetVal );
}



///
/// Apply delta file to target volume. The delta file should contain DRTD blocks (i.e. blocks
/// with the actual payload to write). The address and lengths in the delta file should all
/// be block aligned.
///


HRESULT ProcessAndApplyDeltaFile( char const* filename,
                                 HANDLE handleVolume,
                                 SV_FileList *pCurSV_FileList) //Used for transmitting status to the SV box                                 
{
    HRESULT hr = S_OK;
    SVD_PREFIX prefix = { 0 };
    char* buffer = NULL;
    unsigned bufferSize = 0;
    FILE* file = NULL;
    BOOL bFixInvalidFile = FALSE;
    
    
    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED ProcessAndApplyDeltaFile()...\n" );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING fopen()...\n" );
        file = fopen( filename, "rb" );

        if( NULL == file )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED ProcessAndApplyDeltaFile(%s)... hr = %08X\n", filename, hr );
            break;
        }

        while( TRUE )
        {
            if( 1 != fread( &prefix, sizeof( prefix ), 1, file ) )
            {
                break;
            }

            switch( prefix.tag )
            {
            default:
                hr = E_FAIL;
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "FAILED Invalid Tag... (0x%08X) %s\n", prefix.tag, filename );
                if( 0x1000 == prefix.tag && !bFixInvalidFile )
                {
                    DebugPrintf( "WARNING Attempting fixup %s\n", filename );
                    fseek( file, 0, SEEK_SET );
                    bFixInvalidFile = TRUE;
                    hr = S_OK;
                }
                //assert( !"Invalid Tag" );
                break;

            case SVD_TAG_HEADER1:
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_TAG_HEADER1...\n" );
                if( ( prefix.count*sizeof( SVD_HEADER1 ) ) > bufferSize )
                {
                    delete [] buffer;
                    INM_SAFE_ARITHMETIC(bufferSize = prefix.count*InmSafeInt<size_t>::Type(sizeof( SVD_HEADER1 )), INMAGE_EX(prefix.count)(sizeof( SVD_HEADER1 )))
                    buffer = new char[ bufferSize];

                    if( NULL == buffer )
                    {
                        break;
                    }
                }
                if( 1 != fread( buffer, 
                    prefix.count*sizeof( SVD_HEADER1 ),
                    1,
                    file ) )
                {
                    break;
                }

                hr = OnHeader( (SVD_HEADER1*) buffer,
                    prefix.count*sizeof( SVD_HEADER1 ) );
                break;

            case SVD_TAG_TIME_STAMP_OF_FIRST_CHANGE:
                //__asm int 3 ;
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_TAG_TIME_STAMP_OF_FIRST_CHANGE...\n" );        
                if ( 0 != fseek(file, prefix.count*sizeof( SVD_TIME_STAMP ), SEEK_CUR) )
                {
                    hr = E_FAIL ;
                    break ;
                }
                break;

            case SVD_TAG_USER:
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_USER_DEFINED_TAG...\n" );
                assert(prefix.count == 1);
                if (  0 != fseek(file, prefix.Flags, SEEK_CUR) )
                {
                    hr = E_FAIL;
                    break;
                }

                break;

            case SVD_TAG_LENGTH_OF_DRTD_CHANGES:
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_TAG_LENGTH_OF_DRTD_CHANGE...\n" );
                if ( 0 != fseek(file, prefix.count*sizeof(ULARGE_INTEGER ), SEEK_CUR) )
                {
                    hr = E_FAIL ;
                    break ;
                }

                break;

            case SVD_TAG_DIRTY_BLOCK_DATA:
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_TAG_DIRTY_BLOCK_DATA...\n" );

                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "CALLING OnDirtyBlockData()...\n" );
                if( bFixInvalidFile )
                {
                    prefix.count++;
                    bFixInvalidFile = FALSE;
                }

                hr = OnDirtyBlockData( file,
                    handleVolume,
                    pCurSV_FileList, //Used for transmitting status to the SV box
                    prefix.count);                    
                break;

            case SVD_TAG_TIME_STAMP_OF_LAST_CHANGE :
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "Got SVD_TAG_TIME_STAMP_OF_LAST_CHANGE...\n" );
                if ( 0 != fseek(file, sizeof( SVD_TIME_STAMP ), SEEK_CUR ) )
                {
                    hr = E_FAIL ;
                    // break ;
                }
                break;

            }
            if( FAILED( hr ) )
            {
                break;
            }
        }
        if( FAILED( hr ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED ProcessAndApplyDeltaFile()... hr = %08X\n", hr );
            break;
        }
    }
    while( FALSE );

    if( NULL != file )
    {
        fclose( file );
    }
    delete [] buffer;

    return( hr );
}

///
/// Parser callback when encountering delta file header. Just print out debugging info.
///
HRESULT OnHeader( SVD_HEADER1 const* header, unsigned int size )
{
    HRESULT hr = S_OK;

    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( "ENTERED OnHeader()...\n" );

    //
    // We know the version, because this is SVD_HEADER1
    //
    printf( "Line %d @ %s: : Header:\n\tVersion: %d.%d\n",
        __LINE__,
        __FILE__,
        1,
        0);

    return( hr );
}



///
/// Faster version of OnDirtyBlockData().
///
HRESULT OnDirtyBlockData( FILE* file,
                         HANDLE handleVolume,
                         SV_FileList *pCurSV_FileList, // Updated to transmit status to the SV box
                         SV_ULONG sv_ulongBlockCount)                         
{
    HRESULT hr = S_OK;
    SVD_DIRTY_BLOCK svd_dirty_block = { 0 };
    BYTE* pBuffer = NULL;
    SV_LONGLONG cbRemaining = 0;
    SV_LONGLONG cbWritten = 0;
    SV_LONGLONG cbTotalWritten = 0;

    assert( NULL != file );
    assert( !IS_INVALID_HANDLE_VALUE( handleVolume ) );
    

    int const MAX_WRITE = 64*1024;  // Write this much at a time. Must be multiple of sector length.

    do
    {
        pBuffer = new BYTE[ MAX_WRITE ];
        if( NULL == pBuffer )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "FAILED Out of memory for pBuffer in OnDirtyBlockData\n" );
            hr = E_OUTOFMEMORY;
            break;
        }

        for( unsigned i = 0; i < sv_ulongBlockCount; i++ )
        {
            if( 1 != fread( &svd_dirty_block, 
                sizeof( svd_dirty_block ),
                1,
                file ) )
            {
                hr = E_FAIL;
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED fread()... hr = %08X\n", hr );
                break;
            }
            //DebugPrintf( "Block %d of %d: %u at\t%I64u\n", i, sv_ulongBlockCount, (DWORD) svd_dirty_block.Length, svd_dirty_block.ByteOffset );

            
            cbRemaining = svd_dirty_block.Length;
            cbTotalWritten = 0;

            while( cbRemaining > 0 )
            {
                DWORD cbToRead = (DWORD) std::min(cbRemaining, (SV_LONGLONG)MAX_WRITE );
                if( 1 != fread( pBuffer, cbToRead, 1, file ) )
                {
                    hr = E_FAIL;
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( "FAILED fread %d bytes...\n", cbToRead );
                    break;
                }
                LONG lowpart = (LONG) (svd_dirty_block.ByteOffset + cbTotalWritten);
                LONG highpart = (LONG) ((svd_dirty_block.ByteOffset + cbTotalWritten) >> 32);

                if( -1 == SetFilePointer( handleVolume, lowpart, &highpart, FILE_BEGIN ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED SetFilePointer()... hr = %08X\n", hr );
                    break;
                }

                DWORD cbToWrite = cbToRead;
                DWORD cbWritten = 0;
                if( !WriteFile( handleVolume, pBuffer, cbToWrite, &cbWritten, NULL ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED Write to volume %d bytes at %I64u, err %d\n", cbToWrite, svd_dirty_block.ByteOffset + cbTotalWritten, GetLastError() );
                    break;
                }
                if (0 != pCurSV_FileList) {
                    pCurSV_FileList->BytesWritten += cbWritten;

                    pCurSV_FileList->VolumePosition = std::max( pCurSV_FileList->VolumePosition,
                        svd_dirty_block.ByteOffset + cbTotalWritten );
                }
                cbTotalWritten += cbWritten;
                cbRemaining -= cbWritten;
            }
            
        }
    } while( FALSE );

    delete [] pBuffer;

    return( hr );
}


///
/// Applies the difference given the volume handle, the delta buffer,
/// its length, and offset
///
HRESULT ApplyDifference( HANDLE handleVolume, 
                        char *pchDeltaBuffer,
                        SV_LONGLONG sv_ulongLength,
                        SV_LONGLONG sv_large_integerByteOffset)                        
{
    HRESULT hr = S_OK;
    BOOL bFlag = FALSE;
    DWORD dwFlag = 0;
    SV_ULONG sv_ulongLengthWritten = 0;  //Used in WriteFile
    int count = 0;

    do
    {
        /*
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED ApplyDifference()...\n" );
        */

        if( ( INVALID_HANDLE_VALUE == handleVolume ) ||
            ( NULL == pchDeltaBuffer ) ||
            ( 0 >= sv_ulongLength ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED ApplyDifference()... hr = %08X\n", hr );
            break;
        }


        LONG lowpart = 0;
        LONG highpart = 0;

        lowpart = (LONG) sv_large_integerByteOffset;
        highpart = (LONG) (sv_large_integerByteOffset >> 32);

        //      printf( "Byte Offset: %LLu \n", sv_large_integerByteOffset );

        /*
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING SetFilePointer()...\n" );
        */
        dwFlag = SetFilePointer( handleVolume,
            lowpart,
            &( highpart ),
            FILE_BEGIN );
        //BUGBUG: -1 should actually be INVALID_SET_FILE_POINTER.  See why it doesn't work
        if( -1 == dwFlag )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SetFilePointer()... hr = %08X\n", hr );
            break;
        }

        /*
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING WriteFile()...\n" );
        */
        bFlag = WriteFile( handleVolume,
            pchDeltaBuffer,
            (LONG) sv_ulongLength,
            &sv_ulongLengthWritten,
            NULL );
        if( 0 == bFlag )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
            break;
        }
    }
    while( FALSE );

    return( hr );
}

///
/// Returns true if the buffer contains only zeros. Used to optimize away writes to target
/// volumes.
///
BOOL BufferIsZero( char *pszBuffer, SV_ULONG sv_ulongLength )
{
    BOOL bRetVal = TRUE;
    DWORD *pdwBuffer = ( DWORD * ) pszBuffer;

    do
    {
        /*
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED BufferIsZero()...\n" );
        */
        if( NULL == pszBuffer )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "FAILED BufferIsZero()... Invalid Arg\n" );
            bRetVal = FALSE;
            break;
        }

        unsigned long ulLengthInDword = sv_ulongLength/4;
        unsigned long i = 0;
        for( i = 0; i < ulLengthInDword; i++ )
        {
            if( 0 != pdwBuffer[i] )
            {
                //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                //DebugPrintf( "BUFFER IS NON ZERO; i: %ul\n", i );
                bRetVal = FALSE;
                break;
            }
        }
    }
    while( FALSE );

    return( bRetVal );
}


///
/// Goes through the linked list and deletes every element.
///
HRESULT DeleteCopyFileList( /* in */ SV_FileList *pSV_FileList )
{
    HRESULT hr = S_OK;
    SV_FileList *pTempSV_FileList = NULL;

    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
    DebugPrintf( "ENTERED DeleteCopyFileList()...\n" );

    while( NULL != pSV_FileList )
    {
        pTempSV_FileList = pSV_FileList->pNext;
        if(NULL != pSV_FileList->fileName)
            free(pSV_FileList->fileName);
        delete pSV_FileList;
        pSV_FileList = pTempSV_FileList;
    }
    return( hr );
}

///
/// Copies a block aligned segment of the volume to an open delta file handle.
///
HRESULT CopyVolumeChunkToLocalFile( HANDLE handleVolume,
                                   char *pchFilename,
                                   SV_LONGLONG sv_longlongLength,
                                   SV_LONGLONG sv_longlongByteOffset,
                                   SV_LONGLONG *psv_longlongBytesRead )
{
    HRESULT hr = S_OK;
    char *pchBuffer = NULL;
    DWORD dwBytesRead = 0;
    DWORD dwBytesWritten = 0;
    DWORD dwLength = VOLUME_READ_CHUNK_SIZE;  //BUGBUG: Remove hardcoded value
    HANDLE handleFile = INVALID_HANDLE_VALUE;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED CopyVolumeChunkToLocalFile()...\n" );

        pchBuffer = new char[ dwLength ];
        if( NULL == pchBuffer )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED CopyVolumeChunkToLocalFile()... hr = %08X\n", hr );
            break;
        }

        LONG lowpart = (LONG) sv_longlongByteOffset;
        LONG highpart = (LONG) ( sv_longlongByteOffset >> 32 );

        //BUGBUG: -1 should actually be INVALID_SET_FILE_POINTER.  See why it doesn't work
        //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        //DebugPrintf( "CALLING SetFilePointer()...\n" );
        if( -1 == SetFilePointer( handleVolume,
            lowpart,
            &( highpart ),
            FILE_BEGIN ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SetFilePointer()... hr = %08X\n", hr );
            break;
        }

        try
        {
            CreatePaths::createPathsAsNeeded(pchFilename);
        }
        catch (std::exception ex)
        {
            DebugPrintf(SV_LOG_ERROR, "%s failed: %s\n", FUNCTION_NAME, ex.what());
            hr = S_FALSE;
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING CreateFile()...\n" );
        // PR#10815: Long Path support
        handleFile = SVCreateFile( pchFilename,
            GENERIC_WRITE,
            FILE_SHARE_WRITE | FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
        if( INVALID_HANDLE_VALUE == handleFile )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED CreateFile(%s)... hr = %08X\n", pchFilename, hr );
            break;
        }

        //BUGBUG: Dump headers into the file here.

        //
        // Write SVD1 header
        //
        SVD_PREFIX prefix = { SVD_TAG_HEADER1, 1, 0 };
        if( !WriteFile( handleFile,
            &prefix,
            sizeof( prefix ),
            &dwBytesWritten, 
            NULL ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
            break;
        }
        assert( sizeof( prefix ) == dwBytesWritten );

        SVD_HEADER1 header = { 0 };
        if( !WriteFile( handleFile,
            &header,
            sizeof( header ),
            &dwBytesWritten,
            NULL ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
            break;
        }
        assert( sizeof( header ) == dwBytesWritten );

        //
        // Write single dirty block header
        //
        SVD_PREFIX drtdPrefix = { SVD_TAG_DIRTY_BLOCK_DATA, 1, 0 };
        if( !WriteFile( handleFile,
            &drtdPrefix,
            sizeof( drtdPrefix ),
            &dwBytesWritten,
            NULL ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
            break;
        }
        assert( sizeof( prefix ) == dwBytesWritten );

        //
        // Write the data for DRTD
        //
        SVD_DIRTY_BLOCK block = { sv_longlongLength, sv_longlongByteOffset };
        if( !WriteFile( handleFile,
            &block,
            sizeof( block ),
            &dwBytesWritten,
            NULL ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
            break;
        }

        *psv_longlongBytesRead = 0;
        SV_LONGLONG sv_longlongBytesRemaining = sv_longlongLength;

        while( sv_longlongBytesRemaining > 0 )
        {
            dwLength = std::min( dwLength, (DWORD) sv_longlongBytesRemaining );

            if( FALSE == ReadFile( handleVolume, 
                pchBuffer,
                dwLength,
                &dwBytesRead,
                NULL ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED ReadFile()... hr = %08X\n", hr );
                break;
            }
            if( 0 == WriteFile( handleFile,
                pchBuffer,
                dwBytesRead,
                &dwBytesWritten,
                NULL ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED WriteFile()... hr = %08X\n", hr );
                break;
            }
            assert( dwBytesRead == dwBytesWritten );
            sv_longlongBytesRemaining -= dwBytesRead;
            *psv_longlongBytesRead += dwBytesRead;
        }
    }
    while( FALSE );

    delete[] pchBuffer;

    if( 0 == CloseHandle( handleFile ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( hr, "FAILED CloseHandle()... hr = %08X\n", hr );
    }

    return( hr );
}

///
/// Uncompresses a delta file.
///
SVERROR UncompressFile( char *pszFilename )
{
    HRESULT hr = S_OK;
    PROCESS_INFORMATION ProcessInfo = { 0 };
    STARTUPINFO StartupInfo = { 0 };
    StartupInfo.cb = sizeof( StartupInfo );
    char szUnzipExe[ SV_MAX_PATH ];
    DWORD dwWaitReturn = 0;
    DWORD dwResult = 0;
    DWORD dwCount = 0;
    CRegKey cregkey;

    do
    {
        //
        // We only uncompress the file if it ends with a '.gz' extension
        //
        if( endsWith( pszFilename, ".dat", false ) )
        {
            hr = S_FALSE;
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED UncompressFile()...\n" );

        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        dwCount = sizeof( szUnzipExe );
        dwResult = cregkey.QueryStringValue( "UncompressExe", szUnzipExe, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
            break;
        }

        char szArgs[ 2*SV_MAX_PATH ];


        inm_strcpy_s( szArgs, ARRAYSIZE(szArgs), "\"" );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), szUnzipExe );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), "\"" );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), " " );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), "-f" );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), " " );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), " \"" );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), pszFilename );
        inm_strcat_s( szArgs, ARRAYSIZE(szArgs), "\"" );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING CreateProcess(%s)...\n", szArgs );
        if( !CreateProcess( NULL, //szUnzipExe,
            szArgs,
            NULL,
            NULL,
            FALSE,
            CREATE_DEFAULT_ERROR_MODE,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInfo ) )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED CreateProcess(%s)... hr = %08X\n", szArgs, hr );
            break;
        }

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING WaitForSingleObject()...\n" );
        dwWaitReturn = WaitForSingleObject( ProcessInfo.hProcess, INFINITE );
        if( WAIT_OBJECT_0 == dwWaitReturn )
        {
            //
            // Check for the return code of the Uncompress. If it fails, quit.
            //
            DWORD dwProcessExitCode = 0;
            if( !GetExitCodeProcess( ProcessInfo.hProcess, &dwProcessExitCode ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED GetExitCodeProcess()... hr = %08X\n", hr );
                break;
            }
            else
            {
                if( 0 != dwProcessExitCode )
                {
                    hr = E_FAIL;
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED Uncompress... Error Code: %d... Exiting... hr = %08X\n", dwProcessExitCode, hr );
                    break;
                }
                else
                {
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( "Uncompress Successful...\n" );
                }
            }
        }
        else if( WAIT_ABANDONED == dwWaitReturn )
        {
            hr = E_FAIL;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WaitForSingleObject()... hr = %08X\n", hr );
            break;
        }
        else if( WAIT_FAILED == dwWaitReturn )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED WaitForSingleObject()... hr = %08X\n", hr );
            break;
        }

        pszFilename[ strlen( pszFilename ) - 3 ] = '\0';
    }
    while( FALSE );

    if( ( NULL != ProcessInfo.hProcess ) &&
        ( INVALID_HANDLE_VALUE != ProcessInfo.hProcess ) )
    {
        CloseHandle( ProcessInfo.hProcess );
    }

    if( ( NULL != ProcessInfo.hThread ) &&
        ( INVALID_HANDLE_VALUE != ProcessInfo.hThread ) )
    {
        CloseHandle( ProcessInfo.hThread );
    }

    return( hr );
}

///
/// Returns how far into the volume the boot strap process has proceeded.
///
HRESULT GetInitialSyncVolumeCheckPoint( DWORD *pdwOffsetLow, DWORD *pdwOffsetHigh, const char* pszDriveLetter )
{
    HRESULT hr = S_OK;
    DWORD dwOffsetLow = 0;
    DWORD dwOffsetHigh = 0;
    CRegKey cregkey;
    char szDriveLetter[MAX_PATH];
    char szDriveKey[MAX_PATH];

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetInitialSyncVolumeCheckPoint()...\n" );
        if( ( NULL == pdwOffsetLow ) || ( NULL == pdwOffsetHigh ) || (NULL == pszDriveLetter) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetInitialSyncVolumeCheckPoint()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s(szDriveLetter, ARRAYSIZE(szDriveLetter), pszDriveLetter);
        strupr  (szDriveLetter);
        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), SV_INITIALSYNC_OFFSET_LOW_VALUE_NAME);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey), " ");
        AppendChar (szDriveKey, szDriveLetter[0]); // E.G: Just "C" out of "C:\0"

        dwResult = cregkey.QueryDWORDValue( szDriveKey, dwOffsetLow);

        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            if (ERROR_SUCCESS != cregkey.SetDWORDValue(szDriveKey, 0) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
            // Query the value again
            else
            {
                dwResult = cregkey.QueryDWORDValue( szDriveKey, dwOffsetLow );
                if( ERROR_SUCCESS != dwResult )
                {
                    hr = HRESULT_FROM_WIN32( dwResult );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                    break;
                }
            }
        }

        inm_strcpy_s(szDriveLetter, ARRAYSIZE(szDriveLetter), pszDriveLetter);
        strupr  (szDriveLetter);
        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), SV_INITIALSYNC_OFFSET_HIGH_VALUE_NAME);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey), " ");
        AppendChar (szDriveKey, szDriveLetter[0]); // E.G: Just "C" out of "C:\0"

        dwResult = cregkey.QueryDWORDValue( szDriveKey, dwOffsetHigh);
        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            if (ERROR_SUCCESS != cregkey.SetDWORDValue(szDriveKey, 0) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
            // Query the value again
            else
            {
                dwResult = cregkey.QueryDWORDValue(szDriveKey, dwOffsetHigh);
                if( ERROR_SUCCESS != dwResult )
                {
                    hr = HRESULT_FROM_WIN32( dwResult );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                    break;
                }
            }
        }

        *pdwOffsetLow = dwOffsetLow;
        *pdwOffsetHigh = dwOffsetHigh;
    }
    while( FALSE );

    return( hr );
}

///
/// Marks how far we've proceeded in the bootstrap process.
///
HRESULT SetInitialSyncVolumeCheckPoint( DWORD dwOffsetLow, DWORD dwOffsetHigh, const char* pszDriveLetter )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szDriveLetter[MAX_PATH];
    char szDriveKey[MAX_PATH];

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED SetInitialSyncVolumeCheckPoint()...\n" );

        if( (NULL == pszDriveLetter) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SetResyncVolumeCheckPoint()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s(szDriveLetter, ARRAYSIZE(szDriveLetter), pszDriveLetter);
        strupr  (szDriveLetter);
        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), SV_INITIALSYNC_OFFSET_LOW_VALUE_NAME);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey), " ");
        AppendChar (szDriveKey, szDriveLetter[0]); // E.G: Just "C" out of "C:\"

        dwResult = cregkey.SetDWORDValue(szDriveKey, dwOffsetLow);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s(szDriveLetter, ARRAYSIZE(szDriveLetter), pszDriveLetter);
        strupr  (szDriveLetter);
        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), SV_INITIALSYNC_OFFSET_HIGH_VALUE_NAME);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey),  " ");
        AppendChar (szDriveKey, szDriveLetter[0]); // E.G: Just "C" out of "C:\"

        dwResult = cregkey.SetDWORDValue(szDriveKey, dwOffsetHigh);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }
    }
    while( FALSE );

    return( hr );
}

HRESULT GetVolumeChunkSizeFromRegistry( DWORD *pdwVolumeChunkSize )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;

    do
    {
        //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        //DebugPrintf( "ENTERED GetVolumeChunkSizeFromRegistry()...\n" );
        if( NULL == pdwVolumeChunkSize )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVolumeChunkSizeFromRegistry()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        dwResult = cregkey.QueryDWORDValue( SV_VOLUME_CHUNK_SIZE_VALUE_NAME,
            *pdwVolumeChunkSize );

        if( ( ERROR_SUCCESS != dwResult ) || ( 0 == *pdwVolumeChunkSize ) )
        {
            // Try and create the key as it might be the first time we tried to open it           
            *pdwVolumeChunkSize = DEFAULT_VOLUME_CHUNK_SIZE;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( SV_VOLUME_CHUNK_SIZE_VALUE_NAME,
                *pdwVolumeChunkSize ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}



HRESULT GetMaxResyncThreadsFromRegistry( DWORD *pdwMaxResyncThreads )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;

    do
    {
        //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        //DebugPrintf( "ENTERED GetMaxResyncThreadsFromRegistry()...\n" );
        if( NULL == pdwMaxResyncThreads )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetMaxResyncThreadsFromRegistry()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        dwResult = cregkey.QueryDWORDValue( SV_MAX_RESYNC_THREADS_VALUE_NAME,
            *pdwMaxResyncThreads );

        if( ( ERROR_SUCCESS != dwResult ) ||
            ( 0 == *pdwMaxResyncThreads )  ||
            ( *pdwMaxResyncThreads > MAX_RESYNC_THREADS ) )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwMaxResyncThreads = DEFAULT_RESYNC_THREADS;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( SV_MAX_RESYNC_THREADS_VALUE_NAME,
                *pdwMaxResyncThreads ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}


HRESULT GetMaxOutpostThreadsFromRegistry( DWORD *pdwMaxOutpostThreads )
{
    HRESULT hr = S_OK;
    CRegKey cregkey;

    do
    {
        //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        //DebugPrintf( "ENTERED GetMaxResyncThreadsFromRegistry()...\n" );
        if( NULL == pdwMaxOutpostThreads )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetMaxOutpostThreadsFromRegistry()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        dwResult = cregkey.QueryDWORDValue( SV_MAX_OUTPOST_THREADS_VALUE_NAME,
            *pdwMaxOutpostThreads );

        if( ( ERROR_SUCCESS != dwResult ) ||
            ( 0 == *pdwMaxOutpostThreads )  ||
            ( *pdwMaxOutpostThreads > MAX_OUTPOST_THREADS ) )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwMaxOutpostThreads = DEFAULT_OUTPOST_THREADS;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( SV_MAX_OUTPOST_THREADS_VALUE_NAME,
                *pdwMaxOutpostThreads ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}

void ReplaceChar(string & str, char inputChar, char outputChar) 
{
    SV_ULONG i;

    for (i=0;  i < str.length(); ++i) 
    {
        if (str[i] == inputChar) {
            str[i] = outputChar;
        }
    }
}


HRESULT SVCleanupDirectory( const char *pszDirectory, const char *pszFilePattern )
{
    HRESULT hr = S_OK;
    WIN32_FIND_DATA win32_find_data;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char szFilePattern[ SV_MAX_PATH ];
    DWORD dwResult = 0;

    do
    {
        if( ( NULL == pszDirectory ) || ( NULL == pszFilePattern ) )
        {
            hr = E_INVALIDARG;
            break;
        }
        inm_strcpy_s( szFilePattern, ARRAYSIZE(szFilePattern), pszDirectory );
        inm_strcat_s( szFilePattern, ARRAYSIZE(szFilePattern), "\\" );
        inm_strcat_s( szFilePattern, ARRAYSIZE(szFilePattern), pszFilePattern );

        hFile = FindFirstFile( szFilePattern, &win32_find_data );
        if( INVALID_HANDLE_VALUE != hFile ) 
        {
            char szFilePath[ SV_MAX_PATH ];
            inm_strcpy_s( szFilePath, ARRAYSIZE(szFilePath), pszDirectory );
            inm_strcat_s( szFilePath, ARRAYSIZE(szFilePath), "\\" );
            inm_strcat_s( szFilePath, ARRAYSIZE(szFilePath), win32_find_data.cFileName );
            if( !DeleteFile( szFilePath ) )
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED DeleteFile()... hr = %08X\n", hr );
            }
        }
        else
        {
            break;
        }

        while( TRUE )
        {
            if( !FindNextFile( hFile, &win32_find_data ) )
            {
                dwResult = GetLastError();
                if( ERROR_NO_MORE_FILES == dwResult )
                {
                    break;
                }
            }
            else
            {
                char szFilePath[ SV_MAX_PATH ];
                inm_strcpy_s( szFilePath, ARRAYSIZE(szFilePath), pszDirectory );
                inm_strcat_s( szFilePath, ARRAYSIZE(szFilePath), "\\" );
                inm_strcat_s( szFilePath, ARRAYSIZE(szFilePath), win32_find_data.cFileName );
                if( !DeleteFile( szFilePath ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED DeleteFile()... hr = %08X\n", hr );
                }
            }
        }
    }
    while( FALSE );

    if( INVALID_HANDLE_VALUE != hFile )
    {
        if( !FindClose( hFile ) )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED FindClose()... hr = %08X\n", hr );
        }
    }

    return( hr );
}

HRESULT GetBytesAppliedThreshold( DWORD *pdwBytesAppliedThreshold, char const *pszDestVolume )
{
    HRESULT hr = S_OK;
    char szBytesAppliedThresholdPerVolumeValueName[ SV_MAX_PATH ];
    CRegKey cregkey;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetBytesAppliedThreshold()...\n" );
        if( ( NULL == pdwBytesAppliedThreshold ) ||
            ( NULL == pszDestVolume ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetBytesAppliedThreshold()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( szBytesAppliedThresholdPerVolumeValueName,
            ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), SV_BYTES_APPLIED_THRESHOLD_VALUE_NAME );
        inm_strcat_s( szBytesAppliedThresholdPerVolumeValueName, ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), "\\" );
        char szUpperCaseVolume[ 10 ];
        szUpperCaseVolume[ 0 ] = toupper( *pszDestVolume );
        szUpperCaseVolume[ 1 ] = '\0';
        inm_strcat_s( szBytesAppliedThresholdPerVolumeValueName, ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), szUpperCaseVolume );

        dwResult = cregkey.QueryDWORDValue( szBytesAppliedThresholdPerVolumeValueName,
            *pdwBytesAppliedThreshold );

        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwBytesAppliedThreshold = DEFAULT_BYTES_APPLIED_THRESHOLD;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( szBytesAppliedThresholdPerVolumeValueName,
                *pdwBytesAppliedThreshold ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}

// HACK: temp hack to simulate volume groups
HRESULT GetFilesAppliedThreshold( DWORD *pdwAppliedThreshold, char const *pszDestVolume )
{
    HRESULT hr = S_OK;
    char szAppliedThresholdPerVolumeValueName[ SV_MAX_PATH ];
    CRegKey cregkey;

    do
    {                
        if( ( NULL == pdwAppliedThreshold ) ||
            ( NULL == pszDestVolume ) )
        {
            hr = E_INVALIDARG;            
            DebugPrintf( hr, "FAILED GetFilesAppliedThreshold()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );            
            DebugPrintf( hr, "FAILED GetFilesAppliedThreshold cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( szAppliedThresholdPerVolumeValueName,
            ARRAYSIZE(szAppliedThresholdPerVolumeValueName), SV_FILES_APPLIED_THRESHOLD_VALUE_NAME );
        inm_strcat_s( szAppliedThresholdPerVolumeValueName, ARRAYSIZE(szAppliedThresholdPerVolumeValueName), "\\" );
        char szUpperCaseVolume[ 10 ];
        szUpperCaseVolume[ 0 ] = toupper( *pszDestVolume );
        szUpperCaseVolume[ 1 ] = '\0';
        inm_strcat_s( szAppliedThresholdPerVolumeValueName, ARRAYSIZE(szAppliedThresholdPerVolumeValueName), szUpperCaseVolume );

        dwResult = cregkey.QueryDWORDValue( szAppliedThresholdPerVolumeValueName,
            *pdwAppliedThreshold );

        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwAppliedThreshold = DEFAULT_FILES_APPLIED_THRESHOLD;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( szAppliedThresholdPerVolumeValueName,
                *pdwAppliedThreshold ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );                
                DebugPrintf( hr, "FAILED GetFilesAppliedThreshold cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}

// HACK: temp hack to simulate volume groups
HRESULT GetTimestampAppliedThreshold( DWORD *pdwAppliedThreshold, char const *pszDestVolume )
{
    HRESULT hr = S_OK;
    char szAppliedThresholdPerVolumeValueName[ SV_MAX_PATH ];
    CRegKey cregkey;

    do
    {                
        if( ( NULL == pdwAppliedThreshold ) ||
            ( NULL == pszDestVolume ) )
        {
            hr = E_INVALIDARG;            
            DebugPrintf( hr, "FAILED GetFilesAppliedThreshold()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );            
            DebugPrintf( hr, "FAILED GetFilesAppliedThreshold cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( szAppliedThresholdPerVolumeValueName,
            ARRAYSIZE(szAppliedThresholdPerVolumeValueName), SV_TIME_STAMP_APPLIED_THRESHOLD_VALUE_NAME );
        inm_strcat_s( szAppliedThresholdPerVolumeValueName, ARRAYSIZE(szAppliedThresholdPerVolumeValueName), "\\" );
        char szUpperCaseVolume[ 10 ];
        szUpperCaseVolume[ 0 ] = toupper( *pszDestVolume );
        szUpperCaseVolume[ 1 ] = '\0';
        inm_strcat_s( szAppliedThresholdPerVolumeValueName, ARRAYSIZE(szAppliedThresholdPerVolumeValueName), szUpperCaseVolume );

        dwResult = cregkey.QueryDWORDValue( szAppliedThresholdPerVolumeValueName,
            *pdwAppliedThreshold );

        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwAppliedThreshold = DEFAULT_FILES_APPLIED_THRESHOLD;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( szAppliedThresholdPerVolumeValueName,
                *pdwAppliedThreshold ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );                
                DebugPrintf( hr, "FAILED GetFilesAppliedThreshold cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}

HRESULT GetSyncBytesAppliedThreshold( DWORD *pdwBytesAppliedThreshold, char const *pszDestVolume )
{
    HRESULT hr = S_OK;
    char szBytesAppliedThresholdPerVolumeValueName[ SV_MAX_PATH ];
    CRegKey cregkey;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetSyncBytesAppliedThreshold()...\n" );
        if( ( NULL == pdwBytesAppliedThreshold ) ||
            ( NULL == pszDestVolume ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetSyncBytesAppliedThreshold()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            SV_VXAGENT_VALUE_NAME );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( szBytesAppliedThresholdPerVolumeValueName,
            ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), SV_SYNC_BYTES_APPLIED_THRESHOLD_VALUE_NAME );
        inm_strcat_s( szBytesAppliedThresholdPerVolumeValueName, ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), "\\" );
        char szUpperCaseVolume[ 10 ];
        szUpperCaseVolume[ 0 ] = toupper( *pszDestVolume );
        szUpperCaseVolume[ 1 ] = '\0';
        inm_strcat_s( szBytesAppliedThresholdPerVolumeValueName, ARRAYSIZE(szBytesAppliedThresholdPerVolumeValueName), szUpperCaseVolume );

        dwResult = cregkey.QueryDWORDValue( szBytesAppliedThresholdPerVolumeValueName,
            *pdwBytesAppliedThreshold );

        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            *pdwBytesAppliedThreshold = DEFAULT_SYNC_BYTES_APPLIED_THRESHOLD;
            if (ERROR_SUCCESS != cregkey.SetDWORDValue( szBytesAppliedThresholdPerVolumeValueName,
                *pdwBytesAppliedThreshold ) )
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
        }
    }
    while( FALSE );

    return( hr );
}

///
/// Returns the total bytes in the volume (not the free space).
/// 0 returned upon failure.
///
SV_LONGLONG GetVolumeCapacity( char chDriveLetter )
{
    assert( isalpha( chDriveLetter ) );

    SV_LONGLONG capacity = 0;

    char szDrive[] = "X:\\";
    szDrive[ 0 ] = chDriveLetter;

    ULARGE_INTEGER uliFreeBytesAvailable, uliVolumeCapacity;
    // PR#10815: Long Path support
    if( SVGetDiskFreeSpaceEx( szDrive, &uliFreeBytesAvailable, &uliVolumeCapacity, NULL ) )
    {
        capacity = uliVolumeCapacity.QuadPart;
    }

    return( capacity );
}

#define TEST_GET_EXTENTS 0
#define TEST_BINARY_SEARCH 0
#define TEST_COMPLETE_BINARY_SEARCH 0
#define PRINT_SIZES 0

//-----------------------------------------------------------------------------
// This is a function that works under W2K and later to get the correct volume
// size on all flavors of volumes (static/dynamic, raid/striped/mirror/spanned)
// On WXP and later, IOCTL_DISK_GET_LENGTH_INFO already does this, so that is
// what we try first. This code is a direct translation to user mode of some
// kernel code in host\driver\win32\shared\MntMgr.cpp
//
// IMPORTANT: It turns out from user mode, a file system will slightly reduce
// the size of a volume by hiding some blocks. You need to call the ioctl
// DeviceIoControl(volumeHandle,
//                 FSCTL_ALLOW_EXTENDED_DASD_IO, 
//                 NULL, 0, NULL, 0, &status, NULL);
// on any volume that uses this function, or expect to read or write ALL of a
// volume. This tells the filesystem to let the underlying volume do the bounds
// checking, not the filesystem This ioctl will probably fail on raw volumes,
// just ignore the error and call it on every volume. You may 
// need #define _WIN32_WINNT 0x0510 before your headers to get this ioctl
//
// This function expects as input a handle opened on a volume with at least
// read access. A status value from GetLastErrro() will be return if needed
// The volume size will be in bytes, although always a multiple of 512
//-----------------------------------------------------------------------------
// Use GetVolumeSize in portablehelpers.cpp for portability 
/*
DWORD GetVolumeSize(HANDLE volumeHandle, 
                    LONGLONG * volumeSize)
{
#define MAXIMUM_SECTORS_ON_VOLUME (1024i64 * 1024i64 * 1024i64 * 1024i64)
#define DISK_SECTOR_SIZE (512)

    PVOLUME_DISK_EXTENTS pDiskExtents; // dynamically sized allocation based on number of extents
    GET_LENGTH_INFORMATION  lengthInfo;
    DWORD bytesReturned;
    DWORD returnBufferSize;
    DWORD status;
    BOOLEAN fSuccess;
    LONGLONG currentGap;
    LONGLONG currentOffset;
    LARGE_INTEGER largeInt;
    PUCHAR sectorBuffer;

    // try a simple IOCTl to get size, should work on WXP and later
    fSuccess = DeviceIoControl(volumeHandle,
                               IOCTL_DISK_GET_LENGTH_INFO,
                               NULL, 
                               0,
                               &lengthInfo, 
                               sizeof(lengthInfo),
                               &bytesReturned,
                               NULL);
#if (PRINT_SIZES)
    printf("Volume size via IOCTL_DISK_GET_LENGTH_INFO is hex:%I64X or decimal:%I64i\n", lengthInfo.Length.QuadPart, lengthInfo.Length.QuadPart);
#endif

#if (TEST_GET_EXTENTS || TEST_BINARY_SEARCH || TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (fSuccess) {
         *volumeSize = lengthInfo.Length.QuadPart;
         return ERROR_SUCCESS;
    }

    // next attempt, see if the volume is only 1 extent, if yes use it
    returnBufferSize = sizeof(VOLUME_DISK_EXTENTS);
    pDiskExtents = (PVOLUME_DISK_EXTENTS) new UCHAR[returnBufferSize];
    fSuccess =DeviceIoControl(volumeHandle,
                              IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                              NULL,
                              0,
                              pDiskExtents,
                              returnBufferSize,
                              &bytesReturned,
                              NULL);
#if (PRINT_SIZES)
    printf("Volume size via IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS is hex:%I64X or decimal:%I64i\n", pDiskExtents->Extents[0].ExtentLength.QuadPart, pDiskExtents->Extents[0].ExtentLength.QuadPart);
#endif
#if (TEST_BINARY_SEARCH || TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (fSuccess) {
        // must only have 1 extent as the buffer only has space for one
        assert(pDiskExtents->NumberOfDiskExtents == 1);
        *volumeSize = pDiskExtents->Extents[0].ExtentLength.QuadPart;
        delete pDiskExtents;
        return ERROR_SUCCESS;
    }

    // use harder ways of finding the size
    status = GetLastError();
#if (TEST_BINARY_SEARCH)
    status = ERROR_MORE_DATA;
#endif
    if (ERROR_MORE_DATA != status) {
        // some error other than buffer too small happened
        delete pDiskExtents;
        return status;
    }

    // has multiple extents
    returnBufferSize = sizeof(VOLUME_DISK_EXTENTS) + ((pDiskExtents->NumberOfDiskExtents - 1) * sizeof(DISK_EXTENT));
    delete pDiskExtents;
    pDiskExtents = (PVOLUME_DISK_EXTENTS) new UCHAR[returnBufferSize];
    fSuccess = DeviceIoControl(volumeHandle,
                               IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                               NULL, 0,
                               pDiskExtents, returnBufferSize,
                               &bytesReturned,
                               NULL);
#if (TEST_COMPLETE_BINARY_SEARCH)
    fSuccess = FALSE;
#endif
    if (!fSuccess) {
        currentOffset = MAXIMUM_SECTORS_ON_VOLUME; // if IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed, do a binary search with a large limit
    } else {
        // must be multiple extents, so will have to binary search last valid sector
       
        currentOffset = 0;
        for(unsigned int i = 0;i < pDiskExtents->NumberOfDiskExtents;i++) {
            currentOffset += pDiskExtents->Extents[i].ExtentLength.QuadPart;
        }

        // scale things now so we don't need to divide on every read
        currentOffset += DISK_SECTOR_SIZE; // these two are needed to make the search algorithm work
        currentOffset /= DISK_SECTOR_SIZE;
    }

    delete pDiskExtents;

    // the search gap MUST be a power of two, so find an appropriate value
    currentGap = 1i64;
    while ((currentGap * 2i64) < currentOffset) {
        currentGap *= 2i64;
    }

    // we are all ready to binary search for the last valid sector
    sectorBuffer = (PUCHAR)VirtualAlloc(NULL, DISK_SECTOR_SIZE, MEM_COMMIT, PAGE_READWRITE); 
    do {
        largeInt.QuadPart = currentOffset * DISK_SECTOR_SIZE;
        SetFilePointerEx(volumeHandle,
                         largeInt,
                         NULL,
                         FILE_BEGIN);

        fSuccess = ReadFile(volumeHandle,
                            sectorBuffer,
                            DISK_SECTOR_SIZE,
                            &bytesReturned,
                            NULL);

        if (fSuccess && (bytesReturned == DISK_SECTOR_SIZE)) {
            currentOffset += currentGap;
            if (currentGap == 0) {
                *volumeSize = (currentOffset * DISK_SECTOR_SIZE) + DISK_SECTOR_SIZE;
                status = ERROR_SUCCESS;
                break;
            }
        } else {
            currentOffset -= currentGap;
            if (currentGap == 0) {
                *volumeSize =  currentOffset * DISK_SECTOR_SIZE;
                status = ERROR_SUCCESS;
                break;
            }
        }
        currentGap /= 2;

    } while (TRUE);

    VirtualFree(sectorBuffer, DISK_SECTOR_SIZE, MEM_DECOMMIT);

#if (PRINT_SIZES)
    printf("Volume size via binary search is hex:%I64X or decimal:%I64i\n", *volumeSize, *volumeSize);
#endif

    return status;
}
*/

void FillMemoryPattern( BYTE* pBuffer, int cbBuffer, DWORD dwPattern )
{
    /*
    int n = cbBuffer / 4;
    while( n-- > 0 )
    {
    *((DWORD*) pBuffer) = dwPattern;
    pBuffer += sizeof( DWORD );
    }

    char* pchPattern = (char*) &dwPattern;
    switch( n & 3 )
    {
    case 3: pBuffer[ 2 ] = pchPattern[ 2 ]; 
    case 2: pBuffer[ 1 ] = pchPattern[ 1 ]; 
    case 1: pBuffer[ 0 ] = pchPattern[ 0 ];
    case 0: break;
    }
    */
    assert( !"To implement" );
}

bool GetTargetReplicationJobId(char const * server,
                               SV_INT httpPort,                               
                               char const * hostId,
                               char const * vol,                                        
                               std::string& jobId)
{
    std::stringstream request;
    jobId.clear();

    request << "get_replication_jobid.php?id=" << hostId << "&vol=" << vol[0];

    char* buffer = 0;

    SVERROR err = GetFromSVServer(server, httpPort, request.str().c_str(), &buffer);  
    if (err.succeeded()) {
        char * end = buffer + (strlen(buffer) - 1);        
        while (end >= buffer && !std::isprint((*end), std::cout.getloc())) {
            (*end) = '\0'; 
            --end;
        }

        while ( end >= buffer ) {
            if(!std::isdigit((*end), std::cout.getloc()))
            {
                delete [] buffer;
                return false;
            }
            --end;
        }
   
        jobId = buffer;
    }

    delete [] buffer;

    return err.succeeded();
}

HRESULT GetVolumeCheckpointFromSVServer(char const * pszSVServerName,
                                        SV_INT HttpPort,
                                        char const * pszGetVolumeCheckpointURL,
                                        char const * pszHostID,
                                        char const * pchSrcVolume,
                                        ULARGE_INTEGER* cbTotalSize,
                                        std::string& jobId)
{
    HRESULT hr = S_OK;
    char *pszGetBuffer = NULL;
    char *pszGetURL = NULL;
    //char strippedVolume[1];

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetVolumeCheckpointFromSVServer()...\n" );

        const size_t PSZGETURL_SIZE = 1024;
        pszGetURL = new char[ PSZGETURL_SIZE ];
        if( NULL == pszGetURL )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetVolumeCheckpointFromSVServer()... hr = %08X\n", hr );
            break;
        }


        memset(pszGetURL,0,1024);
        inm_strncpy_s( pszGetURL, PSZGETURL_SIZE, pszGetVolumeCheckpointURL,1023 );
        pszGetURL[1023] = '\0';
        inm_strncat_s( pszGetURL, PSZGETURL_SIZE, "?id=",std::min( sizeof("?id="), 
            MAXSTRINGLEN(pszGetURL)-strlen(pszGetURL))  );
        inm_strncat_s( pszGetURL, PSZGETURL_SIZE, pszHostID,std::min( strlen(pszHostID), 
            MAXSTRINGLEN(pszGetURL)-strlen(pszGetURL))  );      

        inm_strncat_s( pszGetURL, PSZGETURL_SIZE, "&vol=",std::min( sizeof("&vol="), 
            MAXSTRINGLEN(pszGetURL)-strlen(pszGetURL))  );
        inm_strncat_s( pszGetURL, PSZGETURL_SIZE, pchSrcVolume,std::min( (size_t)1, 
            MAXSTRINGLEN(pszGetURL)-strlen(pszGetURL))  );      

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "[INFO] GetVolumeCheckpointFromSVServer CALLING GetFromSVServer()with URL = %s...\n",pszGetURL );

        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "CALLING GetVolumeCheckpointFromSVServer GetFromSVServer()...\n" );
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
            DebugPrintf( hr, "FAILED GetVolumeCheckpointFromSVServer GetFromSVServer()... SVServerName: %s; URL: %s; hr = %08X\n", pszSVServerName, pszGetURL, hr );
            break;
        }
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "[INFO] GetVolumeCheckpointFromSVServer Received this response string for Volume Checkpoint from Server - %s \n",pszGetBuffer );
        if(pszGetBuffer == NULL || strlen(pszGetBuffer)<=0)
        {
            hr = E_POINTER;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "[WARNING] GetVolumeCheckpointFromSVServer Got invalid response from server... hr = %08X\n", hr );
            break;

        }

        //parse server response
        // now jobid,offset
        char* delim = strchr(pszGetBuffer, ',');
        if (0 == delim) {
            hr = E_FAIL;
            break;
        }
        *delim  = '\0';
        ++delim;

        jobId = pszGetBuffer;      
        if (0 == strlen(delim)) 
        {
            hr = E_FAIL;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "GetVolumeCheckpointFromSVServer offset missing (%s, %s)\n", pszHostID, pchSrcVolume);
            break;
        }

        cbTotalSize->QuadPart = _atoi64(delim);
    }
    while( FALSE );

    delete[] pszGetBuffer;
    delete[] pszGetURL;

    return( hr );
}

bool
DriveLetterToGUID(char DriveLetter, WCHAR *VolumeGUID, ULONG ulBufLen)
{
    WCHAR   MountPoint[4];
    WCHAR   UniqueVolumeName[60];

    MountPoint[0] = (WCHAR)DriveLetter;
    MountPoint[1] = L':';
    MountPoint[2] = L'\\';
    MountPoint[3] = L'\0';

    if (ulBufLen  < (GUID_SIZE_IN_CHARS * sizeof(WCHAR)))
        return false;

    if (0 == GetVolumeNameForVolumeMountPointW((WCHAR *)MountPoint, (WCHAR *)UniqueVolumeName, 60)) {
        DebugPrintf(SV_LOG_ERROR, "DriveLetterToGUID failed for %c: \n", DriveLetter);
        return false;
    }

    inm_memcpy_s(VolumeGUID, ulBufLen,&UniqueVolumeName[11], (GUID_SIZE_IN_CHARS * sizeof(WCHAR)));

    return true;
}

///
/// Removes all trailing backslashes
///
LPSTR PathRemoveAllBackslashesA( LPSTR pszPath )
{
    assert( NULL != pszPath );

    int cch = strlen( pszPath );
    char* psz = pszPath + cch - 1 ;
    for( ; psz >= pszPath && '\\' == *psz; psz-- )
    {
        *psz = 0;
    }

    return( std::max( psz, pszPath ) );
}

///
/// Removes all trailing backslashes
///
LPWSTR PathRemoveAllBackslashesW( LPWSTR pszPath )
{
    assert( NULL != pszPath );

    WCHAR* psz;
    do
    {
        psz = PathRemoveBackslashW( pszPath ) - 1;
    } while( 0 != pszPath[0] && L'\\' == *psz );

    return( psz );
}

//
// BUGBUG: this function signature is not portably defined. How to distinguish between
// SV_INVALID_FILE_SIZE and a valid file size?
//
SV_ULONG SVGetFileSize( char const* pszPathname, OPTIONAL SV_ULONG* pHighSize )
{
    DWORD dwLowSize = INVALID_FILE_SIZE;
    HANDLE hFile = NULL;

    assert( NULL != pszPathname );

    do
    {
        // PR#10815: Long Path support
        hFile = SVCreateFile( pszPathname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( !IS_INVALID_HANDLE_VALUE( hFile ) )
        {
            dwLowSize = GetFileSize( hFile, pHighSize );
        }
    }
    while( FALSE );

    SAFE_CLOSEHANDLE( hFile );
    return( dwLowSize );
}

//
// Checks whether a file path exists
//
bool SVIsFilePathExists( char const* pszPathname )
{
    HANDLE hFile = NULL;

    assert( NULL != pszPathname );

    do
    {
        // PR#10815: Long Path support
        hFile = SVCreateFile( pszPathname, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
        if( IS_INVALID_HANDLE_VALUE( hFile ) )
        {
            SAFE_CLOSEHANDLE( hFile );
            return false;
        }
    }
    while( FALSE );

    SAFE_CLOSEHANDLE( hFile );
    return true;
}

SVERROR GetConfigFileName(std::string &strConfFileName)
{
    return SVS_OK;
}

static BOOL GetProductVersionFromResource(LPSTR pszFile, LPTSTR pszBuffer, int pszBufferSize)
{
    assert( NULL != pszFile );
    assert( NULL != pszBuffer );

    //
    // TODO: clean up this code. Make language/region independent
    //
    DWORD dwVerHnd;
    DWORD dwVerInfoSize = GetFileVersionInfoSize( pszFile, &dwVerHnd );
    if( !dwVerInfoSize )
        return FALSE;

    BYTE* pVerInfo = new BYTE[ dwVerInfoSize ];
    if( !pVerInfo )
        return FALSE;

    BOOL fRet = GetFileVersionInfo( pszFile, 0L, dwVerInfoSize, pVerInfo );
    if (fRet)
    {
        TCHAR* pVer = NULL;
        UINT ccVer = 0;
        fRet = VerQueryValue(pVerInfo,                            
                _T("\\StringFileInfo\\040904B0\\FileVersion"),
                (void**)&pVer,
                    &ccVer);
        if (fRet && pVer)
            inm_tcscpy_s(pszBuffer, pszBufferSize, pVer);
    }
 
    delete pVerInfo;
    return fRet;
}

const char* GetProductVersion()
{
    TCHAR szFile[ SV_MAX_PATH ];
    GetModuleFileName( NULL, szFile, SV_MAX_PATH );

    static char szVersion[ 200 ];
    
    if (!GetProductVersionFromResource(szFile, szVersion, ARRAYSIZE(szVersion)))
    {
        return( "" );
    }
    else
    {
        return( szVersion );
    }
}

const char* GetDriverVersion()
{
    TCHAR szFile[ SV_MAX_PATH ];
    if( !ExpandEnvironmentStrings( _T( "%systemroot%\\System32\\drivers\\" FILTER_NAME FILTER_EXTENSION ), szFile, ARRAYSIZE( szFile ) ) )
    {
        return( "" );
    }

    static char szVersion[ 200 ];
    if (!GetProductVersionFromResource(szFile, szVersion, ARRAYSIZE(szVersion)))
    {
        return( "" );
    }
    else
    {
        return( szVersion );
    }
}

char* GetThreadsafeErrorStringBuffer()
{
    __declspec(thread) static char szBuffer[ THREAD_SAFE_ERROR_STRING_BUFFER_LENGTH ];
    return( szBuffer );
}

SVERROR DeleteRegistrySettings(SV_HOST_AGENT_TYPE AgentType)
{
    SVERROR rc;
    std::string sConfigFilePath;
    CRegKey cregkey;
    char KeyName[256];
    char szValue[ 4*MAX_PATH ];
    DWORD dwCount = sizeof( szValue );
    DWORD dwResult = ERROR_SUCCESS;
    char AGENT_NAME[256];
    char Reg_Agent_value[256];



   if(AgentType == SV_AGENT_SENTINEL)
    {
        inm_strcpy_s(KeyName, ARRAYSIZE(KeyName), SV_VXAGENT_VALUE_NAME);
        inm_strcpy_s(Reg_Agent_value, ARRAYSIZE(Reg_Agent_value), SV_FILEREP_AGENT_VALUE_NAME);
        inm_strcpy_s(AGENT_NAME, ARRAYSIZE(AGENT_NAME), "FxAgent");
    }
   else if(AgentType == SV_AGENT_FILE_REPLICATION)
    {
        inm_strcpy_s(KeyName, ARRAYSIZE(KeyName), SV_FILEREP_AGENT_VALUE_NAME);
        inm_strcpy_s(Reg_Agent_value, ARRAYSIZE(Reg_Agent_value), SV_VXAGENT_VALUE_NAME);
        inm_strcpy_s(AGENT_NAME, ARRAYSIZE(AGENT_NAME), "VxAgent");
    }

    if(AgentType != SV_AGENT_UNKNOWN)
    {
        rc = GetConfigFileName(sConfigFilePath);
        if(rc == SVS_OK)
        {
            if( ERROR_SUCCESS == cregkey.Open( HKEY_LOCAL_MACHINE, KeyName ))
            {
                // Enforce return checks 
                cregkey.DeleteValue(SV_CACHE_DIRECTORY_VALUE_NAME);
                cregkey.Close();
            }

            
            if ( ERROR_SUCCESS == cregkey.Open( HKEY_LOCAL_MACHINE, Reg_Agent_value ))
            {
                dwResult = cregkey.QueryStringValue( AGENT_NAME, szValue, &dwCount );    
                if( ERROR_SUCCESS != dwResult )
                {
                    char SVKeyName[256];
                    inm_strcpy_s(SVKeyName, ARRAYSIZE(SVKeyName), SV_VALUE_NAME);
                    if ( ERROR_SUCCESS == cregkey.Open( HKEY_LOCAL_MACHINE, SVKeyName ))
                    {
                        /* Enforce return checks */
                      cregkey.DeleteValue(SV_HOST_ID_VALUE_NAME);
                      cregkey.Close();
                    }
                }
            }

        
        }
    }
    return SVS_OK;
}

SVERROR DeleteStaleRegistrySettings(SV_HOST_AGENT_TYPE AgentType)
{
    SVERROR rc;
    std::string sConfigFilePath;
    CRegKey cregkey;
    char KeyName[256], ValueName[256];
    int AgentCount = 0;

    if(AgentType == SV_AGENT_SENTINEL)
    {
        inm_strcpy_s(KeyName, ARRAYSIZE(KeyName), SV_VXAGENT_VALUE_NAME);
    }
    else if(AgentType == SV_AGENT_OUTPOST )
    {
        inm_strcpy_s(KeyName, ARRAYSIZE(KeyName), SV_VXAGENT_VALUE_NAME);
    }
    else if(AgentType == SV_AGENT_FILE_REPLICATION)
    {
        inm_strcpy_s(KeyName, ARRAYSIZE(KeyName), SV_FILEREP_AGENT_VALUE_NAME);
    }

    if(AgentType != SV_AGENT_UNKNOWN)
    {
        rc = GetConfigFileName(sConfigFilePath);
        if(rc == SVS_OK)
        {
            if( ERROR_SUCCESS == cregkey.Open( HKEY_LOCAL_MACHINE, KeyName ))
            {
                if(AgentType == SV_AGENT_SENTINEL)
                {
                    CRegKey cregkey2;
                    char ch_str[] = "A", ch_end = 'Z';
                    cregkey2.Open( HKEY_LOCAL_MACHINE, SV_ROOT_TRANSPORT_KEY );

                    while(ch_str[0] <= ch_end)
                    {
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_SRC_SECURE_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey2.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_DEST_SECURE_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey2.DeleteValue(ValueName);

                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_INITIALSYNC_OFFSET_LOW_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_INITIALSYNC_OFFSET_HIGH_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        ch_str[0] += 1;
                    }
                    cregkey2.Close();
                    cregkey.DeleteValue(SV_REGISTER_SYSTEM_DRIVE);
                }
                if(AgentType == SV_AGENT_OUTPOST)
                {
                    CRegKey cregkey2;
                    char ch_str[] = "A", ch_end = 'Z';
                    cregkey2.Open( HKEY_LOCAL_MACHINE, SV_ROOT_TRANSPORT_KEY );

                    while(ch_str[0] <= ch_end)
                    {
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_SRC_SECURE_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey2.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_DEST_SECURE_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey2.DeleteValue(ValueName);

                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_BYTES_APPLIED_THRESHOLD_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), "\\");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_SYNC_BYTES_APPLIED_THRESHOLD_VALUE_NAME);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), "\\");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_SNAPSHOT_OFFSET_HIGH);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        inm_strcpy_s(ValueName, ARRAYSIZE(ValueName), SV_SNAPSHOT_OFFSET_LOW);
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), " ");
                        inm_strcat_s(ValueName, ARRAYSIZE(ValueName), ch_str);
                        cregkey.DeleteValue(ValueName);
                        ch_str[0] += 1;
                    }
                    cregkey2.Close();
                }
                if(AgentType == SV_AGENT_FILE_REPLICATION)
                {
                }
                cregkey.Close();
            }
        }
    }

    /* Now delete shared values */
    if(ERROR_SUCCESS == cregkey.Open(HKEY_LOCAL_MACHINE, SV_VXAGENT_VALUE_NAME))
    {
        cregkey.Close();
        AgentCount++;
    }
    if(ERROR_SUCCESS == cregkey.Open(HKEY_LOCAL_MACHINE, SV_VXAGENT_VALUE_NAME))
    {
        cregkey.Close();
        AgentCount++;
    }
    if(ERROR_SUCCESS == cregkey.Open(HKEY_LOCAL_MACHINE, SV_FILEREP_AGENT_VALUE_NAME))
    {
        cregkey.Close();
        AgentCount++;
    }
    if( AgentCount <= 1)
    {
        if(ERROR_SUCCESS == cregkey.Open(HKEY_LOCAL_MACHINE, SV_VALUE_NAME))
        {
            /* Delete shared keys under SV root */
            cregkey.DeleteValue(SV_SERVER_NAME_VALUE_NAME);
            cregkey.DeleteValue(SV_SERVER_HTTP_PORT_VALUE_NAME);
            cregkey.DeleteValue(SV_HOST_AGENT_TYPE_VALUE_NAME);
            cregkey.DeleteValue(SV_UNCOMPRESS_EXE_VALUE_NAME);
            cregkey.DeleteValue(THROTTLE_BOOTSTRAP_URL);
            cregkey.DeleteValue(THROTTLE_SENTINEL_URL);
            cregkey.DeleteValue(THROTTLE_OUTPOST_URL);
            cregkey.DeleteValue(THROTTLE_AGENT_URL);
            cregkey.DeleteValue(SV_DEBUG_LOG_LEVEL_VALUE_NAME);
            cregkey.DeleteValue(SV_SERVER_NAME_VALUE_NAME);
            cregkey.Close();
        }

        if(ERROR_SUCCESS == cregkey.Open(HKEY_LOCAL_MACHINE, SV_TRANSPORT_VALUE_NAME))
        {
            /* Delete Transport shared keys */
            cregkey.DeleteValue(SV_FTP_HOST_VALUE_NAME);
            cregkey.DeleteValue(SV_FTP_USER_VALUE_NAME);
            cregkey.DeleteValue(SV_FTP_PASSWORD_VALUE_NAME);
            cregkey.DeleteValue(SV_FTP_PORT_VALUE_NAME);
            cregkey.DeleteValue(SV_FTP_URL_VALUE_NAME);
            cregkey.DeleteValue(SV_INMSYNC_TIMEOUT_SECONDS_VALUE_NAME);
            cregkey.Close();
        }
    }
    return SVS_OK;
}

bool GetInmsyncTimeoutFromRegistry(unsigned long& timeOut)
{
    bool ok = true;

    CRegKey regKey;
    
    char* transportKeyName = "\\Transport";
    char keyName[256];

    inm_sprintf_s(keyName, ARRAYSIZE(keyName), "%s%s", SV_VALUE_NAME, transportKeyName);

    DWORD result = regKey.Open(HKEY_LOCAL_MACHINE, keyName);
    if(ERROR_SUCCESS != result )
    {        
        DebugPrintf( "@ LINE %d in FILE %s GetInmsyncTimeoutFromRegistry failed open: %d\n", __LINE__, __FILE__, result );
        ok = false;
    } else {
        result = regKey.QueryDWORDValue("InmsyncTimeoutSeconds", timeOut);
        if (ERROR_SUCCESS != result) {
            DebugPrintf( "@ LINE %d in FILE %s GetInmsyncTimeoutFromRegistry failed query: %d\n", __LINE__, __FILE__, result );
            ok = false; 
        }
    }

    return ok;
}

void GetKernelVersion(std::string& kernelVersion)
{
    try {
        const Object & osinfo = OperatingSystem::GetInstance().GetOsInfo();
        std::stringstream ssversion;

        std::vector<std::string> keys;
        keys.push_back(NSOsInfo::MAJOR_VERSION);
        keys.push_back(NSOsInfo::MINOR_VERSION);
        keys.push_back(NSOsInfo::BUILD);

        ConstAttributesIter_t it;
        std::vector<std::string>::iterator keyIter = keys.begin();
        for (/*empty*/; keyIter != keys.end(); keyIter++)
        {
            it = osinfo.m_Attributes.find(*keyIter);
            if (osinfo.m_Attributes.end() != it)
            {
                ssversion << it->second;
                ssversion << ".";
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR,
                    "%s: failed to find %s in OsInfo.\n",
                    FUNCTION_NAME,
                    keyIter->c_str());
                return;
            }
        }
        kernelVersion = ssversion.str();
    }
    catch (const std::exception &e)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: error %s\n", FUNCTION_NAME, e.what());
    }

    return;
}

void GetOperatingSystemInfo(std::string& strInfo)
{
    std::string osinfo("");
    //taken from MSDN code
    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;
    char tmpbuf[80];
    memset(tmpbuf,0,80);

   // Try calling GetVersionEx using the OSVERSIONINFOEX structure.
   // If that fails, try using the OSVERSIONINFO structure.

   ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

   if( !(bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi)) )
   {
      osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
      if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) ) 
         return;
   }

   switch (osvi.dwPlatformId)
   {
      // Test for the Windows NT product family.
      case VER_PLATFORM_WIN32_NT:

      // Test for the specific product.
      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
         osinfo += "Microsoft Windows Server 2003 ";

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
         osinfo += "Microsoft Windows XP ";

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
         osinfo += "Microsoft Windows 2000 ";

      if ( osvi.dwMajorVersion <= 4 )
         osinfo += "Microsoft Windows NT ";

      // Test for specific product on Windows NT 4.0 SP6 and later.
      if( bOsVersionInfoEx )
      {
         // Test for the workstation type.
         if ( osvi.wProductType == VER_NT_WORKSTATION )
         {
            if( osvi.dwMajorVersion == 4 )
               osinfo +=  "Workstation 4.0 " ;
            else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
               osinfo +=  "Home Edition " ;
            else osinfo +=  "Professional " ;
         }
            
         // Test for the server type.
         else if ( osvi.wProductType == VER_NT_SERVER || 
                   osvi.wProductType == VER_NT_DOMAIN_CONTROLLER )
         {
            if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==2)
            {
               if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                  osinfo +=  "Datacenter Edition " ;
               else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  osinfo +=  "Enterprise Edition " ;
               else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                  osinfo +=  "Web Edition " ;
               else osinfo +=  "Standard Edition " ;
            }
            else if(osvi.dwMajorVersion==5 && osvi.dwMinorVersion==0)
            {
               if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                  osinfo +=  "Datacenter Server " ;
               else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  osinfo +=  "Advanced Server " ;
               else osinfo +=  "Server " ;
            }
            else  // Windows NT 4.0 
            {
               if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                  osinfo += "Server 4.0, Enterprise Edition " ;
               else osinfo +=  "Server 4.0 " ;
            }
         }
      }
      // Test for specific product on Windows NT 4.0 SP5 and earlier
      else  
      {
         HKEY hKey;
         char szProductType[BUFSIZE];
         DWORD dwBufLen=BUFSIZE;
         LONG lRet;

         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
            "SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet != ERROR_SUCCESS )
            return ;

         lRet = RegQueryValueEx( hKey, "ProductType", NULL, NULL,
            (LPBYTE) szProductType, &dwBufLen);
         if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
            return;

         RegCloseKey( hKey );

         if ( lstrcmpi( "WINNT", szProductType) == 0 )
            osinfo +=  "Workstation " ;
         if ( lstrcmpi( "LANMANNT", szProductType) == 0 )
            osinfo += "Server " ;
         if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
            osinfo += "Advanced Server " ;
         inm_sprintf_s(tmpbuf, ARRAYSIZE(tmpbuf), "%d.%d ", osvi.dwMajorVersion, osvi.dwMinorVersion);
         osinfo += tmpbuf;         
      }

      // Display service pack (if any) and build number.

      if( osvi.dwMajorVersion == 4 && 
          lstrcmpi( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
      { 
         HKEY hKey;
         LONG lRet;

         // Test for SP6 versus SP6a.
         lRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
  "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009",
            0, KEY_QUERY_VALUE, &hKey );
         if( lRet == ERROR_SUCCESS )
         {
            memset(tmpbuf, 0, sizeof(tmpbuf));
            inm_sprintf_s(tmpbuf, ARRAYSIZE(tmpbuf), "Service Pack 6a (Build %d)",
            osvi.dwBuildNumber & 0xFFFF );    
            osinfo += tmpbuf;        
         }
         else // Windows NT 4.0 prior to SP6a
         {
             memset(tmpbuf, 0, sizeof(tmpbuf));
             inm_sprintf_s(tmpbuf, ARRAYSIZE(tmpbuf), "%s (Build %d)\n",
               osvi.szCSDVersion,
               osvi.dwBuildNumber & 0xFFFF);
            osinfo += tmpbuf;
         }

         RegCloseKey( hKey );
      }
      else // not Windows NT 4.0 
      {
          memset(tmpbuf, 0, sizeof(tmpbuf));
          inm_sprintf_s(tmpbuf, ARRAYSIZE(tmpbuf), "%s (Build %d)\n",
            osvi.szCSDVersion,
            osvi.dwBuildNumber & 0xFFFF);
         osinfo += tmpbuf;
      }

      break;

      // Test for the Windows Me/98/95.
      case VER_PLATFORM_WIN32_WINDOWS:

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
      {
          osinfo += "Microsoft Windows 95 ";
          if (osvi.szCSDVersion[1]=='C' || osvi.szCSDVersion[1]=='B')
             osinfo += "OSR2 " ;
      } 

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
      {
          osinfo += "Microsoft Windows 98 ";
          if ( osvi.szCSDVersion[1] == 'A' )
             osinfo += "SE ";
      } 

      if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
      {
          osinfo += "Microsoft Windows Millennium Edition";
      } 
      break;

      case VER_PLATFORM_WIN32s:

      osinfo += "Microsoft Win32s";
      break;

    }
    strInfo = osinfo;
}

//utility routine to get value from registry
HRESULT GetValueFromRegistry( char  *pdwValue,  ULONG* pnChars , const char* pszsrcVol, const char* pszHostKey, 
                                      const char* pszKeyName)
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szsrcDriveLetter[MAX_PATH];
    char szDriveKey[MAX_PATH];

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetValuetFromRegistry()...\n" );
        if( (NULL == pszsrcVol) || (NULL == pszHostKey) || 
            (NULL == pszKeyName) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetValueFromRegistry()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, pszHostKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s(szsrcDriveLetter, ARRAYSIZE(szsrcDriveLetter), pszsrcVol);
        strupr  (szsrcDriveLetter);
        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), pszKeyName);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey), " ");
        AppendChar (szDriveKey, szsrcDriveLetter[0]); // E.G: Just "C" out of "C:\0"

        dwResult = cregkey.QueryStringValue( szDriveKey, pdwValue , pnChars);
        if( ERROR_SUCCESS != dwResult )
        {
            // Try and create the key as it might be the first time we tried to open it
            if (ERROR_SUCCESS != cregkey.SetStringValue(szDriveKey, " "))
            {
                hr = HRESULT_FROM_WIN32( dwResult );
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                break;
            }
            // Query the value again
            else
            {
                dwResult = cregkey.QueryStringValue( szDriveKey, pdwValue , pnChars);
                if( ERROR_SUCCESS != dwResult )
                {
                    hr = HRESULT_FROM_WIN32( dwResult );
                    DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                    DebugPrintf( hr, "FAILED cregkey.QueryValue()... hr = %08X\n", hr );
                    break;
                }
            }
        }
    
    }
    while( FALSE );

    return( hr );
}


// utility routine to set value in registry

HRESULT SetValueInRegistry( const char * value, const char * pszsrcVol, const char* pszHostKey, 
                                      const char* pszKeyName)
{
    HRESULT hr = S_OK;
    CRegKey cregkey;
    char szsrcDriveLetter[MAX_PATH];
    char szDriveKey[MAX_PATH];

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED SetValueInRegistry()...\n" );

        if( (NULL == pszsrcVol) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED SetVolumeCheckPointInRegistry()... hr = %08X\n", hr );
            break;
        }

        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE, pszHostKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s(szsrcDriveLetter, ARRAYSIZE(szsrcDriveLetter), pszsrcVol);
        strupr  (szsrcDriveLetter);

        inm_strcpy_s(szDriveKey, ARRAYSIZE(szDriveKey), pszKeyName);
        inm_strcat_s(szDriveKey, ARRAYSIZE(szDriveKey), " ");
        AppendChar (szDriveKey, szsrcDriveLetter[0]); // E.G: Just "C" out of "C:\"


        //dwResult = cregkey.SetDWORDValue(szDriveKey, value);
        dwResult = cregkey.SetStringValue(szDriveKey,value);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.SetValue()... hr = %08X\n", hr );
            break;
        }

    }
    while( FALSE );

    return( hr );
}

AutoInitWinsock::AutoInitWinsock( SCODE* pStatus )
{
    WSADATA WsaData;
    if( WSAStartup( MAKEWORD( 1, 0 ), &WsaData ) )
    {
        SCODE const dwError = GetLastError();
        DebugPrintf( "FAILED Couldn't init winsock, err %d\n", dwError );
        if( NULL != pStatus )
        {
            *pStatus = dwError;
        }
    }
}

AutoInitWinsock::~AutoInitWinsock()
{
    WSACleanup();
}


// --------------------------------------------------------------------------
// idles for waitTimeSeconds or returns if a WM_QUIT was received
// --------------------------------------------------------------------------
void AgentIdle(int waitTimeSeconds)
{
    DWORD rc = MsgWaitForMultipleObjects(0, NULL, FALSE, static_cast<unsigned>(waitTimeSeconds * 1000), QS_ALLINPUT);            
    if (WAIT_TIMEOUT == rc) {
        return;
    }
    if (WAIT_FAILED == rc) {
        DebugPrintf(SV_LOG_ERROR, "MsgWaitForMultipleObjects failed: %d\n", GetLastError());
        return;                       
    }                       

    if (WAIT_OBJECT_0 == rc) {
        MSG msg;             
        if (!GetMessage(&msg, NULL, 0, 0)) {                    
            return;            
        } 
        TranslateMessage(&msg);    
        DispatchMessage(&msg);                                                     
    }
}


SVERROR Update_identity_file(const char* priv_key_path, const char* identity_file) 
{
    return SVS_OK;
}
SVERROR convert_OpenSSH_SecSH(const char* ssh_key_gen_path,                            
                            const char* pubkey_file,
                            struct SSHOPTIONS &sshOptions) {
    DebugPrintf("Entering convert_OpenSSH_SecSH\n");
    SVERROR hr;
    std::stringstream ssSzKeygenCommand;
    std::stringstream ssSzSecSH_pubkey_file;
    FILE *pFile = NULL;
    ssSzSecSH_pubkey_file << pubkey_file << "_SecSH";
    ssSzKeygenCommand << ssh_key_gen_path << " -e -f \"" << ssSzSecSH_pubkey_file.str() << "\"";
    pFile = fopen(ssSzSecSH_pubkey_file.str().c_str(), "r");
    if(pFile != NULL) {
        char szstr[BUFSIZ];
        fread(szstr, BUFSIZ, 1, pFile);
        DebugPrintf("secsh public key is %s\n", szstr);
        inm_strncpy_s(sshOptions.pubKey_SecSH, ARRAYSIZE(sshOptions.pubKey_SecSH), szstr, BUFSIZ);
        SAFE_FCLOSE( pFile );
        return SVS_OK;
    }
    DebugPrintf(SV_LOG_DEBUG, "key-gen command to convert the key to SecSH format is %s\n", ssSzKeygenCommand.str().c_str());
    CProcess *pProcess = NULL;
    SV_ULONG exitCode = 0xDEADBEEF;
    hr = CProcess::Create(ssSzKeygenCommand.str().c_str(), &pProcess, ssSzSecSH_pubkey_file.str().c_str());
    if(hr.failed()) {
        
        if(pProcess) {
            pProcess->terminate();
            pProcess->getExitCode(&exitCode);
            SAFE_DELETE( pProcess );
        }
        DebugPrintf(SV_LOG_DEBUG, "key conversion to secsh format is failed with exit code %d\n", exitCode);
        return SVS_FALSE;
    }
    CMeteredBool<60,1> bStillWaiting;
    bool bProcessExited = false;
    if( pProcess )
    {
        while(!pProcess->hasExited())
                pProcess->waitForExit( 1*1000 );
        pProcess->terminate();
        pProcess->getExitCode(&exitCode);
        SAFE_DELETE( pProcess );
    }
    pFile = fopen(ssSzSecSH_pubkey_file.str().c_str(), "r");
    if(pFile == NULL) {
        return SVS_FALSE;
    }
    char szstr[BUFSIZ];
    int size = fread(szstr, 1, BUFSIZ, pFile);
    szstr[size - 1 ] = '\0';
    DebugPrintf(SV_LOG_DEBUG, "secsh public key is %s\n", szstr);
    inm_strncpy_s(sshOptions.pubKey_SecSH, ARRAYSIZE(sshOptions.pubKey_SecSH), szstr, 2048);
    SAFE_FCLOSE( pFile );        
    DebugPrintf(SV_LOG_DEBUG,"Pub key in sechsh format is pubKey_secSH_format %s\n", sshOptions.pubKey_SecSH);
    DebugPrintf(SV_LOG_DEBUG,"Exiting convert_OpenSSH_SecSH --- 2\n");
    return SVS_OK;
}
SVERROR generate_key_pair_impl (SV_ULONG JobId,
                                SV_ULONG JobConfigId, 
                                struct SSHOPTIONS & sshOptions, 
                                struct SSH_Configuration& ssh_config, 
                                const char* szCacheDirectory) 
{

    DebugPrintf("Entering generate_key_pair_impl\n");
    CProcess *pProcess = NULL;
    SVERROR hr;
    char szKeygenCommand[ BUFSIZ ];
    char pub_key_file[ SV_MAX_PATH ];
    char key_type[ 10 ];
    char key_prefix[ SV_MAX_PATH ];    
    SV_ULONG sshFlags = sshOptions.sshFlags;
    char szComment[ BUFSIZ ];
    char ssh_key_gen_path[ SV_MAX_PATH ];
    char ssh_keygen_version[ BUFSIZ ];
    char szidentity_file[ SV_MAX_PATH ];
    inm_strncpy_s(ssh_key_gen_path, ARRAYSIZE(ssh_key_gen_path), ssh_config.ssh_keygen_path, SV_MAX_PATH);
    inm_strncpy_s(ssh_keygen_version, ARRAYSIZE(ssh_keygen_version), ssh_config.sshd_prod_version, SV_MAX_PATH);
    inm_strncpy_s(szidentity_file, ARRAYSIZE(szidentity_file), ssh_config.identity_file, SV_MAX_PATH);
    
    
    inm_sprintf_s(key_prefix, ARRAYSIZE(key_prefix), "\"%s\\%s%d\"",
                                        szCacheDirectory,
                                        INMAGE_KEYFILE_PATTERN, 
                                        JobConfigId);    
    char file_key_prefix[SV_MAX_PATH];
    inm_sprintf_s(file_key_prefix, ARRAYSIZE(file_key_prefix), "%s\\%s%d",
                                        szCacheDirectory,
                                        INMAGE_KEYFILE_PATTERN, 
                                        JobConfigId);
    inm_sprintf_s(pub_key_file, ARRAYSIZE(pub_key_file), "%s\\%s%d.pub",
                                        szCacheDirectory,
                                        INMAGE_KEYFILE_PATTERN, 
                                        JobConfigId);
    FILE *pFile = NULL;
    pFile = fopen(pub_key_file, "r");

    if( pFile != NULL ) {
        DebugPrintf(SV_LOG_DEBUG, "Keys already generated for the JobConfigId\n");
        SAFE_FCLOSE( pFile );
        return SVS_OK;
    }
    if((sshFlags & SSHOPTIONS::KEY_TYPE_RSA_KEY) != 0) {
        inm_sprintf_s(key_type, ARRAYSIZE(key_type), "rsa");
    } else if((sshFlags & SSHOPTIONS::KEY_TYPE_DSA_KEY) != 0) {
        inm_sprintf_s(key_type, ARRAYSIZE(key_type), "dsa");
    }

    inm_sprintf_s(szComment, ARRAYSIZE(szComment), "The_PUBLIC_KEY_FOR_JOB_CONFIG_ID_%d", JobConfigId);
    if(strstr(ssh_keygen_version, "OpenSSH"))
        inm_sprintf_s(szKeygenCommand, ARRAYSIZE(szKeygenCommand), "%s -q -t %s -N \"\" -f %s -C %s", ssh_key_gen_path, key_type, key_prefix, szComment);
#ifndef SV_WINDOWS
    else
        inm_sprintf_s(szKeygenCommand, ARRAYSIZE(szKeygenCommand),  "%s -q -t %s -P -c %s %s", ssh_key_gen_path, key_type,  szComment, key_prefix);
#endif
    DebugPrintf(SV_LOG_DEBUG, "The encryption keys generateed for Job Id %d with cmd %s\n", JobConfigId, szKeygenCommand );
    hr = CProcess::Create( szKeygenCommand, &sshOptions.kProcess);
    if(hr.failed()) {
        DebugPrintf(SV_LOG_ERROR, "Failed to generate the keys with command %s for JobConfigId %d\n", szKeygenCommand, JobConfigId);
        return SVS_FALSE;    
    }
    
    if (sshOptions.kProcess)
    {
        int waitTime = 0;
        bool bProcessExited = sshOptions.kProcess->hasExited();
        while( !bProcessExited ) 
        {
            waitTime += 10;
            sshOptions.kProcess->waitForExit( 10*1000 );
            bProcessExited = sshOptions.kProcess->hasExited();
            if ( waitTime > 180 )
            {
                DebugPrintf(SV_LOG_ERROR, "Key generation process not exited even after 180 seconds\n");
                break;
            }            
        }
        SV_ULONG keygen_exitCode = 0xDEADBEEF;
        sshOptions.kProcess->getExitCode(&keygen_exitCode);
        sshOptions.kProcess->terminate();
        SAFE_DELETE( sshOptions.kProcess );
        DebugPrintf(SV_LOG_INFO, "ssh-keygen process exited with %d\n", keygen_exitCode );
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to Create tge ssh-keygen process\n");
        return SVS_FALSE;
    }
    return SVS_OK;
}

SVERROR fetch_keys(SV_ULONG JobConfigId, 
                    struct SSHOPTIONS &sshOptions, 
                    struct SSH_Configuration& ssh_config, 
                    const char* szCacheDirectory) 
{
    SVERROR hr;
    char pub_key_file[ SV_MAX_PATH ];
    char pub_key[ 2048 ];
    char key_prefix[ SV_MAX_PATH ];    
    char szidentity_file[ SV_MAX_PATH ];
    inm_strncpy_s(szidentity_file, ARRAYSIZE(szidentity_file), ssh_config.identity_file, SV_MAX_PATH);
    char ssh_key_gen_path[ SV_MAX_PATH ];
    char ssh_keygen_version[ BUFSIZ ];
    inm_strncpy_s(ssh_key_gen_path, ARRAYSIZE(ssh_key_gen_path), ssh_config.ssh_keygen_path, SV_MAX_PATH);
    inm_strncpy_s(ssh_keygen_version, ARRAYSIZE(ssh_keygen_version), ssh_config.sshd_prod_version, SV_MAX_PATH);
    
    inm_sprintf_s(key_prefix, ARRAYSIZE(key_prefix), "%s\\%s%d",
                                        szCacheDirectory,
                                        INMAGE_KEYFILE_PATTERN, 
                                        JobConfigId);    
    FILE *pFile = NULL;    
    inm_sprintf_s(pub_key_file, ARRAYSIZE(pub_key_file), "%s.pub", key_prefix);
    pFile = fopen(pub_key_file, "r");
    if( pFile == NULL ) {
        DebugPrintf(SV_LOG_ERROR, "Failed to read the public keys file");
        return SVS_FALSE;
    }
#ifndef SV_WINDOWS    
    if(strstr(ssh_keygen_version, "OpenSSH")) {
        hr = convert_OpenSSH_SecSH(ssh_key_gen_path, pub_key_file, sshOptions);
        if(hr == SVS_FALSE)
            DebugPrintf(SV_LOG_ERROR, "Unable to convert the public key from OpenSSH to SecSH format\n");
    }
#endif
    int n = fread(pub_key, 1, 2048, pFile);
    pub_key[ n - 1 ] = '\0';
    SAFE_FCLOSE( pFile );    
    if(strstr(ssh_keygen_version, "OpenSSH") ) {
        DebugPrintf("4");
        inm_strncpy_s(sshOptions.pubKey_OpenSSH, ARRAYSIZE(sshOptions.pubKey_OpenSSH), pub_key, 2048);
    }
#ifndef SV_WINDOWS
    else {
        Update_identity_file(key_prefix, szidentity_file);
        inm_strncpy_s(sshOptions.pubKey_SecSH, ARRAYSIZE(sshOptions.pubKey_SecSH), pub_key, 2048);
    }
#endif
    DebugPrintf(SV_LOG_DEBUG, "generate_key_pair_impl openssh public key is %s\n", sshOptions.pubKey_OpenSSH);
    DebugPrintf(SV_LOG_DEBUG, "generate_key_pair_impl SecSH public key is %s\n", sshOptions.pubKey_SecSH);
    return SVS_OK;    

}

SVERROR update_authorized_keys_impl(const char* szAuthKeys_file,                                     
                                    const struct SSHOPTIONS& sshOptions)
{
    return SVS_OK;
}

SVERROR remove_pubkey_auth_keys(const char *sedPath, const char *pszuser, char* authkeys_file, SV_ULONG JobConfigId) {
    return SVS_OK;
}

SVERROR update_authorization_file_impl(SV_ULONG JobConfigID,
                                    const char* name1,
                                    const struct SSHOPTIONS& sshoptions,                                    
                                    const char*name2) {
    return SVS_OK;
}
SVERROR convert_SecSH_OpenSSH(char * name1, 
                            char* name2, 
                            const char* name3, 
                            SV_ULONG i) 
{ 
    return SVS_OK;
}
SVERROR cleanup_server_keys_impl(SSH_Configuration sshconfig, const char* name, SV_ULONG JobId, SV_ULONG JobConfigId) 
{
    return remove_pubkey_auth_keys(sshconfig.szSedPath, sshconfig.szUserName, sshconfig.auth_keys_file, JobConfigId);

}
SVERROR cleanup_client_keys_impl(SSH_Configuration sshconfig, const char*name, SV_ULONG jobconfig)
{
    return SVS_OK;
}

SVERROR create_ssh_tunnel_impl(const char* szTunnel, CProcess **pProcess, const char* szLogFileName, SV_ULONG* exitCode) 
{
    SVERROR hr = SVS_OK;
    hr = CProcess::Create(szTunnel, pProcess, szLogFileName);            
    if(hr.failed()) {    
        if(*pProcess) {
            (*pProcess)->terminate();
            (*pProcess)->getExitCode(exitCode);
            DebugPrintf(SV_LOG_ERROR, "Tunnel Creation Failed with exit Code %d\n", *exitCode);
        }
        return SVS_FALSE;
    }
    Sleep(10 *1000);    
    return SVS_OK;
}

SVERROR delete_encryption_keys_impl( const char*path, SV_ULONG configId) {
    std::stringstream SSFileName;
    SSFileName << path << "\\.inmage_key_" << configId;
    remove(SSFileName.str().c_str());
    SSFileName << ".pub";
    remove(SSFileName.str().c_str());
    return SVS_OK;
}
SV_ULONG getFreePort(SV_ULONG startPort,SV_ULONG endPort)
{    
    ACE_SOCK_Acceptor acceptor;
    SV_ULONG localPort = -255;
    for( int i = (int )startPort; i <= (int)endPort; i++)
    {
        ACE_INET_Addr port_to_listen (i, "HAStatus");
        if (acceptor.open (port_to_listen , 1) == -1)
            continue;
        else
        {
            localPort = i;
            break;
        }
    }
    if( localPort == -255 )
        DebugPrintf(SV_LOG_ERROR, "FAILED to get local port to forward the ssh traffic");
    return (SV_ULONG)localPort;
}
SVERROR waitTillTunnelEstablishes(int local_port)
{
    //----------------------
    // Initialize Winsock
    WSADATA wsaData;
    int time_out = 120;
    int wait_sec = 5;
    int retries = 0;
    SVERROR ret = SVS_OK;
    char known_host_file[SV_MAX_PATH];
    ofstream pFile;
    SVERROR hr = SVS_OK;
    SV_ULONG cChars;
    CRegKey cregkey;
    DWORD dwResult = ERROR_SUCCESS;
    char szCygwinHomePath[ SV_MAX_PATH+1 ];
    cChars = sizeof( szCygwinHomePath );
    dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,"Software\\Cygnus Solutions\\Cygwin\\mounts v2\\/home" );
    if( ERROR_SUCCESS != dwResult )
    {
        hr = HRESULT_FROM_WIN32( dwResult );
        DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf(SV_LOG_ERROR, "FAILED cregkey.Open()... hr = %08X\n", hr );
        DebugPrintf(SV_LOG_ERROR,"waitTillTunnelEstablishes:Failed to open CygwinHomePath regkey\n");
        cregkey.Close();
        
    } else {        
        dwResult = cregkey.QueryStringValue( _T("native"), szCygwinHomePath, &cChars );
        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf(SV_LOG_DEBUG, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf(SV_LOG_ERROR,"waitTillTunnelEstablishes:Failed to get CygwinHomePath\n");
            cregkey.Close();
            
        }
        else
        {
            inm_sprintf_s(known_host_file, ARRAYSIZE(known_host_file), "%s\\%s\\.ssh\\known_hosts", szCygwinHomePath, ssh_config.szUserName);
            pFile.open(known_host_file,ios::trunc); 
            pFile.close();
        }

    }

    
    
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != NO_ERROR)
    DebugPrintf("Error at WSAStartup()\n");

    //----------------------
    // Create a SOCKET for connecting to server
    SOCKET ConnectSocket;
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        DebugPrintf(SV_LOG_ERROR, "Error at socket(): %ld\n", WSAGetLastError());
        WSACleanup();
        return SVS_FALSE;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    sockaddr_in clientService; 
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    clientService.sin_port = htons( local_port );
    do
    {
        //----------------------
        // Connect to server.
        if ( connect( ConnectSocket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) 
        {
            retries ++;
            if(wait_sec * retries  >  time_out)
            {
                    DebugPrintf(SV_LOG_ERROR, "Local port failed to enter into listening state even after %d seconds\n", time_out);
                    ret = SVS_FALSE;
                    break;
            }
            Sleep(wait_sec * 1000);
        }
        else
        {
                DebugPrintf(SV_LOG_DEBUG, "Local port went to listening mode after %d seconds..\n", retries * wait_sec);
                DebugPrintf(SV_LOG_INFO, "Tunnel established successfully... for port %d\n", local_port);    
                ret = SVS_OK;
                break;
        }

    }while(1);
    closesocket(ConnectSocket);
    WSACleanup();
    cregkey.Close();
    return ret;

}

void PostFailOverJobStatusToCX(char* cxip, SV_ULONG httpPort, SV_ULONG processId, SV_ULONG jobId,bool https)
{
    CRegKey cregkey;
    DWORD result;
    string failOverJobStatusFileName  ;
    result = cregkey.Open(HKEY_LOCAL_MACHINE, "SOFTWARE\\SV Systems\\VxAgent");

    if (ERROR_SUCCESS != result)
        return;

    char path[ BUFSIZ ];
    DWORD dwCount = sizeof( path );
    result = cregkey.QueryStringValue("InstallDirectory", path, &dwCount);
    cregkey.Close();
    if (ERROR_SUCCESS != result) 
    {
        return;
    }
    failOverJobStatusFileName = path;
    failOverJobStatusFileName += '\\';
    failOverJobStatusFileName += Application_Data_Folder;
    failOverJobStatusFileName += '\\';
    failOverJobStatusFileName += "Failover_" ;
    char str[10];

    failOverJobStatusFileName += ltoa( processId, str,10 );
    DebugPrintf( SV_LOG_INFO, "Fail Over script's o/p path name %s\n", failOverJobStatusFileName.c_str());
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    hFind = FindFirstFile( failOverJobStatusFileName.c_str(), &FindFileData );
    char inBuffer[BUFSIZ];
    if( hFind !=  INVALID_HANDLE_VALUE )
    {
        FindClose( hFind );
        
        DWORD nBytesToRead      = BUFSIZ;
        DWORD dwBytesRead       = 0;
        DWORD dwError  = 0;
        BOOL bResult   = FALSE;
        // PR#10815: Long Path support
        hFind = SVCreateFile( failOverJobStatusFileName.c_str(), 
                                GENERIC_READ, 
                                FILE_SHARE_READ, 
                                NULL, 
                                OPEN_EXISTING, 
                                FILE_ATTRIBUTE_NORMAL, 
                                NULL );
        if( hFind != INVALID_HANDLE_VALUE )
        {
            bResult = ReadFile(hFind,
                        inBuffer,
                        nBytesToRead,
                        &dwBytesRead,
                        NULL); 
            dwError = GetLastError();
            
            if( !bResult )
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to read the file %s with error %d\n", failOverJobStatusFileName.c_str(), dwError );
                return ;
            }
            else if( dwBytesRead > 0 )
            {
                inBuffer[ dwBytesRead ] = '\0';
                DebugPrintf(SV_LOG_DEBUG, "file %s and its contents %s\n", failOverJobStatusFileName.c_str(), inBuffer );
            }
            CloseHandle( hFind );
            remove(failOverJobStatusFileName.c_str());
            SVERROR hr = SVS_OK;
            char szUrl[SV_INTERNET_MAX_URL_LENGTH];
            inm_sprintf_s(szUrl, ARRAYSIZE(szUrl), "/failover_status.php?JobId=%ld", jobId);
            char postBuffer[ BUFSIZ ];
            memset( postBuffer, '\0', BUFSIZ );
            inm_sprintf_s(postBuffer, ARRAYSIZE(postBuffer), "&JobType=%s&ScripExitStatus=%s", "FailOver", inBuffer);
            DebugPrintf("Update Failover script's status url is details url %s\n", szUrl);
    
            hr = postToCx(cxip, httpPort, szUrl, postBuffer,NULL,https);
            
            if ( hr.failed() )
            {
                DebugPrintf(SV_LOG_ERROR, "PostToServer Failed. Unable to update failover script status for job id %ld\n", jobId);
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "Successfully updated the fail over job status to CX for job id %ld\n", jobId);
            }
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "CreateFile for %s got invalid handle\n", failOverJobStatusFileName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "fail over script file %s not present \n", failOverJobStatusFileName.c_str());
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
}

SVERROR SetFXRegStringValue(const char * pszValueName, const char * pszValue)
{
    HRESULT hr = S_OK;
    CRegKey regKey;

    if(pszValueName == NULL || pszValue == NULL)
    {
        DebugPrintf(SV_LOG_ERROR,"ValueName or value should not be NULL.\n",hr);
        return SVE_FAIL;
    }

    hr = regKey.Open(HKEY_LOCAL_MACHINE, SV_FILEREP_AGENT_VALUE_NAME);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.Open(). Error %08X\n",hr);
        return SVE_FAIL;
    }
    hr = regKey.SetStringValue(pszValueName,pszValue);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.SetStringValue() error %08X. ValueName %s, value %s",pszValueName,pszValue);
        return SVE_FAIL;
    }
    return SVS_OK;
}

SVERROR GetFXRegStringValue(const char* pszValueName, std::string & strValue)
{
    HRESULT hr = S_OK;
    char szValue[2048];
    CRegKey regKey;

    if(pszValueName == NULL)
    {
        DebugPrintf(SV_LOG_ERROR,"ValueName should not be NULL.\n",hr);
        return SVE_FAIL;
    }

    hr = regKey.Open(HKEY_LOCAL_MACHINE, SV_FILEREP_AGENT_VALUE_NAME);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.Open(). Error %08X\n",hr);
        return SVE_FAIL;
    }
    DWORD dwCount = sizeof(szValue);
    hr = regKey.QueryStringValue(pszValueName,szValue,&dwCount);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.QueryStringValue() error %08X. ValueName %s\n",hr,pszValueName);
        return SVE_FAIL;
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG,"Successfuly read the keyValue %s=%s\n",pszValueName,szValue);
        strValue = szValue;
    }
    return SVS_OK;
}


//To set fxupgrade key value to 0
#ifdef SV_WINDOWS
SVERROR SetFXRegDWordValue(const char * pszValueName, DWORD pszValue)
{
    HRESULT hr = S_OK;
    CRegKey regKey;

    if(pszValueName == NULL )
    {
        DebugPrintf(SV_LOG_ERROR,"ValueName or value should not be NULL.\n",hr);
        return SVE_FAIL;
    }

    hr = regKey.Open(HKEY_LOCAL_MACHINE, SV_FILEREP_AGENT_VALUE_NAME);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.Open(). Error %08X\n",hr);
        return SVE_FAIL;
    }
    hr = regKey.SetDWORDValue(pszValueName,pszValue);
    if(ERROR_SUCCESS != hr)
    {
        hr = HRESULT_FROM_WIN32(hr);
        DebugPrintf(SV_LOG_ERROR,"Failed regKey.SetStringValue() error %08X. ValueName %s, value %s",pszValueName,pszValue);
        return SVE_FAIL;
    }
    return SVS_OK;
}
#endif
LONG GetRegMultiStringValues(CRegKey & key, std::string value, std::vector<std::string> & stringArray)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;

    ULONG ulBuffLength = 0;
    LONG lResult = key.QueryMultiStringValue(value.c_str(),NULL,&ulBuffLength);
    if(ulBuffLength == 0 || lResult != ERROR_SUCCESS)
    {
        DebugPrintf(SV_LOG_DEBUG, "Failed to read %s from registry. Error %ld\n",value.c_str(), lResult);
    }
    else
    {
        char *databuff = new (std::nothrow) char[ulBuffLength];
        if(NULL == databuff)
        {
            DebugPrintf(SV_LOG_ERROR,"Memory allocation error\n");
            lResult = ERROR_OUTOFMEMORY;
            return lResult;
        }
        lResult = key.QueryMultiStringValue(value.c_str(),databuff,&ulBuffLength);
        if(ERROR_SUCCESS != lResult)
        {
            DebugPrintf(SV_LOG_DEBUG,"Failed to read %s from registry. Error %ld\n",value.c_str(),lResult);
        }
        else
        {
            char * strPos = databuff;
            while(*strPos != '\0')
            {
                stringArray.push_back(strPos);
                strPos += strlen(strPos);
                strPos++;
            }
        }
        delete [] databuff;
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return lResult;
}

SVERROR GetPhysicalIpAndSubnetMasksFromRegistry(std::string interface_guid,
                                 std::vector<std::string> & ips,
                                 std::vector<std::string> & subnetmasks)
{
    DebugPrintf(SV_LOG_DEBUG, "Registry lookup for the NIC : %s\n",interface_guid.c_str());
    SVERROR retStatus = SVS_OK;

    std::string strTcpInterfaceRegKey = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    strTcpInterfaceRegKey += interface_guid;

    CRegKey key;

    const char key_dhcp_enabled[] = "EnableDHCP";
    const char key_dhcp_ip_addr[] = "DhcpIPAddress";
    const char key_dhcp_subnet[]  = "DhcpSubnetMask";
    const char key_ip_addr[]      = "IPAddress";
    const char key_subnet_mask[]  = "SubnetMask";

    do
    {
        LONG lResult = key.Open(HKEY_LOCAL_MACHINE,strTcpInterfaceRegKey.c_str(),KEY_READ);
        if(ERROR_SUCCESS != lResult)
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to open the reg key %s. Error %ld\n",
                strTcpInterfaceRegKey.c_str(),lResult);
            retStatus = SVE_FAIL;
            break;
        }
        
        DWORD dwDHCPEnabled = 0;
        lResult = key.QueryDWORDValue(key_dhcp_enabled,dwDHCPEnabled);
        if(ERROR_SUCCESS != lResult)
        {
            DebugPrintf(SV_LOG_ERROR,"Failed to query reg value %s. Error %ld\n",key_dhcp_enabled,lResult);
            key.Close();
            retStatus = SVE_FAIL;
            break;
        }
        if(dwDHCPEnabled)
        {
            char szValue[512] = { 0 };
            ULONG nChars = 512;
            lResult = key.QueryStringValue(key_dhcp_ip_addr,szValue,&nChars);
            if(ERROR_FILE_NOT_FOUND == lResult)
            {
                DebugPrintf(SV_LOG_DEBUG,"Failed to query reg value %s. Error %ld.\
                    \nQuerying interface may be a virtual interface.(or)\
                    \nNo physical IPs available for this interface.\n",key_dhcp_ip_addr,lResult);
                key.Close();
                break;
            }
            else if(ERROR_SUCCESS != lResult)
            {
                DebugPrintf(SV_LOG_ERROR,"Failed to query reg value %s. Error %ld\n",key_dhcp_ip_addr,lResult);
                key.Close();
                retStatus = SVE_FAIL;
                break;
            }
            ips.push_back(szValue);

            nChars = 512;
            lResult = key.QueryStringValue(key_dhcp_subnet,szValue,&nChars);
            if(ERROR_SUCCESS != lResult)
            {
                DebugPrintf(SV_LOG_ERROR,"Failed to query reg value %s. Error %ld\n",key_dhcp_subnet,lResult);
                key.Close();
                retStatus = SVE_FAIL;
                break;
            }
            subnetmasks.push_back(szValue);
        }
        else
        {
            lResult = GetRegMultiStringValues(key, key_ip_addr, ips);
            if(ERROR_FILE_NOT_FOUND == lResult)
            {
                DebugPrintf(SV_LOG_DEBUG,"Failed to query reg value %s. Error %ld.\
                                         \nQuerying interface may be a virtual interface. (or)\
                                         \nNo physical IPs available for this interface.\n",key_ip_addr,lResult);
                key.Close();
                break;
            }
            else if(ERROR_SUCCESS != lResult)
            {
                DebugPrintf(SV_LOG_ERROR,"Failed to query reg value %s. Error %ld\n",key_ip_addr,lResult);
                key.Close();
                retStatus = SVE_FAIL;
                break;
            }
            lResult = GetRegMultiStringValues(key, key_subnet_mask, subnetmasks);
            if(ERROR_SUCCESS != lResult)
            {
                DebugPrintf(SV_LOG_ERROR,"Failed to query reg value %s. Error %ld\n",key_subnet_mask,lResult);
                key.Close();
                retStatus = SVE_FAIL;
                break;
            }
        }

        key.Close();

    }while(false);

    return retStatus;
}

///
/// Retrieve the FTP configuration from the registry. While most configuration changes
/// are kept with the SV box, the SV box may not always be reachable. For this scenario, we
/// persistent configuration changes in the registry. Upon initial startup of the agent, we
/// begin with the registry settings, and then update these settings with changes from the SV
/// server
///
HRESULT GetFTPParamsFromRegistry( const char *pszSVRootKey,
                                 SV_HOST_AGENT_PARAMS *pSV_HOST_AGENT_PARAMS )
{
    HRESULT hr = S_OK;
    char szValue[ 256 ];
    CRegKey cregkey;    
    DWORD dwTmpPort;
    DWORD dwCount = 0;
    SVERROR svhr = SVS_OK;
    char *FTPPassword = NULL;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetAgentParamsFromRegistry()...\n" );

        if( ( NULL == pszSVRootKey ) ||
            ( NULL == pSV_HOST_AGENT_PARAMS ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetAgentParamsFromRegistry()... hr = %08X\n", hr );
            break;
        }
        USES_CONVERSION;
        //
        // 
        //
        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszSVRootKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }   

        // 
        // Ashish - 05/01/04 - added registry calles to extract FTP information
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_FTP_HOST_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPHost, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPHost), szValue );          
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_FTP_HOST_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPHost, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPHost), pSV_HOST_AGENT_PARAMS->szSVServerName );            
        }
        //FTP User Name
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_FTP_USER_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPUserName, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPUserName), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_FTP_USER_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPUserName, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPUserName), SV_FTP_LOGIN );         
        }

        //FTP Password
        // Bugfix for bug26 - get the FTPPassword from config instead of
        // from local registry
        //    
        FTPPassword = new char[LENGTH_TRANSPORT_PASSWORD + 1];   
        if( NULL == FTPPassword )
        {
            hr = E_OUTOFMEMORY;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED new()... hr = %08X\n", hr );

            // use the default FTP password
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPPassword, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPPassword), SV_FTP_PASSWORD ); 
            break;
        }

        inm_strncpy_s(FTPPassword, LENGTH_TRANSPORT_PASSWORD + 1, SV_FTP_PASSWORD, LENGTH_TRANSPORT_PASSWORD); 
        FTPPassword[LENGTH_TRANSPORT_PASSWORD] = '\0';
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVFTPPassword, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVFTPPassword), FTPPassword );
    
        //FTP Port      
        dwResult = cregkey.QueryDWORDValue( SV_FTP_PORT_VALUE_NAME, dwTmpPort );
        if( ERROR_SUCCESS == dwResult )
        {
            pSV_HOST_AGENT_PARAMS->SVFTPPort = static_cast<INTERNET_PORT>( dwTmpPort );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_FTP_PORT_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );            
            pSV_HOST_AGENT_PARAMS->SVFTPPort = SV_FTP_PORT;       
        }

        //FTP URL
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_FTP_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetFTPInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetFTPInfoURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_FTP_URL_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetFTPInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetFTPInfoURL), SV_FTP_URL );         
        }

        // Bug fix for #23
        //
        // SecureModes URL
               
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_SECURE_MODES_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetSecureModesInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetSecureModesInfoURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_SECURE_MODES_URL_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetSecureModesInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetSecureModesInfoURL), SV_SECURE_MODES_URL );         
        }

        //SSL Key Path
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_SSL_KEY_PATH, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVSSLKeyPath, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVSSLKeyPath), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_SSL_KEY_PATH, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVSSLKeyPath, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVSSLKeyPath), SV_SSL_DEFAULT_SSLKEYPATH );         
        }
        
        //SSL Cert Path
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_SSL_CERT_PATH, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVSSLCertificatePath, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVSSLCertificatePath), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {            
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_SSL_CERT_PATH, HRESULT_FROM_WIN32( GetLastError() ) );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVSSLCertificatePath, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVSSLCertificatePath), SV_SSL_DEFAULT_SSLCERTPATH );         
        }


    }
    while( FALSE );
    delete [] FTPPassword;

    cregkey.Close();

    return( hr );
}

LONG GetAllPhysicalIPsOfSystemEx(std::vector<std::string> & physicalIPs)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pAdapterAddress = NULL;
    ULONG ulBuffLength = 0;
    ULONG flags = GAA_FLAG_INCLUDE_ALL_INTERFACES;
    ULONG family = AF_INET;

    LONG lReturn = NO_ERROR;
    lReturn = GetAdaptersAddresses(family, flags, NULL, pAdapterAddresses, &ulBuffLength);
    if (ERROR_BUFFER_OVERFLOW != lReturn) 
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to get the adapters information buffer size. Error %ld\n", lReturn);
        return lReturn;
    }
    pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(ulBuffLength);
    if (NULL == pAdapterAddresses)
    {
        DebugPrintf(SV_LOG_FATAL, "Out of memory\n");
        lReturn = ERROR_OUTOFMEMORY;
        return lReturn;
    }
    lReturn = GetAdaptersAddresses(family, flags, NULL, pAdapterAddresses, &ulBuffLength);
    if (NO_ERROR == lReturn)
    {
        pAdapterAddress = pAdapterAddresses;
        while (pAdapterAddress)
        {
            stringstream mac;
            vector<string> ips, placeholder;
            DebugPrintf(SV_LOG_DEBUG, "AdapterInfo : index:%d name:%s description:%S\n", pAdapterAddress->IfIndex, pAdapterAddress->AdapterName, pAdapterAddress->Description);

            if (GetPhysicalIpAndSubnetMasksFromRegistry(pAdapterAddress->AdapterName, ips, placeholder) == SVS_OK)
            {
                physicalIPs.insert(physicalIPs.end(), ips.begin(), ips.end());
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "Failed to read IP address from registry for adapter %s.\n", pAdapterAddress->AdapterName);
            }
            pAdapterAddress = pAdapterAddress->Next;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to get the adapters information. Error %ld\n", lReturn);
    }
    free(pAdapterAddresses);
    
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return lReturn;
}

LONG GetAllPhysicalIPsOfSystem(std::vector<std::string> & physicalIPs)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    // cache the discovery info as the NIC changes are not that often
    // this avoids repetitive discovery in short duration
    static std::vector<std::string> s_physicalAddresses;
    static steady_clock::time_point s_lastDiscoveryTimepoint = steady_clock::time_point::min();
    if (steady_clock::now() < s_lastDiscoveryTimepoint + minutes(2))
    {
        physicalIPs = s_physicalAddresses;
        DebugPrintf(SV_LOG_DEBUG, "%s: returning cached IP address discovery info.\n", FUNCTION_NAME);
        DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
        return NO_ERROR;
    }

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG ulBuffLength = 0;

    LONG lReturn = NO_ERROR;
    lReturn = GetAdaptersInfo(pAdapterInfo,&ulBuffLength);
    if( ERROR_BUFFER_OVERFLOW != lReturn &&
        NO_ERROR != lReturn)
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to get the adapters information buffer size. Error %ld\n",lReturn);
        return lReturn;
    }
    pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulBuffLength);
    if(NULL == pAdapterInfo)
    {
        DebugPrintf(SV_LOG_FATAL,"Out of memory\n");
        lReturn = ERROR_OUTOFMEMORY;
        return lReturn;
    }
    lReturn = GetAdaptersInfo(pAdapterInfo,&ulBuffLength);
    if(NO_ERROR == lReturn)
    {
        pAdapter = pAdapterInfo;
        while(pAdapter)
        {
            stringstream mac;
            vector<string> ips, placeholder;
            DebugPrintf(SV_LOG_DEBUG, "AdapterInfo : index:%d name:%s description:%s\n", pAdapter->Index, pAdapter->AdapterName, pAdapter->Description);

            if(GetPhysicalIpAndSubnetMasksFromRegistry(pAdapter->AdapterName,ips,placeholder) == SVS_OK)
            {
                physicalIPs.insert(physicalIPs.end(),ips.begin(),ips.end());
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG,"Failed to read IP address from registry.\n");
            }
            pAdapter = pAdapter->Next;
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to get the adapters information. Error %ld\n",lReturn);
    }
    free(pAdapterInfo);

    // if no IP address found, try the new interface
    if (physicalIPs.empty())
        lReturn = GetAllPhysicalIPsOfSystemEx(physicalIPs);

    s_physicalAddresses = physicalIPs;
    s_lastDiscoveryTimepoint = steady_clock::now();
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return lReturn;
}


SVERROR ReadAgentConfiguration( SV_HOST_AGENT_PARAMS *pConfiguration, char const* pszConfigPathname, SV_HOST_AGENT_TYPE AgentType )
{
    SVERROR rc;
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    const char* SV_ROOT_KEY = "SOFTWARE\\SV Systems";
    rc = GetAgentParamsFromRegistry( SV_ROOT_KEY, pConfiguration, AgentType );
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return( rc );
}

///
/// Retrieve the host agent configuration from the registry. While most configuration changes
/// are kept with the SV box, the SV box may not always be reachable. For this scenario, we
/// persistent configuration changes in the registry. Upon initial startup of the agent, we
/// begin with the registry settings, and then update these settings with changes from the SV
/// server
///
HRESULT GetAgentParamsFromRegistry( const char *pszSVRootKey,
                                   SV_HOST_AGENT_PARAMS *pSV_HOST_AGENT_PARAMS, SV_HOST_AGENT_TYPE AgentType )
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    HRESULT hr = S_OK;
    char szValue[ 4*MAX_PATH ];
    CRegKey cregkey;
    DWORD httpsvalue = 0;

    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED GetAgentParamsFromRegistry()...\n" );

        if( ( NULL == pszSVRootKey ) ||
            ( NULL == pSV_HOST_AGENT_PARAMS ) )
        {
            hr = E_INVALIDARG;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED GetAgentParamsFromRegistry()... hr = %08X\n", hr );
            break;
        }

        ZeroMemory( pSV_HOST_AGENT_PARAMS, sizeof( *pSV_HOST_AGENT_PARAMS ) );

        USES_CONVERSION;
        //
        // 
        //
        DWORD dwResult = ERROR_SUCCESS;
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pszSVRootKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.Open()... hr = %08X\n", hr );
            break;
        }


        DWORD dwCount = sizeof( szValue );
        
        dwResult = cregkey.QueryStringValue( SV_SERVER_NAME_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_SERVER_NAME_VALUE_NAME, hr );
            break;
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVServerName, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVServerName), szValue );
        dwCount = sizeof( szValue );

        //Akshay - Server HTTP Port defaults to 80
        dwResult = cregkey.QueryDWORDValue( SV_SERVER_HTTP_PORT_VALUE_NAME, pSV_HOST_AGENT_PARAMS->ServerHttpPort);
        if( ERROR_SUCCESS != dwResult )
        {
            pSV_HOST_AGENT_PARAMS->ServerHttpPort = INTERNET_DEFAULT_HTTP_PORT;

            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_SERVER_HTTP_PORT_VALUE_NAME, hr );
            break;
        }
        else if(pSV_HOST_AGENT_PARAMS->ServerHttpPort > 65535 || pSV_HOST_AGENT_PARAMS->ServerHttpPort <= 0 )
        {
            pSV_HOST_AGENT_PARAMS->ServerHttpPort = INTERNET_DEFAULT_HTTP_PORT;
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "Port is out of range \n");
            break;

        }
        //Reading Https Reg Key value 
            
        dwResult = cregkey.QueryDWORDValue( "Https",httpsvalue);

        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "ERROR failed to Read Https Key\n" );
        }
        else
        {
            DebugPrintf( "Successfully Read Https Key %d\n",httpsvalue );
            if(httpsvalue==1)
            {
                pSV_HOST_AGENT_PARAMS->Https = true;
                DebugPrintf( "Successfully Read Https Key  and value %d \n",pSV_HOST_AGENT_PARAMS->Https );

            }
            else
            {
                pSV_HOST_AGENT_PARAMS->Https = false;
                DebugPrintf( "Https Key value is not set value=%d\n",pSV_HOST_AGENT_PARAMS->Https );
            
            }
        }
        

        if( AgentType  == SV_AGENT_FILE_REPLICATION) {
            DWORD value = 0;
            dwResult = cregkey.QueryDWORDValue( SV_DEBUG_LOG_LEVEL_VALUE_NAME, value);
            if( ERROR_SUCCESS != dwResult )
                SetLogLevel(SV_LOG_DISABLE);
            else
                SetLogLevel(value);
        }
        // Throttle Bootstrap
        dwResult = cregkey.QueryStringValue( THROTTLE_BOOTSTRAP_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleBootstrapURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleBootstrapURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue(), using default. key: %s; hr = %08X\n", THROTTLE_BOOTSTRAP_URL, hr );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleBootstrapURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleBootstrapURL), "/throttle_bootstrap.php" );
            dwCount = sizeof("/throttle_bootstrap.php");
        }

        // Throttle Sentinel
        dwResult = cregkey.QueryStringValue( THROTTLE_SENTINEL_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleSentinelURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleSentinelURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue(), using default. key: %s; hr = %08X\n", THROTTLE_SENTINEL_URL, hr );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleSentinelURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleSentinelURL), "/throttle_sentinel.php" );
            dwCount = sizeof("/throttle_sentinel.php");
        }

        // Throttle OA
        dwResult = cregkey.QueryStringValue( THROTTLE_OUTPOST_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleOutpostURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleOutpostURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", THROTTLE_OUTPOST_URL, hr );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleOutpostURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleOutpostURL), "/throttle_outpost.php" );
            dwCount = sizeof( "/throttle_outpost.php" );
        }

        //Throttle url added 2/2/2006 
        
        dwResult = cregkey.QueryStringValue( THROTTLE_AGENT_URL, szValue, &dwCount );
        if( ERROR_SUCCESS == dwResult )
        {
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleURL), szValue );
            dwCount = sizeof( szValue );
        }
        else 
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", THROTTLE_AGENT_URL, hr );
            inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szThrottleURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szThrottleURL), "/throttle_agent.php" );
            dwCount = sizeof( "/throttle_agent.php" );
        }


        if( SV_AGENT_FILE_REPLICATION == AgentType )
        {
            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "FileReplicationAgent" );
        }
        else
        {
            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "VxAgent" );
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVHostAgentType, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVHostAgentType), szValue );
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey), pszSVRootKey );
        inm_strcat_s( pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey), "\\" );
        inm_strcat_s( pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey), 
            pSV_HOST_AGENT_PARAMS->szSVHostAgentType );

        cregkey.Close();
        dwResult = cregkey.Open( HKEY_LOCAL_MACHINE,
            pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr,
                "FAILED cregkey.Open()... key: %s; hr = %08X\n",
                pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey,
                hr );
            break;
        }

        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_REGISTER_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_REGISTER_URL_VALUE_NAME, hr );
            break;
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVRegisterURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVRegisterURL), szValue );

        //Reading IsFxUpgraded Key for Fx _upgraded case start

        dwResult = cregkey.QueryDWORDValue( SV_FX_UPGRADED, pSV_HOST_AGENT_PARAMS->Fx_Upgraded );

        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_FX_UPGRADED, hr );
        }
        else
        {
            DebugPrintf( " Successfully Read SV_FX_UPGRADED value = %d \n", pSV_HOST_AGENT_PARAMS->Fx_Upgraded );    
        }
        //Reading IsFxUpgraded Key for Fx _upgraded case end
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_UNREGISTER_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_UNREGISTER_URL_VALUE_NAME, hr );
            break;
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szUnregisterURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUnregisterURL), szValue );

        dwCount = sizeof( szValue );

        dwResult = cregkey.QueryStringValue( SV_PROTECTED_DRIVES_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_PROTECTED_DRIVES_URL_VALUE_NAME, hr );
            break;
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetProtectedDrivesURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetProtectedDrivesURL), szValue );

        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_TARGET_DRIVES_URL_VALUE_NAME, szValue, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X\n", SV_TARGET_DRIVES_URL_VALUE_NAME, hr );
            hr = S_FALSE;
            szValue[ 0 ] = 0;
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVGetTargetDrivesURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetTargetDrivesURL), szValue );

        hr = GetHostId( szValue, sizeof( szValue ) );
        if( FAILED( hr ) )
        {
            DebugPrintf( "FAILED Couldn't get host id, hr = %08X\n", hr );
            break;
        }

        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szSVHostID, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVHostID), szValue );

        dwResult = cregkey.QueryDWORDValue( SV_AVAILABLE_DRIVES_VALUE_NAME, pSV_HOST_AGENT_PARAMS->AvailableDrives);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_AVAILABLE_DRIVES_VALUE_NAME, hr );
            break;
        }

        //
        // NOTE: Do not set protected drives == dwAvailable drives as a safety precaution against 
        // protected drives == 0. It's okay to have no protected drives; we don't want to start protecting
        // drives that were unprotected, just because the sentinel restarted.
        //
        dwResult = cregkey.QueryDWORDValue( SV_PROTECTED_DRIVES_VALUE_NAME, pSV_HOST_AGENT_PARAMS->ProtectedDrives );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X\n", SV_PROTECTED_DRIVES_VALUE_NAME, hr );
            //break;
        }

        // Get Protected Volume Settings
        dwCount = sizeof( szValue );

        dwResult = cregkey.QueryStringValue( SV_PROTECTED_VOLUME_SETTINGS_URL, szValue,&dwCount);
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X\n", SV_PROTECTED_VOLUME_SETTINGS_URL, hr );
            //break;
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szProtectedVolumeSettingsURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szProtectedVolumeSettingsURL), szValue );

        //
        // Optional: MAX_SENTINEL_PAYLOAD
        //
        pSV_HOST_AGENT_PARAMS->cbMaxSentinelPayload = 0;
        dwResult = cregkey.QueryDWORDValue( SV_MAX_SENTINEL_PAYLOAD_VALUE_NAME, pSV_HOST_AGENT_PARAMS->cbMaxSentinelPayload );
        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_DEBUG, "WARNING cregkey.QueryValue()... key: %s; hr = %08X\n", SV_MAX_SENTINEL_PAYLOAD_VALUE_NAME, hr );
        }

        //
        // Get 'report if resync required' url. If not present, use a default.
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_REPORT_RESYNC_REQUIRED_URL_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            char const* pszDefaultUrl = "/report_resync_required.php";

            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_REPORT_RESYNC_REQUIRED_URL_VALUE_NAME, hr, pszDefaultUrl );

            inm_strncpy_s( pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL), pszDefaultUrl, sizeof( pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL ) );
            hr = S_FALSE;
        }
        else
        {
            inm_strncpy_s( pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL), szValue, sizeof( pSV_HOST_AGENT_PARAMS->szReportResyncRequiredURL ) );
        }

        //
        // Get update_status url. If not present, use a default.
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_UPDATE_STATUS_URL_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            char const* pszDefaultUrl = "/update_status.php";

            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_UPDATE_STATUS_URL_VALUE_NAME, hr, pszDefaultUrl );

            inm_strncpy_s( pSV_HOST_AGENT_PARAMS->szUpdateStatusURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdateStatusURL), pszDefaultUrl, sizeof( pSV_HOST_AGENT_PARAMS->szUpdateStatusURL ) );
            hr = S_FALSE;
        }
        else
        {
            inm_strncpy_s( pSV_HOST_AGENT_PARAMS->szUpdateStatusURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdateStatusURL), szValue, sizeof( pSV_HOST_AGENT_PARAMS->szUpdateStatusURL ) );
        }

        //
        // Get the CacheDirectory.
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_CACHE_DIRECTORY_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "FAILED cregkey.QueryValue()... key: %s; hr = %08X\n", SV_CACHE_DIRECTORY_VALUE_NAME, hr );
            break;
        }
        else
        {
            inm_strncpy_s( pSV_HOST_AGENT_PARAMS->szCacheDirectory, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szCacheDirectory), szValue, sizeof( pSV_HOST_AGENT_PARAMS->szCacheDirectory )-1 );
        }

        //
        // Optional parameter: CxupdateIntervel
        //
        dwResult = cregkey.QueryDWORDValue( SV_CX_UPDATE_INTERVAL, pSV_HOST_AGENT_PARAMS->CxUpdateInterval);
        if( ERROR_SUCCESS != dwResult )
        {
            
            hr = HRESULT_FROM_WIN32( dwResult );
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_DEBUG, "Warning cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_CX_UPDATE_INTERVAL, hr );
            pSV_HOST_AGENT_PARAMS->CxUpdateInterval = DEFAULT_CX_UPDATE_INTERVAL;
        }

        //
        // Optional parameter: FileTransport. Default is FTP.
        //
        dwResult = cregkey.QueryDWORDValue( SV_FILE_TRANSPORT_VALUE_NAME, (DWORD&) pSV_HOST_AGENT_PARAMS->FileTransport);

        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_DEBUG, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_FILE_TRANSPORT_VALUE_NAME, hr );
            pSV_HOST_AGENT_PARAMS->FileTransport = SV_HOST_AGENT_PARAMS::FILE_TRANSPORT_FTP;
        }

        //
        // Optional parameter: ShouldResync. By default, use 0.
        //
        DWORD dwTemp = 0;
        dwResult = cregkey.QueryDWORDValue( SV_SHOULD_RESYNC_VALUE_NAME, dwTemp );
        pSV_HOST_AGENT_PARAMS->ShouldResync = 0 != dwTemp;

        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_DEBUG, "WARNING cregkey.QueryValue()... key: %s; using default value. hr = %08X\n", SV_SHOULD_RESYNC_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            pSV_HOST_AGENT_PARAMS->ShouldResync = false;
        }

        //
        // Optional parameter: GetFileReplicationConfigurationURL
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_GET_FILE_REPLICATION_CONFIGURATION_URL_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( hr, "WARNING read key: %s; using default value. hr = %08X\n", SV_GET_FILE_REPLICATION_CONFIGURATION_URL_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );

            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "/get_file_replication_config.php" );
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szGetFileReplicationConfigurationURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szGetFileReplicationConfigurationURL), szValue );

        //
        // Optional parameter: UpdateFileReplicationStatusURL
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_UPDATE_FILE_REPLICATION_STATUS_URL_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( hr, "WARNING read key: %s; using default value. hr = %08X\n", SV_UPDATE_FILE_REPLICATION_STATUS_URL_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );

            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "/update_file_replication_status.php" );
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szUpdateFileReplicationStatusURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdateFileReplicationStatusURL), szValue );

        //
        // Optional parameter: InmSyncExe
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_INMSYNC_EXE_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( hr, "WARNING read key: %s; using default value. hr = %08X\n", SV_RSYNC_EXE_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );

            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "" );
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szInmsyncExe, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szInmsyncExe), szValue );


        //
        // Optional parameter: InmsyncExe timeout
        //
        if (!GetInmsyncTimeoutFromRegistry(pSV_HOST_AGENT_PARAMS->RsyncTimeOut)) {
            pSV_HOST_AGENT_PARAMS->RsyncTimeOut = 1800; // default seconds
        }


        //
        // Optional parameter: ShellCommand
        //
        dwCount = sizeof( szValue );
        dwResult = cregkey.QueryStringValue( SV_SHELL_COMMAND_VALUE_NAME, szValue, &dwCount );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( hr, "WARNING read key: %s; using default value. hr = %08X\n", SV_SHELL_COMMAND_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );

            inm_strcpy_s( szValue, ARRAYSIZE(szValue), "" );
        }
        inm_strcpy_s( pSV_HOST_AGENT_PARAMS->szShellCommand, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szShellCommand), szValue );

        //now get Transport parameters
        // BUGBUG: should not fail if params not available. Use default values.
        hr = GetFTPParamsFromRegistry(SV_TRANSPORT_VALUE_NAME,pSV_HOST_AGENT_PARAMS);   
        if(S_OK != hr)
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( hr, "WARNING GetFTPParamsFromRegistry FAILED()... hr = %08X\n", hr ) ;
        }
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf("[INFO] Host agent type = %s\n", pSV_HOST_AGENT_PARAMS->szSVHostAgentType ) ;
        //AutoFS parameters
        if( SV_AGENT_FILE_REPLICATION != AgentType )
        {
            SV_AUTO_FS_PARAMETERS autoFSParams;
            hr = GetAutoFSInfoFromRegistry( pSV_HOST_AGENT_PARAMS->szSVHostAgentRegKey, &autoFSParams );

            if( FAILED( hr ) )
            {
                DebugPrintf( SV_LOG_ERROR, "FAILED Couldn't read AutoFS parameters, hr %08X\n", hr );
                break;
            }

            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadOnlyDrivesURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadOnlyDrivesURL), autoFSParams.szGetVisibleReadOnlyDrivesURL, INTERNET_MAX_URL_LENGTH);
            pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadOnlyDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';

            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadWriteDrivesURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadWriteDrivesURL), autoFSParams.szGetVisibleReadWriteDrivesURL, INTERNET_MAX_URL_LENGTH);
            pSV_HOST_AGENT_PARAMS->szSVGetVisibleReadWriteDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';

            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szSVShouldUpdateResyncDrivesURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSVShouldUpdateResyncDrivesURL), autoFSParams.szUpdateShouldResyncDrive, INTERNET_MAX_URL_LENGTH);
            pSV_HOST_AGENT_PARAMS->szSVShouldUpdateResyncDrivesURL[INTERNET_MAX_URL_LENGTH] = '\0';
        }


        //
        // Optional parameter: EnableFrLogFileDeletion
        //
        dwResult = cregkey.QueryDWORDValue( SV_ENABLE_FR_LOG_FILE_DELETION, pSV_HOST_AGENT_PARAMS->EnableFrLogFileDeletion );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( "WARNING read key: %s; using default value 0. hr = %08X\n", SV_ENABLE_FR_LOG_FILE_DELETION, HRESULT_FROM_WIN32( GetLastError() ) );
        }

        //
        // Optional parameter: EnableFrDeleteOptions
        //
        dwResult = cregkey.QueryDWORDValue( SV_ENABLE_FR_DELETE_OPTIONS, pSV_HOST_AGENT_PARAMS->EnableFrDeleteOptions );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( "WARNING read key: %s; using default value 0. hr = %08X\n", SV_ENABLE_FR_DELETE_OPTIONS, HRESULT_FROM_WIN32( GetLastError() ) );
        }

        //
        // Optional parameter: UseConfiguredIP
        //
        DWORD dwUseConfiguredIP = 0;
        dwResult = cregkey.QueryDWORDValue( SV_USE_CONFIGURED_IP, dwUseConfiguredIP );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( "WARNING read key: %s; using default value 0. hr = %08X\n", SV_USE_CONFIGURED_IP, HRESULT_FROM_WIN32( GetLastError() ) );
        }

        if( dwUseConfiguredIP )
        {
            ULONG cChars = sizeof( s_szConfiguredIP );
            dwResult = cregkey.QueryStringValue( SV_CONFIGURED_IP, s_szConfiguredIP, &cChars );

            if( ERROR_SUCCESS != dwResult )
            {
                hr = GetLastError();
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "FAILED Configured IP not read, err %d\n", GetLastError() );
            }
        }
        else
        {
            s_szConfiguredIP[ 0 ] = 0;
        }

        //
        // Optional parameter: UseConfiguredHostname
        //
        DWORD dwUseConfiguredHostname = 0;
        dwResult = cregkey.QueryDWORDValue( SV_USE_CONFIGURED_HOSTNAME, dwUseConfiguredHostname );

        if( ERROR_SUCCESS != dwResult )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( "WARNING read key: %s; using default value 0. hr = %08X\n", SV_USE_CONFIGURED_HOSTNAME, HRESULT_FROM_WIN32( GetLastError() ) );
        }

        if( dwUseConfiguredHostname )
        {
            ULONG cChars = sizeof( s_szConfiguredHostname );
            dwResult = cregkey.QueryStringValue( SV_CONFIGURED_HOSTNAME, s_szConfiguredHostname, &cChars );

            if( ERROR_SUCCESS != dwResult )
            {
                hr = GetLastError();
                DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
                DebugPrintf( "FAILED Configured hostname not read, err %d\n", GetLastError() );
            }
        }
        else
        {
            s_szConfiguredHostname[ 0 ] = 0;
        }

        dwResult = cregkey.QueryDWORDValue( SV_MAX_FR_LOG_PAYLOAD_VALUE_NAME, pSV_HOST_AGENT_PARAMS->MaxFrLogPayload );

        if( ERROR_SUCCESS != dwResult || 0 == pSV_HOST_AGENT_PARAMS->MaxFrLogPayload )
        {
            //DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            //DebugPrintf( "WARNING read key: %s; using default value 500K. hr = %08X\n", SV_MAX_FR_LOG_PAYLOAD_VALUE_NAME, HRESULT_FROM_WIN32( GetLastError() ) );
            pSV_HOST_AGENT_PARAMS->MaxFrLogPayload = 500 * 1024;
        }

        //
        // Optional: FR uses 'SendSignalExe' to send signals to rsync
        //
        char szSendSignalExe[ SV_MAX_PATH+1 ];
        ULONG cChars = sizeof( szSendSignalExe );
        dwResult = cregkey.QueryStringValue( SV_FR_SEND_SIGNAL_EXE, szSendSignalExe, &cChars );

        if( ERROR_SUCCESS != dwResult )
        {
            DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( "WARNING Couldn't read %s, err %d\n", SV_FR_SEND_SIGNAL_EXE, GetLastError() );
        }
        else
        {
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szSendSignalExe, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szSendSignalExe), szSendSignalExe, ARRAYSIZE(szSendSignalExe));
        }

        DWORD dwenableEncryption = 0;
        dwResult = cregkey.QueryDWORDValue( SV_FR_ENABLE_ENCRYPTION, dwenableEncryption );
        if( ERROR_SUCCESS == dwResult )
        {
            if(dwenableEncryption == 1 )
            {    
                bEnableEncryption =  true;
                DebugPrintf(SV_LOG_DEBUG, "Encryption is Enabled\n");
            }
            else
            {
                bEnableEncryption = false;
                DebugPrintf(SV_LOG_DEBUG, "Encryption is disabled\n");
            }
        }
        else
        {
            bEnableEncryption =  true;
            DebugPrintf(SV_LOG_DEBUG, "Encryption is Enabled\n");
        }
        DWORD dwRegisterHostTimeIntervel = 0;
        dwResult = cregkey.QueryDWORDValue( SV_FR_REGISTER_HOST_INTERVEL, dwRegisterHostTimeIntervel );
        if( ERROR_SUCCESS == dwResult )
        {
            RegisterHostIntervel =     dwRegisterHostTimeIntervel;
            DebugPrintf(SV_LOG_DEBUG, "RegisterHost call for every %d seconds\n",RegisterHostIntervel);
        }
        else
        {
            RegisterHostIntervel =  180;
            DebugPrintf(SV_LOG_DEBUG, "RegisterHost call for every %d seconds\n",RegisterHostIntervel);
        }

        // SV_REGISTER_CLUSTER_INFO_URL_VALUE_NAME
        dwCount = sizeof(szValue);
        dwResult = cregkey.QueryStringValue(SV_REGISTER_CLUSTER_INFO_URL_VALUE_NAME, szValue, &dwCount);
        if (ERROR_SUCCESS != dwResult) {
            char const* pszDefaultUrl = "/register_cluster_info.php";
            hr = HRESULT_FROM_WIN32(dwResult);
            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_REGISTER_CLUSTER_INFO_URL_VALUE_NAME, hr, pszDefaultUrl);
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL), pszDefaultUrl, sizeof(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL));
            hr = S_FALSE;
        } else {
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL), szValue, sizeof(pSV_HOST_AGENT_PARAMS->szRegisterClusterInfoURL));
        }

        // SV_UPDATE_AGENT_LOG_URL_VALUE_NAME
        dwCount = sizeof(szValue);
        dwResult = cregkey.QueryStringValue(SV_UPDATE_AGENT_LOG_URL_VALUE_NAME, szValue, &dwCount);
        if (ERROR_SUCCESS != dwResult) {
            char const* pszDefaultUrl = "/update_agent_log.php";
            hr = HRESULT_FROM_WIN32(dwResult);
            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_UPDATE_AGENT_LOG_URL_VALUE_NAME, hr, pszDefaultUrl);
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL), pszDefaultUrl, sizeof(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL));
            hr = S_FALSE;
        } else {
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL), szValue, sizeof(pSV_HOST_AGENT_PARAMS->szUpdateAgentLogURL));
        }

        // SV_GET_VOLUME_GROUP_SETTINGS_URL_VALUE_NAME
        dwCount = sizeof(szValue);
        dwResult = cregkey.QueryStringValue(SV_GET_VOLUME_GROUP_SETTINGS_URL_VALUE_NAME, szValue, &dwCount);
        if (ERROR_SUCCESS != dwResult) {
            char const* pszDefaultUrl = "/get_volume_group_settings.php";
            hr = HRESULT_FROM_WIN32(dwResult);
            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
            DebugPrintf(hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_GET_VOLUME_GROUP_SETTINGS_URL_VALUE_NAME, hr, pszDefaultUrl);
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl), pszDefaultUrl, sizeof(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl));
            hr = S_FALSE;
        } else {
            inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl), szValue, sizeof(pSV_HOST_AGENT_PARAMS->szGetVolumeGroupSettingsUrl));
        }
        
        if( SV_AGENT_FILE_REPLICATION == AgentType ) {
            // SV_UPDATE_PERMISSION_STATE_URL_VALUE_NAME
        dwCount = sizeof(szValue);
            dwResult = cregkey.QueryStringValue(SV_UPDATE_PERMISSION_STATE_URL_VALUE_NAME, szValue, &dwCount);
        if (ERROR_SUCCESS != dwResult) {
                char const* pszDefaultUrl = "/update_permission_state.php";
            hr = HRESULT_FROM_WIN32(dwResult);
            DebugPrintf("@ LINE %d in FILE %s \n", __LINE__, __FILE__);
                DebugPrintf(hr, "WARNING cregkey.QueryValue()... key: %s; hr = %08X, using default %s\n", SV_UPDATE_PERMISSION_STATE_URL_VALUE_NAME, hr, pszDefaultUrl);
                inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl), pszDefaultUrl, sizeof(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl));
            hr = S_FALSE;
        } else {
                inm_strncpy_s(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl, ARRAYSIZE(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl), szValue, sizeof(pSV_HOST_AGENT_PARAMS->szUpdatePermissionStateUrl));
            }
        }  

        // fast sync compare sizes
        dwResult = cregkey.QueryDWORDValue(FAST_SYNC_MAX_CHUNK_SIZE_NAME, pSV_HOST_AGENT_PARAMS->fastSyncMaxChunkSize);
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                        FAST_SYNC_MAX_CHUNK_SIZE_NAME, dwResult);
            pSV_HOST_AGENT_PARAMS->fastSyncMaxChunkSize = FAST_SYNC_MAX_CHUNK_SIZE_DEFAULT;  // default value to use 64MB 
            dwResult = cregkey.SetDWORDValue(FAST_SYNC_MAX_CHUNK_SIZE_NAME, pSV_HOST_AGENT_PARAMS->fastSyncMaxChunkSize);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry set failed (using default value): %d\n", 
                             FAST_SYNC_MAX_CHUNK_SIZE_NAME, dwResult);
            }
        }

        dwResult = cregkey.QueryDWORDValue(FAST_SYNC_HASH_COMPARE_DATA_SIZE_NAME, pSV_HOST_AGENT_PARAMS->fastSyncHashCompareDataSize);
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                        FAST_SYNC_HASH_COMPARE_DATA_SIZE_NAME, dwResult);
            pSV_HOST_AGENT_PARAMS->fastSyncHashCompareDataSize = FAST_SYNC_HASH_COMPARE_DATA_SIZE_DEFAULT; 
            dwResult = cregkey.SetDWORDValue(FAST_SYNC_HASH_COMPARE_DATA_SIZE_NAME, pSV_HOST_AGENT_PARAMS->fastSyncHashCompareDataSize);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry set %s failed (using default value): %d\n", 
                            FAST_SYNC_HASH_COMPARE_DATA_SIZE_NAME, dwResult);
            }
        } 

        // resync source directory path (i.e. cx location)
        dwCount = sizeof( pSV_HOST_AGENT_PARAMS->szResyncSourceDirectoryPath );
        dwResult = cregkey.QueryStringValue(RESYNC_SOURCE_DIRECTORY_PATH_NAME, pSV_HOST_AGENT_PARAMS->szResyncSourceDirectoryPath, &dwCount );
        if( ERROR_SUCCESS != dwResult )
        {
            hr = HRESULT_FROM_WIN32( dwResult );            
            DebugPrintf( SV_LOG_ERROR, "FAILED GetAgentParamsFromRegistry query %s: hr =  %08X\n",RESYNC_SOURCE_DIRECTORY_PATH_NAME, hr );
            break;
        }

        // max resync threads
        dwResult = cregkey.QueryDWORDValue( SV_MAX_RESYNC_THREADS_VALUE_NAME, pSV_HOST_AGENT_PARAMS->maxRsyncThreads );
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                SV_MAX_RESYNC_THREADS_VALUE_NAME, dwResult);
            pSV_HOST_AGENT_PARAMS->maxRsyncThreads = DEFAULT_RESYNC_THREADS;
            dwResult = cregkey.SetDWORDValue(SV_MAX_RESYNC_THREADS_VALUE_NAME, pSV_HOST_AGENT_PARAMS->maxRsyncThreads);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry set %s failed (using default value): %d\n", 
                    SV_MAX_RESYNC_THREADS_VALUE_NAME, dwResult);
            }
        } 

        // settings polling interval 
        dwResult = cregkey.QueryDWORDValue( SV_SETTINGS_POLLING_INTERVAL, pSV_HOST_AGENT_PARAMS->settingsPollingInterval );
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                SV_SETTINGS_POLLING_INTERVAL, dwResult);
            pSV_HOST_AGENT_PARAMS->settingsPollingInterval = DEFAULT_SETTINGS_POLLING_INTERVAL;
        }

        // Query the agent install path
        CRegKey cregkeyInstall;

        if( SV_AGENT_FILE_REPLICATION == AgentType )
            dwResult = cregkeyInstall.Open(HKEY_LOCAL_MACHINE, SV_FILEREP_AGENT_VALUE_NAME);
        else
            dwResult = cregkeyInstall.Open(HKEY_LOCAL_MACHINE, SV_VXAGENT_VALUE_NAME);

        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry cregKeyInstall.open() to %s or %s failed. Error=%d\n", 
                SV_FILEREP_AGENT_VALUE_NAME, SV_VXAGENT_VALUE_NAME, dwResult);
        }
        else
        {
            DWORD dwCount = sizeof( pSV_HOST_AGENT_PARAMS->AgentInstallPath );
            dwResult = cregkeyInstall.QueryStringValue(SV_AGENT_INSTALL_LOCATON_VALUE_NAME, pSV_HOST_AGENT_PARAMS->AgentInstallPath, &dwCount);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry cregKeyInstall.QueryStringValue() for %s failed. Error=%d\n", 
                SV_VXAGENT_VALUE_NAME, dwResult);
            }
        }

        // min cache free disk space percent
        dwResult = cregkey.QueryDWORDValue( SV_MIN_CACHE_FREE_DISK_SPACE_PERCENT, pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpacePercent );
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                SV_MIN_CACHE_FREE_DISK_SPACE_PERCENT, dwResult);
            pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpacePercent = DEFAULT_MIN_CACHE_FREE_DISK_SPACE_PERCENT;
            dwResult = cregkey.SetDWORDValue(SV_MIN_CACHE_FREE_DISK_SPACE_PERCENT, pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpacePercent);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry set %s failed (using default value): %d\n", 
                    SV_MIN_CACHE_FREE_DISK_SPACE_PERCENT, dwResult);
            }
        } 

        // min cache free disk space 
        dwResult = cregkey.QueryDWORDValue( SV_MIN_CACHE_FREE_DISK_SPACE, pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpace );
        if (ERROR_SUCCESS != dwResult) {
            DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry query %s failed (settting to default value): %d\n", 
                SV_MIN_CACHE_FREE_DISK_SPACE, dwResult);
            pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpace = DEFAULT_MIN_CACHE_FREE_DISK_SPACE;
            dwResult = cregkey.SetDWORDValue(SV_MIN_CACHE_FREE_DISK_SPACE, pSV_HOST_AGENT_PARAMS->minCacheFreeDiskSpace);
            if (ERROR_SUCCESS != dwResult) {
                DebugPrintf(SV_LOG_WARNING, "WARNING GetAgentParamsFromRegistry set %s failed (using default value): %d\n", 
                    SV_MIN_CACHE_FREE_DISK_SPACE, dwResult);
            }
        } 
    }
    while( false );

    cregkey.Close();
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return( hr );
}

