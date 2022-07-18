//                                       
// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : debugbreak.h
// 
// Description: 
// 

#ifndef DEBUGBREAK_H
#define DEBUGBREAK_H

// NOTE: the launchDebbugger parameter has not affect on windows
// but is needed for portablility. windows will either automatically launch
// the debugger or prompt the user depending on how the user's envrinment
// is setup
void DbgBreak(bool launchDebugger = false);

#endif // ifndef DEBUGBREAK_H

