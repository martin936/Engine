#include "Engine/Engine.h"
#include "../Textures.h"



#define FOURCC_DXT1		0x31545844
#define FOURCC_DXT2		0x32545844
#define FOURCC_DXT3		0x33545844
#define FOURCC_DXT4		0x34545844
#define FOURCC_DXT5		0x35545844
#define FOURCC_RGBA16F	0x71
#define FOURCC_RGBA32F	0x74

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


void CTexture::LoadUncompressedDDS(const void* pHeader, const unsigned char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount > 0 ? header->dwMipMapCount : 1;
	unsigned int nBytesPerPixel = m_nBitsPerPixel >> 3U;

	GLenum F = GL_RGBA;
	GLenum type = GL_FLOAT;

	if (format == GL_RGBA16F || format == GL_RGBA32F)
	{
		if (format == GL_RGBA16F)
			nBytesPerPixel = 8;
		else
			nBytesPerPixel = 16;
	}

	else
	{
		if (format == GL_RGB8)
			F = GL_BGR;

		else if (format == GL_RGBA8)
			F = GL_BGRA;

		else if (format == GL_R8)
			F = GL_R;

		type = GL_UNSIGNED_BYTE;
	}

	glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_2D, m_nID);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int offset = 0;

	for (unsigned int level = 0; level < mipMapCount && (m_nWidth > 1 && m_nHeight > 1); ++level)
	{
		unsigned int size = width * height * nBytesPerPixel;

		glTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, F, type, pBuffer + offset);

		offset += size;
		width /= 2;
		height /= 2;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}


void CTexture::LoadCompressedDDS(const void* pHeader, const unsigned char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int fourCC = header->ddspf.dwFourCC;

	unsigned int blockSize = (fourCC == FOURCC_DXT1) ? 8 : 16;

	glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_2D, m_nID);

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int offset = 0;

	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level)
	{
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height, 0, size, pBuffer + offset);

		offset += size;
		width >>= 1;
		height >>= 1;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}


void CTexture::LoadUncompressedCubeMapDDS(const void* pHeader, const unsigned char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int nBytesPerPixel;

	if (format == GL_RGBA16F)
		nBytesPerPixel = 8;
	else
		nBytesPerPixel = 16;

	glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, m_nID);

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

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, format, width, height, 0, GL_RGBA, GL_FLOAT, pBuffer + offset);

			offset += size;
			width >>= 1;
			height >>= 1;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


void CTexture::LoadCompressedCubeMapDDS(const void* pHeader, const unsigned char* pBuffer, unsigned int format)
{
	DDS_HEADER* header = (DDS_HEADER*)pHeader;

	unsigned int mipMapCount = header->dwMipMapCount;
	unsigned int fourCC = header->ddspf.dwFourCC;

	unsigned int blockSize = (fourCC == FOURCC_DXT1) ? 8 : 16;

	glGenTextures(1, &m_nID);

	glBindTexture(GL_TEXTURE_CUBE_MAP, m_nID);

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

			glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, format, width, height, 0, size, pBuffer + offset);

			offset += size;
			width >>= 1;
			height >>= 1;
		}
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


