///
///  \file inmsafecapismajor.h
///
///  \brief interface header for unix specific inm_*_s functions. Included by default in inmsafecapis/inmsafecapis.h
///

#ifndef INM_SAFE_C_APIS_MAJOR_H
#define INM_SAFE_C_APIS_MAJOR_H

#include "inmsafecapiswrappersmajor.h"

typedef int errno_t;

#define inm_sprintf_s(buf, bufsz, fmt, arg...) inm_sprintf_s_wrapper(buf, bufsz, fmt, ## arg)

#define inm_vsnprintf_s(buf, bufsz, fmt, arg) inm_vsnprintf_s_wrapper(buf, bufsz, fmt, arg)

#endif /* INM_SAFE_C_APIS_MAJOR_H */
