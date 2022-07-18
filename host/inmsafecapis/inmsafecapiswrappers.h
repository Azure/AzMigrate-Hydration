///
///  \file inmsafecapiswrappers.h
///
///  \brief implementation of c++ wrapper functions around safe c ones
///

#ifndef INM_SAFE_C_APIS_WRAPPERS_H
#define INM_SAFE_C_APIS_WRAPPERS_H

#include "inmageex.h"
#include "safecapisincludesmajor.h"

inline void inm_memcpy_s_wrapper(
   void *dest,
   size_t numberOfElements,
   const void *src,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = memcpy_s(dest, numberOfElements, src, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_strcpy_s_wrapper(
   char *strDestination,
   size_t numberOfElements,
   const char *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = strcpy_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

inline void inm_strncpy_s_wrapper(
   char *strDest,
   size_t numberOfElements,
   const char *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = strncpy_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_strcat_s_wrapper(
   char *strDestination,
   size_t numberOfElements,
   const char *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = strcat_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

inline void inm_strncat_s_wrapper(
   char *strDest,
   size_t numberOfElements,
   const char *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = strncat_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_wcscpy_s_wrapper(
   wchar_t *strDestination,
   size_t numberOfElements,
   const wchar_t *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = wcscpy_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

inline void inm_wcscat_s_wrapper(
   wchar_t *strDestination,
   size_t numberOfElements,
   const wchar_t *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = wcscat_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

#endif /* INM_SAFE_C_APIS_WRAPPERS_H */
