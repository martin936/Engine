#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "LightField.h"
#include <vector>



unsigned int g_GenerateLightFieldCommandListID = 0;


int			CLightField::ms_nNumProbes[3] = { 0 };
int			CLightField::ms_nTotalNumProbes = 0;

bool		CLightField::ms_bIsLightFieldGenerated	= false;
bool		CLightField::ms_bShowLightField			= false;
bool		CLightField::ms_bEnable					= false;

int			CLightField::ms_nNumRenderedProbes = 0;
int			CLightField::ms_nNumProbesInBatch = 0;

CTexture*	CLightField::ms_LightFieldIrradiance;
CTexture*	CLightField::ms_LightFieldMetaData;

CTexture*	CLightField::ms_SurfelDist;
CTexture*	CLightField::ms_SurfelIrradiance;
//CTexture*	CLightField::ms_LightFieldGradient;
//CTexture*	CLightField::ms_LightFieldDepth;
//
//CTexture*	CLightField::ms_SurfelDepth;
//CTexture*	CLightField::ms_pLightFieldRayData;
//
//CTexture*	CLightField::ms_pLightFieldDepthMaps;
//CTexture*	CLightField::ms_pLightFieldLowDepthMaps;
//CTexture*	CLightField::ms_pLightFieldGBuffer;
//
//CTexture*	CLightField::ms_pLightFieldDepthCubeMaps;
//CTexture*	CLightField::ms_pLightFieldGBufferCubeMaps;

float3		CLightField::ms_Center;
float3		CLightField::ms_Size;

//float		CLightField::ms_fBias = 0.3f;
//float		CLightField::ms_fMinCellAxis = 1.f;


float4x4	gs_LightFieldMatrices[64 * 6];
float3		gs_LightFieldPos[64];


struct SShowLightFieldConstants
{
	float3		m_Up;
	float3		m_Right;
	float3		m_Center;
	float3		m_Size;
	int			m_NumProbes[4];
};


struct SSunConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
	float4		m_SunDir;
};


struct SOITSunConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
	float4		m_SunDir;
};


struct SConstants
{
	float4 m_Center;
	float4 m_Size;
	float4 m_Eye;

	unsigned int m_FrameIndex;
	float m_Near;
	float m_Far;
};


extern bool gs_bEnableDiffuseGI_Saved;

extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);



void CLightField::Init(int numProbesX, int numProbesY, int numProbesZ)
{
	ms_nNumProbes[0] = numProbesX;
	ms_nNumProbes[1] = numProbesY;
	ms_nNumProbes[2] = numProbesZ;

	ms_nTotalNumProbes = numProbesX * numProbesY * numProbesZ;

	ms_LightFieldIrradiance			= new CTexture(numProbesX * 10, numProbesY * 10, numProbesZ, ETextureFormat::e_R32_UINT, eTextureStorage2DArray);
	//ms_LightFieldDepth			= new CTexture(numProbesX * 18, numProbesY * 18, numProbesZ, ETextureFormat::e_R16G16_FLOAT, eTextureStorage2DArray);

	ms_LightFieldMetaData			= new CTexture(numProbesX, numProbesY, numProbesZ, ETextureFormat::e_R8G8B8A8_SINT, eTextureStorage2DArray);

	ms_SurfelIrradiance				= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ, ETextureFormat::e_R32_UINT, eTextureStorage2DArray);
	ms_SurfelDist					= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ, ETextureFormat::e_R16_FLOAT, eTextureStorage2DArray);
	//ms_SurfelDepth				= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ, ETextureFormat::e_R32_FLOAT, eTextureStorage2DArray);

	//ms_pLightFieldDepthMaps		= new CTexture(128 * numProbesX, 128 * numProbesY, numProbesZ, ETextureFormat::e_R32_FLOAT, eTextureStorage2DArray);
	//ms_pLightFieldGBuffer			= new CTexture(128 * numProbesX, 128 * numProbesY, numProbesZ, ETextureFormat::e_R16G16B16A16_UINT, eTextureStorage2DArray);

	//ms_pLightFieldDepthCubeMaps	= new CTexture(128, 128, 64, ETextureFormat::e_R24_DEPTH_G8_STENCIL, eCubeMapArray);
	//ms_pLightFieldGBufferCubeMaps	= new CTexture(128, 128, 64, ETextureFormat::e_R16G16B16A16_UINT, eCubeMapArray);

	//int nWidth	= CDeviceManager::GetDeviceWidth();
	//int nHeight = CDeviceManager::GetDeviceHeight();

	//ms_pLightFieldRayData			= new CTexture(nWidth, nHeight, ETextureFormat::e_R32G32B32A32_UINT, eTextureStorage2D);

	//g_GenerateLightFieldCommandListID = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	ms_nNumRenderedProbes = 0;

	ms_bIsLightFieldGenerated = false;
}


