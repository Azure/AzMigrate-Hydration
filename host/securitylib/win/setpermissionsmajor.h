
///
/// \file setpermissionsmajor.h
///
/// \brief
///

#ifndef SETPERMISSIONSMAJOR_H
#define SETPERMISSIONSMAJOR_H

#include <Windows.h>

namespace securitylib {

    char const * const ROOT_ADMIN_NAME = "Administrator";

    inline bool isRootAdmin()
    {
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        PSID adminGroup = 0;
        BOOL isMember = FALSE;
        if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
            CheckTokenMembership(NULL, adminGroup, &isMember);
            FreeSid(adminGroup);
        }
        return (TRUE == isMember); // using TRUE == just to keep compiler quiet
    }

    /// defaultSecurityAttributes function returns default security attribute for file or directory as,
    ///            Windows                                 Unix
    /// File       AUTHORITY\SYSTEM:(F)                    root       : rw-
    ///            BUILTIN\Administrators:(F)              root group : rw-
    /// Directory  NT AUTHORITY\SYSTEM:(OI)(CI)(F)         root       : rwx
    ///            BUILTIN\Administrators:(OI)(CI)(F)      root group : rwx
    /// Parameters:
    /// isNameDirectory[in]
    ///     If true name object is directory else file. Default is false.
    ///
    /// allowWebServer[in]
    ///     If true IUSR is allowed with default access permissions. Default is false.
    ///
    /// ignoreWebUserChk[in]
    ///     If true IUSR check is ignored and no exception throws if IUSR not exist. Default is false.
    ///
    /// eg.
    ///        CreateFile( <file name>,
    ///                        GENERIC_ALL,
    ///                        securitylib::defaultFilePermissions(),
    ///                        securitylib::defaultSecurityAttributes(),
    ///                        CREATE_ALWAYS,
    ///                        FILE_ATTRIBUTE_NORMAL,
    ///                        NULL );
    ///
    ///        ACE_OS::open( <file name>,
    ///                        O_WRONLY|O_CREAT|O_TRUNC,
    ///                        securitylib::defaultFilePermissions(),
    ///                        securitylib::defaultSecurityAttributes() );
    ///
    ///        CreateDirectory( <directory name>, securitylib::defaultSecurityAttributes(true) );
    ///
    ///        Create directory with access to web server user:
    ///        CreateDirectory( <directory name>, securitylib::defaultSecurityAttributes(true, true) );
    const LPSECURITY_ATTRIBUTES defaultSecurityAttributes(bool isNameDirectory = false,
                                                          bool allowWebServer = false,
                                                          bool ignoreWebUserChk = false);

    /// defaultSharingMode function returns default file share mode.
    /// Note: Do not use this API if specific share mode is required.
    /// eg.
    ///            CreateFile(<file name>,
    ///                            GENERIC_ALL,
    ///                         securitylib::defaultSharingMode(),
    ///                         securitylib::defaultSecurityAttributes(),
    ///                         CREATE_ALWAYS,
    ///                         FILE_ATTRIBUTE_NORMAL,
    ///                         NULL);
    ///
    ///            ACE_OS::open(<file name>,
    ///                            O_WRONLY|O_CREAT|O_TRUNC,
    ///                            securitylib::defaultSharingMode(),
    ///                            securitylib::defaultSecurityAttributes() );
    ///    
    /// On Unix defaultSharingMode function returns default file permission mode "rw" to root and its group.
    /// eg.
    ///            open(<file name>, O_CREAT|O_WRONLY|O_TRUNC, securitylib::defaultSharingMode());
    ///            creat(<file name>, securitylib::defaultSharingMode());
    ///            ACE_OS::creat(<file name>, securitylib::defaultSharingMode());
    ///    Note: Do not use ACE_OS::creat on windows to create files as it does not given an option to specify security attributes.
    int defaultSharingMode();

} // namespace securitylib


#endif // SETPERMISSIONSMAJOR_H
