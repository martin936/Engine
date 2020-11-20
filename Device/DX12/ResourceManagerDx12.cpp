#include "../DeviceManager.h"
#include "../ResourceManager.h"
#include "../CommandListManager.h"
#include "../RenderThreads.h"
#include "../PipelineManager.h"



#define MAX_GLOBAL_SRV_CBV_UAV_DESCRIPTOR_COUNT		8192
#define MAX_LOCAL_SRV_CBV_UAV_DESCRIPTOR_COUNT		65536

#define MAX_RTV_DESCRIPTOR_COUNT					256
#define MAX_DSV_DESCRIPTOR_COUNT					128

#define MAX_GLOBAL_SAMPLER_DESCRIPTOR_COUNT			16 
#define MAX_LOCAL_SAMPLER_DESCRIPTOR_COUNT			2048 

#define CONSTANT_BUFFER_MAX_VERSIONS_PER_FRAME		64


std::vector<void*>												CResourceManager::ms_pCurrentSRVDescriptorHeap;
std::vector<void*>												CResourceManager::ms_pCurrentSamplerDescriptorHeap;

std::vector<CResourceManager::SAllocator>						CResourceManager::ms_Allocators;
std::vector<CResourceManager::SSamplerAllocator>				CResourceManager::ms_SamplerAllocators;
std::vector<CResourceManager::SDecriptorCopyQuery>				CResourceManager::ms_CopyQueries;
std::vector<CResourceManager::SSamplerDecriptorCopyQuery>		CResourceManager::ms_SamplerCopyQueries;

void*															CResourceManager::ms_pGlobalSRVDescriptorHeap = nullptr;
std::vector<void*>												CResourceManager::ms_pLocalSRVDescriptorHeap;

void*															CResourceManager::ms_pGlobalSamplerDescriptorHeap = nullptr;
std::vector<void*>												CResourceManager::ms_pLocalSamplerDescriptorHeap;

std::vector<void*>												CResourceManager::ms_pUploadResources[CSchedulerThread::ms_nMaxThreadCount];
std::vector<void*>												CResourceManager::ms_pResources;

std::vector<unsigned int>										CResourceManager::ms_pFreeSRVDescriptorSlot;

std::vector<CResourceManager::SConstantBuffer>					CResourceManager::ms_pConstantBuffers[CDeviceManager::m_FrameCount];

thread_local std::vector<CResourceManager::SSlotAssociation>	CResourceManager::ms_SRVs;
thread_local std::vector<CResourceManager::SSlotAssociation>	CResourceManager::ms_CBVs;
thread_local std::vector<CResourceManager::SSlotAssociation>	CResourceManager::ms_UAVs;
thread_local std::vector<CResourceManager::SSlotAssociation>	CResourceManager::ms_Samplers;

thread_local unsigned int										CResourceManager::ms_nMaxConstantBufferVersions = CONSTANT_BUFFER_MAX_VERSIONS_PER_FRAME;

void*															CResourceManager::ms_pGlobalRTVDescriptorHeap = nullptr;
void*															CResourceManager::ms_pGlobalDSVDescriptorHeap = nullptr;

unsigned int													CResourceManager::ms_nGlobalSRVDescriptorHeapOffset = 0;
unsigned int													CResourceManager::ms_nGlobalRTVDescriptorHeapOffset = 0;
unsigned int													CResourceManager::ms_nGlobalDSVDescriptorHeapOffset = 0;

unsigned int													CResourceManager::ms_nDynamicSRVHeapStart = 0;
unsigned int													CResourceManager::ms_nDynamicRTVHeapStart = 0;
unsigned int													CResourceManager::ms_nDynamicDSVHeapStart = 0;
unsigned int													CResourceManager::ms_nDynamicResourceStartIndex = 0;

unsigned int													CResourceManager::ms_nDescriptorSizeSRV = 0;
unsigned int													CResourceManager::ms_nDescriptorSizeRTV = 0;
unsigned int													CResourceManager::ms_nDescriptorSizeDSV = 0;
unsigned int													CResourceManager::ms_nDescriptorSizeSampler = 0;

CMutex*															CResourceManager::ms_pViewDescriptorLock;
CMutex*															CResourceManager::ms_pUploadLock;

void*															CResourceManager::ms_UploadBatch[CSchedulerThread::ms_nMaxThreadCount] = { nullptr };
unsigned int													CResourceManager::ms_nCopyCommandList = 0;


