#ifndef __MUTEX_WIN_H__
#define __MUTEX_WIN_H__

#include "../Mutex.h"
#include <windows.h>
#include <winbase.h>

class CMutexWin :public CMutex
{
protected:
	HANDLE m_MutexHandle;
public:
	CMutexWin();
	~CMutexWin();
	virtual bool Take();
	virtual bool Release();
};


//-----------------------------------------------------------------------------
// Critical Section : version plus rapide du mutex sous Windows
//-----------------------------------------------------------------------------
class CFastMutexWin : public CMutex
{
protected:
	CRITICAL_SECTION m_CriticalSection;

public:
	CFastMutexWin();
	~CFastMutexWin();
	virtual bool Take();
	virtual bool Release();
	virtual bool TryTake();
};

#endif
