#ifndef __HBIL_H__
#define __HBIL_H__



class CHBIL
{
public:

	static void Init();
	static void Terminate();

	static void Apply(SRenderTarget* pIrradianceTarget, unsigned int nZBuffer);

private:

	static void ReprojectIrradiance(unsigned int nSrc, unsigned int nZBuffer);
	static void InterleavedHBIL(unsigned int nZBuffer);
	static void Accumulate(unsigned int nZBuffer);
	static void Merge(SRenderTarget* pTarget);

	static SRenderTarget*	ms_pTotalIrradiance;
	static SRenderTarget*	ms_pReprojectedIrradiance;
	static SRenderTarget*	ms_pAccumulatedIrradiance;
	static SRenderTarget*	ms_pInterleavedIrradiance;

	static unsigned int		ms_nSamplingPatternIndex;
};


#endif
