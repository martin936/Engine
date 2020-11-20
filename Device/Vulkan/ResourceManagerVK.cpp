#include "../DeviceManager.h"
#include "../ResourceManager.h"
#include "../CommandListManager.h"
#include "../RenderThreads.h"
#include "../PipelineManager.h"



#define PERMANENT_CONSTANT_BUFFER_POOL_SIZE		50 * 1024 * 1024
#define FRAME_CONSTANT_BUFFER_POOL_SIZE			2 * 1024 * 1024
#define CONSTANT_BUFFER_POOL_SIZE				100 * 1024 * 1024

unsigned int		CResourceManager::ms_CurrentBuffer = 0;

BufferId			CResourceManager::ms_pPermanentConstantBuffers;
BufferId			CResourceManager::ms_pFrameConstantBuffers[ms_NumBuffers];
BufferId			CResourceManager::ms_pConstantBuffers[ms_NumBuffers];

void*				CResourceManager::ms_pLocalSRVDescriptorHeap[CDeviceManager::ms_FrameCount];

CMutex*				CResourceManager::ms_pBufferCreationLock;
CMutex*				CResourceManager::ms_pConstantBufferCreationLock;

void*				CResourceManager::ms_pSamplers[ESamplerState::e_NbSamplers];
void*				CResourceManager::ms_pMappedConstantBuffers;

size_t				CResourceManager::ms_nPermanentConstantBufferOffset;
size_t				CResourceManager::ms_nFrameConstantBufferOffset[ms_NumBuffers];
size_t				CResourceManager::ms_nConstantBufferOffset[ms_NumBuffers];

size_t				CResourceManager::ms_nMinConstantBufferAlignment;
size_t				CResourceManager::ms_nMinConstantBufferOffsetAlignment;
size_t				CResourceManager::ms_nMinMemoryAlignment;

std::vector<CResourceManager::SBuffer>	CResourceManager::ms_pBuffers;
std::vector<CResourceManager::SFence>	CResourceManager::ms_pFences;


uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(CDeviceManager::GetPhysicalDevice(), &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	return 0;
}


void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(CDeviceManager::GetDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(CDeviceManager::GetDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	VkResult res = vkAllocateMemory(CDeviceManager::GetDevice(), &allocInfo, nullptr, &bufferMemory);
	ASSERT(res == VK_SUCCESS);

	vkBindBufferMemory(CDeviceManager::GetDevice(), buffer, bufferMemory, 0);
}



void clearBuffer(VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset, uint32_t data)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	vkCmdFillBuffer(commandBuffer, buffer, offset, size, data);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}



void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, size_t size)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}


BufferId CResourceManager::CreateBuffer(void* buffer, void* memory, size_t size, size_t align, size_t byteOffset)
{
	SBuffer buf = { buffer, memory, size, align, byteOffset, 0 };

	ms_pBufferCreationLock->Take();

	buf.m_nId = static_cast<unsigned int>(ms_pBuffers.size());
	ms_pBuffers.push_back(buf);

	ms_pBufferCreationLock->Release();

	return buf.m_nId;
}


void* CResourceManager::GetBufferHandle(BufferId buffer)
{
	ASSERT(buffer >= 0 && buffer < ms_pBuffers.size());

	return ms_pBuffers[buffer].m_pBuffer;
}


size_t CResourceManager::GetBufferOffset(BufferId buffer)
{
	ASSERT(buffer >= 0 && buffer < ms_pBuffers.size());

	return ms_pBuffers[buffer].m_nByteOffset;
}


size_t CResourceManager::GetBufferSize(BufferId buffer)
{
	ASSERT(buffer >= 0 && buffer < ms_pBuffers.size());

	return ms_pBuffers[buffer].m_nSize;
}


