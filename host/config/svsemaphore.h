//                                       
// Copyright (c) 2005 InMage.
// This file contains proprietary and confidential information and
// remains the unpublished property of InMage. Use,
// disclosure, or reproduction is prohibited except as permitted
// by express written license agreement with InMage.
// 
// File       : svsemaphore.h
// 
// Description: 
// 

#ifndef SVSEMAPHORE__H
#define SVSEMAPHORE__H

#include <string>

#ifdef WIN32
#include <windows.h>
typedef HANDLE svsema_t;
#else
#include <semaphore.h>
typedef sem_t * svsema_t;
#endif

// Note:  1. CDPSemaphore object cannot be passed across threads
//           The object has to be used in the same thread where it
//           has been created.
//        2. This implementation is not a counting semaphore. 
//           It is a binary named semaphore.

class SVSemaphore
{
public:
  // = Initialization and termination.
  // Initialize the semaphore, with initial value of "count".
    SVSemaphore   (const std::string &name,
                  unsigned int count = 1, // By default make this unlocked.
                  int max = 1);

  /// Implicitly destroy the semaphore.
  ~SVSemaphore (void);


  /// Block the thread until the semaphore count becomes
  /// greater than 0, then decrement it.
  bool acquire (void);

  /**
   * Conditionally decrement the semaphore if count is greater than 0
   * (i.e., won't block).  Returns -1 on failure.  If we "failed"
   * because someone else already had the lock, <errno> is set to
   * <EBUSY>.
   */
  bool tryacquire (void);

  /// Increment the semaphore by 1, potentially unblocking a waiting
  /// thread.
  bool release (void);

protected:
    svsema_t m_semaphore;
    std::string m_name;
	unsigned long m_owner;
	

private:
  // = Prevent assignment and initialization.
  void operator= (const SVSemaphore &);
  SVSemaphore (const SVSemaphore &);
  int CloseSVSemaphore(void);
};


#endif
