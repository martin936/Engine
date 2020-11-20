#include "DeviceManager.h"
#include "Engine/Threads/Thread.h"
#include "Engine/Misc/Event.h"
#include "CommandListManager.h"
#include "PipelineManager.h"
#include "RenderThreads.h"
#include "ResourceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Viewports/Viewports.h"


CSchedulerThread*									CRenderWorkerThread::ms_pSchedulerThread;

thread_local unsigned int							CRenderWorkerThread::ms_nWorkerThreadID = 0;
thread_local unsigned int							CRenderWorkerThread::ms_nCurrentCommandListID = 0;

std::vector<CRenderWorkerThread*>					CRenderWorkerThread::ms_pThreads;
volatile bool										CSchedulerThread::ms_bIsWorkerThreadAvailable[ms_nMaxThreadCount];
std::vector<CSchedulerThread::SRenderTask>			CSchedulerThread::ms_pTaskList;

CMutex*												CSchedulerThread::ms_pTaskLock;

std::vector<CEvent*>								gs_bStartWorkLoad;
std::vector<CEvent*>								gs_bWorkLoadDone;
CEvent*												gs_bStartScheduling;
CEvent*												gs_bHaltScheduling;


CRenderWorkerThread::CRenderWorkerThread(unsigned int threadID)
{
	m_nWorkerThreadID = threadID;
	char cName[256] = "";
	sprintf(cName, "RenderWorkerThread %d", threadID);

	m_bStop = false;
	m_CurrentTask.m_nCommandListID = 0;

	CThread *pThread = CThread::Create(1024 * 1024, cName);
	AttachToThread(pThread);
	pThread->Run();
}


