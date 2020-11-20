#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/Reflections/SSR/SSR.h"
#include "Engine/Renderer/StencilBits.h"
#include "SSS.h"


ADJUSTABLE("Follow Surface", bool, gs_bSSSFollowSurface, false, false, true, "Graphics/PostFX/SSS")
ADJUSTABLE("Follow Surface Strength", float, gs_fSSSFollowSurfaceStrength, 4.f, 0.f, 10.f, "Graphics/PostFX/SSS")


CTexture* CSSS::ms_pTranslucencyMap = NULL;
CTexture* CSSS::ms_pSSSProfiles = NULL;
CTexture* CSSS::ms_pSSSLinearProfiles = NULL;
CTexture* CSSS::ms_pSeparableSSSTarget = NULL;
CTexture* CSSS::ms_pDepthCopy = NULL;

ProgramHandle CSSS::ms_nComputeProfilesPID = INVALID_PROGRAM_HANDLE;
ProgramHandle CSSS::ms_nSSSPID = INVALID_PROGRAM_HANDLE;
ProgramHandle CSSS::ms_nSSSFollowSurfacePID = INVALID_PROGRAM_HANDLE;
ProgramHandle CSSS::ms_nCopyDepthPID = INVALID_PROGRAM_HANDLE;

unsigned int CSSS::ms_nComputeProfilesConstantBufferID = 0;
unsigned int CSSS::ms_nSSSConstantBufferID = 0;

CSSS::SProfile* CSSS::ms_pProfiles = NULL;

bool CSSS::ms_bShouldComputeProfiles = true;

int CSSS::ms_nNbSamples = 5;
int CSSS::ms_nMaxNbProfiles = 5;
int CSSS::ms_nNbProfiles = 0;



struct SComputeProfilesConstants
{
	float	m_Near[20];
	float	m_Far[20];
	float	m_Ratio[20];
};


struct SSSSConstants
{
	float m_InvViewProj[16];
	float m_Eye[4];
	float m_Dir[2];
	float m_Pixel[2];
};



void CSSS::Init()
{
	/*int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();
	
	ms_pSSSProfiles			= new SRenderTarget(ms_nNbSamples, ms_nMaxNbProfiles, e_R16G16B16A16_FLOAT);
	ms_pSSSLinearProfiles	= new SRenderTarget(ms_nNbSamples, ms_nMaxNbProfiles, e_R16G16B16A16_FLOAT);
	ms_pTranslucencyMap		= new SRenderTarget(nWidth, nHeight, e_R32_FLOAT);
	ms_pSeparableSSSTarget	= new SRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT);
	ms_pDepthCopy			= new SRenderTarget(nWidth, nHeight, e_R32_FLOAT);

	ms_nComputeProfilesPID	= CShader::LoadProgram(SHADER_PATH("SSS"), "ComputeProfiles", "ComputeProfiles");
	ms_nSSSPID				= CShader::LoadProgram(SHADER_PATH("SSS"), "SSS", "SSS");
	ms_nSSSFollowSurfacePID = CShader::LoadProgram(SHADER_PATH("SSS"), "SSS", "SSSFollowSurface");
	ms_nCopyDepthPID		= CShader::LoadProgram(SHADER_PATH("SSS"), "SSS", "CopyDepth");

	ms_pProfiles = new SProfile[ms_nMaxNbProfiles];

	ms_nComputeProfilesConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SComputeProfilesConstants));
	ms_nSSSConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SSSSConstants));

	SProfile profile;
	profile.m_Near = float3(4.f, 12.5f, 50.f);
	profile.m_Far = float3(1.f, 3.33f, 20.f);
	profile.m_Ratio = 0.6f;

	AddProfile(profile);

	profile.m_Near = float3(12.5f, 4.f, 50.f);
	profile.m_Far = float3(3.33f, 1.f, 20.f);
	profile.m_Ratio = 0.6f;

	for (int i = 1; i < ms_nMaxNbProfiles; i++)
		AddProfile(profile);*/
}



void CSSS::Terminate()
{
	/*CShader::DeleteProgram(ms_nComputeProfilesPID);
	CShader::DeleteProgram(ms_nSSSPID);
	CShader::DeleteProgram(ms_nSSSFollowSurfacePID);
	CShader::DeleteProgram(ms_nCopyDepthPID);

	CDeviceManager::DeleteConstantBuffer(ms_nComputeProfilesConstantBufferID);
	CDeviceManager::DeleteConstantBuffer(ms_nSSSConstantBufferID);

	delete ms_pSSSProfiles;
	delete ms_pSSSLinearProfiles;
	delete ms_pTranslucencyMap;
	delete ms_pDepthCopy;
	delete[] ms_pProfiles;*/
}



