#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "../Textures.h"



#define FOURCC_DXT1		0x31545844
#define FOURCC_DXT2		0x32545844
#define FOURCC_DXT3		0x33545844
#define FOURCC_DXT4		0x34545844
#define FOURCC_DXT5		0x35545844
#define FOURCC_BPTC		0x30315844
#define FOURCC_RGBA16F	0x71
#define FOURCC_RGBA32F	0x74
#define FOURCC_R32F		0x72
#define FOURCC_R16F		0x6F

#define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
#define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

#define DDSD_CAPS			0x1
#define DDSD_HEIGHT			0x2
#define DDSD_WIDTH			0x4
#define DDSD_PITCH			0x8
#define DDSD_PIXELFORMAT	0x1000
#define DDSD_MIPMAPCOUNT	0x20000
#define DDSD_LINEARSIZE		0x80000
#define DDSD_DEPTH			0x800000

#define DDPF_ALPHAPIXELS	0x1
#define DDPF_ALPHA			0x2
#define DDPF_FOURCC			0x4
#define DDPF_RGB			0x40
#define DDPF_YUV			0x200
#define DDPF_LUMINANCE		0x20000

#define DDSCAPS_COMPLEX		0x8
#define DDSCAPS_MIPMAP		0x400000
#define DDSCAPS_TEXTURE		0x1000

#define DDSCAPS2_CUBEMAP			0x200
#define DDSCAPS2_CUBEMAP_POSITIVEX	0x400
#define DDSCAPS2_CUBEMAP_NEGATIVEX	0x800
#define DDSCAPS2_CUBEMAP_POSITIVEY	0x1000
#define DDSCAPS2_CUBEMAP_NEGATIVEY	0x2000
#define DDSCAPS2_CUBEMAP_POSITIVEZ	0x4000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ	0x8000
#define DDSCAPS2_VOLUME				0x200000


struct DDS_PIXELFORMAT
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFourCC;
	DWORD dwRGBBitCount;
	DWORD dwRBitMask;
	DWORD dwGBitMask;
	DWORD dwBBitMask;
	DWORD dwABitMask;
};

typedef struct
{
	DWORD           dwSize;
	DWORD           dwFlags;
	DWORD           dwHeight;
	DWORD           dwWidth;
	DWORD           dwPitchOrLinearSize;
	DWORD           dwDepth;
	DWORD           dwMipMapCount;
	DWORD           dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	DWORD           dwCaps;
	DWORD           dwCaps2;
	DWORD           dwCaps3;
	DWORD           dwCaps4;
	DWORD           dwReserved2;

} DDS_HEADER;


void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);


void copyBufferToTexture(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = numMips;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numRegions, regions);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}


void copyTextureToBuffer(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = numMips;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyImageToBuffer(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, numRegions, regions);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}


void copyBufferToCubeMap(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = numMips;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 6;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numRegions, regions);

	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}


