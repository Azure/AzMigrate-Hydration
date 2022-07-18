#include <stdio.h>
#include <sys/types.h>
#include <string>
#include "portablehelpers.h"
using namespace std;
#include <errno.h>
#include "svtypes.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "svutils.h"
#include "portable.h"
#include "../log/logger.h"
#ifdef SV_UNIX
#include <libgen.h> // Commented as DirName function is moved to portablehelpers.cpp
#endif

void trim(std::string& s, std::string trim_chars)
{
    size_t begIdx = s.find_first_not_of(trim_chars);
    if (begIdx == std::string::npos) 
    {
        begIdx = 0;
    }
    size_t endIdx = s.find_last_not_of(trim_chars);
    if (endIdx == std::string::npos) 
    {
        endIdx = s.size() - 1;
    }
    s = s.substr(begIdx, endIdx-begIdx+1);
    if(endIdx == begIdx)
    {
        s.clear();
    }
}

SVERROR GetProcessOutput(const std::string& cmd, std::string& output)
{
#ifdef SV_UNIX
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
    trim(output, " \n\b\t\a\r\xc");
    /* close the pipe */
    pclose(in);
#endif    
    return SVS_OK;
}

SVERROR SVGetFileSize( char const* pszPathname, SV_LONGLONG* pFileSize )
{
	SVERROR rc;
	ACE_HANDLE hHandle = ACE_INVALID_HANDLE;
	SV_LONGLONG llSize = 0;

// PR#10815: Long Path support
#ifdef SV_USES_LONGPATHS
	//Can't use getLongPathName() as it uses DebugPrintf()
	//and DebugPrintf() inturn calls SVGetFileSize() causing stack overflow
	std::wstring filePath;
#ifdef SV_WINDOWS
	filePath = L"\\\\?\\";
	char const* iter = pszPathname;
	std::locale loc;
	for (/* empty */ ; *iter != '\0'; ++iter) {
		// convert ascii to wide char make sure to convert any slashes to back slashes
		// as that is required by windows extended-length path
		filePath += std::use_facet<std::ctype<wchar_t> >(loc).widen('/' == *iter ? '\\' : *iter);
	}

	// remove repeating '\\' (if any)
	// Note start check at index 3 to skip over prefixes '\\?' and '\\.'
	// (as those are valid repeating '\\' that should not be removed)
	if (filePath.size() > 4) {
		std::wstring::size_type index = 3;
		while (std::wstring::npos != (index = filePath.find(L"\\\\", index))) {
			filePath.erase(index, 1);
		}
	}
#else   /* SV_WINDOWS */
	filePath += ACE_Ascii_To_Wide(pszPathname).wchar_rep();
#endif /* SV_WINDOWS */

#else /* SV_USES_LONGPATHS */
	std::string filePath = pszPathname;
#endif /* SV_USES_LONGPATHS */

	if ( ACE_INVALID_HANDLE == (hHandle = ACE_OS::open(filePath.c_str(),O_RDONLY) ) )
    {
        rc = ACE_OS::last_error();
        // SVGetFileSize is invoked from logger. 
        // Calling DebugPrintf would end up in infinite recursion.
        // Let Logger component take care of the error
		//DebugPrintf(SV_LOG_ERROR,"FAILED: Failed to determine size of file %s. @ LINE %d in FILE %s.",pszPathname);
    }
    else
    {
		llSize = ACE_OS::filesize(hHandle);
        ACE_OS::close(hHandle);            
		if ( llSize < 0 )
        {
            llSize = 0;
            rc = ACE_OS::last_error();
            // SVGetFileSize is invoked from logger. 
            // Calling DebugPrintf would end up in infinite recursion.
            // Let Logger component take care of the error
			//DebugPrintf(SV_LOG_ERROR,"FAILED: Failed to determine size of file %s. @ LINE %d in FILE %s. ",pszPathname );
        }
    }
    *pFileSize = llSize;
	return( rc );
}

/*
 * Retrieves string value for a given key in an input string delimited by one or more space.
 * Basically this function searches key string in given input string and returns the following string as its value ignoring all space chars in between.
 * Note: return value is string of all consequent non space chars.
 * For example, input="-mn  10.150.208.123 -cn 10.150.208.188,10.150.1.30"
 *             1) key=-mn then returns "10.150.208.123"
 *             2) key=-cn then returns "10.150.208.188,10.150.1.30"
*/
std::string GetValueForPropertyKey(const std::string& input, const::std::string& key, std::size_t valueSize)
{
    std::string val;
    if (!key.length())
    {
        return val;
    }

    std::size_t keyPos = input.find(key);
    if (std::string::npos != keyPos)
    {
        keyPos += key.length();
        std::size_t valueStart = input.find_first_not_of(" ", keyPos);

        if (std::string::npos != valueStart)
        {
            std::size_t valueEnd;
            if (0 == valueSize)
            {
                valueEnd = input.find_first_of(" ", valueStart);
                if (std::string::npos != valueEnd)
                {
                    valueSize = valueEnd - valueStart;
                }
                else if (std::string::npos == valueEnd)
                {
                    valueSize = input.length() - valueStart;
                }
            }

            val = input.substr(valueStart, valueSize);
        }
    }

    return val;
}