CRenderWorkerThread::~CRenderWorkerThread()
{
	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


CSchedulerThread::CSchedulerThread()
{
	m_bStop = false;

	CThread *pThread = CThread::Create(1024 * 1024, "Render Workload Scheduler");
	AttachToThread(pThread);
	pThread->Run();
}


CSchedulerThread::~CSchedulerThread()
{
	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


void CSchedulerThread::Init(unsigned int NumWorkerThreads)
{
	ms_pTaskLock = CMutex::Create();

	ms_pTaskList.reserve(256);
	ms_pTaskList.clear();

	gs_bStartScheduling = CEvent::Create();
	gs_bHaltScheduling	= CEvent::Create();

	for (unsigned int i = 0; i < NumWorkerThreads; i++)
	{
		ms_bIsWorkerThreadAvailable[i] = true;
	}
}


void CRenderWorkerThread::Init(unsigned int NumWorkerThreads)
{
	NumWorkerThreads = clamp(NumWorkerThreads, 1, 32);

	gs_bStartWorkLoad.reserve(NumWorkerThreads);

	CCommandListManager::Init(NumWorkerThreads);
	CSchedulerThread::Init(NumWorkerThreads);

	gs_bWorkLoadDone.resize(NumWorkerThreads);
	CEvent::CreateMultipleEvents(NumWorkerThreads, gs_bWorkLoadDone.data());

	for (unsigned int i = 0; i < NumWorkerThreads; i++)
	{
		gs_bStartWorkLoad.push_back(CEvent::Create());
		gs_bWorkLoadDone[i]->Throw();

		CRenderWorkerThread* pRenderThread = new CRenderWorkerThread(i);
		ms_pThreads.push_back(pRenderThread);
	}

	ms_pSchedulerThread = new CSchedulerThread();
}


void CRenderWorkerThread::Terminate()
{
	unsigned int NumWorkerThreads = static_cast<unsigned int>(ms_pThreads.size());

	ms_pSchedulerThread->Stop();
	gs_bStartScheduling->Throw();
	gs_bHaltScheduling->Wait();

	for (unsigned int i = 0; i < NumWorkerThreads; i++)
	{
		gs_bWorkLoadDone[i]->Reset();
		ms_pThreads[i]->Stop();
		gs_bStartWorkLoad[i]->Throw();

		gs_bWorkLoadDone[i]->Wait();
	}

	for (unsigned int i = 0; i < NumWorkerThreads; i++)
	{
		delete gs_bStartWorkLoad[i];
		delete gs_bWorkLoadDone[i];

		delete ms_pThreads[i];
	}

	ms_pThreads.clear();

	delete ms_pSchedulerThread;

	CSchedulerThread::Terminate();
}


void CSchedulerThread::Terminate()
{
	delete ms_pTaskLock;
	delete gs_bStartScheduling;
	delete gs_bHaltScheduling;

	ms_pTaskList.clear();
}


void CSchedulerThread::AddRenderTask(unsigned int nCommandListID, std::vector<SRenderPassTask>& pRenderPasses, void* params)
{
	if (CFrameBlueprint::IsSorting())
	{
		std::vector<SRenderPassTask>::iterator it;

		for (it = pRenderPasses.begin(); it < pRenderPasses.end(); it++)
			CFrameBlueprint::SetNextRenderPass(*it);
	}

	else
	{
		ms_pTaskLock->Take();

		SRenderTask task;
		task.m_pRenderPasses = pRenderPasses;
		task.m_nCommandListID = nCommandListID;
		task.m_pParams = params;

		CCommandListManager::GetCommandList(nCommandListID)->m_pCompletedEvent->Reset();

		ms_pTaskList.push_back(task);

		ms_pTaskLock->Release();

		gs_bStartScheduling->Throw();
	}
}


void CSchedulerThread::AddRenderTask(unsigned int nCommandListID, SRenderPassTask pRenderPass, void* params)
{
	if (CFrameBlueprint::IsSorting())
	{
		CFrameBlueprint::SetNextRenderPass(pRenderPass);
	}

	else
	{
		ms_pTaskLock->Take();

		SRenderTask task;
		task.m_pRenderPasses.clear();
		task.m_pRenderPasses.push_back(pRenderPass);
		task.m_nCommandListID = nCommandListID;
		task.m_pParams = params;

		CCommandListManager::GetCommandList(nCommandListID)->m_pCompletedEvent->Reset();

		ms_pTaskList.push_back(task);

		ms_pTaskLock->Release();

		gs_bStartScheduling->Throw();
	}
}


void CRenderWorkerThread::Run()
{
	ms_nWorkerThreadID = m_nWorkerThreadID;
	m_nGlobalThreadID = CThread::GetCurrentThreadId();

	while (!m_bStop)
	{
		gs_bStartWorkLoad[m_nWorkerThreadID]->Wait();
		gs_bStartWorkLoad[m_nWorkerThreadID]->Reset();

		if (m_bStop)
		{
			gs_bWorkLoadDone[m_nWorkerThreadID]->Throw();
			break;
		}

		m_nCurrentCommandListID = m_CurrentTask.m_nCommandListID;
		ms_nCurrentCommandListID = m_nCurrentCommandListID;

		int nNumRenderPasses = static_cast<int>(m_CurrentTask.m_pRenderPasses.size());

		if (nNumRenderPasses > 0)
		{
			unsigned int index = 0;
			while (!(m_CurrentTask.m_pRenderPasses[0].m_nSubPassMask & (1 << index)))
				index++;

			unsigned int nPipelineStateID = m_CurrentTask.m_pRenderPasses[0].m_pRenderPass->GetPipeline(index);
			CCommandListManager::GetCommandList(m_nCurrentCommandListID)->m_nInitialPipelineStateID = nPipelineStateID;

			CPipelineManager::SetCurrentPipeline(INVALIDHANDLE);
			CDeviceManager::ResetViewport();
			CViewportManager::BindViewport(0);

			CCommandListManager::BeginRecording(m_nCurrentCommandListID, m_nWorkerThreadID);

			CRenderer::UpdateLocalMatrices();

			for (int i = 0; i < nNumRenderPasses; i++)
				m_CurrentTask.m_pRenderPasses[i].m_pRenderPass->Run(m_nCurrentCommandListID, m_CurrentTask.m_pParams, m_CurrentTask.m_pRenderPasses[i].m_nSubPassMask);

			CCommandListManager::EndRecording(m_nCurrentCommandListID);
		}

		CCommandListManager::GetCommandList(m_nCurrentCommandListID)->m_pCompletedEvent->Throw();

		CSchedulerThread::ms_bIsWorkerThreadAvailable[m_nWorkerThreadID] = true;

		gs_bWorkLoadDone[m_nWorkerThreadID]->Throw();
	}
}


void CSchedulerThread::Run()
{
	while (!m_bStop)
	{
		ms_pTaskLock->Take();

		if (ms_pTaskList.size() > 0)
		{
			SRenderTask task = ms_pTaskList[0];

			ms_pTaskList.erase(ms_pTaskList.begin());
			ms_pTaskLock->Release();

			unsigned int NumThreads = CRenderWorkerThread::GetNumThreads();
			unsigned int WorkerThreadID = 0;

			WorkerThreadID = CEvent::MultipleWait(static_cast<unsigned int>(gs_bWorkLoadDone.size()), gs_bWorkLoadDone.data());

			while (!ms_bIsWorkerThreadAvailable[WorkerThreadID]);

			CRenderWorkerThread::ms_pThreads[WorkerThreadID]->m_CurrentTask = task;

			ms_bIsWorkerThreadAvailable[WorkerThreadID] = false;

			gs_bWorkLoadDone[WorkerThreadID]->Reset();
			gs_bStartWorkLoad[WorkerThreadID]->Throw();
		}

		else
		{
			ms_pTaskLock->Release();
			gs_bStartScheduling->Wait();
			gs_bStartScheduling->Reset();
		}
	}

	gs_bHaltScheduling->Throw();
}

