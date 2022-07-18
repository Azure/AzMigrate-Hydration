#include "portablehelpers.h"
#include "portablehelperscommonheaders.h"
#include "portablehelpersmajor.h"
#include "inmquitfunction.h"
#include "svutils.h"
#include "version.h"
#include "inmageex.h"
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include "file.h"
#include "npwwnif.h"
#include "printfunctions.h"
#include "inmdefines.h"
#include "inm_md5.h"
#include "transport_settings.h"
#include "genericstream.h"
#include "transportstream.h"
#include "serializevolumegroupsettings.h"
#include "configutils.h"
#include "diskslayoutupdater.h"
#include "diskslayoutupdaterimp.h"
#include "inmsafecapis.h"
#include "inmsafeint.h"
#include "inmageex.h"
#include "curlwrapper.h"
#include "configwrapper.h"
#include "inmalertdefs.h"
#include "fingerprintmgr.h"
#include "csgetfingerprint.h"
#include "errorexception.h"

#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstdarg>
#include <cerrno>
#include <boost/algorithm/string/replace.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio.hpp>
#include <ace/Time_Value.h>
#include <curl/curl.h>
#include <boost/chrono.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::chrono;
using std::max;

const std::string AZURE_ASSET_TAG("7783-7084-3265-9085-8269-3286-77");

typedef struct tagMemoryStruct 
{
    char *memory;
    size_t insize;
    size_t size;
} MemoryStruct;

SVERROR MakeReadOnly(const char *drive, void *VolumeGuid, etBitOperation Op);
SVERROR MakeVirtualVolumeReadOnly(const char *mountpoint, void * volumeGuid, etBitOperation ReadOnlyBitFlag);
void GetHypervisorInfo(HypervisorInfo_t &hypervisorinfo);
void PrintHypervisorInfo(const HypervisorInfo_t &hypervisorinfo);
void GetNicInfos(NicInfos_t &nicinfos);
bool HasNicInfoPhysicaIPs(const NicInfos_t &nicinfos);
void PrintNicInfos(NicInfos_t &nicinfos, const SV_LOG_LEVEL logLevel = SV_LOG_DEBUG);
void mountBootableVolumesIfRequried();
extern bool SystemDiskUEFICheck(std::string &errormsg);
extern bool IsSystemAndBootDiskSame(const VolumeSummaries_t &volumeSummaries, std::string &errormsg);
extern bool IsUEFIBoot(void);

boost::mutex g_RegisterHostMutex; //Provide mutual exclusive access to RegisterHost() calls
const size_t SECTOR_SIZE = 512;

//bool IsNewSparseFileFormat(const std::string vol_name)
//{
//    std::string  sparsefile;
//    ACE_stat s;
//    if(!IsVolPackDevice(vol_name.c_str() ,sparsefile) )
//        return false;
//
//    std::string partfile = sparsefile + SPARSE_PARTFILE_EXT;
//    partfile += "0";
//
//    if( sv_stat( getLongPathName(partfile.c_str()).c_str(), &s ) == 0 )
//    {
//        return true;
//    }
//    return false;
//}
/*
    Function: IsVolPackDevice
    Description: 1. Find the given device file is a volpack volume or not
                 2. Detect whether its a multi sparse or single sparse file
    Note: Here there is a extra check if devicefile supplied a sparse file of a volpack
           then it will return true and also tell whether is multi sparse or not
    Input: deviceFile
    output: sparsefile,multisparse
    return value: true if volpack device or sparse file for a volpack device otherwise false
*/
bool IsVolPackDevice(char const * deviceFile,std::string & sparsefile,bool & multisparse)
{
    LocalConfigurator TheLocalConfigurator;
    multisparse = false;

    int counter = TheLocalConfigurator.getVirtualVolumesId();
    while(counter != 0) {
        char regData[26];
        inm_sprintf_s(regData, ARRAYSIZE(regData), "VirVolume%d", counter);

        std::string data = TheLocalConfigurator.getVirtualVolumesPath(regData);

        std::string sparsefilename;
        std::string volume;

        if (!ParseSparseFileNameAndVolPackName(data, sparsefilename, volume))
        {
            counter--;
            continue;
        }
        std::string deviceName = deviceFile;
        FormatVolumeName(deviceName);

        if(0 == stricmp(volume.c_str(),deviceName.c_str())) {
            sparsefile = sparsefilename;
            std::string partfile = sparsefile;
            partfile += ".filepart0";
            ACE_stat s;
            if( sv_stat( getLongPathName(partfile.c_str()).c_str(), &s ) == 0 )
            {
                multisparse = true;
            }            
            return true;
        } else if(0 == stricmp(sparsefilename.c_str(),deviceFile))
        {
            sparsefile = sparsefilename;
            std::string partfile = sparsefile;
            partfile += ".filepart0";
            ACE_stat s;
            if( sv_stat( getLongPathName(partfile.c_str()).c_str(), &s ) == 0 )
            {
                multisparse = true;
            }            
            return true;
        }else {
            counter--;
            continue;
        }
    }
    return false;
}

bool containsVolpackFiles(const std::string & volumename)
{
    LocalConfigurator TheLocalConfigurator;
    int counter = TheLocalConfigurator.getVirtualVolumesId();
    std::string inputvolume = volumename;

    while(counter!=0)
    {
        char regData[26]; // TODO: why 26?
        inm_sprintf_s(regData, ARRAYSIZE(regData), "VirVolume%d", counter);
        std::string data = TheLocalConfigurator.getVirtualVolumesPath(regData);

        std::string sparsefile;
        std::string virtualvolume;

        if(!ParseSparseFileNameAndVolPackName(data, sparsefile, virtualvolume))
        {
            counter--;
            continue;
        }

        FormatVolumeName(inputvolume);
        std::string sparsefile_volume_mnpt;
        GetVolumeRootPath(sparsefile,sparsefile_volume_mnpt);
        std::string sparsefilevolumename;
        GetDeviceNameForMountPt(sparsefile_volume_mnpt,sparsefilevolumename);
        FormatVolumeName(sparsefilevolumename);

        if(sameVolumeCheck(sparsefilevolumename,inputvolume))
        {
            return true;
        }
        counter--;
    }
    return false;
}

