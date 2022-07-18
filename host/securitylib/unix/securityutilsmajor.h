
///
/// \file securityutilsmajor.h
///
/// \brief
///

#ifndef SECURITYUTILSMAJOR_H
#define SECURITYUTILSMAJOR_H

#include <string>

#include <sys/utsname.h>

namespace securitylib {

    inline bool getNodeName(std::string& nodeName)
    {
        nodeName.clear();
        struct utsname utsName;
        if (0 == uname(&utsName)) {
            nodeName = utsName.nodename;
            return true;
        }
        return false;
    }

} // namespace securitylib


#endif // SECURITYUTILSMAJOR_H
