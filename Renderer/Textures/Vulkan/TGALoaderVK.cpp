#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "../Textures.h"


typedef struct
{
	char r;
	char g;
	char b;
	char a;
}SPixel;


void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

extern void copyBufferToTexture(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions);


char* CTexture::LoadTGAData(const char* cFileName)
{
	unsigned char Header[18] = { 0 };

	static char DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static char IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	char* pData;

	FILE* pFile;
	fopen_s(&pFile, cFileName, "rb");

	if (pFile == NULL)
	{
		ASSERT_MSG(0, "Error : Could not find TGA file %s\n", cFileName);
		return NULL;
	}

	fread_s(Header, 18, sizeof(unsigned char), 18, pFile);

	if (!memcmp(DeCompressed, Header, 12))
	{
		m_nBitsPerPixel = Header[16];
		m_nWidth = Header[13] * 256 + Header[12];

		m_nHeight = Header[15] * 256 + Header[14];
		m_nSize = ((m_nWidth * m_nBitsPerPixel + 31) / 32) * 4 * m_nHeight;

		if (m_nBitsPerPixel != 24 && m_nBitsPerPixel != 32)
		{
			ASSERT_MSG(0, "Error : Could not load %s : Corrupted file format\n", cFileName);
			fclose(pFile);
			return NULL;
		}

		pData = new char[m_nSize];

		fread_s(pData, m_nSize, sizeof(char), m_nSize, pFile);
	}

	else if (!memcmp(IsCompressed, Header, 12))
	{
		m_nBitsPerPixel = Header[16];
		m_nWidth = Header[13] * 256 + Header[12];
		m_nHeight = Header[15] * 256 + Header[14];
		m_nSize = ((m_nWidth * m_nBitsPerPixel + 31) / 32) * 4 * m_nHeight;

		if (m_nBitsPerPixel != 24 && m_nBitsPerPixel != 32)
		{
			ASSERT_MSG(0, "Error : Could not load %s : Corrupted file format\n", cFileName);
			fclose(pFile);
			return NULL;
		}

		SPixel pixel;
		int CurrentByte = 0;
		size_t CurrentPixel = 0;
		unsigned char ChunkHeader = 0;
		int BytesPerPixel = m_nBitsPerPixel >> 3;

		pData = new char[4 * m_nWidth * m_nHeight];

		do
		{
			fread_s(&ChunkHeader, sizeof(ChunkHeader), sizeof(ChunkHeader), 1, pFile);

			if (ChunkHeader < 128)
			{
				++ChunkHeader;
				for (int i = 0; i < ChunkHeader; ++i, ++CurrentPixel)
				{
					fread_s(&pixel, sizeof(pixel), sizeof(char), BytesPerPixel, pFile);

					pData[CurrentByte++] = pixel.b;
					pData[CurrentByte++] = pixel.g;
					pData[CurrentByte++] = pixel.r;
					if (m_nBitsPerPixel > 24) pData[CurrentByte++] = pixel.a;
				}
			}

			else
			{
				ChunkHeader -= 127;

				fread_s(&pixel, sizeof(pixel), sizeof(char), BytesPerPixel, pFile);

				for (int i = 0; i < ChunkHeader; ++i, ++CurrentPixel)
				{
					pData[CurrentByte++] = pixel.b;
					pData[CurrentByte++] = pixel.g;
					pData[CurrentByte++] = pixel.r;
					if (m_nBitsPerPixel > 24) pData[CurrentByte++] = pixel.a;
				}
			}

		} while (CurrentPixel < m_nWidth * m_nHeight);
	}
	else
	{
		ASSERT_MSG(0, "Error : Could not load %s : Corrupted file format\n", cFileName);
		fclose(pFile);
		return NULL;
	}

	fclose(pFile);

	return pData;
}


void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	barrier.subresourceRange.baseMipLevel = 1;
	barrier.subresourceRange.levelCount = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		if (i > 1)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	CCommandListManager::EndOneTimeCommandList(commandBuffer);
}


void CTexture::LoadTGA(const char* cFileName, bool sRGB)
{
	m_eFormat = sRGB ? e_R8G8B8A8_SRGB : e_R8G8B8A8;

	m_nArrayDim			= 1;
	m_nDepth			= 1;
	m_nSampleCount		= 1;
	m_nSampleQuality	= 0;

	char* pData = LoadTGAData(cFileName);

	if (pData == NULL)
		return;

	m_nSize = 4 * m_nWidth * m_nHeight;

	m_nMipMapCount = static_cast<int>(log2(MAX(m_nWidth, m_nHeight))) + 1;

	CreateTexture(true);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	memcpy(data, pData, static_cast<size_t>(m_nSize));
	vkUnmapMemory(CDeviceManager::GetDevice(), stagingBufferMemory);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { (uint32_t)m_nWidth, (uint32_t)m_nHeight, 1U };

	copyBufferToTexture(stagingBuffer, m_pImage, 1, 1, &region);

	delete[] pData;

	generateMipmaps(m_pImage, m_nWidth, m_nHeight, m_nMipMapCount);
}


void CTexture::SaveTGA(const char* cFileName, bool bUseRLE)
{

}


CTexture* CTexture::LoadCubeMapFromTGA(const char* cPosX, const char* cNegX, const char* cPosY, const char* cNegY, const char* cPosZ, const char* cNegZ)
{
	return nullptr;
}