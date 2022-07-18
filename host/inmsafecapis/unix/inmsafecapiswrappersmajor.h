///
///  \file inmsafecapiswrappers.h
///
///  \brief implementation of c++ wrapper functions around safe c ones (Linux specific)
///

#ifndef INM_SAFE_C_APIS_WRAPPERS_MAJOR_H
#define INM_SAFE_C_APIS_WRAPPERS_MAJOR_H

#include "inmageex.h"
#include "safecapisincludesmajor.h"

#define inm_sprintf_s_wrapper(strDest, numberOfElements, fmt, args...)                                                                              \
({                                                                                                                                                  \
    errno_t __e = sprintf_s(strDest, numberOfElements, fmt, ## args);                                                                               \
    if (-1 == __e)                                                                                                                                  \
        throw ContextualException(__FILE__, __LINE__, __FUNCTION__)("inm_sprintf_s_wrapper")("error")(__e)("numberOfElements")(numberOfElements);   \
    __e;                                                                                                                                            \
})

#define inm_vsnprintf_s_wrapper(strDest, numberOfElements, fmt, args)                                                                              \
({                                                                                                                                                    \
    errno_t __e = vsnprintf_s(strDest, numberOfElements, fmt, args);                                                                               \
    if (-1 == __e)                                                                                                                                    \
        throw ContextualException(__FILE__, __LINE__, __FUNCTION__)("inm_vsnprintf_s_wrapper")("error")(__e)("numberOfElements")(numberOfElements);   \
    __e;                                                                                                                                              \
})

#endif /* INM_SAFE_C_APIS_WRAPPERS_MAJOR_H */
