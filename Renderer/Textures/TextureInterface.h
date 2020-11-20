#ifndef __TEXTURES_INTERFACE_H__
#define __TEXTURES_INTERFACE_H__

#include "Textures.h"
#include <vector>

class CTextureInterface
{
	friend class CTexture;
public:

	static void Init();
	static void Terminate();

	static unsigned int		LoadTexture(const char* path, bool bSRGB = false);

	static CTexture*		GetTexture(unsigned int nTextureID);

	static ETextureFormat	GetTextureFormat(unsigned int nTextureID);
	static ETextureType		GetTextureType(unsigned int nTextureID);

	static unsigned int GetCurrentState(unsigned int nTextureID);
	static void SetCurrentState(unsigned int nTextureID, unsigned int eState);

	static unsigned int		GetTextureWidth(unsigned int nTextureID, int nLevel = -1);
	static unsigned int		GetTextureHeight(unsigned int nTextureID, int nLevel = -1);
	static unsigned int		GetTextureDepth(unsigned int nTextureID, int nLevel = -1);
	static unsigned int		GetTextureArraySize(unsigned int nTextureID);

	static unsigned int		GetTextureMipCount(unsigned int nTextureID);
	static size_t			GetTextureSize(unsigned int nTextureID);
	static unsigned int		GetTextureBitsPerPixel(unsigned int nTextureID);

	static unsigned int		GetTextureSampleCount(unsigned int nTextureID);
	static unsigned int		GetTextureSampleQuality(unsigned int nTextureID);

	static void				SetTexture(unsigned int nTextureID, unsigned int nSlot, CShader::EShaderType eShaderStage = CShader::EShaderType::e_FragmentShader, int nSlice = -1, int nLevel = -1);
	static void				SetRWTexture(unsigned int nTextureID, unsigned int nSlot, CShader::EShaderType eShaderStage = CShader::EShaderType::e_FragmentShader, int nSlice = -1, int nLevel = -1);

	//static void				SetTarget(unsigned int nTextureID, unsigned int nSlot, int nSlice = 0, int nLevel = 0, int nFace = 0);
	//static void				SetDepthStencil(unsigned int nTextureID, int nSlice = 0, int nLevel = 0, int nFace = 0);

#ifdef __VULKAN__
	static void				GetTextureImageView();
#endif

private:

	static std::vector<CTexture*>		ms_pTextures;
};


#endif