void CResourceManager::Init()
{
	ms_pLocalSRVDescriptorHeap.resize(CDeviceManager::m_FrameCount);
	ms_pLocalSamplerDescriptorHeap.resize(CDeviceManager::m_FrameCount);

	HRESULT hr = S_OK;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = MAX_RTV_DESCRIPTOR_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pGlobalRTVDescriptorHeap);
	ASSERT(SUCCEEDED(hr));

	D3D12_CPU_DESCRIPTOR_HANDLE handle = ((ID3D12DescriptorHeap*)ms_pGlobalRTVDescriptorHeap)->GetCPUDescriptorHandleForHeapStart();

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = MAX_DSV_DESCRIPTOR_COUNT;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pGlobalDSVDescriptorHeap);
	ASSERT(SUCCEEDED(hr));

	handle = ((ID3D12DescriptorHeap*)ms_pGlobalDSVDescriptorHeap)->GetCPUDescriptorHandleForHeapStart();

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
	cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvHeapDesc.NumDescriptors = MAX_GLOBAL_SRV_CBV_UAV_DESCRIPTOR_COUNT;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&cbvSrvHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pGlobalSRVDescriptorHeap);
	ASSERT(SUCCEEDED(hr));

	cbvSrvHeapDesc.NumDescriptors = MAX_LOCAL_SRV_CBV_UAV_DESCRIPTOR_COUNT;
	cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	for (int i = 0; i < CDeviceManager::m_FrameCount; i++)
	{
		hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&cbvSrvHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pLocalSRVDescriptorHeap[i]);
		ASSERT(SUCCEEDED(hr));
	}

	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.NumDescriptors = MAX_GLOBAL_SAMPLER_DESCRIPTOR_COUNT;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&samplerHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pGlobalSamplerDescriptorHeap);
	ASSERT(SUCCEEDED(hr));

	samplerHeapDesc.NumDescriptors = MAX_LOCAL_SAMPLER_DESCRIPTOR_COUNT;
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	for (int i = 0; i < CDeviceManager::m_FrameCount; i++)
	{
		hr = CDeviceManager::GetDevice()->CreateDescriptorHeap(&samplerHeapDesc, __uuidof(ID3D12DescriptorHeap), &ms_pLocalSamplerDescriptorHeap[i]);
		ASSERT(SUCCEEDED(hr));
	}

	ms_nDescriptorSizeSRV = CDeviceManager::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ms_nDescriptorSizeRTV = CDeviceManager::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	ms_nDescriptorSizeDSV = CDeviceManager::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	ms_nDescriptorSizeSampler = CDeviceManager::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	unsigned int nNumThreads = CRenderWorkerThread::GetNumThreads();

	ms_Allocators.resize(nNumThreads);
	ms_SamplerAllocators.resize(nNumThreads);
	ms_CopyQueries.resize(nNumThreads);
	ms_SamplerCopyQueries.resize(nNumThreads);
	ms_pCurrentSRVDescriptorHeap.resize(nNumThreads);
	ms_pCurrentSamplerDescriptorHeap.resize(nNumThreads);

	for (unsigned int i = 0; i < nNumThreads; i++)
	{
		ms_Allocators[i].m_nPoolStart = i * (MAX_LOCAL_SRV_CBV_UAV_DESCRIPTOR_COUNT / nNumThreads);
		ms_Allocators[i].m_nPoolSize = MAX_LOCAL_SRV_CBV_UAV_DESCRIPTOR_COUNT / nNumThreads;
		ms_Allocators[i].m_nNextAvailableAddress = ms_Allocators[i].m_nPoolStart;

		ms_SamplerAllocators[i].m_nPoolStart = i * (MAX_LOCAL_SAMPLER_DESCRIPTOR_COUNT / nNumThreads);
		ms_SamplerAllocators[i].m_nPoolSize = MAX_LOCAL_SAMPLER_DESCRIPTOR_COUNT / nNumThreads;
		ms_SamplerAllocators[i].m_nNextAvailableAddress = ms_SamplerAllocators[i].m_nPoolStart;

		ms_CopyQueries[i].m_CopyDesc.clear();
		ms_SamplerCopyQueries[i].m_CopyDesc.clear();
	}

	ms_pViewDescriptorLock = CMutex::Create();
	ms_pUploadLock = CMutex::Create();

	for (UINT i = 0; i < CSchedulerThread::ms_nMaxThreadCount; i++)
	{
		DirectX::ResourceUploadBatch* batch = new DirectX::ResourceUploadBatch(CDeviceManager::GetDevice());
		batch->Begin();

		ms_UploadBatch[i] = (void*)batch;
	}

	// Create command list for main thread loading
	ms_nCopyCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Loading, INVALIDHANDLE, CCommandListManager::e_NoFrameBuffering | CCommandListManager::e_OnlyOnce);
	CCommandListManager::BeginRecording(ms_nCopyCommandList, 0);

	ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));

	CreateSamplers();
}


void CResourceManager::Terminate()
{
	for (UINT i = 0; i < CSchedulerThread::ms_nMaxThreadCount; i++)
		CleanUploadHeaps(i);

	/*for (size_t i = 0; i < ms_pResources.size(); i++)
		((ID3D12Resource*)ms_pResources[i])->Release();*/

	ms_pResources.clear();

	((ID3D12DescriptorHeap*)ms_pGlobalSRVDescriptorHeap)->Release();

	UINT numConstantBuffers = static_cast<UINT>(ms_pConstantBuffers[0].size());

	for (int i = 0; i < CDeviceManager::m_FrameCount; i++)
	{
		((ID3D12DescriptorHeap*)ms_pLocalSRVDescriptorHeap[i])->Release();
		((ID3D12DescriptorHeap*)ms_pLocalSamplerDescriptorHeap[i])->Release();

		for (UINT j = 0; j < numConstantBuffers; j++)
			ms_pConstantBuffers[i][j].Release();

		ms_pConstantBuffers[i].clear();
	}

	ms_pLocalSRVDescriptorHeap.clear();

	((ID3D12DescriptorHeap*)ms_pGlobalRTVDescriptorHeap)->Release();
	((ID3D12DescriptorHeap*)ms_pGlobalDSVDescriptorHeap)->Release();
	((ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap)->Release();

	for (UINT i = 0; i < CSchedulerThread::ms_nMaxThreadCount; i++)
	{
		delete ((DirectX::ResourceUploadBatch*)ms_UploadBatch[i]);

		ms_UploadBatch[i] = nullptr;
	}

	delete ms_pViewDescriptorLock;
	ms_pViewDescriptorLock = nullptr;

	delete ms_pUploadLock;
	ms_pUploadLock = nullptr;
}



void* CResourceManager::GetUploadBatch()
{
	unsigned int nCommandListID = CCommandListManager::GetCurrentThreadCommandListID();
	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	if (!pCommandList)
	{
		nCommandListID = CCommandListManager::CreateCommandList(CCommandListManager::e_Loading);
		CCommandListManager::BeginRecording(nCommandListID, 0);
		pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	}

	UINT threadID = CCommandListManager::GetCommandList(nCommandListID)->m_nWorkerThreadID;

	return ms_UploadBatch[threadID];
}



void CResourceManager::LoadPendingResources()
{
	unsigned int nCommandListID = CCommandListManager::GetCurrentThreadCommandListID();
	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();

	if (pCommandList == nullptr)
		return;

	UINT threadID = CCommandListManager::GetCommandList(nCommandListID)->m_nWorkerThreadID;

	CCommandListManager::EndRecording(nCommandListID);

	std::vector<unsigned int> IDs;
	IDs.push_back(nCommandListID);

	CCommandListManager::GetCommandList(nCommandListID)->m_pCompletedEvent->Throw();

	CCommandListManager::ExecuteCommandLists(IDs);

#ifdef EKOPLF_X2_DEFINE
	CCommandListManager::LockCommandQueue();
#endif

	((DirectX::ResourceUploadBatch*)ms_UploadBatch[threadID])->End((ID3D12CommandQueue*)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct));

	// Set an event so we get notified when the GPU has completed all its work
	ComPtr<ID3D12Fence> fenceDirect;
	CDeviceManager::GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fenceDirect.GetAddressOf()));

	ID3D12CommandQueue* directQueue = (ID3D12CommandQueue*)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct);

	HANDLE gpuDirectCompletedEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
	ASSERT(gpuDirectCompletedEvent);

	directQueue->Signal(fenceDirect.Get(), 1ULL);
	fenceDirect->SetEventOnCompletion(1ULL, gpuDirectCompletedEvent);