/** COMMONTOALL : START */
SVERROR OpenVolume( /* out */ ACE_HANDLE *pHandleVolume,
                   /* in */  const char   *pch_VolumeName
                   )
{
    return OpenVolumeExtended (pHandleVolume, pch_VolumeName, O_RDWR);
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */

///
/// Frees the resources associated with an opened volume. See also CloseVolumeExclusive.
///
SVERROR CloseVolume( ACE_HANDLE handleVolume )
{
    SVERROR sve = SVS_OK;
    ACE_OS::last_error(0);
    do
    {
        DebugPrintf( "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( "ENTERED CloseVolume()...\n" );

        if( ( ACE_INVALID_HANDLE != handleVolume ) &&
            ACE_OS::close( handleVolume ))
        {
            sve = ACE_OS::last_error();
            DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
            DebugPrintf( SV_LOG_ERROR, "FAILED CloseVolume()... err = %s\n", sve.toString() );
            break;
        }
    }

    while( FALSE );

    return( sve );
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */

// TODO: we need to do more investigation on the volume size for now
// GetFileSystemVolumeSize
// GetDriverVolumeSize
// CanReadDriverSize
// GetVolumeSize
// all are used to determine the volume size
// the current approach 
// get the file system reported size
// get the driver reported size
// if files system size is < driver size
// then see if the volume can be read up to the driver volume size
// if so use it, else use the file system size

bool GetFileSystemVolumeSize(char *vol, SV_ULONGLONG* size, unsigned int* sectorSize)
{

    std::string volName;

    if (vol == NULL)
        return false;

    volName = vol;

    SVERROR sve = GetFsVolSize(volName.c_str(), size);

    if (sve.failed())
    {
        DebugPrintf(SV_LOG_WARNING, "WARN: GetFsVolSize() failed. volume = %s (OK if volume is a hidden target or cluster volume offline)\n", volName.c_str());        
        return false;
    }

    (void)GetFsSectorSize(volName.c_str(), sectorSize);

    return true;
}


/** COMMONTOALL : END */



/** COMMONTOALL : START */

bool GetVolumeSize(char *vol, SV_ULONGLONG* size)
{    
    unsigned int sectorSize;
    SV_ULONGLONG fsSize = 0;
    SV_ULONGLONG driverSize = 0;

    bool fsOk = GetFileSystemVolumeSize(vol, &fsSize, &sectorSize);
    bool driverOk = GetDriverVolumeSize(vol, &driverSize);

    if (!(fsOk || driverOk)) {
        // both failed
        return false;
    }

    if (fsOk && driverOk) {
        // both succeeded
        if (fsSize < driverSize) {
            // OK let's see if we can read the driver reported size
            if (CanReadDriverSize(vol, driverSize, sectorSize)) {
                (*size) = driverSize;
                return true;
            }
        }
        (*size) = fsSize;            
    } else if (fsOk) {
        // only file system succeeded
        (*size) = fsSize;
    } else {
        // only driver succeeded
        // TODO: for now return 0 so that the CX does not update this info
        // if we return the driverSize it causes several issues in that the
        // size can be incorrect and the file system type may not be reported
        // this case will happen for cluster nodes that are offline or volumes
        // that are hidden 
        (*size) = 0; // driverSize; 
    }

    return true;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
bool Tokenize(const std::string& input, std::vector<std::string>& outtoks,const std::string& separators)
{
    std::string::size_type pp, cp;

    outtoks.clear();
    pp = input.find_first_not_of(separators, 0);
    cp = input.find_first_of(separators, pp);
    while ((std::string::npos != cp) || (std::string::npos != pp)) {
        outtoks.push_back(input.substr(pp, cp-pp));
        pp = input.find_first_not_of(separators, cp);
        cp = input.find_first_of(separators, pp);
    }

    return true;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
SVERROR HideDrive(const char * drive, char const* mountPoint )
{
    std::string output, error;
    SVERROR sve =  HideDrive(drive, mountPoint, output, error);
    if(sve.failed())
    {
        DebugPrintf(SV_LOG_ERROR, "%s\n", error.c_str());
    }
    return sve;
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
SVERROR SetReadOnly(const char * drive, void * VolumeGUID, bool isVirtualVolume )
{
    if( ! isVirtualVolume)
        return MakeReadOnly(drive, VolumeGUID, ecBitOpSet);
    return MakeVirtualVolumeReadOnly(drive, VolumeGUID, ecBitOpSet);
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
SVERROR ResetReadOnly(const char * drive, void * VolumeGUID, bool isVirtualVolume)
{
    LocalConfigurator localConfigurator;
    if( ! isVirtualVolume && localConfigurator.IsFilterDriverAvailable())
        return MakeReadOnly(drive, VolumeGUID, ecBitOpReset);
    else if (isVirtualVolume && localConfigurator.IsFilterDriverAvailable() && localConfigurator.IsVolpackDriverAvailable())
        return MakeVirtualVolumeReadOnly(drive, VolumeGUID, ecBitOpReset);
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "Filter driver is not present. So cannot reset read only flag.\n");
        return SVS_OK;
    }

}
/** COMMONTOALL : END */



/** COMMONTOALL : START */

// --------------------------------------------------------------------------
// deletes the local file
// --------------------------------------------------------------------------
bool DeleteLocalFile(std::string const & name)
{         
    // PR#10815: Long Path support
    return ( -1 != ACE_OS::unlink(getLongPathName(name.c_str()).c_str()));
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */

SVERROR BaseName( const char *pszFullName, char *pszBaseName, const int pszBaseNameLen)
{
    SVERROR hr  = SVS_OK;
    char *curr  = (char *) pszFullName;
    char *last  = (char *) pszFullName;
    char *first = (char *) pszFullName;

    // Verify input
    if ( (NULL == pszFullName) || (NULL == pszBaseName) )
    {
        hr = SVE_INVALIDARG;
        return hr;
    }

    // Prep the return string
    pszBaseName[0] = '\0'; 

    // Find the last path delimiter
    while (*curr) 
    {
        if ( ( *curr == '/' )  || ( *curr == '\\' ) ) {
            last = curr;
        }
        ++curr;
    }

    // Skip the last path delimiter
    if (last != first) 
    {
        last++;
    }

    // Copy the basename to return string
    inm_strcpy_s(pszBaseName, pszBaseNameLen, last);

    //printf("Basename is %s\n", pszBaseName);
    return hr;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
SVERROR DirName( const char *pszFullName, char *pszDirName, const int pszDirNameLen)
{
    SVERROR hr = SVS_OK;
    char *curr  = (char *) pszFullName;
    char *last  = (char *) pszFullName;
    char *first = (char *) pszFullName;

    // Verify input
    if ( (NULL == pszFullName) || (NULL == pszDirName) )
    {
        hr = SVE_INVALIDARG;
        return hr;
    }

    // Prep the return string
    pszDirName[0] = '\0'; 

    // Find the last path delimiter
    while (*curr) 
    {
        if ( ( *curr == '/')  || ( *curr == '\\' ) ) {
            last = curr;
            //printf("Match at %s, \n", last); 
        }
        ++curr;
    }

    // Copy over the dirname and null terminate it
    inm_strncpy_s(pszDirName, pszDirNameLen, first, (last - first));
    pszDirName[last-first] = '\0';

    //printf("Dirname is %s\n", pszDirName);
    return hr;
}
/** COMMONTOALL : END */


std::string basename_r(const std::string &name, const char &separator_char)
{
    std::string::size_type scp = name.rfind(separator_char);
    return (std::string::npos == scp) ? name : name.substr(scp+1);
}


std::string dirname_r(const std::string &name, const char &separator_char)
{
    std::string::size_type scp = name.rfind('\\');
    if (std::string::npos == scp)
        scp = name.rfind('/');

    return (std::string::npos == scp) ? std::string(".") : name.substr(0, scp);
}


std::string PlatformBasedDirname(const std::string &name, const char &separator_char)
{
    std::string::size_type scp = name.rfind(separator_char);
    return (std::string::npos == scp) ? std::string(".") : name.substr(0, scp);
}


/** COMMONTOALL : START */

///
/// Replaces (Translates) all instances of inputChar in the string pszInString with outputChar. This is
/// an inplace replacement; no new memory is allocated. Equivalent to perl's tr// operator.
///
SVERROR ReplaceChar(char *pszInString, char inputChar, char outputChar) {
    int i;

    if (NULL == pszInString) {
        return EINVAL;
    }

    for (i=0; '\0' != pszInString[i]; i++) 
    {
        if (pszInString[i] == inputChar) {
            pszInString[i] = outputChar;
        }
    }
    return SVS_OK;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */

///
/// Appends the input character and a null terminator 
/// to the input string
///
SVERROR AppendChar(char *pszInString, char inputChar) {
    unsigned int i;

    if (!pszInString)
    { 
        return SVE_FAIL;
    }

    if (NULL == pszInString) {
        DebugPrintf( SV_LOG_ERROR, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_ERROR, "FAILED in AppendChar(), pszInString == NULL\n");
        return EINVAL;
    }

    for (i=0; '\0' != pszInString[i]; i++) 
    {
    }

    // Warning 
    if (i > 10000) 
    {
        DebugPrintf( SV_LOG_WARNING, "@ LINE %d in FILE %s \n", __LINE__, __FILE__ );
        DebugPrintf( SV_LOG_WARNING, "WARNING Possibly a bad input string?, i = %d\n", i);
    }
    pszInString[i] = inputChar;
    pszInString[i+1] = '\0';

    return SVS_OK;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
bool IsColon( std::string::value_type ch ) { return ':' == ch; }
bool IsQuestion( std::string::value_type ch ) { return '?' == ch; }
/** COMMONTOALL : END */


/** COMMONTOALL : START */
bool RegExpMatch(const char *srchptrn, const char *input)
{
    char *p;
    char t;
    const char *ps;
    char c;

    for (ps = input;;)
    {
        switch (c = *srchptrn++) 
        {
            case '\0':
                return (*input == '\0');
            case '?':
                if (*input == '\0')
                {
                    return false;
                }
                if ((*input == '.') && (input == ps))
                {
                    return false;
                }
                ++input;
                break;
            case '*':
                c = *srchptrn;
                while (c == '*')
                {
                    c = *++srchptrn;
                }
                if ((*input == '.') && (input == ps))
                {
                    return false;
                }
                if (c == '\0') 
                {
                    return true;
                }
                while ((t = *input) != '\0') 
                {
                    if (RegExpMatch(srchptrn, input))
                    {
                        return true;
                    }
                    ++input;
                }
                return false;
            case '[':
                if (*input == '\0')
                {
                    return false;
                }
                if ((*input == '.') && (input == ps))
                {
                    return false;
                }
                switch (DoesRegExpRangeMatch(srchptrn, *input,&p)) 
                {
                    case -1:
                        goto regular;
                    case 1:
                        srchptrn = p;
                        break;
                    case 0:
                        return false;
                }
                ++input;
                break;
            case '\\':
                if ((c = *srchptrn++) == '\0') 
                {
                    c = '\\';
                    --srchptrn;
                }
            default:
regular:
            /* The regular string */
            if (c != *input )
            {
                return false;
            }
            ++input;
            break;
        }
    }
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
svector_t split(const std::string &arg,
                const std::string &delim,
                const int numFields) 
{

    svector_t splitList;
    int delimetersFound = 0;
    std::string::size_type delimLength = delim.length();
    std::string::size_type argEndIndex   = arg.length();
    std::string::size_type front = 0;

    //
    // Find the location of the first delimeter.
    //
    std::string::size_type back = arg.find(delim, front);

    while( back != std::string::npos ) {
        ++delimetersFound;

        if( (numFields) && (delimetersFound >= numFields) ) {
            break;
        }

        //
        // Put the first (left most) field of the string into
        // the vector of strings.
        //
        // Example:
        //
        //      field1:field2:field3
        //      ^     ^
        //      front back
        //
        // The substr() call will take characters starting
        // at front extending to the length of (back - front).
        //
        // This puts 'field1' into the splitList.
        //
        splitList.push_back(arg.substr(front, (back - front)));

        //
        // Get the front of the second field in arg.  This is done
        // by adding the length of the delimeter to the back (ending
        // index) of the previous find() call.
        //
        // This makes front point to the first character after
        // the delimeter.
        //
        // Example:
        //
        //      'field1:field2:field3'
        //             ^^
        //         back front
        //
        //       delimLength = 1
        //
        front = back + delimLength;

        //
        // Find the location of the next delimeter.
        //
        back = arg.find(delim, front);
    }

    //
    // After we get through the entire string, there may be data at the
    // end of the arg that has not been stored.  The data will be
    // the trailing remnant (or the entire string if no delimeter was found).
    // So put the "remainder" into the splitList (this would add 'field3'
    // to the list.)
    //
    // Example:
    //
    //      'field1:field2:field3'
    //                     ^     ^
    //                 front     argEndIndex
    //
    // (argEndIndex - front) yeilds the number of elements to grab.
    //
    if( front < argEndIndex ) {
        splitList.push_back(arg.substr(front, (argEndIndex - front)));
    }

    return splitList;

}


/** COMMONTOALL : START */
int DoesRegExpRangeMatch(const char *srchptrn, char verify, char **ptrcurr)
{
    char eachchar;
    char tmpchar;
    int fine;
    int reverse;

    if ((reverse = (*srchptrn == '!' || *srchptrn == '^')))
    {
        ++srchptrn;
    }
    fine = 0;
    eachchar = *srchptrn++;
    do {
        if (eachchar == '\\')
        {
            eachchar = *srchptrn++;
        }
        if (eachchar == '\0')
        {
            return -1;
        }
        if ((*srchptrn == '-') && ((tmpchar = *(srchptrn+1)) != '\0') && (tmpchar != ']')) 
        {
            srchptrn += 2;
            if (tmpchar == '\\' )
            {
                tmpchar = *srchptrn++;
            }
            if (tmpchar == '\0')
            {
                return -1;
            }
            if (eachchar <= verify && verify <= tmpchar)
            {
                fine = 1;
            }
        } 
        else if (eachchar == verify)
        {
            fine = 1;
        }
    } while ((eachchar = *srchptrn++) != ']');
    *ptrcurr = (char *)(srchptrn);

    return ((fine == reverse)?0:1);
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
SVERROR CleanupDirectory( const char *pszDirectory, const char *pszFilePattern )
{
    SVERROR sve = SVS_OK;

    do
    {
        if( ( NULL == pszDirectory ) || ( NULL == pszFilePattern ) )
        {
            sve = EINVAL;
            break;
        }

        //Open directory
        // PR#10815: Long Path support
        ACE_DIR *dirp = sv_opendir(pszDirectory);

        if (dirp == NULL)
        {
            int lasterror = ACE_OS::last_error();
            if (ENOENT == lasterror || ESRCH == lasterror || EXDEV == lasterror)
            {
                sve = SVS_OK;
            } else 
            {
                sve = lasterror;
                DebugPrintf(SV_LOG_ERROR, "opendir failed, path = %s, errno = 0x%x\n", pszDirectory, ACE_OS::last_error()); 
            }
            break;
        }

        struct ACE_DIRENT *dp = NULL;

        do
        {
            ACE_OS::last_error(0);

            //Get directory entry
            if ((dp = ACE_OS::readdir(dirp)) != NULL)
            {
                //Find a matching entry
                if (RegExpMatch(pszFilePattern, ACE_TEXT_ALWAYS_CHAR(dp->d_name)))
                {
                    std::string filePath;

                    filePath = pszDirectory;                    
                    filePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
                    filePath += ACE_TEXT_ALWAYS_CHAR(dp->d_name);

                    //Delete file
                    // PR#10815: Long Path support
                    if (ACE_OS::unlink(getLongPathName(filePath.c_str()).c_str()) != 0)
                    {
                        DebugPrintf(SV_LOG_ERROR, "unlink failed, path = %s, errno = 0x%x\n", filePath.c_str(), ACE_OS::last_error()); 
                        sve = ACE_OS::last_error();
                    }
                }
            }
        } while(dp);

        //Close directory
        ACE_OS::closedir(dirp);

    } while(false);

    return( sve );
}

bool sv_get_filecount_spaceusage(const char * directory, const char * file_pattern,
                      SV_ULONGLONG & filecount, SV_ULONGLONG & size_on_disk ,SV_ULONGLONG& logicalsize)
{
    bool rv = true;
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s \n", FUNCTION_NAME);

    do
    {
        size_on_disk = 0;
        filecount = 0;
        logicalsize = 0;

        if( ( NULL == directory ) || ( NULL == file_pattern ) )
        {
            DebugPrintf(SV_LOG_ERROR, "FUNCTION:%s invalid (null) input.\n",
                FUNCTION_NAME);
            rv = false;
            break;
        }

        // open directory
        // PR#10815: Long Path support
        ACE_DIR *dirp = sv_opendir(directory);

        if (dirp == NULL)
        {
            int lasterror = ACE_OS::last_error();
            if (ENOENT == lasterror || ESRCH == lasterror || EXDEV == lasterror)
            {
                size_on_disk = 0;
                filecount = 0;
                rv = true;
            } else 
            {
                rv = false;
                DebugPrintf(SV_LOG_ERROR, 
                    "FUNCTION:%s opendir failed, path = %s, errno = %d\n", 
                    FUNCTION_NAME, directory, ACE_OS::last_error()); 
            }
            break;
        }

        struct ACE_DIRENT *dp = NULL;

        do
        {
            ACE_OS::last_error(0);

            // Get directory entry
            if ((dp = ACE_OS::readdir(dirp)) != NULL)
            {
                //Find a matching entry
                if (RegExpMatch(file_pattern, ACE_TEXT_ALWAYS_CHAR(dp->d_name)))
                {
                    std::string filePath;

                    filePath = directory;                    
                    filePath += ACE_DIRECTORY_SEPARATOR_CHAR_A;
                    filePath += ACE_TEXT_ALWAYS_CHAR(dp->d_name);
                    size_on_disk += File::GetSizeOnDisk(filePath);
                    ACE_stat statbuf = {0};
                    if(sv_stat(getLongPathName(filePath.c_str()).c_str(), &statbuf) != 0)
                    {
                        DebugPrintf(SV_LOG_ERROR, "FUNCTION:%s Failed to get file :%s statistics. errno = %d.\n",
                            FUNCTION_NAME,filePath.c_str(), ACE_OS::last_error());
                        continue;
                        //rv = false;
                        //break;
                    }
                    else
                    {
                        logicalsize += statbuf.st_size;
                    }
                    filecount += 1;
                }
            }
        } while(dp);

        //Close directory
        ACE_OS::closedir(dirp);

        DebugPrintf(SV_LOG_DEBUG, "FUNCTION:%s directory:%s pattern:%s, size_on_disk =" ULLSPEC " \n",
            FUNCTION_NAME, directory,file_pattern, size_on_disk);

    } while(false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s \n", FUNCTION_NAME);
    return rv;
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
///
/// Returns true if the string ends with the ending string argument
///
bool endsWith( const char* str, const char* ending, bool caseSensitive )
{
    bool rc = false;
    int const stringLength = strlen( str );
    int const endingLength = strlen( ending );

    if( stringLength >= endingLength )
    {
        char const* stringEnd = str + stringLength - endingLength;

        rc =  0 == (caseSensitive ? strcmp( stringEnd, ending ) : strcmpi( stringEnd, ending ) );
    }

    return( rc );
}

/** COMMONTOALL : END */



/** COMMONTOALL : START */
static int mkdirRecursiveHelper( char* pathname, mode_t mode ) 
{
    // PR#10815: Long Path support
    if( ACE_OS::mkdir( getLongPathName(pathname).c_str(), mode ) == 0 )
    {
        return 0;
    }

    switch( errno )
    {
    default:
        break;
    case EEXIST:
        {
            ACE_stat stat;
            // PR#10815: Long Path support
            if (sv_stat(getLongPathName(pathname).c_str(), &stat) == 0)
            {
                errno = ((stat.st_mode & S_IFDIR) == S_IFDIR) ? EEXIST : ENOTDIR;
                //errno = S_ISDIR(stat.st_mode) ? EEXIST : ENOTDIR;
            }
            break;
        }
    case ENOENT:
    case ENOTDIR:
        char* last_sep( ACE_OS::strrchr(
            pathname, ACE_DIRECTORY_SEPARATOR_CHAR_A ) );
        if( 0 != last_sep && pathname != last_sep )
        {
            *last_sep = '\0';
            int result = mkdirRecursive( pathname, mode );
            *last_sep = ACE_DIRECTORY_SEPARATOR_CHAR_A;
            if( 0 == result || EEXIST == errno )
            {
                // Now that the parent is made, try to remake this one
                // PR#10815: Long Path support
                return ACE_OS::mkdir( getLongPathName(pathname).c_str(), mode );
            }
        }
        break;
    }
    return -1; 
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
///
/// Creates a directory even if the parent directories do not exist
/// E.g. creates /tmp/parent/mydir  even if /tmp/parent is missing.
/// NOTE: On Windows, doesn't respect / as a separator
///
int mkdirRecursive( char const* pathname, mode_t mode )  
{
    char buffer[ MAXPATHLEN + 1 ];
    if( strlen( pathname ) > sizeof( buffer ) - 1 )
    {
        errno = ENAMETOOLONG;
        return -1;
    }
    else 
    {
        inm_strcpy_s( buffer, ARRAYSIZE(buffer), pathname );
    }

    return mkdirRecursiveHelper( buffer, mode ); 
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
// Function moved from CDPCli
void GetConsoleYesOrNoInput(char & ch)
{
    ch = getchar();
    while((ch != 'y') && (ch != 'n') && (ch != 'Y') && (ch != 'N'))
    {
        fflush(stdin);
        printf("Press y/Y/N/n....\n");
        ch = getchar();
    }
}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
std::string getLocalTime()
{
    char szLocalTime[90];
    tm *today;
    time_t ltime;

    time( &ltime );
    today = localtime(&ltime);

    inm_sprintf_s(szLocalTime, ARRAYSIZE(szLocalTime), "20%02d-%02d-%02d %02d:%02d:%02d",
        today->tm_year - 100,
        today->tm_mon + 1,
        today->tm_mday,
        today->tm_hour,
        today->tm_min,
        today->tm_sec
        );
    DebugPrintf(SV_LOG_DEBUG,"[Portablehelpers::RegisterHost():getLocalTime()] AgentLocalTime = %s\n", szLocalTime);

    return std::string(szLocalTime);
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
std::string getTimeZone()
{
    static std::string zone;
    if(!zone.empty())
        return zone;
    int  zonediff;
    char zonesign;
    char zonebuf[10];
    time_t t, lt, gt; 

    t  = time(NULL);
    lt = mktime (localtime(&t));
    gt = mktime (gmtime(&t));
    zonediff = (int)difftime(lt, gt);

    zonesign = (zonediff >= 0)? '+' : '-';
    inm_sprintf_s(zonebuf, ARRAYSIZE(zonebuf), "%c%.2d%.2d", zonesign, abs(zonediff) / 3600, abs(zonediff) % 3600 / 60);
    zone = std::string(zonebuf);
    DebugPrintf(SV_LOG_DEBUG,"[Portablehelpers::RegisterHost():getTimeZone()] AgentZone = %s\n", zonebuf);
    return zone;
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
// Prepare patch_history with fields seperated by ',' & records seperated by '|'
std::string getPatchHistory(std::string pszInstallPath)
{
    char line[2001];
    std::fstream fp;
    std::string patchfile, patch_history;

    patchfile = pszInstallPath + PATCH_FILE;
    fp.open (patchfile.c_str(), std::fstream::in);

    if(!fp.fail())
    {
        while(true)
        {
            fp.getline(line, sizeof(line));
            if (0 == fp.gcount())
                break;
            if(line[0] == '#' )
                continue;
            if(patch_history.empty())
                patch_history  = std::string(line) ;
            else 
                patch_history += '|' + std::string(line) ;
        }
        fp.close();
        DebugPrintf(SV_LOG_DEBUG,"[Portablehelpers::RegisterHost()]PatchHistory= %s\n", patch_history.c_str());
    }
    else
        DebugPrintf(SV_LOG_DEBUG,"[Portablehelpers::RegisterHost()] %s File cannot be opened \n", patchfile.c_str());

    return patch_history;
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
std::string ToUpper(const std::string& toConvert)
{
    std::string sUpper = toConvert; 
    const int length = sUpper.length();
    for(int i=0; i!=length ; ++i)
    {
        sUpper[i] = std::toupper(sUpper[i]);
    }
    return sUpper;
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
std::string ToLower(const std::string& toConvert)
{

    std::string sLower = toConvert; 
    const int length = sLower.length();
    for(int i=0; i!=length ; ++i)
    {
        sLower[i] = std::tolower(sLower[i]);
    }
    return sLower;

}
/** COMMONTOALL : END */


/** COMMONTOALL : START */
/** Added by BSR - Issue#5288 **/
bool resolvePathAndVerify( std::string& pathName ) 
{
    bool bReturn = false ;

    // PR#10815: Long Path support
#if defined (SV_WINDOWS) && defined (SV_USES_LONGPATHS)
    ACE_TCHAR *resolvedPath = NULL;

    // ACE_OS::realpath calls _wfullpath on windows. This function will malloc the buffer of sefficient size
    if( (resolvedPath = ACE_OS::realpath( ACE_TEXT_CHAR_TO_TCHAR(pathName.c_str()), NULL )) != NULL )
    {
        ACE_stat stat ;
        if( sv_stat( getLongPathName(ACE_TEXT_ALWAYS_CHAR(resolvedPath)).c_str(), &stat ) == 0 )
        {
            bReturn = true ;
            pathName = ACE_TEXT_ALWAYS_CHAR(resolvedPath) ;
        }        
    }

    free(resolvedPath);
    resolvedPath = NULL;

#else

    ACE_TCHAR resolvedPath[SV_MAX_PATH];

    // ACE::realpath calls realpath() on linux/solaris. realpath() on solaris fails if any
    // of the pointer passed as arg is NULL. So we provide the buffer.
    if( ACE_OS::realpath( ACE_TEXT_CHAR_TO_TCHAR(pathName.c_str()), resolvedPath ) != NULL )
    {
        ACE_stat stat ;
        if( sv_stat( getLongPathName(ACE_TEXT_ALWAYS_CHAR(resolvedPath)).c_str(), &stat ) == 0 )
        {
            bReturn = true ;
            pathName = ACE_TEXT_ALWAYS_CHAR(resolvedPath) ;
        }        
    }

#endif

    return bReturn ;
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */

// Bug# 5527 Added SVUnlink to be used by DeleteVsnapLogs 

/*
* FUNCTION NAME :     SVUnlink
*
* DESCRIPTION :  Deletes the file specified by filename and prints the errno 
*                and error string if failed to delete
*
* INPUT PARAMETERS :  filename - the file to delete
*
* OUTPUT PARAMETERS :  None
*
* NOTES : If the specified file doesn't exist then it considers 
*         it as deleted and returns true.
*
* return value : true if the file was deleted, otherwise false is returned
*
*/

bool SVUnlink( const std::string& filename )
{
    // PR#10815: Long Path support
    if(0!=ACE_OS::unlink(getLongPathName(filename.c_str()).c_str()))
    {
        int err = ACE_OS::last_error();
        if(err != ENOENT)
        {
            std::stringstream msg;
            msg << "Error deleting file " << filename << ", errno = " << err << std::endl
                << "FAILED @ LINE " << __LINE__ << " in FUNCTION " << __FUNCTION__ << " in FILE "
                << __FILE__ << std::endl;
            DebugPrintf(SV_LOG_ERROR,"%s", msg.str().c_str());
            ACE_OS::last_error(0);
            return false;
        }
    }
    return true;
}
/** COMMONTOALL : END */



/** COMMONTOALL : START */
void TruncateTrailingSlash(char *str)
{
    size_t Length = strlen(str);

    Length--;
    while (((str[Length]=='\\') && Length)  || ((str[Length]=='/') && Length))
    {
        str[Length] = '\0';
        Length--;
    }

    return ;
}
/** COMMONTOALL : END */

int RegisterHost(const std::string & onDemandRegistrationRequestId, 
                 const std::string & disksLayoutOption,
                 QuitFunction_t qf)
{
    boost::mutex::scoped_lock guard(g_RegisterHostMutex);
    DebugPrintf( SV_LOG_DEBUG,"ENTERED: RegisterHost with on demand request id: %s, disks layout option: %s, qf = %s\n", 
                              onDemandRegistrationRequestId.c_str(), disksLayoutOption.c_str(), (qf?"set":"not set"));
    int ret = 0 ;
    try{
        Configurator* TheConfigurator = NULL;
        if(!GetConfigurator(&TheConfigurator))
        {
            DebugPrintf(SV_LOG_DEBUG, "Unable to obtain configurator %s %d", FUNCTION_NAME, LINE_NO);
            DebugPrintf(SV_LOG_DEBUG,"EXITED: RegisterHost\n" );
            ret = -1;
            return ret;
        }
        // version info
        std::string agentVersion(InmageProduct::GetInstance().GetProductVersionStr());
        std::string driverVersion;
        if (TheConfigurator->getVxAgent().isMobilityAgent())
        {
            driverVersion = InmageProduct::GetInstance().GetDriverVersion();
            if (driverVersion.empty())
            {
                DebugPrintf(SV_LOG_ERROR, "%s: failed to get driver version.\n", FUNCTION_NAME);
                ret = -1;
                return ret;
            }
        }

        // hostname
        std::string hostName;
        if( TheConfigurator->getVxAgent().getUseConfiguredHostname() ) {
            hostName = TheConfigurator->getVxAgent().getConfiguredHostname();
        }
        else {
            hostName = Host::GetInstance().GetHostName();
        }

        // ip address
        std::string ipAddress;
        if( TheConfigurator->getVxAgent().getUseConfiguredIpAddress() ) {
            ipAddress = TheConfigurator->getVxAgent().getConfiguredIpAddress();
        }
        else {
            ipAddress = Host::GetInstance().GetIPAddress();
            if (ipAddress.empty())
            {
                DebugPrintf(SV_LOG_ERROR, "RegisterHost failed: ipAddress blank.\n");
                ret = -1;
                return ret;
            }
        }

        static HypervisorInfo_t hypervinfo;
        if (HypervisorInfo_t::HypervisorStateUnknown == hypervinfo.state)
        {
            GetHypervisorInfo(hypervinfo);
        }
        PrintHypervisorInfo(hypervinfo);

        NicInfos_t nicinfos;
        GetNicInfos(nicinfos);
        if (!HasNicInfoPhysicaIPs(nicinfos))
        {
            std::stringstream sserrormsg;
            sserrormsg << "No valid physical IP found in available nicinfos.";
            sserrormsg << " Possible cause is one of Tcpip interface in registry path";
            sserrormsg << " \"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\\"";
            sserrormsg << " does not have valid DhcpIPAddress but EnableDHCP is set.";
            DebugPrintf(SV_LOG_ERROR, "%s: %s\n", FUNCTION_NAME, sserrormsg.str().c_str());
            DebugPrintf(SV_LOG_ALWAYS, "%s: Printing discovered nicinfos,\n", FUNCTION_NAME);
            PrintNicInfos(nicinfos, SV_LOG_ALWAYS);
            throw ERROR_EXCEPTION << sserrormsg.str() << '\n';
        }
        PrintNicInfos(nicinfos);

        DebugPrintf("sending hostname = %s, IP address = %s to CS\n",
            hostName.c_str(), ipAddress.c_str());

        // Mount bootable volumes so that their vol info is also collected
        mountBootableVolumesIfRequried();

        // volume information
        VolumeSummaries_t volumeSummaries;
        VolumeDynamicInfos_t volumeDynamicInfos;

        VolumeInfoCollector volumeCollector((DeviceNameTypes_t)GetDeviceNameTypeToReport(hypervinfo.name));
        volumeCollector.GetVolumeInfos(volumeSummaries, volumeDynamicInfos, true); // TODO: should have a configurator option to deterime
        std::string errormsg;
        if (TheConfigurator->getVxAgent().isMobilityAgent() && !IsSystemAndBootDiskSame(volumeSummaries, errormsg))
        {
            throw ERROR_EXCEPTION << errormsg << '\n';
        }

        Options_t miscInfo;
        if (!onDemandRegistrationRequestId.empty())
        {
            miscInfo.insert(std::make_pair( NameSpaceInitialSettings::ON_DEMAND_REGISTRATION_REQUEST_ID_KEY, onDemandRegistrationRequestId ));
        }
        boost::shared_ptr<DisksLayoutUpdater> dlu(new DisksLayoutUpdaterImp(TheConfigurator));
        dlu->Update(disksLayoutOption, miscInfo);
        TheConfigurator->getVxAgent().insertRole(miscInfo);
        miscInfo.insert(std::make_pair(NameSpaceInitialSettings::RESOURCE_ID_KEY, TheConfigurator->getVxAgent().getResourceId()));

        miscInfo.insert(std::make_pair(KEY_MT_SUPPORTED_DATAPLANES, TheConfigurator->getVxAgent().getMTSupportedDataPlanes()));
        std::string externalIpAddress = TheConfigurator->getVxAgent().getExternalIpAddress();
        if (!externalIpAddress.empty())
        {
            miscInfo.insert(std::make_pair(external_ip_address, externalIpAddress));
        }
#ifdef WIN32
        if (TheConfigurator->getVxAgent().isMasterTarget())
        {
            std::list<InstalledProduct> marsProduct;
            if (GetMarsProductDetails(marsProduct))
            {
                DebugPrintf(SV_LOG_DEBUG, "Number of products received : %d\n", marsProduct.size());
                marsProduct.sort(CompareProudctVersion);
                marsProduct.unique();

                if (1 == marsProduct.size())
                {
                    DebugPrintf(SV_LOG_DEBUG, "Mars Agent Version: %s\n", marsProduct.begin()->version.c_str());
                    miscInfo.insert(make_pair("MarsAgentVersion", marsProduct.begin()->version));
                }
                else
                {
                    if (marsProduct.size())
                    {
                        DebugPrintf(SV_LOG_ERROR, "More than one MARS agent has been discovered on MT, not sending the MARS agent verison details.\n");
                        DumpProductDetails(marsProduct);
                    }
                    else
                    {
                        DebugPrintf(SV_LOG_ERROR, "No MARS agent has been discovered on MT, not sending the MARS agent verison details.\n");
                    }
                }
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "Failed to discover MARS agent details on MT, not sending the MARS agent details.\n");
            }

            
        }
#endif
        miscInfo.insert(std::make_pair("FirmwareType", (IsUEFIBoot()) ? "UEFI" : "BIOS"));

        miscInfo.insert(std::make_pair("SystemUUID", GetSystemUUID()));
        std::string fqdn;

#ifdef SV_WINDOWS
        fqdn = Host::GetInstance().GetFQDN();
#else
        fqdn = Host::GetInstance().GetHostName();
#endif
        miscInfo.insert(std::make_pair("FQDN", fqdn.empty() ? hostName : fqdn));

        DebugPrintf(SV_LOG_DEBUG, "MiscInfo map:\n");
        PrintOptions(miscInfo);

        std::string installPath = TheConfigurator->getVxAgent().getInstallPath();

        // System Volume, Capacity, FreeSpace

        std::string sysVol;
        unsigned long long sysVolFreeSpace = 0;
        unsigned long long sysVolCapacity = 0;
        unsigned long long insVolFreeSpace = 0;
        unsigned long long insVolCapacity = 0;

        /** 
        * Defined function that will give system volume
        * , capacity and freespace based on platform
        */

        if (!GetSysVolCapacityAndFreeSpace(sysVol, sysVolFreeSpace, sysVolCapacity))
        {
            DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, GetSysVolCapacityAndFreeSpace failed.\n", LINE_NO, 
                FILE_NAME);
            ret = -1 ;
            return ret;
        }

        /** 
        * Defined function that will give installation volume
        * , capacity and freespace based on platform
        */
        if (!GetInstallVolCapacityAndFreeSpace(installPath, insVolFreeSpace, insVolCapacity))
        {
            DebugPrintf(SV_LOG_ERROR, "FAILED: @LINE %d in FILE %s, GetInstallVolCapacityAndFreeSpace failed.\n", LINE_NO, 
                FILE_NAME);
            ret = -1 ;
            return ret;
        }
        std::string prod_version = PROD_VERSION;

        NwwnPwwns_t npwwns;
        GetNwwnPwwns(npwwns);
        PrintNPWwns(npwwns);

        std::map<std::string, std::string> acnvpairs;
        if (TheConfigurator->getVxAgent().getAccountInfo(acnvpairs))
        {
            DebugPrintf(SV_LOG_DEBUG, "account information:\n");
            InmPrint(acnvpairs);
        }
        else
        {
            DebugPrintf(SV_LOG_DEBUG, "could not read account information\n");
        }

        // Passing all the information to cx
        std::string agentMode = TheConfigurator->getVxAgent().getAgentMode();
        DebugPrintf(SV_LOG_DEBUG, "Agent Mode: %s\n", agentMode.c_str());
        DebugPrintf(SV_LOG_DEBUG, "Product Version: %s\n", prod_version.c_str() ); 
        DebugPrintf(SV_LOG_DEBUG, "Agent Version: %s\n", agentVersion.c_str() ); 
        DebugPrintf(SV_LOG_DEBUG, "Driver Version: %s\n", driverVersion.c_str());

        const CpuInfos_t & cpuinfos = Host::GetInstance().GetCpuInfos();
        const Object & osinfo = OperatingSystem::GetInstance().GetOsInfo();
        const ENDIANNESS & e = OperatingSystem::GetInstance().GetEndianness();
        const OS_VAL & osval = OperatingSystem::GetInstance().GetOsVal();
        const unsigned long long memory = Host::GetInstance().GetAvailableMemory();
        const int RETRY_INTERVAL_IN_SECONDS = 120;
        bool shouldRetry = !TheConfigurator->getCxAgent().IsHostStaticInfoRegisteredAtleastOnce();

        const uint32_t maxRetryTimeInSeconds = TheConfigurator->getVxAgent().getRegisterHostInterval();
        steady_clock::time_point endTime = steady_clock::now();
        endTime += seconds(maxRetryTimeInSeconds);

        DebugPrintf(SV_LOG_DEBUG, "Retry immediately = %s.\n", STRBOOL(shouldRetry));
        do {
            ConfigureCxAgent::RegisterHostStatus_t rgss = TheConfigurator->getCxAgent().RegisterHostStaticInfo(
                !onDemandRegistrationRequestId.empty(),
                agentVersion,
                driverVersion,
                hostName,
                ipAddress,
                osinfo,
                CurrentCompatibilityNum(),
                TheConfigurator->getVxAgent().getInstallPath(),
                osval,
                getTimeZone(),
                sysVol,
                getPatchHistory(TheConfigurator->getVxAgent().getInstallPath()),
                volumeSummaries,
                agentMode,
                prod_version,
                nicinfos,
                hypervinfo,
                npwwns,
                e,
                cpuinfos,
                acnvpairs,
                memory,
                miscInfo
                );

            std::string rgsstr = TheConfigurator->getCxAgent().GetRegisterHostStatusStr(rgss);
            DebugPrintf(SV_LOG_DEBUG, "RegisterHostStaticInfo returned %s\n", rgsstr.c_str());
            if (ConfigureCxAgent::HostRegistered != rgss)
            {
                DebugPrintf(SV_LOG_ERROR, "RegisterHostStaticInfo failed with %s. Immediate retry with interval %d seconds will occur in case host did not register atleast once.\n", rgsstr.c_str(), RETRY_INTERVAL_IN_SECONDS);
                HTTP_CONNECTION_SETTINGS s = TheConfigurator->getVxTransport().getHttp();
                VxPeriodicRegistrationFailedAlert a(GetCxIpAddress(), s.port, TheConfigurator->getVxTransport().IsHttps());
                SendAlertToCx(SV_ALERT_CRITICAL, SV_ALERT_MODULE_HOST_REGISTRATION, a);
                ret = -1;

                shouldRetry = shouldRetry && (steady_clock::now() < endTime);
                DebugPrintf(SV_LOG_DEBUG, "Retry register host = %s.\n", STRBOOL(shouldRetry));
            }
            else
            {
                DebugPrintf(SV_LOG_DEBUG, "RegisterHostStaticInfo succeeded\n");
                dlu->Clear(miscInfo);
                ret = 0;
                break;
            }
        } while (shouldRetry && qf && !qf(RETRY_INTERVAL_IN_SECONDS));

        if (0 == ret) { // do dynamic registration when static succeeds or static info checksum match
            do {
                ConfigureCxAgent::RegisterHostStatus_t rgsd = TheConfigurator->getCxAgent().RegisterHostDynamicInfo(
                    getLocalTime(),
                    sysVolCapacity,
                    sysVolFreeSpace,
                    insVolCapacity,
                    insVolFreeSpace,
                    volumeDynamicInfos
                    );

                std::string rgsdstr = TheConfigurator->getCxAgent().GetRegisterHostStatusStr(rgsd);
                DebugPrintf(SV_LOG_DEBUG, "RegisterHostDynamicInfo returned %s. \n", rgsdstr.c_str());
                if (ConfigureCxAgent::HostRegistered != rgsd)
                {
                    DebugPrintf(SV_LOG_ERROR, "RegisterHostDynamicInfo failed with %s. Immediate retry with interval %d seconds will occur in case host did not register atleast once.\n", rgsdstr.c_str(), RETRY_INTERVAL_IN_SECONDS);
                    ret = -1;

                    shouldRetry = shouldRetry && (steady_clock::now() < endTime);
                }
                else
                {
                    DebugPrintf(SV_LOG_DEBUG, "RegisterHostDynamicInfo succeeded\n");
                    ret = 0;
                    break;
                }
            } while (shouldRetry && qf && !qf(RETRY_INTERVAL_IN_SECONDS)); //Retry dynamic call also in case host static registration never happened earlier
        }
    }
    catch( std::string const& e ) {
        ret = -1;
        DebugPrintf( SV_LOG_ERROR,"FAILED: RegisterHost: exception %s\n", e.c_str() );
    }
    catch( char const* e ) {
        ret = -1;
        DebugPrintf( SV_LOG_ERROR,"FAILED: RegisterHost: exception %s\n", e );
    }
    catch( std::exception const& e ) {
        ret = -1;
        DebugPrintf( SV_LOG_ERROR,"FAILED: RegisterHost: exception %s\n", e.what() );
    }
    catch(...) {
        ret = -1;
        DebugPrintf( SV_LOG_ERROR,"FAILED: RegisterHost: exception...\n" );
    }
    DebugPrintf( SV_LOG_DEBUG,"EXITED: RegisterHost %s with return value = %d\n", onDemandRegistrationRequestId.c_str(), ret);
    return ret;
}


bool IsDirectoryExisting(const std::string& path)
{
    std::string sDir = path;

    if ( !sDir.empty())
    {
        ToStandardFileName(sDir);
        ACE_stat fileStat;

        // PR#10815: Long Path support
        if ( sv_stat(getLongPathName(sDir.c_str()).c_str(),&fileStat) >= 0 )
        {
            if ( fileStat.st_mode & S_IFDIR )
                return true;
            else
                return false;
        }
        else
            return false;
    }
    return false;

}


/*
* FUNCTION NAME : volnamecmp::operator()
*
* DESCRIPTION :  
*
*  compare two volume names
*
*  windows:
*    based on volume guid
*
*  unix:
*   based on real path
*             
* INPUT PARAMETERS : name of volumes to compare
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
* return value : true if both represent same volume, false otherwise
*
*/
bool volnamecmp::operator() (const std::string & vol1, const std::string & vol2) const
{
    std::string s1 = vol1;
    std::string s2 = vol2;

    // Convert the volume name to a standard format
    FormatVolumeName(s1);
    FormatVolumeNameToGuid(s1);
    ExtractGuidFromVolumeName(s1);

    // convert to standard device name if symbolic link 
    // was passed as argument

    /** 
    *
    * TODO: 
    * Currently not calling GetDeviceNameFromSymLink in solaris
    * since link name is what CX has and if we operate on actual
    * device name, calls to CX are failing. This has to be handled
    * in a more neet manner.
    * 
    */

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s1);
    }

    // Convert the volume name to a standard format
    FormatVolumeName(s2);
    FormatVolumeNameToGuid(s2);
    ExtractGuidFromVolumeName(s2);

    // convert to standard device name if symbolic link 
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s2);
    }

    /** 
    * Wrapper function that will call strcmp on
    * unix and stricmp on windows 
    */
    return comparevolnames(s1.c_str(), s2.c_str()) < 0;
}


/*
* FUNCTION NAME : volequalitycmp::operator()
*
* DESCRIPTION :  
*
*  compare two volume names
*
*  windows:
*    based on volume guid
*
*  unix:
*   based on real path
*             
* INPUT PARAMETERS : name of volumes to compare
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
* return value : true if both represent same volume, false otherwise
*
*/
bool volequalitycmp::operator() (const std::string & vol1, const std::string & vol2) const
{
    std::string s1 = vol1;
    std::string s2 = vol2;

    // Convert the volume name to a standard format
    FormatVolumeName(s1);
    FormatVolumeNameToGuid(s1);
    ExtractGuidFromVolumeName(s1);

    // convert to standard device name if symbolic link 
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s1);
    }

    // Convert the volume name to a standard format
    FormatVolumeName(s2);
    FormatVolumeNameToGuid(s2);
    ExtractGuidFromVolumeName(s2);

    // convert to standard device name if symbolic link 
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s2);
    }

    /** 
    * Wrapper function that will call strcmp on
    * unix and stricmp on windows 
    */
    return (!comparevolnames(s1.c_str(), s2.c_str()));
}


/*
* FUNCTION NAME :  sameVolumecheck
*
* DESCRIPTION : checks if the specified volumes are same
*
*
* INPUT PARAMETERS : none
*
* OUTPUT PARAMETERS : none
*
* NOTES :
*
*
* return value : true on success, otherwise false
*
*/
bool sameVolumeCheck(const std::string & vol1, const std::string & vol2)
{
    std::string s1 = vol1;
    std::string s2 = vol2;

    // Convert the volume name to a standard format
    FormatVolumeName(s1);
    FormatVolumeNameToGuid(s1);
    ExtractGuidFromVolumeName(s1);

    // convert to standard device name if symbolic link
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s1);
    }

    // Convert the volume name to a standard format
    FormatVolumeName(s2);
    FormatVolumeNameToGuid(s2);
    ExtractGuidFromVolumeName(s2);

    // convert to standard device name if symbolic link
    // was passed as argument

    if (IsReportingRealNameToCx())
    {
        GetDeviceNameFromSymLink(s2);
    }
    return (s1 == s2);
}

/*
* FUNCTION NAME :     IsCacheVolume
*
* DESCRIPTION :  Determines if the volume is being used as a cache volume
*
* INPUT PARAMETERS :  volume name
*
* OUTPUT PARAMETERS : cache - set to true if volume is cache volume. otherwise false
*
* 
* return value : true if the function succeeds. else false
*
*/

bool IsCacheVolume(std::string const & volume)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s for %s\n", FUNCTION_NAME, volume.c_str());

    bool rv = false;

    do 
    {
        LocalConfigurator localConfigurator;
        std::string cacheDir = localConfigurator.getCacheDirectory();

        std::string cacheRoot;

        if(!GetVolumeRootPath(cacheDir, cacheRoot))
        {
            DebugPrintf(SV_LOG_DEBUG, "Unable to determine the root of %s\n", cacheDir.c_str());
            rv = true;
            break;
        }

        std::string cacheDevice;

        if(!GetDeviceNameForMountPt(cacheRoot, cacheDevice))
        {
            DebugPrintf(SV_LOG_DEBUG, "Unable to get the devicename for %s\n", cacheRoot.c_str());
            rv = true;
            break;
        }

        rv = sameVolumeCheck(volume, cacheDevice);

    } while (0);

    DebugPrintf( SV_LOG_DEBUG, "EXITED %s for %s\n", FUNCTION_NAME, volume.c_str());
    return rv;
}


/*
* FUNCTION NAME :     IsInstallPathVolume
*
* DESCRIPTION :  Determines if the volume is being used as a instalation path
*
* INPUT PARAMETERS :  volume name
*
* OUTPUT PARAMETERS : cache - set to true if volume is cache volume. otherwise false
*
* 
* return value : true if the function succeeds. else false
*
*/

bool IsInstallPathVolume(std::string const & volume)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s for %s\n", FUNCTION_NAME, volume.c_str());

    bool rv = false;
    do 
    {
        LocalConfigurator localConfigurator;
        std::string installDir = localConfigurator.getInstallPath();

        std::string installRoot;

        if(!GetVolumeRootPath(installDir, installRoot))
        {
            DebugPrintf(SV_LOG_DEBUG, "Unable to determine the root of %s\n", installDir.c_str());
            rv = true;
            break;
        }

        std::string installDevice;

        if(!GetDeviceNameForMountPt(installRoot, installDevice))
        {
            DebugPrintf(SV_LOG_DEBUG, "Unable to get the devicename for %s\n", installRoot.c_str());
            rv = true;
            break;
        }

        rv = sameVolumeCheck(volume, installDevice);

    } while (0);
    DebugPrintf( SV_LOG_DEBUG, "EXITED %s for %s\n", FUNCTION_NAME, volume.c_str());
    return rv;
}


bool IsPreAdded(const std::string &filename, const char &delim, std::string &prefile)
{
    bool bpreadded = false;

    prefile = filename;
    size_t basenamepos = prefile.rfind(delim);
    if (std::string::npos == basenamepos)
    {
        basenamepos = 0;
    }
    else
    {
        basenamepos++;
    }
    const char *p = prefile.c_str();
    size_t prelen = strlen(PREUNDERSCORE);

    if (*(p + basenamepos) != '\0')
    {
        if (strncmp(p + basenamepos, PREUNDERSCORE, prelen))
        {
            p = NULL;
            prefile.insert(basenamepos, PREUNDERSCORE);
            bpreadded = true;
        }
    }
    return bpreadded;
}


// PR#10815: Long Path support
ACE_DIR *sv_opendir(const char *filename)
{
#if defined (SV_USES_LONGPATHS)
    std::string fileName(filename);
    fileName += ACE_DIRECTORY_SEPARATOR_CHAR_A;
    return ACE_OS::opendir(getLongPathName(fileName.c_str()).c_str());
#else
    return ACE_OS::opendir(filename);
#endif
}


bool IsVirtual(std::string &hypervisorname, std::string &hypervisorvers)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bisvirtual = false;
    const std::string delim = ",";

    do 
    {
        LocalConfigurator lc;
        std::string vmtext = lc.getVMPats();
        std::vector<std::string> vmops;
        Tokenize(vmtext, vmops, delim);
        Pats pats(vmops);

        /* step 1: use some deterministic ways specific to OS */
        bisvirtual = IsVmDeterministic(pats, hypervisorname, hypervisorvers);
        if (bisvirtual) 
        {
            break;
        }

        /* step 2: use cmds that if succeed says vm */
        vmops.clear();
        vmtext = lc.getVMCmds();
        Tokenize(vmtext, vmops, delim);
        bisvirtual = IsAnyCmdSucceeded(vmops, hypervisorname);
        if (bisvirtual)
        {
            break;
        }

        /* step 3: if command output has pattern, then a vm */
        vmops.clear();
        vmtext = lc.getVMCmdsForPats();
        Tokenize(vmtext, vmops, delim);
        bisvirtual = HasPatternInCmd(vmops, pats, hypervisorname);
        if (bisvirtual)
        {
            break;
        }

        /* step 4: if wmi any column has pattern, then a vm */
        /* TODO: For now this code just returns false;
        *  since for a generic wmi to give information,
        *  we not only need columns but also their types
        vmops.clear();
        vmtext = lc.getVMWMIsForPats();
        Tokenize(vmtext, vmops, delim);
        bisvirtual = HasPatternInWMIDB(vmops, pats);
        */

    } while (false);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s IsVirtual: %s\n", FUNCTION_NAME, bisvirtual? "True":"False");

    return bisvirtual;
}


bool IsAnyCmdSucceeded(const std::vector<std::string> &cmds, std::string &cmd)
{
    bool bsucceeded = false;
    std::vector<std::string>::const_iterator iter = cmds.begin();

    for ( /* empty */ ; iter != cmds.end(); iter++)
    {
        std::string output, err;
        int ecode = 0;
        bsucceeded = RunInmCommand(*iter, output, err, ecode);
        if (bsucceeded)
        {
            cmd = *iter;
            break;
        }
    }

    return bsucceeded;
}


bool HasPatternInCmd(const std::vector<std::string> cmds, 
                     Pats &pats, std::string &pat)
{
    bool bhaspat = false;
    std::vector<std::string>::const_iterator iter = cmds.begin();

    for ( /* empty */ ; (iter != cmds.end()) && (false == bhaspat) ; iter++)
    {
        std::string output, err;
        int ecode = 0;
        bool bsucceeded = RunInmCommand(*iter, output, err, ecode);
        if (bsucceeded)
        {
            bhaspat = pats.IsMatched(output, pat);
        }
    }

    return bhaspat;
}

bool RunInmCommand(
                   const std::string& cmd,
                   std::string &outputmsg,
                   std::string& errormsg,
                   int & ecode
                   )
{
    bool rv = true;
    InmCommand mount(cmd);
    InmCommand::statusType status = mount.Run();
    if (status != InmCommand::completed)
    {
        if(!mount.StdErr().empty())
        {
            errormsg = mount.StdErr();
            ecode = mount.ExitCode();
        }
        if(!mount.StdOut().empty())
        {
            outputmsg = mount.StdOut();
        }
        rv = false;
    }
    else if (mount.ExitCode())
    {
        errormsg = mount.StdErr();
        ecode = mount.ExitCode();
        if(!mount.StdOut().empty())
        {
            outputmsg = mount.StdOut();
        }
        rv = false;
    }
    else
    {
        outputmsg = mount.StdOut();
    }
    return rv;
}


bool IsVmDeterministic(Pats &pats, std::string &hypervisorname, std::string &hypervisorvers)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    bool bisvm = false;

    bisvm = IsVmFromDeterministics(pats, hypervisorname, hypervisorvers);

    if (false == bisvm)
    {
        bisvm = IsNativeVm(pats, hypervisorname, hypervisorvers);
    }

    if (false == bisvm)
    {
        bisvm = IsOpenVzVM(pats, hypervisorname, hypervisorvers);
    }

    if (false == bisvm)
    {
        bisvm = IsVMFromCpuInfo(pats, hypervisorname, hypervisorvers);
    }

    if (false == bisvm)
    {
        bisvm = IsXenVm(pats, hypervisorname, hypervisorvers);
    }

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s IsVM = %s\n", FUNCTION_NAME, bisvm? "TRUE":"FALSE");

    return bisvm;
}


void PrintHypervisorInfo(const HypervisorInfo_t &hypervisorinfo)
{
    DebugPrintf(SV_LOG_DEBUG, "======\n");
    DebugPrintf(SV_LOG_DEBUG, "Hypervisor State: %s\n", StrHypervisorState[hypervisorinfo.state]);
    DebugPrintf(SV_LOG_DEBUG, "Hypervisor Name: %s\n", hypervisorinfo.name.c_str());
    DebugPrintf(SV_LOG_DEBUG, "Hypervisor Version: %s\n", hypervisorinfo.version.c_str());
    DebugPrintf(SV_LOG_DEBUG, "======\n");
}


bool IsValidIP(const std::string &ip)
{
    bool bisvalid = false;
    const size_t READABLE_IP_SIZE = 15;
    const size_t NTABS_INIP = 4;
    const size_t MAX_BYTE = 255;
    const char IP_TAB_SEP = '.';
    size_t iplen = ip.length();

    if (iplen && (iplen <= READABLE_IP_SIZE))
    {
        int a, b, c, d;
        a = b = c = d = -1;
        int rc = sscanf(ip.c_str(), "%3d.%3d.%3d.%3d", &a, &b, &c, &d);
        std::stringstream formedbackip;
        formedbackip << a << IP_TAB_SEP
            << b << IP_TAB_SEP
            << c << IP_TAB_SEP
            << d;
        if ((NTABS_INIP == rc) && (formedbackip.str() == ip))
        {
            if (a || b || c || d)
            {
                if ((a >= 0) && (a <= MAX_BYTE) &&
                    (b >= 0) && (b <= MAX_BYTE) &&
                    (c >= 0) && (c <= MAX_BYTE) &&
                    (d >= 0) && (d <= MAX_BYTE))
                {
                    bisvalid = true;
                }
            }
        } 
    }
    if (!bisvalid)
    {
        DebugPrintf(SV_LOG_WARNING, "ip %s is invalid\n", ip.c_str());
    }
    return bisvalid;
}


void GetFirstInpFromFile(const std::string &filename, NUMFORMAT nfmt, SV_LONGLONG *pcontents)
{
    std::ifstream ifile(filename.c_str());
    if (ifile.is_open())
    {
        if (ifile.good())
        {
            if (E_HEX == nfmt)
                ifile >> std::hex >> (*pcontents);
            else if (E_OCT == nfmt)
                ifile >> std::oct >> (*pcontents);
            else
                ifile >> (*pcontents);
        }
        ifile.close();
    }
}

void GetFirstStringFromFile(const std::string &filename, std::string &str)
{
    std::ifstream ifile(filename.c_str());
    if (ifile.is_open())
    {
        if (ifile.good())
        {
            ifile >> str;
        }
        ifile.close();
    }
}

void GetFirstLineFromFile(const std::string &filename, std::string &str)
{
    std::ifstream ifile(filename.c_str());
    if (ifile.is_open())
    {
        if (ifile.good())
        {
            std::getline(ifile, str);
        }
        ifile.close();
    }
}

void RemoveChar(std::string &s, const char &c)
{
    std::string::iterator nend = std::remove(s.begin(), s.end(), c);
    s.erase(nend, s.end());
}

void RemoveLastMatchingChar(std::string &s, const char &c)
{
    size_t ns = s.length();
    if ((ns > 0) && c == s[ns - 1])
    {
        s.erase(ns - 1);
    }
}


void PrintString(const std::string &s)
{
    DebugPrintf(SV_LOG_DEBUG, "%s\n", s.c_str());
}


void Trim(std::string& s, const std::string &trim_chars ) 
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
}


bool IsNotxString(const std::string &s)
{
    return !IsxString(s);
}


bool IsxString(const std::string &s)
{
    return (s.end() == find_if(s.begin(), s.end(), isnotxdigit));
}


bool isnotxdigit(const char &c)
{
    return !isxdigit(c);
}


std::string GetUpperCase(const std::string &str)
{
    std::string ustr;
    for (size_t i = 0; i < str.length(); i++)
    {
        ustr.push_back(toupper(str[i]));
    }

    return ustr;
}


const char *GetStartOfLastNumber(const char *cs)
{
    const char *num = NULL;
    size_t len = strlen(cs);
    const char *start = cs;
    const char *p = start + (len - 1);
    const char *numstart = NULL;

    while ((p >= start) && (isdigit(*p)))
    {
        numstart = p;
        p--;
    }

    if ((p >= start) && numstart)
    {
        num = numstart;
    }

    return num;
}

std::string RemoveSuffix(const std::string &s, const std::string &suffix)
{
    std::string final;
    size_t suffixidx = s.rfind(suffix);

    if (std::string::npos != suffixidx)
    {
        final = s.substr(0, suffixidx);
    }
    else
    {
        final = s;
    }
    return final;
}


bool AreAllDigits(const char *p)
{
    bool baredigits = strlen(p);

    for (const char *q = p; *q ; q++)
    {
        if (!isdigit(*q))
        {
            baredigits = false;
            break;
        }
    }

    return baredigits;
}


SVERROR getFileContent( std::string filePath, std::string& content )
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    ACE_HANDLE handle = ACE_INVALID_HANDLE;
    SVERROR status = SVS_FALSE;
    handle = ACE_OS::open( filePath.c_str(), O_RDONLY );
    if( handle == ACE_INVALID_HANDLE )
    {
        DebugPrintf(SV_LOG_WARNING, "%s: The %s is not readable.. Error %d\n", FUNCTION_NAME, filePath.c_str(), ACE_OS::last_error());
    }
    else
    {
        SV_ULONGLONG const fileSize = File::GetSizeOnDisk( filePath );
        size_t infosize;
        INM_SAFE_ARITHMETIC(infosize = InmSafeInt<SV_ULONGLONG>::Type(fileSize) + 1, INMAGE_EX(fileSize))
        boost::shared_array<char> info (new (std::nothrow) char[infosize]);
        if( (size_t) fileSize == ACE_OS::read(handle, info.get(), (size_t)fileSize) )
        {
            info[ (ptrdiff_t)fileSize ] = '\0';
            content = info.get();
            status = SVS_OK;
            DebugPrintf(SV_LOG_DEBUG, "%s: read the entire file %s. Requested Read size %d %s\n", FUNCTION_NAME, filePath.c_str(), fileSize, content.c_str());
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "%s: Failed to read the file %s. Requested Read size %d\n", FUNCTION_NAME, filePath.c_str(), fileSize);

        }
        ACE_OS::close( handle );
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return status;
}

bool isFileExited(const std::string& fileNamePath)
{
    bool retVal = false;
    ACE_stat stat ;
    if(sv_stat(getLongPathName(fileNamePath.c_str()).c_str(), &stat) == 0)
    {
        retVal = true;
        DebugPrintf(SV_LOG_DEBUG, "File %s is already available \n", fileNamePath.c_str());
    }
    else
    {
        DebugPrintf(SV_LOG_DEBUG, "File %s is not available \n", fileNamePath.c_str());
    }
    return retVal;
}

void pruneFiles(const std::string& dirName, const std::string& date, bool recursive, bool bFirst)
{
    ACE_stat stat ;
    ACE_DIR * ace_dir = NULL ;
    ACE_DIRENT * dirent = NULL ;
    static std::string lastFile;
    if(bFirst)
    {
        lastFile = "";
    }
    std::string currentPath;
    if(dirName.empty() == false && sv_stat(getLongPathName(dirName.c_str()).c_str(), &stat) == 0)
    {
        ace_dir = sv_opendir(dirName.c_str()) ;
        if( ace_dir != NULL )
        {
            for(dirent = ACE_OS::readdir(ace_dir); dirent != NULL; dirent = ACE_OS::readdir(ace_dir))
            {
                std::string fileName = ACE_TEXT_ALWAYS_CHAR(dirent->d_name);
                if( strcmp(fileName.c_str(), "." ) == 0 || strcmp(fileName.c_str(), ".." ) == 0 )
                {
                    continue ;
                }
                currentPath = "";
                currentPath = dirName+ACE_DIRECTORY_SEPARATOR_CHAR_A+ fileName;
                if(sv_stat(getLongPathName(currentPath.c_str()).c_str(), &stat) == 0)
                {
                    if(stat.st_mode & S_IFDIR)
                    {
                        if(recursive)
                        {
                            pruneFiles(currentPath, date, true, false);
                        }
                    }
                    else
                    {
                        std::string::size_type index = currentPath.find_last_of("_");
                        if(index != std::string::npos)
                        {
                            std::string leftString = currentPath.substr(0, index+1);
                            std::string rightString = currentPath.substr(index+1);
                            if(strcmp(rightString.c_str(), date.c_str()) <= 0)
                            {
                                if(lastFile.empty())
                                {
                                    lastFile = rightString;
                                }
                                else
                                {
                                    std::string removedFile = leftString + rightString;
                                    if(strcmp(lastFile.c_str(), rightString.c_str()) < 0)
                                    {
                                        removedFile = leftString + lastFile;
                                        lastFile = rightString;
                                    }
                                    remove(removedFile.c_str());
                                    DebugPrintf(SV_LOG_DEBUG, "Successfully removed file: %s \n", removedFile.c_str());
                                }
                            }
                        }
                    }
                }
                else
                {
                    DebugPrintf(SV_LOG_ERROR, "getLongPathName() Failed. Path Name: %s \n.", currentPath.c_str());
                }
            }
            ACE_OS::closedir( ace_dir ) ;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "sv_opendir() failed. Dir Name: %s \n", dirName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "getLogPathName() Failed. Dir Name: %s \n", dirName.c_str());
    }
}

std::list<std::string> getFirstLevelDirPathNameList(const std::string& dirName)
{
    std::list<std::string> dirNamePathList;
    ACE_stat stat ;
    ACE_DIR * ace_dir = NULL ;
    ACE_DIRENT * dirent = NULL ;
    std::string currentPath;
    if(dirName.empty() == false && sv_stat(getLongPathName(dirName.c_str()).c_str(), &stat) == 0)
    {
        ace_dir = sv_opendir(dirName.c_str()) ;
        if( ace_dir != NULL )
        {
            for(dirent = ACE_OS::readdir(ace_dir); dirent != NULL; dirent = ACE_OS::readdir(ace_dir))
            {
                std::string fileName = ACE_TEXT_ALWAYS_CHAR(dirent->d_name);
                if( strcmp(fileName.c_str(), "." ) == 0 || strcmp(fileName.c_str(), ".." ) == 0 )
                {
                    continue ;
                }
                currentPath = dirName+ACE_DIRECTORY_SEPARATOR_CHAR_A+ fileName;
                if(sv_stat(getLongPathName(currentPath.c_str()).c_str(), &stat) == 0)
                {
                    if(stat.st_mode & S_IFDIR)
                    {
                        dirNamePathList.push_back(currentPath);
                    }
                }
                else
                {
                    DebugPrintf(SV_LOG_ERROR, "getLongPathName() Failed. Path Name: %s \n.", currentPath.c_str());
                }
            }
            ACE_OS::closedir( ace_dir ) ;
        }
        else
        {
            DebugPrintf(SV_LOG_ERROR, "sv_opendir() failed. Dir Name: %s \n", dirName.c_str());
        }
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "getLogPathName() Failed. Dir Name: %s \n", dirName.c_str());
    }
    return dirNamePathList;
}

void pruneDirs(const std::string& repoPath, const std::list<std::string>& sourceHostIdList)
{
    std::list<std::string> dirNamePathList = getFirstLevelDirPathNameList(repoPath);
    std::list<std::string>::iterator begIter = dirNamePathList.begin();
    std::list<std::string>::iterator endIter = dirNamePathList.end();
    while(begIter != endIter)
    {
        std::list<std::string>::const_iterator sourceHostIdListBegIter = sourceHostIdList.begin();
        std::list<std::string>::const_iterator sourceHostIdListEndIter = sourceHostIdList.end();
        while(sourceHostIdListBegIter != sourceHostIdListEndIter)
        {
            if(strstr(begIter->c_str(), sourceHostIdListBegIter->c_str()))
            {
                break;
            }
            sourceHostIdListBegIter++;
        }
        if(sourceHostIdListBegIter == sourceHostIdListEndIter)
        {
            remove(begIter->c_str());
        }
        begIter++;
    }
}

void PrintStatuses(const Statuses_t &ss, SV_LOG_LEVEL lvl)
{
    for (Statuses_t::size_type i = 0; i < ss.size(); i++)
    {
        std::stringstream msg;
        msg << "For stage " << i 
            << ", state is " << StrState[ss[i].m_State]
            << ", exception message is " << ss[i].m_ExcMsg;
        DebugPrintf(lvl, "%s\n", msg.str().c_str());
    }
}


void GetStatuses(const Statuses_t &ss, std::string &s)
{
    for (Statuses_t::size_type i = 0; i < ss.size(); i++)
    {
        std::stringstream msg;
        msg << "For subtask: " << i 
            << ", state is " << StrState[ss[i].m_State]
            << ", exception message is " << ss[i].m_ExcMsg << ". ";
        s += msg.str();
    }
}


void DataAndLength::Print(void)
{
    DebugPrintf(SV_LOG_DEBUG, "data = %p, length = %u\n", m_pData, m_length);
}

std::string getSourceConfigFilesTimeFormat(SV_ULONGLONG timeValue)
{
    time_t t1 = timeValue;
    char localTime[128];
    strftime(localTime, 128, "%Y%m%d%H%M%S", localtime(&t1));
    std::stringstream timeFormat;
    timeFormat << localTime;
    return timeFormat.str();
}

bool RestoreNtfsOrFatSignature(const std::string & volumename)
{
    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    bool rv  = true;
    bool shouldwriteheader = false;
    // open the handle
    // read the first 512 bytes
    ACE_HANDLE handle = ACE_INVALID_HANDLE;
    SVERROR status = SVS_FALSE;
    size_t sectorsize = 512;
    char pbuffer[SECTOR_SIZE];
    ACE_LOFF_T seekOffset;
    seekOffset = (ACE_LOFF_T)0LL;

    handle = ACE_OS::open( volumename.c_str(), O_RDWR );
    do
    {
        if( handle == ACE_INVALID_HANDLE )
        {
            DebugPrintf(SV_LOG_WARNING, "unable to open %s. error %d\n", volumename.c_str(), ACE_OS::last_error());
            rv = false;
            break;
        }

    
        if( (size_t) sectorsize != ACE_OS::read(handle, pbuffer, sectorsize) )
        {
            DebugPrintf(SV_LOG_ERROR, "Failed to read the volume %s. Requested Read size %d , error %d\n", volumename.c_str(), sectorsize ,ACE_OS::last_error());
            rv = false;
            break;
        }
            

            // Check if this is FAT and not NTFS (NTFS can look like FAT if we don't make additional checks)
            Bpb* bpb = (Bpb*)(pbuffer);
            if ((bpb->BS_jmpBoot[0] == BS_JMP_BOOT_VALUE_0xEC || bpb->BS_jmpBoot[0] == BS_JMP_BOOT_VALUE_0xEA) && 
                (0 != memcmp(&pbuffer[SV_NTFS_MAGIC_OFFSET], SV_MAGIC_SVFS, SV_FSMAGIC_LEN)) &&
                (0 != memcmp(&pbuffer[SV_NTFS_MAGIC_OFFSET], SV_MAGIC_NTFS, SV_FSMAGIC_LEN)) )
            {
                if (FST_SCTR_OFFSET_510_VALUE == pbuffer[FST_SCTR_OFFSET_510] && FST_SCTR_OFFSET_511_VALUE == pbuffer[FST_SCTR_OFFSET_511]) {
                    // Change the BPB to hide it
                    // I.E We make jmpboot instruction to be either 0xEB, or 0xE9
                    // Which makes it an invalid FAT volume
                    bpb->BS_jmpBoot[0] -= 1;
                    shouldwriteheader = true;
                }
            }
            // check for svfs
            if (0 == memcmp(&pbuffer[SV_NTFS_MAGIC_OFFSET], SV_MAGIC_SVFS, SV_FSMAGIC_LEN)) {
                inm_memcpy_s(&pbuffer[SV_NTFS_MAGIC_OFFSET], sizeof(pbuffer) - SV_NTFS_MAGIC_OFFSET, SV_MAGIC_NTFS, SV_FSMAGIC_LEN);
                shouldwriteheader = true;
            } 
        
            if(shouldwriteheader )
            {
#ifndef LLSEEK_NOT_SUPPORTED
                seekOffset = (ACE_LOFF_T)0LL;
                if (ACE_OS::llseek(handle, seekOffset, SEEK_SET) < 0) {
#else
                seekOffset = (off_t)0LL;
                if (ACE_OS::lseek(handleVolume, seekOffset, SEEK_SET) < 0) {
#endif
                    DebugPrintf( SV_LOG_ERROR, "FAILED ACE_OS::llseek, err = %d\n", ACE_OS::last_error());
                    rv = false;
                    break;
                }
                //done with altering the buffer ,now lets dump it 
                if(sectorsize != ACE_OS::write(handle,pbuffer,sectorsize))
                {
                     DebugPrintf(SV_LOG_ERROR, "Failed to write volume %s. Requested write size %d .error %d\n", volumename.c_str(), sectorsize ,ACE_OS::last_error() );
                     rv = false;
                     break;
                }
            }
    }while(0);
    // close the handle

    if( handle != ACE_INVALID_HANDLE)
            ACE_OS::close(handle);
    DebugPrintf( SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return rv;
}

size_t WriteMemoryCallbackFileReplication(void *ptr, size_t size, size_t nmemb, void *data)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    size_t realsize;
    INM_SAFE_ARITHMETIC(realsize = InmSafeInt<size_t>::Type(size) * nmemb, INMAGE_EX(size)(nmemb))
    MemoryStruct *mem = (MemoryStruct *)data;

    size_t memorylen;
    INM_SAFE_ARITHMETIC(memorylen = InmSafeInt<size_t>::Type(mem->size) + realsize + 1, INMAGE_EX(mem->size)(realsize))
    mem->memory = (char *)realloc(mem->memory, memorylen);

    if (mem->memory) {
        inm_memcpy_s(&(mem->memory[mem->size]), realsize + 1, ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return realsize;
}

SVERROR postToCx(const char* pszHost, 
                 SV_INT Port, 
                 const char* pszUrl, 
                 const char* pszBuffer,
                 char** ppszBuffer,bool useSecure, 
                 SV_ULONG size, 
                 SV_ULONG retryCount
                 ) 
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);


    SVERROR rc = SVE_FAIL ;

    int retries = 0;
    MemoryStruct chunk = {0};

    if ( Port > 65535 || Port < 1 )
    {
        DebugPrintf(SV_LOG_ERROR, "The Port is out of range.. Please choose port number between range 1-65535 on which the CX server is listening\n");  
    }
    else
    {

        DebugPrintf(SV_LOG_DEBUG,"Server = %s\n",pszHost);
        DebugPrintf(SV_LOG_DEBUG,"Port = %d\n",Port);
        DebugPrintf(SV_LOG_DEBUG,"Secure = %d\n",useSecure);
        DebugPrintf(SV_LOG_DEBUG,"PHP URL = %s\n",pszUrl);
        std::string result;
        CurlOptions options(pszHost,Port, pszUrl, useSecure);

        options.writeCallback( static_cast<void *>( &chunk ),WriteMemoryCallbackFileReplication);
        options.lowSpeedLimit(10);
        options.lowSpeedTime(120);
        options.transferTimeout(CX_TIMEOUT);

        while ( retries < retryCount && rc == SVE_FAIL )
        {

            DebugPrintf(SV_LOG_DEBUG, "Attemt %d: PostToServer ...\n", retries );

            chunk.memory = NULL; 
            chunk.size = 0; /* no data at this point */

            try {
                CurlWrapper cw;
                cw.post(options,pszBuffer);
                if( chunk.size > 0)
                {

                    if(ppszBuffer != NULL) 
                    {
                        *ppszBuffer = chunk.memory;
                    }
                    else
                    {
                        free( chunk.memory ) ;
                    }
                }
                rc = SVS_OK;
            } catch(ErrorException& exception )
            {
                DebugPrintf(SV_LOG_ERROR, "FAILED : %s with error %s\n",FUNCTION_NAME, exception.what());
                rc = SVE_FAIL;
            }

            retries++;

#ifdef SV_WINDOWS
            Sleep(retries * 5000 );
#else
            sleep(retries * 5 );
#endif 
        }

        if( rc == SVE_FAIL )
        {
            DebugPrintf(SV_LOG_ERROR, "PostToServer server failed even after %d attempts, This may cause file replication jobs to fail...\n", retries);  
        }
    }
    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return( rc );
}


bool DoesFileExist(const std::string &file)
{
    E_INM_TRISTATE st;
    std::string errmsg;

    st = DoesFileExist(file, errmsg);
    if (E_INM_TRISTATE_FLOATING == st)
        DebugPrintf(SV_LOG_ERROR, "%s\n", errmsg.c_str());

    return (E_INM_TRISTATE_TRUE == st);
}


/*
1. It is working as expected on local files.
2. Sample run on cifs:
   ==================
--> Not logged into cifs :

C:\run>exists.exe \\10.0.0.5\public\read.exe
Failed to find file existence with error Failed to open file \\10.0.0.5\public\read.exe, 
to check its existence, with error Invalid argument.

C:\run>exists.exe \\10.0.0.5\public\re
Failed to find file existence with error Failed to open file \\10.0.0.5\public\re, 
to check its existence, with error Invalid argument.
 
--> Now login to cifs :

C:\run>exists.exe \\10.0.0.5\public\read.exe
File exists.

C:\run>exists.exe \\10.0.0.5\public\re
File does not exist.
*/
E_INM_TRISTATE DoesFileExist(const std::string &file, std::string &errmsg)
{
    std::string openmode("r");
    openmode += FOPEN_MODE_NOINHERIT;
    FILE *fp = fopen(file.c_str(), openmode.c_str());
    E_INM_TRISTATE st = E_INM_TRISTATE_FLOATING;

    if (fp) {
        st = E_INM_TRISTATE_TRUE;
        fclose(fp);
    } else if (ENOENT == errno)
        st = E_INM_TRISTATE_FALSE;
    else {
        int saveerrno = errno;
        errmsg = "Failed to open file ";
        errmsg += file;
        errmsg += ", to check its existence, with error ";
        errmsg += strerror(saveerrno);
    }

    return st;
}


bool InmFtruncate(const std::string &file, std::string &errmsg)
{
    std::string openmode("w");
    openmode += FOPEN_MODE_NOINHERIT;
    FILE *fp = fopen(file.c_str(), openmode.c_str());

    if (fp)
    {
        fclose(fp);
    }
    else
    {
        int saveerrno = errno;
        errmsg = "Failed to truncate(open in write mode) file ";
        errmsg += file;
        errmsg += " with error ";
        errmsg += strerror(saveerrno);
    }

    return fp;
}


bool InmFopen(FILE **fpp, const std::string &name, const std::string &mode, std::string &errmsg)
{
    if ((*fpp = fopen(name.c_str(), mode.c_str())) == 0)
    {
        int saveerrno = errno;
        errmsg = "Opening of file " + name + " in mode " + mode + " failed with error: " + strerror(saveerrno);
    }
    
    return *fpp;
}


bool InmFclose(FILE **fpp, const std::string &name, std::string &errmsg)
{
    bool bclosed = true;

    if (*fpp)
    {
        bclosed = false;
        if (0 != fclose(*fpp))
        {
            int saveerrno = errno;
            errmsg = "Closing file ";
            errmsg += name;
            errmsg += ", failed with error: ";
            errmsg += strerror(saveerrno);
        }
        else
        {
            *fpp = 0;
            bclosed = true;
        }
    }

    return bclosed;
}


bool InmFprintf(FILE *fp, const std::string &name, std::string &errmsg, const bool &brestorefilepointer, const long &writepos, const char *fmt, ...)
{
    bool bproceed = true;

    std::string filestr = "For file ";
    filestr += name;
    filestr += ", ";
    long savepos = -1;
    if (brestorefilepointer)
    {
        savepos = ftell(fp);
        if (-1 == savepos)
        {
            int saveerrno = errno;
            errmsg = filestr;
            errmsg += "getting file pointer position failed with error: ";
            errmsg += strerror(saveerrno);
            bproceed = false;
        }
    }

    if (bproceed && writepos)
    {
        if (0 != fseek(fp, writepos, SEEK_SET))
        {
            int saveerrno = errno;
            std::stringstream sswp;
            sswp << writepos;
            errmsg = filestr;
            errmsg += "seeking to write position ";
            errmsg += sswp.str();
            errmsg += " failed with error: ";
            errmsg += strerror(saveerrno);
            bproceed = false;
        }
    }

    if (bproceed)
    {
        va_list ap;
        va_start(ap, fmt);
    
        if (vfprintf(fp, fmt, ap) < 0)
        {
            int saveerrno = errno;
            errmsg = filestr;
            errmsg += "writing formatted text";
            errmsg += " failed with error: ";
            errmsg += strerror(saveerrno);
            bproceed = false;
        }
        else if (0 != fflush(fp))
        {
            int saveerrno = errno;
            bproceed = false;
            errmsg = filestr;
            errmsg += "flushing file failed with error: ";
            errmsg += strerror(saveerrno);
        }
        va_end(ap);
    }

    /* TODO: restore even if no write or flush failure ? */
    if (brestorefilepointer)
    {
        if (0 != fseek(fp, savepos, SEEK_SET))
        {
            int saveerrno = errno;
            errmsg = filestr;
            errmsg += "seeking in file failed with error: ";
            errmsg += strerror(saveerrno);
            bproceed = false;
        }
    }

    return bproceed;
}


bool SyncFile(const std::string &file, std::string &errmsg)
{
    bool bsynced = false;
    ACE_HANDLE h = ACE_OS::open(file.c_str(), O_RDWR);

    if (ACE_INVALID_HANDLE != h)
    {
        if (-1 != ACE_OS::fsync(h))
        {
            bsynced = true;
        }
        else
        {
            std::stringstream sserr;
            sserr << "failed to sync file " << file 
                  << " with error number: " << ACE_OS::last_error();
            errmsg = sserr.str();
        }

        if (0 != ACE_OS::close(h))
        {
            bsynced = false;
            std::stringstream sserr;
            sserr << "failed to close file " << file 
                  << " with error number: " << ACE_OS::last_error();
            errmsg = sserr.str();
        }
    }
    else
    {
        std::stringstream sserr;
        sserr << "failed to open file " << file 
              << " with error number: " << ACE_OS::last_error();
        errmsg = sserr.str();
    }

    return bsynced;
}


bool InmCopyTextFile(const std::string &src, const std::string &dest, const bool &busetemporary, std::string &errmsg)
{
    FILE *srcfp, *destfp;
    bool bcopied = false;

    std::string rmode = "r";
    rmode += FOPEN_MODE_NOINHERIT;

    std::string wmode = "w";
    wmode += FOPEN_MODE_NOINHERIT;

    std::string ferrmsg;
    srcfp = destfp = NULL;
    std::string filetowrite = busetemporary ? (dest + ".tmp") : dest;
    if (InmFopen(&srcfp, src, rmode, ferrmsg))
    {
        if (InmFopen(&destfp, filetowrite, wmode, ferrmsg))
        {
            int c;
            while ((c = getc(srcfp)) != EOF)
            {
                putc(c, destfp);
            }
            /* for output functions, fclose gives the 
             * output was successfully written or not */
            bcopied = InmFclose(&destfp, filetowrite, ferrmsg);
        }
        InmFclose(&srcfp, src, ferrmsg);
    }

    if (!bcopied)
    {
        errmsg = "Failed to copy file ";
        errmsg += src;
        errmsg += " to ";
        errmsg += filetowrite;
        errmsg += " with error message: ";
        errmsg += ferrmsg;
    }
    else if (busetemporary)
    {
        bcopied = InmRename(filetowrite.c_str(), dest.c_str(), errmsg);
        if (!bcopied)
        {
            /* remove temporary if we did not 
             * rename */
            std::string err;
            InmRemoveFile(filetowrite, err);
        }
    }

    return bcopied;
}


bool InmRemoveFile(const std::string &file, std::string &errmsg)
{
    bool bremoved = false;
    std::string suberrmsg;
    E_INM_TRISTATE st = DoesFileExist(file, suberrmsg);

    if (E_INM_TRISTATE_FLOATING != st) {
        bremoved = (E_INM_TRISTATE_TRUE == st) ? (0 == remove(file.c_str())) : true;
        if (!bremoved)
            suberrmsg = strerror(errno);
    }

    if (!bremoved) {
        errmsg = "failed to remove file ";
        errmsg += file;
        errmsg += " with error: ";
        errmsg += suberrmsg;
    }

    return bremoved;
}


void TrimChar(char *str, const char &c)
{
    size_t len = strlen(str);
 
    if (len && (str[len - 1] == c))
    {
        str[len - 1] = '\0';
    }
}


bool InmRenameAndRemoveFile(const std::string &file, std::string &errmsg)
{
    std::string renamefile = file;
    renamefile += ".D.";

    ACE_Time_Value aceTv = ACE_OS::gettimeofday();
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') <<
       ACE_TIME_VALUE_AS_USEC(aceTv);
    renamefile += ss.str();

    return InmRename(file, renamefile, errmsg) && InmRemoveFile(renamefile, errmsg);
}


SVERROR WriteStringIntoFile(const std::string& fileName,const std::string& str)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME) ;
    SVERROR retValue = SVS_OK;
    std::ofstream out;
    if( !fileName.empty() )
    {
        out.open(fileName.c_str(),std::ios::app);
        if (!out.is_open()) 
        {
            DebugPrintf(SV_LOG_ERROR,"Failed to createFile %s\n",fileName.c_str());
        }
        else
        {
            out<<str;
        }
        out.close();
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME) ;
    return retValue;
}


