
///
/// \file setpermissins.h
///
/// \brief
///

#ifndef SETPERMISSINS_H
#define SETPERMISSINS_H

namespace securitylib {

    enum SET_PERMISSIONS_FLAG {
        SET_PERMISSIONS_PARENT_DIR          = 0x0001,  // Also set the parent dir (convient to avoid having to
                                                       // set parent separately.
        SET_PERMISSIONS_NO_CREATE           = 0x0002,  // If files does not exist, do not create treat as error.
        SET_PERMISSIONS_ALLOW_WEB_SERVER    = 0x0004,  // Allow web server user (grp) access.
        SET_PERMISSIONS_IGNORE_WEBUSER_CHK  = 0x0008,  // Web user check is ignored and no exception throws
                                                       // if web user not exist.
        SET_PERMISSIONS_NAME_IS_DIR         = 0x0010   // Says the name is a directory not a file.
    };
    
    // Set default permissions on the object (file or directory), creates the file if object does not exist.
    int const SET_PERMISSIONS_DEFAULT = 0;

    /// setPermissions function set the default permissions on name object (file/directory) as,
    ///            Windows                                 Unix
    /// File       AUTHORITY\SYSTEM:(F)                    root       : rw-
    ///            BUILTIN\Administrators:(F)              root group : rw-
    /// Directory  NT AUTHORITY\SYSTEM:(OI)(CI)(F)         root       : rwx
    ///            BUILTIN\Administrators:(OI)(CI)(F)      root group : rwx
    ///
    /// Default action:
    ///     Set default permissions on existing name object (file/directory).
    ///     If name object (file/directory) does not exist, setPermissions creates name as a file.
    ///     eg. securitylib::setPermissions(<file/directory name>);
    /// Note: setPermissions throws std::exception in error condition.
    ///
    /// Parameters:
    /// name[in]
    ///     file/directory name
    ///
    /// setFlags[in]
    ///     Default is SET_PERMISSIONS_DEFAULT.
    ///     Values of SET_PERMISSIONS_FLAG can be ORed to set combination of flags.
    ///     eg. To indicate name object is directory and to allows permissions to web server user,
    ///     securitylib::setPermissions(<directory name>, (securitylib::SET_PERMISSIONS_NAME_IS_DIR
    ///                                                 | securitylib::SET_PERMISSIONS_ALLOW_WEB_SERVER));
    ///
    ///     If caller is a common code of CS and host then web server user may or may not exist.
    ///     In this case if caller do not wish exception to be thrown then use combination of
    ///     (SET_PERMISSIONS_ALLOW_WEB_SERVER | SET_PERMISSIONS_IGNORE_WEBUSER_CHK)
    void setPermissions(std::string const& name, int setFlags = SET_PERMISSIONS_DEFAULT);

    /// defaultFilePermissions function has different semantics on Unix and Windows.
    /// On Windows defaultFilePermissions returns default file share mode.
    /// Note: Do not use this API if specific share mode is required.
    /// eg.
    ///     CreateFile(<file name>,
    ///                     GENERIC_ALL,
    ///                     securitylib::defaultFilePermissions(),
    ///                     securitylib::defaultSecurityAttributes(),
    ///                     CREATE_ALWAYS,
    ///                     FILE_ATTRIBUTE_NORMAL,
    ///                     NULL);
    ///
    ///     ACE_OS::open(<file name>,
    ///                     O_WRONLY|O_CREAT|O_TRUNC,
    ///                     securitylib::defaultFilePermissions(),
    ///                     securitylib::defaultSecurityAttributes() );
    ///    
    /// On Unix defaultFilePermissions returns default file permission mode "rw" to root and its group.
    /// eg.
    ///     open(<file name>, O_CREAT|O_WRONLY|O_TRUNC, securitylib::defaultFilePermissions());
    ///     creat(<file name>, securitylib::defaultFilePermissions());
    ///     ACE_OS::creat(<file name>, securitylib::defaultFilePermissions());
    /// Note: Do not use ACE_OS::creat on windows to create files as it does not given an option to
    ///       specify security attributes.
    int defaultFilePermissions();

    /// setPermissions function is the wrapper around the above setPermissions to indicate the failure if any exception is thrown
    ///                and the error message can be used to print by the caller if it fails
    bool setPermissions(const std::string &path, std::string &errstr);
} // namespace securitylib


#include "setpermissionsmajor.h"


#endif // SETPERMISSINS_H