void CTexture::LoadUncompressedDDS(const void* pHeader, const char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount > 0 ? header->dwMipMapCount : 1;
	unsigned int nBytesPerPixel = m_nBitsPerPixel >> 3U;

	CreateTexture();

	if (m_eFormat == e_R16G16B16A16_FLOAT || m_eFormat == e_R32G32B32A32_FLOAT)
		nBytesPerPixel = (m_eFormat == e_R16G16B16A16_FLOAT) ? 8 : 16;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pBuffer, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy* regions = new VkBufferImageCopy[mipMapCount]();

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int offset = 0;

	for (unsigned int level = 0; level < mipMapCount && (m_nWidth > 1 || m_nHeight > 1); ++level)
	{
		unsigned int size = width * height * m_nDepth * nBytesPerPixel;

		regions[level].bufferOffset = offset;
		regions[level].bufferRowLength = 0;
		regions[level].bufferImageHeight = 0;

		regions[level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[level].imageSubresource.mipLevel = level;
		regions[level].imageSubresource.baseArrayLayer = 0;
		regions[level].imageSubresource.layerCount = 1;

		regions[level].imageOffset = { 0, 0, 0 };
		regions[level].imageExtent = { width, height, (uint32_t)m_nDepth };

		offset += size;
		width /= 2;
		height /= 2;
	}

	copyBufferToTexture(stagingBuffer, m_pImage, mipMapCount, mipMapCount, regions);

	delete[] regions;

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}


void CTexture::LoadCompressedDDS(const void* pHeader, const char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int fourCC = header->ddspf.dwFourCC;

	unsigned int blockSize = (fourCC == FOURCC_DXT1) ? 8 : 16;

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int offset = 0;

	CreateTexture();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pBuffer, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy* regions = new VkBufferImageCopy[mipMapCount]();

	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
	{
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
		
		regions[level].bufferOffset = offset;
		regions[level].bufferRowLength = 0;
		regions[level].bufferImageHeight = 0;

		regions[level].imageSubresource.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;
		regions[level].imageSubresource.mipLevel	= level;
		regions[level].imageSubresource.baseArrayLayer = 0;
		regions[level].imageSubresource.layerCount = 1;

		regions[level].imageOffset = { 0, 0, 0 };
		regions[level].imageExtent = { width, height, 1 };

		offset += size;
		width >>= 1;
		height >>= 1;
	}

	copyBufferToTexture(stagingBuffer, m_pImage, mipMapCount, mipMapCount, regions);
	
	delete[] regions;

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}


void CTexture::LoadUncompressedCubeMapDDS(const void* pHeader, const char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int nBytesPerPixel = m_nBitsPerPixel >> 3U;

	if (m_eFormat == e_R16G16B16A16_FLOAT || m_eFormat == e_R32G32B32A32_FLOAT)
		nBytesPerPixel = (m_eFormat == e_R16G16B16A16_FLOAT) ? 8 : 16;

	CreateTexture();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pBuffer, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy* regions = new VkBufferImageCopy[mipMapCount * 6]();

	unsigned int offset = 0;

	for (unsigned int face = 0; face < 6; ++face)
	{
		unsigned int width = m_nWidth;
		unsigned int height = m_nHeight;

		for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;

			unsigned int size = width * height * nBytesPerPixel;

			regions[face * mipMapCount + level].bufferOffset = offset;
			regions[face * mipMapCount + level].bufferRowLength = 0;
			regions[face * mipMapCount + level].bufferImageHeight = 0;
							  
			regions[face * mipMapCount + level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[face * mipMapCount + level].imageSubresource.mipLevel = level;
			regions[face * mipMapCount + level].imageSubresource.baseArrayLayer = face;
			regions[face * mipMapCount + level].imageSubresource.layerCount = 1;
							  
			regions[face * mipMapCount + level].imageOffset = { 0, 0, 0 };
			regions[face * mipMapCount + level].imageExtent = { width, height, 1 };

			offset += size;
			width >>= 1;
			height >>= 1;
		}
	}

	copyBufferToCubeMap(stagingBuffer, m_pImage, mipMapCount, mipMapCount * 6, regions);

	delete[] regions;

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}


void CTexture::LoadCompressedCubeMapDDS(const void* pHeader, const char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int fourCC = header->ddspf.dwFourCC;

	unsigned int blockSize = (fourCC == FOURCC_DXT1) ? 8 : 16;

	CreateTexture();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pBuffer, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy* regions = new VkBufferImageCopy[mipMapCount * 6]();

	unsigned int offset = 0;

	for (unsigned int face = 0; face < 6; ++face)
	{
		unsigned int width = m_nWidth;
		unsigned int height = m_nHeight;

		for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;

			unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

			regions[face * mipMapCount + level].bufferOffset = offset;
			regions[face * mipMapCount + level].bufferRowLength = 0;
			regions[face * mipMapCount + level].bufferImageHeight = 0;

			regions[face * mipMapCount + level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			regions[face * mipMapCount + level].imageSubresource.mipLevel = level;
			regions[face * mipMapCount + level].imageSubresource.baseArrayLayer = face;
			regions[face * mipMapCount + level].imageSubresource.layerCount = 1;

			regions[face * mipMapCount + level].imageOffset = { 0, 0, 0 };
			regions[face * mipMapCount + level].imageExtent = { width, height, 1 };

			offset += size;
			width >>= 1;
			height >>= 1;
		}
	}

	copyBufferToCubeMap(stagingBuffer, m_pImage, mipMapCount, mipMapCount * 6, regions);

	delete[] regions;

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}



void CTexture::LoadDDSFromMemory(const char* memory, bool bSRGB)
{
	DDS_HEADER header;

	const char* ptr = memory;

	if (strncmp(ptr, "DDS ", 4) != 0)
	{
		ASSERT_MSG(0, "Error : Corrupted file\n");
		return;
	}

	ptr += 4;

	memcpy(&header, ptr, sizeof(header));
	ptr += sizeof(header);

	m_nHeight = header.dwHeight;
	m_nWidth = header.dwWidth;
	m_nArrayDim = 1;

	if (header.dwCaps2 & DDSCAPS2_VOLUME)
		m_nDepth = MAX(1, header.dwDepth);
	else
		m_nDepth = 1;

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int linearSize;
	VkFormat format;
	unsigned int fourCC;

	if (header.ddspf.dwFlags & DDPF_FOURCC)
	{
		fourCC = header.ddspf.dwFourCC;

		switch (fourCC)
		{
		case FOURCC_DXT1:
			format = bSRGB ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT1 : e_DXT1_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT3:
			format = bSRGB ? VK_FORMAT_BC2_SRGB_BLOCK : VK_FORMAT_BC2_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT3 : e_DXT3_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT5:
			format = bSRGB ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT5 : e_DXT5_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_BPTC:
			format = bSRGB ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT7 : e_DXT7_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_RGBA16F:
			format = VK_FORMAT_R16G16B16A16_SFLOAT;
			m_eFormat = e_R16G16B16A16_FLOAT;
			m_nBitsPerPixel = 64;
			break;

		case FOURCC_RGBA32F:
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			m_eFormat = e_R32G32B32A32_FLOAT;
			m_nBitsPerPixel = 128;
			break;

		case FOURCC_R32F:
			format = VK_FORMAT_R32_SFLOAT;
			m_eFormat = e_R32_FLOAT;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_R16F:
			format = VK_FORMAT_R16_SFLOAT;
			m_eFormat = e_R16_FLOAT;
			m_nBitsPerPixel = 16;
			break;

		default:
			ASSERT_MSG(0, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
			return;
		}
	}

	else if (header.ddspf.dwFlags & DDPF_RGB)
	{
		m_nBitsPerPixel = header.ddspf.dwRGBBitCount;

		switch (m_nBitsPerPixel)
		{
		case 8:
			format = VK_FORMAT_R8_UINT;
			m_eFormat = e_R8_UINT;
			break;

		case 24:
			format = VK_FORMAT_R8G8B8_SRGB;
			m_eFormat = e_R8G8B8;
			break;

		case 32:
			format = VK_FORMAT_R8G8B8A8_SRGB;
			m_eFormat = e_R8G8B8A8;
			break;

		default:
			break;
		}
	}

	else if (header.ddspf.dwFlags & DDPF_LUMINANCE)
	{
		m_nBitsPerPixel = header.ddspf.dwRGBBitCount;

		switch (m_nBitsPerPixel)
		{
		case 8:
			format = VK_FORMAT_R8_UNORM;
			m_eFormat = e_R8;
			break;

		default:
			break;
		}
	}

	else
	{
		ASSERT_MSG(0, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
		return;
	}

	if (header.dwFlags & DDSD_LINEARSIZE)
	{
		linearSize = header.dwPitchOrLinearSize;

		if (fourCC != FOURCC_DXT1 && fourCC != FOURCC_DXT3 && fourCC != FOURCC_DXT5 && fourCC != FOURCC_BPTC)
			linearSize *= header.dwHeight;
	}
	else
		linearSize = width * height * m_nDepth * (m_nBitsPerPixel >> 3);

	m_nMipMapCount = header.dwMipMapCount;

	bool bIsCubeMap = (header.dwCaps2 & 0x200) ? true : false;

	unsigned int bufsize;

	bufsize = m_nMipMapCount > 1 ? linearSize * 2 : linearSize;

	if (bIsCubeMap)
	{
		bufsize *= 6;
		m_eType = eCubeMap;
	}

	else if (m_nDepth > 1)
		m_eType = eTexture3D;

	else
		m_eType = eTexture2D;

	m_nSize = bufsize;

	if (header.ddspf.dwFlags & DDPF_FOURCC && (fourCC == FOURCC_DXT1 || fourCC == FOURCC_DXT3 || fourCC == FOURCC_DXT5 || fourCC == FOURCC_BPTC))
	{
		if (bIsCubeMap)
			LoadCompressedCubeMapDDS(&header, ptr, format);
		else
			LoadCompressedDDS(&header, ptr, format);
	}

	else
	{
		if (bIsCubeMap)
			LoadUncompressedCubeMapDDS(&header, ptr, format);
		else
			LoadUncompressedDDS(&header, ptr, format);
	}
}



void CTexture::LoadDDS(const char * cFileName, bool bSRGB)
{
	DDS_HEADER header;

	FILE *pFile;

	pFile = fopen(cFileName, "rb");
	if (pFile == NULL)
	{
		ASSERT_MSG(0, "Error : Could not find DDS file %s\n", cFileName);
		return;
	}

	char filecode[5] = "";
	fread(filecode, 1, 4, pFile);
	if (strncmp(filecode, "DDS ", 4) != 0)
	{
		fclose(pFile);
		ASSERT_MSG(0, "Error : Corrupted file %s\n", cFileName);
		return;
	}

	fread(&header, 1, sizeof(header), pFile);

	m_nHeight = header.dwHeight;
	m_nWidth = header.dwWidth;
	m_nArrayDim = 1;

	if (header.dwCaps2 & DDSCAPS2_VOLUME)
		m_nDepth = MAX(1, header.dwDepth);
	else
		m_nDepth = 1;

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int linearSize;
	VkFormat format;
	unsigned int fourCC;

	if (header.ddspf.dwFlags & DDPF_FOURCC)
	{
		fourCC = header.ddspf.dwFourCC;

		switch (fourCC)
		{
		case FOURCC_DXT1:
			format = bSRGB ? VK_FORMAT_BC1_RGBA_SRGB_BLOCK : VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT1 : e_DXT1_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT3:
			format = bSRGB ? VK_FORMAT_BC2_SRGB_BLOCK : VK_FORMAT_BC2_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT3 : e_DXT3_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT5:
			format = bSRGB ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT5 : e_DXT5_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_BPTC:
			format = bSRGB ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
			m_eFormat = bSRGB ? e_DXT7 : e_DXT7_SRGB;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_RGBA16F:
			format = VK_FORMAT_R16G16B16A16_SFLOAT;
			m_eFormat = e_R16G16B16A16_FLOAT;
			m_nBitsPerPixel = 64;
			break;

		case FOURCC_RGBA32F:
			format = VK_FORMAT_R32G32B32A32_SFLOAT;
			m_eFormat = e_R32G32B32A32_FLOAT;
			m_nBitsPerPixel = 128;
			break;

		case FOURCC_R32F:
			format = VK_FORMAT_R32_SFLOAT;
			m_eFormat = e_R32_FLOAT;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_R16F:
			format = VK_FORMAT_R16_SFLOAT;
			m_eFormat = e_R16_FLOAT;
			m_nBitsPerPixel = 16;
			break;

		default:
			ASSERT_MSG(0, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
			return;
		}
	}

	else if (header.ddspf.dwFlags & DDPF_RGB)
	{
		m_nBitsPerPixel = header.ddspf.dwRGBBitCount;

		switch (m_nBitsPerPixel)
		{
		case 8:
			format = VK_FORMAT_R8_UINT;
			m_eFormat = e_R8_UINT;
			break;

		case 24:
			format = VK_FORMAT_R8G8B8_SRGB;
			m_eFormat = e_R8G8B8;
			break;

		case 32:
			format = VK_FORMAT_R8G8B8A8_SRGB;
			m_eFormat = e_R8G8B8A8;
			break;

		default:
			break;
		}
	}

	else if (header.ddspf.dwFlags & DDPF_LUMINANCE)
	{
		m_nBitsPerPixel = header.ddspf.dwRGBBitCount;

		switch (m_nBitsPerPixel)
		{
		case 8:
			format = VK_FORMAT_R8_UNORM;
			m_eFormat = e_R8;
			break;

		default:
			break;
		}
	}

	else
	{
		ASSERT_MSG(0, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
		return;
	}

	if (header.dwFlags & DDSD_LINEARSIZE)
	{
		linearSize = header.dwPitchOrLinearSize;

		if (fourCC != FOURCC_DXT1 && fourCC != FOURCC_DXT3 && fourCC != FOURCC_DXT5 && fourCC != FOURCC_BPTC)
			linearSize *= header.dwHeight;
	}
	else
		linearSize = width * height * m_nDepth * (m_nBitsPerPixel >> 3);	

	m_nMipMapCount = header.dwMipMapCount;

	bool bIsCubeMap = (header.dwCaps2 & 0x200) ? true : false;

	char * buffer;
	unsigned int bufsize;

	bufsize = m_nMipMapCount > 1 ? linearSize * 2 : linearSize;

	if (bIsCubeMap)
	{
		bufsize *= 6;
		m_eType = eCubeMap;
	}

	else if (m_nDepth > 1)
		m_eType = eTexture3D;

	else
		m_eType = eTexture2D;

	m_nSize = bufsize;

	buffer = new char[bufsize];
	fread(buffer, 1, bufsize, pFile);

	fclose(pFile);

	if (header.ddspf.dwFlags & DDPF_FOURCC && (fourCC == FOURCC_DXT1 || fourCC == FOURCC_DXT3 || fourCC == FOURCC_DXT5 || fourCC == FOURCC_BPTC))
	{
		if (bIsCubeMap)
			LoadCompressedCubeMapDDS(&header, buffer, format);
		else
			LoadCompressedDDS(&header, buffer, format);
	}

	else
	{
		if (bIsCubeMap)
			LoadUncompressedCubeMapDDS(&header, buffer, format);
		else
			LoadUncompressedDDS(&header, buffer, format);
	}

	delete[] buffer;

}


void CTexture::SaveDDS(const char * cFileName)
{
	DDS_HEADER header;

	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

	if (m_eFormat == e_DXT1 || m_eFormat == e_DXT3 || m_eFormat == e_DXT5)
	{
		header.dwFlags |= DDSD_LINEARSIZE;
		header.dwPitchOrLinearSize = MAX(1, ((m_nWidth + 3) / 4) * ((m_nHeight + 3) / 4)) * (m_eFormat == e_DXT1 ? 8 : 16);
	}

	else
	{
		header.dwFlags |= DDSD_PITCH;
		header.dwPitchOrLinearSize = (m_nWidth * m_nBitsPerPixel + 7) / 8;
	}

	if (m_nMipMapCount > 1)
		header.dwFlags |= DDSD_MIPMAPCOUNT;

	header.dwHeight = m_nHeight;
	header.dwWidth = m_nWidth;
	header.dwDepth = m_nDepth;
	header.dwMipMapCount = MAX(1, m_nMipMapCount - 1);
	header.ddspf.dwSize = 32;

	switch (m_eFormat)
	{
	case e_DXT1:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_DXT1;
		break;

	case e_DXT3:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_DXT3;
		break;

	case e_DXT5:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_DXT5;
		break;

	case e_DXT7:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_BPTC;
		break;

	case e_R16G16B16A16_FLOAT:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_RGBA16F;
		break;

	case e_R32G32B32A32_FLOAT:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_RGBA32F;
		break;

	case e_R8G8B8A8:
		header.ddspf.dwFlags	= DDPF_RGB;
		header.ddspf.dwRGBBitCount = 32;
		header.ddspf.dwRBitMask = 0xff000000;
		header.ddspf.dwGBitMask = 0xff0000;
		header.ddspf.dwBBitMask = 0xff00;
		header.ddspf.dwABitMask = 0xff;
		break;

	case e_R32_FLOAT:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_R32F;
		break;

	case e_R16_FLOAT:
		header.ddspf.dwFlags	= DDPF_FOURCC;
		header.ddspf.dwFourCC	= FOURCC_R16F;
		break;

	case e_R8_UINT:
		header.ddspf.dwFlags	= DDPF_RGB;
		header.ddspf.dwRGBBitCount = 8;
		header.ddspf.dwRBitMask = 0xff;
		header.ddspf.dwGBitMask = 0;
		header.ddspf.dwBBitMask = 0;
		header.ddspf.dwABitMask = 0;
		break;

	default:
		fprintf(stderr, "Error : Incompatible surface format\n");
		return;
		break;
	}

	header.dwCaps = DDSCAPS_TEXTURE;
	header.dwCaps2 = 0;

	if (m_nMipMapCount > 1 || m_eType == eCubeMap)
	{
		header.dwCaps |= DDSCAPS_COMPLEX;

		if (m_nMipMapCount > 1)
			header.dwCaps |= DDSCAPS_MIPMAP;

		if (m_eType == eCubeMap)
		{
			header.dwCaps2 = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_NEGATIVEZ | \
				DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ;
		}
	}

	if (m_nDepth > 1)
	{
		header.dwFlags |= DDS_HEADER_FLAGS_VOLUME;
		header.dwCaps2 |= DDSCAPS2_VOLUME;
	}

	FILE* pFile = fopen(cFileName, "wb+");
	ASSERT_MSG(pFile != NULL, "Error : Could not find DDS file %s\n", cFileName);

	char filecode[5] = "DDS ";
	fwrite(filecode, 1, 4, pFile);
	fwrite(&header, sizeof(header), 1, pFile);

	if (m_eFormat == e_DXT1 || m_eFormat == e_DXT3 || m_eFormat == e_DXT5 || m_eFormat == e_DXT1_SRGB || m_eFormat == e_DXT3_SRGB || m_eFormat == e_DXT5_SRGB)
	{
		if (m_eType == eCubeMap)
			SaveCompressedCubeMapDDS(pFile);
		else
			SaveCompressedDDS(pFile);
	}

	else
	{
		if (m_eType == eCubeMap)
			SaveUncompressedCubeMapDDS(pFile);
		else
			SaveUncompressedDDS(pFile);
	}

	fclose(pFile);
}


void CTexture::SaveUncompressedDDS(FILE* pFile)
{
	unsigned int width	= m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int depth	= m_nDepth;
	unsigned int offset = 0;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	VkBufferImageCopy* regions = new VkBufferImageCopy[m_nMipMapCount]();

	for (int level = 0; level < m_nMipMapCount && (width > 1 || height > 1); ++level)
	{
		unsigned int size = width * height * m_nBitsPerPixel / 8;

		regions[level].bufferOffset = offset;
		regions[level].bufferRowLength = 0;
		regions[level].bufferImageHeight = 0;

		regions[level].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[level].imageSubresource.mipLevel = level;
		regions[level].imageSubresource.baseArrayLayer = 0;
		regions[level].imageSubresource.layerCount = 1;

		regions[level].imageOffset = { 0, 0, 0 };
		regions[level].imageExtent = { width, height, depth };

		offset += size;
		width /= 2;
		height /= 2;
	}

	copyTextureToBuffer(stagingBuffer, m_pImage, m_nMipMapCount, m_nMipMapCount, regions);

	delete[] regions;

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	fwrite(data, m_nSize, 1, pFile);
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	vkDestroyBuffer(CDeviceManager::GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(CDeviceManager::GetDevice(), stagingBufferMemory, nullptr);
}