void CSSS::AddProfile(SProfile& p_Profile)
{
	if (ms_nNbProfiles < ms_nMaxNbProfiles)
	{
		ms_pProfiles[ms_nNbProfiles].m_Near = p_Profile.m_Near;
		ms_pProfiles[ms_nNbProfiles].m_Far = p_Profile.m_Far;
		ms_pProfiles[ms_nNbProfiles].m_Ratio = p_Profile.m_Ratio;

		ms_nNbProfiles++;

		ms_bShouldComputeProfiles = true;
	}
}



void CSSS::ComputeProfiles()
{
	/*CFramebuffer::SetDrawBuffers(2);
	CFramebuffer::BindRenderTarget(0, ms_pSSSProfiles);
	CFramebuffer::BindRenderTarget(1, ms_pSSSLinearProfiles);
	CFramebuffer::BindDepthStencil(NULL);

	CShader::BindProgram(ms_nComputeProfilesPID);

	SComputeProfilesConstants pregisters;

	for (int i = 0; i < ms_nMaxNbProfiles; i++)
	{
		Copy(&pregisters.m_Near[4 * i], &ms_pProfiles[i].m_Near.x);
		Copy(&pregisters.m_Far[4 * i], &ms_pProfiles[i].m_Far.x);
		Copy(&pregisters.m_Ratio[4 * i], &ms_pProfiles[i].m_Ratio.x);
	}

	CDeviceManager::FillConstantBuffer(ms_nComputeProfilesConstantBufferID, &pregisters, sizeof(SComputeProfilesConstants));
	CDeviceManager::BindConstantBuffer(ms_nComputeProfilesConstantBufferID, ms_nComputeProfilesPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();*/
}



void CSSS::Apply()
{
	/*if (ms_bShouldComputeProfiles)
	{
		ComputeProfiles();
		ms_bShouldComputeProfiles = false;
	}

	CShader::BindProgram(ms_nCopyDepthPID);

	CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindDepthStencil(NULL);
	CFramebuffer::BindRenderTarget(0, ms_pDepthCopy);

	CTexture::SetTexture(ms_nCopyDepthPID, CRenderer::GetDepthStencil()->m_nTextureId, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();

	ProgramHandle nPID;

	if (gs_bSSSFollowSurface)
		nPID = ms_nSSSFollowSurfacePID;
	else
		nPID = ms_nSSSPID;

	CShader::BindProgram(nPID);

	CFramebuffer::BindRenderTarget(0, ms_pSeparableSSSTarget);
	CRenderer::ClearColorBits();

	CFramebuffer::BindDepthStencil(CRenderer::GetDepthStencil());

	CTexture::SetTexture(nPID, CLightsManager::GetDiffuseTarget()->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(nPID, ms_pSSSProfiles->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(nPID, ms_pDepthCopy->m_nTextureId, 2);

	CTexture::SetTexture(nPID, CDeferredRenderer::GetInfoTarget()->m_nTextureId, 3);
	CSampler::BindSamplerState(3, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	SSSSConstants pregisters;

	float4x4 mInvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	mInvViewProj.transpose();

	memcpy(&pregisters.m_InvViewProj, &(mInvViewProj.m00), 16 * sizeof(float));
	memcpy(&pregisters.m_Eye, CRenderer::GetCurrentCamera()->GetPosition().v(), 3 * sizeof(float));

	pregisters.m_Eye[3] = gs_fSSSFollowSurfaceStrength;

	pregisters.m_Dir[0] = 1.f;
	pregisters.m_Dir[1] = 0.f;

	pregisters.m_Pixel[0] = 1.f / CDeviceManager::GetDeviceWidth();
	pregisters.m_Pixel[1] = 1.f / CDeviceManager::GetDeviceHeight();

	CDeviceManager::FillConstantBuffer(ms_nSSSConstantBufferID, &pregisters, sizeof(SSSSConstants));
	CDeviceManager::BindConstantBuffer(ms_nSSSConstantBufferID, nPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderStates::SetDepthStencil(e_Zero, true);
	CRenderStates::SetStencil(e_StencilOp_Keep, e_StencilOp_Keep, e_StencilOp_Keep, e_StencilFunc_Equal, STENCIL_BIT_SSS, STENCIL_BIT_SSS, STENCIL_BIT_SSS);

	CRenderer::RenderQuadScreen();

	CFramebuffer::BindRenderTarget(0, CLightsManager::GetDiffuseTarget());

	CTexture::SetTexture(nPID, ms_pSeparableSSSTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	pregisters.m_Dir[0] = 0.f;
	pregisters.m_Dir[1] = 1.f;

	CDeviceManager::FillConstantBuffer(ms_nSSSConstantBufferID, &pregisters, sizeof(SSSSConstants));
	CDeviceManager::BindConstantBuffer(ms_nSSSConstantBufferID, nPID, 0);

	CRenderer::RenderQuadScreen();*/
}
