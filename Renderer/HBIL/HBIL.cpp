#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "HBIL.h"


/*SRenderTarget*	CHBIL::ms_pAccumulatedIrradiance	= NULL;
SRenderTarget*	CHBIL::ms_pInterleavedIrradiance	= NULL;
SRenderTarget*	CHBIL::ms_pReprojectedIrradiance	= NULL;
SRenderTarget*	CHBIL::ms_pTotalIrradiance			= NULL;
unsigned int	CHBIL::ms_nSamplingPatternIndex		= 0U;


ProgramHandle	gs_ReprojectionPID	= INVALID_PROGRAM_HANDLE;
ProgramHandle	gs_HBILPID			= INVALID_PROGRAM_HANDLE;
ProgramHandle	gs_AccumulatePID	= INVALID_PROGRAM_HANDLE;
ProgramHandle	gs_HBILMergePID		= INVALID_PROGRAM_HANDLE;

unsigned int	gs_ReprojectionConstants	= INVALIDHANDLE;
unsigned int	gs_HBILConstants			= INVALIDHANDLE;*/


const float gs_SamplingPattern[32] = 
{
	0.125f,		0.125f,
	-0.125f,	-0.375f,
	0.375f,		0.25f,
	0.5f,		-0.125f,
	-0.625f,	-0.25f,
	0.25f,		0.625f,
	0.625f,		0.375f,
	0.375f,		-0.625f,
	-0.25f,		0.75f,
	0.f,		-0.875f,
	-0.5f,		-0.75f,
	-0.75f,		0.5f,
	-1.f,		0.f,
	0.875f,		-0.5f,
	0.75f,		0.875f,
	-0.875f,	-1.f
};


struct SHBILConstants
{
	float4		m_Params;
	float4		m_Rotation;
	float4		m_Params2;

	float4x4	m_ViewMat;
};


void CHBIL::Init()
{
	/*int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pAccumulatedIrradiance	= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT);
	ms_pReprojectedIrradiance	= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT);
	ms_pTotalIrradiance			= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R32G32B32A32_FLOAT);
	ms_pInterleavedIrradiance	= CFramebuffer::CreateRenderTarget(nWidth / 4, nHeight / 4, e_R16G16B16A16_FLOAT);

	gs_ReprojectionPID			= CShader::LoadProgram(SHADER_PATH("HBIL"), "HBIL", "Reprojection");
	gs_HBILPID					= CShader::LoadProgram(SHADER_PATH("HBIL"), "HBIL", "HBIL");
	gs_AccumulatePID			= CShader::LoadProgram(SHADER_PATH("HBIL"), "HBIL", "Accumulate");
	gs_HBILMergePID				= CShader::LoadProgram(SHADER_PATH("HBIL"), "HBIL", "Merge");

	gs_HBILConstants			= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SHBILConstants));
	gs_ReprojectionConstants	= CDeviceManager::CreateConstantBuffer(NULL, 3 * sizeof(float4x4));

	ms_nSamplingPatternIndex = 0U;*/
}


void CHBIL::Terminate()
{
	/*delete ms_pAccumulatedIrradiance;
	delete ms_pInterleavedIrradiance;
	delete ms_pReprojectedIrradiance;
	delete ms_pTotalIrradiance;

	CShader::DeleteProgram(gs_ReprojectionPID);
	CShader::DeleteProgram(gs_HBILPID);
	CShader::DeleteProgram(gs_AccumulatePID);
	CShader::DeleteProgram(gs_HBILMergePID);

	CDeviceManager::DeleteConstantBuffer(gs_HBILConstants);
	CDeviceManager::DeleteConstantBuffer(gs_ReprojectionConstants);*/
}


void CHBIL::Apply(SRenderTarget* pIrradianceTarget, unsigned int nZBuffer)
{
	/*ReprojectIrradiance(pIrradianceTarget->m_nTextureId, nZBuffer);

	InterleavedHBIL(nZBuffer);

	Accumulate(nZBuffer);

	Merge(pIrradianceTarget);

	ms_nSamplingPatternIndex = (ms_nSamplingPatternIndex + 1U) % 16U;*/
}



