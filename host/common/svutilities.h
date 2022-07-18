#ifndef SV_UTILITIESS__H
#define SV_UTILITIES__H
#include <iostream>
#include <string>
#include <vector>
typedef std::vector<std::string> svector_t;
svector_t split(const std::string arg,const std::string delim,const int numFields = 0);

#endif