//
// FUNCTION NAME :  getvolpackproperties
//
// DESCRIPTION : Fetches the properties of specified volpack device
//
// INPUT PARAMETERS : volpack device name
//
// OUTPUT PARAMETERS : volpackproperties structure
//
//Return Value : INM_NOT_A_VOLPACK (if device is not a volpack)
//                 0  (on success)
//   -1 ( on failure)
//
int getvolpackproperties(const std::string& volpack_device,volpackproperties& volpackinstance )
{
    int retvalue = 0;
    volpackinstance.m_logicalsize = 0;
    volpackinstance.m_sizeondisk = 0;
    do
    {
        std::string sparsefilename;
        bool new_sparsefile_format = false;
        ACE_stat statbuf = {0};
        if(!IsVolPackDevice(volpack_device.c_str(),sparsefilename,new_sparsefile_format))
        {
            retvalue = INM_NOT_A_VOLPACK;
            break;
        }
        volpackinstance.m_sparsefilename = sparsefilename;

        if(!new_sparsefile_format)
        {
            if(sv_stat(getLongPathName(sparsefilename.c_str()).c_str(), &statbuf) != 0)
            {
                DebugPrintf(SV_LOG_ERROR, "FUNCTION:%s Failed to get file :%s statitics.\n",
                    FUNCTION_NAME,sparsefilename.c_str());
                retvalue = -1;
                break;
            }
            volpackinstance.m_logicalsize = statbuf.st_size;
            volpackinstance.m_sizeondisk = File::GetSizeOnDisk(sparsefilename);
            break;
        }

        int i = 0;
        std::stringstream sparsepartfile;
        while(true)
        {
            sparsepartfile.str("");
            sparsepartfile << sparsefilename << SPARSE_PARTFILE_EXT << i;
            if( sv_stat( getLongPathName(sparsepartfile.str().c_str()).c_str(), &statbuf ) < 0 )
            {
                break;
            }     
            volpackinstance.m_logicalsize += statbuf.st_size;
            volpackinstance.m_sizeondisk += File::GetSizeOnDisk(sparsepartfile.str());
            i++;
        }

    } while(0);
    return retvalue;
}


