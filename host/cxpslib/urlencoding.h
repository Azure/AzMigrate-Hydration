
///
/// \file urlencoding.h
///
/// \brief
///

#ifndef URLENCODING_H
#define URLENCODING_H

#include <string>
#include <cstdio>
#include <sstream>
#include <iomanip>

#include "errorexception.h"

/// \brief encode special chars to url encoding
///
/// \return string with url encodeing
inline std::string urlEncode(std::string const& source) ///< data to be encoded
{
    static char g_toHex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    std::string target;
    
    char hexBuffer[4] = { '%', 0 };
    
    std::string::size_type size = source.size();
    for (std::string::size_type i = 0; i < size; ++i) {
        char ch = source[i];

        if (('0' <= ch && ch <= '9')
            || ('a' <= ch && ch <= 'z')
            || ('A' <= ch && ch <= 'Z')
            || '-' == ch
            || '_' == ch
            || '.' == ch
            || '!' == ch
            || '*' == ch
            || '(' == ch
            || ')' == ch
            ) {
            target += ch;
        } else {
            hexBuffer[1] = g_toHex[(short)((ch & 0xf0) >> 4)];
            hexBuffer[2] = g_toHex[(short)(ch & 0x0f)];
            target.append(hexBuffer, 3);
        }
    }
    return target;
}

/// \brief convert url encoded value to non-encode value
///
/// \return string with decoded data
inline std::string urlDecode(std::string const& source) ///< data to be decoded
{
    std::string target;

    std::string::size_type i = 0;
    while (i < source.size()) {
        switch(source[i]) {
            case '%':
                if (i + 3 <= source.size()) {
                    int value;
                    std::istringstream convertStream(source.substr(i + 1, 2));
                    if (convertStream >> std::hex >> value) {
                        target += static_cast<char>(value);
                        i += 2;
                    } else {
                        throw ERROR_EXCEPTION << "invalid url encoding";
                    }
                } else {
                    throw ERROR_EXCEPTION << "invalid url encoding";
                }
                break;
            default:
                target += source[i];
                break;
        }
        i++;
    }
    return target;
}


#endif // URLENCODING_H
