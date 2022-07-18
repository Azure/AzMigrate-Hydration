//
// Copyright (c) 2005 InMage
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use, disclosure,
// or reproduction is prohibited except as permitted by express
// written license aggreement with InMage.
//
// File:       compatibility.h
//
// Description: 
//

#ifndef COMPATABILITY__H
#define COMPATABILITY__H

#include "svtypes.h"
#define CURRENT_COMPATIBILITY_NO 9500000

inline SV_ULONG CurrentCompatibilityNum() { return CURRENT_COMPATIBILITY_NO ;}

#define REQUIRED_COMPATIBILITY_NO 400000

inline SV_ULONG RequiredCompatibilityNum() { return REQUIRED_COMPATIBILITY_NO ;}

#endif

