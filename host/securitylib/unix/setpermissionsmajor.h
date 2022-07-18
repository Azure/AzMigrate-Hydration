
///
/// \file setpermissionsmajor.h
///
/// \brief
///

#ifndef SETPERMISSIONSMAJOR_H
#define SETPERMISSIONSMAJOR_H

namespace securitylib {

    char const * const ROOT_ADMIN_NAME = "root";
    
    inline bool isRootAdmin()
    {
        return (0 == getuid());
    }
    
    /// defaultDirectoryPermissions function returns default permission mode as "rwx" to root and its group.
    /// eg.
    ///        ACE_OS::creat(<directory name>, securitylib::defaultDirectoryPermissions());
    ///        ACE_OS::mkdir(<directory name>, securitylib::defaultDirectoryPermissions());
    ///        creat(<directory name>, securitylib::defaultDirectoryPermissions());
    ///        mkdir(<directory name>, securitylib::defaultDirectoryPermissions());
    int defaultDirectoryPermissions();
    
    /// defaultSecurityAttributes function on Unix is a place holder function for fourth
    /// parameter of ACE_OS::open function used in common platform code.
    inline int defaultSecurityAttributes(bool isNameDirectory = false, bool allowWebServer = false) { return 0; }
    
} // namespace securitylib


#endif // SETPERMISSIONSMAJOR_H
