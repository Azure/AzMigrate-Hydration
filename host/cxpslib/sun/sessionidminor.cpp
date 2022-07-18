
///
/// \file sessionidmajor.cpp
///
/// \brief
///

#include <pthread.h>

#include "sessionid.h"

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

void SessionId::inc()
{
    int status = pthread_mutex_lock(&g_mutex);
    ++m_sessionId;
    if (0 == status) {
        pthread_mutex_unlock(&g_mutex);
    }
}
