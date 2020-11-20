#include "Engine/Engine.h"
#include "../Textures.h"


void CTexture::LoadBMP(const char* cFileName)
{
	FILE* pFile;

	fopen_s(&pFile, cFileName, "rb");

	if (pFile == NULL)
	{
		fprintf(stderr, "Error : Could not find BMP file %s\n", cFileName);
		m_bIsLoaded = false;
		return;
	}

	unsigned char Header[54];
	unsigned char BitsPerPixels;

	fread_s(Header, 54, sizeof(unsigned char), 54, pFile);

	BitsPerPixels = Header[28];

	if (Header[0] != 'B' || Header[1] != 'M' || (BitsPerPixels != 24 && BitsPerPixels != 32))
	{
		fprintf(stderr, "Error : Could not load %s : Corrupted file format\n", cFileName);
		fclose(pFile);
		m_bIsLoaded = false;
		return;
	}

	m_nWidth = Header[18] + (Header[19] << 8);
	m_nHeight = Header[22] + (Header[23] << 8);

	unsigned int PixelsOffset = Header[10] + (Header[11] << 8);
	unsigned int Size = ((m_nWidth * BitsPerPixels + 31) / 32) * 4 * m_nHeight;

	char* pData = new char[Size];

	fread_s(pData, Size, sizeof(char), Size, pFile);

	glGenTextures(1, &m_nID);
	glBindTexture(GL_TEXTURE_2D, m_nID);
	glTexImage2D(GL_TEXTURE_2D, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, m_nWidth, m_nHeight, 0, BitsPerPixels == 24 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, pData);

	glGenerateTextureMipmap(m_nID);

	m_bIsLoaded = true;

	fclose(pFile);

	delete[] pData;
}


void CTexture::SaveBMP(const char* cFileName)
{

}
