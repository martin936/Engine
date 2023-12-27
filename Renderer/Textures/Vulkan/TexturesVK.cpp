#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/ResourceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "../Textures.h"


extern VkFormat ConvertFormat(ETextureFormat format);
extern uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

extern void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
extern void copyBufferToTexture(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions);

CTexture::CTexture(int nWidth, int nHeight, ETextureFormat eFormat, ETextureType eType, int sampleCount, bool bGenMipMaps) : CTexture(nWidth, nHeight, 1, eFormat, eType, sampleCount, bGenMipMaps) {}


CTexture::CTexture(int nWidth, int nHeight, int nDepth, ETextureFormat eFormat, ETextureType eType, int sampleCount, bool bGenMipMaps) : CTexture()
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nDepth = 1;
	m_nArrayDim = 1;
	m_eFormat = eFormat;
	m_eType = eType;
	m_nSampleCount = sampleCount;
	m_nSampleQuality = 0;
	m_nMipMapCount = bGenMipMaps ? static_cast<int>(log2(MAX(nWidth, nHeight))) + 1 : 1;

	VkImageCreateInfo imageInfo{};
	imageInfo.sType				= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;	
	imageInfo.extent.width		= nWidth;
	imageInfo.extent.height		= nHeight;
	imageInfo.arrayLayers		= 1;
	imageInfo.format			= ConvertFormat(eFormat);
	imageInfo.usage				= VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.mipLevels			= m_nMipMapCount;
	imageInfo.samples			= (VkSampleCountFlagBits)(1 << (sampleCount - 1));
	imageInfo.tiling			= VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.format		= ConvertFormat(eFormat);
	imageViewInfo.viewType		= VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.components	= { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	if (eFormat >= e_R16_DEPTH && eFormat <= e_R32_DEPTH_G8_STENCIL)
	{
		imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		//if (eFormat == e_R24_DEPTH_G8_STENCIL || eFormat == e_R32_DEPTH_G8_STENCIL)
			//imageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
	{
		imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (eType == eTextureStorage2D || eType == eTextureStorage2DArray || eType == eTextureStorage3D)
		imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (eType == eTexture3D || eType == eTextureStorage3D)
	{
		imageInfo.imageType		= VK_IMAGE_TYPE_3D;
		imageInfo.extent.depth	= nDepth;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
		m_nDepth = nDepth;
	}

	else
	{
		imageInfo.imageType		= VK_IMAGE_TYPE_2D;		
		imageInfo.extent.depth	= 1;
		m_nDepth = 1;
	}

	if (eType == eTextureArray || eType == eTextureStorage2DArray || eType == eCubeMapArray)
	{
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageInfo.arrayLayers = (eType == eCubeMapArray) ? nDepth * 6 : nDepth;
		m_nArrayDim = nDepth;
	}

	if (eType == eCubeMap || eType == eCubeMapArray)
	{
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageViewInfo.viewType = (eType == eCubeMap) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
	}

	VkResult res = vkCreateImage(CDeviceManager::GetDevice(), &imageInfo, nullptr, &m_pImage);
	ASSERT(res == VK_SUCCESS);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(CDeviceManager::GetDevice(), m_pImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	res = vkAllocateMemory(CDeviceManager::GetDevice(), &allocInfo, nullptr, &m_pDeviceMemory);
	ASSERT(res == VK_SUCCESS);

	vkBindImageMemory(CDeviceManager::GetDevice(), m_pImage, m_pDeviceMemory, 0);

	imageViewInfo.image		= m_pImage;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.layerCount = (eType == eCubeMapArray) ? 6 * m_nArrayDim : m_nArrayDim;
	imageViewInfo.subresourceRange.levelCount = m_nMipMapCount;

	VkImageView imageView;
	res = vkCreateImageView(CDeviceManager::GetDevice(), &imageViewInfo, nullptr, &imageView);
	ASSERT(res == VK_SUCCESS);

	m_pImageViews.push_back(imageView);

	int factor = (eType == eCubeMapArray || eType == eCubeMap) ? 6 : 1;

	if (m_nArrayDim > 1 || m_nMipMapCount > 1)
	{
		for (int layer = 0; layer < m_nArrayDim; layer++)
		{
			imageViewInfo.subresourceRange.baseArrayLayer = layer * factor;
			imageViewInfo.subresourceRange.layerCount = factor;

			for (int mip = 0; mip < m_nMipMapCount; mip++)
			{
				imageViewInfo.subresourceRange.baseMipLevel = mip;
				imageViewInfo.subresourceRange.levelCount = 1;

				res = vkCreateImageView(CDeviceManager::GetDevice(), &imageViewInfo, nullptr, &imageView);
				ASSERT(res == VK_SUCCESS);

				m_pImageViews.push_back(imageView);
			}
		}
	}
}


CTexture::CTexture(unsigned int EmbeddedResourceID, bool bSRGB) : CTexture()
{
	m_eType = eTexture2D;

	HRSRC hResource = FindResource(nullptr, MAKEINTRESOURCEA(EmbeddedResourceID), "DDS");
	ASSERT(hResource != NULL);

	if (hResource != NULL)
	{
		HGLOBAL hMemory = LoadResource(nullptr, hResource);
		ASSERT(hMemory != NULL);

		if (hMemory != NULL)
		{
			const char* data = reinterpret_cast<const char*>(LockResource(hMemory));

			LoadDDSFromMemory(data, bSRGB);
		}

		FreeResource(hResource);
	}
}


CTexture::CTexture(const char* cFileName, bool bSRGB) : CTexture()
{
	m_eType = eTexture2D;

	strcpy(m_cPath, cFileName);

	if (strstr(cFileName, ".dds") != NULL)
		LoadDDS(cFileName, bSRGB);

	else
	{
		char Filename[1024] = "";
		strncpy(Filename, cFileName, 1024);

		char* ptr = Filename + strlen(Filename) - 1;

		while (*ptr != '.' && ptr > Filename)
			ptr--;

		ASSERT(ptr != nullptr);

		strcpy(ptr, ".dds");

		FILE *pFile;

		pFile = fopen(Filename, "rb");
		if (pFile != nullptr)
		{
			fclose(pFile);
			LoadDDS(Filename, bSRGB);
		}

		else if (strstr(cFileName, ".tga") != NULL)
			LoadTGA(cFileName, bSRGB);

		else if (strstr(cFileName, ".bmp") != NULL)
			LoadBMP(cFileName, bSRGB);

		else
			AssertNotReached();
	}
}


CTexture::CTexture(int nWidth, int nHeight, int nDepth, ETextureFormat eFormat, ETextureType eType, const void* pData) : CTexture()
{
	m_nWidth	= nWidth;
	m_nHeight	= nHeight;
	m_nDepth	= nDepth;

	m_eFormat	= eFormat;
	m_eType		= eType;

	m_nArrayDim			= 1;
	m_nSampleCount		= 1;
	m_nSampleQuality	= 0;
	m_nMipMapCount		= 1;

	m_nBitsPerPixel = 0;

	if (m_eFormat == e_R16G16B16A16_FLOAT || m_eFormat == e_R32G32B32A32_FLOAT)
		m_nBitsPerPixel = (m_eFormat == e_R16G16B16A16_FLOAT) ? 64 : 128;

	else if (m_eFormat == e_R32_FLOAT)
		m_nBitsPerPixel = 32;

	else if (m_eFormat == e_R8_UINT)
		m_nBitsPerPixel = 8;

	else if (m_eFormat == e_R8G8B8A8)
		m_nBitsPerPixel = 32;

	unsigned int nBytesPerPixel = m_nBitsPerPixel >> 3U;

	CreateTexture();

	m_nSize = m_nWidth * m_nHeight * m_nDepth * nBytesPerPixel;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pData, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy region;

	region.bufferOffset			= 0;
	region.bufferRowLength		= 0;
	region.bufferImageHeight	= 0;

	region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel		= 0;
	region.imageSubresource.baseArrayLayer	= 0;
	region.imageSubresource.layerCount		= 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { (uint32_t)m_nWidth, (uint32_t)m_nHeight, (uint32_t)m_nDepth };

	copyBufferToTexture(stagingBuffer, m_pImage, 1, 1, &region);

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}


CTexture::CTexture()
{
	m_nTextureID = static_cast<int>(CTextureInterface::ms_pTextures.size());
	m_cPath[0] = '\0';

	CTextureInterface::ms_pTextures.push_back(this);
}


CTexture::~CTexture()
{
	vkFreeMemory(CDeviceManager::GetDevice(), m_pDeviceMemory, nullptr);
	vkDestroyImage(CDeviceManager::GetDevice(), m_pImage, nullptr);

	std::vector<VkImageView>::iterator it;
	for (it = m_pImageViews.begin(); it < m_pImageViews.end(); it++)
		vkDestroyImageView(CDeviceManager::GetDevice(), *it, nullptr);

	m_pImageViews.clear();
}


void CTexture::Save(const char* cFileName)
{
	if (strstr(cFileName, ".dds") != NULL)
		SaveDDS(cFileName);

	else
		fprintf(stderr, "Error : Unsupported texture format : %s\n", cFileName);
}


void CTexture::InitFrame()
{

}


void CTexture::CreateTexture(bool bGenMips)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.extent.width	= m_nWidth;
	imageInfo.extent.height = m_nHeight;
	imageInfo.extent.depth	= m_nDepth;
	imageInfo.arrayLayers	= m_eType == eCubeMap ? 6 * m_nArrayDim : m_nArrayDim;
	imageInfo.format		= ConvertFormat(m_eFormat);
	imageInfo.usage			= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	imageInfo.mipLevels		= m_nMipMapCount;
	imageInfo.samples		= VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling		= VK_IMAGE_TILING_OPTIMAL;
	imageInfo.imageType		= VK_IMAGE_TYPE_2D;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.format = ConvertFormat(m_eFormat);
	imageViewInfo.image = m_pImage;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

	if (m_eType == eTexture3D)
	{
		imageInfo.imageType = VK_IMAGE_TYPE_3D;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
	}

	if (bGenMips)
		imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if (m_eType == eCubeMap)
	{
		imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	}

	VkResult res = vkCreateImage(CDeviceManager::GetDevice(), &imageInfo, nullptr, &m_pImage);
	ASSERT(res == VK_SUCCESS);

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(CDeviceManager::GetDevice(), m_pImage, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	res = vkAllocateMemory(CDeviceManager::GetDevice(), &allocInfo, nullptr, &m_pDeviceMemory);
	ASSERT(res == VK_SUCCESS);

	vkBindImageMemory(CDeviceManager::GetDevice(), m_pImage, m_pDeviceMemory, 0);

	imageViewInfo.image = m_pImage;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.layerCount = m_eType == eCubeMap ? 6 * m_nArrayDim : m_nArrayDim;
	imageViewInfo.subresourceRange.levelCount = m_nMipMapCount;

	VkImageView imageView;
	res = vkCreateImageView(CDeviceManager::GetDevice(), &imageViewInfo, nullptr, &imageView);
	ASSERT(res == VK_SUCCESS);

	m_pImageViews.push_back(imageView);
}


void CTexture::SetTexture(unsigned int slot, CShader::EShaderType eShaderStage, int nSlice, int nLevel)
{
	int index = 0;

	if (nSlice >= 0 || nLevel >= 0)
		index = MAX(0, nSlice) * m_nMipMapCount + MAX(0, nLevel) + 1;

	CResourceManager::SetTexture(slot, m_pImageViews[index]);
}


void CTexture::SetRwTexture(unsigned int slot, CShader::EShaderType eShaderStage, int nSlice, int nLevel)
{
	int index = 0;

	if (nSlice >= 0 || nLevel >= 0)
		index = MAX(0, nSlice) * m_nMipMapCount + MAX(0, nLevel) + 1;

	CResourceManager::SetRwTexture(slot, m_pImageViews[index]);
}



VkImageView CTexture::GetImageView(int nSlice, int nLevel) const
{
	int index = 0;

	if (nSlice >= 0 || nLevel >= 0)
		index = MAX(0, nSlice) * m_nMipMapCount + MAX(0, nLevel) + 1;

	return m_pImageViews[index];
}



void CTexture::TransitionToState(int state)
{
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

	switch (state)
	{
	case CRenderPass::e_PixelShaderResource:
	case CRenderPass::e_NonPixelShaderResource:
	case CRenderPass::e_ShaderResource:
		layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		break;

	case CRenderPass::e_RenderTarget:
		layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		break;

	case CRenderPass::e_DepthStencil_Write:
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		break;

	case CRenderPass::e_DepthStencil_Read:
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		break;

	case CRenderPass::e_UnorderedAccess:
		layout = VK_IMAGE_LAYOUT_GENERAL;
		break;

	case CRenderPass::e_CopyDest:
		layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		break;

	case CRenderPass::e_CopySrc:
		layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		break;

	default:
		ASSERT(0);
		return;
	}

	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = m_pImage;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = m_nMipMapCount;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = m_nArrayDim;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}



void CTexture::GenMipMaps()
{
	
}


