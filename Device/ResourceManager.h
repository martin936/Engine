#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

typedef unsigned int BufferId;
typedef unsigned int FenceId;

#include "DeviceManager.h"
#include "Engine/Misc/Mutex.h"
#include "RenderThreads.h"
#include <vector>


enum ESamplerState
{
	e_MinMagMip_Point_UVW_Clamp = 0,
	e_MinMagMip_Point_UVW_Wrap,
	e_MinMagMip_Point_UVW_Mirror,

	e_MinMagMip_Linear_UVW_Clamp,
	e_MinMagMip_Linear_UVW_Wrap,
	e_MinMagMip_Linear_UVW_Mirror,

	e_Anisotropic_Point_UVW_Clamp,
	e_Anisotropic_Point_UVW_Wrap,
	e_Anisotropic_Point_UVW_Mirror,

	e_Anisotropic_Linear_UVW_Clamp,
	e_Anisotropic_Linear_UVW_Wrap,
	e_Anisotropic_Linear_UVW_Mirror,

	e_ZComparison_Linear_UVW_Clamp,

	e_NbSamplers,
};


class CResourceManager
{
public:

	static void								Init();
	static void								Terminate();

	static void								BeginFrame();
	static void								EndFrame();

	static void								DestroyBuffers();
	static void								DestroyFences();

	static BufferId							CreateRwBuffer(size_t size, bool bReadback = false, bool bClear = false);

	static BufferId							CreateVertexBuffer(size_t size, void* pData = nullptr);
	static BufferId							CreateVertexBuffer(BufferId bufferId, size_t byteOffset);

	static FenceId							CreateFence();
	static void								SubmitFence(FenceId fence);
	static bool								WaitForFence(FenceId fence, uint64_t nanoseconds);

	static BufferId							CreateIndexBuffer(size_t size, void* pData = nullptr);

	static BufferId							CreateMappableVertexBuffer(size_t size, void* pData = nullptr);
	static BufferId							CreateMappableIndexBuffer(size_t size, void* pData = nullptr);

	static void*							MapBuffer(BufferId bufferId);
	static void								UnmapBuffer(BufferId bufferId);

	static void								UploadBuffer(BufferId bufferId, void* pData);

	static void*							GetBufferHandle(BufferId buffer);
	static size_t							GetBufferOffset(BufferId buffer);
	static size_t							GetBufferSize(BufferId buffer);

	static BufferId							CreatePermanentConstantBuffer(void* pData, size_t nSize);
	static BufferId							CreateFrameConstantBuffer(void* pData, size_t nSize);
	static void								UpdateFrameConstantBuffer(BufferId buffer, void* pData);
	static void								UpdateFrameConstantBuffer(BufferId buffer, void* pData, size_t size);
	static void								UpdateConstantBuffer(BufferId bufferId, void* pData);
	static void								UpdateConstantBuffer(BufferId bufferId, void* pData, size_t nSize, size_t nByteOffset);

	static void								SetConstantBuffer(unsigned int nSlot, BufferId bufferId);
	static void								SetConstantBuffer(unsigned int nSlot, BufferId bufferId, size_t range);
	static void								SetConstantBuffer(unsigned int nSlot, void* pData, size_t nSize);

	static void								SetPushConstant(unsigned int shaderStage, void* pData, size_t size);

	static void								SetConstantBufferOffset(unsigned int nSlot, size_t byteOffset);

	static void								SetTexture(unsigned int nSlot, void* pTexture);

	static void								SetTextures(unsigned int nSlot, std::vector<void*>& pTextures);
	static void								SetTextures(unsigned int nSlot, std::vector<CTexture*>& pTextures);

	static void								SetRwTexture(unsigned int nSlot, void* pRwTexture);

	static void								SetBuffer(unsigned int nSlot, BufferId pBuffer);
	static void								SetRwBuffer(unsigned int nSlot, BufferId pRwBuffer);

	static void								SetSampler(unsigned int nSlot, ESamplerState eSamplerID);

	static void*							GetLocalSRVDecriptorHeap(int frameIndex)
	{
		return ms_pLocalSRVDescriptorHeap[frameIndex];
	}

	static size_t							GetConstantBufferOffsetAlignment()
	{
		return ms_nMinConstantBufferOffsetAlignment;
	}

private:

	static const unsigned int ms_NumBuffers = CDeviceManager::ms_FrameCount + 3;
	static unsigned int ms_CurrentBuffer;

	struct SBuffer
	{
		void*			m_pBuffer;
		void*			m_pMemoryHandle;
		size_t			m_nSize;
		size_t			m_nAlign;
		size_t			m_nByteOffset;
		unsigned int	m_nId;
	};

	struct SFence
	{
		void* m_Fence;
	};

	static BufferId							CreateBuffer(void* buffer, void* memory, size_t size, size_t align, size_t byteOffset);

	static std::vector<SBuffer>				ms_pBuffers;
	static std::vector<SFence>				ms_pFences;

	static size_t							ms_nMinConstantBufferAlignment;
	static size_t							ms_nMinConstantBufferOffsetAlignment;
	static size_t							ms_nMinMemoryAlignment;

	static void								CreateSamplers();

	static void*							ms_pSamplers[ESamplerState::e_NbSamplers];

	static void*							ms_pLocalSRVDescriptorHeap[CDeviceManager::ms_FrameCount];

	static BufferId							ms_pPermanentConstantBuffers;
	static BufferId							ms_pFrameConstantBuffers[ms_NumBuffers];
	static BufferId							ms_pConstantBuffers[ms_NumBuffers];
	static void*							ms_pMappedConstantBuffers;

	static size_t							ms_nPermanentConstantBufferOffset;
	static size_t							ms_nFrameConstantBufferOffset[ms_NumBuffers];
	static size_t							ms_nConstantBufferOffset[ms_NumBuffers];

	static CMutex*							ms_pBufferCreationLock;
	static CMutex*							ms_pConstantBufferCreationLock;

	static void								CreateLocalShaderResourceHeap();
	static void								CreatePermanentConstantBufferPool();
	static void								CreateFrameConstantBufferPool();
	static void								CreateConstantBufferPool();
};

#endif
