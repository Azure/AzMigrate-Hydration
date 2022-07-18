
///
/// \file strutils.h
///
/// \brief
///

#ifndef STRUTILS_H
#define STRUTILS_H


#include <boost/algorithm/string.hpp>

// windows use case insentive compare
#define STARTS_WITH boost::algorithm::istarts_with
#define IS_EQUAL boost::algorithm::iequals
#define CONTAINS_STR boost::algorithm::icontains


#endif // STRUTILS_H
