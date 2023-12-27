#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "VolumetricMedia.h"


CTexture*		CVolumetricMedia::ms_pScatteringValue		= nullptr;
CTexture*		CVolumetricMedia::ms_pScatteredLight		= nullptr;
CTexture*		CVolumetricMedia::ms_pScatteredLightHistory = nullptr;
CTexture*		CVolumetricMedia::ms_pIntegratedLight		= nullptr;

float3			CVolumetricMedia::ms_Scattering				= float3(0.f, 0.f, 0.f);
float			CVolumetricMedia::ms_Absorption				= 0.f;


struct SOITSunConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
	float4		m_SunDir;
};


void CVolumetricMedia::Init()
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pScatteringValue			= new CTexture(192, 128, 64, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);
	ms_pScatteredLight			= new CTexture(192, 128, 64, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);
	ms_pScatteredLightHistory	= new CTexture(192, 128, 64, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);
	ms_pIntegratedLight			= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	//if (CRenderPass::BeginCompute("Fill Scattering Grid"))
	//{
	//	CRenderPass::BindResourceToRead(0,	ms_pScatteredLight->GetID(),		CShader::e_ComputeShader);
	//	CRenderPass::BindResourceToWrite(1, ms_pScatteringValue->GetID(),		CRenderPass::e_UnorderedAccess);
	//	CRenderPass::BindResourceToWrite(2, ms_pScatteredLightHistory->GetID(),	CRenderPass::e_UnorderedAccess);

	//	CRenderPass::BindProgram("FillScatteringGrid");

	//	CRenderPass::SetEntryPoint(FillGrid);

	//	CRenderPass::End();
	//}

	//if (CRenderPass::BeginCompute("Compute Volumetrics"))
	//{
	//	// Light Scattering
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0,	CLightsManager::GetLightGridTexture(),		CShader::e_ComputeShader);
	//		CRenderPass::BindResourceToRead(1,	CLightsManager::GetLightIndexBuffer(),		CShader::e_ComputeShader, CRenderPass::e_Buffer);
	//		CRenderPass::BindResourceToRead(2,	ms_pScatteringValue->GetID(),				CShader::e_ComputeShader);
	//		CRenderPass::BindResourceToRead(3,	CShadowRenderer::GetShadowmapArray(),		CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(4,	CShadowRenderer::GetSunShadowmapArray(),	CShader::e_FragmentShader);
	//		CRenderPass::SetNumSamplers(5, 1);
	//		CRenderPass::BindResourceToRead(6,	CLightField::GetIrradianceField(0),			CShader::e_FragmentShader);
	//		//CRenderPass::BindResourceToRead(7,	CLightField::GetFieldDepth(),				CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(8,	CLightField::GetProbeMetadata(0),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(9,	ms_pScatteredLightHistory->GetID(),			CShader::e_FragmentShader);
	//		CRenderPass::SetNumSamplers(10, 1);

	//		CRenderPass::BindResourceToWrite(11, ms_pScatteredLight->GetID(),			CRenderPass::e_UnorderedAccess);

	//		CRenderPass::BindProgram("ComputeScatteredLight");

	//		CRenderPass::SetEntryPoint(ComputeScatteredLight);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Ray Integration
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0,	CDeferredRenderer::GetDepthTarget(),	CShader::e_ComputeShader);	
	//		CRenderPass::BindResourceToRead(1,	ms_pScatteredLight->GetID(),			CShader::e_ComputeShader);
	//		CRenderPass::SetNumSamplers(2, 1);
	//		CRenderPass::BindResourceToWrite(3, ms_pIntegratedLight->GetID(),			CRenderPass::e_UnorderedAccess);

	//		CRenderPass::BindProgram("IntegrateRays");

	//		CRenderPass::SetEntryPoint(IntegrateRays);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Merge
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0,	ms_pIntegratedLight->GetID(),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),	CRenderPass::e_RenderTarget);

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

	//		CRenderPass::BindProgram("copy", "MergeVolumetrics");

	//		CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_SrcAlpha, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

	//		CRenderPass::SetEntryPoint(Merge);

	//		CRenderPass::EndSubPass();
	//	}

	//	CRenderPass::End();
	//}
}




void CVolumetricMedia::FillGrid()
{
	float4 scatt = float4(ms_Scattering, ms_Absorption);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &scatt, sizeof(float4));

	CDeviceManager::Dispatch((ms_pScatteringValue->GetWidth() + 7) / 8, (ms_pScatteringValue->GetHeight() + 7) / 8, (ms_pScatteringValue->GetDepth() + 7) / 8);
}



void CVolumetricMedia::ComputeScatteredLight()
{
	CRenderer::SetViewProjConstantBuffer(12);
	CLightsManager::SetLightListConstantBuffer(13);
	CLightsManager::SetShadowLightListConstantBuffer(14);

	CResourceManager::SetSampler(5, e_ZComparison_Linear_UVW_Clamp);
	CResourceManager::SetSampler(10, e_MinMagMip_Linear_UVW_Clamp);

	CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

	SOITSunConstants sunConstants;
	sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
	sunConstants.m_ShadowMatrix.transpose();
	sunConstants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
	sunConstants.m_SunDir = float4(desc.m_Dir, 0.f);

	CResourceManager::SetConstantBuffer(15, &sunConstants, sizeof(sunConstants));

	static unsigned int offset = 1;

	float4 constants[3];
	constants[0] = float4(CLightField::GetCenter(0), CRenderer::GetNear4EngineFlush());
	constants[1] = float4(CLightField::GetSize(0), CRenderer::GetFar4EngineFlush());
	constants[2].x = offset * (1.f / 16.f);

	offset = (offset + 7U) & 15U;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, 2 * sizeof(float4) + sizeof(float));

	CTimerManager::GetGPUTimer("Compute Light Scattering")->Start();

	CDeviceManager::Dispatch((ms_pScatteredLight->GetWidth() + 7) / 8, (ms_pScatteredLight->GetHeight() + 7) / 8, (ms_pScatteredLight->GetDepth() + 7) / 8);

	CTimerManager::GetGPUTimer("Compute Light Scattering")->Stop();
}



void CVolumetricMedia::IntegrateRays()
{
	float NearFar[2];
	NearFar[0] = CRenderer::GetNear4EngineFlush();
	NearFar[1] = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, NearFar, sizeof(NearFar));

	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	
	CTimerManager::GetGPUTimer("Integrate Scattered Light")->Start();

	CDeviceManager::Dispatch((ms_pIntegratedLight->GetWidth() + 7) / 8, (ms_pIntegratedLight->GetHeight() + 7) / 8, 1);

	CTimerManager::GetGPUTimer("Integrate Scattered Light")->Stop();
}



void CVolumetricMedia::Merge()
{
	CRenderer::RenderQuadScreen();
}
