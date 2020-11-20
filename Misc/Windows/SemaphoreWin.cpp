#include "SemaphoreWin.h"
#include <assert.h>

CSemaphoreWin::CSemaphoreWin(int nb_tokens_to_create)
{
	m_SemaphoreHandle = CreateSemaphoreEx(NULL, 0, nb_tokens_to_create, NULL, NULL, SEMAPHORE_MODIFY_STATE | SEMAPHORE_ALL_ACCESS);// par defaut tous les tokens sont pris
	assert(m_SemaphoreHandle);
}

CSemaphoreWin::~CSemaphoreWin()
{
	const BOOL bOK = CloseHandle(m_SemaphoreHandle);
	assert(bOK);
}

bool CSemaphoreWin::TakeToken()
{
	const DWORD status = WaitForSingleObject(m_SemaphoreHandle, INFINITE);
	return status == WAIT_OBJECT_0;
}

bool CSemaphoreWin::ReleaseToken()
{
	const BOOL bOK = ReleaseSemaphore(m_SemaphoreHandle, 1, NULL);
	return !!bOK;
}