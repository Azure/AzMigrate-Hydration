///
/// \file defaultdirsmajor.h
///
/// \brief
///

#ifndef DEFAULTDIRSMAJOR_H
#define DEFAULTDIRSMAJOR_H

namespace securitylib {

    std::string const SECURITY_DIR_SEPARATOR("/");
    
    inline std::string securityTopDir()
    {
        return "/usr/local/InMage";
    }

    inline std::string getPSInstallPath(void)
    {
        /* empty for now */
        return std::string();
    }

} // namespace securitylib

#endif // DEFAULTDIRSMAJOR_H