void CLightField::InitRenderPasses()
{
	//int numRenderPasses = MIN(1, (ms_nTotalNumProbes + 63) / 64);

	//char name[1024] = "";

	/*if (CRenderPass::BeginCompute("Update Light Field"))
	{
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pLightFieldGBuffer->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pLightFieldDepthMaps->GetID(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, CLightsManager::GetStaticLightGridTexture(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CLightsManager::GetStaticLightIndexBuffer(), CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(4, ms_LightFieldIrradiance->GetID(),			CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(5, 1024);
			CRenderPass::BindResourceToRead(5, ms_LightFieldDepth->GetID(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, ms_LightFieldMetaData->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CShadowRenderer::GetShadowmapArray(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CShadowDir::GetSunShadowmapArray(),			CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(9, 1);
			CRenderPass::BindResourceToRead(10, CMaterial::GetMaterialBuffer(),				CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::SetNumTextures(11, 1024);
			CRenderPass::SetNumSamplers(12, 1);
			CRenderPass::SetNumTextures(13, 1);

			CRenderPass::BindResourceToWrite(14, ms_SurfelIrradiance->GetID(),				CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(15, ms_SurfelDepth->GetID(),					CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("ComputeLightFieldSamples");

			CRenderPass::SetEntryPoint(CLightField::ComputeLightFieldSamples);

			CRenderPass::End();
		}

		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_SurfelIrradiance->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1,	ms_LightFieldMetaData->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(2, ms_LightFieldIrradiance->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateLightField");

			CRenderPass::SetEntryPoint(CLightField::UpdateLightField);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_SurfelDepth->GetID(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(1, ms_LightFieldDepth->GetID(),				CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_LightFieldMetaData->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateFieldDepth");

			CRenderPass::SetEntryPoint(CLightField::UpdateFieldDepth);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_LightFieldIrradiance->GetID(),			CRenderPass::e_UnorderedAccess);
			//CRenderPass::BindResourceToWrite(1, ms_LightFieldGradient->GetID(),				CRenderPass::e_UnorderedAccess);
			//CRenderPass::BindResourceToWrite(2, ms_LightFieldDepth->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateLightFieldBorder");

			CRenderPass::SetEntryPoint(CLightField::UpdateLightFieldBorder);

			CRenderPass::EndSubPass();
		}
		
		CRenderPass::End();
	}*/

	/*if (CRenderPass::BeginGraphics("Ray Trace Light Field"))
	{
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pLightFieldDepthMaps->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, ms_pLightFieldLowDepthMaps->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, ms_pLightFieldGBuffer->GetID(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetNormalTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetDepthTarget(), CShader::e_FragmentShader);

			CRenderPass::BindResourceToWrite(0, ms_pLightFieldRayData->GetID(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("ComputeLighting", "LightField_RayTrace");

			CRenderPass::SetEntryPoint(CLightField::RayTraceLightField);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}*/

	if (CRenderPass::BeginCompute("Update Light Field"))
	{
		// Update Probe Position
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);

			CRenderPass::BindResourceToWrite(3, ms_LightFieldMetaData->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateProbePosition");

			CRenderPass::SetEntryPoint(UpdateProbePosition);

			CRenderPass::EndSubPass();
		}

		// Ray March Samples
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToRead(2, ms_LightFieldMetaData->GetID(), CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(3, ms_SurfelDist->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("RayMarchProbeSamples");

			CRenderPass::SetEntryPoint(RayMarchSamples);

			CRenderPass::EndSubPass();
		}

		// Light Samples
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3, ms_LightFieldMetaData->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, ms_SurfelDist->GetID(),						CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, CLightsManager::GetStaticLightGridTexture(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CLightsManager::GetStaticLightIndexBuffer(), CShader::e_ComputeShader,	CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(7, CShadowRenderer::GetShadowmapArray(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CShadowDir::GetSunShadowmapArray(),			CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(9, 1);
			CRenderPass::SetNumTextures(10, 1);
			CRenderPass::BindResourceToRead(11, ms_LightFieldIrradiance->GetID(),			CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(12, ms_SurfelIrradiance->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("LightProbeSamples");

			CRenderPass::SetEntryPoint(LightSamples);

			CRenderPass::EndSubPass();
		}

		// Update Probes
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_SurfelIrradiance->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1,	ms_LightFieldMetaData->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(2, ms_LightFieldIrradiance->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateLightField");

			CRenderPass::SetEntryPoint(CLightField::UpdateLightField);

			CRenderPass::EndSubPass();
		}

		// Update Border Texels
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_LightFieldIrradiance->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("UpdateLightFieldBorder");

			CRenderPass::SetEntryPoint(CLightField::UpdateLightFieldBorder);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}

	if (CRenderPass::BeginGraphics("Show Light Field"))
	{
		CRenderPass::BindResourceToRead(0, ms_LightFieldIrradiance->GetID(),				CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, ms_LightFieldMetaData->GetID(),					CShader::e_VertexShader | CShader::e_FragmentShader);
		CRenderPass::SetNumSamplers(2, 1, CShader::e_VertexShader | CShader::e_FragmentShader);

		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),			CRenderPass::e_RenderTarget);
		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("ShowLightField", "ShowLightField");

		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);

		CRenderPass::SetEntryPoint(CLightField::ShowLightField);

		CRenderPass::End();
	}


	/*for (int i = 0; i < numRenderPasses; i++)
	{
		sprintf(name, "Build Light Field %d", i);
		if (CRenderPass::BeginGraphics(name))
		{
			if (CRenderPass::BeginGraphicsSubPass())
			{
				CRenderPass::BindResourceToWrite(0, ms_pLightFieldGBufferCubeMaps->GetID(), CRenderPass::e_RenderTarget);
				CRenderPass::BindDepthStencil(ms_pLightFieldDepthCubeMaps->GetID());

				CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

				CRenderPass::BindProgram("BuildLightField", "BuildLightField", "BuildLightField");

				CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
				CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, false, 1.f / 65536.f, -3.f);

				CRenderPass::SetEntryPoint(CLightField::BuildLightField);

				CRenderPass::EndSubPass();
			}

			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pLightFieldGBufferCubeMaps->GetID(),	CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(1, ms_pLightFieldDepthCubeMaps->GetID(),	CShader::e_ComputeShader);
				CRenderPass::SetNumSamplers(2, 1);

				CRenderPass::BindResourceToWrite(3, ms_pLightFieldGBuffer->GetID(),		CRenderPass::e_UnorderedAccess);
				CRenderPass::BindResourceToWrite(4, ms_pLightFieldDepthMaps->GetID(),	CRenderPass::e_UnorderedAccess);
				CRenderPass::BindResourceToWrite(5, ms_LightFieldMetaData->GetID(),		CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("LightField_WriteOctahedronMaps");

				CRenderPass::SetEntryPoint(CLightField::WriteOctahedronMaps);

				CRenderPass::EndSubPass();
			}

			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pLightFieldDepthMaps->GetID(),		CShader::e_ComputeShader);
				CRenderPass::BindResourceToWrite(1, ms_pLightFieldLowDepthMaps->GetID(),	CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("LightField_ReduceDepthMaps");

				CRenderPass::SetEntryPoint(CLightField::ReduceDepthMaps);

				CRenderPass::EndSubPass();
			}

			CRenderPass::End();
		}
	}*/
}


void Encode(float3& v)
{
	float3 p;
	p.x = v.x * (1.f / (abs(v.x) + abs(v.y) + abs(v.z)));
	p.y = v.y * (1.f / (abs(v.x) + abs(v.y) + abs(v.z)));

	if (v.z <= 0.f)
	{
		p.x = (1.f - abs(p.x)) * ((v.x >= 0.f) ? 1.f : -1.f);
		p.y = (1.f - abs(p.y)) * ((v.y >= 0.f) ? 1.f : -1.f);
	}

	v = p * 0.5f + float3(0.5f, 0.5f, 0.5f);
}


void CLightField::UpdateProbePosition()
{
	CTimerManager::GetGPUTimer("Update Probe Position")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(2);

	float4 constants[2];
	constants[0] = ms_Center;
	constants[1] = ms_Size;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumProbes[0] + 3) / 4, (ms_nNumProbes[1] + 3) / 4, (ms_nNumProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Update Probe Position")->Stop();
}


void CLightField::RayMarchSamples()
{
	CTimerManager::GetGPUTimer("Ray March Probe Samples")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(4);

	float sampleCoords[512];
	static int index = 1;

	float angle = VanDerCorput3(index);
	float offset = VanDerCorput2(index);

	for (int i = 0; i < 256; i++)
	{
		float x = (i + offset) / 256.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = 2.f * sqrtf(MAX(0.f, x * (1.f - x)));
		float sin_th = 1.f - 2.f * x;

		float3 dir = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);

		Encode(dir);

		sampleCoords[2 * i] = dir.x;
		sampleCoords[2 * i + 1] = dir.y;
	}

	index++;

	CResourceManager::SetConstantBuffer(5, sampleCoords, sizeof(sampleCoords));

	float4 constants[2];
	constants[0]	= ms_Center;
	constants[0].w	= CRenderer::GetNear4EngineFlush();
	constants[1]	= ms_Size;
	constants[1].w	= CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch(((ms_nNumProbes[0] + 3) / 4) * 16, ((ms_nNumProbes[1] + 3) / 4) * 16, (ms_nNumProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Ray March Probe Samples")->Stop();
}


void CLightField::LightSamples()
{
	CTimerManager::GetGPUTimer("Light Probe Samples")->Start();

	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CResourceManager::SetSampler(9, e_ZComparison_Linear_UVW_Clamp);
	CTextureInterface::SetTexture(CSkybox::GetSkyboxTexture(), 10);

	CSDF::SetSDFConstantBuffer(13);
	CLightsManager::SetLightListConstantBuffer(14);
	CLightsManager::SetShadowLightListConstantBuffer(15);

	SSunConstants sunConstants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		sunConstants.m_ShadowMatrix.transpose();
		sunConstants.m_SunColor		= float4(desc.m_Color, desc.m_fIntensity);
		sunConstants.m_SunDir		= float4(desc.m_Dir, 0.f);
	}

	else
	{
		sunConstants.m_SunColor = 0.f;
	}

	CResourceManager::SetConstantBuffer(16, &sunConstants, sizeof(sunConstants));

	float sampleCoords[512];
	static int index = 1;

	float angle = VanDerCorput3(index);
	float offset = VanDerCorput2(index);

	for (int i = 0; i < 256; i++)
	{
		float x = (i + offset) / 256.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = 2.f * sqrtf(MAX(0.f, x * (1.f - x)));
		float sin_th = 1.f - 2.f * x;

		float3 dir = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);

		Encode(dir);

		sampleCoords[2 * i] = dir.x;
		sampleCoords[2 * i + 1] = dir.y;
	}

	index++;

	CResourceManager::SetConstantBuffer(17, sampleCoords, sizeof(sampleCoords));

	float4 constants[3];
	constants[0] = CLightField::GetCenter();
	constants[1] = CLightField::GetSize();
	constants[2].x = CSkybox::GetSkyLightIntensity();
	constants[2].y = CRenderer::GetNear4EngineFlush();
	constants[2].z = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch(((ms_nNumProbes[0] + 3) / 4) * 16, ((ms_nNumProbes[1] + 3) / 4) * 16, (ms_nNumProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Light Probe Samples")->Stop();
}



void CLightField::RayTraceLightField()
{
	CTimerManager::GetGPUTimer("Ray Trace Light Field")->Start();

	CRenderer::SetViewProjConstantBuffer(5);

	float4 constants[2];
	constants[0]	= CLightField::GetCenter();
	constants[0].w	= CRenderer::GetNear4EngineFlush();
	constants[1]	= CLightField::GetSize();
	constants[1].w	= CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, constants, sizeof(constants));

	CRenderer::RenderQuadScreen();

	CTimerManager::GetGPUTimer("Ray Trace Light Field")->Stop();
}


void CLightField::ComputeReflections()
{
	/*CResourceManager::SetSampler(4, e_Anisotropic_Linear_UVW_Wrap);
	CResourceManager::SetSampler(7, e_ZComparison_Linear_UVW_Clamp);
	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialBuffer(15);
	CLightsManager::SetLightListConstantBuffer(16);
	CLightsManager::SetShadowLightListConstantBuffer(17);

	float sampleCoords[32];

	static int index = 1;

	float offset = 0.5f;//VanDerCorput2(index);
	float angle = 2.f * M_PI * VanDerCorput3(index);

	for (int i = 0; i < 16; i++)
	{
		float x = i + offset;
		float r = sqrtf(x / 16.f);
		float theta = 3.88322f * x + angle;

		sampleCoords[2 * i] = r * cosf(theta);
		sampleCoords[2 * i + 1] = r * sinf(theta);
	}

	CResourceManager::SetConstantBuffer(17, sampleCoords, sizeof(sampleCoords));

	CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

	SOITSunConstants sunConstants;
	sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
	sunConstants.m_ShadowMatrix.transpose();
	sunConstants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
	sunConstants.m_SunDir = float4(desc.m_Dir, 0.f);

	CResourceManager::SetConstantBuffer(18, &sunConstants, sizeof(sunConstants));

	float FOV = CRenderer::GetFOV4EngineFlush();
	float NearPlane = CRenderer::GetNear4EngineFlush();
	float FarPlane = CRenderer::GetFar4EngineFlush();

	SConstants constant;

	constant.m_Center = CLightField::GetCenter();
	constant.m_Center.w = CLightField::GetMinCellAxis();
	constant.m_Size = CLightField::GetSize();
	constant.m_Size.w = CLightField::GetBias();
	constant.m_Eye = CRenderer::GetViewerPosition4EngineFlush();
	constant.m_Eye.w = gs_bEnableDiffuseGI_Saved ? 1.f : 0.f;

	constant.m_FrameIndex = index;
	constant.m_Near = CRenderer::GetNear4EngineFlush();
	constant.m_Far = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &constant, 3 * sizeof(float4) + sizeof(unsigned int));

	CRenderer::RenderQuadScreen();*/
}



void CLightField::Generate()
{
	std::vector<SRenderPassTask> renderpasses;

	int numRenderPasses = MIN(1, (ms_nTotalNumProbes + 63) / 64);
	char name[1024] = "";

	for (int i = 0; i < numRenderPasses; i++)
	{
		sprintf(name, "Build Light Field %d", i);
		renderpasses.push_back(CRenderPass::GetRenderPassTask(name));
	}

	CSchedulerThread::AddRenderTask(g_GenerateLightFieldCommandListID, renderpasses);
	CCommandListManager::ScheduleDeferredKickoff(g_GenerateLightFieldCommandListID);
}


void CLightField::UpdateLightField()
{
	CTimerManager::GetGPUTimer("Update Irradiance Probes")->Start();

	static int index = 1;

	float sampleCoords[512];

	float angle = VanDerCorput3(index);
	float offset = VanDerCorput2(index);

	for (int i = 0; i < 256; i++)
	{
		float x = (i + offset) / 256.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = 2.f * sqrtf(MAX(0.f, x * (1.f - x)));
		float sin_th = 1.f - 2.f * x;

		float3 dir = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);

		Encode(dir);

		sampleCoords[2 * i] = dir.x;
		sampleCoords[2 * i + 1] = dir.y;
	}

	index++;

	CResourceManager::SetConstantBuffer(3, sampleCoords, sizeof(sampleCoords));

	CDeviceManager::Dispatch(CLightField::ms_nNumProbes[0], CLightField::ms_nNumProbes[1], CLightField::ms_nNumProbes[2] * 2);

	CTimerManager::GetGPUTimer("Update Irradiance Probes")->Stop();
}