void removeStringSpaces( std::string& s)
{
    Trim(s, " \n\b\t\a\r\xc");
}


void PrintNicInfos(NicInfos_t &nicinfos, const SV_LOG_LEVEL logLevel)
{
    DebugPrintf(SV_LOG_DEBUG, "nicinfos:\n");
    for (NicInfosIter_t it = nicinfos.begin(); it != nicinfos.end(); it++)
    {
        DebugPrintf(logLevel, "Hardware Address: %s\n", it->first.c_str());
        DebugPrintf(logLevel, "Configurations:\n");
        Objects_t &nicconfs = it->second;
        for (ObjectsIter_t cit = nicconfs.begin(); cit != nicconfs.end(); cit++)
        {
            cit->Print(logLevel);
        }
    }
}

// ValidateAndGetIPAddress chacks input ipAddress(es) for an IPv4 private address space format.
// On Windows input is comma saparated IP addresses. For example,
//          10.150.2.120,fe80::6d35:68a9:f2f9:956a,2404:f801:4800:2:6d35:68a9:f2f9:956a
// On Linux input is single IP addresses. For example,
//          10.150.2.135
// It checks input IP address(es) and returns an ipAddress string in IPv4 private address space format.
// If check fails it returns a blank string.
std::string ValidateAndGetIPAddress(const std::string &ipAddress, std::string &errMsg)
{
    std::string validIPAddress;

    try{
        typedef boost::tokenizer<boost::char_separator<char> > tokenizer_t;
        boost::char_separator<char> ipSep(",");
        tokenizer_t ipTokens(ipAddress, ipSep);
        tokenizer_t::iterator ipItr(ipTokens.begin());
        for (ipItr; ipItr != ipTokens.end(); ++ipItr)
        {
            if (!boost::asio::ip::address::from_string(*ipItr).is_v4())
            {
                errMsg = *ipItr;
                errMsg += " not in IPv4 format";
                continue;
            }

            if (boost::asio::ip::address::from_string(*ipItr).is_loopback())
            {
                errMsg = *ipItr;
                errMsg += " not in IPv4 private address space format";
                continue;
            }

            // check APIPA format
            const std::string strAPIPA_IPAddress = "169.254.";
            if (0 == strAPIPA_IPAddress.compare((*ipItr).substr(0, strAPIPA_IPAddress.length())))
            {
                errMsg = *ipItr;
                errMsg += " not in IPv4 private address space format";
                continue;
            }
            validIPAddress = *ipItr;
            break;
        }
    }catch (const std::exception &e){
        errMsg += "caught exception: ";
        errMsg += e.what();
    }catch (...){
        errMsg += FUNCTION_NAME;
        errMsg += " ";
        errMsg += "caught unknown exception";
    }

    return validIPAddress;
}

