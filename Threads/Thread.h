#ifndef __THREAD_H__
#define __THREAD_H__

#ifdef _WIN32
#include <windows.h>
typedef DWORD			ThreadId;
#else
typedef void*			ThreadId;
#endif

class CThreadWorker
{
	friend class CThread;
protected:

	class CThread	*m_pThread;
	volatile bool 	m_bRunning;
	volatile bool 	m_bCompleted;

public:

	virtual			~CThreadWorker() {}

	void			AttachToThread(CThread *p_Thread);

	virtual void	Run() = 0;
	virtual void	Cancel() = 0;

	bool			IsComplete() 
	{ 
		return m_bCompleted; 
	}

	bool			IsRunning() 
	{ 
		return m_bRunning; 
	}

	CThread*		GetThread() 
	{ 
		return m_pThread; 
	}
};



class CThread
{
protected:

	unsigned int m_nByteStackSize;
	CThreadWorker *m_pThreadWorker;	//object working for the thread

	CThread(unsigned int p_nByteStackSize, char *p_pcName);
	virtual ~CThread();

	static  void StaticThreadEntryPoint(void *p_which_thread);
	virtual void EntryPoint() = 0;

public:

	enum ETHREADAFFINITY
	{
		e_Core_Default = 0,
		e_Core0 = 1 << 0,
		e_Core1 = 1 << 1,
		e_Core2 = 1 << 2,

		e_HardwareThread0 = 1 << 31,
		e_HardwareThread1 = 1 << 30,
	};

	static CThread *	Create(unsigned int p_nByteStackSize, const char *p_pcName = "Unnamed thread");
	static  void		Destroy(CThread *thread);
	virtual void		Run();
	virtual void		Stop();
	virtual void		Pause();
	virtual void		Resume();
	virtual void		WaitEndOfRun() {};
	virtual void		SetExitWithoutWaiting() {}		// a utiliser sur PS3 avant le run pour que le thread disparaisse apres l'exit (au lieu de rester zombie jusqu'a l'appel de WaitEndOfRun)
	virtual void		SetThreadAffinity(unsigned int p_nCoreAffinity);

	void SetWorker(CThreadWorker *p_Worker)
	{
		if (m_pThreadWorker && m_pThreadWorker != p_Worker)
		{
			m_pThreadWorker->m_pThread = 0;
		}
		m_pThreadWorker = p_Worker;
	}

	//Retrieve current calling thread ID
	static ThreadId		GetCurrentThreadId();
	static ThreadId		GetMainThreadId();
	static void			SetMainThreadId();
	static bool			IsThisThreadMainThread();

	static void			ThreadSleep(int p_nMilliseconds);
};
#endif
