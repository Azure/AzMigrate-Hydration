
///
/// \file linux/setsockettimeoutsminor.h
///
/// \brief
///

#ifndef SETSOCKETTIMEOUTSMINOR_H
#define SETSOCKETTIMEOUTSMINOR_H

#include <sys/types.h>
#include <sys/socket.h>

#include <boost/asio.hpp>

#include "errorexception.h"

//#define  USE_SOCKET_TIMEOUTS_IF_AVAILABLE

/// brief attempts to set socket timeouts for send anc receive
///
/// \return
/// \li \c true : platform supports socket timeouts and they were set
/// \li \c false: platform does not support socket timeouts, need to use async request to get timeouts if needed
inline bool setSocketTimeoutOptions(int socket, long milliSeconds)
{
#ifdef USE_SOCKET_TIMEOUTS_IF_AVAILABLE
    struct timeval tv;
    tv.tv_sec = milliSeconds / 1000;
    tv.tv_usec = milliSeconds % 1000;

    if (0 != setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv))) {
        return false;
    }
    if (0 != setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv))) {
        return false;
    }
    return true;
#else
    return false;
#endif
}

inline void setSocketTimeoutForAsyncRequests(int socket, long milliSeconds)
{
    struct timeval tv;
    tv.tv_sec = milliSeconds / 1000;
    tv.tv_usec = milliSeconds % 1000;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (void*)&tv, sizeof(tv));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (void*)&tv, sizeof(tv));
}

#endif // SETSOCKETTIMEOUTSMINOR_H
