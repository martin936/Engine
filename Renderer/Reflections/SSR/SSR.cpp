#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/StencilBits.h"
#include "Engine/Renderer/IBLBaker/EnvMaps.h"
#include "Engine/Renderer/TAA/TAA.h"
#include "SSR.h"


CTexture* CSSR::ms_pRayDatas		= NULL;
CTexture* CSSR::ms_pResolveTarget	= NULL;
CTexture* CSSR::ms_pZMips			= NULL;
CTexture* CSSR::ms_pHistory			= NULL;
CTexture* CSSR::ms_pBlurTarget		= NULL;

ProgramHandle gs_SSR_AllocateRaysPID		= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_RayMarchingPID			= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_ResolvePID				= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_MergePID				= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_TemporalPID			= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_BuildHiZPID			= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_LinearizeDepthPID		= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_SSR_BlurPID				= INVALID_PROGRAM_HANDLE;

unsigned int gs_SSR_PixelConstants			= INVALIDHANDLE;
unsigned int gs_SSR_LinearizeConstants		= INVALIDHANDLE;
unsigned int gs_SSR_AllocateRaysConstants	= INVALIDHANDLE;
unsigned int gs_SSR_ResolveConstants		= INVALIDHANDLE;


struct SAllocateRaysConstants
{
	float4x4	InvViewProj;
	float4		Eye;
};

struct SRaytracePixelConstants
{
	float4x4	m_ViewProj;
	float4x4	m_InvViewProj;
	float4		m_Eye;
	float4		m_NearFar;
};


struct SResolveConstants
{
	float4x4	m_ViewProj;
	float4x4	m_InvViewProj;
	float4		m_Eye;
	float4		m_NearFar;

	int			m_SampleOffsets[128];
};


struct SLinearizeDepthConstants
{
	float4		m_NearFar;
};



const int gs_SampleOffset1[32] =
{
	0, 0,
	0, -1,
	-1, 2,
	-2, 1,
	-2, -1,
	-1, -2,
	1, -2,
	2, -1,
	2, 3,
	-3, 3,
	-1, 4,
	-4, -1,
	0, -4,
	3, -3,
	4, 0,
	3, 3
};


const int gs_SampleOffset2[32] =
{
	1, 0,
	-1, -1,
	0, 2,
	2, 1,
	-3, 0,
	3, 1,
	1, 3,
	1, -3,
	4, 2,
	2, 4,
	-4, 0,
	-2, 3,
	-3, 2,
	-2, -3,
	2, -2,
	4, -2
};


const int gs_SampleOffset3[32] =
{
	0, 1,
	1, -1,
	-1, 1,
	-2, 0,
	1, 2,
	-3, 1,
	3, -1,
	-1, -3,
	4, -1,
	4, 1,
	3, 2,
	0, 4,
	-1, 3,
	-2, 4,
	-3, -2,
	2, -3
};


const int gs_SampleOffset4[32] =
{
	1, 1,
	-1, 0,
	0, -2,
	2, 0,
	3, 0,
	0, -3,
	0, 3,
	2, 2,
	-2, -2,
	1, 4,
	-2, 2,
	-3, -1,
	3, -2,
	1, -4,
	2, -4,
	-4, 1
};



void CSSR::Init()
{
	/*int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();


	ms_pRayDatas					= new SRenderTarget((nWidth + 1) / 2, (nHeight + 1) / 2, e_R16G16B16A16_FLOAT, CTexture::eTextureStorage2D);
	ms_pResolveTarget				= new SRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT, CTexture::eTextureStorage2D);
	ms_pZMips						= new SRenderTarget(((nWidth + 127) / 128) * 128, ((nHeight + 127) / 128) * 128, e_R32_FLOAT, CTexture::eTexture2D, 1, 1, true);
	ms_pHistory						= new SRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT, CTexture::eTextureStorage2D);
	ms_pBlurTarget					= new SRenderTarget(nWidth, nHeight, e_R16G16B16A16_FLOAT, CTexture::eTexture2D);

	gs_SSR_BuildHiZPID				= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "SSR", "BuildHiZ");
	gs_SSR_AllocateRaysPID			= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "AllocateRays");
	gs_SSR_RayMarchingPID			= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "RayMarching");
	gs_SSR_ResolvePID				= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "Resolve");
	gs_SSR_MergePID					= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "SSR", "Merge");
	gs_SSR_TemporalPID				= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "TemporalFiltering");
	gs_SSR_LinearizeDepthPID		= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "SSR", "LinearizeDepth");
	gs_SSR_BlurPID					= CShader::LoadProgram(SHADER_PATH("Reflections/SSR"), "SSR", "Blur");

	gs_SSR_PixelConstants			= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SRaytracePixelConstants));
	gs_SSR_LinearizeConstants		= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SLinearizeDepthConstants));
	gs_SSR_AllocateRaysConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SAllocateRaysConstants));
	gs_SSR_ResolveConstants			= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SResolveConstants));*/
}


