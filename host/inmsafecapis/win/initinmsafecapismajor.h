#ifndef INIT_INM_SAFE_C_APIS_MAJOR_H
#define INIT_INM_SAFE_C_APIS_MAJOR_H

#include <cstdlib>

inline void inm_invalid_param_handler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
    /* do nothing */
}

inline void init_inm_safe_c_apis(void)
{
    _invalid_parameter_handler oldHandler, newHandler;
    newHandler = inm_invalid_param_handler;
    oldHandler = _set_invalid_parameter_handler(newHandler);
}

#endif /* INIT_INM_SAFE_C_APIS_MAJOR_H */
