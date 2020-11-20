#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "Thread.h"

#ifdef _WIN32
#include "Windows/ThreadWin.h"
#endif


CThread::CThread(unsigned int p_nByteStackSize, char *p_pcName)
{
	m_pThreadWorker = NULL;
}

CThread::~CThread()
{
}

CThread *	CThread::Create(unsigned int p_nByteStackSize, const char *p_pcName)
{
	CThread *to_ret = NULL;

#ifdef _WIN32
	to_ret = new CThreadWin(p_nByteStackSize, (char *)p_pcName);
#endif

	return to_ret;
}

void CThread::Destroy(CThread *p_Thread)
{
	//	assert(0);
	if (p_Thread)
		delete p_Thread;
	p_Thread = NULL;
}


void CThread::Run()
{
}

void CThread::Stop()
{
}

void CThread::Pause()
{
}

void CThread::Resume()
{
}

void CThread::SetThreadAffinity(unsigned int)
{
}

void CThread::StaticThreadEntryPoint(void *p_which_thread)
{
	CThread *which = (CThread *)p_which_thread;
	which->EntryPoint();
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////


void CThreadWorker::AttachToThread(CThread *p_Thread)
{
	m_pThread = p_Thread;
	p_Thread->SetWorker(this);
}

ThreadId  CThread::GetCurrentThreadId()
{
	return ::GetCurrentThreadId();
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
ThreadId gs_MainThreadId = CThread::GetCurrentThreadId();
ThreadId	CThread::GetMainThreadId()
{
	return gs_MainThreadId;
}

void CThread::SetMainThreadId()
{
	gs_MainThreadId = CThread::GetCurrentThreadId();
}

bool CThread::IsThisThreadMainThread()
{
	const ThreadId currentThreadId = GetCurrentThreadId();
	return currentThreadId == gs_MainThreadId;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void CThread::ThreadSleep(int p_nMilliseconds)
{
	Sleep(p_nMilliseconds);
}