void CSSR::Terminate()
{
	/*CShader::DeleteProgram(gs_SSR_AllocateRaysPID);
	CShader::DeleteProgram(gs_SSR_RayMarchingPID);
	CShader::DeleteProgram(gs_SSR_ResolvePID);
	CShader::DeleteProgram(gs_SSR_MergePID);
	CShader::DeleteProgram(gs_SSR_TemporalPID);
	CShader::DeleteProgram(gs_SSR_BuildHiZPID);
	CShader::DeleteProgram(gs_SSR_LinearizeDepthPID);
	CShader::DeleteProgram(gs_SSR_BlurPID);

	CDeviceManager::DeleteConstantBuffer(gs_SSR_PixelConstants);
	CDeviceManager::DeleteConstantBuffer(gs_SSR_LinearizeConstants);
	CDeviceManager::DeleteConstantBuffer(gs_SSR_AllocateRaysConstants);
	CDeviceManager::DeleteConstantBuffer(gs_SSR_ResolveConstants);

	delete ms_pRayDatas;
	delete ms_pResolveTarget;
	delete ms_pZMips;
	delete ms_pHistory;*/
}


void CSSR::Apply(CTexture* pTarget)
{
	BuildHiZ();

	AllocateRays();

	Raytrace();

	//Resolve(pTarget->m_nTextureId);

	BilateralCleanUp();

	Merge(pTarget);
}


void CSSR::BuildHiZ()
{
	/*CShader::BindProgram(gs_SSR_LinearizeDepthPID);

	CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindRenderTarget(0, ms_pZMips, 0, 0);
	CFramebuffer::BindDepthStencil(nullptr);

	CTexture::SetTexture(gs_SSR_LinearizeDepthPID, CRenderer::GetDepthStencil()->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	SLinearizeDepthConstants pregisters;

	pregisters.m_NearFar = float4(CRenderer::GetCurrentCamera()->GetNearPlane(), CRenderer::GetCurrentCamera()->GetFarPlane(), 0.f, 0.f);

	CDeviceManager::FillConstantBuffer(gs_SSR_LinearizeConstants, &pregisters, sizeof(pregisters));
	CDeviceManager::BindConstantBuffer(gs_SSR_LinearizeConstants, gs_SSR_LinearizeDepthPID, 0);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);

	CRenderer::RenderQuadScreen();

	CShader::BindProgram(gs_SSR_BuildHiZPID);

	int fac = 1;

	for (int i = 1; i < 8; i++)
	{
		CFramebuffer::BindRenderTarget(0, ms_pZMips, 0, i);

		CTexture::SetTexture(gs_SSR_BuildHiZPID, ms_pZMips->m_nTextureId, 0);
		CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_Fixed_UVW_Clamp);
		CSampler::SetMipLevel(0, 1.f * i - 1);

		fac *= 2;

		CDeviceManager::SetUniform(gs_SSR_BuildHiZPID, "level", i);

		CRenderer::RenderQuadScreen();
	}*/
}



void CSSR::AllocateRays()
{
	/*CShader::BindProgram(gs_SSR_AllocateRaysPID);

	ms_pRayDatas->m_pTexture->BindImage(0, CTexture::e_WriteOnly);

	CTexture::SetTexture(gs_SSR_AllocateRaysPID, CRenderer::GetDepthStencil()->m_nTextureId, 0);
	CTexture::SetTexture(gs_SSR_AllocateRaysPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 1);
	CTexture::SetTexture(gs_SSR_AllocateRaysPID, CDeferredRenderer::GetInfoTarget()->m_nTextureId, 2);

	static int index = 0;

	SAllocateRaysConstants registers;
	registers.Eye = float4(CRenderer::GetCurrentCamera()->GetPosition(), 1.f * index);
	registers.InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	registers.InvViewProj.transpose();

	CDeviceManager::FillConstantBuffer(gs_SSR_AllocateRaysConstants, &registers, sizeof(SAllocateRaysConstants));
	CDeviceManager::BindConstantBuffer(gs_SSR_AllocateRaysConstants, gs_SSR_AllocateRaysPID, 0);

	index++;

	CDeviceManager::Dispatch((ms_pRayDatas->m_nWidth + 7) / 8, (ms_pRayDatas->m_nHeight + 7) / 8, 1);*/

	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}



