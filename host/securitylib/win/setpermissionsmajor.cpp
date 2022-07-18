#include <windows.h>
#include <fstream>
#include <vector>
#include <string>
#include <Aclapi.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/thread/mutex.hpp>

#include "errorexception.h"
#include "scopeguard.h"
#include "createpaths.h"
#include "extendedlengthpath.h"

#include "setpermissions.h"


static SECURITY_ATTRIBUTES g_defaultFileSA;
static SECURITY_ATTRIBUTES g_defaultFileSA_WithIuser;
static SECURITY_ATTRIBUTES g_defaultDirectorySA;
static SECURITY_ATTRIBUTES g_defaultDirectorySA_WithIuser;

static bool g_defaultFileSAInitialized = false;
static bool g_defaultFileSAInitialized_WithIuser = false;
static bool g_defaultDirectorySAInitialized = false;
static bool g_defaultDirectorySAInitialized_WithIuser = false;

static boost::mutex g_defaultFileSAMutex;
static boost::mutex g_defaultFileSAMutex_WithIuser;
static boost::mutex g_defaultDirectorySAMutex;
static boost::mutex g_defaultDirectorySAMutex_WithIuser;


namespace  securitylib {

    typedef std::vector<char> sid_t;

    int defaultFilePermissions() {
        return (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    }

    sid_t getIusrSid(bool ignoreWebUserChk = false)
    {
        SID_NAME_USE sidNameUse;
        DWORD sidSize = 0;
        DWORD domainSize = 0;
        sid_t sid;
        DWORD res = 0;
        if (!LookupAccountName(0, "IUSR", 0, &sidSize, 0, &domainSize, &sidNameUse)) {
            res = GetLastError();
            if (ignoreWebUserChk){
                return sid;
            }
            else {
                throw ERROR_EXCEPTION << "LookupAccountName Error: " << res << '\n';
            }
            sid.resize(sidSize);
            std::vector<char> domainName(domainSize);
            if (!LookupAccountName(0, "IUSR", &sid[0], &sidSize, &domainName[0], &domainSize, &sidNameUse)) {
                res = GetLastError();
                throw ERROR_EXCEPTION << "LookupAccountName Error: " << res << '\n';
            }
        }
        return sid;
    }

    ///
    /// Returns System ACL or IUSR ACL for dir or file as specified in input params.
    /// Caller is required to free returned ACL after use.
    ///
    PACL getAcl(bool isNameDirectory = false, bool allowWebServer = false, bool ignoreWebUserChk = false)
    {
        DWORD res = 0;
        PSID adminSid = 0;
        SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
        if (!AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminSid)) {
            res = GetLastError();
            throw ERROR_EXCEPTION << "AllocateAndInitializeSid Error: " << res << '\n';
        }
        ON_BLOCK_EXIT(boost::bind<PVOID>(&FreeSid, adminSid));

        PACL adminAcl = 0;
        EXPLICIT_ACCESS explicitAccess = { 0 };
        explicitAccess.grfAccessMode = SET_ACCESS;
        explicitAccess.grfAccessPermissions = TRUSTEE_ACCESS_ALL;
        if (isNameDirectory) {
            explicitAccess.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
        }
        else {
            explicitAccess.grfInheritance = NO_INHERITANCE;
        }
        explicitAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
        explicitAccess.Trustee.pMultipleTrustee = 0;
        explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        explicitAccess.Trustee.ptstrName = (LPSTR)adminSid;

        res = SetEntriesInAcl(1, &explicitAccess, 0, &adminAcl);
        if (ERROR_SUCCESS != res)
        {
            throw ERROR_EXCEPTION << "SetEntriesInAcl failed for admin. Error: " << res << '\n';
        }
        ON_BLOCK_EXIT(boost::bind<void>(&LocalFree, adminAcl));

        PSID systemSid = 0;
        if (!AllocateAndInitializeSid(&SIDAuthNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &systemSid)) {
            res = GetLastError();
            throw ERROR_EXCEPTION << "AllocateAndInitializeSid Error: " << res << '\n';
        }
        ON_BLOCK_EXIT(boost::bind<PVOID>(&FreeSid, systemSid));

        explicitAccess.Trustee.ptstrName = (LPSTR)systemSid;
        PACL systemAcl = 0;
        res = SetEntriesInAcl(1, &explicitAccess, adminAcl, &systemAcl);
        if (ERROR_SUCCESS != res)
            throw ERROR_EXCEPTION << "SetEntriesInAcl failed for SYSTEM. Error: " << res << '\n';

        sid_t iusrSid;
        PACL iusrAcl = 0;
        if (allowWebServer) {
            iusrSid = securitylib::getIusrSid(ignoreWebUserChk);
            if (!iusrSid.empty()) {
                EXPLICIT_ACCESS iusrAccess = { 0 };
                iusrAccess.grfAccessMode = SET_ACCESS;
                iusrAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
                if (isNameDirectory) {
                    iusrAccess.grfInheritance = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
                }
                else {
                    iusrAccess.grfInheritance = NO_INHERITANCE;
                }
                iusrAccess.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                iusrAccess.Trustee.pMultipleTrustee = 0;
                iusrAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
                iusrAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
                iusrAccess.Trustee.ptstrName = (LPSTR)&iusrSid[0];
                res = SetEntriesInAcl(1, &iusrAccess, systemAcl, &iusrAcl);
                if (ERROR_SUCCESS != res)
                {
                    if (systemAcl) {
                        ON_BLOCK_EXIT(boost::bind<void>(&LocalFree, systemAcl));
                    }
                    throw ERROR_EXCEPTION << "SetEntriesInAcl failed for iusr. Error: " << res << '\n';
                }
            }
        }

        if (iusrAcl) {
            return iusrAcl;
        }

        return systemAcl;
    }

