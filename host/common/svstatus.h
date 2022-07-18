//
// @file svstatus.h
// Defines error codes used InMage Systems. 
// Included in different ways to define error codes & error code functions.
//
// SVSTATUS_LIST is       a list of char* literals if SVSTATUS_GET_STRINGS is defined.
//                        a list of enum constants otherwise. Creates enum SVSTATUS.
//
#ifndef SVSTATUS__H
#define SVSTATUS__H

#undef SVSTATUS_ENUM
#ifdef SVSTATUS_GET_STRINGS
#define SVSTATUS_ENUM(x) #x
#else
#define SVSTATUS_ENUM(x) x
#endif

/* N.B. SVS_* denotes successful returns
 *      SVE_* denotes failures 
 */
#define SVSTATUS_LIST \
    SVSTATUS_ENUM( SVS_OK ), \
    SVSTATUS_ENUM( SVS_FALSE ), \
    SVSTATUS_ENUM( SVE_INVALIDARG ), \
    SVSTATUS_ENUM( SVE_OUTOFMEMORY ), \
    SVSTATUS_ENUM( SVE_FAIL ), \
    SVSTATUS_ENUM( SVE_INVALID_FORMAT ), \
    SVSTATUS_ENUM( SVE_NOTIMPL ), \
    SVSTATUS_ENUM( SVE_DISK_CAPACITY_EXCEEDED ), \
    SVSTATUS_ENUM( SVE_ABORT ), \
    SVSTATUS_ENUM( SVE_FILE_NOT_FOUND ), \
    SVSTATUS_ENUM( SVE_HTTP_RESPONSE_FAILED ), \
    SVSTATUS_ENUM( SVE_FILE_OPEN_FAILED )

#ifndef SVSTATUS_GET_STRINGS
enum SVSTATUS { SVSTATUS_LIST };
#endif

#endif // SVSTATUS__H
