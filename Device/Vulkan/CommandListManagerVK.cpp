#include "../DeviceManager.h"
#include "../CommandListManager.h"
#include "../PipelineManager.h"
#include "../RenderThreads.h"



CCommandListManager::SCommandList*				CCommandListManager::ms_pCommandLists[CCommandListManager::ms_nMaxCommandListID];
void*											CCommandListManager::ms_pCommandAllocators[CCommandListManager::e_NumTypes][CDeviceManager::ms_FrameCount][12];
void*											CCommandListManager::ms_pMainRenderingThreadCommandAllocator[CDeviceManager::ms_FrameCount];
void*											CCommandListManager::ms_pCommandQueue[CCommandListManager::EQueueType::e_NumQueues];

int												CCommandListManager::ms_nNumWorkerThreads = 8;
unsigned int									CCommandListManager::ms_nNumLoadingThreads = 0;
unsigned int									CCommandListManager::ms_nNumCommandLists = 0;

thread_local void*								CCommandListManager::ms_pCurrentCommandList = nullptr;
thread_local void*								CCommandListManager::ms_pCurrentLoadingCommandAllocator = nullptr;
thread_local unsigned int						CCommandListManager::ms_nCurrentCommandListID = 0;
CCommandListManager::SExecutable				CCommandListManager::ms_DeferredKickoffs[50][100];
unsigned int 									CCommandListManager::ms_nNumExecutablePerKickoff[50];
unsigned int 									CCommandListManager::ms_nCurrentKickoffID = 0;

CMutex*											CCommandListManager::ms_pCommandListCreationLock = nullptr;

VkSemaphore										gs_AsyncComputeFence[CDeviceManager::ms_FrameCount] = { nullptr };

namespace
{
	// Per-queue sync attachments accumulated by callers during the frame and
	// folded into the actual vkQueueSubmits by ExecuteCommandLists. See the
	// header comment on AttachQueueWait/Signal/Fence for the contract.
	struct SPendingQueueSync
	{
		std::vector<VkSemaphore>           m_WaitSemaphores;
		std::vector<VkPipelineStageFlags>  m_WaitStages;
		std::vector<VkSemaphore>           m_SignalSemaphores;
		VkFence                            m_Fence = VK_NULL_HANDLE;
	};

	SPendingQueueSync gs_PendingSync[CCommandListManager::EQueueType::e_NumQueues];
}


void CCommandListManager::AttachQueueWait(EQueueType queueType, VkSemaphore sem, VkPipelineStageFlags waitStage)
{
	gs_PendingSync[queueType].m_WaitSemaphores.push_back(sem);
	gs_PendingSync[queueType].m_WaitStages   .push_back(waitStage);
}


void CCommandListManager::AttachQueueSignal(EQueueType queueType, VkSemaphore sem)
{
	gs_PendingSync[queueType].m_SignalSemaphores.push_back(sem);
}


void CCommandListManager::AttachQueueFence(EQueueType queueType, VkFence fence)
{
	// One fence per queue per frame — overwriting silently is almost certainly
	// a caller bug (the previous fence would never be signaled).
	ASSERT(gs_PendingSync[queueType].m_Fence == VK_NULL_HANDLE);
	gs_PendingSync[queueType].m_Fence = fence;
}


