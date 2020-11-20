#include "Semaphore.h"
#include "Assert.h"

#ifdef _WIN32
#include "Windows/SemaphoreWin.h"
#else
#error "-----NO PLATFORM DEFINED IN _SEMAPHORE"
#endif


CSemaphore *CSemaphore::Create(int max_nb_tokens, const char *p_pcName)
{
	CSemaphore *to_ret = 0;
#ifdef _WIN32
	to_ret = new CSemaphoreWin(max_nb_tokens);
#else
#error "-----NO PLATFORM DEFINED IN _SEMAPHORE"
	to_ret = nullptr
	ASSERT(0);
#endif

	return to_ret;
}

