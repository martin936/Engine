#ifndef __DOF_H__
#define __DOF_H__


class CDOF
{
public:

	static void Init();

	inline static void SetPlaneInFocus(float dist)
	{
		ms_fFocalLength = dist;
	}

	inline static float GetPlaneInFocus()
	{
		return ms_fFocalLength;
	}

	inline static void SetAperture(float aperture)
	{
		ms_fAperture = aperture;
	}

	inline static float GetAperture()
	{
		return ms_fAperture;
	}

private:

	static CTexture*	ms_pCoCTiles;
	static CTexture*	ms_pBlurredCoCTiles;
	static CTexture*	ms_pPresortTarget;
	static CTexture*	ms_pPrefilteredColor;
	static CTexture*	ms_pFilterTarget;

	static CTexture*	ms_pPresortTarget_Full;
	static CTexture*	ms_pPresortHistoryTarget_Full;
	static CTexture*	ms_pPrefilteredColor_Full;
	static CTexture*	ms_pFinalTarget_Full;

	static float		 ms_fFocalLength;
	static float		 ms_fAperture;
};


#endif
