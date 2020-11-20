#ifndef __BLOOM_H__
#define __BLOOM_H__


class CBloom
{
public:

	static void Init();

	static void SetIntensity(float intensity)
	{
		ms_fIntensity = intensity;
	}

	static float GetIntensity()
	{
		return ms_fIntensity;
	}

private:

	static void DownscaleExtract();
	static void Downscale();
	static void Upscale();

	static float		ms_fIntensity;

	static CTexture*	ms_pDownscaleTargets;
};


#endif
