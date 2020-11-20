#include "../../Misc/Assert.h"
#include "../Thread.h"
#include "ThreadWin.h"


//-----------------------------------------------------------------------------
// Code pour renommer un thread
//-----------------------------------------------------------------------------
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)


static void SetThreadName(DWORD dwThreadID, char* threadName)
{
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = threadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try
	{
		RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------



DWORD  WINAPI  CThreadWin::WinStaticThreadEntryPoint(void *p_which_thread)
{
	CThreadWin *which = (CThreadWin *)p_which_thread;
	which->EntryPoint();
	return 0;
}

CThreadWin::CThreadWin(unsigned int p_nByteStackSize, char * p_pcName) :
	CThread(p_nByteStackSize, "unnamed thread")
{
	m_nByteStackSize = p_nByteStackSize;
	m_ThreadHandle = 0;

	strcpy(m_pcThreadName, "Cactus Thread");
	if (p_pcName != NULL)
	{
		ASSERT(strlen(p_pcName) < e_NameMaxSize);
		strcpy(m_pcThreadName, p_pcName);
	}
}

CThreadWin::~CThreadWin()
{
	Stop();
}


void CThreadWin::EntryPoint()
{
	SetThreadName(-1, m_pcThreadName);
	if (m_pThreadWorker)
	{
		m_pThreadWorker->Run();
	}
	ExitThread(0);
}


void CThreadWin::Run()
{
	if (m_ThreadHandle)
	{
		WaitEndOfRun();
		CloseHandle(m_ThreadHandle);
	}
	m_ThreadHandle = CreateThread(NULL, m_nByteStackSize, CThreadWin::WinStaticThreadEntryPoint, this, CREATE_SUSPENDED, &m_nThreadId);
	ASSERT(m_ThreadHandle != NULL);
	SetThreadPriority(m_ThreadHandle, THREAD_PRIORITY_NORMAL);
	ASSERT(m_ThreadHandle != NULL);

	ResumeThread(m_ThreadHandle);
}

void CThreadWin::Stop()
{
	CThread::Stop();
	if (m_ThreadHandle)
	{
		WaitEndOfRun();
		CloseHandle(m_ThreadHandle);
	}
	m_ThreadHandle = 0;
}

void CThreadWin::Pause()
{
	if (m_ThreadHandle)
		SuspendThread(m_ThreadHandle);
}

void CThreadWin::Resume()
{
	if (m_ThreadHandle)
		ResumeThread(m_ThreadHandle);
}

void CThreadWin::WaitEndOfRun()
{
	if (m_ThreadHandle)
		WaitForSingleObject(m_ThreadHandle, INFINITE);
}