void CLightField::UpdateFieldDepth()
{
	/*static int index = 1;

	if (index > 200)
		return;

	CTimerManager::GetGPUTimer("Update Probes Visibility")->Start();

	float sampleCoords[512];

	float angle = VanDerCorput3(index);

	for (int i = 0; i < 256; i++)
	{
		float x = (i + 0.5f) / 256.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = 2.f * sqrtf(MAX(0.f, x * (1.f - x)));
		float sin_th = 1.f - 2.f * x;

		float3 dir = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);

		Encode(dir);

		sampleCoords[2 * i] = dir.x;
		sampleCoords[2 * i + 1] = dir.y;
	}

	index++;

	CResourceManager::SetConstantBuffer(3, sampleCoords, sizeof(sampleCoords));

	CDeviceManager::Dispatch(CLightField::ms_nNumProbes[0], CLightField::ms_nNumProbes[1], CLightField::ms_nNumProbes[2] * 8);

	CTimerManager::GetGPUTimer("Update Probes Visibility")->Stop();*/
}


void CLightField::UpdateLightFieldBorder()
{
	CDeviceManager::Dispatch(ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2]);
}


void CLightField::ComputeLightFieldGradient()
{
	CDeviceManager::Dispatch(ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2]);
}


void CLightField::ComputeLightFieldSamples()
{
	/*CLightsManager::WaitForLightList();

	CTimerManager::GetGPUTimer("Compute Light Field Samples")->Start();

	float4 constants[3];
	constants[0]	= CLightField::GetCenter();
	constants[0].w	= CLightField::GetMinCellAxis();
	constants[1]	= CLightField::GetSize();
	constants[1].w	= CLightField::GetBias();
	constants[2].x	= CSkybox::GetSkyLightIntensity();
	constants[2].y	= CRenderer::GetNear4EngineFlush();
	constants[2].z	= CRenderer::GetFar4EngineFlush();

	CSDF::BindSDFs(5);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CResourceManager::SetSampler(9, e_ZComparison_Linear_UVW_Clamp);

	CMaterial::BindMaterialTextures(11);
	CResourceManager::SetSampler(12, e_MinMagMip_Linear_UVW_Wrap);

	CTextureInterface::SetTexture(CSkybox::GetSkyboxTexture(), 13);

	CLightsManager::SetLightListConstantBuffer(16);
	CLightsManager::SetShadowLightListConstantBuffer(17);

	SSunConstants sunConstants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		sunConstants.m_ShadowMatrix.transpose();
		sunConstants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
		sunConstants.m_SunDir = float4(desc.m_Dir, 0.f);
	}

	else
	{
		sunConstants.m_SunColor = 0.f;
	}

	CResourceManager::SetConstantBuffer(18, &sunConstants, sizeof(sunConstants));

	static int index = 1;

	float sampleCoords[512];

	float offset = VanDerCorput2(index);
	float angle = VanDerCorput3(index);

	for (int i = 0; i < 256; i++)
	{
		float x = (i + offset) / 256.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = 2.f * sqrtf(MAX(0.f, x * (1.f - x)));
		float sin_th = 1.f - 2.f * x;

		float3 dir = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);

		Encode(dir);

		sampleCoords[2 * i] = dir.x;
		sampleCoords[2 * i + 1] = dir.y;
	}

	index++;

	CResourceManager::SetConstantBuffer(19, sampleCoords, sizeof(sampleCoords));
	CSDF::SetSDFConstantBuffer(20);

	CDeviceManager::Dispatch(CLightField::ms_nTotalNumProbes, 1, 1);

	CTimerManager::GetGPUTimer("Compute Light Field Samples")->Stop();*/
}