void CCommandListManager::FlushPendingSync(EQueueType queueType)
{
	SPendingQueueSync& p = gs_PendingSync[queueType];
	if (p.m_WaitSemaphores.empty() && p.m_SignalSemaphores.empty() && p.m_Fence == VK_NULL_HANDLE)
		return;

	// Drain any remaining attachments via an empty submit. Reached only when
	// no command-list batch on this queue ran this frame to consume them.
	VkSubmitInfo info{};
	info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.waitSemaphoreCount   = static_cast<uint32_t>(p.m_WaitSemaphores.size());
	info.pWaitSemaphores      = p.m_WaitSemaphores.empty() ? nullptr : p.m_WaitSemaphores.data();
	info.pWaitDstStageMask    = p.m_WaitStages.empty()     ? nullptr : p.m_WaitStages.data();
	info.signalSemaphoreCount = static_cast<uint32_t>(p.m_SignalSemaphores.size());
	info.pSignalSemaphores    = p.m_SignalSemaphores.empty() ? nullptr : p.m_SignalSemaphores.data();

	VkResult res = vkQueueSubmit((VkQueue)ms_pCommandQueue[queueType], 1, &info, p.m_Fence);
	ASSERT(res == VK_SUCCESS);

	p.m_WaitSemaphores.clear();
	p.m_WaitStages.clear();
	p.m_SignalSemaphores.clear();
	p.m_Fence = VK_NULL_HANDLE;
}


void CCommandListManager::Init(int nNumWorkerThreads)
{
	for (int type = 0; type < e_NumTypes; type++)
	{
		int numThreads = type == e_Loading ? 12 : nNumWorkerThreads;

		UINT numFrames = 1;

		if (type == e_Direct || type == e_Compute)
			numFrames = CDeviceManager::ms_FrameCount;

		for (UINT frame = 0; frame < numFrames; frame++)
			for (int threadID = 0; threadID < numThreads; threadID++)
			{
				ms_pCommandAllocators[type][frame][threadID] = CreateCommandAllocator((ECommandListType)type);
			}
	}

	for (unsigned i = 0u; i < ms_nMaxCommandListID; i++)
		ms_pCommandLists[i] = nullptr;

	for (unsigned int i = 0; i < 50; i++)
		ms_nNumExecutablePerKickoff[i] = 0;

	for (UINT frame = 0; frame < CDeviceManager::ms_FrameCount; frame++)
		ms_pMainRenderingThreadCommandAllocator[frame] = CreateCommandAllocator(e_Direct);

	ms_nCurrentKickoffID = 0;

	ms_pCommandListCreationLock = CMutex::Create();

	ms_nNumWorkerThreads = nNumWorkerThreads;

	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_Direct,		(VkQueue*)&ms_pCommandQueue[e_Queue_Direct]);
	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_Copy,			(VkQueue*)&ms_pCommandQueue[e_Queue_Copy]);
	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_AsyncCompute,	(VkQueue*)&ms_pCommandQueue[e_Queue_AsyncCompute]);

	for (int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		VkSemaphoreCreateInfo semaInfo = {};
		semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaInfo.flags = 0;
		semaInfo.pNext = nullptr;

		VkResult hr = vkCreateSemaphore(CDeviceManager::GetDevice(), &semaInfo, nullptr, &gs_AsyncComputeFence[i]);
		ASSERT(hr == VK_SUCCESS);
	}
}


void CCommandListManager::Terminate()
{
	for (unsigned int i = 0; i < ms_nMaxCommandListID; i++)
	{
		if (ms_pCommandLists[i] != nullptr)
			delete ms_pCommandLists[i];

		ms_pCommandLists[i] = nullptr;
	}

	delete ms_pCommandListCreationLock;

	for (int type = 0; type < e_NumTypes; type++)
	{
		for (int frame = 0; frame < CDeviceManager::ms_FrameCount; frame++)
		{
			for (int i = 0; i < 12; i++)
			{
				if (ms_pCommandAllocators[type][frame][i] != nullptr)
					vkDestroyCommandPool(CDeviceManager::GetDevice(), (VkCommandPool)(ms_pCommandAllocators[type][frame][i]), nullptr);

				ms_pCommandAllocators[type][frame][i] = nullptr;
			}
		}
	}

	for (UINT frame = 0; frame < CDeviceManager::ms_FrameCount; frame++)
		vkDestroyCommandPool(CDeviceManager::GetDevice(), (VkCommandPool)(ms_pMainRenderingThreadCommandAllocator[frame]), nullptr);

	for (int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		if (gs_AsyncComputeFence[i] != VK_NULL_HANDLE)
		{
			vkDestroySemaphore(CDeviceManager::GetDevice(), gs_AsyncComputeFence[i], nullptr);
			gs_AsyncComputeFence[i] = VK_NULL_HANDLE;
		}
	}
}


