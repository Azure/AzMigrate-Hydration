///
///  \file errorexceptionmajor.h
///
///  \brief 
///
///

#ifndef ERROREXCEPTIONMAJOR_H
#define ERROREXCEPTIONMAJOR_H

#include <windows.h>

inline DWORD lastKnownError() { return GetLastError(); }

#endif
