#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <dirent.h>
#include <fnmatch.h>
#include <time.h>

#ifdef __hpux__ // ski: hp-ux header files are brain dead
#undef _APP32_64BIT_OFF_T
#endif

#include <sys/socket.h>

#if defined(__hpux__) && defined(__LP64__) // hp-ux header files are brain dead
#define _APP32_64BIT_OFF_T
#endif

#include <string>
#ifdef HAVE_NETINET_H 
	#include <netinet/in.h>
#endif
#ifdef HAVE_NETDB_H
	#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef HAVE_UUID_GENERATE
	#include <uuid/uuid.h>
#endif
#include "hostagenthelpers_ported.h"
#include "svconfig.h"
#include "svmacros.h"
#include "svutils.h"
#include "logger.h"
//#include <libgen.h> // Commented as DirName function is moved to portablehelpers.cpp

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif
#ifndef OSINFOCMDDEF
#define OSINFOCMDDEF
const char OSINFOCMD[] =  "uname -rsv";
#define OSINFOLENGTH 500
#endif
#define UUID_STRING_LENGTH 36
#define BUFSIZE  512
char g_szEmptyString[] = "";

const char KERNELINFOCMD[] = "uname -r";

ACE_Recursive_Thread_Mutex dirCreateMutex;

SV_ULONG SVGetFileSize( char const* pszPathname, OPTIONAL SV_ULONG* pHighSize )
{
	// BUGBUG: should set pHighSize if not null
     struct stat file;
     if(!stat(pszPathname,&file))
     {
         return file.st_size;
     }
     else return SV_INVALID_FILE_SIZE;
}

bool SVIsFilePathExists( char const* pszPathname )
{
     struct stat file;
     if(!stat(pszPathname,&file))
         return true;
     else 
		 return false;
}


SVERROR ReadAgentConfiguration( SV_HOST_AGENT_PARAMS *pConfiguration,
                                const char* pFileName,
                                SV_HOST_AGENT_TYPE AgentType )
{
    /* place holder function */
    throw std::string("ReadAgentConfiguration is an obsolete now.");
    return SVS_OK;
}


///
/// Removes all trailing backslashes
///
char* PathRemoveAllBackslashesA( char* pszPath )
{
    char* psz = NULL;
    assert( NULL != pszPath );

    int cch = strlen( pszPath );
    for( psz = pszPath + cch - 1; psz >= pszPath && '\\' == *psz;
         psz-- )
    {
        *psz = 0;
    }

    return( std::max( psz, pszPath ) );
}

///
/// Removes all trailing backslashes, NOT IMPLEMENTED
///
SV_WCHAR* PathRemoveAllBackslashesW( SV_WCHAR* pszPath )
{
    /*
    assert( NULL != pszPath );

    SV_WCHAR* psz;
    do
    {
        psz = PathRemoveBackslashW( pszPath ) - 1;
    }
    while( 0 != pszPath[0] && L'/' == *psz );

    return( psz );
    */
    return NULL;
}


SVERROR GetProcesOutput(const std::string& cmd, std::string& output)
{
    FILE *in;
    char buff[512];

    /* popen creates a pipe so we can read the output
     of the program we are invoking */
    if (!(in = popen(cmd.c_str(), "r")))
    {
        return SVS_FALSE;
    }

    /* read the output of command , one line at a time */
    while (fgets(buff, sizeof(buff), in) != NULL )
    {
        output += buff;
    }

    /* close the pipe */
    pclose(in);
    return SVS_OK;
}

SVERROR GetConfigFileName(std::string &strConfFileName)
{
    SVConfig* conf = SVConfig::GetInstance();
    return conf->GetConfigFileName(strConfFileName);
}