void* CCommandListManager::CreateCommandAllocator(ECommandListType eType)
{
	VkCommandPool pCommandPool = nullptr;

	VkCommandPoolCreateInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	desc.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	desc.queueFamilyIndex = CDeviceManager::GetGraphicsQueueFamilyIndex();

	int res = vkCreateCommandPool(CDeviceManager::GetDevice(), &desc, nullptr, &pCommandPool);
	ASSERT(res == VK_SUCCESS);

	return pCommandPool;
}



void CCommandListManager::DelayCommandListCreation()
{
	ms_pCommandListCreationLock->Take();
}


void CCommandListManager::ResumeCommandListCreation()
{
	ms_pCommandListCreationLock->Release();
}



void* CCommandListManager::GetLoadingCommandListAllocator()
{
	if (ms_pCurrentLoadingCommandAllocator == nullptr)
	{
		ms_nNumLoadingThreads++;
		ms_pCurrentLoadingCommandAllocator = ms_pCommandAllocators[e_Loading][0][ms_nNumLoadingThreads];
	}

	return ms_pCurrentLoadingCommandAllocator;
}


void*	CCommandListManager::BeginOneTimeCommandList()
{

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = reinterpret_cast<VkCommandPool>(CCommandListManager::GetLoadingCommandListAllocator());
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VkResult res = vkAllocateCommandBuffers(CDeviceManager::GetDevice(), &allocInfo, &commandBuffer);
	ASSERT(res == VK_SUCCESS);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}


void	CCommandListManager::EndOneTimeCommandList(void* pCmdBuffer)
{
	vkEndCommandBuffer((VkCommandBuffer)pCmdBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = (VkCommandBuffer*)&pCmdBuffer;

	vkQueueSubmit(reinterpret_cast<VkQueue>(CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct)), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(reinterpret_cast<VkQueue>(CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct)));

	vkFreeCommandBuffers(CDeviceManager::GetDevice(), reinterpret_cast<VkCommandPool>(CCommandListManager::GetLoadingCommandListAllocator()), 1, (VkCommandBuffer*)&pCmdBuffer);
}



unsigned int CCommandListManager::CreateCommandList(const SCommandListParams& p)
{
	return CreateCommandList(p.m_eType, p.m_nInitialPipelineStateID, p.m_eFlags, p.m_uSize, p.m_pcName);
}