void CResourceManager::CreateLocalShaderResourceHeap()
{
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * lengthof(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)lengthof(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	for (int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		VkResult res = vkCreateDescriptorPool(CDeviceManager::GetDevice(), &pool_info, nullptr, (VkDescriptorPool*)&ms_pLocalSRVDescriptorHeap[i]);
		ASSERT(res == VK_SUCCESS);
	}
}



void CResourceManager::CreatePermanentConstantBufferPool()
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;
	createBuffer((PERMANENT_CONSTANT_BUFFER_POOL_SIZE + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pBuffer, bufferMemory);

	ms_pPermanentConstantBuffers = CreateBuffer(pBuffer, bufferMemory, PERMANENT_CONSTANT_BUFFER_POOL_SIZE, ms_nMinConstantBufferAlignment, 0);
}


void CResourceManager::CreateFrameConstantBufferPool()
{
	for (int i = 0; i < ms_NumBuffers; i++)
	{
		VkBuffer pBuffer;
		VkDeviceMemory bufferMemory;
		createBuffer((FRAME_CONSTANT_BUFFER_POOL_SIZE + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pBuffer, bufferMemory);

		ms_pFrameConstantBuffers[i] = CreateBuffer(pBuffer, bufferMemory, FRAME_CONSTANT_BUFFER_POOL_SIZE, ms_nMinConstantBufferAlignment, 0);
	}
}


void CResourceManager::CreateConstantBufferPool()
{
	for (int i = 0; i < ms_NumBuffers; i++)
	{
		VkBuffer pBuffer;
		VkDeviceMemory bufferMemory;
		createBuffer((CONSTANT_BUFFER_POOL_SIZE + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pBuffer, bufferMemory);

		ms_pConstantBuffers[i] = CreateBuffer(pBuffer, bufferMemory, CONSTANT_BUFFER_POOL_SIZE, ms_nMinConstantBufferAlignment, 0);
	}
}



void CResourceManager::Init()
{
	ms_pBufferCreationLock = CMutex::Create();
	ms_pConstantBufferCreationLock = CMutex::Create();

	VkPhysicalDeviceProperties prop;
	vkGetPhysicalDeviceProperties(CDeviceManager::GetPhysicalDevice(), &prop);

	ms_nMinConstantBufferAlignment			= prop.limits.minUniformBufferOffsetAlignment;
	ms_nMinConstantBufferAlignment			= prop.limits.minMemoryMapAlignment;
	ms_nMinConstantBufferOffsetAlignment	= 0x100;

	CreateLocalShaderResourceHeap();

	CreatePermanentConstantBufferPool();
	CreateFrameConstantBufferPool();
	CreateConstantBufferPool();

	CreateSamplers();
}


void CResourceManager::Terminate()
{
	for (int i = 0; i < e_NbSamplers; i++)
		vkDestroySampler(CDeviceManager::GetDevice(), (VkSampler)ms_pSamplers[i], nullptr);

	DestroyBuffers();
	DestroyFences();

	for (int i = 0; i < CDeviceManager::ms_FrameCount; i++)
		vkDestroyDescriptorPool(CDeviceManager::GetDevice(), (VkDescriptorPool)ms_pLocalSRVDescriptorHeap[i], nullptr);

	delete ms_pBufferCreationLock;
	delete ms_pConstantBufferCreationLock;
}


void CResourceManager::DestroyBuffers()
{
	int numBuffers = static_cast<int>(ms_pBuffers.size());

	for (int i = 0; i < numBuffers; i++)
		if (ms_pBuffers[i].m_pMemoryHandle != nullptr)
		{
			vkFreeMemory(CDeviceManager::GetDevice(), (VkDeviceMemory)ms_pBuffers[i].m_pMemoryHandle, nullptr);
			vkDestroyBuffer(CDeviceManager::GetDevice(), (VkBuffer)ms_pBuffers[i].m_pBuffer, nullptr);
		}

	ms_pBuffers.clear();
}


void CResourceManager::DestroyFences()
{
	int numFences = static_cast<int>(ms_pFences.size());

	for (int i = 0; i < numFences; i++)
		vkDestroyFence(CDeviceManager::GetDevice(), (VkFence)ms_pFences[i].m_Fence, nullptr);

	ms_pFences.clear();
}


void CResourceManager::BeginFrame()
{
	ms_CurrentBuffer = (ms_CurrentBuffer + 1) % ms_NumBuffers;

	vkMapMemory(CDeviceManager::GetDevice(), (VkDeviceMemory)ms_pBuffers[ms_pConstantBuffers[ms_CurrentBuffer]].m_pMemoryHandle, 0, CONSTANT_BUFFER_POOL_SIZE, 0, &ms_pMappedConstantBuffers);

	ms_nConstantBufferOffset[ms_CurrentBuffer] = 0;
	ms_nFrameConstantBufferOffset[ms_CurrentBuffer] = 0;
}


void CResourceManager::EndFrame()
{
	vkUnmapMemory(CDeviceManager::GetDevice(), (VkDeviceMemory)ms_pBuffers[ms_pConstantBuffers[ms_CurrentBuffer]].m_pMemoryHandle);
}


void		CResourceManager::UploadBuffer(BufferId bufferId, void* pData)
{
	SBuffer buffer = ms_pBuffers[bufferId];

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer((buffer.m_nSize + MAX(1, buffer.m_nAlign) - 1) & ~buffer.m_nAlign, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, buffer.m_nSize, 0, &data);
	memcpy(data, pData, buffer.m_nSize);
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	copyBuffer(reinterpret_cast<VkBuffer>(buffer.m_pBuffer), stagingBuffer, buffer.m_nSize);

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}



FenceId		CResourceManager::CreateFence()
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	VkFence fence;

	VkResult res = vkCreateFence(CDeviceManager::GetDevice(), &fenceInfo, nullptr, &fence);
	ASSERT(res == VK_SUCCESS);

	ms_pBufferCreationLock->Take();

	FenceId ID = static_cast<FenceId>(ms_pFences.size());
	ms_pFences.push_back({ fence });

	ms_pBufferCreationLock->Release();

	return ID;
}


void		CResourceManager::SubmitFence(FenceId fence)
{
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	VkResult res = vkQueueSubmit((VkQueue)CCommandListManager::GetCommandQueuePtr(CCommandListManager::e_Queue_Direct), 1, &submitInfo, (VkFence)ms_pFences[fence].m_Fence);
	ASSERT(res == VK_SUCCESS);
}



bool		CResourceManager::WaitForFence(FenceId fence, uint64_t nanoseconds)
{
	VkResult res = vkWaitForFences(CDeviceManager::GetDevice(), 1, (VkFence*)&ms_pFences[fence].m_Fence, VK_TRUE, nanoseconds);

	if (res == VK_SUCCESS)
	{
		VkResult reset = vkResetFences(CDeviceManager::GetDevice(), 1, (VkFence*)&ms_pFences[fence].m_Fence);
		ASSERT(reset == VK_SUCCESS);
	}

	return (res == VK_SUCCESS);
}



BufferId	CResourceManager::CreateVertexBuffer(size_t size, void* pData)
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;

	createBuffer((size + MAX(1, ms_nMinMemoryAlignment) - 1) & ~(MAX(1, ms_nMinMemoryAlignment) - 1), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pBuffer, bufferMemory);

	BufferId bufferId = CreateBuffer(pBuffer, bufferMemory, size, ms_nMinMemoryAlignment, 0);

	if (pData != nullptr)
		UploadBuffer(bufferId, pData);

	return bufferId;
}



BufferId	CResourceManager::CreateRwBuffer(size_t size, bool bReadback, bool bClear)
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;

	VkBufferUsageFlags flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	if (bReadback)
		memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

	if (bClear)
		flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	createBuffer((size + MAX(1, ms_nMinMemoryAlignment) - 1) & ~(MAX(1, ms_nMinMemoryAlignment) - 1), flags, memFlags, pBuffer, bufferMemory);

	BufferId bufferId = CreateBuffer(pBuffer, bufferMemory, size, ms_nMinMemoryAlignment, 0);

	if (bClear)
		clearBuffer(pBuffer, size, 0, 0);

	return bufferId;
}