void CLightField::BuildLightField()
{
	int numProbesLeft = CLightField::ms_nTotalNumProbes - CLightField::ms_nNumRenderedProbes;

	if (numProbesLeft < 0)
		return;

	CLightField::ms_nNumProbesInBatch = MIN(64, numProbesLeft);

	float Near = CRenderer::GetNear4EngineFlush();
	float Far = CRenderer::GetFar4EngineFlush();

	float d = 1.f / (Far - Near);
	float a = 0.5f - 0.5f * (Far + Near) * d;
	float b = (Far * Near) * d;

	std::vector<unsigned int> slices;

	for (int i = 0; i < CLightField::ms_nNumProbesInBatch; i++)
	{
		int probeIndex = CLightField::ms_nNumRenderedProbes + i;
		int z = probeIndex / (CLightField::ms_nNumProbes[0] * CLightField::ms_nNumProbes[1]);
		int y = (probeIndex - z * CLightField::ms_nNumProbes[0] * CLightField::ms_nNumProbes[1]) / CLightField::ms_nNumProbes[0];
		int x = probeIndex - z * CLightField::ms_nNumProbes[0] * CLightField::ms_nNumProbes[1] - y * CLightField::ms_nNumProbes[0];

		float3 pos = CLightField::GetCenter() + float3((x + 0.5f) / CLightField::ms_nNumProbes[0] - 0.5f, (y + 0.5f) / CLightField::ms_nNumProbes[1] - 0.5f, (z + 0.5f) / CLightField::ms_nNumProbes[2] - 0.5f) * CLightField::GetSize();

		gs_LightFieldPos[i] = pos;

		gs_LightFieldMatrices[6 * i + 0] = float4x4(float4(0.f, 0.f, -1.f, pos.z), float4(0.f, -1.f, 0.f, pos.y), float4(a, 0.f, 0.f, b - a * pos.x), float4(1.f, 0.f, 0.f, -pos.x));
		gs_LightFieldMatrices[6 * i + 1] = float4x4(float4(0.f, 0.f, 1.f, -pos.z), float4(0.f, -1.f, 0.f, pos.y), float4(-a, 0.f, 0.f, b + a * pos.x), float4(-1.f, 0.f, 0.f, pos.x));
		gs_LightFieldMatrices[6 * i + 2] = float4x4(float4(1.f, 0.f, 0.f, -pos.x), float4(0.f, 0.f, -1.f, pos.z), float4(0.f, -a, 0.f, b + a * pos.y), float4(0.f, -1.f, 0.f, pos.y));
		gs_LightFieldMatrices[6 * i + 3] = float4x4(float4(1.f, 0.f, 0.f, -pos.x), float4(0.f, 0.f, 1.f, -pos.z), float4(0.f, a, 0.f, b - a * pos.y), float4(0.f, 1.f, 0.f, -pos.y));
		gs_LightFieldMatrices[6 * i + 4] = float4x4(float4(1.f, 0.f, 0.f, -pos.x), float4(0.f, -1.f, 0.f, pos.y), float4(0.f, 0.f, a, b - a * pos.z), float4(0.f, 0.f, 1.f, -pos.z));
		gs_LightFieldMatrices[6 * i + 5] = float4x4(float4(-1.f, 0.f, 0.f, pos.x), float4(0.f, -1.f, 0.f, pos.y), float4(0.f, 0.f, -a, b + a * pos.z), float4(0.f, 0.f, -1.f, pos.z));

		gs_LightFieldMatrices[6 * i + 0].transpose();
		gs_LightFieldMatrices[6 * i + 1].transpose();
		gs_LightFieldMatrices[6 * i + 2].transpose();
		gs_LightFieldMatrices[6 * i + 3].transpose();
		gs_LightFieldMatrices[6 * i + 4].transpose();
		gs_LightFieldMatrices[6 * i + 5].transpose();

		slices.push_back(6 * i + 0);
		slices.push_back(6 * i + 1);
		slices.push_back(6 * i + 2);
		slices.push_back(6 * i + 3);
		slices.push_back(6 * i + 4);
		slices.push_back(6 * i + 5);
	}

	CDeviceManager::ClearDepth(slices, 0.f);

	CResourceManager::SetConstantBuffer(0, gs_LightFieldMatrices, sizeof(gs_LightFieldMatrices));
	CResourceManager::SetConstantBuffer(1, gs_LightFieldPos, sizeof(gs_LightFieldPos));

	CPacketManager::ForceShaderHook(CLightField::UpdateShader);

	CRenderer::DisableViewportCheck();

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}



