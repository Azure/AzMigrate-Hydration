#include "svsemaphore.h"
#include <iostream>
#include <ace/OS.h>
#include "errno.h"
#include "logger.h"
#include "portable.h"
#include "portablehelpers.h"



int SVSemaphore::CloseSVSemaphore(void)
{
    return ::sem_close(m_semaphore);
}