// GetIPAddressSetFromNicInfo collects all IP addresses in IPv4 private address space format in NIC in an out param ips.
// After successful processing ips set contains IP address(es) in following order,
//          10.0.0.0 - 10.255.255.255  (10 / 8 prefix)
//          172.16.0.0 - 172.31.255.255  (172.16 / 12 prefix)
//          192.168.0.0 - 192.168.255.255 (192.168 / 16 prefix)
// 
// Returns true if any of NIC found with IP addresses in IPv4 private address space format.
// Returns false if none of NIC found with IP addresses in IPv4 private address space format.
// Returns false for any other error condition.
bool GetIPAddressSetFromNicInfo(strset_t &ips, std::string &errMsg)
{
    NicInfos_t nicinfos;
    GetNicInfos(nicinfos);
    ConstNicInfosIter_t nicinfosItr = nicinfos.begin();
    if (nicinfosItr == nicinfos.end())
    {
        errMsg = "nicinfo missing";
        return false;
    }
    for (nicinfosItr; nicinfosItr != nicinfos.end(); nicinfosItr++)
    {
        const Objects_t &nicconfs = nicinfosItr->second;
        for (ConstObjectsIter_t objectsItr = nicconfs.begin(); objectsItr != nicconfs.end(); objectsItr++)
        {
            Attributes_t::const_iterator attributeItr = objectsItr->m_Attributes.find(NSNicInfo::IP_ADDRESSES);
            if (attributeItr != objectsItr->m_Attributes.end())
            {
                std::string err;
                std::string ipAddr = ValidateAndGetIPAddress(attributeItr->second, err);
                if (!ipAddr.empty())
                {
                    ips.insert(ipAddr);
                }
                else
                {
                    errMsg += err;
                    errMsg += ", ";
                }
            }
            else
            {
                errMsg = NSNicInfo::IP_ADDRESSES;
                errMsg += " missing in nicinfo ";
                errMsg += nicinfosItr->first;
                errMsg += ", ";
            }
        }
    }
    if (ips.empty())
    {
        errMsg += "No valid IP found in nicifo.";
        return false;
    }
    return true;
}