#ifdef EKOPLF_X2_DEFINE
	CCommandListManager::ReleaseCommandQueue();
#endif

	DWORD wr = WaitForSingleObject(gpuDirectCompletedEvent, INFINITE);
	if (wr != WAIT_OBJECT_0)
	{
		if (wr == WAIT_FAILED)
		{
			HRESULT_FROM_WIN32(GetLastError());
		}
		else
		{
			AssertNotReached();
		}
	}

	CleanUploadHeaps(threadID);

	CCommandListManager::BeginRecording(nCommandListID, 0);

	DirectX::ResourceUploadBatch* batch = (DirectX::ResourceUploadBatch*)ms_UploadBatch[threadID];
	batch->Begin();
}


void CResourceManager::SetCurrentSRVDescriptorHeap()
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();
	unsigned int nCommandListID = CRenderWorkerThread::GetCurrentThreadCommandListID();

	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nCommandListID);

	ID3D12DescriptorHeap* ppHeaps[] = { (ID3D12DescriptorHeap*)(ms_pLocalSRVDescriptorHeap[CDeviceManager::m_FrameIndex]), (ID3D12DescriptorHeap*)(ms_pLocalSamplerDescriptorHeap[CDeviceManager::m_FrameIndex]) };

	pCommandList->SetDescriptorHeaps(2, ppHeaps);

	ms_pCurrentSRVDescriptorHeap[nThreadID] = ms_pLocalSRVDescriptorHeap[CDeviceManager::m_FrameIndex];
	ms_pCurrentSamplerDescriptorHeap[nThreadID] = ms_pLocalSamplerDescriptorHeap[CDeviceManager::m_FrameIndex];
}


void CResourceManager::ResetAllocators()
{
	UINT nNumThreads = static_cast<UINT>(ms_Allocators.size());

	for (UINT i = 0; i < nNumThreads; i++)
	{
		ms_Allocators[i].Reset();
		ms_SamplerAllocators[i].Reset();
		ms_CopyQueries[i].m_CopyDesc.clear();
		ms_SamplerCopyQueries[i].m_CopyDesc.clear();
	}
}


void GetCPUDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE* handle, ID3D12DescriptorHeap* pHeap, unsigned int index, unsigned int increment)
{
	handle->ptr = static_cast<SIZE_T>(pHeap->GetCPUDescriptorHandleForHeapStart().ptr + UINT64(index) * UINT64(increment));
}


void GetGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE* handle, ID3D12DescriptorHeap* pHeap, unsigned int index, unsigned int increment)
{
	handle->ptr = static_cast<SIZE_T>(pHeap->GetGPUDescriptorHandleForHeapStart().ptr + UINT64(index) * UINT64(increment));
}


void CResourceManager::GetRenderTargetDescriptor(void* destDesc, unsigned int nID)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* pHandle = (D3D12_CPU_DESCRIPTOR_HANDLE*)destDesc;

	if (nID > 0 && nID < MAX_RTV_DESCRIPTOR_COUNT)
		pHandle->ptr = static_cast<SIZE_T>(((ID3D12DescriptorHeap*)ms_pGlobalRTVDescriptorHeap)->GetCPUDescriptorHandleForHeapStart().ptr + UINT64(nID - 1) * UINT64(ms_nDescriptorSizeRTV));

	else
		pHandle->ptr = 0;
}


void CResourceManager::GetSamplerDescriptor(void* pHandle, unsigned int eSampler)
{
	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, eSampler, ms_nDescriptorSizeSampler);
}


void CResourceManager::GetDepthStencilDescriptor(void* destDesc, unsigned int nID)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* pHandle = (D3D12_CPU_DESCRIPTOR_HANDLE*)destDesc;

	if (nID > 0 && nID < MAX_DSV_DESCRIPTOR_COUNT)
		pHandle->ptr = static_cast<SIZE_T>(((ID3D12DescriptorHeap*)ms_pGlobalDSVDescriptorHeap)->GetCPUDescriptorHandleForHeapStart().ptr + UINT64(nID - 1) * UINT64(ms_nDescriptorSizeDSV));

	else
		pHandle->ptr = 0;
}



unsigned int  CResourceManager::SAllocator::AllocateDescriptorRange(unsigned int nNumSRV, unsigned int nNumCBV, unsigned int nNumUAV)
{
	m_nCurrentChunkSize = nNumSRV + nNumCBV + nNumUAV;
	m_nNumSRVInCurrentChunk = nNumSRV;
	m_nNumCBVInCurrentChunk = nNumCBV;
	m_nNumUAVInCurrentChunk = nNumUAV;

	if (m_nPoolStart + m_nPoolSize - m_nNextAvailableAddress > m_nCurrentChunkSize)
	{
		unsigned int addr = m_nNextAvailableAddress;
		m_nNextAvailableAddress += m_nCurrentChunkSize;
		m_nCurrentChunkStartAddr = addr;
	}

	else
	{
		m_nNextAvailableAddress = m_nPoolStart + m_nCurrentChunkSize;
		m_nCurrentChunkStartAddr = m_nPoolStart;
	}

	return m_nCurrentChunkStartAddr;
}