BufferId	CResourceManager::CreateVertexBuffer(BufferId bufferId, size_t byteOffset)
{
	return CreateBuffer(ms_pBuffers[bufferId].m_pBuffer, nullptr, ms_pBuffers[bufferId].m_nSize, ms_pBuffers[bufferId].m_nAlign, byteOffset);
}



BufferId	CResourceManager::CreateIndexBuffer(size_t size, void* pData)
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;

	createBuffer((size + MAX(1, ms_nMinMemoryAlignment) - 1) & ~(MAX(1, ms_nMinMemoryAlignment) - 1), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, pBuffer, bufferMemory);

	BufferId bufferId = CreateBuffer(pBuffer, bufferMemory, size, ms_nMinMemoryAlignment, 0);

	if (pData != nullptr)
		UploadBuffer(bufferId, pData);

	return bufferId;
}



BufferId	CResourceManager::CreateMappableVertexBuffer(size_t size, void* pData)
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;
	createBuffer((size + MAX(1, ms_nMinMemoryAlignment) - 1) & ~(MAX(1, ms_nMinMemoryAlignment) - 1), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pBuffer, bufferMemory);

	BufferId bufferId = CreateBuffer(pBuffer, bufferMemory, size, ms_nMinMemoryAlignment, 0);

	if (pData != nullptr)
		UploadBuffer(bufferId, pData);

	return bufferId;
}



