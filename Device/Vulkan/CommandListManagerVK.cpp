#include "../DeviceManager.h"
#include "../CommandListManager.h"
#include "../PipelineManager.h"
#include "../RenderThreads.h"



std::vector<CCommandListManager::SCommandList*>	CCommandListManager::ms_pCommandLists;
std::vector<void*>								CCommandListManager::ms_pCommandAllocators[ECommandListType::e_NumTypes][CDeviceManager::ms_FrameCount];
void*											CCommandListManager::ms_pMainRenderingThreadCommandAllocator[CDeviceManager::ms_FrameCount];
void*											CCommandListManager::ms_pCommandQueue[CCommandListManager::EQueueType::e_NumQueues];

int												CCommandListManager::ms_nNumWorkerThreads = 8;
unsigned int									CCommandListManager::ms_nNumLoadingThreads = 0;

thread_local void*								CCommandListManager::ms_pCurrentCommandList = nullptr;
thread_local void*								CCommandListManager::ms_pCurrentLoadingCommandAllocator = nullptr;
thread_local unsigned int						CCommandListManager::ms_nCurrentCommandListID = 0;
std::vector<std::vector<unsigned int>>			CCommandListManager::ms_DeferredKickoffs;

CMutex*											CCommandListManager::ms_pCommandListCreationLock = nullptr;


void CCommandListManager::Init(int nNumWorkerThreads)
{
	for (int type = 0; type < e_NumTypes; type++)
	{
		int numThreads = type == e_Loading ? 8 : nNumWorkerThreads;

		for (int threadID = 0; threadID < numThreads; threadID++)
		{
			UINT numFrames = type == e_Direct ? CDeviceManager::ms_FrameCount : 1;

			for (UINT frame = 0; frame < numFrames; frame++)
				ms_pCommandAllocators[type][frame].push_back(CreateCommandAllocator((ECommandListType)type));
		}
	}

	for (UINT frame = 0; frame < CDeviceManager::ms_FrameCount; frame++)
		ms_pMainRenderingThreadCommandAllocator[frame] = CreateCommandAllocator(e_Direct);

	ms_pCommandLists.clear();
	ms_pCommandLists.reserve(ms_nMaxCommandListID);

	ms_pCommandListCreationLock = CMutex::Create();

#ifdef EKOPLF_X2_DEFINE
	ms_pCommandQueueLock = CMutex::Create();
#endif

	ms_nNumWorkerThreads = nNumWorkerThreads;

	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_Direct,		(VkQueue*)&ms_pCommandQueue[e_Queue_Direct]);
	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_Copy,			(VkQueue*)&ms_pCommandQueue[e_Queue_Copy]);
	vkGetDeviceQueue(CDeviceManager::GetDevice(), CDeviceManager::GetGraphicsQueueFamilyIndex(), e_Queue_AsyncCompute,	(VkQueue*)&ms_pCommandQueue[e_Queue_AsyncCompute]);
}