unsigned int  CResourceManager::SSamplerAllocator::AllocateDescriptorRange(unsigned int nNumSampler)
{
	m_nCurrentChunkSize = nNumSampler;
	m_nNumSamplersInCurrentChunk = nNumSampler;

	if (m_nPoolStart + m_nPoolSize - m_nNextAvailableAddress > m_nCurrentChunkSize)
	{
		unsigned int addr = m_nNextAvailableAddress;
		m_nNextAvailableAddress += m_nCurrentChunkSize;
		m_nCurrentChunkStartAddr = addr;
	}

	else
	{
		m_nNextAvailableAddress = m_nPoolStart + m_nCurrentChunkSize;
		m_nCurrentChunkStartAddr = m_nPoolStart;
	}

	return m_nCurrentChunkStartAddr;
}


unsigned int CResourceManager::SAllocator::GetSRVAddr(unsigned int nSlot)
{
	ASSERT(nSlot < m_nNumSRVInCurrentChunk);

	return m_nCurrentChunkStartAddr + nSlot;
}


unsigned int CResourceManager::SAllocator::GetCBVAddr(unsigned int nSlot)
{
	ASSERT(nSlot < m_nNumCBVInCurrentChunk);

	return m_nCurrentChunkStartAddr + m_nNumSRVInCurrentChunk + nSlot;
}


unsigned int CResourceManager::SAllocator::GetUAVAddr(unsigned int nSlot)
{
	ASSERT(nSlot < m_nNumUAVInCurrentChunk);

	return m_nCurrentChunkStartAddr + m_nNumSRVInCurrentChunk + m_nNumCBVInCurrentChunk + nSlot;
}


void CResourceManager::BeginDescriptorsDesc(unsigned int nNumSRV, unsigned int nNumCBV, unsigned int nNumUAV)
{
	if (nNumSRV + nNumCBV + nNumUAV == 0)
		return;

	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	SDescriptorCopyDesc desc;
	desc.m_nSrcAddr.resize(nNumSRV + nNumCBV + nNumUAV);
	desc.m_nDstAddr = ms_Allocators[nThreadID].AllocateDescriptorRange(nNumSRV, nNumCBV, nNumUAV);

	desc.m_nSrcAddr[0] = INVALIDHANDLE;

	ms_CopyQueries[nThreadID].m_CopyDesc.push_back(desc);
}


void CResourceManager::BeginSamplerDescriptorsDesc(unsigned int nNumSamplers)
{
	if (nNumSamplers == 0)
		return;

	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	SSamplerDescriptorCopyDesc desc;
	desc.m_nSrcAddr.resize(nNumSamplers);
	desc.m_nDstAddr = ms_SamplerAllocators[nThreadID].AllocateDescriptorRange(nNumSamplers);

	desc.m_nSrcAddr[0] = INVALIDHANDLE;

	ms_SamplerCopyQueries[nThreadID].m_CopyDesc.push_back(desc);
}


void CResourceManager::SetSRV(unsigned int nSRV, unsigned int nSlot)
{
	ms_SRVs.push_back({ nSlot, nSRV });
}


void CResourceManager::SetCBV(unsigned int nCBV, unsigned int nSlot)
{
	ms_CBVs.push_back({ nSlot, nCBV });
}


void CResourceManager::SetUAV(unsigned int nUAV, unsigned int nSlot)
{
	ms_UAVs.push_back({ nSlot, nUAV });
}


void CResourceManager::SetSampler(unsigned int nSamplerID, unsigned int nSlot)
{
	ms_Samplers.push_back({ nSlot, nSamplerID });
}


void CResourceManager::CopyResource(unsigned int nDestID, unsigned int nSrcID)
{
	ID3D12GraphicsCommandList* pCommandList = reinterpret_cast<ID3D12GraphicsCommandList*>(CCommandListManager::GetCurrentThreadCommandListPtr());

	ID3D12Resource* pDst = reinterpret_cast<ID3D12Resource*>(CBitmapInterface::GetDeviceTexture(nDestID));
	ID3D12Resource* pSrc = reinterpret_cast<ID3D12Resource*>(CBitmapInterface::GetDeviceTexture(nSrcID));

	pCommandList->CopyResource(pDst, pSrc);
}


void CResourceManager::PrepareResources()
{
	UINT numSRV = static_cast<UINT>(ms_SRVs.size());
	UINT numCBV = static_cast<UINT>(ms_CBVs.size());
	UINT numUAV = static_cast<UINT>(ms_UAVs.size());
	UINT numSamplers = static_cast<UINT>(ms_Samplers.size());

	UINT nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	UINT realNumSRV = 0;
	UINT realNumCBV = 0;
	UINT realNumUAV = 0;
	UINT realNumSamplers = 0;

	// Slots may overlap or may not be consecutive, so we define the real number of slots to allocate as the higher used slot + 1
	for (UINT i = 0; i < numSRV; i++)
		realNumSRV = max(realNumSRV, ms_SRVs[i].nSlot + 1);

	for (UINT i = 0; i < numCBV; i++)
		realNumCBV = max(realNumCBV, ms_CBVs[i].nSlot + 1);

	for (UINT i = 0; i < numUAV; i++)
		realNumUAV = max(realNumUAV, ms_UAVs[i].nSlot + 1);

	for (UINT i = 0; i < numSamplers; i++)
		realNumSamplers = max(realNumSamplers, ms_Samplers[i].nSlot + 1);

	BeginDescriptorsDesc(realNumSRV, realNumCBV, realNumUAV);
	BeginSamplerDescriptorsDesc(realNumSamplers);

	for (UINT i = 0; i < numSRV; i++)
	{
		UINT index = ms_Allocators[nThreadID].GetSRVAddr(ms_SRVs[i].nSlot) - ms_Allocators[nThreadID].GetCurrentChunkAddr();

		ms_CopyQueries[nThreadID].m_CopyDesc.back().m_nSrcAddr[index] = ms_SRVs[i].nResourceID - 1; // Convert ID into address (IDs begin at 1 and addresses at 0)
	}

	for (UINT i = 0; i < numCBV; i++)
	{
		UINT index = ms_Allocators[nThreadID].GetCBVAddr(ms_CBVs[i].nSlot) - ms_Allocators[nThreadID].GetCurrentChunkAddr();

		ms_CopyQueries[nThreadID].m_CopyDesc.back().m_nSrcAddr[index] = ms_CBVs[i].nResourceID - 1; // Convert ID into address (IDs begin at 1 and addresses at 0)
	}

	for (UINT i = 0; i < numUAV; i++)
	{
		UINT index = ms_Allocators[nThreadID].GetUAVAddr(ms_UAVs[i].nSlot) - ms_Allocators[nThreadID].GetCurrentChunkAddr();

		ms_CopyQueries[nThreadID].m_CopyDesc.back().m_nSrcAddr[index] = ms_UAVs[i].nResourceID - 1; // Convert ID into address (IDs begin at 1 and addresses at 0)
	}

	for (UINT i = 0; i < numSamplers; i++)
	{
		ms_SamplerCopyQueries[nThreadID].m_CopyDesc.back().m_nSrcAddr[ms_Samplers[i].nSlot] = ms_Samplers[i].nResourceID;
	}

	ms_SRVs.clear();
	ms_CBVs.clear();
	ms_UAVs.clear();
	ms_Samplers.clear();
}


