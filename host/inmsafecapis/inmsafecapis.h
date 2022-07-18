///
///  \file inmsafecapis.h
///
///  \brief interface header to be included by applications to use inm_*_s (for eg: inm_memcpy_s etc,.). ContextualException is thrown on error.
///

#ifndef INM_SAFE_C_APIS_H
#define INM_SAFE_C_APIS_H

#if !defined(ARRAYSIZE)
#define ARRAYSIZE( a ) ( sizeof( a ) / sizeof( a[ 0 ] ) )
#endif

#include "initinmsafecapismajor.h"

#include "inmsafecapismajor.h"
#include "inmsafecapiswrappers.h"

/* Define the macros to be used by applications. File name, line number, function name of caller are recorded in exception message */
#define inm_memcpy_s(dest, numberOfElements, src, count) inm_memcpy_s_wrapper(dest, numberOfElements, src, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_strcpy_s(strDestination, numberOfElements, strSource) inm_strcpy_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)
#define inm_strncpy_s(strDest, numberOfElements, strSource, count) inm_strncpy_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_strcat_s(strDestination, numberOfElements, strSource) inm_strcat_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)
#define inm_strncat_s(strDest, numberOfElements, strSource, count) inm_strncat_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_wcscpy_s(strDestination, numberOfElements, strSource) inm_wcscpy_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)
#define inm_wcscat_s(strDestination, numberOfElements, strSource) inm_wcscat_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)

#endif /* INM_SAFE_C_APIS_H */
