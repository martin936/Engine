#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Textures/TextureInterface.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "SDFGI.h"


CTexture* CSDFGI::ms_pUnfilteredChromaHistoryIn		= nullptr;
CTexture* CSDFGI::ms_pUnfilteredChromaHistoryOut	= nullptr;
CTexture* CSDFGI::ms_pUnfilteredLumaHistoryIn		= nullptr;
CTexture* CSDFGI::ms_pUnfilteredLumaHistoryOut		= nullptr;
CTexture* CSDFGI::ms_pFilteredChromaIn				= nullptr;
CTexture* CSDFGI::ms_pFilteredChromaOut				= nullptr;
CTexture* CSDFGI::ms_pFilteredLumaIn				= nullptr;
CTexture* CSDFGI::ms_pFilteredLumaOut				= nullptr;
CTexture* CSDFGI::ms_pMomentsHistoryIn				= nullptr;
CTexture* CSDFGI::ms_pMomentsHistoryOut				= nullptr;
CTexture* CSDFGI::ms_pNoisyChroma					= nullptr;
CTexture* CSDFGI::ms_pNoisyLumaSH					= nullptr;
CTexture* CSDFGI::ms_pIrradianceCache				= nullptr;
CTexture* CSDFGI::ms_pLastFlatNormal				= nullptr;


struct SSDFGIConstants
{
	float4 Center0;
	float4 Size0;
	float4 Center1;
	float4 Size1;
	float4 Center2;
	float4 Size2;
	float4 RealCenter;

	float4 SunColor;
	float4 SunDir;

	unsigned int TemporalOffset;
};


struct SSDFGIFullConstants
{
	float4x4 ShadowMatrix;

	float4 Center0;
	float4 Size0;
	float4 Center1;
	float4 Size1;
	float4 Center2;
	float4 Size2;
	float4 RealCenter;

	float4 SunColor;
	float4 SunDir;

	unsigned int TemporalOffset;
};


struct SSDFGIReprojConstants
{
	float4x4	InvViewProj;
	float4x4	LastInvViewProj;
	float		Near;
	float		Far;
};