void CCommandListManager::Terminate()
{
	ms_pCommandLists.clear();

	delete ms_pCommandListCreationLock;

	for (int type = 0; type < e_NumTypes; type++)
	{
		for (int frame = 0; frame < CDeviceManager::ms_FrameCount; frame++)
		{
			int size = static_cast<int>(ms_pCommandAllocators[type][frame].size());

			for (int i = 0; i < size; i++)
				if (ms_pCommandAllocators[type][frame][i] != nullptr)
					vkDestroyCommandPool(CDeviceManager::GetDevice(), (VkCommandPool)(ms_pCommandAllocators[type][frame][i]), nullptr);

			ms_pCommandAllocators[type][frame].clear();
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

	if (eType == e_Loading)
	{
		UINT threadID = GetNumLoadingCommandLists();

		list->m_nWorkerThreadID = threadID;

		void* pCommandList = nullptr;

		VkCommandBufferAllocateInfo desc = {};
		desc.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		desc.commandPool		= (VkCommandPool)ms_pCommandAllocators[eType][0][threadID];
		desc.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		desc.commandBufferCount = 1;

		int ret = vkAllocateCommandBuffers(CDeviceManager::GetDevice(), &desc, (VkCommandBuffer*)(&pCommandList));

		list->m_pCommandList[0].push_back(pCommandList);

		list->m_eState[0].push_back(e_Executable);
	}


	else
	{
		for (UINT frame = 0; frame < numFrames; frame++)
		{
			for (int threadID = 0; threadID < ms_nNumWorkerThreads; threadID++)
			{
				void* pCommandList = nullptr;

				VkCommandBufferAllocateInfo desc = {};
				desc.sType				= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				desc.commandPool		= (VkCommandPool)ms_pCommandAllocators[eType][0][threadID];
				desc.level				= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				desc.commandBufferCount = 1;

				int ret = vkAllocateCommandBuffers(CDeviceManager::GetDevice(), &desc, (VkCommandBuffer*)(&pCommandList));
				ASSERT(ret == VK_SUCCESS);

				list->m_pCommandList[frame].push_back(pCommandList);
				list->m_eState[frame].push_back(e_Executable);
			}
		}
	}

	unsigned int nID = static_cast<unsigned int>(ms_pCommandLists.size());

	ms_pCommandLists.push_back(list);

	ms_pCommandListCreationLock->Release();

	return nID + 1;
}


CCommandListManager::SCommandList::SCommandList()
{
	for (unsigned int i = 0; i < CDeviceManager::ms_FrameCount; i++)
		m_pCommandList[i].reserve(128);
}


CCommandListManager::SCommandList::~SCommandList()
{
	delete m_pCompletedEvent;
}


void* CCommandListManager::GetCommandListPtr(unsigned int nID)
{
	ASSERT(nID > 0 && nID <= ms_pCommandLists.size());

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
	UINT NumLoadingLists = 0;

	std::vector<SCommandList*>::iterator it;

	for (it = ms_pCommandLists.begin(); it < ms_pCommandLists.end(); it++)
		if ((*it)->m_eType == e_Loading)
			NumLoadingLists++;

	return NumLoadingLists;
}



void CCommandListManager::BeginRecording(unsigned int nID, unsigned int nWorkerThreadID)
{
	ASSERT(nID > 0 && nID <= ms_pCommandLists.size());

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	pCommandList->m_pLock->Take();

	if (pCommandList->m_eType != e_Loading)
		pCommandList->m_nWorkerThreadID = nWorkerThreadID;

	ms_pCurrentCommandList = CCommandListManager::GetCommandListPtr(nID);
	ms_nCurrentCommandListID = nID;

	pCommandList->m_nGlobalThreadID = CThread::GetCurrentThreadId();

	/*CRenderManager::ResetEngineState();
	CRenderManager::RenderTargetsRAZ(true);
	CRenderManager::ViewportRAZ();*/

	if (pCommandList->GetState() != e_Invalid)
		ResetCommandList(nID);

	VkCommandBufferBeginInfo desc = {};
	desc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	desc.flags = 0;
	desc.pNext = nullptr;
	desc.pInheritanceInfo = nullptr;

	VkResult res = vkBeginCommandBuffer((VkCommandBuffer)GetCommandListPtr(nID), &desc);
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
	ASSERT(nID > 0 && nID <= ms_pCommandLists.size());

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	ASSERT(pCommandList->GetState() == e_Recording);

	VkResult res = vkEndCommandBuffer((VkCommandBuffer)GetCommandListPtr(nID));
	ASSERT(res == VK_SUCCESS);

	pCommandList->SetState(e_Executable);
	pCommandList->m_pLock->Release();
}


void CCommandListManager::ScheduleDeferredKickoff(std::vector<unsigned int>& kickoff)
{
	if (!CFrameBlueprint::IsSorting())
		ms_DeferredKickoffs.push_back(kickoff);
}


void CCommandListManager::ScheduleDeferredKickoff(unsigned int kickoff)
{
	if (!CFrameBlueprint::IsSorting())
	{
		std::vector<unsigned int> v;
		v.push_back(kickoff);

		ms_DeferredKickoffs.push_back(v);
	}
}


void CCommandListManager::LaunchDeferredKickoffs()
{
	if (!CFrameBlueprint::IsSorting())
	{
		UINT nNumKickoffs = static_cast<UINT>(ms_DeferredKickoffs.size());

		for (UINT i = 0; i < nNumKickoffs; i++)
			ExecuteCommandLists(ms_DeferredKickoffs[i]);

		ms_DeferredKickoffs.clear();
	}
}


void CCommandListManager::ExecuteCommandLists(std::vector<unsigned int>& IDs)
{
	if (IDs.size() == 0)
		return;

	std::vector<VkCommandBuffer> pCommandListPtr;

	UINT NumCommandLists = static_cast<UINT>(IDs.size());

	ECommandListType eType = ms_pCommandLists[IDs[0] - 1]->m_eType;

	for (UINT i = 0; i < NumCommandLists; i++)
	{
		ASSERT(eType == ms_pCommandLists[IDs[i] - 1]->m_eType);

		ms_pCommandLists[IDs[i] - 1]->m_pCompletedEvent->Wait();
		ms_pCommandLists[IDs[i] - 1]->m_pCompletedEvent->Reset();

		ms_pCommandLists[IDs[i] - 1]->m_pLock->Take();

		ASSERT(ms_pCommandLists[IDs[i] - 1]->GetState() == e_Executable);

		pCommandListPtr.push_back((VkCommandBuffer)GetCommandListPtr(IDs[i]));
	}

	EQueueType queueType;

	if (eType == e_Copy)
		queueType = e_Queue_Copy;

	else if (eType == e_Compute)
		queueType = e_Queue_AsyncCompute;

	else
		queueType = e_Queue_Direct;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType				= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount	= NumCommandLists;
	submitInfo.pCommandBuffers		= pCommandListPtr.data();
	submitInfo.waitSemaphoreCount	= 0;
	submitInfo.signalSemaphoreCount	= 0;

	vkQueueSubmit((VkQueue)ms_pCommandQueue[queueType], 1, &submitInfo, VK_NULL_HANDLE);

	for (UINT i = 0; i < NumCommandLists; i++)
	{
		ms_pCommandLists[IDs[i] - 1]->m_pLock->Release();
	}
}


void CCommandListManager::ResetCommandList(unsigned int nID)
{
	ASSERT(nID > 0 && nID <= ms_pCommandLists.size());

	SCommandList* pCommandList = ms_pCommandLists[nID - 1];

	UINT frameIndex = (pCommandList->m_eType == e_Direct && !(pCommandList->m_eFlags & e_NoFrameBuffering)) ? CDeviceManager::GetFrameIndex() : 0;

	vkResetCommandBuffer((VkCommandBuffer)GetCommandListPtr(nID), 0);

	pCommandList->SetState(e_Invalid);
}