BufferId	CResourceManager::CreateMappableIndexBuffer(size_t size, void* pData)
{
	VkBuffer pBuffer;
	VkDeviceMemory bufferMemory;
	createBuffer((size + MAX(1, ms_nMinMemoryAlignment) - 1) & ~(MAX(1, ms_nMinMemoryAlignment) - 1), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, pBuffer, bufferMemory);

	BufferId bufferId = CreateBuffer(pBuffer, bufferMemory, size, ms_nMinMemoryAlignment, 0);

	if (pData != nullptr)
		UploadBuffer(bufferId, pData);

	return bufferId;
}


void* CResourceManager::MapBuffer(BufferId bufferId)
{
	void* pData;
	VkResult res = vkMapMemory(CDeviceManager::GetDevice(), (VkDeviceMemory)ms_pBuffers[bufferId].m_pMemoryHandle, ms_pBuffers[bufferId].m_nByteOffset, ms_pBuffers[bufferId].m_nSize, 0, &pData);
	ASSERT(res == VK_SUCCESS);

	return pData;
}


void CResourceManager::UnmapBuffer(BufferId bufferId)
{
	vkUnmapMemory(CDeviceManager::GetDevice(), (VkDeviceMemory)ms_pBuffers[bufferId].m_pMemoryHandle);
}


BufferId	CResourceManager::CreatePermanentConstantBuffer(void* pData, size_t size)
{
	size_t byteOffset;

	ms_pConstantBufferCreationLock->Take();

	byteOffset = ms_nPermanentConstantBufferOffset;
	ms_nPermanentConstantBufferOffset += (size + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);

	ms_pConstantBufferCreationLock->Release();

	BufferId buffer = CreateBuffer(ms_pBuffers[ms_pPermanentConstantBuffers].m_pBuffer, nullptr, size, ms_nMinConstantBufferAlignment, byteOffset);

	UpdateConstantBuffer(buffer, pData);

	return buffer;
}


BufferId	CResourceManager::CreateFrameConstantBuffer(void* pData, size_t size)
{
	size_t byteOffset;

	ms_pConstantBufferCreationLock->Take();

	byteOffset = (ms_nFrameConstantBufferOffset[ms_CurrentBuffer] + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);
	ms_nFrameConstantBufferOffset[ms_CurrentBuffer] += (size + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1);

	ms_pConstantBufferCreationLock->Release();

	BufferId buffer = CreateBuffer(ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pBuffer, nullptr, size, ms_nMinConstantBufferAlignment, byteOffset);

	UpdateConstantBuffer(buffer, pData);

	return buffer;
}


