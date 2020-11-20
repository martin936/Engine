#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "AO.h"



void CAO::ApplySSAO()
{
	/*if (!ms_bIsInit)
		return;

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetBlendingState(e_Opaque);

	unsigned int ZBuffer = CRenderer::GetDepthStencil()->m_nTextureId;
	static bool FrameTick = false;
	SSSAOFragmentConstants fregisters;

	float4x4 mInvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	mInvViewProj.transpose();
	float4x4 mViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	mViewProj.transpose();
	float4x4 mLastViewProj = CRenderer::GetCurrentCamera()->GetLastFrameViewProjMatrix();
	mLastViewProj.transpose();

	memcpy(&fregisters.m_InvViewProj, &(mInvViewProj.m00), 16 * sizeof(float));
	memcpy(&fregisters.m_ViewProj, &(mViewProj.m00), 16 * sizeof(float));
	memcpy(&fregisters.m_LastViewProj, &(mLastViewProj.m00), 16 * sizeof(float));
	memcpy(&fregisters.m_Kernel, ms_fKernel, 64 * sizeof(float));
	fregisters.m_FrameTick = FrameTick ? 0.f : 0.5f;
	fregisters.m_KernelSize = ms_fKernelSize;
	FrameTick = !FrameTick;

	CShader::BindProgram(ms_SSAOPID);

	CTexture::SetTexture(ms_SSAOPID, ZBuffer, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_SSAOPID, ms_pLastSSAOTarget->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_SSAOPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CFramebuffer::SetDrawBuffers(2);
	CFramebuffer::BindRenderTarget(0, ms_pBlendedSSAOTarget);
	CFramebuffer::BindRenderTarget(1, ms_pSSAOTarget);
	CFramebuffer::BindDepthStencil(NULL);

	CDeviceManager::FillConstantBuffer(ms_nSSAOConstantBuffer, &fregisters, sizeof(SSSAOFragmentConstants));
	CDeviceManager::BindConstantBuffer(ms_nSSAOConstantBuffer, ms_SSAOPID, 0);

	CRenderer::RenderQuadScreen();

	CShader::BindProgram(ms_BlurPID);

	CFramebuffer::BindRenderTarget(0, ms_pBlurredTarget);
	CFramebuffer::BindRenderTarget(1, ms_pFinalTarget);

	CTexture::SetTexture(ms_BlurPID, ms_pBlendedSSAOTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(ms_BlurPID, ZBuffer, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	SRenderTarget* pTmp = ms_pLastSSAOTarget;
	ms_pLastSSAOTarget = ms_pSSAOTarget;
	ms_pSSAOTarget = pTmp;

	SBlurFragmentConstants bfregisters;

	float3 CamDir = CRenderer::GetCurrentCamera()->GetDirection();

	memcpy(&bfregisters.m_InvViewProj, &(mInvViewProj.m00), 16 * sizeof(float));
	memcpy(&bfregisters.m_CamDir, &(CamDir.x), 3 * sizeof(float));
	bfregisters.m_Dir[0] = 1.f;
	bfregisters.m_Dir[1] = 0.f;
	bfregisters.m_Pixel[0] = 1.f / ms_pBlurredTarget->m_nWidth;
	bfregisters.m_Pixel[1] = 1.f / ms_pBlurredTarget->m_nHeight;

	CDeviceManager::FillConstantBuffer(ms_nBlurConstantBuffer, &bfregisters, sizeof(SBlurFragmentConstants));
	CDeviceManager::BindConstantBuffer(ms_nBlurConstantBuffer, ms_SSAOPID, 0);

	CRenderer::RenderQuadScreen();

	CFramebuffer::BindRenderTarget(0, ms_pFinalTarget);
	CFramebuffer::BindRenderTarget(1, nullptr);

	CTexture::SetTexture(ms_BlurPID, ms_pBlurredTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	bfregisters.m_Dir[0] = 0.f;
	bfregisters.m_Dir[1] = 1.f;

	CDeviceManager::FillConstantBuffer(ms_nBlurConstantBuffer, &bfregisters, sizeof(SBlurFragmentConstants));
	CDeviceManager::BindConstantBuffer(ms_nBlurConstantBuffer, ms_SSAOPID, 0);

	CRenderer::RenderQuadScreen();

	ms_pFinalTarget = ms_pSSAOFinalTarget;*/
}
