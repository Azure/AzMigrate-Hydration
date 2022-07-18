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

/* 
 *
 * until we find a break instrution for 
 * solaris sparc and solaris x86 
 * No application should call dbgbreak 
 * 
 */

#define BREAK_INSTRUCTION 0

void DbgBreak(bool launchDebugger = false);

#endif // ifndef DBGBREAK_H