void CHBIL::ReprojectIrradiance(unsigned int nSrc, unsigned int nZBuffer)
{
	/*CFramebuffer::SetDrawBuffers(2);

	CFramebuffer::BindRenderTarget(0, ms_pTotalIrradiance);
	CFramebuffer::BindRenderTarget(1, ms_pReprojectedIrradiance);

	CShader::BindProgram(gs_ReprojectionPID);

	CTexture::SetTexture(gs_ReprojectionPID, nZBuffer, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_ReprojectionPID, nSrc, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_ReprojectionPID, ms_pAccumulatedIrradiance->m_nTextureId, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_ReprojectionPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 3);
	CSampler::BindSamplerState(3, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_ReprojectionPID, CDeferredRenderer::GetAlbedoTarget()->m_nTextureId, 4);
	CSampler::BindSamplerState(4, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	float4x4 pregisters[3];

	pregisters[0] = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	pregisters[1] = CRenderer::GetCurrentCamera()->GetLastFrameViewProjMatrix();
	pregisters[2] = CRenderer::GetCurrentCamera()->GetViewMatrix();

	pregisters[0].transpose();
	pregisters[1].transpose();
	pregisters[2].transpose();

	CDeviceManager::FillConstantBuffer(gs_ReprojectionConstants, pregisters, 3 * sizeof(float4x4));
	CDeviceManager::BindConstantBuffer(gs_ReprojectionConstants, gs_ReprojectionPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();*/
}



void CHBIL::InterleavedHBIL(unsigned int nZBuffer)
{
	/*CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindRenderTarget(0, ms_pInterleavedIrradiance);

	CShader::BindProgram(gs_HBILPID);

	CTexture::SetTexture(gs_HBILPID, nZBuffer, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_HBILPID, ms_pTotalIrradiance->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_HBILPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	SHBILConstants pregisters;

	pregisters.m_Params.x = gs_SamplingPattern[ms_nSamplingPatternIndex * 2U] * 0.5f;
	pregisters.m_Params.y = gs_SamplingPattern[ms_nSamplingPatternIndex * 2U + 1U] * 0.5f;
	pregisters.m_Params.z = 1.f / ms_pInterleavedIrradiance->m_nWidth;
	pregisters.m_Params.w = 1.f / ms_pInterleavedIrradiance->m_nHeight;

	float angle = Randf(0.f, 6.28f);
	float fNear = CRenderer::GetCurrentCamera()->GetNearPlane();
	float fFar	= CRenderer::GetCurrentCamera()->GetFarPlane();

	pregisters.m_Rotation = float4(cosf(angle), sinf(angle), fNear, fFar);
	pregisters.m_ViewMat = CRenderer::GetCurrentCamera()->GetViewMatrix();
	pregisters.m_ViewMat.transpose();

	float FOV = CRenderer::GetCurrentCamera()->GetFOV();
	float AR = CWindow::GetMainWindow()->GetAspectRatio();

	pregisters.m_Params2 = float4(0.f, 0.f, tanf(FOV * 3.141592f / 360.f), tanf(FOV * 3.141592f / 360.f) / AR);

	CDeviceManager::FillConstantBuffer(gs_HBILConstants, &pregisters, sizeof(SHBILConstants));
	CDeviceManager::BindConstantBuffer(gs_HBILConstants, gs_HBILPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();*/
}


void CHBIL::Accumulate(unsigned int nZBuffer)
{
	/*CFramebuffer::BindRenderTarget(0, ms_pAccumulatedIrradiance);

	CShader::BindProgram(gs_AccumulatePID);

	CTexture::SetTexture(gs_AccumulatePID, ms_pInterleavedIrradiance->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_AccumulatePID, ms_pReprojectedIrradiance->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_AccumulatePID, nZBuffer, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CDeviceManager::BindConstantBuffer(gs_HBILConstants, gs_AccumulatePID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();*/
}


void CHBIL::Merge(SRenderTarget* pTarget)
{
	/*CFramebuffer::BindRenderTarget(0, pTarget);

	CShader::BindProgram(gs_HBILMergePID);

	CTexture::SetTexture(gs_HBILMergePID, ms_pAccumulatedIrradiance->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CRenderStates::SetBlendingState(e_Additive);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();*/
}