void InsertNicInfo(NicInfos_t &nicInfos, const std::string &name, const std::string &hardwareaddress, const std::string &ipaddress, 
                   const std::string &netmask, const E_INM_TRISTATE &isdhcpenabled)
{
    NicInfosIter_t nicinfoiter = nicInfos.find(hardwareaddress);
    Object *p;
    if (nicinfoiter != nicInfos.end())
    {
        Objects_t &nicobjs = nicinfoiter->second;
        ObjectsIter_t oit = nicobjs.insert(nicobjs.end(), Object());
        Object &o = *oit;
        p = &o;
    }
    else
    {
        std::pair<NicInfosIter_t, bool> pr = nicInfos.insert(std::make_pair(hardwareaddress, Objects_t()));
        Objects_t &nicobjs = pr.first->second;
        ObjectsIter_t oit = nicobjs.insert(nicobjs.end(), Object());
        Object &o = *oit;
        p = &o;
    }
     
    /* name is assumed to non empty */
    p->m_Attributes.insert(std::make_pair(NSNicInfo::NAME, name));

    if (!ipaddress.empty())
    {
        p->m_Attributes.insert(std::make_pair(NSNicInfo::IP_ADDRESSES, ipaddress));
    }

    if (!netmask.empty())
    {
        p->m_Attributes.insert(std::make_pair(NSNicInfo::IP_SUBNET_MASKS, netmask));
    }

    if (E_INM_TRISTATE_FLOATING != isdhcpenabled)
    {
        const char *v = (E_INM_TRISTATE_TRUE == isdhcpenabled) ? "true" : "false";
        p->m_Attributes.insert(std::make_pair(NSNicInfo::IS_DHCP_ENABLED, v));
    }
}