void CResourceManager::UpdateFrameConstantBuffer(BufferId buffer, void* pData)
{
	size_t byteOffset;
	size_t size = ms_pBuffers[buffer].m_nSize;

	ms_pConstantBufferCreationLock->Take();

	byteOffset = (ms_nFrameConstantBufferOffset[ms_CurrentBuffer] + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);
	ms_nFrameConstantBufferOffset[ms_CurrentBuffer] += (size + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1);

	ms_pConstantBufferCreationLock->Release();

	ms_pBuffers[buffer].m_pBuffer		= ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pBuffer;
	ms_pBuffers[buffer].m_nByteOffset	= byteOffset;

	UpdateConstantBuffer(buffer, pData);
}


void CResourceManager::UpdateFrameConstantBuffer(BufferId buffer, void* pData, size_t size)
{
	size_t byteOffset;

	ms_pConstantBufferCreationLock->Take();

	byteOffset = (ms_nFrameConstantBufferOffset[ms_CurrentBuffer] + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);
	ms_nFrameConstantBufferOffset[ms_CurrentBuffer] += (size + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1);

	ms_pConstantBufferCreationLock->Release();

	ms_pBuffers[buffer].m_pBuffer		= ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pBuffer;
	ms_pBuffers[buffer].m_nByteOffset	= byteOffset;
	ms_pBuffers[buffer].m_nSize			= size;

	UpdateConstantBuffer(buffer, pData);
}


void CResourceManager::UpdateConstantBuffer(BufferId bufferId, void* pData, size_t nSize, size_t nByteOffset)
{
	SBuffer& buffer = ms_pBuffers[bufferId];
	VkDeviceMemory memoryHandle = nullptr;

	if (buffer.m_pBuffer == ms_pBuffers[ms_pPermanentConstantBuffers].m_pBuffer)
		memoryHandle = (VkDeviceMemory)ms_pBuffers[ms_pPermanentConstantBuffers].m_pMemoryHandle;

	else if (buffer.m_pBuffer == ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pBuffer)
		memoryHandle = (VkDeviceMemory)ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pMemoryHandle;

	else
		AssertNotReached();

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), memoryHandle, buffer.m_nByteOffset, buffer.m_nSize, 0, &data);
	memcpy((char*)data + nByteOffset, pData, nSize);
	vkUnmapMemory(CDeviceManager::GetDevice(), memoryHandle);
}


void CResourceManager::UpdateConstantBuffer(BufferId bufferId, void* pData)
{
	SBuffer& buffer = ms_pBuffers[bufferId];
	VkDeviceMemory memoryHandle = nullptr;

	if (buffer.m_pBuffer == ms_pBuffers[ms_pPermanentConstantBuffers].m_pBuffer)
		memoryHandle = (VkDeviceMemory)ms_pBuffers[ms_pPermanentConstantBuffers].m_pMemoryHandle;

	else if (buffer.m_pBuffer == ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pBuffer)
		memoryHandle = (VkDeviceMemory)ms_pBuffers[ms_pFrameConstantBuffers[ms_CurrentBuffer]].m_pMemoryHandle;

	else
		AssertNotReached();

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), memoryHandle, buffer.m_nByteOffset, buffer.m_nSize, 0, &data);
	memcpy(data, pData, buffer.m_nSize);
	vkUnmapMemory(CDeviceManager::GetDevice(), memoryHandle);
}


void CResourceManager::SetConstantBuffer(unsigned int nSlot, BufferId bufferId)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer	= (VkBuffer)ms_pBuffers[bufferId].m_pBuffer;
	bufferInfo.offset	= ms_pBuffers[bufferId].m_nByteOffset;
	bufferInfo.range	= ms_pBuffers[bufferId].m_nSize;

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement	= 0;
	desc.dstBinding			= nSlot;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc.descriptorCount	= 1;
	desc.pBufferInfo		= &bufferInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetConstantBuffer(unsigned int nSlot, BufferId bufferId, size_t range)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer	= (VkBuffer)ms_pBuffers[bufferId].m_pBuffer;
	bufferInfo.offset	= ms_pBuffers[bufferId].m_nByteOffset;
	bufferInfo.range	= range;

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement	= 0;
	desc.dstBinding			= nSlot;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc.descriptorCount	= 1;
	desc.pBufferInfo		= &bufferInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetConstantBufferOffset(unsigned int nSlot, size_t byteOffset)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	int index = 0;
	int numBuffers = static_cast<int>(pipeline->m_nDynamicBufferBinding.size());

	for (index = 0; index < numBuffers; index++)
		if (pipeline->m_nDynamicBufferBinding[index] == nSlot)
			break;

	ASSERT(index < numBuffers);

	pipeline->m_nDynamicOffsets[index] = (unsigned int)byteOffset;
}


