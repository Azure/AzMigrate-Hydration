
#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <map>
#include "TestGetInitialSettings.h"

void InitializeGlobalVariables();
void cmdLineParser(int, char** );
void Test_ResyncProgress();

enum cmdargs{targetdrivename,
                    object,
                    method1,
                    method2};

static std::map<std::string,cmdargs> CmdArgsMap;

struct GBLS
{
std::string ipAddress;
std::string hostId;
};

#endif
