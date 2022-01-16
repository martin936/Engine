#ifndef __SSR_H__
#define __SSR_H__


class CSSR
{
public:

	static void Init();

	static unsigned int GetZMips()
	{
		return ms_pZMips->GetID();
	}

private:

	static void RayTrace();
	static void RayTraceSDFReflections();
	static void LightSDFReflections();
	static void Resolve();
	static void ComputeBlurRadius();
	static void Blur();
	static void UpscaleDepth();
	static void BuildHiZ();
	static void Merge();

	static void HiZRayTracing();

	static CTexture* ms_pRayData;
	static CTexture* ms_pRayLength;
	static CTexture* ms_pResolvedColor;
	static CTexture* ms_pBlurredColor;
	static CTexture* ms_pZMips;
};


#endif