void CTexture::LoadDDS(const char * cFileName)
{
	DDS_HEADER header;

	FILE *pFile;

	pFile = fopen(cFileName, "rb");
	if (pFile == NULL)
	{
		fprintf(stderr, "Error : Could not find DDS file %s\n", cFileName);
		m_bIsLoaded = false;
		return;
	}

	char filecode[5] = "";
	fread(filecode, 1, 4, pFile);
	if (strncmp(filecode, "DDS ", 4) != 0)
	{
		fclose(pFile);
		fprintf(stderr, "Error : Corrupted file %s\n", cFileName);
		m_bIsLoaded = false;
		return;
	}

	fread(&header, 1, sizeof(header), pFile);

	m_nHeight = header.dwHeight;
	m_nWidth = header.dwWidth;

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int linearSize;
	unsigned int format;
	unsigned int fourCC;

	if (header.ddspf.dwFlags & DDPF_FOURCC)
	{
		fourCC = header.ddspf.dwFourCC;

		switch (fourCC)
		{
		case FOURCC_DXT1:
			format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			m_eFormat = e_DXT1;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT3:
			format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			m_eFormat = e_DXT3;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_DXT5:
			format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			m_eFormat = e_DXT5;
			m_nBitsPerPixel = 32;
			break;

		case FOURCC_RGBA16F:
			format = GL_RGBA16F;
			m_eFormat = e_R16G16B16A16_FLOAT;
			m_nBitsPerPixel = 64;
			break;

		case FOURCC_RGBA32F:
			format = GL_RGBA32F;
			m_eFormat = e_R32G32B32A32_FLOAT;
			m_nBitsPerPixel = 128;
			break;

		default:
			fprintf(stderr, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
			m_bIsLoaded = false;
			return;
		}
	}

	else if (header.ddspf.dwFlags & DDPF_RGB)
	{
		m_nBitsPerPixel = header.ddspf.dwRGBBitCount;

		switch (m_nBitsPerPixel)
		{
		case 24:
			format = GL_RGB8;
			m_eFormat = e_R8G8B8;
			break;

		case 32:
			format = GL_RGBA8;
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
			format = GL_R8;
			m_eFormat = e_R8;
			break;

		default:
			break;
		}
	}

	else
	{
		fprintf(stderr, "Failed to load Image: dds file format not supported (supported formats: DXT1, DXT3, DXT5, RGBA16, RGBA32)");
		m_bIsLoaded = false;
		return;
	}

	if (header.dwFlags & DDSD_LINEARSIZE)
	{
		linearSize = header.dwPitchOrLinearSize;

		if (fourCC != FOURCC_DXT1 && fourCC != FOURCC_DXT3 && fourCC != FOURCC_DXT5)
			linearSize *= header.dwHeight;
	}
	else
		linearSize = width * height * (m_nBitsPerPixel >> 3);	

	m_nMipMapCount = header.dwMipMapCount;

	m_bIsCubeMap = (header.dwCaps2 & 0x200) ? true : false;

	unsigned char * buffer;
	unsigned int bufsize;

	bufsize = m_nMipMapCount > 1 ? linearSize * 2 : linearSize;

	if (m_bIsCubeMap)
	{
		bufsize *= 6;
		m_eType = eCubeMap;
	}

	else
		m_eType = eTexture2D;

	buffer = new unsigned char[bufsize];
	fread(buffer, 1, bufsize, pFile);

	fclose(pFile);

	if (header.ddspf.dwFlags & DDPF_FOURCC && (fourCC == FOURCC_DXT1 || fourCC == FOURCC_DXT3 || fourCC == FOURCC_DXT5))
	{
		if (m_bIsCubeMap)
			LoadCompressedCubeMapDDS(&header, buffer, format);
		else
			LoadCompressedDDS(&header, buffer, format);
	}

	else
	{
		if (m_bIsCubeMap)
			LoadUncompressedCubeMapDDS(&header, buffer, format);
		else
			LoadUncompressedDDS(&header, buffer, format);
	}

	delete[] buffer;
}


void CTexture::SaveUncompressedDDS(FILE* pFile)
{
	glBindTexture(GL_TEXTURE_2D, m_nID);

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;

	for (int level = 0; level < m_nMipMapCount && (width || height); ++level)
	{
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		unsigned int size = width * height * (m_nBitsPerPixel >> 3);
		unsigned char* pBuffer = new unsigned char[size];

		glGetTexImage(GL_TEXTURE_2D, level, GL_RGBA, GL_FLOAT, pBuffer);

		fwrite(pBuffer, sizeof(unsigned char), size, pFile);

		delete[] pBuffer;

		width >>= 1;
		height >>= 1;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}


void CTexture::SaveCompressedDDS(FILE* pFile)
{
	glBindTexture(GL_TEXTURE_2D, m_nID);

	unsigned int width = m_nWidth;
	unsigned int height = m_nHeight;
	unsigned int blockSize = m_eFormat == e_DXT1 ? 8 : 16;

	for (int level = 0; level < m_nMipMapCount && (width || height); ++level)
	{
		if (width == 0)
			width = 1;
		if (height == 0)
			height = 1;

		unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

		unsigned char* pBuffer = new unsigned char[size];

		glGetCompressedTexImage(GL_TEXTURE_2D, level, pBuffer);

		fwrite(pBuffer, sizeof(unsigned char), size, pFile);

		delete[] pBuffer;
		
		width >>= 1;
		height >>= 1;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}


void CTexture::SaveUncompressedCubeMapDDS(FILE* pFile)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_nID);

	unsigned int format;

	if (m_eFormat == e_R16G16B16A16_FLOAT)
		format = GL_HALF_FLOAT;

	else
		format = GL_FLOAT;

	for (unsigned int face = 0; face < 6; ++face)
	{
		unsigned int width = m_nWidth;
		unsigned int height = m_nHeight;

		for (int level = 0; level < m_nMipMapCount && (width || height); ++level)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;

			unsigned int size = width * height * (m_nBitsPerPixel >> 3);
			unsigned char* pBuffer = new unsigned char[size]();
			pBuffer[0] = 1;

			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, GL_RGBA, format, pBuffer);

			fwrite(pBuffer, sizeof(unsigned char), size, pFile);

			delete[] pBuffer;

			width >>= 1;
			height >>= 1;
		}
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


void CTexture::SaveCompressedCubeMapDDS(FILE* pFile)
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_nID);

	unsigned int blockSize = m_eFormat == e_DXT1 ? 8 : 16;

	for (unsigned int face = 0; face < 6; ++face)
	{
		unsigned int width = m_nWidth;
		unsigned int height = m_nHeight;

		for (int level = 0; level < m_nMipMapCount && (width || height); ++level)
		{
			if (width == 0)
				width = 1;
			if (height == 0)
				height = 1;

			unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;

			unsigned char* pBuffer = new unsigned char[size];

			glGetCompressedTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, pBuffer);

			fwrite(pBuffer, sizeof(unsigned char), size, pFile);

			delete[] pBuffer;

			width >>= 1;
			height >>= 1;
		}
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


