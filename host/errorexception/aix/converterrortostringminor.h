
///
/// \file aix/converterrortostringminor.h
///
/// \brief aix implementation
///

#ifndef CONVERTERRORTOSTRINGMINOR_H
#define CONVERTERRORTOSTRINGMINOR_H

#include <sstream>
#include <cstring>
#include <vector>

/// \brief converts error code to error string
inline void convertErrorToString(int err,             ///< error to convert
                                 std::string& errStr  ///< receives converted error
                                 )
{
    std::vector<char> buffer(1024);
    if (0 != ::strerror_r(err, &buffer[0], buffer.size())) {
        std::ostringstream msg;
        msg << "Error message not found for: " << err;
        errStr = msg.str();
    } else {
        errStr = &buffer[0];
    }
}

#endif // CONVERTERRORTOSTRINGMINOR_H
