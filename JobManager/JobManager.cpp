#include "Engine/Device/DeviceManager.h"
#include "JobManager.h"


CJobScheduler*		CJobManager::ms_pSchedulerThread	= nullptr;
CJobWorker*			CJobManager::ms_pWorkerThreads		= nullptr;

int					CJobManager::ms_nNumThreads			= 0;
thread_local int	CJobWorker::ms_ThreadIndex			= 0;


void CJobManager::Init(int numThreads)
{
	ASSERT(numThreads > 0);

	ms_nNumThreads = numThreads;

	ms_pWorkerThreads	= new CJobWorker[numThreads];
	ms_pSchedulerThread = new CJobScheduler;

	for (int i = 0; i < numThreads; i++)
		ms_pWorkerThreads[i].m_nThreadIndex = i;
}


void CJobManager::Terminate()
{
	ms_pSchedulerThread->Stop();

	for (int i = 0; i < ms_nNumThreads; i++)
		ms_pWorkerThreads[i].Stop();

	for (int i = 0; i < ms_nNumThreads; i++)
		ms_pWorkerThreads[i].WaitUntilJobFinished();

	ms_pSchedulerThread->m_pStartScheduling->Throw();
	ms_pSchedulerThread->m_pSchedulingFinished->Wait();

	delete[] ms_pWorkerThreads;
	delete	 ms_pSchedulerThread;
}


void CJobManager::RunJob(void (*pFunctionToRun)(void* pData), void* pData, EPriority priority)
{
	CJobScheduler::STask task;
	task.m_pFunctionToRun	= pFunctionToRun;
	task.m_pData			= pData;
	task.m_ePriority		= priority;

	ms_pSchedulerThread->AddTask(task);
}


CJobScheduler::CJobScheduler()
{
	m_bStop = false;

	m_pTaskLock				= CMutex::Create();
	m_pStartScheduling		= CEvent::Create();
	m_pSchedulingFinished	= CEvent::Create();

	CThread* pThread = CThread::Create(1024 * 1024, "Job Scheduler Thread");
	AttachToThread(pThread);
	pThread->Run();
}


CJobScheduler::~CJobScheduler()
{
	delete m_pTaskLock;
	delete m_pStartScheduling;
	delete m_pSchedulingFinished;

	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


void CJobScheduler::AddTask(STask& task)
{
	m_pTaskLock->Take();

	m_TaskList.push_back(task);

	m_pTaskLock->Release();

	m_pStartScheduling->Throw();
}


void CJobScheduler::Run()
{
	m_pSchedulingFinished->Reset();

	CEvent** pEvents = new CEvent * [CJobManager::ms_nNumThreads];
	
	for (int i = 0; i < CJobManager::ms_nNumThreads; i++)
		pEvents[i] = CJobManager::ms_pWorkerThreads[i].m_pFinishedEvent;

	while (!m_bStop)
	{
		m_pTaskLock->Take();

		if (m_TaskList.size() > 0)
		{
			STask task = m_TaskList[0];

			m_TaskList.erase(m_TaskList.begin());
			m_pTaskLock->Release();

			unsigned int WorkerThreadID = 0;

			WorkerThreadID = CEvent::MultipleWait(CJobManager::ms_nNumThreads, pEvents);

			CJobManager::ms_pWorkerThreads[WorkerThreadID].RunJob(task.m_pFunctionToRun, task.m_pData);
		}

		else
		{
			m_pTaskLock->Release();
			m_pStartScheduling->Wait();
			m_pStartScheduling->Reset();
		}
	}

	m_pSchedulingFinished->Throw();
}


CJobWorker::CJobWorker()
{
	m_bStop = false;

	m_pTaskFunction = nullptr;
	m_pFunctionData = nullptr;

	m_pStartEvent		= CEvent::Create();
	m_pFinishedEvent	= CEvent::Create();
	m_pFinishedEvent->Throw();

	m_eStatus			= e_Ready;
	m_nThreadIndex		= 0;

	CThread* pThread = CThread::Create(1024 * 1024, "Job Worker");
	AttachToThread(pThread);
	pThread->Run();
}


CJobWorker::~CJobWorker()
{
	delete m_pStartEvent;
	delete m_pFinishedEvent;

	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


void CJobWorker::RunJob(void (*pFunctionToRun)(void* pData), void* pData)
{
	m_pTaskFunction = pFunctionToRun;
	m_pFunctionData = pData;

	m_eStatus = e_Busy;

	m_pFinishedEvent->Reset();
	m_pStartEvent->Throw();
}


void CJobWorker::WaitUntilJobFinished()
{
	m_pFinishedEvent->Wait();
}


void CJobWorker::Run()
{
	ms_ThreadIndex = m_nThreadIndex;

	while (!m_bStop)
	{
		m_pStartEvent->Wait();

		m_pTaskFunction(m_pFunctionData);

		m_eStatus = e_Ready;

		m_pStartEvent->Reset();
		m_pFinishedEvent->Throw();
	}
}