unsigned int CCommandListManager::CreateCommandList(ECommandListType eType, unsigned int nInitialPipelineStateID, unsigned int eFlags, size_t uSize, const char* pcName)
{
	// It's not currently used, but if we ever start using it, know that it can be 0, meaning "default".
	if (uSize == 0)
		uSize = 1024 * 1024;

	ms_pCommandListCreationLock->Take();

	SCommandList* list = new SCommandList;

	list->m_eType = eType;
	list->m_eFlags = eFlags;
	list->m_nInitialPipelineStateID = nInitialPipelineStateID;
	list->m_nWorkerThreadID = 0;
	list->m_nGlobalThreadID = 0;
	list->m_pCompletedEvent = CEvent::Create();
	list->m_pCompletedEvent->Throw();
	list->m_pLock = CMutex::Create();
	list->m_pcName = pcName;
	list->m_pExtraData = nullptr;

	UINT numFrames = (eType == e_Direct && !(list->m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::ms_FrameCount : 1;

	for (UINT frame = 0; frame < numFrames; frame++)
	{
		list->m_pCommandList[frame] = new void* [ms_nNumWorkerThreads];
		list->m_eState[frame]		= new ECommandListState[ms_nNumWorkerThreads];

		for (int threadID = 0; threadID < ms_nNumWorkerThreads; threadID++)
		{
			VkCommandBuffer pCommandList = nullptr;

			VkCommandBufferAllocateInfo desc = {};
			desc.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			desc.commandPool		= (VkCommandPool)ms_pCommandAllocators[eType][frame][threadID];
			desc.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			desc.commandBufferCount = 1;

			int ret = vkAllocateCommandBuffers(CDeviceManager::GetDevice(), &desc, &pCommandList);
			ASSERT(ret == VK_SUCCESS);

			list->m_pCommandList[frame][threadID]	= pCommandList;
			list->m_eState[frame][threadID]			= e_Invalid;
		}
	}

	unsigned int nID = ms_nNumCommandLists;

	ms_pCommandLists[nID] = list;

	ms_nNumCommandLists++;

	ms_pCommandListCreationLock->Release();

	return nID + 1;
}


CCommandListManager::SCommandList::SCommandList()
{
	for (unsigned int i = 0; i < CDeviceManager::ms_FrameCount; i++)
		m_pCommandList[i] = nullptr;
}


CCommandListManager::SCommandList::~SCommandList()
{
	for (unsigned int i = 0u; i < CDeviceManager::ms_FrameCount; i++)
	{
		if (m_pCommandList[i] != nullptr)
		{
			/*for (unsigned int j = 0u; j < m_nNumWorkerThreads; j++)
			{
				if (m_pCommandList[i][j] != nullptr)
					((ID3D12CommandList*)(m_pCommandList[i][j]))->Release();

				m_pCommandList[i][j] = nullptr;
			}*/

			delete[] m_pCommandList[i];
			delete[] m_eState[i];

			m_pCommandList[i] = nullptr;
			m_eState[i] = nullptr;
		}
	}

	delete m_pCompletedEvent;
}


void* CCommandListManager::GetCommandListPtr(unsigned int nID)
{
	ASSERT(nID > 0 && nID <= ms_nNumCommandLists);

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	UINT frameIndex = (pCommandList->m_eType == e_Direct && !(pCommandList->m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::GetFrameIndex() : 0;

	UINT threadID = pCommandList->m_eType == e_Loading ? 0 : pCommandList->m_nWorkerThreadID;

	return pCommandList->m_pCommandList[frameIndex][threadID];
}


CCommandListManager::ECommandListState CCommandListManager::SCommandList::GetState()
{
	UINT frameIndex = (m_eType == e_Direct && !(m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::GetFrameIndex() : 0;
	UINT threadID = m_eType == e_Loading ? 0 : m_nWorkerThreadID;

	return m_eState[frameIndex][threadID];
}


void CCommandListManager::SCommandList::SetState(ECommandListState eState)
{
	UINT frameIndex = (m_eType == e_Direct && !(m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::GetFrameIndex() : 0;
	UINT threadID = m_eType == e_Loading ? 0 : m_nWorkerThreadID;

	m_eState[frameIndex][threadID] = eState;
}



void CCommandListManager::BeginFrame()
{
	for (int threadID = 0; threadID < ms_nNumWorkerThreads; threadID++)
	{
		VkResult res = vkResetCommandPool(CDeviceManager::GetDevice(), (VkCommandPool)ms_pCommandAllocators[e_Direct][CDeviceManager::GetFrameIndex()][threadID], 0);
		ASSERT(res == VK_SUCCESS);
	}
}



unsigned int CCommandListManager::GetNumLoadingCommandLists()
{
	return 0;
}



void CCommandListManager::BeginRecording(unsigned int nID, unsigned int nWorkerThreadID)
{
	ASSERT(nID > 0 && nID <= ms_nNumCommandLists);

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	pCommandList->m_pLock->Take();

	if (pCommandList->m_eType != e_Loading)
		pCommandList->m_nWorkerThreadID = nWorkerThreadID;

	VkCommandBuffer cmd = (VkCommandBuffer)GetCommandListPtr(nID);

	ms_pCurrentCommandList = cmd;
	ms_nCurrentCommandListID = nID;

	pCommandList->m_nGlobalThreadID = CThread::GetCurrentThreadId();

	/*CRenderManager::ResetEngineState();
	CRenderManager::RenderTargetsRAZ(true);
	CRenderManager::ViewportRAZ();*/

	if (pCommandList->GetState() != e_Invalid)
		ResetCommandList(nID);

	VkCommandBufferBeginInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	desc.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	desc.pNext = nullptr;
	desc.pInheritanceInfo = nullptr;

	VkResult res = vkBeginCommandBuffer(cmd, &desc);
	ASSERT(res == VK_SUCCESS);

	if (pCommandList->m_nInitialPipelineStateID != INVALIDHANDLE)
	{
		/*ID3D12RootSignature* pRootSignature = (ID3D12RootSignature*)CPipelineManager::GetPipelineRootSignature(pCommandList->m_nInitialPipelineStateID);

		if (CPipelineManager::GetPipelineType(pCommandList->m_nInitialPipelineStateID) == CPipelineManager::e_GraphicsPipeline)
			((ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nID))->SetGraphicsRootSignature(pRootSignature);

		else
			((ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nID))->SetComputeRootSignature(pRootSignature);*/

		CPipelineManager::BindShaders(pCommandList->m_nInitialPipelineStateID);
	}

	pCommandList->SetState(e_Recording);
}



void CCommandListManager::EndRecording(unsigned int nID)
{
	ASSERT(nID > 0 && nID <= ms_nNumCommandLists);

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	ASSERT(pCommandList->GetState() == e_Recording);

	VkResult res = vkEndCommandBuffer((VkCommandBuffer)GetCommandListPtr(nID));
	ASSERT(res == VK_SUCCESS);

	pCommandList->SetState(e_Executable);
	pCommandList->m_pLock->Release();
}


void CCommandListManager::LaunchKickoff()
{
	if (!CFrameBlueprint::IsSorting())
	{
		ms_nCurrentKickoffID++;
		ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID] = 0;
	}
}


void CCommandListManager::ScheduleForNextKickoff(unsigned int cmdList)
{
	if (!CFrameBlueprint::IsSorting())
	{
		if (ms_pCommandLists[cmdList - 1]->m_eType == ECommandListType::e_Compute)
			ms_DeferredKickoffs[ms_nCurrentKickoffID][ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]] = { SExecutable::e_CommandList, e_Queue_AsyncCompute, cmdList };
		else
			ms_DeferredKickoffs[ms_nCurrentKickoffID][ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]] = { SExecutable::e_CommandList, e_Queue_Direct, cmdList };

		ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]++;
	}
}



void CCommandListManager::InsertFence(EQueueType queueType, unsigned int fenceValue)
{
	if (!CFrameBlueprint::IsSorting())
	{
		ms_DeferredKickoffs[ms_nCurrentKickoffID][ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]] = { SExecutable::e_InsertComputeFence, queueType, fenceValue };

		ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]++;
	}
}


void CCommandListManager::WaitOnFence(EQueueType queueType, unsigned int fenceValue)
{
	if (!CFrameBlueprint::IsSorting())
	{
		ms_DeferredKickoffs[ms_nCurrentKickoffID][ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]] = { SExecutable::e_WaitOnComputeFence, queueType, fenceValue };

		ms_nNumExecutablePerKickoff[ms_nCurrentKickoffID]++;
	}
}


void CCommandListManager::LaunchDeferredKickoffs()
{
	if (!CFrameBlueprint::IsSorting())
	{
		for (UINT i = 0; i < ms_nCurrentKickoffID; i++)
			ExecuteCommandLists(ms_DeferredKickoffs[i], ms_nNumExecutablePerKickoff[i]);

		ms_nCurrentKickoffID = 0;
		ms_nNumExecutablePerKickoff[0] = 0;
	}
}


void CCommandListManager::ExecuteCommandLists(SExecutable* IDs, unsigned int numExecutables)
{
	if (numExecutables == 0)
		return;

	for (unsigned i = 0u; i < numExecutables; i++)
	{
		if (IDs[i].m_Type == SExecutable::e_CommandList)
		{

			ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_pCompletedEvent->Wait();
			ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_pCompletedEvent->Reset();

			ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_pLock->Take();

			ASSERT(ms_pCommandLists[IDs[i].m_CmdListID - 1]->GetState() == e_Executable);
		}
	}

	EQueueType commandType_to_queueType[e_NumTypes] = {

		e_Queue_Direct,				// e_Direct
		e_Queue_Direct,				// e_Bundle,
		e_Queue_AsyncCompute,		// e_Compute,
		e_Queue_Copy,				// e_Copy,
		e_Queue_Direct,				// e_VideoDecode,
		e_Queue_Direct,				// e_VideoProcess,
		e_Queue_Copy,				// e_Loading,
	};

	// Two-pass design: first build the full list of batches we'd submit, then
	// fold per-queue pending waits onto the FIRST batch on each queue and
	// pending signals/fence onto the LAST. Issuing inline (as before) made it
	// impossible to attach signals to the last batch without an extra empty
	// submit, which is exactly what we're refactoring away.
	struct SBatch
	{
		EQueueType                          m_QueueType;
		std::vector<VkCommandBuffer>        m_CommandBuffers;	// empty for fence-only batches
		std::vector<VkSemaphore>            m_WaitSemaphores;
		std::vector<VkPipelineStageFlags>   m_WaitStages;
		std::vector<VkSemaphore>            m_SignalSemaphores;
		VkFence                             m_Fence = VK_NULL_HANDLE;
	};

	std::vector<SBatch> batches;
	batches.reserve(numExecutables + 1);

	auto FlushPending = [&](std::vector<VkCommandBuffer>& pending, EQueueType queueType)
	{
		if (pending.empty())
			return;

		SBatch b;
		b.m_QueueType      = queueType;
		b.m_CommandBuffers = std::move(pending);
		batches.push_back(std::move(b));
		pending.clear();
	};

	std::vector<VkCommandBuffer> pendingCommandBuffers;
	ECommandListType currentType = ms_pCommandLists[IDs[0].m_CmdListID - 1]->m_eType;

	for (unsigned i = 0u; i < numExecutables; i++)
	{
		if (IDs[i].m_Type == SExecutable::e_CommandList)
		{
			ECommandListType eType = ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_eType;

			// On a queue change, close out the run on the previous queue.
			if (eType != currentType)
				FlushPending(pendingCommandBuffers, commandType_to_queueType[currentType]);

			pendingCommandBuffers.push_back((VkCommandBuffer)GetCommandListPtr(IDs[i].m_CmdListID));
			currentType = eType;
		}
		else
		{
			// Fence-style executable: flush any pending command lists first
			// (so the fence batch sits at the right point in submission order),
			// then add a stand-alone empty batch for the fence semaphore.
			FlushPending(pendingCommandBuffers, commandType_to_queueType[currentType]);

			SBatch b;
			b.m_QueueType = commandType_to_queueType[currentType];

			if (IDs[i].m_Type == SExecutable::e_InsertComputeFence)
				b.m_SignalSemaphores.push_back(gs_AsyncComputeFence[CDeviceManager::GetFrameIndex()]);

			else if (IDs[i].m_Type == SExecutable::e_WaitOnComputeFence)
			{
				b.m_WaitSemaphores.push_back(gs_AsyncComputeFence[CDeviceManager::GetFrameIndex()]);
				// AsyncComputeFence is signaled at top-of-pipe semantics — we
				// gate the next color/compute work on it. ALL_COMMANDS keeps
				// the original behaviour (no implicit stage was set before).
				b.m_WaitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
			}

			batches.push_back(std::move(b));
		}
	}

	// Trailing run of command lists on the current queue.
	FlushPending(pendingCommandBuffers, commandType_to_queueType[currentType]);

	// Apply per-queue pending sync: pending waits → first batch on that queue,
	// pending signals/fence → last batch on that queue. The pending state is
	// cleared as it's consumed so a subsequent ExecuteCommandLists in the
	// same frame doesn't re-attach the same semaphores.
	int firstBatchIdx[EQueueType::e_NumQueues];
	int lastBatchIdx [EQueueType::e_NumQueues];
	for (int q = 0; q < EQueueType::e_NumQueues; ++q)
	{
		firstBatchIdx[q] = -1;
		lastBatchIdx [q] = -1;
	}

	for (int i = 0; i < (int)batches.size(); ++i)
	{
		const int q = batches[i].m_QueueType;
		if (firstBatchIdx[q] == -1)
			firstBatchIdx[q] = i;
		lastBatchIdx[q] = i;
	}

	for (int q = 0; q < EQueueType::e_NumQueues; ++q)
	{
		SPendingQueueSync& p = gs_PendingSync[q];

		if (firstBatchIdx[q] != -1 && !p.m_WaitSemaphores.empty())
		{
			SBatch& b = batches[firstBatchIdx[q]];
			b.m_WaitSemaphores.insert(b.m_WaitSemaphores.end(), p.m_WaitSemaphores.begin(), p.m_WaitSemaphores.end());
			b.m_WaitStages    .insert(b.m_WaitStages    .end(), p.m_WaitStages    .begin(), p.m_WaitStages    .end());
			p.m_WaitSemaphores.clear();
			p.m_WaitStages    .clear();
		}

		if (lastBatchIdx[q] != -1)
		{
			SBatch& b = batches[lastBatchIdx[q]];

			if (!p.m_SignalSemaphores.empty())
			{
				b.m_SignalSemaphores.insert(b.m_SignalSemaphores.end(), p.m_SignalSemaphores.begin(), p.m_SignalSemaphores.end());
				p.m_SignalSemaphores.clear();
			}

			if (p.m_Fence != VK_NULL_HANDLE)
			{
				ASSERT(b.m_Fence == VK_NULL_HANDLE && "Two pending fences on the same queue's last batch");
				b.m_Fence = p.m_Fence;
				p.m_Fence = VK_NULL_HANDLE;
			}
		}
	}

	for (SBatch& b : batches)
	{
		VkSubmitInfo submitInfo{};
		submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount   = static_cast<uint32_t>(b.m_CommandBuffers.size());
		submitInfo.pCommandBuffers      = b.m_CommandBuffers.empty() ? nullptr : b.m_CommandBuffers.data();
		submitInfo.waitSemaphoreCount   = static_cast<uint32_t>(b.m_WaitSemaphores.size());
		submitInfo.pWaitSemaphores      = b.m_WaitSemaphores.empty() ? nullptr : b.m_WaitSemaphores.data();
		submitInfo.pWaitDstStageMask    = b.m_WaitStages.empty()     ? nullptr : b.m_WaitStages.data();
		submitInfo.signalSemaphoreCount = static_cast<uint32_t>(b.m_SignalSemaphores.size());
		submitInfo.pSignalSemaphores    = b.m_SignalSemaphores.empty() ? nullptr : b.m_SignalSemaphores.data();

		VkResult res = vkQueueSubmit((VkQueue)ms_pCommandQueue[b.m_QueueType], 1, &submitInfo, b.m_Fence);
		ASSERT(res == VK_SUCCESS);
	}

	for (unsigned i = 0u; i < numExecutables; i++)
	{
		if (IDs[i].m_Type == SExecutable::e_CommandList)
			ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_pLock->Release();
	}
}


void CCommandListManager::ResetCommandList(unsigned int nID)
{
	ASSERT(nID > 0 && nID <= ms_nNumCommandLists);

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	UINT frameIndex = (pCommandList->m_eType == e_Direct && !(pCommandList->m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::GetFrameIndex() : 0;

	vkResetCommandBuffer((VkCommandBuffer)GetCommandListPtr(nID), 0);

	pCommandList->SetState(e_Invalid);
}
