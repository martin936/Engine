#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "SMAA.h"
#include "Engine/Renderer/StencilBits.h"


CTexture* CSMAA::ms_pEdgesTarget		= nullptr;
CTexture* CSMAA::ms_pWeightsTarget		= nullptr;
CTexture* CSMAA::ms_pFinalTarget		= nullptr;

CTexture*	CSMAA::ms_pAreaTex		= nullptr;
CTexture*	CSMAA::ms_pSearchTex	= nullptr;

ProgramHandle	CSMAA::ms_EdgesPID	= 0;
ProgramHandle	CSMAA::ms_WeightPID	= 0;
ProgramHandle	CSMAA::ms_BlendPID	= 0;
ProgramHandle	CSMAA::ms_ComputeVelocities = 0;

unsigned int	CSMAA::ms_nConstantBuffer = 0;


void CSMAA::Init()
{
	/*CWindow* pWindow = CWindow::GetMainWindow();
	int nWidth = pWindow->GetWidth();
	int nHeight = pWindow->GetHeight();

	ms_pEdgesTarget		= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R8G8B8A8);
	ms_pWeightsTarget	= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R8G8B8A8);
	ms_pFinalTarget		= CFramebuffer::CreateRenderTarget(nWidth, nHeight, e_R8G8B8A8);

	ms_pAreaTex		= new CTexture("../Data/Textures/AreaTex.dds");
	ms_pSearchTex	= new CTexture("../Data/Textures/SearchTex.dds");

	//ms_ComputeVelocities	= CShader::LoadProgram(SHADER_PATH("SMAA"), "ComputeVelocities", "ComputeVelocities");	
	ms_EdgesPID				= CShader::LoadProgram(SHADER_PATH("PostFX/SMAA"), "EdgesSMAA", "EdgesSMAA");
	ms_WeightPID			= CShader::LoadProgram(SHADER_PATH("PostFX/SMAA"), "BlendingWeight", "BlendingWeight");
	ms_BlendPID				= CShader::LoadProgram(SHADER_PATH("PostFX/SMAA"), "Blending", "Blending");

	ms_nConstantBuffer = CDeviceManager::CreateConstantBuffer(NULL, 4 * sizeof(float));*/
}


void CSMAA::Terminate()
{
	//delete ms_pFinalTarget;
	//delete ms_pWeightsTarget;
	//
	//delete ms_pAreaTex;
	//delete ms_pSearchTex;
	//
	//CShader::DeleteProgram(ms_EdgesPID);
	//CShader::DeleteProgram(ms_WeightPID);
	//CShader::DeleteProgram(ms_BlendPID);
}


void CSMAA::Process(CTexture* pColorTarget)
{
	/*CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindRenderTarget(0, ms_pEdgesTarget);
	CFramebuffer::BindDepthStencil(CRenderer::GetDepthStencil());
	CRenderer::ClearColorBits();

	CShader::BindProgram(ms_EdgesPID);

	int nWidth = pColorTarget->m_nWidth;
	int nHeight = pColorTarget->m_nHeight;

	float fPixelSize[4] = { 1.f / nWidth, 1.f / nHeight, static_cast<float>(nWidth), static_cast<float>(nHeight) };

	CDeviceManager::FillConstantBuffer(ms_nConstantBuffer, fPixelSize, 4 * sizeof(float));
	CDeviceManager::BindConstantBuffer(ms_nConstantBuffer, ms_EdgesPID, 0);

	CTexture::SetTexture(ms_EdgesPID, pColorTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, true);

	CRenderStates::SetStencil(e_StencilOp_Replace, e_StencilOp_Keep, e_StencilOp_Keep, e_StencilFunc_Always, STENCIL_BIT_SMAA, STENCIL_BIT_SMAA, STENCIL_BIT_SMAA);

	CRenderer::RenderQuadScreen();

	CFramebuffer::BindRenderTarget(0, ms_pWeightsTarget);
	CRenderer::ClearColorBits();

	CShader::BindProgram(ms_WeightPID);

	CDeviceManager::BindConstantBuffer(ms_nConstantBuffer, ms_WeightPID, 0);

	CTexture::SetTexture(ms_WeightPID, ms_pEdgesTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_WeightPID, ms_pAreaTex->m_nID, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_WeightPID, ms_pSearchTex->m_nID, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CRenderStates::SetStencil(e_StencilOp_Keep, e_StencilOp_Keep, e_StencilOp_Keep, e_StencilFunc_Equal, STENCIL_BIT_SMAA, STENCIL_BIT_SMAA, STENCIL_BIT_SMAA);

	CRenderer::RenderQuadScreen();

	CShader::BindProgram(ms_BlendPID);

	CDeviceManager::BindConstantBuffer(ms_nConstantBuffer, ms_BlendPID, 0);

	CFramebuffer::BindRenderTarget(0, ms_pFinalTarget);

	CTexture::SetTexture(ms_BlendPID, ms_pWeightsTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_BlendPID, pColorTarget->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CRenderStates::SetDepthStencil(e_Zero, false);

	CRenderer::RenderQuadScreen();*/
}
