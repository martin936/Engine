#include "Engine/Engine.h"
#include "../Textures.h"


unsigned int CTexture::ms_nActiveSlot = INVALIDHANDLE;
unsigned int CTexture::ms_nActiveTexture[16] = { INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE };
unsigned int CTexture::ms_nActiveTextureArray[16] = { INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE };
unsigned int CTexture::ms_nActiveCubeMap[16] = { INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE, INVALIDHANDLE };



void CTexture::InitFrame()
{
	for (int i = 0; i < 16; i++)
	{
		ms_nActiveTexture[i] = INVALIDHANDLE;
		ms_nActiveCubeMap[i] = INVALIDHANDLE;
		ms_nActiveTextureArray[i] = INVALIDHANDLE;

		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	for (int i = 0; i < 8; i++)
		glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

	ms_nActiveSlot = INVALIDHANDLE;

	CSampler::InitFrame();
}



void CTexture::GetFormat(ETextureFormat eFormat, void* outFormat, void* outType)
{
	GLenum format = GL_RGBA;
	GLenum type = GL_RGBA;

	switch (eFormat)
	{
	case e_R8G8B8A8:
		format = GL_RGBA8;
		type = GL_RGBA;
		//m_nBitsPerPixel = 32;
		break;

	case e_R8G8B8A8_SNORM:
		format = GL_RGBA8_SNORM;
		type = GL_RGBA;
		//m_nBitsPerPixel = 32;
		break;

	case e_R8G8:
		format = GL_RG8;
		type = GL_RG;
		//m_nBitsPerPixel = 16;
		break;

	case e_R8G8_SNORM:
		format = GL_RG8_SNORM;
		type = GL_RG;
		//m_nBitsPerPixel = 16;
		break;

	case e_R8G8B8:
		format = GL_RGB8;
		type = GL_RGB;
		//m_nBitsPerPixel = 24;
		break;

	case e_R8:
		format = GL_R8;
		type = GL_RED;
		//m_nBitsPerPixel = 8;
		break;

	case e_R8G8B8_SNORM:
		format = GL_RGB8_SNORM;
		type = GL_RGB;
		//m_nBitsPerPixel = 24;
		break;

	case e_R16G16B16A16_FLOAT:
		format = GL_RGBA16F;
		type = GL_RGBA;
		//m_nBitsPerPixel = 64;
		break;

	case e_R16G16B16_FLOAT:
		format = GL_RGB16F;
		type = GL_RGB;
		//m_nBitsPerPixel = 48;
		break;

	case e_R32_FLOAT:
		format = GL_R32F;
		type = GL_RED;
		//m_nBitsPerPixel = 32;
		break;

	case e_R16_FLOAT:
		format = GL_R16F;
		type = GL_RED;
		//m_nBitsPerPixel = 16;
		break;

	case e_R16G16_FLOAT:
		format = GL_RG16F;
		type = GL_RG;
		//m_nBitsPerPixel = 32;
		break;

	case e_R10G10B10A2:
		format = GL_RGB10_A2;
		type = GL_RGBA;
		//m_nBitsPerPixel = 32;
		break;

	case e_R11G11B10_FLOAT:
		format = GL_R11F_G11F_B10F;
		type = GL_RGB;
		//m_nBitsPerPixel = 32;
		break;

	case e_R8G8B8_SRGB:
		format = GL_SRGB8;
		type = GL_RGB;
		//m_nBitsPerPixel = 24;
		break;

	case e_R16_DEPTH:
		format = GL_DEPTH_COMPONENT16;
		type = GL_RGBA;
		//m_nBitsPerPixel = 16;
		break;

	case e_R32G32B32A32_FLOAT:
		format = GL_RGBA32F;
		type = GL_RGBA;
		//m_nBitsPerPixel = 128;
		break;

	case e_R32G32B32A32_UINT:
		format = GL_RGBA32UI;
		type = GL_RGBA;
		//m_nBitsPerPixel = 128;
		break;

	case e_R8_UINT:
		format = GL_R8UI;
		type = GL_RED;
		//m_nBitsPerPixel = 8;
		break;

	case e_R16_UINT:
		format = GL_R16UI;
		type = GL_RED;
		//m_nBitsPerPixel = 16;
		break;

	case e_R32_UINT:
		format = GL_R32UI;
		type = GL_RED;
		//m_nBitsPerPixel = 32;
		break;

	case e_R16G16_UINT:
		format = GL_RG16UI;
		type = GL_RG;
		break;

	case e_R8G8_UINT:
		format = GL_RG8UI;
		type = GL_RG;
		break;

	default:
		break;
	}

	*((GLenum*)outFormat) = format;
	*((GLenum*)outType) = type;
}



void CTexture::GenerateTexture2D()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_2D, m_nID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, m_nWidth, m_nHeight, 0, type, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_bIsCubeMap = false;

	m_bIsLoaded = true;
}


