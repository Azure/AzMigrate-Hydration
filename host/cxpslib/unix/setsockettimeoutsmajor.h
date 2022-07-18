
///
/// \file setsockettimeoutsmajor.h
///
/// \brief
///

#ifndef SETSOCKETTIMEOUTSMAJOR_H
#define SETSOCKETTIMEOUTSMAJOR_H


#include <sys/types.h>
#include <sys/socket.h>

#include <boost/asio.hpp>

#include "errorexception.h"

/// brief attempts to set socket timeouts for send anc receive
///
/// \return
/// \li \c true : platform supports socket timeouts and they were set
/// \li \c false: platform does not support socket timeouts, need to use async request to get timeouts if needed
inline bool setSocketTimeoutOptions(int socket, long milliSeconds)
{
    // should only define USE_SOCKET_TIMEOUTS_IF_AVAILABLE on unix/linx platforms
    // where setting socket timeouts actually does work. some platforms may support
    // them being set but does not have any affect
#ifdef USE_SOCKET_TIMEOUTS_IF_AVAILABLE

#if defined(SO_RCVTIMEO) && defined(SO_SNDTIMEO)
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

#else // defined(SO_RCVTIMEO) && defined(SO_SNDTIMEO)

    // this platform does not support socket timeouts
    return false;

#endif

#else // USE_SOCKET_TIMEOUTS_IF_AVAILABLE

    // not using socket timeouts
    return false;

#endif
}

inline void setSocketTimeoutForAsyncRequests(int socket, long milliSeconds)
{
    setSocketTimeoutOptions(socket, milliSeconds);
}

#endif // SETSOCKETTIMEOUTSMAJOR_H
