#ifndef __SKYBOX_H__
#define __SKYBOX_H__


#include "Engine/Renderer/Renderer.h"


class CSkybox
{
public:

	static void Init();

	static unsigned int GetSkyboxTexture()
	{
		return ms_pSkybox->GetID();
	}

	static float GetSkyLightIntensity()
	{
		return ms_fSkyLightIntensity;
	}

	static void SetSkyLightIntensity(float fIntensity)
	{
		ms_fSkyLightIntensity = fIntensity;
	}

private:

	static float		ms_fSkyLightIntensity;

	static CTexture*	ms_pSkybox;
};


#endif