void CResourceManager::SSamplerDecriptorCopyQuery::Flush()
{
	std::vector<SSamplerDescriptorCopyDesc>::iterator it;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> destHandles;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcHandles;
	std::vector<UINT> dstRangeSize;
	std::vector<UINT> srcRangeSize;

	UINT numCopy = 0;

	for (it = m_CopyDesc.begin(); it < m_CopyDesc.end(); it++)
	{
		UINT numDesc = static_cast<UINT>((*it).m_nSrcAddr.size());
		bool discard = false;

		for (UINT i = 0; i < numDesc; i++)
		{
			if ((*it).m_nSrcAddr[i] == INVALIDHANDLE)
			{
				discard = true;
				break;
			}
		}

		if (discard)
			continue;

		D3D12_CPU_DESCRIPTOR_HANDLE dest;
		GetCPUDescriptorHandle(&dest, (ID3D12DescriptorHeap*)ms_pLocalSamplerDescriptorHeap[CDeviceManager::m_FrameIndex], (*it).m_nDstAddr, ms_nDescriptorSizeSampler);
		destHandles.push_back(dest);

		dstRangeSize.push_back(numDesc);

		for (UINT i = 0; i < numDesc; i++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE src;
			GetCPUDescriptorHandle(&src, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, (*it).m_nSrcAddr[i], ms_nDescriptorSizeSampler);
			srcHandles.push_back(src);

			srcRangeSize.push_back(1);
		}

		numCopy++;
	}

	if (numCopy > 0)
		CDeviceManager::GetDevice()->CopyDescriptors(static_cast<UINT>(destHandles.size()), destHandles.data(), dstRangeSize.data(),
			static_cast<UINT>(srcHandles.size()), srcHandles.data(), srcRangeSize.data(),
			D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		);

	m_CopyDesc.clear();
}


void CResourceManager::SDecriptorCopyQuery::Flush()
{
	std::vector<SDescriptorCopyDesc>::iterator it;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> destHandles;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcHandles;
	std::vector<UINT> dstRangeSize;
	std::vector<UINT> srcRangeSize;

	UINT numCopy = 0;

	for (it = m_CopyDesc.begin(); it < m_CopyDesc.end(); it++)
	{
		UINT numDesc = static_cast<UINT>((*it).m_nSrcAddr.size());
		bool discard = false;

		for (UINT i = 0; i < numDesc; i++)
		{
			if ((*it).m_nSrcAddr[i] == INVALIDHANDLE)
			{
				discard = true;
				break;
			}
		}

		if (discard)
			continue;

		D3D12_CPU_DESCRIPTOR_HANDLE dest;
		GetCPUDescriptorHandle(&dest, (ID3D12DescriptorHeap*)ms_pLocalSRVDescriptorHeap[CDeviceManager::m_FrameIndex], (*it).m_nDstAddr, ms_nDescriptorSizeSRV);
		destHandles.push_back(dest);

		dstRangeSize.push_back(numDesc);

		for (UINT i = 0; i < numDesc; i++)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE src;
			GetCPUDescriptorHandle(&src, (ID3D12DescriptorHeap*)ms_pGlobalSRVDescriptorHeap, (*it).m_nSrcAddr[i], ms_nDescriptorSizeSRV);
			srcHandles.push_back(src);

			srcRangeSize.push_back(1);
		}

		numCopy++;
	}

	if (numCopy > 0)
		CDeviceManager::GetDevice()->CopyDescriptors(static_cast<UINT>(destHandles.size()), destHandles.data(), dstRangeSize.data(),
			static_cast<UINT>(srcHandles.size()), srcHandles.data(), srcRangeSize.data(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);

	m_CopyDesc.clear();
}


void CResourceManager::CopyDescriptors()
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	ms_CopyQueries[nThreadID].Flush();
	ms_SamplerCopyQueries[nThreadID].Flush();
}


void CResourceManager::GetSamplerBaseDescriptor(void* pHandle)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pCurrentSamplerDescriptorHeap[nThreadID], ms_SamplerAllocators[nThreadID].GetCurrentChunkAddr(), ms_nDescriptorSizeSampler);
}


void CResourceManager::GetBaseDescriptor(void* pHandle)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pCurrentSRVDescriptorHeap[nThreadID], ms_Allocators[nThreadID].GetCurrentChunkAddr(), ms_nDescriptorSizeSRV);
}


void CResourceManager::GetSRVBaseDescriptor(void* pHandle)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pCurrentSRVDescriptorHeap[nThreadID], ms_Allocators[nThreadID].GetSRVAddr(0), ms_nDescriptorSizeSRV);
}


