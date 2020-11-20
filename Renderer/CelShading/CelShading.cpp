#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "CelShading.h"


ADJUSTABLE("Line Threshold", float, gs_fLineThreshold, 1.f, 0.f, 1.f, "Graphics/Cel Shading")

SRenderTarget* CCelShading::ms_pEdgesTarget = nullptr;
SRenderTarget* CCelShading::ms_pFinalTarget = nullptr;

ProgramHandle CCelShading::ms_SobelPID = 0;
ProgramHandle CCelShading::ms_MergePID = 0;

unsigned int CCelShading::ms_nEdgesConstantBuffer = 0;


struct SSobelFragmentConstants
{
	float m_Pixel[4];
};


void CCelShading::Init()
{
	/*int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pEdgesTarget = CFramebuffer::CreateRenderTarget(nWidth, nHeight, ETextureFormat::e_R8G8B8A8);
	ms_pFinalTarget = CFramebuffer::CreateRenderTarget(nWidth, nHeight, ETextureFormat::e_R8G8B8A8);

	ms_SobelPID = CShader::LoadProgram(SHADER_PATH("CelShading"), "sobel", "sobel");
	ms_MergePID = CShader::LoadProgram(SHADER_PATH("CelShading"), "merge", "merge");

	ms_nEdgesConstantBuffer = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SSobelFragmentConstants));*/
}


void CCelShading::Process()
{
	/*CShader::BindProgram(ms_SobelPID);

	CFramebuffer::BindRenderTarget(0, ms_pEdgesTarget);
	CFramebuffer::SetDrawBuffers(1);

	CTexture::SetTexture(ms_SobelPID, CRenderer::GetDepthStencil()->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_SobelPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	SSobelFragmentConstants fregisters;
	fregisters.m_Pixel[0] = 1.f / ms_pEdgesTarget->m_nWidth;
	fregisters.m_Pixel[1] = 1.f / ms_pEdgesTarget->m_nHeight;
	fregisters.m_Pixel[2] = gs_fLineThreshold;

	CDeviceManager::FillConstantBuffer(ms_nEdgesConstantBuffer, &fregisters, sizeof(SSobelFragmentConstants));
	CDeviceManager::BindConstantBuffer(ms_nEdgesConstantBuffer, ms_SobelPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);

	CRenderer::RenderQuadScreen();

	CShader::BindProgram(ms_MergePID);

	CFramebuffer::BindRenderTarget(0, ms_pFinalTarget);

	CTexture::SetTexture(ms_MergePID, ms_pEdgesTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//CTexture::SetTexture(ms_MergePID, CRenderer::GetInstance()->GetMergeFramebuffer()->GetTarget(0)->m_nTextureId, 1);

	CRenderer::RenderQuadScreen();*/
}


void CCelShading::Terminate()
{
	//delete ms_pEdgesTarget;
	//delete ms_pFinalTarget;
	//
	//ms_pEdgesTarget = nullptr;
	//ms_pFinalTarget = nullptr;
}
