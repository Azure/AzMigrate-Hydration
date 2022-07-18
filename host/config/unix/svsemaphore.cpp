#include "svsemaphore.h"
#include <iostream>
#include <ace/OS.h>
#include "errno.h"
#include "portable.h"
#include "portablehelpers.h"


SVSemaphore::SVSemaphore(const std::string &name,
                           unsigned int count, // By default make this unlocked.
                           int max)
{
    m_name = name;
    m_semaphore = NULL;
    m_owner = 0;
    m_semaphore = ::sem_open(name.c_str(), O_CREAT, (S_IRWXU|S_IRWXG|S_IRWXO), 1);
    if(SEM_FAILED == m_semaphore)
    {
//        DebugPrintf(SV_LOG_ERROR,"semaphore (%s) creation  failed. %s\n", 
//        m_name.c_str(), Error::Msg().c_str());
		std::cerr << "Semaphore (" << m_name << ") creation  failed. errno: " << errno << std::endl;
    }
}

SVSemaphore::~SVSemaphore()
{
    if(SEM_FAILED != m_semaphore)
    {
        if(m_owner == ACE_OS::thr_self())
            release();
        CloseSVSemaphore();
    }
}

bool SVSemaphore::acquire()
{
    int rv = -1;

    if(SEM_FAILED == m_semaphore)
        return false;

    // If same thread calls acquire again
    // without releasing, we return success
	if(m_owner == ACE_OS::thr_self())
		return true;

    // P ( m_semaphore )
    rv = ::sem_wait(m_semaphore);
    if(0 == rv)
    {
		m_owner = ACE_OS::thr_self();
        return true;
    }

    return false;
}

bool SVSemaphore::tryacquire()
{
    int rv = -1;

    if(SEM_FAILED == m_semaphore)
        return false;

    // If same thread calls acquire again
    // without releasing, we return success
    if(m_owner == ACE_OS::thr_self())
        return true;

    // P ( m_semaphore )
    rv = ::sem_trywait(m_semaphore);
    if(0 == rv)
    {
        m_owner = ACE_OS::thr_self();
        return true;
    }

    return false;
}

bool SVSemaphore::release()
{
    if(SEM_FAILED == m_semaphore)
        return false;

    if(m_owner == ACE_OS::thr_self())
    {
        m_owner = 0;
        ::sem_post(m_semaphore);
        return true;
    }

    return false;
}