    void setAcl(std::string const& name, int setFlags)
    {
        PACL effectiveacl = getAcl( (SET_PERMISSIONS_NAME_IS_DIR & setFlags),
                                    (SET_PERMISSIONS_ALLOW_WEB_SERVER & setFlags),
                                    (SET_PERMISSIONS_IGNORE_WEBUSER_CHK & setFlags) );
        ON_BLOCK_EXIT(boost::bind<void>(&LocalFree, effectiveacl));
        DWORD res = 0;
        // this will remove everything and replace it with what is in the acl that is used
        res = SetNamedSecurityInfoW( (LPWSTR)ExtendedLengthPath::name(name).c_str(),
                                        SE_FILE_OBJECT,
                                        DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                        0,
                                        0,
                                        effectiveacl,
                                        0 );
        if (ERROR_SUCCESS != res) {
            res = GetLastError();
            throw ERROR_EXCEPTION << "SetNamedSecurityInfo Error: " << res << '\n';
        }
    }

    // NOTE:  name can be file or directory,
    // if directroy need to set SET_PERMISSIONS_NAME_IS_DIR or SET_PERMISSIONS_NO_CREATE
    void setPermissions(std::string const& name, int setFlags)
    {
        if (name.empty())
        {
            throw ERROR_EXCEPTION << "name is blank." << '\n';
        }
        if (!boost::filesystem::exists(name)) {
            if (0 == (SET_PERMISSIONS_NO_CREATE & setFlags)) {
                CreatePaths::createPathsAsNeeded(name);
                if (0 == (SET_PERMISSIONS_NAME_IS_DIR & setFlags)) {
                    HANDLE hfile = CreateFileW( ExtendedLengthPath::name(name).c_str(),
                                    GENERIC_ALL,
                                    defaultFilePermissions(),
                                    defaultSecurityAttributes(false,
                                                              SET_PERMISSIONS_ALLOW_WEB_SERVER & setFlags,
                                                              SET_PERMISSIONS_IGNORE_WEBUSER_CHK & setFlags),
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );
                    if (hfile == INVALID_HANDLE_VALUE)
                    {
                        DWORD err = GetLastError();
                        throw ERROR_EXCEPTION << "FAILED to create file: " << name << ". Error: " << err << '\n';
                    }
                    CloseHandle(hfile);
                }
                else {
                    bool res = false;
                    res = CreateDirectoryW( ExtendedLengthPath::name(name).c_str(),
                                            defaultSecurityAttributes(true,
                                                SET_PERMISSIONS_ALLOW_WEB_SERVER & setFlags,
                                                SET_PERMISSIONS_IGNORE_WEBUSER_CHK & setFlags) );
                    if (!res) {
                        DWORD err = GetLastError();
                        throw ERROR_EXCEPTION << "FAILED to create directory: " << name << ". Error: " << err << '\n';
                    }
                }
            }
            else {
                return;
            }
        }
        else
        {
            setAcl(name, setFlags);
        }
        if (0 != (SET_PERMISSIONS_PARENT_DIR & setFlags)) {
            std::string::size_type idx = name.find_last_of("/\\");
            if (std::string::npos != idx) {
                setAcl( name.substr(0, idx), setFlags );
            }
        }
    }

    // TODO This is conterpart of Linux function and to compile the Windows code, added
    // this function. This windows code may use this function to set the correct permissions
    // for world writeable files.
    bool setPermissions(const std::string &path, std::string &errstr)
    {
        return true;
    }

