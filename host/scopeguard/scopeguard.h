
///
/// \file scopeguard.h
///
/// \brief convenience macros to make using boost scope guard stuff easier since it is
///        not yet its own project but part of another project
///

#ifndef SCOPEGUARD_H
#define SCOPEGUARD_H

#include <boost/multi_index/detail/scope_guard.hpp>

// convenience macros so you do not have to remember that scope gaurd is actually in the
// details of another project
#define SCOPE_GUARD boost::multi_index::detail::scope_guard               ///< use to declare a scope_guard type
#define MAKE_SCOPE_GUARD boost::multi_index::detail::make_guard           ///< use to make a guard
#define MAKE_SCOPE_OBJ_GUARD boost::multi_index::detail::make_obj_guard   ///< use to make a object guard

// for creating anonymous names used by the ON_BLOCK macros
#define CONCATENATE_DIRECT(s1, s2) s1##s2
#define CONCATENATE(s1, s2) CONCATENATE_DIRECT(s1, s2)
#define ANONYMOUS_VARIABLE(str) CONCATENATE(str, __LINE__)

// convenience macros for creating anonymous guards that do not need to be dismissed
// e.g. to make sure the file descriptor fd is always closed no matter what use
// ON_BLOCK_EXIT(&close, fd)
/// \brief used to make an anonymous guard for cases where the guard does not need to be dismissed
#define ON_BLOCK_EXIT boost::multi_index::detail::scope_guard ANONYMOUS_VARIABLE(scopeGuard) = boost::multi_index::detail::make_guard
/// \brief use to make an anonymous object guard
#define ON_BLOCK_EXIT_OBJ boost::multi_index::detail::scope_guard ANONYMOUS_VARIABLE(scopeGuard) = boost::multi_index::detail::make_obj_guard


#endif // ifndef SCOPEGUARD_H