void CSSR::Raytrace()
{
	/*int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	SRaytracePixelConstants pregisters;
	pregisters.m_InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	pregisters.m_InvViewProj.transpose();

	pregisters.m_ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	pregisters.m_ViewProj.transpose();

	pregisters.m_Eye = CRenderer::GetCurrentCamera()->GetPosition();
	pregisters.m_NearFar = float4(CRenderer::GetCurrentCamera()->GetNearPlane(), CRenderer::GetCurrentCamera()->GetFarPlane(), 1.f * ms_pRayDatas->m_nWidth, 1.f * ms_pRayDatas->m_nHeight);

	CDeviceManager::FillConstantBuffer(gs_SSR_PixelConstants, &pregisters, sizeof(SRaytracePixelConstants));

	RayMarching();*/

	//HiZRayTracing();
}



void CSSR::RayMarching()
{
	/*CShader::BindProgram(gs_SSR_RayMarchingPID);

	ms_pRayDatas->m_pTexture->BindImage(0, CTexture::e_ReadWrite);

	CDeviceManager::BindConstantBuffer(gs_SSR_PixelConstants, gs_SSR_RayMarchingPID, 0);

	CTexture::SetTexture(gs_SSR_RayMarchingPID, ms_pZMips->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMagMip_Point_UVW_Clamp);

	CDeviceManager::Dispatch((ms_pRayDatas->m_nWidth + 7) / 8, (ms_pRayDatas->m_nHeight + 7) / 8, 1);*/

	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}



void CSSR::HiZRayTracing()
{
	/*CShader::BindProgram(gs_SSR_HizRayTracingPID);

	CDeviceManager::BindConstantBuffer(gs_SSR_PixelConstants, gs_SSR_HizRayTracingPID, 0);

	CTexture::SetTexture(gs_SSR_HizRayTracingPID, ms_pZMips->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMagMip_Point_UVW_Clamp);

	CTexture::SetTexture(gs_SSR_HizRayTracingPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CRenderStates::SetDepthStencil(e_Zero, true);
	CRenderStates::SetStencil(e_StencilOp_Keep, e_StencilOp_Keep, e_StencilOp_Keep, e_StencilFunc_Equal, STENCIL_BIT_SSR_RAYS, STENCIL_BIT_SSR_RAYS, STENCIL_BIT_SSR_RAYS);

	CRenderer::RenderQuadScreen();*/
}



void CSSR::Resolve(unsigned int nSrc)
{
	/*CShader::BindProgram(gs_SSR_ResolvePID);

	ms_pResolveTarget->m_pTexture->BindImage(0, CTexture::e_WriteOnly);

	CTexture::SetTexture(gs_SSR_ResolvePID, CRenderer::GetDepthStencil()->m_nTextureId, 1);
	CTexture::SetTexture(gs_SSR_ResolvePID, ms_pRayDatas->m_nTextureId, 2);
	CTexture::SetTexture(gs_SSR_ResolvePID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, 3);
	CTexture::SetTexture(gs_SSR_ResolvePID, CDeferredRenderer::GetInfoTarget()->m_nTextureId, 4);

	CTexture::SetTexture(gs_SSR_ResolvePID, nSrc, 5);
	CSampler::BindSamplerState(5, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetCubeMap(gs_SSR_ResolvePID, CIBLBaker::GetBakedEnvMap(), 6);
	CSampler::BindSamplerState(6, CSampler::e_MinMagMip_Linear_UVW_Clamp);

	CTexture::SetTexture(gs_SSR_ResolvePID, CTAA::ms_pVelocityMap->m_nTextureId, 7);

	CTexture::SetTexture(gs_SSR_ResolvePID, ms_pHistory->m_nTextureId, 8);
	CSampler::BindSamplerState(8, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	SResolveConstants registers;

	registers.m_InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	registers.m_InvViewProj.transpose();

	registers.m_ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	registers.m_ViewProj.transpose();

	registers.m_Eye = CRenderer::GetCurrentCamera()->GetPosition();

	float fov = CRenderer::GetCurrentCamera()->GetFOV();

	registers.m_NearFar = float4(CRenderer::GetCurrentCamera()->GetNearPlane(), CRenderer::GetCurrentCamera()->GetFarPlane(), ms_pResolveTarget->m_nWidth / tanf(fov * 3.1415926f / 360.f), 0.f);

	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 32; i++)
		{
			if (j == 0)
				registers.m_SampleOffsets[i] = gs_SampleOffset1[i];

			else if (j == 1)
				registers.m_SampleOffsets[32 + i] = gs_SampleOffset2[i];

			else if (j == 2)
				registers.m_SampleOffsets[64 + i] = gs_SampleOffset3[i];

			else
				registers.m_SampleOffsets[96 + i] = gs_SampleOffset4[i];
		}
	}

	CDeviceManager::FillConstantBuffer(gs_SSR_ResolveConstants, &registers, sizeof(SResolveConstants));
	CDeviceManager::BindConstantBuffer(gs_SSR_ResolveConstants, gs_SSR_ResolvePID, 0);

	CDeviceManager::Dispatch((ms_pResolveTarget->m_nWidth + 31) / 32, (ms_pResolveTarget->m_nHeight + 31) / 32, 1);*/

	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}