void CLightField::WriteOctahedronMaps()
{
	CResourceManager::SetSampler(2, e_MinMagMip_Point_UVW_Clamp);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &CLightField::ms_nNumRenderedProbes, sizeof(int));

	CDeviceManager::Dispatch(16, 16, CLightField::ms_nNumProbesInBatch);
}



void CLightField::ReduceDepthMaps()
{
	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &CLightField::ms_nNumRenderedProbes, sizeof(int));

	CDeviceManager::Dispatch(16, 16, CLightField::ms_nNumProbesInBatch);

	CLightField::ms_nNumRenderedProbes += 64;
	if (CLightField::ms_nNumRenderedProbes >= CLightField::ms_nTotalNumProbes)
	{
		CLightField::ms_nNumRenderedProbes = 0;
		CLightField::ms_bIsLightFieldGenerated = true;
	}
}



void CLightField::ShowLightField()
{
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);

	SShowLightFieldConstants constants;

	float4x4 View = CRenderer::GetViewMatrix4EngineFlush();
	View.transpose();

	Copy(constants.m_Right.v(), View.m());
	Copy(constants.m_Up.v(),	View.m() + 4);

	constants.m_Center = CLightField::GetCenter();
	constants.m_Size = CLightField::GetSize();

	constants.m_NumProbes[0] = CLightField::ms_nNumProbes[0];
	constants.m_NumProbes[1] = CLightField::ms_nNumProbes[1];
	constants.m_NumProbes[2] = CLightField::ms_nNumProbes[2];
	
	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_FragmentShader, &constants, sizeof(constants));

	CRenderer::SetViewProjConstantBuffer(3);

	CRenderer::RenderQuadScreen(CLightField::ms_nTotalNumProbes);
}



int	CLightField::UpdateShader(Packet* packet, void* pData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)pData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	unsigned int viewportMask[15] = { 0 };
	unsigned int numInstances = ms_nNumProbesInBatch * 6;

	/*for (int i = 0; i < ms_nNumProbesInBatch; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			gs_LightFieldMatrices[i * 6 + j].transpose();

			if (CViewportManager::IsVisible(gs_LightFieldPos[i], gs_LightFieldMatrices[i * 6 + j], packet->m_Center, packet->m_fBoundingSphereRadius))
			{
				int viewportID = i * 6 + j;
				viewportMask[viewportID / 32] |= (1U << (viewportID & 31));
				numInstances++;
			}
		}
	}*/

	pShaderData->m_nNbInstances = numInstances;

	float n = CRenderer::GetNear4EngineFlush();
	float f = CRenderer::GetFar4EngineFlush();

	unsigned int matID = packet->m_pMaterial->GetID();
	viewportMask[12] = matID;
	viewportMask[13] = *reinterpret_cast<unsigned int*>(&n);
	viewportMask[14] = *reinterpret_cast<unsigned int*>(&f);

	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_FragmentShader, viewportMask, sizeof(viewportMask));
	
	return 1;
}

