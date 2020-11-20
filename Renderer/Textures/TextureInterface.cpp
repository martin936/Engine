#include "TextureInterface.h"
#include "Engine/Device/DeviceManager.h"


std::vector<CTexture*>	CTextureInterface::ms_pTextures;


void CTextureInterface::Init()
{
	ms_pTextures.clear();
}


void CTextureInterface::Terminate()
{
	int numTextures = static_cast<int>(ms_pTextures.size());

	for (int i = 0; i < numTextures; i++)
		delete ms_pTextures[i];

	ms_pTextures.clear();
}


unsigned int CTextureInterface::LoadTexture(const char* path, bool bSRGB)
{
	unsigned int numTextures = static_cast<unsigned int>(ms_pTextures.size());

	for (unsigned int i = 0; i < numTextures; i++)
	{
		if (!strcmp(ms_pTextures[i]->m_cPath, path))
			return ms_pTextures[i]->GetID();
	}

	CTexture* tex = new CTexture(path, bSRGB);

	return tex->GetID();
}


CTexture* CTextureInterface::GetTexture(unsigned int nTextureID)
{
	return ms_pTextures[nTextureID];
}


ETextureFormat CTextureInterface::GetTextureFormat(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return e_R8G8B8A8_SRGB;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetFormat();
}


ETextureType CTextureInterface::GetTextureType(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return eTexture2D;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetType();
}

unsigned int CTextureInterface::GetTextureWidth(unsigned int nTextureID, int nLevel)
{
	if (nTextureID == INVALIDHANDLE)
		return CDeviceManager::GetDeviceWidth();

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetWidth(nLevel);
}

unsigned int CTextureInterface::GetTextureHeight(unsigned int nTextureID, int nLevel)
{
	if (nTextureID == INVALIDHANDLE)
		return CDeviceManager::GetDeviceHeight();

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetHeight(nLevel);
}

unsigned int CTextureInterface::GetTextureDepth(unsigned int nTextureID, int nLevel)
{
	if (nTextureID == INVALIDHANDLE)
		return 1;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetDepth(nLevel);
}

unsigned int CTextureInterface::GetTextureArraySize(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return 1;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetArraySize();
}

unsigned int CTextureInterface::GetTextureMipCount(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return 1;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetMipMapCount();
}

size_t CTextureInterface::GetTextureSize(unsigned int nTextureID)
{
	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetSize();
}

unsigned int CTextureInterface::GetTextureBitsPerPixel(unsigned int nTextureID)
{
	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetBitsPerPixel();
}

unsigned int CTextureInterface::GetTextureSampleCount(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return 1;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetSampleCount();
}

unsigned int CTextureInterface::GetTextureSampleQuality(unsigned int nTextureID)
{
	if (nTextureID == INVALIDHANDLE)
		return 0;

	CTexture* pTex = GetTexture(nTextureID);

	return pTex->GetSampleQuality();
}


unsigned int CTextureInterface::GetCurrentState(unsigned int nTextureID)
{
	CTexture* pTex = GetTexture(nTextureID);

	return pTex->m_eState;
}

void CTextureInterface::SetCurrentState(unsigned int nTextureID, unsigned int eState)
{
	CTexture* pTex = GetTexture(nTextureID);

	pTex->m_eState = eState;
}


void CTextureInterface::SetTexture(unsigned int nTextureID, unsigned int nSlot, CShader::EShaderType eShaderStage, int nSlice, int nLevel)
{
	CTexture* pTex = GetTexture(nTextureID);

	pTex->SetTexture(nSlot, eShaderStage, nSlice, nLevel);
}


void CTextureInterface::SetRWTexture(unsigned int nTextureID, unsigned int nSlot, CShader::EShaderType eShaderStage, int nSlice, int nLevel)
{
	CTexture* pTex = GetTexture(nTextureID);

	pTex->SetRwTexture(nSlot, eShaderStage, nSlice, nLevel);
}

