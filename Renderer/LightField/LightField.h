#ifndef __LIGHT_FIELD_H__
#define __LIGHT_FIELD_H__

#include "Engine/Device/ResourceManager.h"
#include "Engine/Renderer/Packets/Packet.h"


class CLightField
{
public:

	static void Init(int numProbesX, int numProbesY, int numProbesZ);
	static void InitRenderPasses();

	static void UpdateBeforeFlush();


	static void ShowIrradianceProbes(bool bShow)
	{
		ms_bShowLightField = bShow;
	}

	static bool ShouldShowIrradianceProbes()
	{
		return ms_bShowLightField;
	}

	static void Enable(bool bEnable)
	{
		ms_bEnable = bEnable;
	}

	static bool IsEnabled()
	{
		return ms_bEnable;
	}

	static unsigned int GetIrradianceField(int cascade)
	{
		return ms_LightFieldIrradiance[cascade]->GetID();
	}

	static unsigned int GetProbeMetadata(int cascade)
	{
		return ms_LightFieldMetaData[cascade]->GetID();
	}

	static unsigned int GetLightFieldOcclusion(int cascade, int index)
	{
		return ms_LightFieldOcclusion[cascade][index]->GetID();
	}

	static unsigned int GetLightFieldSH(int cascade)
	{
		return ms_LightFieldSH[cascade]->GetID();
	}

	static bool IsLightFieldGenerated()
	{
		return ms_bIsLightFieldGenerated;
	}

	static void SetCenter(float3& center);

	static void SetSize(int cascade, float3& size)
	{
		ms_Size[cascade] = size;

		float3 cell = ms_Size[cascade] / float3((float)ms_nNumProbes[0], (float)ms_nNumProbes[1], (float)ms_nNumProbes[2]);
	}

	static float3 GetCenter(int cascade)
	{
		return ms_Center4EngineFlush[cascade];
	}

	static float3 GetSize(int cascade)
	{
		return ms_Size[cascade];
	}

	static void SetProbeDisplaySize(float meters)
	{
		ms_fProbesDisplaySize = meters;
	}

	static float GetProbeDisplaySize()
	{
		return ms_fProbesDisplaySize;
	}

	static void StartGeneration()
	{
		ms_bIsLightFieldGenerated = false;
	}

	static int			ms_nNumProbes[3];
	static int			ms_nTotalNumProbes;

	static int			ms_nNumRenderedProbes;
	static int			ms_nNumProbesInBatch;
	   
	static bool			ms_bIsLightFieldGenerated;

	static const int	ms_NumCascades = 2;

private:

	static void			UpdateProbePosition(void* pParam);
	static void			ComputeOcclusion(void* pParam);
	static void			ReprojectLightField(void* pParam);
	static void			RayMarchSamples(void* pParam);
	static void			LightSamples(void* pParam);

	static void			UpdateLightField(void* pParam);
	static void			UpdateLightFieldBorder(void* pParam);
	static void			ShowLightField();

	static void			RayTraceReflections();
	static void			LightReflections();
	static void			ApplyReflections();

	static bool			ms_bShowLightField;
	static bool			ms_bEnable;
	static bool			ms_bRefreshOcclusion[ms_NumCascades];
	static float		ms_fProbesDisplaySize;

	static CTexture*	ms_LightFieldSH[ms_NumCascades];
	static CTexture*	ms_LightFieldIrradiance[ms_NumCascades];
	static CTexture*	ms_LightFieldMetaData[ms_NumCascades];
	static CTexture*	ms_LightFieldOcclusion[ms_NumCascades][2];

	static CTexture*	ms_SurfelDist[ms_NumCascades];
	static CTexture*	ms_SurfelIrradiance[ms_NumCascades];

	static CTexture*	ms_RayData;
	static CTexture*	ms_RayColor;

	static float3		ms_Center[ms_NumCascades];
	static float3		ms_Center4EngineFlush[ms_NumCascades];
	static float3		ms_LastCenter4EngineFlush[ms_NumCascades];
	static float3		ms_Size[ms_NumCascades];
};


#endif

