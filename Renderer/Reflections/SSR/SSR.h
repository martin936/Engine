#ifndef __SSR_H__
#define __SSR_H__


class CSSR
{
public:

	static void Init();
	static void Terminate();

	static void Apply(CTexture* pTarget);

private:

	static void AllocateRays();
	static void Raytrace();
	static void Resolve(unsigned int nSrc);
	static void TemporalFiltering();
	static void BuildHiZ();
	static void BilateralCleanUp();
	static void Merge(CTexture* pTarget);

	static void RayMarching();
	static void HiZRayTracing();

	static CTexture* ms_pRayDatas;
	static CTexture* ms_pResolveTarget;
	static CTexture* ms_pZMips;
	static CTexture* ms_pHistory;
	static CTexture* ms_pBlurTarget;
};


#endif