void CResourceManager::GetCBVBaseDescriptor(void* pHandle)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pCurrentSRVDescriptorHeap[nThreadID], ms_Allocators[nThreadID].GetCBVAddr(0), ms_nDescriptorSizeSRV);
}


void CResourceManager::GetUAVBaseDescriptor(void* pHandle)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	GetGPUDescriptorHandle((D3D12_GPU_DESCRIPTOR_HANDLE*)pHandle, (ID3D12DescriptorHeap*)ms_pCurrentSRVDescriptorHeap[nThreadID], ms_Allocators[nThreadID].GetUAVAddr(0), ms_nDescriptorSizeSRV);
}


void CResourceManager::EndPermanentResourceLoading()
{
	ms_nDynamicSRVHeapStart = ms_nGlobalSRVDescriptorHeapOffset;
	ms_nDynamicRTVHeapStart = ms_nGlobalRTVDescriptorHeapOffset;
	ms_nDynamicDSVHeapStart = ms_nGlobalDSVDescriptorHeapOffset;
	ms_nDynamicResourceStartIndex = static_cast<unsigned int>(ms_pResources.size());
}


void CResourceManager::Reset()
{
	//for (size_t i = ms_nDynamicResourceStartIndex; i < ms_pResources.size(); i++)
		//((ID3D12Resource*)ms_pResources[i])->Release();

	ms_pResources.resize(ms_nDynamicResourceStartIndex);

	//ms_nGlobalSRVDescriptorHeapOffset = ms_nDynamicSRVHeapStart;
	ms_nGlobalRTVDescriptorHeapOffset = ms_nDynamicRTVHeapStart;
	ms_nGlobalDSVDescriptorHeapOffset = ms_nDynamicDSVHeapStart;
}


void CResourceManager::FreeViewDescriptor(EViewType eType, unsigned int nID)
{
	if (ms_pViewDescriptorLock)
	{
		ms_pViewDescriptorLock->Take();
		ms_pFreeSRVDescriptorSlot.push_back(nID - 1);
		ms_pViewDescriptorLock->Release();
	}
}


unsigned int CResourceManager::CreateViewDescriptor(EViewType eType, void* pResource, void* pDesc)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;
	unsigned int index = 0;

	ms_pViewDescriptorLock->Take();

	HRESULT hr = S_OK;

	switch (eType)
	{
	case e_ShaderResourceView:

		if (ms_pFreeSRVDescriptorSlot.size() > 0)
		{
			index = ms_pFreeSRVDescriptorSlot[0];
			ms_pFreeSRVDescriptorSlot.erase(ms_pFreeSRVDescriptorSlot.begin());
		}

		else
		{
			index = ms_nGlobalSRVDescriptorHeapOffset;
			ms_nGlobalSRVDescriptorHeapOffset++;
		}

		GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSRVDescriptorHeap, index, ms_nDescriptorSizeSRV);
		CDeviceManager::GetDevice()->CreateShaderResourceView((ID3D12Resource*)pResource, (D3D12_SHADER_RESOURCE_VIEW_DESC*)pDesc, handle);
		ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));
		break;

	case e_ConstantBufferView:
		GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSRVDescriptorHeap, ms_nGlobalSRVDescriptorHeapOffset, ms_nDescriptorSizeSRV);
		CDeviceManager::GetDevice()->CreateConstantBufferView((D3D12_CONSTANT_BUFFER_VIEW_DESC*)pDesc, handle);
		ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));
		index = ms_nGlobalSRVDescriptorHeapOffset;
		ms_nGlobalSRVDescriptorHeapOffset++;
		break;

	case e_UnorderedAccessView:
		GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSRVDescriptorHeap, ms_nGlobalSRVDescriptorHeapOffset, ms_nDescriptorSizeSRV);
		CDeviceManager::GetDevice()->CreateUnorderedAccessView((ID3D12Resource*)pResource, nullptr, (D3D12_UNORDERED_ACCESS_VIEW_DESC*)pDesc, handle);
		ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));
		index = ms_nGlobalSRVDescriptorHeapOffset;
		ms_nGlobalSRVDescriptorHeapOffset++;
		break;

	case e_RenderTargetView:
		GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalRTVDescriptorHeap, ms_nGlobalRTVDescriptorHeapOffset, ms_nDescriptorSizeRTV);
		CDeviceManager::GetDevice()->CreateRenderTargetView((ID3D12Resource*)pResource, (D3D12_RENDER_TARGET_VIEW_DESC*)pDesc, handle);
		ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));
		index = ms_nGlobalRTVDescriptorHeapOffset;
		ms_nGlobalRTVDescriptorHeapOffset++;
		break;

	case e_DepthStencilView:
		GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalDSVDescriptorHeap, ms_nGlobalDSVDescriptorHeapOffset, ms_nDescriptorSizeDSV);
		CDeviceManager::GetDevice()->CreateDepthStencilView((ID3D12Resource*)pResource, (D3D12_DEPTH_STENCIL_VIEW_DESC*)pDesc, handle);
		ASSERT(SUCCEEDED(CDeviceManager::GetDevice()->GetDeviceRemovedReason()));
		index = ms_nGlobalDSVDescriptorHeapOffset;
		ms_nGlobalDSVDescriptorHeapOffset++;
		break;

	default:
		AssertNotReached();
		break;
	}

	ms_pViewDescriptorLock->Release();

	return index + 1;  // Convert address into ID (IDs begin at 1 and addresses at 0)
}


void CResourceManager::ResetMaxConstantBufferVersions()
{
	ms_nMaxConstantBufferVersions = CONSTANT_BUFFER_MAX_VERSIONS_PER_FRAME;
}


