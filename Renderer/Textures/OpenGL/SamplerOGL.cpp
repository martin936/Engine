#include "Engine/Engine.h"
#include "../Sampler.h"


CSampler::ESamplerState CSampler::ms_eActiveSamplerState[16] = { e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid, e_Invalid };


float CSampler::GetMaxAnisotropicFactor()
{
	float value;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &value);

	return value;
}


void CSampler::InitFrame()
{
	for (int i = 0; i < 16; i++)
	{
		ms_eActiveSamplerState[i] = e_Invalid;
		glBindSampler(i, 0);
	}
}


CSampler::CSampler(EFilter eFilter, EWrapper eWrapper)
{
	glGenSamplers(1, &m_nID);

	switch (eFilter)
	{
	case e_MinMag_Point_Mip_None:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MIN_LOD, 0.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_LOD, 0.f);
		break;

	case e_MinMag_Point_Mip_Fixed:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		break;

	case e_MinMagMip_Point:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		break;

	case e_MinMag_Linear_Mip_None:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MIN_LOD, 0.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_LOD, 0.f);
		break;

	case e_MinMag_Linear_Mip_Fixed:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		break;

	case e_MinMagMip_Linear:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		break;

	case e_Anisotropic_Point:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, ms_fMaxAnisotropy);
		break;

	case e_Anisotropic_Linear:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, ms_fMaxAnisotropy);
		break;

	case e_ZComparison_Linear:
		glSamplerParameteri(m_nID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glSamplerParameteri(m_nID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_ANISOTROPY, 1.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MIN_LOD, 0.f);
		glSamplerParameterf(m_nID, GL_TEXTURE_MAX_LOD, 0.f);
		glSamplerParameteri(m_nID, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
		glSamplerParameteri(m_nID, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		break;

	default:
		break;
	}

	switch (eWrapper)
	{
	case e_Clamp:
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		break;

	case e_Wrap:
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_R, GL_REPEAT);
		break;

	case e_Mirror:
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glSamplerParameteri(m_nID, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT);
		break;

	default:
		break;
	}
}


CSampler::~CSampler()
{
	glDeleteSamplers(1, &m_nID);
}


void CSampler::BindSamplerState(unsigned int nTexUnit, ESamplerState eState)
{
	if (ms_eActiveSamplerState[nTexUnit] != eState)
	{
		ms_eActiveSamplerState[nTexUnit] = eState;

		glBindSampler(nTexUnit, ms_pSamplerStates[eState]->m_nID);
	}
}


void CSampler::SetMipLevel(unsigned int nTexUnit, float fMipLevel)
{
	if (ms_eActiveSamplerState[nTexUnit] == e_MinMag_Linear_Mip_Fixed_UVW_Clamp  || ms_eActiveSamplerState[nTexUnit] == e_MinMag_Point_Mip_Fixed && fMipLevel != ms_fMipLevel)
	{
		ms_fMipLevel = fMipLevel;

		glSamplerParameterf(ms_pSamplerStates[e_MinMag_Linear_Mip_Fixed_UVW_Clamp]->m_nID, GL_TEXTURE_MIN_LOD, fMipLevel);
		glSamplerParameterf(ms_pSamplerStates[e_MinMag_Linear_Mip_Fixed_UVW_Clamp]->m_nID, GL_TEXTURE_MAX_LOD, fMipLevel);
	}
}