void CSSR::TemporalFiltering()
{
	/*CShader::BindProgram(gs_SSR_TemporalPID);

	ms_pResolveTarget->m_pTexture->BindImage(0, CTexture::e_ReadWrite);

	CTexture::SetTexture(gs_SSR_TemporalPID, CTAA::ms_pVelocityMap->m_nTextureId, 0);

	CTexture::SetTexture(gs_SSR_TemporalPID, ms_pHistory->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CDeviceManager::Dispatch((ms_pResolveTarget->m_nWidth + 7) / 8, (ms_pResolveTarget->m_nHeight + 7) / 8, 1);*/
}



void CSSR::BilateralCleanUp()
{
	/*CFramebuffer::BindRenderTarget(0, ms_pBlurTarget);
	CFramebuffer::BindDepthStencil(NULL);
	CFramebuffer::SetDrawBuffers(1);

	CShader::BindProgram(gs_SSR_BlurPID);

	CTexture::SetTexture(gs_SSR_BlurPID, ms_pResolveTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CRenderStates::SetBlendingState(e_Opaque);

	CDeviceManager::SetUniform(gs_SSR_BlurPID, "PassCount", 0U);

	CRenderer::RenderQuadScreen();*/

	/*ms_pFBO->BindRenderTarget(0, ms_pResolveTarget);

	CDeviceManager::SetUniform(gs_SSR_BlurPID, "PassCount", 1U);

	CTexture::SetTexture(gs_SSR_BlurPID, ms_pBlurTarget->m_nTextureId, 0);

	CRenderer::RenderQuadScreen();*/
}



void CSSR::Merge(CTexture* pTarget)
{
	/*CShader::BindProgram(gs_SSR_MergePID);

	CFramebuffer::BindRenderTarget(0, pTarget);
	CFramebuffer::BindDepthStencil(NULL);
	CFramebuffer::SetDrawBuffers(1);

	CTexture::SetTexture(gs_SSR_MergePID, ms_pBlurTarget->m_nTextureId, 0);

	CTexture::SetTexture(gs_SSR_MergePID, CIBLBaker::GetBrdfMap(), 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetTexture(gs_SSR_MergePID, CDeferredRenderer::GetInfoTarget()->m_nTextureId, 2);
	CTexture::SetTexture(gs_SSR_MergePID, CDeferredRenderer::GetFresnelTarget()->m_nTextureId, 3);

	CRenderStates::SetBlendingState(e_Additive);

	CRenderer::RenderQuadScreen();

	SRenderTarget* pTmp = ms_pHistory;
	ms_pHistory = ms_pResolveTarget;
	ms_pResolveTarget = pTmp;*/

	/*CShader::BindProgram(gs_SSR_CopyPID);

	ms_pFBO->BindRenderTarget(0, ms_pLastFrameSSR);

	CTexture::SetTexture(gs_SSR_MergePID, pTarget->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMagMip_Point_UVW_Clamp);

	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetDepthStencil(e_Zero, false);

	CRenderer::RenderQuadScreen();*/
}

