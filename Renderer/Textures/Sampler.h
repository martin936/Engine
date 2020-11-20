#ifndef RENDERER_SAMPLER_INC
#define RENDERER_SAMPLER_INC


class CSampler
{
	friend class CTexture;

public:

	enum ESamplerState
	{
		e_MinMag_Point_Mip_None_UVW_Clamp = 0,
		e_MinMag_Point_Mip_Fixed_UVW_Clamp,
		e_MinMag_Point_Mip_None_UVW_Wrap,
		e_MinMag_Point_Mip_None_UVW_Mirror,

		e_MinMagMip_Point_UVW_Clamp,
		e_MinMagMip_Point_UVW_Wrap,
		e_MinMagMip_Point_UVW_Mirror,

		e_MinMag_Linear_Mip_None_UVW_Clamp,
		e_MinMag_Linear_Mip_Fixed_UVW_Clamp,
		e_MinMag_Linear_Mip_None_UVW_Wrap,
		e_MinMag_Linear_Mip_None_UVW_Mirror,

		e_MinMagMip_Linear_UVW_Clamp,
		e_MinMagMip_Linear_UVW_Wrap,
		e_MinMagMip_Linear_UVW_Mirror,

		e_Anisotropic_Point_UVW_Clamp,
		e_Anisotropic_Point_UVW_Wrap,
		e_Anisotropic_Point_UVW_Mirror,

		e_Anisotropic_Linear_UVW_Clamp,
		e_Anisotropic_Linear_UVW_Wrap,
		e_Anisotropic_Linear_UVW_Mirror,

		e_ZComparison_Linear_UVW_Clamp,

		e_NbSamplers,
		e_Invalid
	};

private:

	enum EFilter
	{
		e_MinMag_Point_Mip_None,
		e_MinMag_Point_Mip_Fixed,
		e_MinMagMip_Point,
		e_MinMag_Linear_Mip_None,
		e_MinMag_Linear_Mip_Fixed,
		e_MinMagMip_Linear,
		e_Anisotropic_Point,
		e_Anisotropic_Linear,
		e_ZComparison_Linear
	};

	enum EWrapper
	{
		e_Clamp,
		e_Wrap,
		e_Mirror
	};

public:

	static void Init();
	static void Terminate();

	static void InitFrame();

	static void BindSamplerState(unsigned int nTexUnit, ESamplerState eState);
	static void SetMipLevel(unsigned int nTexUnit, float fMipLevel);

	CSampler(EFilter eFilter, EWrapper eWrapper);
	~CSampler();

private:

	static float GetMaxAnisotropicFactor();

	static CSampler** ms_pSamplerStates;

	static ESamplerState ms_eActiveSamplerState[16];

	static float ms_fMaxAnisotropy;
	static float ms_fMipLevel;

	unsigned int m_nID;
};



#endif
