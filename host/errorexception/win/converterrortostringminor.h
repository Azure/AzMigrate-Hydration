
///
/// \file linux/converterrortostringminor.h
///
/// \brief
///

#ifndef CONVERTERRORTOSTRINGMINOR_H
#define CONVERTERRORTOSTRINGMINOR_H

#include<windows.h>
#include <string>
#include <sstream>

/// \brief converts error code to error string
inline void convertErrorToString(unsigned long err,             ///< error to convert
                                 std::string& errStr  ///< receives converted error
                                 )
{
    char* buffer = 0;
    if (0 == FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           err,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR) &buffer,
                           0,
                           NULL)) {
        try {
            std::stringstream str;
            str << "failed to get message for error code: " << std::hex << err;
            errStr = str.str();
            return;
        } catch (...) {
            // no throw
        }
    } else {
        try {
            errStr = buffer;
        } catch (...) {
            // no throw
        }
    }
    if (0 != buffer) {
        LocalFree(buffer);
    }
}



#endif // CONVERTERRORTOSTRINGMINOR_H
