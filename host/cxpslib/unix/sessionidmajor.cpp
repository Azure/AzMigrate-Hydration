
///
/// \file sessionidmajor.cpp
///
/// \brief
///

#include <boost/version.hpp>

#include "sessionid.h"

#if (BOOST_VERSION == 104300)

#include <boost/interprocess/detail/atomic.hpp>

void SessionId::inc()
{
    boost::interprocess::detail::atomic_inc32(&m_sessionId);
}

#else


void SessionId::inc()
{
    ++m_sessionId;
}

#endif
