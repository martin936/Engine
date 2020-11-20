#include "MutexWin.h"
#include "../Assert.h"

CMutexWin::CMutexWin()
{
	m_MutexHandle = CreateMutex(NULL, FALSE, NULL);
}

CMutexWin::~CMutexWin()
{
	if (m_MutexHandle)
		ReleaseMutex(m_MutexHandle);
}

bool CMutexWin::Take()
{
	return (WaitForSingleObject(m_MutexHandle, INFINITE) == WAIT_OBJECT_0);

}

bool CMutexWin::Release()
{
	return ReleaseMutex(m_MutexHandle) == TRUE;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
CFastMutexWin::CFastMutexWin()
{
	BOOL bReturnValue = InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 2000);
	ASSERT(bReturnValue || "Problem with InitializeCriticalSectionAndSpinCount" == nullptr);
}

CFastMutexWin ::~CFastMutexWin()
{
	DeleteCriticalSection(&m_CriticalSection);
}

bool CFastMutexWin::Take()
{
	EnterCriticalSection(&m_CriticalSection);
	return true;
}

bool CFastMutexWin::Release()
{
	LeaveCriticalSection(&m_CriticalSection);
	return true;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CFastMutexWin::TryTake()
{
	return TryEnterCriticalSection(&m_CriticalSection) == TRUE;
}