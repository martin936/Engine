#ifndef __RENDER_THREADS_H__
#define __RENDER_THREADS_H__

#include "Engine/Misc/Mutex.h"
#include "Engine/Threads/Thread.h"
#include "RenderPass.h"
#include <vector>

class CSchedulerThread : public CThreadWorker
{
	friend class CRenderWorkerThread;
public:

	static const unsigned int ms_nMinThreadCount = 4;
	static const unsigned int ms_nMaxThreadCount = 32;

	static void Init(unsigned int NumWorkerThreads);
	static void Terminate();

	CSchedulerThread();
	~CSchedulerThread();

	struct SRenderTask
	{
		std::vector<SRenderPassTask>	m_pRenderPasses;
		unsigned int					m_nCommandListID;
		void*							m_pParams;
	};

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {};

	static void AddRenderTask(unsigned int nCommandListID, std::vector<SRenderPassTask>& pRenderPasses, void* params = nullptr);
	static void AddRenderTask(unsigned int nCommandListID, SRenderPassTask pRenderPass, void* params = nullptr);

private:

	bool							m_bStop;

	volatile static bool			ms_bIsWorkerThreadAvailable[ms_nMaxThreadCount];
	static std::vector<SRenderTask>	ms_pTaskList;
	static CMutex*			ms_pTaskLock;
};


class CRenderWorkerThread : public CThreadWorker
{
	friend class CSchedulerThread;
public:

	static void Init(unsigned int nNumWorkerThreads);
	static void Terminate();

	CRenderWorkerThread(unsigned int threadID);
	~CRenderWorkerThread();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {};

	inline static unsigned int GetCurrentThreadCommandListID()
	{
		return ms_nCurrentCommandListID;
	}

	inline static unsigned int GetCurrentThreadWorkerID()
	{
		return ms_nWorkerThreadID;
	}

	static unsigned int GetNumThreads() { return static_cast<unsigned int>(ms_pThreads.size()); }

private:

	thread_local static unsigned int			ms_nWorkerThreadID;
	thread_local static unsigned int			ms_nCurrentCommandListID;

	unsigned int								m_nWorkerThreadID;
	ThreadId									m_nGlobalThreadID;
	unsigned int								m_nCurrentCommandListID;
	bool										m_bStop;
	CSchedulerThread::SRenderTask				m_CurrentTask;

	static CSchedulerThread*					ms_pSchedulerThread;

	static std::vector<CRenderWorkerThread*>	ms_pThreads;
};


#endif

