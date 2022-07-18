#define ACE_HAS_STANDARD_CPP_LIBRARY 1
#define ACE_AS_STATIC 1

// TODO: refactor this to be nicer and handle all platforms
#if defined(WIN32)
#define ACE_HAS_WINNT4 1
//#define ACE_HAS_MFC 1

// NOTE: we use the windows value instead of ACE's default 1024 so as not
// to have issues with header file include order which can mess up
// struct/class sizes if the FD_SETSIZE doesn't match in all compilation units
#if !defined (FD_SETSIZE)
#define FD_SETSIZE 64
#elif (64 != FD_SETSIZE) 
// not perfect but will dectect if including windows files ahead of this one 
// and the windows size has changed
#error FD_SETSIZE mismatch
#endif

// PR#10815: Long Path support for windows
#define ACE_USES_WCHAR 1
#define SV_ACE_UNICODE_PATH_MAX 4096
#include "ace/config-win32.h"

#else

#define _LARGEFILE_SOURCE 1
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#if defined(__NetBSD__)
    
#include "ace/config-netbsd.h"

#elif defined(linux)

#include "ace/config-linux.h"

#elif defined(__hpux__)

#undef _APP32_64BIT_OFF_T // ski: hp-ux header files are brain dead

#include <sys/socket.h>

#ifndef __LP64__
#define _APP32_64BIT_OFF_T
#endif

#include "ace/config-hpux-11.00.h"

#elif defined(_AIX)

#include "ace/config-aix.h"

#else

#error ACE has not been built for this platform

#endif

#endif

    
