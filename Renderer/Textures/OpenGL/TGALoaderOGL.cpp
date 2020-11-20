#include "Engine/Engine.h"
#include "../Textures.h"


typedef struct
{
	char r;
	char g;
	char b;
	char a;
}SPixel;


char* CTexture::LoadTGAData(const char* cFileName, char* pBpp)
{
	unsigned char Header[18] = { 0 };

	static char DeCompressed[12] = { 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	static char IsCompressed[12] = { 0x0, 0x0, 0xA, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
	char BitsPerPixels;
	size_t Size;
	char* pData;

	FILE* pFile;
	fopen_s(&pFile, cFileName, "rb");

	if (pFile == NULL)
	{
		fprintf(stderr, "Error : Could not find TGA file %s\n", cFileName);
		m_bIsLoaded = false;
		return NULL;
	}

	fread_s(Header, 18, sizeof(unsigned char), 18, pFile);

	if (!memcmp(DeCompressed, Header, 12))
	{
		BitsPerPixels = Header[16];
		m_nWidth = Header[13] * 256 + Header[12];

		m_nHeight = Header[15] * 256 + Header[14];
		Size = ((m_nWidth * BitsPerPixels + 31) / 32) * 4 * m_nHeight;

		if (BitsPerPixels != 24 && BitsPerPixels != 32)
		{
			fprintf(stderr, "Error : Could not load %s : Corrupted file format\n", cFileName);
			fclose(pFile);
			m_bIsLoaded = false;
			return NULL;
		}

		pData = new char[Size];

		fread_s(pData, Size, sizeof(char), Size, pFile);
	}

	else if (!memcmp(IsCompressed, Header, 12))
	{
		BitsPerPixels = Header[16];
		m_nWidth = Header[13] * 256 + Header[12];
		m_nHeight = Header[15] * 256 + Header[14];
		Size = ((m_nWidth * BitsPerPixels + 31) / 32) * 4 * m_nHeight;

		if (BitsPerPixels != 24 && BitsPerPixels != 32)
		{
			fprintf(stderr, "Error : Could not load %s : Corrupted file format\n", cFileName);
			fclose(pFile);
			m_bIsLoaded = false;
			return NULL;
		}

		SPixel pixel;
		int CurrentByte = 0;
		size_t CurrentPixel = 0;
		unsigned char ChunkHeader = 0;
		int BytesPerPixel = BitsPerPixels >> 3;

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
					if (BitsPerPixels > 24) pData[CurrentByte++] = pixel.a;
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
					if (BitsPerPixels > 24) pData[CurrentByte++] = pixel.a;
				}
			}

		} while (CurrentPixel < m_nWidth * m_nHeight);
	}
	else
	{
		fprintf(stderr, "Error : Could not load %s : Corrupted file format\n", cFileName);
		fclose(pFile);
		m_bIsLoaded = false;
		return NULL;
	}

	*pBpp = BitsPerPixels;

	fclose(pFile);

	return pData;
}


void CTexture::LoadTGA(const char* cFileName)
{
	char BitsPerPixels;

	char* pData = LoadTGAData(cFileName, &BitsPerPixels);

	if (pData == NULL)
		return;

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_2D, m_nID);
	glTexImage2D(GL_TEXTURE_2D, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, m_nWidth, m_nHeight, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pData);

	glGenerateTextureMipmap(m_nID);

	m_bIsLoaded = true;

	delete[] pData;
}


void CTexture::SaveTGA(const char* cFileName, bool bUseRLE)
{

}


CTexture* CTexture::LoadCubeMapFromTGA(const char* cPosX, const char* cNegX, const char* cPosY, const char* cNegY, const char* cPosZ, const char* cNegZ)
{
	CTexture* pTexture = new CTexture();

	char BitsPerPixels;

	char* pData[6];

	pData[0] = pTexture->LoadTGAData(cPosX, &BitsPerPixels);
	pData[1] = pTexture->LoadTGAData(cNegX, &BitsPerPixels);
	pData[2] = pTexture->LoadTGAData(cPosY, &BitsPerPixels);
	pData[3] = pTexture->LoadTGAData(cNegY, &BitsPerPixels);
	pData[4] = pTexture->LoadTGAData(cPosZ, &BitsPerPixels);
	pData[5] = pTexture->LoadTGAData(cNegZ, &BitsPerPixels);

	glGenTextures(1, &pTexture->m_nID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, pTexture->m_nID);

	for (int i = 0; i < 6; i++)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, pTexture->m_nWidth, pTexture->m_nHeight, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pData[i]);

	glGenerateTextureMipmap(pTexture->m_nID);

	pTexture->m_bIsLoaded = true;
	pTexture->m_eType = eCubeMap;

	for (int i = 0; i < 6; i++)
		delete[] pData[i];

	return pTexture;
}