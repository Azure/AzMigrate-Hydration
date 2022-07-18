///
/// \file incsessionid.cpp
///
/// \brief
///

#include <pthread.h>

#include "incsessionid.h"

pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

boost::uint32_t atomicIncSessionId(boost::uint32_t* id)
{
  int status = pthread_mutex_lock(&g_mutex);
  ++(*id);
  if (0 == status) {
    pthread_mutex_unlock(&g_mutex);
  }
  return *id;
}

