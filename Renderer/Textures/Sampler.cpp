#include "Engine/Engine.h"
#include "Sampler.h"


CSampler** CSampler::ms_pSamplerStates = NULL;
float CSampler::ms_fMaxAnisotropy	= 1.f;
float CSampler::ms_fMipLevel		= 0.f;


void CSampler::Init()
{
	ms_pSamplerStates = new CSampler*[e_NbSamplers];

	ms_fMaxAnisotropy = GetMaxAnisotropicFactor();

	ms_pSamplerStates[e_MinMag_Point_Mip_None_UVW_Clamp]		= new CSampler(e_MinMag_Point_Mip_None, e_Clamp);
	ms_pSamplerStates[e_MinMag_Point_Mip_Fixed_UVW_Clamp]		= new CSampler(e_MinMag_Point_Mip_Fixed, e_Clamp);
	ms_pSamplerStates[e_MinMag_Point_Mip_None_UVW_Wrap]			= new CSampler(e_MinMag_Point_Mip_None, e_Wrap);
	ms_pSamplerStates[e_MinMag_Point_Mip_None_UVW_Mirror]		= new CSampler(e_MinMag_Point_Mip_None, e_Mirror);

	ms_pSamplerStates[e_MinMagMip_Point_UVW_Clamp]				= new CSampler(e_MinMagMip_Point, e_Clamp);
	ms_pSamplerStates[e_MinMagMip_Point_UVW_Wrap]				= new CSampler(e_MinMagMip_Point, e_Wrap);
	ms_pSamplerStates[e_MinMagMip_Point_UVW_Mirror]				= new CSampler(e_MinMagMip_Point, e_Mirror);

	ms_pSamplerStates[e_MinMag_Linear_Mip_None_UVW_Clamp]		= new CSampler(e_MinMag_Linear_Mip_None, e_Clamp);
	ms_pSamplerStates[e_MinMag_Linear_Mip_None_UVW_Wrap]		= new CSampler(e_MinMag_Linear_Mip_None, e_Wrap);
	ms_pSamplerStates[e_MinMag_Linear_Mip_None_UVW_Mirror]		= new CSampler(e_MinMag_Linear_Mip_None, e_Mirror);

	ms_pSamplerStates[e_MinMag_Linear_Mip_Fixed_UVW_Clamp]		= new CSampler(e_MinMag_Linear_Mip_Fixed, e_Clamp);

	ms_pSamplerStates[e_MinMagMip_Linear_UVW_Clamp]				= new CSampler(e_MinMagMip_Linear, e_Clamp);
	ms_pSamplerStates[e_MinMagMip_Linear_UVW_Wrap]				= new CSampler(e_MinMagMip_Linear, e_Wrap);
	ms_pSamplerStates[e_MinMagMip_Linear_UVW_Mirror]			= new CSampler(e_MinMagMip_Linear, e_Mirror);

	ms_pSamplerStates[e_Anisotropic_Point_UVW_Clamp]			= new CSampler(e_Anisotropic_Point, e_Clamp);
	ms_pSamplerStates[e_Anisotropic_Point_UVW_Wrap]				= new CSampler(e_Anisotropic_Point, e_Wrap);
	ms_pSamplerStates[e_Anisotropic_Point_UVW_Mirror]			= new CSampler(e_Anisotropic_Point, e_Mirror);

	ms_pSamplerStates[e_Anisotropic_Linear_UVW_Clamp]			= new CSampler(e_Anisotropic_Linear, e_Clamp);
	ms_pSamplerStates[e_Anisotropic_Linear_UVW_Wrap]			= new CSampler(e_Anisotropic_Linear, e_Wrap);
	ms_pSamplerStates[e_Anisotropic_Linear_UVW_Mirror]			= new CSampler(e_Anisotropic_Linear, e_Mirror);

	ms_pSamplerStates[e_ZComparison_Linear_UVW_Clamp]			= new CSampler(e_ZComparison_Linear, e_Clamp);
}


void CSampler::Terminate()
{
	for (int i = 0; i < e_NbSamplers; i++)
	{
		delete ms_pSamplerStates[i];
	}

	delete ms_pSamplerStates;

	ms_pSamplerStates = NULL;
}
