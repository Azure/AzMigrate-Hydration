#ifndef INM_CERTUTIL_H
#define INM_CERTUTIL_H

#include <string>

#include "svtypes.h"

SV_ULONG RunUrlAccessCommand(const std::string& uri,
    const std::string& proxy);

#endif