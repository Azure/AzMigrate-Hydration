#include "svsemaphore.h"

//
//dummy implementation for powerpc-linux
//

SVSemaphore::SVSemaphore(const std::string &name,
						 unsigned int count, // By default make this unlocked.
						 int max)
{

}

SVSemaphore::~SVSemaphore()
{

}

bool SVSemaphore::acquire()
{
	return true;
}

bool SVSemaphore::tryacquire()
{
	return true;
}

bool SVSemaphore::release()
{
	return true;
}
