#include "Threads.h"


CThread::CThread(void(*pEntryPoint)(void))
{
	m_pThread = NULL;
	m_pEntryPoint = pEntryPoint;

	m_bIsRunning = false;
}


void CThread::Run()
{
	m_bIsRunning = true;
	m_pThread = new std::thread(m_pEntryPoint);
}

void CThread::WaitForEnd()
{
	m_pThread->join();
	m_bIsRunning = false;

	delete m_pThread;
}