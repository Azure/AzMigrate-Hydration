
///
/// \file strutils.h
///
/// \brief
///

#ifndef STRUTILS_H
#define STRUTILS_H

#include <boost/algorithm/string.hpp>

// windows use case insentive compare
#define STARTS_WITH boost::algorithm::starts_with
#define IS_EQUAL boost::algorithm::equals
#define CONTAINS_STR boost::algorithm::contains


#endif // STRUTILS_H
