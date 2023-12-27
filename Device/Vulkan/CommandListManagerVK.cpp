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

	std::vector<VkCommandBuffer> pCommandListPtr;

	ECommandListType currentType = ms_pCommandLists[IDs[0].m_CmdListID - 1]->m_eType;

	for (unsigned i = 0u; i < numExecutables; i++)
	{
		if (IDs[i].m_Type == SExecutable::e_CommandList)
		{
			ECommandListType eType = ms_pCommandLists[IDs[i].m_CmdListID - 1]->m_eType;

			/* If the CommandListType of this ID is different, flush the list on the current queue, and switch*/
			if (eType != currentType)
			{
				if (pCommandListPtr.size() > 0)
				{
					EQueueType queueType			= commandType_to_queueType[currentType];

					VkSubmitInfo submitInfo = {};
					submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.commandBufferCount	= pCommandListPtr.size();
					submitInfo.pCommandBuffers		= pCommandListPtr.data();
					submitInfo.waitSemaphoreCount	= 0;
					submitInfo.signalSemaphoreCount	= 0;

					vkQueueSubmit((VkQueue)ms_pCommandQueue[queueType], 1, &submitInfo, VK_NULL_HANDLE);
				}

				pCommandListPtr.clear();
			}

			pCommandListPtr.push_back((VkCommandBuffer)GetCommandListPtr(IDs[i].m_CmdListID));
			currentType = eType;
		}
		else
		{
			VkSubmitInfo submitInfo = {};
			submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount	= 0;
			submitInfo.waitSemaphoreCount	= 0;
			submitInfo.signalSemaphoreCount	= 0;

			if (IDs[i].m_Type == SExecutable::e_InsertComputeFence)
			{
				submitInfo.signalSemaphoreCount = 1;
				submitInfo.pSignalSemaphores	= &gs_AsyncComputeFence[CDeviceManager::GetFrameIndex()];
			}
			
			if (IDs[i].m_Type == SExecutable::e_WaitOnComputeFence)
			{
				submitInfo.waitSemaphoreCount = 1;
				submitInfo.pWaitSemaphores = &gs_AsyncComputeFence[CDeviceManager::GetFrameIndex()];
			}

			EQueueType queueType = commandType_to_queueType[currentType];

			vkQueueSubmit((VkQueue)ms_pCommandQueue[queueType], 1, &submitInfo, VK_NULL_HANDLE);
		}
	}

	EQueueType queueType = commandType_to_queueType[currentType];

	if (pCommandListPtr.size() > 0)
	{
		if (pCommandListPtr.size() > 0)
		{
			EQueueType queueType			= commandType_to_queueType[currentType];

			VkSubmitInfo submitInfo = {};
			submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount	= pCommandListPtr.size();
			submitInfo.pCommandBuffers		= pCommandListPtr.data();
			submitInfo.waitSemaphoreCount	= 0;
			submitInfo.signalSemaphoreCount	= 0;

			vkQueueSubmit((VkQueue)ms_pCommandQueue[queueType], 1, &submitInfo, VK_NULL_HANDLE);
		}

		pCommandListPtr.clear();
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
