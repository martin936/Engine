#include "Mutex.h"
#include "Assert.h"

#ifdef _WIN32
#include "Windows/MutexWin.h"
#else
#error "No platform defined in  Mutex"
#endif


CMutex* CMutex::Create()
{
	CMutex* ret = 0;

#ifdef _WIN32
	ret = new CFastMutexWin;
#else
#error "No platform defined in  Mutex"
	ret = NULL;
	ASSERT(!"no platform defined in  Mutex");
#endif
	return ret;
}
