#ifndef __TONE_Mapping_H__
#define __TONE_MAPPING_H__


class CToneMapping
{
public:

	static void Init();

	static CTexture*		ms_p3DLUT;
	static CTexture*		ms_pContrastLUT[7];

	static unsigned int		GetAutoExposureTarget()
	{
		return ms_pAETarget->GetID();
	}

	static void SetContrastLevel(int level)
	{
		ASSERT(level >= 0 && level < 7);
		ms_nCurrentContrast = level;
	}

	static int GetContrastLevel()
	{
		return ms_nCurrentContrast;
	}

	static float GetEyeAdaptationFactor()
	{
		return ms_fEyeAdaptation;
	}

	static float GetLowestBlack()
	{
		return ms_fLowestBlack;
	}

	static float GetHighestWhite()
	{
		return ms_fHighestWhite;
	}

	static float GetEVBias()
	{
		return ms_fEVBias;
	}

	static void SetEyeAdaptationFactor(float factor)
	{
		ms_fEyeAdaptation = factor;
	}

	static void SetLowestBlack(float black)
	{
		ms_fLowestBlack = black;
	}

	static void SetHighestWhite(float white)
	{
		ms_fHighestWhite = white;
	}

	static void SetEVBias(float bias)
	{
		ms_fEVBias = bias;
	}

private:

	static float			ms_fEyeAdaptation;
	static float			ms_fLowestBlack;
	static float			ms_fHighestWhite;
	static float			ms_fEVBias;

	static CTexture*		ms_pHDHTarget;
	static CTexture*		ms_pAETarget;

	static int ms_nCurrentContrast;

	static void LoadSPI3D(const char* pcFileName);
	static void LoadCUBE(const char* pcFileName);
	static void LoadSPI1D(const char* pcFileName);
};


#endif