void CSDFGI::Init()
{
	int width				= CDeviceManager::GetDeviceWidth();
	int height				= CDeviceManager::GetDeviceHeight();

	ms_pUnfilteredChromaHistoryIn	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pUnfilteredChromaHistoryOut	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pUnfilteredLumaHistoryIn		= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pUnfilteredLumaHistoryOut	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pFilteredChromaIn			= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pFilteredChromaOut			= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pFilteredLumaIn				= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pFilteredLumaOut				= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pMomentsHistoryIn			= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pMomentsHistoryOut			= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pNoisyChroma					= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT,	eTextureStorage2D);
	ms_pNoisyLumaSH					= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pLastFlatNormal				= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	ms_pIrradianceCache = new CTexture(128, 128, 128, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);

	if (CRenderPass::BeginCompute("SDFGI"))
	{
		// Build Irradiance Cache
		/*if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3,	CLightField::GetIrradianceField(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4,	CLightField::GetProbeMetadata(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5,	CLightField::GetLightFieldSH(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6,	CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7,	CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8,	CLightField::GetIrradianceField(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(9,	CLightField::GetProbeMetadata(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(10,	CLightField::GetLightFieldSH(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(12, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(13, CLightField::GetIrradianceField(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(14, CLightField::GetProbeMetadata(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(15, CLightField::GetLightFieldSH(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(16, CLightField::GetLightFieldOcclusion(2, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(17, CLightField::GetLightFieldOcclusion(2, 1),	CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(18, ms_pIrradianceCache->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("BuildIrradianceCache");

			CRenderPass::SetEntryPoint(BuildIrradianceCache);

			CRenderPass::EndSubPass();
		}

		// Compute 1spp Ray-Traced GI
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetFlatNormalTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, ms_pIrradianceCache->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, CSkybox::GetSkyboxTexture(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CRenderer::ms_pSobolSequence32->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CRenderer::ms_pOwenScrambling32->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CRenderer::ms_pOwenRanking32->GetID(),		CShader::e_ComputeShader);
			
			CRenderPass::BindResourceToWrite(9, ms_pNoisyChroma->GetID(),					CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(10, ms_pNoisyLumaSH->GetID(),					CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SDFGI");

			CRenderPass::SetEntryPoint(RayTraceGI);

			CRenderPass::EndSubPass();
		}*/

		// Compute 1spp Ray-Traced GI without cache
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetFlatNormalTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, CSkybox::GetSkyboxTexture(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CRenderer::ms_pSobolSequence32->GetID(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CRenderer::ms_pOwenScrambling32->GetID(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CRenderer::ms_pOwenRanking32->GetID(), CShader::e_ComputeShader);
		
			CRenderPass::BindResourceToRead(9, CLightField::GetIrradianceField(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(10, CLightField::GetProbeMetadata(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CLightField::GetLightFieldSH(0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(12, CLightField::GetLightFieldOcclusion(0, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(13, CLightField::GetLightFieldOcclusion(0, 1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(14, CLightField::GetIrradianceField(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(15, CLightField::GetProbeMetadata(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(16, CLightField::GetLightFieldSH(1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(17, CLightField::GetLightFieldOcclusion(1, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(18, CLightField::GetLightFieldOcclusion(1, 1), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(19, CLightField::GetIrradianceField(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(20, CLightField::GetProbeMetadata(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(21, CLightField::GetLightFieldSH(2), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(22, CLightField::GetLightFieldOcclusion(2, 0), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(23, CLightField::GetLightFieldOcclusion(2, 1), CShader::e_ComputeShader);
		
			CRenderPass::BindResourceToRead(24, CShadowRenderer::GetSunShadowmapArray(), CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(25, 1);
		
			CRenderPass::BindResourceToWrite(26, ms_pNoisyChroma->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(27, ms_pNoisyLumaSH->GetID(), CRenderPass::e_UnorderedAccess);
		
			CRenderPass::BindProgram("SDFGI_Full");
		
			CRenderPass::SetEntryPoint(RayTraceGIFull);
		
			CRenderPass::EndSubPass();
		}

		// Update History
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetFlatNormalTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pLastFlatNormal->GetID(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetLastDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetMotionVectorTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, ms_pUnfilteredChromaHistoryIn->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, ms_pUnfilteredLumaHistoryIn->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, ms_pMomentsHistoryIn->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, ms_pNoisyChroma->GetID(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(9, ms_pNoisyLumaSH->GetID(),					CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(10, ms_pUnfilteredChromaHistoryOut->GetID(),	CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(11, ms_pUnfilteredLumaHistoryOut->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(12, ms_pMomentsHistoryOut->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SDFGI_UpdateHistory");

			CRenderPass::SetEntryPoint(UpdateHistory);

			CRenderPass::EndSubPass();
		}

		// Filter Moments
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pMomentsHistoryOut->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pUnfilteredChromaHistoryOut->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pUnfilteredLumaHistoryOut->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetFlatNormalTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(5, ms_pFilteredChromaOut->GetID(),				CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(6, ms_pFilteredLumaOut->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SDFGI_FilterMoments");

			CRenderPass::SetEntryPoint(FilterMoments);

			CRenderPass::EndSubPass();
		}

		// Edge-Avoiding A-Trous Filtering
		for (int i = 0; i < 6; i++)
		{
			if (CRenderPass::BeginComputeSubPass())
			{	
				CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetFlatNormalTarget(),	CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(2, ms_pMomentsHistoryOut->GetID(),				CShader::e_ComputeShader);

				if (i & 1)
				{
					CRenderPass::BindResourceToRead(3, ms_pFilteredChromaIn->GetID(),			CShader::e_ComputeShader);
					CRenderPass::BindResourceToRead(4, ms_pFilteredLumaIn->GetID(),				CShader::e_ComputeShader);

					CRenderPass::BindResourceToWrite(5, ms_pFilteredChromaOut->GetID(),			CRenderPass::e_UnorderedAccess);
					CRenderPass::BindResourceToWrite(6, ms_pFilteredLumaOut->GetID(),			CRenderPass::e_UnorderedAccess);
				}

				else
				{
					CRenderPass::BindResourceToRead(3, ms_pFilteredChromaOut->GetID(),			CShader::e_ComputeShader);
					CRenderPass::BindResourceToRead(4, ms_pFilteredLumaOut->GetID(),			CShader::e_ComputeShader);

					CRenderPass::BindResourceToWrite(5, ms_pFilteredChromaIn->GetID(),			CRenderPass::e_UnorderedAccess);
					CRenderPass::BindResourceToWrite(6, ms_pFilteredLumaIn->GetID(),			CRenderPass::e_UnorderedAccess);
				}

				CRenderPass::BindProgram("SDFGI_ATrousFiltering");

				CRenderPass::SetEntryPoint(ATrousFiltering, reinterpret_cast<void*>(&i), sizeof(i));

				CRenderPass::EndSubPass();
			}
		}

		// Merge
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_pFilteredChromaOut->GetID(),				CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1,	ms_pFilteredLumaOut->GetID(),				CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2,	CDeferredRenderer::GetNormalTarget(),		CShader::e_FragmentShader);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetDiffuseTarget(),		CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "SDFGI_Merge");

			CRenderPass::SetBlendState(EBlendState::e_BlendState_Additive);

			CRenderPass::SetEntryPoint(Merge);

			CRenderPass::EndSubPass();
		}

		// Copy History
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pUnfilteredChromaHistoryOut->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pUnfilteredLumaHistoryOut->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pMomentsHistoryOut->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetFlatNormalTarget(),	CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(4, ms_pUnfilteredChromaHistoryIn->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(5, ms_pUnfilteredLumaHistoryIn->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(6, ms_pMomentsHistoryIn->GetID(),				CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(7, ms_pLastFlatNormal->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SDFGI_CopyHistory");

			CRenderPass::SetEntryPoint(CopyHistory);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}

	if (CRenderPass::BeginCompute("Show SDF"))
	{
		CRenderPass::SetNumTextures(0, 1024);
		CRenderPass::BindResourceToRead(1, CSDFGI::GetIrradianceCache(), CShader::e_ComputeShader);
		CRenderPass::SetNumSamplers(2, 1);

		CRenderPass::BindResourceToWrite(5, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_UnorderedAccess);

		CRenderPass::BindProgram("ShowSDF");

		CRenderPass::SetEntryPoint(CSDF::ShowSDF);

		CRenderPass::End();
	}
}


void CSDFGI::BuildIrradianceCache()
{
	CTimerManager::GetGPUTimer("Irradiance Cache")->Start();

	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(19);
	CLightsManager::SetLightListConstantBuffer(20);
	CLightsManager::SetShadowLightListConstantBuffer(21);

	SSDFGIConstants constants;
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

		constants.SunColor = float4(desc.m_Color, desc.m_fIntensity);
		constants.SunDir = desc.m_Dir;
	}

	else
		constants.SunColor.w = 0.f;

	static unsigned int offset = 0;

	constants.TemporalOffset = offset;
	offset++;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pIrradianceCache->GetWidth() + 7) / 8, (ms_pIrradianceCache->GetHeight() + 7) / 8, (ms_pIrradianceCache->GetDepth() + 7) / 8);

	CTimerManager::GetGPUTimer("Irradiance Cache")->Stop();
}


void CSDFGI::RayTraceGI()
{
	CTimerManager::GetGPUTimer("Ray-Trace GI")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(11);
	CRenderer::SetViewProjConstantBuffer(12);

	static unsigned int offset = 0;

	float constants[2];
	constants[0] = CSkybox::GetSkyLightIntensity();
	constants[1] = *reinterpret_cast<float*>(&offset);

	offset++;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("Ray-Trace GI")->Stop();
}


void CSDFGI::RayTraceGIFull()
{
	CTimerManager::GetGPUTimer("Ray-Trace GI")->Start();

	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CResourceManager::SetSampler(25, e_ZComparison_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(28);
	CLightsManager::SetLightListConstantBuffer(29);
	CLightsManager::SetShadowLightListConstantBuffer(30);
	CRenderer::SetViewProjConstantBuffer(31);

	SSDFGIFullConstants constants;
	constants.ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
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

		constants.SunColor = float4(desc.m_Color, desc.m_fIntensity);
		constants.SunDir = desc.m_Dir;
	}

	else
		constants.SunColor.w = 0.f;

	static unsigned int offset = 0;

	constants.TemporalOffset = offset;
	offset++;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("Ray-Trace GI")->Stop();
}


void CSDFGI::UpdateHistory()
{
	CTimerManager::GetGPUTimer("Reproject GI")->Start();

	SSDFGIReprojConstants constants;
	constants.InvViewProj		= CRenderer::GetInvViewProjMatrix4EngineFlush();
	constants.LastInvViewProj	= CRenderer::GetLastInvViewProjMatrix4EngineFlush();
	constants.Near				= CRenderer::GetNear4EngineFlush();
	constants.Far				= CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("Reproject GI")->Stop();
}


void CSDFGI::FilterMoments()
{
	CTimerManager::GetGPUTimer("Filter Moments")->Start();

	float constants[2];
	constants[0] = CRenderer::GetNear4EngineFlush();
	constants[1] = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("Filter Moments")->Stop();
}


void CSDFGI::ATrousFiltering(void* pData)
{
	int step = 1 << (*reinterpret_cast<int*>(pData));

	float constants[3];
	constants[0] = *reinterpret_cast<float*>(&step);
	constants[1] = CRenderer::GetNear4EngineFlush();
	constants[2] = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);
}


void CSDFGI::Merge()
{
	CRenderer::RenderQuadScreen();
}


void CSDFGI::CopyHistory()
{
	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);
}

