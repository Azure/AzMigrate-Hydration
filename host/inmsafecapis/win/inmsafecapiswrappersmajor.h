///
///  \file inmsafecapiswrappers.h
///
///  \brief implementation of c++ wrapper functions around safe c ones (Windows specific)
///

#ifndef INM_SAFE_C_APIS_WRAPPERS_MAJOR_H
#define INM_SAFE_C_APIS_WRAPPERS_MAJOR_H

#include "inmageex.h"
#include "safecapisincludesmajor.h"
#include <stdarg.h>

inline int inm_sprintf_s_wrapper(char *strDest, size_t numberOfElements, const char *fmt, ...)                                                                                  
{      
    va_list args;
    errno_t __e = 0;
                
    va_start(args, fmt);
    __e = vsprintf_s(strDest, numberOfElements, fmt, args);
    va_end(args);
    if (-1 == __e) {
        throw ContextualException(__FILE__, __LINE__, __FUNCTION__)(__FUNCTION__)("error")(__e)("numberOfElements")(numberOfElements);
                                            }
    return __e;
}

inline void inm_mbstowcs_s_wrapper(
    size_t *pReturnValue,
    wchar_t *destWcstr,
    size_t destSizeInWords,
    const char *srcMbstr,
    size_t count,
    const char *file,
    int line,
    const char *function
)
{
    errno_t e = mbstowcs_s(pReturnValue, destWcstr, destSizeInWords, srcMbstr, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("destSizeInWords")(destSizeInWords)("count")(count);
}

inline void inm_wcstombs_s_wrapper(
    size_t *pReturnValue,
    char *destMbstr,
    size_t destSizeInBytes,
    const wchar_t *srcWcstr,
    size_t count,
    const char *file,
    int line,
    const char *function
)
{
    errno_t e = wcstombs_s(pReturnValue, destMbstr, destSizeInBytes, srcWcstr, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("destSizeInBytes")(destSizeInBytes)("count")(count);
}

inline void inm_wmemcpy_s_wrapper(
   wchar_t *dest,
   size_t numberOfElements,
   const wchar_t *src,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = wmemcpy_s(dest, numberOfElements, src, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_wcsncpy_s_wrapper(
   wchar_t *strDest,
   size_t numberOfElements,
   const wchar_t *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = wcsncpy_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_tcscpy_s_wrapper(
   TCHAR *strDestination,
   size_t numberOfElements,
   const TCHAR *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = _tcscpy_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

inline void inm_tcsncpy_s_wrapper(
   TCHAR *strDest,
   size_t numberOfElements,
   const TCHAR *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = _tcsncpy_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_wcsncat_s_wrapper(
   wchar_t *strDest,
   size_t numberOfElements,
   const wchar_t *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = wcsncat_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

inline void inm_tcscat_s_wrapper(
   TCHAR *strDestination,
   size_t numberOfElements,
   const TCHAR *strSource,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = _tcscat_s(strDestination, numberOfElements, strSource);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements);
}

inline void inm_tcsncat_s_wrapper(
   TCHAR *strDest,
   size_t numberOfElements,
   const TCHAR *strSource,
   size_t count,
   const char *file,
   int line,
   const char *function
)
{
    errno_t e = _tcsncat_s(strDest, numberOfElements, strSource, count);
    if (0 != e)
        throw ContextualException(file, line, function)(__FUNCTION__)("error")(e)("numberOfElements")(numberOfElements)("count")(count);
}

#endif /* INM_SAFE_C_APIS_WRAPPERS_MAJOR_H */
