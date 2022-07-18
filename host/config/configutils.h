#ifndef CONFIG_UTILS_H
#define CONFIG_UTILS_H

#include <string>
#include <sstream>
#include "inm_md5.h"

template<class T>
 void FillMD5OfSerializedStructure(const T &t, unsigned char *hash)
{
    std::stringstream ss;
    ss << CxArgObj<T>(t);
    INM_MD5_CTX ctx;
    INM_MD5Init(&ctx);
    INM_MD5Update(&ctx, (unsigned char*)ss.str().c_str(), ss.str().size());
    INM_MD5Final(hash, &ctx);
}

#endif /* CONFIG_UTILS_H */
