
///
/// \file securityutilsmajor.h
///
/// \brief
///

#ifndef SECURITYUTILSMAJOR_H
#define SECURITYUTILSMAJOR_H

#include <string>
#include <vector>
#include <windows.h>

namespace securitylib {

    inline bool getNodeName(std::string& nodeName)
    {
        nodeName.clear();
        DWORD len = 256;
        std::vector<char> buffer(len);
        if (GetComputerName(&buffer[0], &len)) {        
            nodeName.assign(&buffer[0], len);
            return true;
        }
        return false;
    }

} // namespace securitylib


#endif // SECURITYUTILSMAJOR_H
