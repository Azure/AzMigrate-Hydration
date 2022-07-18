//                                       
// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : Dbgbreak.h
// 
// Description: 
// 

#ifndef DBGBREAK_H
#define DBGBREAK_H

#define BREAK_INSTRUCTION __asm__("int3")

void DbgBreak(bool launchDebugger = false);

#endif // ifndef DBGBREAK_H
