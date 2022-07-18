
///
/// \file setsockopt.h
///
/// \brief
///

#ifndef SETSOCKOPT_H
#define SETSOCKOPT_H

#include <winsock2.h>

#include <boost/asio.hpp>

#include "errorexception.h"

/// brief attempts to set socket timeouts for send anc receive
///
/// \return
/// \li \c true : platform supports socket timeouts and they were set 
/// \li \c false: platform does not support socket timeouts, need to use async request to get timeouts if needed
inline bool setSocketTimeoutOptions(SOCKET socket, long milliSeconds)
{
    if (0 != setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&milliSeconds, sizeof(milliSeconds))) {
        return false;
    }
    if (0 != setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&milliSeconds, sizeof(milliSeconds))) {
        return false;
    }

    return true;
}

inline void setSocketTimeoutForAsyncRequests(SOCKET socket, long milliSeconds)
{
    setSocketTimeoutOptions(socket, milliSeconds);
}

#endif // SETSOCKOPT_H