    const PSECURITY_DESCRIPTOR getSecurityDescriptor(bool isNameDirectory = false, bool allowWebServer = false, bool ignoreWebUserChk = false)
    {
        PACL effectiveacl = getAcl(isNameDirectory, allowWebServer, ignoreWebUserChk); // Not freeing effectiveacl here as is referenced by SecurityDescriptor being created
        DWORD res = 0;

        PSECURITY_DESCRIPTOR pSD = NULL;
        pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (NULL == pSD)
        {
            res = GetLastError();
            throw ERROR_EXCEPTION << "LocalAlloc Error: " << res << '\n';
        }

        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        {
            res = GetLastError();
            ON_BLOCK_EXIT(boost::bind<void>(&LocalFree, pSD));
            throw ERROR_EXCEPTION << "InitializeSecurityDescriptor Error: " << res << '\n';
        }

        if ( !SetSecurityDescriptorDacl(pSD, TRUE, effectiveacl, FALSE) )
        {
            res = GetLastError();
            ON_BLOCK_EXIT(boost::bind<void>(&LocalFree, pSD));
            throw ERROR_EXCEPTION << "SetSecurityDescriptorDacl Error: " << res << '\n';
        }

        return pSD;
    }


    const LPSECURITY_ATTRIBUTES defaultSecurityAttributes(bool isNameDirectory, bool allowWebServer, bool ignoreWebUserChk)
    {
        /// default file security attribute
        if (!allowWebServer && !isNameDirectory) {
            if (!g_defaultFileSAInitialized) {
                boost::mutex::scoped_lock gaurd(g_defaultFileSAMutex);
                if (!g_defaultFileSAInitialized) {
                    g_defaultFileSA.bInheritHandle = false;
                    g_defaultFileSA.nLength = sizeof(SECURITY_ATTRIBUTES);
                    g_defaultFileSA.lpSecurityDescriptor = NULL;
                    g_defaultFileSA.lpSecurityDescriptor = getSecurityDescriptor(isNameDirectory, allowWebServer, ignoreWebUserChk);
                    g_defaultFileSAInitialized = true;
                }
            }
            return &g_defaultFileSA;
        }
        
        /// default file security attribute with default permissions to web user
        if (allowWebServer && !isNameDirectory) {
            if (!g_defaultFileSAInitialized_WithIuser) {
                boost::mutex::scoped_lock gaurd(g_defaultFileSAMutex_WithIuser);
                if (!g_defaultFileSAInitialized_WithIuser) {
                    g_defaultFileSA_WithIuser.bInheritHandle = false;
                    g_defaultFileSA_WithIuser.nLength = sizeof(SECURITY_ATTRIBUTES);
                    g_defaultFileSA_WithIuser.lpSecurityDescriptor = NULL;
                    g_defaultFileSA_WithIuser.lpSecurityDescriptor = getSecurityDescriptor(isNameDirectory, allowWebServer, ignoreWebUserChk);
                    g_defaultFileSAInitialized_WithIuser = true;
                }
            }
            return &g_defaultFileSA_WithIuser;
        }

        /// default directory security attribute
        if (!allowWebServer && isNameDirectory) {
            if (!g_defaultDirectorySAInitialized) {
                boost::mutex::scoped_lock gaurd(g_defaultDirectorySAMutex);
                if (!g_defaultDirectorySAInitialized) {
                    g_defaultDirectorySA.bInheritHandle = false;
                    g_defaultDirectorySA.nLength = sizeof(SECURITY_ATTRIBUTES);
                    g_defaultDirectorySA.lpSecurityDescriptor = NULL;
                    g_defaultDirectorySA.lpSecurityDescriptor = getSecurityDescriptor(isNameDirectory, allowWebServer, ignoreWebUserChk);
                    g_defaultDirectorySAInitialized = true;
                }
            }
            return &g_defaultDirectorySA;
        }

        /// default directory security attribute  with default permissions to web user
        if (allowWebServer && isNameDirectory) {
            if (!g_defaultDirectorySAInitialized_WithIuser) {
                boost::mutex::scoped_lock gaurd(g_defaultDirectorySAMutex_WithIuser);
                if (!g_defaultDirectorySAInitialized_WithIuser) {
                    g_defaultDirectorySA_WithIuser.bInheritHandle = false;
                    g_defaultDirectorySA_WithIuser.nLength = sizeof(SECURITY_ATTRIBUTES);
                    g_defaultDirectorySA_WithIuser.lpSecurityDescriptor = NULL;
                    g_defaultDirectorySA_WithIuser.lpSecurityDescriptor = getSecurityDescriptor(isNameDirectory, allowWebServer, ignoreWebUserChk);
                    g_defaultDirectorySAInitialized_WithIuser = true;
                }
            }
            return &g_defaultDirectorySA_WithIuser;
        }
    }

}  // namespace securitylib
