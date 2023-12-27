#ifndef __COMMAND_LIST_MANAGER_H__
#define __COMMAND_LIST_MANAGER_H__


#include <vector>
#include "DeviceManager.h"
#include "Engine/Misc/Mutex.h"
#include "Engine/Misc/Event.h"
#include "Engine/Threads/Thread.h"

class CCommandListManager
{
public:

	static const unsigned int ms_nMaxCommandListID = 128;

	static void Init(int nNumWorkerThreads);
	static void Terminate();

	enum EQueueType
	{
		e_Queue_Direct = 0,
		e_Queue_AsyncCompute,
		e_Queue_Copy,
		e_Queue_Present,

		e_NumQueues
	};

	enum ECommandListType
	{
		e_Direct = 0,
		e_Bundle,
		e_Compute,
		e_Copy,
		e_VideoDecode,
		e_VideoProcess,
		e_Loading,

		e_NumTypes
	};

	enum ECommandListFlags
	{
		e_None = 0,
		e_OnlyOnce = 1,
		e_NoFrameBuffering = 2
	};

	enum ECommandListState
	{
		e_Invalid,
		e_Recording,
		e_Executable
	};

	struct SCommandList
	{
		ECommandListType				m_eType;
		unsigned int					m_eFlags;
		unsigned int					m_nInitialPipelineStateID;
		unsigned int					m_nWorkerThreadID;
		ThreadId						m_nGlobalThreadID;
		CMutex*							m_pLock;
		CEvent*							m_pCompletedEvent;

		void**							m_pCommandList[CDeviceManager::ms_FrameCount];
		ECommandListState*				m_eState[CDeviceManager::ms_FrameCount];

		const char* m_pcName; // For debug/stats (and useful in case of crash on PS5, in case of out-of-memory in command buffer). May be NULL.
		void* m_pExtraData; // Pointer to extra platform-specific data (this cannot be put in m_pCommandList!)

		ECommandListState				GetState();
		void							SetState(ECommandListState eState);

		SCommandList();
		~SCommandList();
	};

	struct SCommandListParams
	{
		ECommandListType m_eType;
		unsigned int m_nInitialPipelineStateID;
		unsigned int m_eFlags;
		size_t m_uSize;
		const char* m_pcName;

		SCommandListParams()
		{
			m_eType = e_Direct;
			m_nInitialPipelineStateID = 0xffffffff;
			m_eFlags = e_OnlyOnce;
			m_uSize = 0;
			m_pcName = nullptr;
		}
	};

	struct SExecutable
	{
		enum EType
		{
			e_CommandList,
			e_InsertComputeFence,
			e_WaitOnComputeFence,
			e_InsertRTXScratchFence
		};

		EType		m_Type;
		EQueueType	m_QueueType;

		union
		{
			unsigned int m_CmdListID;
			unsigned int m_FenceValue;
		};
	};

	// uSize of 0 means "pick the platform-specific default", for convenience in case we need to change that default value (or give the application an opportunity to change it globally)
	static unsigned int			CreateCommandList(ECommandListType eType, unsigned int nInitialPipelineStateID = 0xffffffff, unsigned int eFlags = e_OnlyOnce, size_t uSize = 0, const char* pcName = nullptr);
	static unsigned int			CreateCommandList(const SCommandListParams& p_pParams);

	static void*				GetLoadingCommandListAllocator();

	static void*				GetMainRenderingThreadCommandListAllocator(int frame)
	{
		return ms_pMainRenderingThreadCommandAllocator[frame];
	}

	static void*				BeginOneTimeCommandList();
	static void					EndOneTimeCommandList(void* pCmdBuffer);

	static void*				GetCommandListPtr(unsigned int nID);
	static SCommandList*		GetCommandList(unsigned int nID)
	{
		ASSERT(nID > 0 && nID <= ms_nNumCommandLists);
		return ms_pCommandLists[nID - 1];
	}

	static void					BeginFrame();

	inline static void*			GetCurrentThreadCommandListPtr()
	{
		return ms_pCurrentCommandList;
	}

	inline static unsigned int	GetCurrentThreadCommandListID()
	{
		return ms_nCurrentCommandListID;
	}

	static void					BeginRecording(unsigned int nID, unsigned int nWorkerThreadID);
	static void					EndRecording(unsigned int nID);

	static void					DelayCommandListCreation();
	static void					ResumeCommandListCreation();

	static void					ExecuteCommandLists(SExecutable* IDs, unsigned int numExecutables);

	static unsigned int			GetNumLoadingCommandLists();

	static void					ScheduleForNextKickoff(unsigned int cmdListID);
	static void					InsertFence(EQueueType queueType, unsigned int fenceValue);
	static void					WaitOnFence(EQueueType queueType, unsigned int fenceValue);

	static void					LaunchKickoff();

	static void					LaunchDeferredKickoffs();

	static void*				GetCommandQueuePtr(EQueueType eType)
	{
		return ms_pCommandQueue[eType];
	}

private:

	static thread_local void*						ms_pCurrentLoadingCommandAllocator;
	static thread_local void*						ms_pCurrentCommandList;
	static thread_local unsigned int				ms_nCurrentCommandListID;

	static unsigned int								ms_nNumLoadingThreads;

	static void*									CreateCommandAllocator(ECommandListType eType);
	static void										ResetCommandList(unsigned int nID);

	static int										ms_nNumWorkerThreads;

	static CMutex*									ms_pCommandListCreationLock;

	static unsigned int								ms_nNumCommandLists;
	static SCommandList*							ms_pCommandLists[ms_nMaxCommandListID];
	static void*									ms_pCommandAllocators[ECommandListType::e_NumTypes][CDeviceManager::ms_FrameCount][12];

	static void*									ms_pMainRenderingThreadCommandAllocator[CDeviceManager::ms_FrameCount];

	static void*									ms_pCommandQueue[EQueueType::e_NumQueues];

	static SExecutable								ms_DeferredKickoffs[50][100];
	static unsigned int								ms_nNumExecutablePerKickoff[50];
	static unsigned int								ms_nCurrentKickoffID;
};


#endif