void CResourceManager::SetConstantBuffer(unsigned int nSlot, void* pData, size_t nSize)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorSet descriptorSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);

	ASSERT(descriptorSet != nullptr);

	size_t byteOffset;

	ms_pConstantBufferCreationLock->Take();

	byteOffset = (ms_nConstantBufferOffset[ms_CurrentBuffer] + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);
	ms_nConstantBufferOffset[ms_CurrentBuffer] += (nSize + MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferOffsetAlignment) - 1);

	ms_pConstantBufferCreationLock->Release();

	memcpy((char*)ms_pMappedConstantBuffers + byteOffset, pData, nSize);

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer	= (VkBuffer)ms_pBuffers[ms_pConstantBuffers[ms_CurrentBuffer]].m_pBuffer;
	bufferInfo.offset	= byteOffset;
	bufferInfo.range	= (nSize + MAX(1, ms_nMinConstantBufferAlignment) - 1) & ~(MAX(1, ms_nMinConstantBufferAlignment) - 1);

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= descriptorSet;
	desc.dstBinding			= nSlot;
	desc.dstArrayElement	= 0;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	desc.descriptorCount	= 1;
	desc.pBufferInfo		= &bufferInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetTexture(unsigned int nSlot, void* pTexture)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView		= (VkImageView)pTexture;

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement	= 0;
	desc.dstBinding			= nSlot;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	desc.descriptorCount	= 1;
	desc.pImageInfo			= &imageInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetRwTexture(unsigned int nSlot, void* pTexture)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	imageInfo.imageView = (VkImageView)pTexture;

	VkWriteDescriptorSet desc{};
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement = 0;
	desc.dstBinding = nSlot;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	desc.descriptorCount = 1;
	desc.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetTextures(unsigned int nSlot, std::vector<void*>& pTextures)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	int numTextures = static_cast<int>(pTextures.size());

	std::vector<VkDescriptorImageInfo> imageInfo(pTextures.size());

	for (int i = 0; i < numTextures; i++)
	{
		imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[i].imageView = (VkImageView)pTextures[i];
	}

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement	= 0;
	desc.dstBinding			= nSlot;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	desc.descriptorCount	= numTextures;
	desc.pImageInfo			= imageInfo.data();

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetTextures(unsigned int nSlot, std::vector<CTexture*>& pTextures)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	int numTextures = static_cast<int>(pTextures.size());

	std::vector<VkDescriptorImageInfo> imageInfo(1024);

	for (int i = 0; i < numTextures; i++)
	{
		imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[i].imageView = pTextures[i]->GetImageView();
	}

	for (int i = numTextures; i < 1024; i++)
	{
		imageInfo[i].imageLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo[i].imageView		= VK_NULL_HANDLE;
	}

	VkWriteDescriptorSet desc{};
	desc.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet				= (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement	= 0;
	desc.dstBinding			= nSlot;
	desc.descriptorType		= VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	desc.descriptorCount	= 1024;
	desc.pImageInfo			= imageInfo.data();

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetBuffer(unsigned int nSlot, BufferId bufferId)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = (VkBuffer)ms_pBuffers[bufferId].m_pBuffer;
	bufferInfo.offset = ms_pBuffers[bufferId].m_nByteOffset;
	bufferInfo.range = ms_pBuffers[bufferId].m_nSize;

	VkWriteDescriptorSet desc{};
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement = 0;
	desc.dstBinding = nSlot;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc.descriptorCount = 1;
	desc.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetRwBuffer(unsigned int nSlot, BufferId bufferId)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = (VkBuffer)ms_pBuffers[bufferId].m_pBuffer;
	bufferInfo.offset = ms_pBuffers[bufferId].m_nByteOffset;
	bufferInfo.range = ms_pBuffers[bufferId].m_nSize;

	VkWriteDescriptorSet desc{};
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement = 0;
	desc.dstBinding = nSlot;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	desc.descriptorCount = 1;
	desc.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetSampler(unsigned int nSlot, ESamplerState eSamplerID)
{
	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkDescriptorImageInfo imageInfo{};
	imageInfo.sampler = (VkSampler)ms_pSamplers[eSamplerID];

	VkWriteDescriptorSet desc{};
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = (VkDescriptorSet)(pipeline->m_pDescriptorSets[CDeviceManager::GetFrameIndex()]);
	desc.dstArrayElement = 0;
	desc.dstBinding = nSlot;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
	desc.descriptorCount = 1;
	desc.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(CDeviceManager::GetDevice(), 1, &desc, 0, nullptr);
}


void CResourceManager::SetPushConstant(unsigned int shaderStage, void* pData, size_t size)
{
	VkShaderStageFlags stageFlags = 0;

	if (shaderStage & CShader::e_ComputeShader)
		stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

	if (shaderStage & CShader::e_VertexShader)
		stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;

	if (shaderStage & CShader::e_HullShader)
		stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

	if (shaderStage & CShader::e_DomainShader)
		stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	if (shaderStage & CShader::e_GeometryShader)
		stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

	if (shaderStage & CShader::e_FragmentShader)
		stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

	CRenderPass* pRenderPass = CFrameBlueprint::GetRunningRenderPass();
	CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pRenderPass->GetPipeline());

	VkCommandBuffer cmdBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdPushConstants(cmdBuffer, (VkPipelineLayout)pipeline->m_pRootSignature, stageFlags, 0, static_cast<uint32_t>(size), pData);
}