unsigned int CResourceManager::CreateConstantBuffer(size_t uSize, bool bOncePerFrame)
{
	HRESULT hr = E_FAIL;
	SConstantBuffer constantBuffer[CDeviceManager::m_FrameCount];

	UINT constantBufferSize = (uSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
	UINT numVersions = bOncePerFrame ? 1 : ms_nMaxConstantBufferVersions;

	unsigned int nConstantBufferID = static_cast<unsigned int>(ms_pConstantBuffers[0].size());

	for (int i = 0; i < CDeviceManager::m_FrameCount; i++)
	{
		const CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize * numVersions);
		hr = CDeviceManager::GetDevice()->CreateCommittedResource(
			&properties,
			D3D12_HEAP_FLAG_NONE,
			&buffer,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS((ID3D12Resource**)(&constantBuffer[i].m_pUploadHeap))
		);

		UINT NumThreads = CRenderWorkerThread::GetNumThreads();

		constantBuffer[i].m_MemOffset.resize(NumThreads);
		for (UINT j = 0; j < NumThreads; j++)
			constantBuffer[i].m_MemOffset[j] = 0;

		constantBuffer[i].m_Size = constantBufferSize;
		constantBuffer[i].m_HeapSize = constantBufferSize * numVersions;
		constantBuffer[i].m_HeapTop = 0;
		constantBuffer[i].m_UsedSize = 0;
		constantBuffer[i].m_CurrentFrameIndex = -1;

		if (bOncePerFrame)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
			desc.BufferLocation = ((ID3D12Resource*)constantBuffer[i].m_pUploadHeap)->GetGPUVirtualAddress();
			desc.SizeInBytes = constantBufferSize;

			constantBuffer[i].m_nCBV = CreateViewDescriptor(e_ConstantBufferView, nullptr, &desc);
		}

		else
			constantBuffer[i].m_nCBV = 0;

		CD3DX12_RANGE readRange(0, 0);
		((ID3D12Resource*)constantBuffer[i].m_pUploadHeap)->Map(0, &readRange, &(constantBuffer[i].m_pCPUMem));

		constantBuffer[i].m_pLock = CMutex::Create();

		ms_pConstantBuffers[i].push_back(constantBuffer[i]);
	}

	return nConstantBufferID + 1;
}


void CResourceManager::UpdateConstantBuffer(unsigned int nConstantBufferID, void* pData, unsigned int nSize)
{
	UINT nID = nConstantBufferID - 1;

	bool bVersioning = ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_nCBV == 0;

	char* ptr = (char*)ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_pCPUMem;

	if (bVersioning)
	{
		unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

		ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_pLock->Take();

		if (ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_CurrentFrameIndex != CRasterizer::Current->CurrentFrame)
		{
			ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_CurrentFrameIndex = CRasterizer::Current->CurrentFrame;
			ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_UsedSize = 0;
		}

		ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_MemOffset[nThreadID] = ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_HeapTop;

		UINT newOffset = ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_HeapTop + ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_Size;

		ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_UsedSize += ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_Size;

		if (ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_UsedSize > ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_HeapSize)
		{
			AssertNotReached(); // This is not good! This means we've underestimated the heap size we needed, and some draw calls will not receive the intended data. Consider increasing CONSTANT_BUFFER_MAX_VERSIONS_PER_FRAME, which is one way to mitigate this.
		}

		// Ring buffer
		if (newOffset + ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_Size > ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_HeapSize)
			newOffset = 0;

		ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_HeapTop = newOffset;

		ptr += ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_MemOffset[nThreadID];

		ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_pLock->Release();
	}

	memcpy(ptr, pData, nSize);
}


void CResourceManager::SetConstantBuffer(unsigned int nConstantBufferID, unsigned int nSlot)
{
	UINT nID = nConstantBufferID - 1;

	if (ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_nCBV == 0)
	{
		unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

		UINT8* baseAddr = (UINT8*)((ID3D12Resource*)ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_pUploadHeap)->GetGPUVirtualAddress();

		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.BufferLocation = (D3D12_GPU_VIRTUAL_ADDRESS)(baseAddr + ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_MemOffset[nThreadID]);
		desc.SizeInBytes = ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_Size;

		unsigned int nCommandListID = CRenderWorkerThread::GetCurrentThreadCommandListID();
		ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nCommandListID);

		if (CPipelineManager::GetCurrentPipelineType() == CPipelineManager::e_GraphicsPipeline)
			pCommandList->SetGraphicsRootConstantBufferView(nSlot, (D3D12_GPU_VIRTUAL_ADDRESS)(baseAddr + ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_MemOffset[nThreadID]));

		else
			pCommandList->SetComputeRootConstantBufferView(nSlot, (D3D12_GPU_VIRTUAL_ADDRESS)(baseAddr + ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_MemOffset[nThreadID]));
	}

	else
	{
		SetCBV(ms_pConstantBuffers[CDeviceManager::m_FrameIndex][nID].m_nCBV, nSlot);
	}
}


void CResourceManager::SConstantBuffer::Release()
{
	if (m_pUploadHeap)
	{
		CD3DX12_RANGE readRange(0, 0);
		((ID3D12Resource*)m_pUploadHeap)->Unmap(0, &readRange);

		((ID3D12Resource*)m_pUploadHeap)->Release();

		m_pUploadHeap = nullptr;
	}

	if (m_pLock)
	{
		delete m_pLock;

		m_pLock = nullptr;
	}
}


void CResourceManager::RegisterUploadHeap(void* pResource)
{
	UINT nCommandListID = CCommandListManager::GetCurrentThreadCommandListID();
	UINT threadID = CCommandListManager::GetCommandList(nCommandListID)->m_nWorkerThreadID;

	ms_pUploadLock->Take();
	ms_pUploadResources[threadID].push_back(pResource);
	ms_pUploadLock->Release();
}


