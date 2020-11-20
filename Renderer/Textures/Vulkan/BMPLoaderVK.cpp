#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "../Textures.h"


void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

extern void copyBufferToTexture(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions);

extern void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);


void CTexture::LoadBMP(const char* cFileName, bool bSRGB)
{
	FILE* pFile;

	fopen_s(&pFile, cFileName, "rb");

	if (pFile == NULL)
	{
		ASSERT_MSG(0, "Error : Could not find BMP file %s\n", cFileName);
		return;
	}

	unsigned char Header[54];
	unsigned char BitsPerPixels;

	fread_s(Header, 54, sizeof(unsigned char), 54, pFile);

	BitsPerPixels = Header[28];

	if (Header[0] != 'B' || Header[1] != 'M' || (BitsPerPixels != 24 && BitsPerPixels != 32))
	{
		ASSERT_MSG(0, "Error : Could not load %s : Corrupted file format\n", cFileName);
		fclose(pFile);
		return;
	}

	m_nWidth = Header[18] + (Header[19] << 8);
	m_nHeight = Header[22] + (Header[23] << 8);

	unsigned int PixelsOffset = Header[10] + (Header[11] << 8);
	unsigned int Size = ((m_nWidth * BitsPerPixels + 31) / 32) * 4 * m_nHeight;

	m_eFormat = bSRGB ? e_R8G8B8A8_SRGB : e_R8G8B8A8;

	m_nArrayDim = 1;
	m_nDepth = 1;
	m_nSampleCount = 1;
	m_nSampleQuality = 0;

	char* pData = new char[Size];

	fread_s(pData, Size, sizeof(char), Size, pFile);

	fclose(pFile);

	m_nSize = 4 * m_nWidth * m_nHeight;

	m_nMipMapCount = static_cast<int>(log2(MAX(m_nWidth, m_nHeight))) + 1;

	CreateTexture(true);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(m_nSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(CDeviceManager::GetDevice(), stagingBufferMemory, 0, m_nSize, 0, &data);
	for (int i = 0; i < m_nHeight; i++)
		for (int j = 0; j < m_nWidth; j++)
		{
			memcpy(&((char*)data)[(i * m_nWidth + j) * 4], &pData[(i * m_nWidth + j) * 3], 3 * sizeof(char));
			((char*)data)[(i * m_nWidth + j) * 4 + 3] = static_cast<uint8_t>(0xff);
		}

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


