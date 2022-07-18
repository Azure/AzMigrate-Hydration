
///
/// \file linux/converterrortostringminor.h
///
/// \brief
///

#ifndef CONVERTERRORTOSTRINGMINOR_H
#define CONVERTERRORTOSTRINGMINOR_H


#include <vector>
#include <string>

/// \brief converts error code to error string
inline void convertErrorToString(int err,             ///< error to convert
                                 std::string& errStr  ///< receives converted error
                                 )
{
    std::vector<char> buffer(1024);
    char* ch = ::strerror_r(err, &buffer[0], buffer.size());
    errStr = ch;
}



#endif // CONVERTERRORTOSTRINGMINOR_H