void CTexture::GenerateTexture3D()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_3D, m_nID);
	glTexImage3D(GL_TEXTURE_3D, 0, format, m_nWidth, m_nHeight, m_nDepth, 0, type, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_bIsCubeMap = false;

	m_bIsLoaded = true;
}


void CTexture::GenerateTextureStorage2D()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_2D, m_nID);
	glTexStorage2D(GL_TEXTURE_2D, 1, format, m_nWidth, m_nHeight);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0.f);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0.f);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 1.f);

	m_bIsCubeMap = false;

	m_bIsLoaded = true;
}


void CTexture::GenerateTextureStorage3D()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_3D, m_nID);
	glTexStorage3D(GL_TEXTURE_3D, 1, format, m_nWidth, m_nHeight, m_nDepth);

	glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_LOD, 0.f);
	glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, 0.f);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_3D, 0);

	m_bIsCubeMap = false;

	m_bIsLoaded = true;
}


void CTexture::SetMip(float fMip)
{
	glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MIN_LOD, fMip);
	glTexParameterf(GL_TEXTURE_3D, GL_TEXTURE_MAX_LOD, fMip);
}


void CTexture::UnbindImageSlot(unsigned int nSlot)
{
	glBindImageTexture(nSlot, 0, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
}


void CTexture::BindImage(unsigned int nSlot, EBufferAccess eAccess, int nLayer)
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	GLboolean layered = ((m_eType == eTexture3D || m_eType == eTextureStorage3D || m_eType == eTextureArray || m_eType == eTextureStorage2DArray) ? GL_TRUE : GL_FALSE);

	switch (eAccess)
	{
	case e_ReadOnly:
		glBindImageTexture(nSlot, m_nID, 0, layered, 0, GL_READ_ONLY, format);
		break;

	case e_WriteOnly:
		glBindImageTexture(nSlot, m_nID, 0, layered, 0, GL_WRITE_ONLY, format);
		break;

	case e_ReadWrite:
		glBindImageTexture(nSlot, m_nID, 0, layered, 0, GL_READ_WRITE, format);
		break;

	default:
		break;
	}
}


void CTexture::GenerateTextureArray()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, m_nID);
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, format, m_nWidth, m_nHeight, m_nArrayDim);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_bIsCubeMap = false;

	m_bIsLoaded = true;
}


void CTexture::GenerateCubeMap()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_nID);

	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, m_nWidth, m_nHeight, 0, type, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_bIsLoaded = true;
}


void CTexture::GenerateCubeMapArray()
{
	GLenum format, type;
	GetFormat(m_eFormat, &format, &type);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, m_nID);
	glTexStorage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 1, format, m_nWidth, m_nHeight, m_nArrayDim * 6);
	//glTexSubImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, 0, 0, 0, m_nWidth, m_nHeight, m_nArrayDim * 6, type, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	m_bIsLoaded = true;
}


CTexture::CTexture(int nWidth, int nHeight, ETextureFormat eFormat, ETextureType eType, int nDepth, int nArrayDim, bool bGenMipMaps) : CTexture()
{
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nDepth = nDepth;
	m_eType = eType;
	m_nArrayDim = nArrayDim;
	m_eFormat = eFormat;

	switch (eType)
	{
	case eTexture2D:
		GenerateTexture2D();
		break;

	case eTextureArray:
		GenerateTextureArray();
		break;

	case eTextureStorage2D:
		GenerateTextureStorage2D();
		break;

	case eCubeMap:
		GenerateCubeMap();
		break;

	case eCubeMapArray:
		GenerateCubeMapArray();
		break;

	case eTexture3D:
		GenerateTexture3D();
		break;

	case eTextureStorage3D:
		GenerateTextureStorage3D();
		break;

	default:
		break;
	}

	if (bGenMipMaps)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1000);
		GenMipMaps();
	}

	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		m_nMipMapCount = 1;
	}
}


