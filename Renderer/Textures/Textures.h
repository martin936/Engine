#ifndef __TEXTURES_H__
#define __TEXTURES_H__

#define INVALIDHANDLE 0xFFFFFFFF

#define TEXTURE_PATH "../Data/Textures/"

enum ETextureFormat
{
	e_UNKOWN,
	e_B8G8R8A8,
	e_R8G8B8A8,
	e_R8G8B8A8_SRGB,
	e_R8G8B8A8_SNORM,
	e_R8G8B8,
	e_R8G8B8_SNORM,
	e_R8G8,
	e_R8G8_SNORM,
	e_R8,
	e_R16G16,
	e_R16G16_SNORM,
	e_R16G16B16A16_FLOAT,
	e_R16G16B16_FLOAT,
	e_R32G32B32A32_FLOAT,
	e_R32G32B32A32_UINT,
	e_R32G32B32_UINT,
	e_R16G16B16A16_UINT,
	e_R16G16B16A16_UNORM,
	e_R8G8B8A8_UINT,
	e_R8G8B8A8_SINT,
	e_R32_FLOAT,
	e_R16G16_FLOAT,
	e_R16_FLOAT,
	e_R10G10B10A2,
	e_R11G11B10_FLOAT,
	e_R9G9B9E5_FLOAT,
	e_R8_UINT,
	e_R16_UINT,
	e_R32_UINT,
	e_R32G32_UINT,
	e_R16G16_UINT,
	e_R16G16_UNORM,
	e_R8G8_UINT,
	e_R8G8B8_SRGB,
	e_R16_DEPTH,
	e_R24_DEPTH,
	e_R32_DEPTH,
	e_R24_DEPTH_G8_STENCIL,
	e_R32_DEPTH_G8_STENCIL,

	e_DXT1,
	e_DXT3,
	e_DXT5,
	e_DXT7,

	e_DXT1_SRGB,
	e_DXT3_SRGB,
	e_DXT5_SRGB,
	e_DXT7_SRGB
};

enum ETextureType
{
	eTexture2D,
	eTextureArray,
	eTextureStorage2D,
	eTextureStorage2DArray,
	eCubeMap,
	eCubeMapArray,
	eTexture3D,
	eTextureStorage3D
};


#include "Engine/Engine.h"
#include "Engine/Device/Shaders.h"
#include <vector>


class CTexture
{
	friend class CResourceManager;
	friend class CTextureInterface;

public:

	enum EBufferAccess
	{
		e_ReadOnly,
		e_WriteOnly,
		e_ReadWrite
	};


	CTexture(int nWidth, int nHeight, ETextureFormat eFormat, ETextureType eType = eTexture2D, int sampleCount = 1, bool bGenMipMaps = false);
	CTexture(int nWidth, int nHeight, int nDepth, ETextureFormat eFormat, ETextureType eType = eTexture2D, int sampleCount = 1, bool bGenMipMaps = false);
	CTexture(const char* cFileName, bool bSRGB = false);
	CTexture(unsigned int EmbeddedResourceID, bool bSRGB = false);
	CTexture(int nWidth, int nHeight, int nDepth, ETextureFormat eFormat, ETextureType eType, const void* pData);
	CTexture();
	~CTexture();

	void Save(const char* cFileName);

	void SetTexture(unsigned int slot, CShader::EShaderType eShaderStage = CShader::EShaderType::e_FragmentShader, int nSlice = -1, int nLevel = -1);
	void SetRwTexture(unsigned int slot, CShader::EShaderType eShaderStage = CShader::EShaderType::e_FragmentShader, int nSlice = -1, int nLevel = -1);

	static CTexture* LoadCubeMapFromTGA(const char* cPosX, const char* cNegX, const char* cPosY, const char* cNegY, const char* cPosZ, const char* cNegZ);

	void	TransitionToState(int state);

#ifdef __VULKAN__
	VkImage GetImage() const
	{
		return m_pImage;
	}

	VkImageView GetImageView(int nSlice = -1, int nLevel = -1) const;
#endif

	static void InitFrame();

	inline unsigned int GetMipMapCount() const
	{
		return m_nMipMapCount;
	}