void CResourceManager::CreateSamplers()
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType				= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter			= VK_FILTER_NEAREST;
	samplerInfo.minFilter			= VK_FILTER_NEAREST;
	samplerInfo.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.mipLodBias			= -0.5f;
	samplerInfo.anisotropyEnable	= VK_FALSE;
	samplerInfo.compareEnable		= VK_FALSE;
	samplerInfo.minLod				= 0;
	samplerInfo.maxLod				= 1000;

	VkResult res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Point_UVW_Clamp]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Point_UVW_Wrap]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Point_UVW_Mirror]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.magFilter			= VK_FILTER_LINEAR;
	samplerInfo.minFilter			= VK_FILTER_LINEAR;
	samplerInfo.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Linear_UVW_Clamp]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Linear_UVW_Wrap]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_MinMagMip_Linear_UVW_Mirror]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.magFilter			= VK_FILTER_NEAREST;
	samplerInfo.minFilter			= VK_FILTER_NEAREST;
	samplerInfo.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable	= VK_TRUE;
	samplerInfo.maxAnisotropy		= 16;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Point_UVW_Clamp]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Point_UVW_Wrap]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Point_UVW_Mirror]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.magFilter			= VK_FILTER_LINEAR;
	samplerInfo.minFilter			= VK_FILTER_LINEAR;
	samplerInfo.mipmapMode			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Linear_UVW_Clamp]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Linear_UVW_Wrap]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_Anisotropic_Linear_UVW_Mirror]);
	ASSERT(res == VK_SUCCESS);

	samplerInfo.addressModeU		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW		= VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable	= VK_FALSE;
	samplerInfo.compareEnable		= VK_TRUE;
	samplerInfo.compareOp			= VK_COMPARE_OP_GREATER;

	res = vkCreateSampler(CDeviceManager::GetDevice(), &samplerInfo, nullptr, (VkSampler*)&ms_pSamplers[e_ZComparison_Linear_UVW_Clamp]);
	ASSERT(res == VK_SUCCESS);
}