void CTexture::SaveDDS(const char * cFileName)
{
	DDS_HEADER header;

	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;

	if (m_eFormat == e_DXT1 || m_eFormat == e_DXT3 || m_eFormat == e_DXT5)
	{
		header.dwFlags |= DDSD_LINEARSIZE;
		header.dwPitchOrLinearSize = max(1, ((m_nWidth + 3) / 4) * ((m_nHeight + 3) / 4)) * (m_eFormat == e_DXT1 ? 8 : 16);
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
	header.dwMipMapCount = max(1, m_nMipMapCount - 1);
	header.ddspf.dwSize = 32;

	switch (m_eFormat)
	{
	case e_DXT1:
		header.dwFlags = DDPF_FOURCC;
		header.ddspf.dwFourCC = FOURCC_DXT1;
		break;

	case e_DXT3:
		header.dwFlags = DDPF_FOURCC;
		header.ddspf.dwFourCC = FOURCC_DXT3;
		break;

	case e_DXT5:
		header.dwFlags = DDPF_FOURCC;
		header.ddspf.dwFourCC = FOURCC_DXT5;
		break;

	case e_R16G16B16A16_FLOAT:
		header.dwFlags = DDPF_FOURCC;
		header.ddspf.dwFourCC = FOURCC_RGBA16F;
		break;

	case e_R32G32B32A32_FLOAT:
		header.dwFlags = DDPF_FOURCC;
		header.ddspf.dwFourCC = FOURCC_RGBA32F;
		break;

	default:
		fprintf(stderr, "Error : Incompatible surface format\n");
		return;
		break;
	}

	header.dwCaps = DDSCAPS_TEXTURE;

	if (m_nMipMapCount > 1 || m_bIsCubeMap)
	{
		header.dwCaps |= DDSCAPS_COMPLEX;

		if (m_nMipMapCount > 1)
			header.dwCaps |= DDSCAPS_MIPMAP;

		if (m_bIsCubeMap)
		{
			header.dwCaps2 =	DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX | DDSCAPS2_CUBEMAP_NEGATIVEY | DDSCAPS2_CUBEMAP_NEGATIVEZ | \
								DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_POSITIVEZ;
		}
	}

	FILE* pFile = fopen(cFileName, "wb+");
	if (pFile == NULL)
	{
		fprintf(stderr, "Error : Could not find DDS file %s\n", cFileName);
		m_bIsLoaded = false;
		return;
	}

	char filecode[5] = "DDS ";
	fwrite(filecode, 1, 4, pFile);
	fwrite(&header, sizeof(header), 1, pFile);

	if (m_eFormat == e_DXT1 || m_eFormat == e_DXT3 || m_eFormat == e_DXT5 || m_eFormat == e_DXT1_SRGB || m_eFormat == e_DXT3_SRGB || m_eFormat == e_DXT5_SRGB)
	{
		if (m_bIsCubeMap)
			SaveCompressedCubeMapDDS(pFile);
		else
			SaveCompressedDDS(pFile);
	}

	else
	{
		if (m_bIsCubeMap)
			SaveUncompressedCubeMapDDS(pFile);
		else
			SaveUncompressedDDS(pFile);
	}

	fclose(pFile);
}