#ifndef __LIGHT_FIELD_H__
#define __LIGHT_FIELD_H__

#include "Engine/Device/ResourceManager.h"
#include "Engine/Renderer/Packets/Packet.h"


class CLightField
{
public:

	static void Init(int numProbesX, int numProbesY, int numProbesZ);
	static void InitRenderPasses();

	static void Generate();
	
	static int	UpdateShader(Packet* packet, void* pData);

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

	static unsigned int GetIrradianceField()
	{
		return ms_LightFieldIrradiance->GetID();
	}

	static unsigned int GetIrradianceGradient()
	{
		return ms_LightFieldGradient->GetID();
	}

	static unsigned int GetProbeMetadata()
	{
		return ms_LightFieldMetaData->GetID();
	}

	static unsigned int GetFieldDepth()
	{
		return ms_LightFieldDepth->GetID();
	}

	static unsigned int GetFieldDepthMaps()
	{
		return ms_pLightFieldDepthMaps->GetID();
	}

	static bool IsLightFieldGenerated()
	{
		return ms_bIsLightFieldGenerated;
	}

	static void SetCenter(float3& center)
	{
		ms_Center = center;
	}

	static void SetSize(float3& size)
	{
		ms_Size = size;

		float3 cell = ms_Size / float3((float)ms_nNumProbes[0], (float)ms_nNumProbes[1], (float)ms_nNumProbes[2]);

		ms_fMinCellAxis = MIN(cell.x, MIN(cell.y, cell.z));
	}

	static void SetBias(float bias)
	{
		ms_fBias = bias;
	}

	static float3 GetCenter()
	{
		return ms_Center;
	}

	static float3 GetSize()
	{
		return ms_Size;
	}

	static float GetMinCellAxis()
	{
		return ms_fMinCellAxis;
	}

	static float GetBias()
	{
		return ms_fBias;
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

private:

	static void			BuildLightField();
	static void			WriteOctahedronMaps();
	static void			ReduceDepthMaps();
	static void			ComputeLightFieldSamples();
	static void			UpdateLightField();
	static void			ComputeLightFieldGradient();
	static void			UpdateFieldDepth();
	static void			UpdateLightFieldBorder();
	static void			ShowLightField();
	static void			RayTraceLightField();
	static void			ComputeReflections();

	static bool			ms_bShowLightField;
	static bool			ms_bEnable;

	static CTexture*	ms_LightFieldIrradiance;
	static CTexture*	ms_LightFieldGradient;
	static CTexture*	ms_LightFieldMetaData;
	static CTexture*	ms_LightFieldDepth;

	static CTexture*	ms_SurfelIrradiance;
	static CTexture*	ms_SurfelDepth;

	static CTexture*	ms_pLightFieldDepthMaps;
	static CTexture*	ms_pLightFieldLowDepthMaps;
	static CTexture*	ms_pLightFieldGBuffer;

	static CTexture*	ms_pLightFieldDepthCubeMaps;
	static CTexture*	ms_pLightFieldGBufferCubeMaps;

	static CTexture*	ms_pLightFieldRayData;

	static float		ms_fMinCellAxis;
	static float		ms_fBias;

	static float3		ms_Center;
	static float3		ms_Size;
};


#endif

