#ifndef __SDFGI_H__
#define __SDFGI_H__

#include "Engine/Renderer/Textures/TextureInterface.h"

class CSDFGI
{
public:

	static void Init();

	static unsigned int GetIrradianceCache()
	{
		return ms_pIrradianceCache->GetID();
	}

private:

	static void BuildIrradianceCache();
	static void RayTraceGI();
	static void RayTraceGIFull();
	static void UpdateHistory();
	static void FilterMoments();
	static void ATrousFiltering(void* pData);
	static void CopyHistory();
	static void Merge();

	static CTexture* ms_pUnfilteredChromaHistoryIn;
	static CTexture* ms_pUnfilteredLumaHistoryIn;
	static CTexture* ms_pUnfilteredChromaHistoryOut;
	static CTexture* ms_pUnfilteredLumaHistoryOut;
	static CTexture* ms_pNoisyChroma;
	static CTexture* ms_pNoisyLumaSH;
	static CTexture* ms_pFilteredChromaIn;
	static CTexture* ms_pFilteredLumaIn;
	static CTexture* ms_pFilteredChromaOut;
	static CTexture* ms_pFilteredLumaOut;
	static CTexture* ms_pIrradianceCache;
	static CTexture* ms_pMomentsHistoryIn;
	static CTexture* ms_pMomentsHistoryOut;
	static CTexture* ms_pLastFlatNormal;
};


#endif