void UpdateNicInfoAttr(const std::string &attr, const std::string &value, NicInfos_t &nicinfos)
{
    for (NicInfosIter_t it = nicinfos.begin(); it != nicinfos.end(); it++)
    {
        Objects_t &objs = it->second;
        for (ObjectsIter_t oit = objs.begin(); oit != objs.end(); oit++)
        {
            Object &o = *oit;
            o.m_Attributes.insert(std::make_pair(attr, value));
        }
    }
}
std::string Escapexml(const std::string& value)
{
    std::string str = value ;
    boost::replace_all ( str, "&", "&amp;" );
    boost::replace_all ( str, "\"", "&quot;" );
    boost::replace_all ( str, "\'", "&apos;" );
    boost::replace_all ( str, ">",  "&gt;" );
    boost::replace_all ( str, "<",  "&lt;" );
    return str;
}

void InmHexStrToUll(const std::string &s, unsigned long long &ull)
{
    std::stringstream ss(s);

    ss >> std::hex >> ull;
}


bool EnQ(ACE_Message_Queue<ACE_MT_SYNCH> *Q, ACE_Message_Block *mb, const int &waitsecs, QuitFunction_t &qf)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s with mb = %p\n", FUNCTION_NAME, mb);
    bool bretval = true;

    do
    {
        ACE_Time_Value time = ACE_OS::gettimeofday() ;
        time += waitsecs ;
        if( -1 == Q->enqueue_prio( mb, &time ) )
        {
            if (errno == EWOULDBLOCK )
            {
                qf && qf( waitsecs ) ;
            }
            else
            {
                DebugPrintf(SV_LOG_ERROR, "failed to enqueue with errno %d\n", errno);
                bretval = false;
                break;
            }
        }
        else
        {
            break;
        }
    } while (qf && !qf(0));

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s with return value = %s\n", FUNCTION_NAME, bretval?"true":"false");
    return bretval;
}


ACE_Message_Block * DeQ( ACE_Message_Queue<ACE_MT_SYNCH> * Q, const int &waitsecs, const bool &getwaittime, ACE_Time_Value *pwaittime )
{
    ACE_Message_Block *mb = NULL;

    DebugPrintf( SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME ) ;

    DeQ_t ds[] = {&DirectDeQ, &DeQWithRecordedWaitTime};
    DeQ_t p = ds[getwaittime];
    ACE_Time_Value wait = ACE_OS::gettimeofday ();
    wait.sec (wait.sec () + waitsecs);
    int deqrv = (*p)(Q, &mb, wait, pwaittime);
    if (-1 == deqrv)
    {
        if ((errno != EWOULDBLOCK) && (errno != ESHUTDOWN))
        {
            DebugPrintf(SV_LOG_ERROR, "failed to deque with errno %d\n", errno);
        }
    }
    DebugPrintf( SV_LOG_DEBUG, "EXITED %s with mb = %p\n", FUNCTION_NAME, mb) ;

    return mb ;
}


int DeQWithRecordedWaitTime( ACE_Message_Queue<ACE_MT_SYNCH> * Q, ACE_Message_Block **mb, ACE_Time_Value &absolutewaittime, ACE_Time_Value *pwaittime)
{
    ACE_Time_Value TimeBeforeDeQ, TimeAfterDeQ;

    TimeBeforeDeQ = ACE_OS::gettimeofday();
    int rv = Q -> dequeue_head(*mb, &absolutewaittime);
    TimeAfterDeQ = ACE_OS::gettimeofday();

    *pwaittime += (TimeAfterDeQ-TimeBeforeDeQ);

    return rv;
}


int DirectDeQ( ACE_Message_Queue<ACE_MT_SYNCH> * Q, ACE_Message_Block **mb, ACE_Time_Value &absolutewaittime, ACE_Time_Value *pwaittime)
{
    return Q -> dequeue_head(*mb, &absolutewaittime);
}


int RemoveDirectories(const std::string &parentdirectory, const std::set<std::string> &exceptionlist)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    struct ACE_DIRENT *dp = NULL;
    // PR#10815: Long Path support
    ACE_DIR *dir = sv_opendir(parentdirectory.c_str());

    // Check if we could successfully open the directory dir1
    if (dir == NULL)
    {
        // we could not open the directory dir1
        if (ACE_OS::last_error() != ENOENT)
        {
            // Get the error and display the error
            DebugPrintf(SV_LOG_WARNING, " LocalDirectSync::CleanUpChkptOffsetDir() opendir failed, path = %s, errno = 0x%x\n", parentdirectory.c_str(), ACE_OS::last_error()); 
        }
        return -1;
    }

    //Get directory entry for parentdirectory
    while ((dp = ACE_OS::readdir(dir)) != NULL)
    {
        std::string filename = ACE_TEXT_ALWAYS_CHAR(dp->d_name);
        // we ignore the "." and ".." files
        if ( strcmp(".", filename.c_str()) == 0 || strcmp(".." , filename.c_str()) == 0 )
            continue;

        if ( exceptionlist.end() == exceptionlist.find(filename) )
        {
            std::string chkptDir = parentdirectory + filename.c_str();
            DebugPrintf(SV_LOG_DEBUG, "Cleaning up directory : %s\n", chkptDir.c_str());
            CleanupDirectory(chkptDir.c_str(), "*");

            // PR#10815: Long Path support
            ACE_OS::rmdir(getLongPathName(chkptDir.c_str()).c_str());
        }
    }

    ACE_OS::closedir(dir);
    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return 0;
}

bool IsFileORVolumeExisting(const std::string& sName)
{
    DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
    bool bExist = false;
    ACE_HANDLE hHandle = ACE_INVALID_HANDLE;
    std::string sFile = sName;
    FormatVolumeNameToGuid(sFile);
    SV_ULONG access = O_RDONLY;
    hHandle = ACE_OS::open(getLongPathName(sFile.c_str()).c_str(),access);
    bExist = (ACE_INVALID_HANDLE == hHandle) ? false : true;
    if (bExist)
    {
        ACE_OS::close(hHandle);
    }
    DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
    return bExist;
}


void CreateTransportStream(boost::shared_ptr<TransportStream> &ts, const TRANSPORT_PROTOCOL &tp,
                           const TRANSPORT_CONNECTION_SETTINGS &tss, const VOLUME_SETTINGS::SECURE_MODE &securemode)
{
    /* this calls delete first */
    ts.reset(new (std::nothrow) TransportStream(tp, tss));

    STREAM_MODE mode = GenericStream::Mode_Open | GenericStream::Mode_RW;
    if ( VOLUME_SETTINGS::SECURE_MODE_SECURE == securemode )
        mode |= GenericStream::Mode_Secure;

    if ( SV_FAILURE == ts->Open(mode) )
    {
        ts.reset();
        DebugPrintf(SV_LOG_ERROR, "FAILED: Unable to open transport stream\n");
        return ;
    }
}


int comparememory(const void *s1, const void *s2, size_t n)
{
    return memcmp(s1, s2, n);
}


int alwaysreturnunequal(const void *s1, const void *s2, size_t n)
{
    return 1;
}


bool GetMD5Hash(const std::string &fileNamePath, unsigned char *hash)
{
    bool got = false;

    std::ifstream is;
    is.open(fileNamePath.c_str(), std::ios::in | std::ios::binary);
    if(is.is_open()) 
    {
        INM_MD5_CTX ctx;
        INM_MD5Init(&ctx);

        /* 4k to be more efficient */
        const size_t ALLOCLEN = 4096;
        boost::shared_array<char> buffer(new char[ALLOCLEN]);
        bool read = false;
        std::streamsize count;
        while (!read)
        {
            is.read(buffer.get(), ALLOCLEN);
            if (is.eof())
                read = true;
            count = is.gcount();
            if (count)
                INM_MD5Update(&ctx, (unsigned char*)buffer.get(), count);
        }
        is.close();
        INM_MD5Final(hash, &ctx);
        got = true;
    }
    else
    {
        DebugPrintf(SV_LOG_ERROR, "Failed to open file %s to calculate md5sum\n", fileNamePath.c_str());
    }

    return got;
}


std::string GetBytesInArrayAsHexString(const unsigned char *a, const size_t &length)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::uppercase;
    for (int i = 0; i < length; i++)
    {
        ss << std::setw(2) << (unsigned int)a[i];
    }

    return ss.str();
}

//
// fileTime is 64-bit value representing the number 
// of 100-nanosecond intervals since January 1, 1601 (UTC) 
//
bool ConvertToSVTime( SV_ULONGLONG fileTime, SV_TIME& svTime )
{

    if( fileTime & 0x8000000000000000LL ) {
        return false;
    }
    long long units = fileTime; // initially in 100 nanosecond intervals

    // Milliseconds remaining after seconds are accounted for
    int const IntervalsPerSecond = 10000000;
    int const IntervalsPerMillisecond = IntervalsPerSecond / 1000;
    svTime.wMilliseconds = (static_cast<unsigned>( units % IntervalsPerSecond ) ) / IntervalsPerMillisecond;

    // Time is now in seconds
    units = units / IntervalsPerSecond;

    // How many seconds remain after days are subtracted off
    // Note: no leap seconds!
    int const SecondsPerDay = 86400;
    long days = static_cast<long>( units / SecondsPerDay );
    int secondsInDay = static_cast<int>( units % SecondsPerDay );

    // Calculate hh:mm:ss from seconds 
    int const SecondsPerHour   = 3600;
    int const SecondsPerMinute = 60;
    svTime.wHour = secondsInDay / SecondsPerHour;
    secondsInDay = secondsInDay % SecondsPerHour;
    svTime.wMinute = secondsInDay / SecondsPerMinute;
    svTime.wSecond = secondsInDay % SecondsPerMinute;

    // Calculate day of week
    int const EpochDayOfWeek = 1;   // Jan 1, 1601 was Monday
    int const DaysPerWeek = 7;
    //svTime.wDayOfWeek = ( EpochDayOfWeek + days ) % DaysPerWeek;

    // Compute year, month, and day
    int const DaysPerFourHundredYears = 365 * 400 + 97;
    int const DaysPerNormalCentury = 365 * 100 + 24;
    int const DaysPerNormalFourYears = 365 * 4 + 1;

    long leapCount = ( 3 * ((4 * days + 1227) / DaysPerFourHundredYears) + 3 ) / 4;
    days += 28188 + leapCount;
    long years = ( 20 * days - 2442 ) / ( 5 * DaysPerNormalFourYears );
    long yearDay = days - ( years * DaysPerNormalFourYears ) / 4;
    long months = ( 64 * yearDay ) / 1959;

    // Resulting month based on March starting the year.
    // Adjust for January and February by adjusting year in tandem.
    if( months < 14 ) {
        svTime.wMonth = static_cast<SV_USHORT>(months - 1);
        svTime.wYear = static_cast<SV_USHORT>(years + 1524);
    }
    else {
        svTime.wMonth = static_cast<SV_USHORT>(months - 13);
        svTime.wYear = static_cast<SV_USHORT>(years + 1525);
    }

    // Calculate day of month by using MonthLengths(n)=floor( n * 30.6 ); works for small n
    svTime.wDay = static_cast<SV_USHORT>(yearDay - (1959 * months) / 64);

    svTime.wMicroseconds = static_cast<SV_USHORT>((fileTime / 10) % 1000);
    svTime.wHundrecNanoseconds = static_cast<SV_USHORT>(fileTime % 10);

    return true;
}


bool ConvertTimeToString(SV_ULONGLONG ts, std::string & display)
{
  DebugPrintf(SV_LOG_DEBUG,"ENTERED %s\n",FUNCTION_NAME);
  bool rv = true;
  do
  {
    std::stringstream displaytime;
    SV_TIME   svtime;

    if(!ConvertToSVTime(ts,svtime))
    {
      std::stringstream l_stdfatal;
      l_stdfatal << "Specified Time " << ts  << " is invalid\n";
      DebugPrintf(SV_LOG_FATAL, "%s", l_stdfatal.str().c_str());
      rv = false;
      break;
    }

    displaytime  << svtime.wYear << std::setfill('0') << std::setw(2) <<
        svtime.wMonth  << std::setfill('0') << std::setw(2) <<
        svtime.wDay << std::setfill('0') << std::setw(2) <<
        svtime.wHour << std::setfill('0') << std::setw(2) <<
        svtime.wMinute << std::setfill('0') << std::setw(2) <<
        svtime.wSecond;

    display = displaytime.str();
  } while ( FALSE );

  DebugPrintf(SV_LOG_DEBUG,"EXITED %s\n",FUNCTION_NAME);
  return rv;
}


void PrintOptions(const Options_t &options)
{
    std::stringstream ss;
    ss << "options:\n";
    InsertIntoStream(options, ss);
    DebugPrintf(SV_LOG_DEBUG, "%s", ss.str().c_str());
}


std::string getSystemTimeInUtc(void)
{
  time_t t1 = time(NULL);
  char gmTime[128];
  strftime(gmTime, 128, "%Y%m%d%H%M%S", gmtime(&t1));
  return gmTime;
}

void replace_nonsupported_chars(std::string & filename,
          const std::string & chars_to_replace, char replaced_char)
{
  for(int i = 0; i < chars_to_replace.length() ; ++i)
   std::replace(filename.begin(), filename.end(), chars_to_replace[i] , replaced_char);
}


void RecordFilterDrvVolumeBitmapDelMsg(const std::string &msg)
{
    DebugPrintf(SV_LOG_ERROR, "NOTE: this is not an error. Filter driver volume bitmap delete message: %s.\n", msg.c_str());

    Configurator* TheConfigurator = NULL;
    if(GetConfigurator(&TheConfigurator)) {
        time_t t = time(NULL);
        std::string stime = ctime(&t);
        stime.erase(stime.end()-1);
        std::string logfilename = TheConfigurator->getVxAgent().getCacheDirectory() + ACE_DIRECTORY_SEPARATOR_STR_A + "FilterDrvVolumeBitmapDelMsg.log";
        std::ofstream out(logfilename.c_str(), std::ios::app);
        if (out.is_open()) 
           out << stime << ": " << msg << ".\n";
        else 
           DebugPrintf(SV_LOG_ERROR, "Failed to locally record filter driver volume bitmap delete message %s, because could not open log file %s\n", 
                                     msg.c_str(), logfilename.c_str());
    } else
        DebugPrintf(SV_LOG_ERROR, "Failed to locally record filter driver volume bitmap delete message %s, because could not get configurator\n", msg.c_str());
}
///
/// Performs an HTTP POST to the SV server. 
///
SVERROR PostToSVServer( const char *pszSVServerName,
                       SV_INT HttpPort,
                       const char *pszPostURL,
                       char *pchPostBuffer,
                       SV_ULONG dwPostBufferLength,
                       const bool& pHttps)
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);
    SVERROR sve = SVE_FAIL;

    DebugPrintf(SV_LOG_DEBUG,"Server = %s\n",pszSVServerName);
    DebugPrintf(SV_LOG_DEBUG,"Port = %d\n",HttpPort);
    DebugPrintf(SV_LOG_DEBUG,"Secure = %d\n",pHttps);
    DebugPrintf(SV_LOG_DEBUG,"PHP URL = %s\n",pszPostURL);


    CurlOptions options(pszSVServerName,HttpPort, pszPostURL, pHttps);


    try {
        CurlWrapper cw;
        cw.post(options,pchPostBuffer);

        DebugPrintf(SV_LOG_DEBUG,"Successfully post to SVServer %s\n",pszPostURL);
        sve = SVS_OK;
    } catch(ErrorException& exception )
    {
        DebugPrintf(SV_LOG_ERROR, "FAILED : %s with error %s\n",FUNCTION_NAME, exception.what());
        return sve;
    }


    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    return sve;
}


std::string GetIOSState(std::ios &stream)
{
    std::stringstream ss;
    ss << "good = " << STRBOOL(stream.good()) << ", "
        << "eof = " << STRBOOL(stream.eof()) << ", "
        << "fail = " << STRBOOL(stream.fail()) << ", "
        << "bad = " << STRBOOL(stream.bad());
    return ss.str();
}

std::string GenerateUuid()
{
    DebugPrintf(SV_LOG_DEBUG, "Entering %s\n", __FUNCTION__);

    std::stringstream suuid;

    try
    {
        boost::uuids::random_generator uuid_gen;

        boost::uuids::uuid new_uuid = uuid_gen();
                
        suuid << new_uuid;
    }
    catch (const std::exception& e)
    {
        DebugPrintf(SV_LOG_ERROR, "Could not generate UUID. Exception: %s\n", e.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "Could not generate UUID. Unkown exception\n");
    }

    DebugPrintf(SV_LOG_DEBUG, "Exiting %s\n", __FUNCTION__);
    
    return suuid.str();
}

