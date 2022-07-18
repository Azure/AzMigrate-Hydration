
///
/// \file win/sessinidmajor.h
///
/// \brief
///

#ifndef SESSINIDMAJOR_H
#define SESSINIDMAJOR_H

#include <boost/version.hpp>

#if (BOOST_VERSION == 104300)

#include <boost/cstdint.hpp>

typedef boost::uint32_t sessiondId_t; ///< type for session id when atomic is supported

#else

#include <boost/atomic.hpp>

typedef boost::atomic<unsigned int> sessiondId_t; ///< type for session id when atomic is supported

#endif


#endif // SESSINIDMAJOR_H
