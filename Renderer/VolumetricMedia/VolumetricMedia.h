#ifndef __VOLUMETRIC_MEDIA_H__
#define __VOLUMETRIC_MEDIA_H__



class CVolumetricMedia
{
public:

	static void Init();

	static void SetScattering(float3& scattering)
	{
		ms_Scattering = scattering;
	}

	static void SetAbsorption(float absorption)
	{
		ms_Absorption = absorption;
	}

	static float3 GetScattering()
	{
		return ms_Scattering;
	}

	static float GetAbsoption()
	{
		return ms_Absorption;
	}

private:

	static void FillGrid();
	static void ComputeScatteredLight();
	static void IntegrateRays();
	static void Merge();

	static float3			ms_Scattering;
	static float			ms_Absorption;

	static CTexture*		ms_pScatteringValue;
	static CTexture*		ms_pScatteredLight;
	static CTexture*		ms_pScatteredLightHistory;
	static CTexture*		ms_pIntegratedLight;
};


#endif
