///
///  \file errorexceptionmajor.h
///
///  \brief 
///
///


#include <errno.h>

inline unsigned long lastKnownError() { return errno; }
