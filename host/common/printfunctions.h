#ifndef PRINT__FUNS__H_
#define PRINT__FUNS__H_

#include <map>
#include <sstream>
#include <string>
#include <iterator>
#include "logger.h"
#include "portable.h"

template <typename K, typename V,typename C>
 void InmPrint(const std::map<K, V, C> &m)
{
    for (typename std::map<K, V, C>::const_iterator i = m.begin(); i != m.end(); i++)
    {
        std::stringstream s;
        s << "key : " << i->first << ", value: " << i->second;
        DebugPrintf(SV_LOG_DEBUG, "%s\n", s.str().c_str());
    }
}

#endif /* PRINT__FUNS__H_ */