	inline int GetWidth(int nLevel = -1) const
	{
		int nWidth = m_nWidth;
		int level = MAX(0, nLevel);

		for (int i = 0; i < level; i++)
			nWidth /= 2;

		return MAX(1, nWidth);
	}

	inline int GetHeight(int nLevel = -1) const
	{
		int nHeight = m_nHeight;
		int level = MAX(0, nLevel);

		for (int i = 0; i < level; i++)
			nHeight /= 2;

		return MAX(1, nHeight);
	}

	inline int GetDepth(int nLevel = -1) const
	{
		int nDepth = m_nDepth;
		int level = MAX(0, nLevel);

		for (int i = 0; i < level; i++)
			nDepth /= 2;

		return MAX(1, nDepth);
	}

	inline int GetArraySize() const
	{
		return m_nArrayDim;
	}

	inline int GetBitsPerPixel() const
	{
		return m_nBitsPerPixel;
	}

	inline int GetSampleCount() const
	{
		return m_nSampleCount;
	}

	inline int GetSampleQuality() const
	{
		return m_nSampleQuality;
	}

	inline size_t GetSize() const
	{
		return m_nSize;
	}

	inline ETextureFormat GetFormat() const
	{
		return m_eFormat;
	}

	inline ETextureType GetType() const
	{
		return m_eType;
	}

	inline unsigned int GetID() const
	{
		return m_nTextureID;
	}

private:

	ETextureFormat m_eFormat;
	ETextureType m_eType;

	char m_cPath[1024];

	int m_nWidth;
	int m_nHeight;
	int m_nDepth;
	int m_nArrayDim;
	int m_nMipMapCount;
	int m_nBitsPerPixel;
	int m_nSampleCount;
	int m_nSampleQuality;
	int m_nTextureID;

	unsigned int m_eState;

	size_t m_nSize;

#ifdef __VULKAN__
	VkImage							m_pImage;
	VkDeviceMemory					m_pDeviceMemory;
	std::vector<VkImageView>		m_pImageViews;
#endif

#ifdef __OPENGL__
	unsigned int m_nID;
#endif

	static unsigned int ms_nActiveSlot;
	static unsigned int ms_nActiveTexture[16];
	static unsigned int ms_nActiveTextureArray[16];
	static unsigned int ms_nActiveCubeMap[16];

	void GenerateTexture2D();
	void GenerateTexture3D();
	void GenerateTextureArray();
	void GenerateTextureStorage2D();
	void GenerateTextureStorage3D();
	void GenerateCubeMap();
	void GenerateCubeMapArray();

	static void GetFormat(ETextureFormat eFormat, void* Type, void* Format);

	// Bitmap
	void LoadBMP(const char* cFileName, bool bSRGB = false);
	void SaveBMP(const char* cFileName);

	// TARGA
	void LoadTGA(const char* cFileName, bool bSRGB = false);
	void SaveTGA(const char* cFileName, bool bUseRLE = true);
	char* LoadTGAData(const char* cFileName);

	// Radiance
	void LoadHDR(const char* cFileName);
	void SaveHDR(const char* cFileName);
	void ReadHDRHeader(FILE* pFile);

	// DDS
	void LoadDDS(const char* cFileName, bool bSRGB = false);
	void LoadDDSFromMemory(const char* memory, bool bSRGB = false);
	void SaveDDS(const char* cFileName);

	void CreateTexture(bool bGenMips = false);

	void LoadCompressedDDS(const void* pHeader, const char* pBuffer, unsigned int format);
	void LoadUncompressedDDS(const void* pHeader, const char* pBuffer, unsigned int format);
	void LoadCompressedCubeMapDDS(const void* pHeader, const char* pBuffer, unsigned int format);
	void LoadUncompressedCubeMapDDS(const void* pHeader, const char* pBuffer, unsigned int format);

	void SaveCompressedDDS(FILE* pFile) {};
	void SaveUncompressedDDS(FILE* pFile);
	void SaveCompressedCubeMapDDS(FILE* pFile) {};
	void SaveUncompressedCubeMapDDS(FILE* pFile) {};

	void GenMipMaps();
};


#endif
