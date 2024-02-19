#ifndef __JOB_MANAGER_H__
#define __JOB_MANAGER_H__

#include <vector>
#include "Engine/Threads/Thread.h"
#include "Engine/Misc/Mutex.h"
#include "Engine/Misc/Event.h"


class CJobWorker : public CThreadWorker
{
	friend class CJobScheduler;
	friend class CJobManager;
public:

	CJobWorker();
	~CJobWorker();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {}

	enum EStatus
	{
		e_Ready,
		e_Busy
	};

	EStatus GetState() const
	{
		return m_eStatus;
	}

	void WaitUntilJobFinished();

	void RunJob(void (*pFunctionToRun)(void* pData), void* pData);

	static int GetThreadIndex()
	{
		return ms_ThreadIndex;
	}

private:

	bool				m_bStop;

	void				(*m_pTaskFunction)(void* pData);
	void*				m_pFunctionData;
	volatile EStatus	m_eStatus;
	int					m_nThreadIndex;

	static thread_local int ms_ThreadIndex;

	CEvent*				m_pStartEvent;
	CEvent*				m_pFinishedEvent;
};


class CJobManager
{
	friend class CJobScheduler;
public:

	static void Init(int numThreads);
	static void Terminate();

	enum EPriority
	{
		e_Low,
		e_Standard,
		e_High
	};

	static void RunJob(void (*pFunctionToRun)(void* pData), void* pData = nullptr, EPriority priority = e_Standard);

	static int GetNumThreads()
	{
		return ms_nNumThreads;
	}

private:
	
	static CJobScheduler*	ms_pSchedulerThread;	
	static CJobWorker*		ms_pWorkerThreads;

	static int				ms_nNumThreads;
};


class CJobScheduler : public CThreadWorker
{
	friend CJobManager;
public:

	CJobScheduler();
	~CJobScheduler();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {}

	struct STask
	{
		void					(*m_pFunctionToRun)(void* pData);
		void*					m_pData;
		CJobManager::EPriority	m_ePriority;
	};

	void AddTask(STask& task);

private:

	bool				m_bStop;

	std::vector<STask>	m_TaskList;

	CMutex*				m_pTaskLock;
	CEvent*				m_pStartScheduling;
	CEvent*				m_pSchedulingFinished;
};





#endif
