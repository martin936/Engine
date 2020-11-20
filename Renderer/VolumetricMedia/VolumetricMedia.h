#ifndef __VOLUMETRIC_MEDIA_H__
#define __VOLUMETRIC_MEDIA_H__



class CVolumetricMedia
{
public:

	static void Init();

private:

	static void FillGrid();
	static void ApplyLights();
	static void IntegrateRays();
	static void Merge();

	static CTexture*		ms_pScatteringValue;
	static CTexture*		ms_pScatteredLight;
	static CTexture*		ms_pIntegratedLight;

	static unsigned int		ms_nFillGridConstantBuffer;
	static unsigned int		ms_nLightsConstantBuffer;
	static unsigned int		ms_nIntegrateRaysConstantBuffer;

	static ProgramHandle	ms_nFillGridPID;
	static ProgramHandle	ms_nApplyLightsPID;
	static ProgramHandle	ms_nIntegrateRaysPID;
	static ProgramHandle	ms_nMergePID;
};


#endif
