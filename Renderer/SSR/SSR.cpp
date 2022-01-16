#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Timer/Timer.h"
#include "SSR.h"


CTexture* CSSR::ms_pRayData			= NULL;
CTexture* CSSR::ms_pZMips			= NULL;
CTexture* CSSR::ms_pRayLength		= NULL;
CTexture* CSSR::ms_pResolvedColor	= NULL;
CTexture* CSSR::ms_pBlurredColor	= NULL;


struct SSDFReflectionConstants
{
	float4 Center0;
	float4 Size0;
	float4 Center1;
	float4 Size1;
	float4 Center2;
	float4 Size2;
	float4 RealCenter;

	float4 SunColor;
	float4x4 SunShadowMatrix;
	float4 SunDir;
};



void CSSR::Init()
{
	int width	= CDeviceManager::GetDeviceWidth();
	int height	= CDeviceManager::GetDeviceHeight();

	ms_pRayData			= new CTexture(width, height, ETextureFormat::e_R16G16_UNORM, eTextureStorage2D);
	ms_pRayLength		= new CTexture(width, height, ETextureFormat::e_R16_FLOAT, eTextureStorage2D);
	ms_pResolvedColor	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pBlurredColor	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	width	= (width + 127) & ~127;
	height	= (height + 127) & ~127;

	ms_pZMips = new CTexture(width, height, ETextureFormat::e_R16_FLOAT, eTexture2D, 1, true);

	if (CRenderPass::BeginGraphics("Depth Mips"))
	{
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget());
			CRenderPass::SetNumSamplers(1, 1);

			CRenderPass::BindResourceToWrite(0, ms_pZMips->GetID(), -1, 0, CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("Bloom", "UpscaleDepth");

			CRenderPass::SetEntryPoint(UpscaleDepth);

			CRenderPass::EndSubPass();
		}

		for (int i = 1; i < 8; i++)
		{
			if (CRenderPass::BeginGraphicsSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pZMips->GetID(), -1, i - 1,	CShader::e_FragmentShader);
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(0, ms_pZMips->GetID(), -1, i,		CRenderPass::e_RenderTarget);

				CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

				CRenderPass::BindProgram("Bloom", "BuildHiZ");

				CRenderPass::SetEntryPoint(BuildHiZ);

				CRenderPass::EndSubPass();
			}
		}

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute("SDF Reflections"))
	{
		// Ray Trace Reflections
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(4, ms_pRayLength->GetID(),					CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("RayTraceSDFReflections");

			CRenderPass::SetEntryPoint(RayTraceSDFReflections);

			CRenderPass::EndSubPass();
		}

		// Light Rays
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetNormalTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, ms_pRayLength->GetID(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CLightField::GetIrradianceField(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CLightField::GetProbeMetadata(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CLightField::GetLightFieldSH(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(9, CLightField::GetLightFieldOcclusion(0, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(10, CLightField::GetLightFieldOcclusion(0, 1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CLightField::GetIrradianceField(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(12, CLightField::GetProbeMetadata(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(13, CLightField::GetLightFieldSH(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(14, CLightField::GetLightFieldOcclusion(1, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(15, CLightField::GetLightFieldOcclusion(1, 1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(16, CLightField::GetIrradianceField(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(17, CLightField::GetProbeMetadata(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(18, CLightField::GetLightFieldSH(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(19, CLightField::GetLightFieldOcclusion(2, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(20, CLightField::GetLightFieldOcclusion(2, 1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(21, CShadowRenderer::GetShadowmapArray(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(22, CShadowRenderer::GetSunShadowmapArray(), CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(23, 1);
			CRenderPass::BindResourceToRead(24, CSkybox::GetSkyboxTexture(), CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(25, ms_pResolvedColor->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("LightSDFReflections");

			CRenderPass::SetEntryPoint(LightSDFReflections);

			CRenderPass::EndSubPass();
		}

		// Merge
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetInfoTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetAlbedoTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(4, ms_pResolvedColor->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(5, CLightsManager::GetBRDFMap(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(6, CRenderer::ms_pSobolSequence32->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(7, CRenderer::ms_pOwenScrambling32->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(8, CRenderer::ms_pOwenRanking32->GetID(), CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(9, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "SSR_Merge");

			CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

			CRenderPass::SetEntryPoint(Merge);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}


	if (CRenderPass::BeginGraphics("SSR + SDF"))
	{
		// Ray Trace
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pZMips->GetID(),						CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindResourceToWrite(4, ms_pRayData->GetID(),					CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(5, ms_pRayLength->GetID(),					CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SSR_RayTracing");

			CRenderPass::SetEntryPoint(RayTrace);

			CRenderPass::EndSubPass();
		}

		// Resolve
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pRayData->GetID(),					CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(2, 1);

			CRenderPass::BindResourceToWrite(3, ms_pResolvedColor->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("SSR_Resolve");

			CRenderPass::SetEntryPoint(Resolve);

			CRenderPass::EndSubPass();
		}

		// Ray Trace Reflections
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetNormalTarget(), CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(4, ms_pRayLength->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("RayTraceSDFReflections");

			CRenderPass::SetEntryPoint(RayTraceSDFReflections);

			CRenderPass::EndSubPass();
		}

		// Light Rays
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetNormalTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, ms_pRayLength->GetID(),						CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CLightField::GetIrradianceField(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CLightField::GetProbeMetadata(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CLightField::GetLightFieldSH(0),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(9, CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(10, CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CLightField::GetIrradianceField(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(12, CLightField::GetProbeMetadata(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(13, CLightField::GetLightFieldSH(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(14, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(15, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(16, CLightField::GetIrradianceField(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(17, CLightField::GetProbeMetadata(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(18, CLightField::GetLightFieldSH(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(19, CLightField::GetLightFieldOcclusion(2, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(20, CLightField::GetLightFieldOcclusion(2, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(21, CShadowRenderer::GetShadowmapArray(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(22, CShadowRenderer::GetSunShadowmapArray(),	CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(23, 1);
			CRenderPass::BindResourceToRead(24, CSkybox::GetSkyboxTexture(),				CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(25, ms_pResolvedColor->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("LightSDFReflections");

			CRenderPass::SetEntryPoint(LightSDFReflections);

			CRenderPass::EndSubPass();
		}

		// Compute Blur Radius
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),	CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, ms_pRayLength->GetID(),					CShader::e_FragmentShader);

			CRenderPass::BindResourceToWrite(0, ms_pResolvedColor->GetID(),				CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "SSR_ComputeBlurRadius");

			CRenderPass::SetBlendState(false, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_Zero, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_Zero, EBlendOp::e_BlendOp_Add, 0x8);

			CRenderPass::SetEntryPoint(ComputeBlurRadius);

			CRenderPass::EndSubPass();
		}

		// Blur Reflections
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pResolvedColor->GetID(),					CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CRenderer::ms_pSobolSequence32->GetID(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, CRenderer::ms_pOwenScrambling32->GetID(),	CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(3, CRenderer::ms_pOwenRanking32->GetID(),		CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(4, 1);

			CRenderPass::BindResourceToWrite(0, ms_pBlurredColor->GetID(),					CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "SSR_Blur");

			CRenderPass::SetEntryPoint(Blur);

			CRenderPass::EndSubPass();
		}

		// Merge
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),			CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetInfoTarget(),			CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetAlbedoTarget(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(4, ms_pBlurredColor->GetID(),					CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(5, CLightsManager::GetBRDFMap(),				CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(6, CRenderer::ms_pSobolSequence32->GetID(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(7, CRenderer::ms_pOwenScrambling32->GetID(),	CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(8, CRenderer::ms_pOwenRanking32->GetID(),		CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(9, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),	CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "SSR_Merge");

			CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

			CRenderPass::SetEntryPoint(Merge);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}



void CSSR::UpscaleDepth()
{
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	float constants[2];
	constants[0] = CRenderer::GetNear4EngineFlush();
	constants[1] = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, constants, sizeof(constants));

	CRenderer::RenderQuadScreen();
}



void CSSR::BuildHiZ()
{
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}



void CSSR::RayTrace()
{
	CTimerManager::GetGPUTimer("SSR Trace Reflections")->Start();

	CRenderer::SetViewProjConstantBuffer(6);

	CResourceManager::SetSampler(3, ESamplerState::e_MinMagMip_Point_UVW_Clamp);

	float4 constants;
	constants.x = CRenderer::GetNear4EngineFlush();
	constants.y = CRenderer::GetFar4EngineFlush();
	constants.z = 0.f;
	constants.w = 1e8f;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	int width = CDeviceManager::GetDeviceWidth();
	int height = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, 1);

	CTimerManager::GetGPUTimer("SSR Trace Reflections")->Stop();
}


void CSSR::RayTraceSDFReflections()
{
	CTimerManager::GetGPUTimer("SDF Trace Reflections")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(5);

	CRenderer::SetViewProjConstantBuffer(6);

	CDeviceManager::Dispatch((ms_pRayData->GetWidth() + 7) / 8, (ms_pRayData->GetHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("SDF Trace Reflections")->Stop();
}


void CSSR::LightSDFReflections()
{
	CTimerManager::GetGPUTimer("SDF Light Reflections")->Start();

	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CResourceManager::SetSampler(23, e_ZComparison_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(26);

	CRenderer::SetViewProjConstantBuffer(27);
	CLightsManager::SetLightListConstantBuffer(28);
	CLightsManager::SetShadowLightListConstantBuffer(29);

	SSDFReflectionConstants constants;
	constants.Center0 = CLightField::GetCenter(0);
	constants.Size0 = CLightField::GetSize(0);
	constants.Center1 = CLightField::GetCenter(1);
	constants.Size1 = CLightField::GetSize(1);
	constants.Center2 = CLightField::GetCenter(2);
	constants.Size2 = CLightField::GetSize(2);
	constants.RealCenter = CLightField::GetRealCenter();

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();
		constants.SunShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		constants.SunShadowMatrix.transpose();

		constants.SunColor = float4(desc.m_Color, desc.m_fIntensity);
		constants.SunDir = desc.m_Dir;
	}

	else
		constants.SunColor.w = 0.f;

	constants.SunDir.w = CSkybox::GetSkyLightIntensity();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("SDF Light Reflections")->Stop();
}


void CSSR::Resolve()
{
	CTimerManager::GetGPUTimer("SSR Resolve")->Start();

	CResourceManager::SetSampler(2, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	int width = CDeviceManager::GetDeviceWidth();
	int height = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, 1);

	CTimerManager::GetGPUTimer("SSR Resolve")->Stop();
}


void CSSR::Blur()
{
	CTimerManager::GetGPUTimer("SSR Blur")->Start();

	CResourceManager::SetSampler(4, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	static unsigned int index = 1;

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &index, sizeof(index));

	index = (index + 1u) & 31u;

	CRenderer::RenderQuadScreen();

	CTimerManager::GetGPUTimer("SSR Blur")->Stop();
}



void CSSR::ComputeBlurRadius()
{
	CRenderer::SetViewProjConstantBuffer(3);

	float fFOV = CRenderer::GetFOV4EngineFlush();
	float constant = tanf(fFOV * 3.14159262f / 360.f);

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &constant, sizeof(constant));

	CRenderer::RenderQuadScreen();
}



void CSSR::Merge()
{
	CRenderer::SetViewProjConstantBuffer(10);
	CResourceManager::SetSampler(9, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	static unsigned int index = 1;

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &index, sizeof(index));

	index = (index + 1u) & 31u;

	CRenderer::RenderQuadScreen();
}

