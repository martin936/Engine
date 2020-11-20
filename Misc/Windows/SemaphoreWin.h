#ifndef CACTUS_SEMAPHORE_PC_H
#define CACTUS_SEMAPHORE_PC_H

#include "../Semaphore.h"
#include <windows.h>
#include <winbase.h>

class CSemaphoreWin : public CSemaphore
{
protected:
	HANDLE m_SemaphoreHandle;
public:
	CSemaphoreWin(int numTokens);
	virtual ~CSemaphoreWin();

	virtual bool TakeToken();
	virtual bool ReleaseToken();
};

#endif