///
/// Does a recursive mkdir(). Argument is an absolute, not relative path.
///
SVERROR SVMakeSureDirectoryPathExists( const char* pszDestPathname )
{
    ACE_Guard<ACE_Recursive_Thread_Mutex> dirCreateMutexGuard(dirCreateMutex);
    
    SVERROR rc = SVS_OK;
    char szPathname[ PATH_MAX ];
    char szPrefix[ PATH_MAX ];
    
    if (NULL == pszDestPathname || 0 == pszDestPathname[0])
    {
        rc = SVE_INVALIDARG;
        return rc;
    }
	
    if( strlen( pszDestPathname ) > PATH_MAX - 1 )
    {
       DebugPrintf(SV_LOG_ERROR, "The path length is greater than %d\n", PATH_MAX) ;
       rc = SVE_FAIL ;
       return rc;
    }
	
    inm_strcpy_s( szPathname, ARRAYSIZE(szPathname), pszDestPathname );
    char* pszToken = NULL;
    char* lasts = NULL;

    do
    {
        ZeroFill( szPrefix ) ;
        if( strchr( szPathname, '/' ) == szPathname ) //is this an absolute path?
        {
            inm_strcpy_s( szPrefix, ARRAYSIZE(szPrefix), "/" );
        }
     
        struct stat buf;
        ZeroFill( buf );

        while( 1 )
        {
            if( strlen( szPrefix ) > 0 )
            {

                if( stat( szPrefix, &buf ) < 0 )
                {
                    if( mkdir( szPrefix, 0777 ) < 0 && EEXIST != errno )
                    {
                        DebugPrintf( SV_LOG_ERROR, "FAILED Couldn't mkdir %s, "
                            "error = %d\n", szPrefix, errno );
                        rc = errno;
                        break;
                    }
                }

                if( stat( szPrefix, &buf) < 0 || !S_ISDIR( buf.st_mode ) )
                {
                    DebugPrintf( SV_LOG_ERROR, "FAILED mkdir(), not a dir: %s\n",
                        szPrefix );
                    rc = SVE_FAIL;
                    break;
                }
            }

            pszToken = strtok_r( NULL == pszToken ? szPathname : NULL, "/" , &lasts );
            if( NULL == pszToken )
            {
                break;
            }

            inm_strcat_s( szPrefix, ARRAYSIZE(szPrefix), pszToken );
            inm_strcat_s( szPrefix, ARRAYSIZE(szPrefix), "/" );
        }
    } while( false );

    return( rc );
}

void StringToSVLongLong( SV_LONGLONG& num, char const* psz )
{
    assert( NULL != psz );
    sscanf( psz, "%lld", &num );
}

//
// NOTE: assumes psz has enough space to hold string
//
char* SVLongLongToString( char* psz, SV_LONGLONG num, size_t size )
{
    assert( NULL != psz );
    inm_sprintf_s( psz, size, "%lld", num );
    return( psz );
}

void GetKernelVersion(std::string &kernelVersion)
{
    try {
        GetProcessOutput(KERNELINFOCMD, kernelVersion);
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: failed to process command %s.\n",
            FUNCTION_NAME,
            KERNELINFOCMD);
    }
}

void GetOperatingSystemInfo(char* const psInfo, int length)
{
    std::string rsInfo;
    GetProcessOutput(OSINFOCMD, rsInfo);
    inm_strncpy_s(psInfo, length, rsInfo.c_str(), rsInfo.length());
    psInfo[length] = '\0';
}

bool WriteKeyValueInConfigFile(const char *key, std::string value)
{
    SVConfig* conf = SVConfig::GetInstance();
    SVERROR er = SVS_OK;
    bool rv = true;
    conf->WriteKeyValuePairToFile(key,value);
    if ( er.failed())
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to Write KeyValuePair to Configfile\n" );
        er = SVE_FAIL;
    }
    if ( er == SVE_FAIL )
        rv = false;

    return rv;
}

bool GetValuesInConfigFile(std::string configFile, const char *key, std::string& value)
{
    SVERROR hr = SVS_OK;
    bool rv = true;
    SVConfig* conf = SVConfig::GetInstance();
    hr = conf->Parse(configFile);
    if (hr.failed())
    {
        DebugPrintf(SV_LOG_ERROR,"Failed to Parse the Configfile\n" );
        return false;
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "Parse succeeded for Configfile\n" );
    }
    hr = conf->GetValue(key, value);
    if(!hr.failed())
    {
        DebugPrintf(SV_LOG_DEBUG," Seting Values In ConfigFile %s = %s\n",key,value.c_str());
    }
    else
    {
        DebugPrintf( SV_LOG_ERROR,"Failed: [SVConfig::GetValue ] for the Key %s = %s\n",key, value.c_str());
        hr = SVE_FAIL;
    }
    if(hr == SVE_FAIL)
        rv = false;

    return rv;
}

bool SetValuesInConfigFile(const char *key, std::string& newValue)
{

    SVConfig* conf = SVConfig::GetInstance();
    SVERROR er = SVS_OK;
    bool rv = true;
    std::string value;
    er = conf->GetValue(key,value);
    if(!er.failed() )
    {
        DebugPrintf(SV_LOG_DEBUG," Seting Values In ConfigFile %s = %s\n",key,value.c_str());
        conf->UpdateConfigParameter(key,newValue);
        conf->Write();
    }
    else
    {
        DebugPrintf( SV_LOG_ERROR,"Failed: [SVConfig::GetValue ] for the Key %s\n",key);
        er = SVE_FAIL ;
    }
    if(er == SVE_FAIL)
        rv = false;

    return rv;
}


