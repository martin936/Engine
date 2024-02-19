#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "../Textures.h"


void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

extern void copyBufferToTexture(VkBuffer buffer, VkImage image, int numMips, int numRegions, VkBufferImageCopy* regions);

extern void generateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

#pragma pack(push, 1)

struct BMPHeader 
{
	uint16_t	signature;			// Signature ('BM' for Bitmap)
	uint32_t	fileSize;			// Size of the BMP file
	uint32_t	reserved;			// Reserved, set to 0
	uint32_t	dataOffset;			// Offset to the start of image data
	uint32_t	headerSize;			// Size of the header (40 for this version)
	int32_t		width;				// Width of the image in pixels
	int32_t		height;				// Height of the image in pixels
	uint16_t	planes;				// Number of color planes (1)
	uint16_t	bitsPerPixel;		// Number of bits per pixel (24 or 32)
	uint32_t	compression;		// Compression method (0 for uncompressed)
	uint32_t	imageSize;			// Size of the image data (can be 0 for uncompressed)
	int32_t		xPixelsPerMeter;	// Horizontal resolution (pixels per meter)
	int32_t		yPixelsPerMeter;	// Vertical resolution (pixels per meter)
	uint32_t	colorsUsed;			// Number of colors in the palette (0 for full color)
	uint32_t	colorsImportant;	// Number of important colors (0 for all)
};

#pragma pack(pop)


// Function to load BMP image
uint8_t* CTexture::LoadBMPData(const char* cFileName, int& width, int& height, int& bpp)
{
	FILE* pFile;

	fopen_s(&pFile, cFileName, "rb");

	if (pFile == NULL)
	{
		ASSERT_MSG(0, "Error : Could not find BMP file %s\n", cFileName);
		return nullptr;
	}

	BMPHeader header;
	fread(&header, sizeof(BMPHeader), 1, pFile);

	if (header.signature != 0x4D42) 
	{ 
		// Check for 'BM' signature
		ASSERT_MSG(0, "Error : Could not load %s : Corrupted file format\n", cFileName);
		fclose(pFile);
		return nullptr;
	}

	width	= header.width;
	height	= header.height;
	bpp		= header.bitsPerPixel;

	// Determine the number of channels (3 for 24-bit, 4 for 32-bit)
	int channels = header.bitsPerPixel / 8;

	// Calculate padding
	int padding = (4 - (width * channels) % 4) % 4;

	// Read image data
	uint8_t* pData = new uint8_t[header.width * header.height * channels];

	if (padding > 0)
	{
		for (int i = 0; i < header.height; i++)
		{
			fread(&pData[i * header.width * channels], 1, header.width * channels, pFile);
			fseek(pFile, padding, SEEK_CUR);
		}
	}

	else
		fread(pData, 1, header.height * header.width * channels, pFile);

	// Close the file
	fclose(pFile);

	return pData;
}


void CTexture::SaveBMPData(const char* cFileName, const uint8_t* pData, int width, int height, int bpp)
{
	FILE* pFile;

	fopen_s(&pFile, cFileName, "wb+");

	if (pFile == NULL)
	{
		ASSERT_MSG(0, "Error : Could not find BMP file %s\n", cFileName);
		return;
	}

    // Calculate image size and padding
    int channels = bpp / 8;
    int padding = (4 - (width * channels) % 4) % 4;
    int rowSize = width * channels + padding;
    int imageSize = rowSize * height;

    // Prepare BMP header
    BMPHeader header;
    header.signature = 0x4D42;  // 'BM'
    header.fileSize = sizeof(BMPHeader) + imageSize;
    header.reserved = 0;
    header.dataOffset = sizeof(BMPHeader);
    header.headerSize = 40;
    header.width = width;
    header.height = height;
    header.planes = 1;
    header.bitsPerPixel = bpp;
    header.compression = 0;
    header.imageSize = imageSize;
    header.xPixelsPerMeter = 0;
    header.yPixelsPerMeter = 0;
    header.colorsUsed = 0;
    header.colorsImportant = 0;

    // Write BMP header
	fwrite(&header, sizeof(BMPHeader), 1, pFile);

	// Write image data
	if (padding > 0)
	{
		uint8_t* paddingData = new uint8_t[padding]();

		for (int i = 0; i < header.height; i++)
		{
			fwrite(&pData[i * header.width * channels], 1, header.width * channels, pFile);
			fwrite(paddingData, 1, padding, pFile);
		}
	}

	else    
		fwrite(pData, 1, imageSize, pFile);

    // Close the file
    fclose(pFile);
}


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


