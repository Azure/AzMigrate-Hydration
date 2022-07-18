///
///  \file inmsafecapismajor.h
///
///  \brief interface header for windows specific inm_*_s functions. Included by default in inmsafecapis/inmsafecapis.h
///

#ifndef INM_SAFE_C_APIS_MAJOR_H
#define INM_SAFE_C_APIS_MAJOR_H

#include "inmsafecapiswrappersmajor.h"

#define inm_mbstowcs_s(pReturnValue, destWcstr, destSizeInWords, srcMbstr, count) inm_mbstowcs_s_wrapper(pReturnValue, destWcstr, destSizeInWords, srcMbstr, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_wcstombs_s(pReturnValue, destMbstr, destSizeInBytes, srcWcstr, count) inm_wcstombs_s_wrapper(pReturnValue, destMbstr, destSizeInBytes, srcWcstr, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_wcsncpy_s(strDest, numberOfElements, strSource, count) inm_wcsncpy_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_tcscpy_s(strDestination, numberOfElements, strSource) inm_tcscpy_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)
#define inm_tcsncpy_s(strDest, numberOfElements, strSource, count) inm_tcsncpy_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_wcsncat_s(strDest, numberOfElements, strSource, count) inm_wcsncat_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_tcscat_s(strDestination, numberOfElements, strSource) inm_tcscat_s_wrapper(strDestination, numberOfElements, strSource, __FILE__, __LINE__, __FUNCTION__)
#define inm_tcsncat_s(strDest, numberOfElements, strSource, count) inm_tcsncat_s_wrapper(strDest, numberOfElements, strSource, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_wmemcpy_s(dest, numberOfElements, src, count) inm_wmemcpy_s_wrapper(dest, numberOfElements, src, count, __FILE__, __LINE__, __FUNCTION__)
#define inm_sprintf_s(buf, bufsz, fmt, ...) inm_sprintf_s_wrapper(buf, bufsz, fmt, __VA_ARGS__)

#endif /* INM_SAFE_C_APIS_MAJOR_H */
