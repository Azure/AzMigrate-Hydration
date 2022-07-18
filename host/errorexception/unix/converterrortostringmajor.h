
///
/// \file unix/converterrortostringmajor.h
///
/// \brief unix implemenation 
///

#ifndef CONVERTERRORTOSTRINGMAJOR_H
#define CONVERTERRORTOSTRINGMAJOR_H

#include <cstring>
#include <string>
#include <sstream>

/// \brief converts error code to errror string
inline void convertErrorToString(unsigned long err,             ///< error to convert
                                 std::string& errStr  ///< receives converted error
                                 )
{
    char* str = ::strerror((int)err);
    if (0 == str) {
        std::ostringstream msg;
        msg << "Error message not found for: " << err;
        errStr = msg.str();
    } else {
        errStr = str;
    }       
}

#endif // CONVERTERRORTOSTRINGMAJOR_H
