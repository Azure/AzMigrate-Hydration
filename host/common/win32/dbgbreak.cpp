#include "dbgbreak.h"

#pragma intrinsic(__debugbreak)

void DbgBreak(bool launchDebugger)
{
    __debugbreak();
}
