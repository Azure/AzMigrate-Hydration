///
/// \file incsessionid.cpp
///
/// \brief
///

#include <boost/interprocess/detail/atomic.hpp>

#include "incsessionid.h"

boost::uint32_t atomicIncSessionId(boost::uint32_t* id)
{
  return boost::interprocess::detail::atomic_inc32(id);
}