void CResourceManager::UploadTexture(void* pTextureHandle, void* pData, unsigned int rowPitch, unsigned int slicePitch)
{
	ID3D12Resource* pUploadHeap;

	UINT64 RequiredSize = 0;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts;
	UINT NumRows;
	UINT64 RowSizesInBytes;

	auto Desc = ((ID3D12Resource*)pTextureHandle)->GetDesc();
	ID3D12Device* pDevice = nullptr;
	((ID3D12Resource*)pTextureHandle)->GetDevice(__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Layouts, &NumRows, &RowSizesInBytes, &RequiredSize);
	pDevice->Release();

	const CD3DX12_HEAP_PROPERTIES properties(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(RequiredSize);
	HRESULT hr = CDeviceManager::GetDevice()->CreateCommittedResource(&properties,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pUploadHeap)
	);

	ASSERT(SUCCEEDED(hr));

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = pData;
	textureData.RowPitch = rowPitch;
	textureData.SlicePitch = slicePitch;

	UINT nCommandListID = CCommandListManager::GetCurrentThreadCommandListID();
	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	if (!pCommandList)
	{
		nCommandListID = CCommandListManager::CreateCommandList(CCommandListManager::e_Loading);
		CCommandListManager::BeginRecording(nCommandListID, 0);
		pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	}

	UINT threadID = CCommandListManager::GetCommandList(nCommandListID)->m_nWorkerThreadID;

	UpdateSubresources(pCommandList, (ID3D12Resource*)pTextureHandle, pUploadHeap, 0, 0, 1, &textureData);
	const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition((ID3D12Resource*)pTextureHandle, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	pCommandList->ResourceBarrier(1, &barrier);

	ms_pUploadLock->Take();
	ms_pUploadResources[threadID].push_back(pUploadHeap);
	ms_pResources.push_back(pTextureHandle);
	ms_pUploadLock->Release();
}


void CResourceManager::UploadTexture(void* pTextureHandle, void* pIntermediateTextureHandle, void* pData, unsigned int rowPitch, unsigned int slicePitch)
{
	ID3D12Resource* pUploadHeap = (ID3D12Resource*)pIntermediateTextureHandle;

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = pData;
	textureData.RowPitch = rowPitch;
	textureData.SlicePitch = slicePitch;

	ID3D12GraphicsCommandList* pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	if (!pCommandList)
	{
		unsigned int nID = CCommandListManager::CreateCommandList(CCommandListManager::e_Loading);
		CCommandListManager::BeginRecording(nID, 0);
		pCommandList = (ID3D12GraphicsCommandList*)CCommandListManager::GetCurrentThreadCommandListPtr();
	}

	UpdateSubresources(pCommandList, (ID3D12Resource*)pTextureHandle, pUploadHeap, 0, 0, 1, &textureData);
}


void CResourceManager::CleanUploadHeaps(unsigned int threadID)
{
	std::vector<void*>::iterator it;

	for (it = ms_pUploadResources[threadID].begin(); it < ms_pUploadResources[threadID].end(); it++)
		if (*it != nullptr)
			((ID3D12Resource*)(*it))->Release();

	ms_pUploadResources[threadID].clear();
}



void CResourceManager::CreateStaticSampler(void* pDesc, unsigned int eSampler)
{
	D3D12_STATIC_SAMPLER_DESC* desc = (D3D12_STATIC_SAMPLER_DESC*)pDesc;
	ZeroMemory(desc, sizeof(D3D12_STATIC_SAMPLER_DESC));

	float LODBias = -0.5f;

	if (CRenderManager::ms_bCheckerboardRendering)
		LODBias -= 0.5f;

	switch (eSampler)
	{
	case ESamplerStateID::e_MinMagMip_Point__UVW_Clamp:
		desc->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_MinMagMip_Point__UVW_Wrap:
		desc->Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_MinMagMip_Linear__UVW_Wrap:
		desc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_MinMagMip_Linear__UVW_Clamp:
		desc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_MinMag_Linear_Mip_Point__UVW_Mirror:
		desc->Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;

	case ESamplerStateID::e_MinMag_Linear_Mip_Point__UVW_Clamp:
		desc->Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;

	case ESamplerStateID::e_Shadow_PCF_Filter_Compare_Z:
		desc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_Shadow_PCF_Filter_Compare_Z_Point:
		desc->Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_Anisotropic_UVW_Clamp:
		desc->Filter = D3D12_FILTER_ANISOTROPIC;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	case ESamplerStateID::e_Anisotropic_UVW_Wrap:
		desc->Filter = D3D12_FILTER_ANISOTROPIC;
		desc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc->MipLODBias = LODBias;
		desc->MaxAnisotropy = 16;
		desc->MinLOD = 0.f;
		desc->MaxLOD = 1000.f;
		break;

	default:
		AssertNotReached();
		break;
	}
}



void CResourceManager::CreateSamplers()
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle;

	float LODBias = -0.5f;

	if (CRenderManager::ms_bCheckerboardRendering)
		LODBias -= 0.5f;

	// e_MinMagMip_Point__UVW_Clamp
	D3D12_SAMPLER_DESC desc = {};
	desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.MipLODBias = LODBias;
	desc.MaxAnisotropy = 16;
	desc.MinLOD = 0.f;
	desc.MaxLOD = 1000.f;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMagMip_Point__UVW_Clamp, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_MinMagMip_Point__UVW_Wrap
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMagMip_Point__UVW_Wrap, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_MinMagMip_Linear__UVW_Wrap
	desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMagMip_Linear__UVW_Wrap, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_MinMagMip_Linear__UVW_Clamp
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMagMip_Linear__UVW_Clamp, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_MinMag_Linear_Mip_Point__UVW_Clamp
	desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMag_Linear_Mip_Point__UVW_Clamp, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_MinMag_Linear_Mip_Point__UVW_Mirror
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_MinMag_Linear_Mip_Point__UVW_Mirror, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_Shadow_PCF_Filter_Compare_Z
	desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	desc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_Shadow_PCF_Filter_Compare_Z, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_Shadow_PCF_Filter_Compare_Z_Point
	desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_Shadow_PCF_Filter_Compare_Z_Point, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_Anisotropic_UVW_Clamp
	desc.Filter = D3D12_FILTER_ANISOTROPIC;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_Anisotropic_UVW_Clamp, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);


	//e_Anisotropic_UVW_Wrap
	desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	GetCPUDescriptorHandle(&handle, (ID3D12DescriptorHeap*)ms_pGlobalSamplerDescriptorHeap, ESamplerStateID::e_Anisotropic_UVW_Wrap, ms_nDescriptorSizeSampler);
	CDeviceManager::GetDevice()->CreateSampler(&desc, handle);
}
