#ifndef __THREAD_WINDOWS_H__
#define __THREAD_WINDOWS_H__

#include "../Thread.h"
#include <windows.h>
#include <winbase.h>

class CThreadWin : public CThread
{
	enum
	{
		e_NameMaxSize = 32,
	};

	friend class CThread;

	void*		m_StackAllocation;
	ThreadId 	m_nThreadId;
	HANDLE		m_ThreadHandle;
	char		m_pcThreadName[e_NameMaxSize];

protected:

	CThreadWin(unsigned int p_nByteStackSize, char * p_pcName);
	~CThreadWin();

	virtual void			Run();
	virtual void			Stop();
	virtual void			Pause();
	virtual void			Resume();
	virtual void			EntryPoint();
	static  DWORD WINAPI	WinStaticThreadEntryPoint(void *p_which_thread);
	virtual void			WaitEndOfRun();

public:
	bool m_bMustExit;
};

#endif