uint64_t GetTimeInMilliSecSinceEpoch1970()
{
    uint64_t msecSinceEpoch = boost::posix_time::time_duration(boost::posix_time::microsec_clock::universal_time()
        - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1))).total_milliseconds();

    return msecSinceEpoch;
}

uint64_t GetTimeInSecSinceEpoch1970()
{
    return GetTimeInMilliSecSinceEpoch1970() / 1000;
}

uint64_t GetTimeInSecSinceAd0001()
{
    const uint64_t TIME_IN_SECS_BETWEEN_AD_AND_EPOCH = 62135596800;
    uint64_t secSinceEpoch = GetTimeInSecSinceEpoch1970();
    secSinceEpoch += TIME_IN_SECS_BETWEEN_AD_AND_EPOCH;

    return secSinceEpoch;
}
uint64_t GetSecsBetweenEpoch1970AndEpoch1601()
{
    uint64_t diff = boost::gregorian::date_duration(boost::gregorian::date(1970, 1, 1)
        - boost::gregorian::date(1601, 1, 1)).days();
    diff *= 24 * 60 * 60;
    return diff;
}
uint64_t GetTimeInSecSinceEpoch1601()
{
    uint64_t secSinceEpoch = GetTimeInSecSinceEpoch1970();
    secSinceEpoch += GetSecsBetweenEpoch1970AndEpoch1601();
    return secSinceEpoch;
}

void GetHypervisorInfo(HypervisorInfo_t &hypervinfo)
{
    DebugPrintf(SV_LOG_DEBUG, "Determining Hypervisor information\n");
    bool bisvirtual = IsVirtual(hypervinfo.name, hypervinfo.version);
    hypervinfo.state = bisvirtual ? HypervisorInfo_t::HyperVisorPresent : 
        HypervisorInfo_t::HypervisorNotPresent;

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return;
}

#ifdef SV_WINDOWS
bool PersistPlatformTypeForDriver()
{
    return true;
}
#else
bool PersistPlatformTypeForDriver()
{
    static bool bstatus = false;

    if (bstatus) return bstatus;

    boost::system::error_code ec;
    std::string platformTypePersistentFile;
    LocalConfigurator lc;
    if (!LocalConfigurator::getVxPlatformTypeForDriverPersistentFile(platformTypePersistentFile))
    {
        DebugPrintf(SV_LOG_ERROR,
            "%s: failed to get platform type persistent file path from config.\n",
            FUNCTION_NAME);

        return bstatus;
    }

    if (boost::filesystem::exists(platformTypePersistentFile, ec))
    {
        DebugPrintf(SV_LOG_DEBUG,
            "%s: platform type persistent file found at %s.\n",
            FUNCTION_NAME,
            platformTypePersistentFile.c_str());
    }
    else
    {
        std::string newDir = dirname_r(platformTypePersistentFile);
        if (!boost::filesystem::exists(newDir, ec))
        {
            boost::filesystem::create_directories(newDir, ec);
            if (ec)
            {
                DebugPrintf(SV_LOG_ERROR,
                    "%s: creating directory %s failed. Error %d (%s).\n",
                    FUNCTION_NAME,
                    ec.value(),
                    ec.message().c_str());

                return bstatus;
            }

            securitylib::setPermissions(newDir, securitylib::SET_PERMISSIONS_NAME_IS_DIR);
        }
    }

    std::ofstream platFile(platformTypePersistentFile.c_str(), std::ios::trunc);
    if (!platFile.is_open())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: file %s open failed.\n",
                        FUNCTION_NAME, platformTypePersistentFile.c_str());

        return bstatus;
    }

    std::string vmPlatform = lc.getVmPlatform();
    std::string csType = lc.getCSType();

    platFile << "VMPLATFORM=" << vmPlatform << std::endl;
    platFile << "CS_TYPE=" << csType << std::endl;
    platFile.flush();

    if (!platFile.good())
    {
        DebugPrintf(SV_LOG_ERROR, "%s: file %s write failed.\n",
                        FUNCTION_NAME, platformTypePersistentFile.c_str());

        platFile.close();
        return bstatus;
    }

    platFile.close();

    securitylib::setPermissions(platformTypePersistentFile);

    bstatus = true;
    DebugPrintf(SV_LOG_ALWAYS,
        "%s: platform type persistent file created at %s.\n",
        FUNCTION_NAME,
        platformTypePersistentFile.c_str());

    return bstatus;
}
#endif

std::string GetImdsMetadata()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    MemoryStruct chunk = {0};
    CURL *curl = curl_easy_init();
    try
    {
        chunk.size = 0;
        chunk.memory = NULL;
        if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_URL, IMDS_URL)) {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to set curl options IMDS_URL.\n";
        }

        if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_NOPROXY, "*")) {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to set curl options CURLOPT_NOPROXY.\n";
        }

        struct curl_slist * pheaders = curl_slist_append(NULL, IMDS_HEADERS);
        if (CURLE_OK != curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pheaders)) {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to set curl options CURLOPT_HEADERDATA.\n";
        }
        
        if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallbackFileReplication)) {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to set curl options CURLOPT_WRITEFUNCTION.\n";
        }

        if(CURLE_OK != curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void *>( &chunk ))) {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to set curl options CURLOPT_WRITEDATA.\n";
        }

        CURLcode curl_code = curl_easy_perform(curl);

        if (curl_code == CURLE_ABORTED_BY_CALLBACK)
        {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to perfvorm curl request, request aborted.\n";
        }

        long response_code = 0L;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != HTTP_OK)
        {
            throw ERROR_EXCEPTION << FUNCTION_NAME << ": Failed to perform curl request, curl error "
                << curl_code << ": " << curl_easy_strerror(curl_code)
                << ", status code " << response_code
                << ((chunk.memory != NULL) ? (std::string(", error ") + chunk.memory) : "") << ".\n";
        }
    }
    catch (std::exception& e)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Failed  with exception: %s.\n", FUNCTION_NAME, e.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Failed  with exception.\n", FUNCTION_NAME);
    }

    std::string ret;
    if (chunk.memory != NULL)
    {
        ret = std::string(chunk.memory, chunk.size);
        free(chunk.memory);
    }
    curl_easy_cleanup(curl);

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);

    return ret;

}
bool IsAzureStackVirtualMachine()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    static bool s_bIsCheckComplete = false;
    static bool s_IsAzureStackVm = false;

    if (s_bIsCheckComplete)
    {
        return s_IsAzureStackVm;
    }

    std::string strAssetTag = GetChassisAssetTag();
    bool isAzureAssetTag = boost::iequals(strAssetTag, AZURE_ASSET_TAG);

    if (!isAzureAssetTag)
    {
        s_bIsCheckComplete = true;
        return s_IsAzureStackVm;
    }

    try
    {
        // Get IMDS information. Always HTTP
        std::string imdsMetadata = GetImdsMetadata();
        std::istringstream stream(imdsMetadata);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json(stream, pt);
        std::string azEnv = pt.get<std::string>(IMDS_COMPUTE_ENV);
        DebugPrintf(SV_LOG_DEBUG, "Az Environment: %s\n", azEnv.c_str());
        s_IsAzureStackVm = boost::iequals(azEnv, IMDS_AZURESTACK_NAME);
    }
    catch (std::exception& e)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Failed  with exception: %s.\n", FUNCTION_NAME, e.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s: Failed  with exception.\n", FUNCTION_NAME);
    }

    
    s_bIsCheckComplete = true;
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return s_IsAzureStackVm;
}

bool IsAzureVirtualMachine()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    static std::string strAssetTag = "";
    if (strAssetTag.empty())
    {
        strAssetTag = GetChassisAssetTag();
    }
    DebugPrintf(SV_LOG_DEBUG, "Asset Tag: %s\n", strAssetTag.c_str());

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return boost::iequals(strAssetTag, AZURE_ASSET_TAG) && !IsAzureStackVirtualMachine();
}

#ifdef SV_WINDOWS
int GetDeviceNameTypeToReport(const std::string& hypervisorName, const bool isMigrateToRcm)
{
    return DEVICE_NAME_TYPE_DISK_SIGNATURE;
}
#else
int GetDeviceNameTypeToReport(const std::string& hypervisorName, const bool isMigrateToRcm)
{
        static std::string s_strVmPlatform;
        static std::string s_strHypervisorName;
        static std::string s_strAssetTag;
        static std::string s_strCsType;

        if (s_strVmPlatform.empty())
        {
            LocalConfigurator   lc;
            s_strVmPlatform = lc.getVmPlatform();
            s_strCsType = lc.getCSType();
        }

        if (hypervisorName.empty())
        {
            if (s_strHypervisorName.empty())
            {
                HypervisorInfo_t hypervinfo;
                GetHypervisorInfo(hypervinfo);
                s_strHypervisorName = hypervinfo.name;
                PrintHypervisorInfo(hypervinfo);
            }
        }

        if (s_strAssetTag.empty())
        {
            s_strAssetTag = GetChassisAssetTag();
        }

        // for A2A and V2A Failover VM (running on Azure)
        if ( boost::iequals(s_strVmPlatform, AZUREPLATFORM) ||
                 ( (boost::iequals(hypervisorName.empty() ? s_strHypervisorName : hypervisorName, MICROSOFTNAME)) &&
                    boost::iequals(s_strVmPlatform, VMWAREPLATFORM) &&
                    boost::iequals(s_strAssetTag, AZURE_ASSET_TAG) ) )
        {
            DebugPrintf(SV_LOG_DEBUG, "EXITED %s with DEVICE_NAME_TYPE_LU_NUMBER\n", FUNCTION_NAME);
            return DEVICE_NAME_TYPE_LU_NUMBER;
        }
        // for V2A VM runnong on VmWare and the CS_TYPE is CS_PRIME
        else if (boost::iequals(s_strVmPlatform, VMWAREPLATFORM) 
            && (boost::iequals(s_strCsType, CSTYPE_CSPRIME) || isMigrateToRcm))
        {
            DebugPrintf(SV_LOG_DEBUG, "EXITED %s with DEVICE_NAME_TYPE_FROM_PERSISTENT_MAP\n", FUNCTION_NAME);
            return DEVICE_NAME_TYPE_FROM_PERSISTENT_MAP;
        }

        DebugPrintf(SV_LOG_DEBUG, "EXITED %s with DEVICE_NAME_TYPE_OS_NAME\n", FUNCTION_NAME);
        return DEVICE_NAME_TYPE_OS_NAME;
}
#endif

bool IsAgentRunningOnAzureVm()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);

    static bool s_bIsCheckComplete = false;
    static bool s_IsAzureVm = false;

    if (s_bIsCheckComplete)
        return s_IsAzureVm;

    HypervisorInfo_t hypervinfo;
    GetHypervisorInfo(hypervinfo);

    DebugPrintf(SV_LOG_DEBUG, "agent is running on a VM on hypervisor %s.\n", hypervinfo.name.c_str());

    if (hypervinfo.state == HypervisorInfo::HyperVisorPresent)
    {
        std::string assettag = GetChassisAssetTag();
        std::string expectedHyperVName;

#ifdef SV_WINDOWS
        expectedHyperVName = HYPERVNAME;
#else
        expectedHyperVName = MICROSOFTNAME;
#endif
        if (boost::iequals(hypervinfo.name, expectedHyperVName))
        {
            if (boost::iequals(assettag, AZURE_ASSET_TAG) && !IsAzureStackVirtualMachine())
            {
                DebugPrintf(SV_LOG_ALWAYS, "Detected agent is running on Azure VM.\n");
                s_IsAzureVm = true;
            }
            else if (assettag.empty())
            {
                throw ERROR_EXCEPTION << "Asset tag on Microsoft Hyper-V VM is empty.\n";
            }
        }
    }
    
    s_bIsCheckComplete = true;

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
    return s_IsAzureVm;
}

std::string GetCxIpAddress()
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    LocalConfigurator lc;
    std::string ipAddress =
        (IsAgentRunningOnAzureVm() && !lc.getCsAddressForAzureComponents().empty()) ?
        lc.getCsAddressForAzureComponents() :
        lc.getHttp().ipAddress;

    DebugPrintf(SV_LOG_DEBUG, "EXITED %s CS IP Addess: %s\n", FUNCTION_NAME, ipAddress.c_str());
    return ipAddress;
}

/// \brief Delete the vxprotdevdetails.dat file present either at the old path or new path
///        to be called during the disable protection
bool DeleteProtectedDeviceDetails()
{
    bool bstatus = true;
    do
    {
        std::string devDetailsCachePath;
        bstatus = LocalConfigurator::getVxProtectedDeviceDetailCachePathname(devDetailsCachePath);
        if (!bstatus)
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s: failed to delete protected device details file as failed to get path from config.\n",
                FUNCTION_NAME);
            break;
        }

        std::string depDevDetailsCachePath;
        bstatus = LocalConfigurator::getDeprecatedVxProtectedDeviceDetailCachePathname(depDevDetailsCachePath);
        if (!bstatus)
        {
            DebugPrintf(SV_LOG_ERROR,
                "%s: failed to delete protected device details file as failed to get old path from config.\n",
                FUNCTION_NAME);
            break;
        }

        boost::system::error_code ec;
        std::vector<std::string> devDetailsCachePaths;
        devDetailsCachePaths.push_back(devDetailsCachePath);
        devDetailsCachePaths.push_back(depDevDetailsCachePath);
        std::vector<std::string>::iterator pathIter = devDetailsCachePaths.begin();

        for (/*empty*/; pathIter != devDetailsCachePaths.end(); pathIter++)
        {
            if (!boost::filesystem::exists(*pathIter, ec))
            {
                DebugPrintf(SV_LOG_DEBUG,
                    "%s: file %s is not present.\n",
                    FUNCTION_NAME,
                    pathIter->c_str());
                continue;
            }

            boost::filesystem::remove(*pathIter, ec);
            if (ec)
            {
                DebugPrintf(SV_LOG_ERROR,
                    "%s: failed to delete protected device details file with error %d (%s).\n",
                    FUNCTION_NAME,
                    ec.value(),
                    ec.message().c_str());
                bstatus = false;
                continue;
            }

            DebugPrintf(SV_LOG_ALWAYS,
                "%s: deleted protected device details file %s.\n",
                FUNCTION_NAME,
                pathIter->c_str());
        }

    } while (false);

    return bstatus;
}
std::string  InmGetFormattedSize(unsigned long long ullSize)
{
    std::stringstream   ssFormattedSize;

    std::string     aformats[] = { "GB", "MB", "KB", "Bytes" };
    std::vector<std::string>     formats(aformats, aformats + INM_ARRAY_SIZE(aformats));

    unsigned long long ullFormatShifter = 30;
    unsigned long long ullFormattedSize = ullSize;

    size_t iFmtIndex = 0;

    for (iFmtIndex = 0; iFmtIndex < formats.size() - 1; iFmtIndex++)
    {
        ullFormattedSize = ullSize >> ullFormatShifter;
        if (ullFormattedSize > 0)  break;
        ullFormatShifter -= 10;
        ullFormattedSize = ullSize;
    }

    ssFormattedSize << ullFormattedSize << formats[iFmtIndex];
    return ssFormattedSize.str();
}

void ExtractCacheStorageNameFromBlobContainerSasUrl(const std::string& blobContainerSasUrl, std::string& cacheStorageAccountName)
{
    DebugPrintf(SV_LOG_DEBUG, "ENTERED %s\n", FUNCTION_NAME);
    try {
        //extracting according to the standard sas format: https://{myaccount}.blob.core.windows.net/{sascontainer}/sasblob.txt
        size_t offset = 2;
        size_t first = blobContainerSasUrl.find("//");
        size_t last = blobContainerSasUrl.find(".blob.core.windows.net");

        if (first == std::string::npos || last == std::string::npos || first >= last)
        {
            std::string blobSasUriToBeLogged = blobContainerSasUrl.substr(
                0, blobContainerSasUrl.find("?"));

            DebugPrintf(SV_LOG_ERROR, "%s: Failed to extract cache storage account name as the blobContainerSasUri %s is not in known format.\n", FUNCTION_NAME, blobSasUriToBeLogged.c_str());
            DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
            return;
        }
        {
            cacheStorageAccountName = blobContainerSasUrl.substr(first + offset, last - first - offset);
        }
    }
    catch (const std::exception& e)
    {
        DebugPrintf(SV_LOG_ERROR, "%s Failed with an exception : %s.\n", FUNCTION_NAME, e.what());
    }
    catch (...)
    {
        DebugPrintf(SV_LOG_ERROR, "%s Failed with an unnknown exception.\n", FUNCTION_NAME);
    }
    DebugPrintf(SV_LOG_DEBUG, "EXITED %s\n", FUNCTION_NAME);
}