CTexture::CTexture(const char* cFileName, ETextureFormat eFormat) : CTexture()
{
	m_eType = eTexture2D;

	if (strstr(cFileName, ".bmp") != NULL)
		LoadBMP(cFileName);

	else if (strstr(cFileName, ".tga") != NULL)
		LoadTGA(cFileName);

	else if (strstr(cFileName, ".dds") != NULL)
		LoadDDS(cFileName);

	else
		fprintf(stderr, "Error : Unsupported texture format : %s\n", cFileName);
}


CTexture::CTexture()
{
	m_bIsLoaded = false;
	m_eType = eTexture2D;

	ms_pTextures.push_back(this);
}


void CTexture::GenMipMaps()
{
	if (m_eType != eTexture2D)
	{
		glGenerateTextureMipmap(m_nID);

		int width = m_nWidth;
		int height = m_nHeight;

		m_nMipMapCount = 1;

		while (width || height)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;

			m_nMipMapCount++;

			width >>= 1;
			height >>= 1;
		}
	}

	else
	{
		m_nMipMapCount = 0;

		glBindTexture(GL_TEXTURE_2D, m_nID);

		unsigned int width = m_nWidth;
		unsigned int height = m_nHeight;
		unsigned int offset = 0;

		GLenum format, type;

		GetFormat(m_eFormat, &format, &type);

		while (width > 1 && height > 1)
		{
			width /= 2;
			height /= 2;
			m_nMipMapCount++;

			glTexImage2D(GL_TEXTURE_2D, m_nMipMapCount, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}

		glBindTexture(GL_TEXTURE_2D, 0);
	}
}


void CTexture::Save(const char* cFileName)
{
	if (strstr(cFileName, ".bmp") != NULL)
		SaveBMP(cFileName);

	else if (strstr(cFileName, ".tga") != NULL)
		SaveTGA(cFileName);

	else if (strstr(cFileName, ".dds") != NULL)
		SaveDDS(cFileName);

	else
		fprintf(stderr, "Error : Unsupported texture format : %s\n", cFileName);
}


void CTexture::SetTexture(ProgramHandle pid, unsigned int textureId, int slot)
{

	if (ms_nActiveTexture[slot] != textureId)
	{
		ms_nActiveTexture[slot] = textureId;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		if (textureId == INVALIDHANDLE)
			glBindTexture(GL_TEXTURE_2D, 0);
		else
			glBindTexture(GL_TEXTURE_2D, textureId);
	}

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::SetTexture3d(ProgramHandle pid, unsigned int textureId, int slot)
{

	if (ms_nActiveTexture[slot] != textureId)
	{
		ms_nActiveTexture[slot] = textureId;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		if (textureId == INVALIDHANDLE)
			glBindTexture(GL_TEXTURE_3D, 0);
		else
			glBindTexture(GL_TEXTURE_3D, textureId);
	}

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::SetTexture(ProgramHandle pid, unsigned int textureId, int slot, const char* cName)
{
	if (ms_nActiveTexture[slot] != textureId)
	{
		ms_nActiveTexture[slot] = textureId;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		glBindTexture(GL_TEXTURE_2D, textureId);
	}

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::SetTextureArray(ProgramHandle pid, unsigned int textureId, int slot)
{
	if (ms_nActiveTextureArray[slot] != textureId)
	{
		ms_nActiveTextureArray[slot] = textureId;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		glBindTexture(GL_TEXTURE_2D_ARRAY, textureId);
	}

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::SetCubeMap(ProgramHandle pid, unsigned int textureId, int slot)
{
	if (ms_nActiveCubeMap[slot] != textureId)
	{
		ms_nActiveCubeMap[slot] = textureId;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
	}

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::SetCubeMapArray(ProgramHandle pid, unsigned int textureId, int slot)
{
	if (ms_nActiveSlot != slot)
	{
		ms_nActiveSlot = slot;

		glActiveTexture(GL_TEXTURE0 + slot);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textureId);

	if (textureId != INVALIDHANDLE)
	{
		glUniform1i(slot, slot);
	}
}


void CTexture::UnsetCubeMap(int slot)
{
	if (ms_nActiveCubeMap[slot] != INVALIDHANDLE)
	{
		ms_nActiveCubeMap[slot] = INVALIDHANDLE;

		if (ms_nActiveSlot != slot)
		{
			ms_nActiveSlot = slot;

			glActiveTexture(GL_TEXTURE0 + slot);
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

		glDisable(GL_TEXTURE_CUBE_MAP);
	}
}


CTexture::~CTexture()
{
	if (m_bIsLoaded)
	{
		glDeleteTextures(1, &m_nID);
		m_nID = 0;

		m_bIsLoaded = false;
	}
}