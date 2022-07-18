///
/// \file incsessionid.h
///
/// \brief
///
#ifndef INCSESSIONID_H
#define INCSESSIONID_H

#include <boost/cstdint.hpp>

boost::uint32_t atomicIncSessionId(boost::uint32_t* id);

#endif // INCSESSIONID_H
