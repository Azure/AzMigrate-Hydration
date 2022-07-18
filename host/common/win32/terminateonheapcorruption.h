#ifndef INM_TERMINATE_ON_HEAP_CORRUPTION
#define INM_TERMINATE_ON_HEAP_CORRUPTION

#include <windows.h>

/*
If function succeeds then it returns true otherwise false.
Call GetLastError() to get extended error information. 

This call should only fail on non-supported platforms, 
And this call failure is not such critical error to exit the application. 
So A correct application can continue to run even if this call fails.
*/

inline bool TerminateOnHeapCorruption()
{
	return HeapSetInformation(
		NULL,
		HeapEnableTerminationOnCorruption,
		NULL,
		0);
}

#endif //~INM_TERMINATE_ON_HEAP_CORRUPTION