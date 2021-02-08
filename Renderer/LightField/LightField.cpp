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


#define FP16_IRRADIANCE_PROBES	1


unsigned int g_GenerateLightFieldCommandListID = 0;


int			CLightField::ms_nNumProbes[3] = { 0 };
int			CLightField::ms_nTotalNumProbes = 0;

bool		CLightField::ms_bIsLightFieldGenerated	= false;
bool		CLightField::ms_bShowLightField			= false;
bool		CLightField::ms_bEnable					= false;
bool		CLightField::ms_bRefreshOcclusion[2]	= { true, true };

int			CLightField::ms_nNumRenderedProbes = 0;
int			CLightField::ms_nNumProbesInBatch = 0;

float		CLightField::ms_fProbesDisplaySize = 0.1f;

CTexture*	CLightField::ms_LightFieldSH[ms_NumCascades];
CTexture*	CLightField::ms_LightFieldIrradiance[ms_NumCascades];
CTexture*	CLightField::ms_LightFieldMetaData[ms_NumCascades];
CTexture*	CLightField::ms_LightFieldOcclusion[ms_NumCascades][2];

CTexture*	CLightField::ms_SurfelDist[ms_NumCascades];
CTexture*	CLightField::ms_SurfelIrradiance[ms_NumCascades];

CTexture*	CLightField::ms_RayData;
CTexture*	CLightField::ms_RayColor;

float3		CLightField::ms_Center[ms_NumCascades];
float3		CLightField::ms_Center4EngineFlush[ms_NumCascades];
float3		CLightField::ms_LastCenter4EngineFlush[ms_NumCascades];
float3		CLightField::ms_Size[ms_NumCascades];

float4x4	gs_LightFieldMatrices[64 * 6];
float3		gs_LightFieldPos[64];


struct SShowLightFieldConstants
{
	float4		m_Up;
	float4		m_Right;
	float4		m_Center0;
	float4		m_Size0;
	float4		m_Center1;
	float4		m_Size1;
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


struct SReflectionConstants
{
	float4 Center;
	float4 Size;

	float4x4 SunShadowMatrix;
	float4 SunColor;
	float4 SunDir;
};


extern bool gs_bEnableDiffuseGI_Saved;

extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);



void CLightField::Init(int numProbesX, int numProbesY, int numProbesZ)
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_nNumProbes[0] = numProbesX;
	ms_nNumProbes[1] = numProbesY;
	ms_nNumProbes[2] = numProbesZ;

	ms_nTotalNumProbes = numProbesX * numProbesY * numProbesZ;

	for (int i = 0; i < ms_NumCascades; i++)
	{
	#if FP16_IRRADIANCE_PROBES
		ms_LightFieldIrradiance[i]			= new CTexture(numProbesX * 10, numProbesY * 10, numProbesZ, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2DArray);
	#else
		ms_LightFieldIrradiance[i]			= new CTexture(numProbesX * 10, numProbesY * 10, numProbesZ, ETextureFormat::e_R32_UINT, eTextureStorage2DArray);
	#endif

		ms_LightFieldSH[i]					= new CTexture(numProbesX * 4, numProbesY * 4, numProbesZ, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2DArray);

		ms_LightFieldMetaData[i]			= new CTexture(numProbesX, numProbesY, numProbesZ, ETextureFormat::e_R8G8B8A8_SINT, eTextureStorage2DArray);

		ms_LightFieldOcclusion[i][0]		= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ * 16, ETextureFormat::e_R8G8B8A8, eTextureStorage3D);
		ms_LightFieldOcclusion[i][1]		= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ * 16, ETextureFormat::e_R8G8B8A8, eTextureStorage3D);

		ms_SurfelIrradiance[i]				= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ, ETextureFormat::e_R32_UINT, eTextureStorage2DArray);
		ms_SurfelDist[i]					= new CTexture(numProbesX * 16, numProbesY * 16, numProbesZ, ETextureFormat::e_R16_FLOAT, eTextureStorage2DArray);

		numProbesX = MAX(1, numProbesX / 2);
		numProbesY = MAX(1, numProbesY / 2);
		numProbesZ = MAX(1, numProbesZ / 2);
	}

	ms_RayData						= new CTexture(nWidth, nHeight, ETextureFormat::e_R16_FLOAT, eTextureStorage2D);
	ms_RayColor						= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	ms_nNumRenderedProbes = 0;

	ms_bIsLightFieldGenerated = false;
}


void CLightField::InitRenderPasses()
{
	if (CRenderPass::BeginCompute("Update Light Field"))
	{
		// Update Probe Position
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::SetNumTextures(0, 1024);
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(3, ms_LightFieldMetaData[i]->GetID(), CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("UpdateProbePosition");

				CRenderPass::SetEntryPoint(UpdateProbePosition, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Reproject Light Field
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToWrite(0, ms_LightFieldIrradiance[i]->GetID(),	CRenderPass::e_UnorderedAccess);
				CRenderPass::BindResourceToWrite(1, ms_LightFieldSH[i]->GetID(),			CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("ReprojectLightField");

				CRenderPass::SetEntryPoint(ReprojectLightField, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Compute Occlusion
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::SetNumTextures(0, 1024);
				CRenderPass::SetNumSamplers(1, 1);
				CRenderPass::BindResourceToRead(2, ms_LightFieldMetaData[i]->GetID(), CShader::e_ComputeShader);

				CRenderPass::BindResourceToWrite(3, ms_LightFieldOcclusion[i][0]->GetID(), CRenderPass::e_UnorderedAccess);
				CRenderPass::BindResourceToWrite(4, ms_LightFieldOcclusion[i][1]->GetID(), CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("ComputeOcclusion");

				CRenderPass::SetEntryPoint(ComputeOcclusion, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Ray March Samples
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::SetNumTextures(0, 1024);
				CRenderPass::SetNumSamplers(1, 1);
				CRenderPass::BindResourceToRead(2, ms_LightFieldMetaData[i]->GetID(), CShader::e_ComputeShader);

				CRenderPass::BindResourceToWrite(3, ms_SurfelDist[i]->GetID(), CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("RayMarchProbeSamples");

				CRenderPass::SetEntryPoint(RayMarchSamples, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Light Samples
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::SetNumTextures(0, 1024);
				CRenderPass::SetNumTextures(1, 1024);
				CRenderPass::SetNumSamplers(2, 1);
				CRenderPass::BindResourceToRead(3, ms_LightFieldMetaData[0]->GetID(),				CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(4, ms_LightFieldMetaData[1]->GetID(),				CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(5, ms_SurfelDist[i]->GetID(),						CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(6, CLightsManager::GetStaticLightGridTexture(),		CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(7, CLightsManager::GetStaticLightIndexBuffer(),		CShader::e_ComputeShader,	CRenderPass::e_Buffer);
				CRenderPass::BindResourceToRead(8, CShadowRenderer::GetShadowmapArray(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(9, CShadowDir::GetSunShadowmapArray(),				CShader::e_ComputeShader);
				CRenderPass::SetNumSamplers(10, 1);
				CRenderPass::SetNumTextures(11, 1);
				CRenderPass::BindResourceToRead(12, ms_LightFieldIrradiance[0]->GetID(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(13, ms_LightFieldOcclusion[0][0]->GetID(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(14, ms_LightFieldOcclusion[0][1]->GetID(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(15, ms_LightFieldIrradiance[1]->GetID(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(16, ms_LightFieldOcclusion[1][0]->GetID(),			CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(17, ms_LightFieldOcclusion[1][1]->GetID(),			CShader::e_ComputeShader);

				CRenderPass::BindResourceToWrite(18, ms_SurfelIrradiance[i]->GetID(),				CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("LightProbeSamples");

				CRenderPass::SetEntryPoint(LightSamples, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Update Probes
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToRead(0,	ms_SurfelIrradiance[i]->GetID(),				CShader::e_ComputeShader);
				CRenderPass::BindResourceToRead(1,	ms_LightFieldMetaData[i]->GetID(),				CShader::e_ComputeShader);
				CRenderPass::BindResourceToWrite(2, ms_LightFieldIrradiance[i]->GetID(),			CRenderPass::e_UnorderedAccess);
				CRenderPass::BindResourceToWrite(3, ms_LightFieldSH[i]->GetID(),					CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("UpdateLightField");

				CRenderPass::SetEntryPoint(CLightField::UpdateLightField, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		// Update Border Texels
		for (int i = 0; i < ms_NumCascades; i++)
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToWrite(0, ms_LightFieldIrradiance[i]->GetID(), CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("UpdateLightFieldBorder");

				CRenderPass::SetEntryPoint(CLightField::UpdateLightFieldBorder, (void*)&i, sizeof(int));

				CRenderPass::EndSubPass();
			}

		CRenderPass::End();
	}

	if (CRenderPass::BeginCompute("Light Field Reflections"))
	{
		// Ray Trace Reflections
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetNormalTarget(), CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(4, ms_RayData->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("RayTraceReflections");

			CRenderPass::SetEntryPoint(RayTraceReflections);

			CRenderPass::EndSubPass();
		}

		// Light Rays
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumTextures(1, 1024);
			CRenderPass::SetNumSamplers(2, 1);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, ms_RayData->GetID(),						CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CLightField::GetIrradianceField(0),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CLightField::GetProbeMetadata(0),			CShader::e_ComputeShader); 
			CRenderPass::BindResourceToRead(8, CLightField::GetLightFieldOcclusion(0, 0),			CShader::e_ComputeShader); 
			CRenderPass::BindResourceToRead(9, CLightField::GetLightFieldOcclusion(0, 1),			CShader::e_ComputeShader); 
			CRenderPass::BindResourceToRead(10, CShadowRenderer::GetShadowmapArray(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CShadowRenderer::GetSunShadowmapArray(), CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(12, 1);

			CRenderPass::BindResourceToWrite(13, ms_RayColor->GetID(),					CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("LightReflections");

			CRenderPass::SetEntryPoint(LightReflections);

			CRenderPass::EndSubPass();
		}

		// Apply Reflections
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),	CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetInfoTarget(),		CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(3, ms_RayColor->GetID(),					CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(4, CLightsManager::GetBRDFMap(),			CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(5, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),	CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("Merge", "ApplyReflections");

			CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

			CRenderPass::SetEntryPoint(ApplyReflections);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}

	if (CRenderPass::BeginGraphics("Show Light Field"))
	{
		CRenderPass::BindResourceToRead(0, ms_LightFieldIrradiance[0]->GetID(),				CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, ms_LightFieldMetaData[0]->GetID(),					CShader::e_VertexShader);
		CRenderPass::SetNumSamplers(2, 1, CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(3, ms_LightFieldSH[0]->GetID(),						CShader::e_FragmentShader);

		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),			CRenderPass::e_RenderTarget);
		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("ShowLightField", "ShowLightField");

		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);

		CRenderPass::SetEntryPoint(CLightField::ShowLightField);

		CRenderPass::End();
	}
}


void CLightField::SetCenter(float3& center)
{
	int numProbesX = ms_nNumProbes[0];
	int numProbesY = ms_nNumProbes[1];
	int numProbesZ = ms_nNumProbes[2];

	for (int i = 0; i < ms_NumCascades; i++)
	{
		float3 cellSize = ms_Size[i] / float3(numProbesX, numProbesY, numProbesZ);
		float3 coords = center / cellSize;
		coords = float3(floor(coords.x), floor(coords.y), floor(coords.z));

		ms_Center[i] = coords * cellSize;

		numProbesX = MAX(1, numProbesX / 2);
		numProbesY = MAX(1, numProbesY / 2);
		numProbesZ = MAX(1, numProbesZ / 2);
	}
}


void CLightField::UpdateBeforeFlush()
{
	for (int i = 0; i < ms_NumCascades; i++)
	{
		if (ms_Center4EngineFlush[i].x != ms_Center[i].x || ms_Center4EngineFlush[i].y != ms_Center[i].y || ms_Center4EngineFlush[i].z != ms_Center[i].z)
			ms_bRefreshOcclusion[i] = true;

		ms_LastCenter4EngineFlush[i] = ms_Center4EngineFlush[i];
		ms_Center4EngineFlush[i] = ms_Center[i];
	}
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


void CLightField::UpdateProbePosition(void* pData)
{
	CTimerManager::GetGPUTimer("Update Probe Position")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(2);

	int cascade = *reinterpret_cast<int*>(pData);

	float4 constants[2];
	constants[0] = ms_Center4EngineFlush[cascade];
	constants[1] = ms_Size[cascade];

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((numProbes[0] + 3) / 4, (numProbes[1] + 3) / 4, (numProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Update Probe Position")->Stop();
}


void CLightField::ComputeOcclusion(void* pData)
{
	int cascade = *reinterpret_cast<int*>(pData);

	if (!ms_bRefreshOcclusion[cascade])
		return;

	ms_bRefreshOcclusion[cascade] = false;

	CTimerManager::GetGPUTimer("Compute Light Field Occlusion")->Start();

	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(5);

	float4 constants[2];
	constants[0]	= ms_Center4EngineFlush[cascade];
	constants[1]	= ms_Size[cascade];

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(4 * numProbes[0], 4 * numProbes[1], 4 * numProbes[2]);

	CTimerManager::GetGPUTimer("Compute Light Field Occlusion")->Stop();
}


void CLightField::ReprojectLightField(void* pData)
{
	if (!ms_bRefreshOcclusion)
		return;

	int cascade = *reinterpret_cast<int*>(pData);

	float4 constants[4];
	constants[0] = ms_Center4EngineFlush[cascade];
	constants[1] = ms_Size[cascade];
	constants[2] = ms_LastCenter4EngineFlush[cascade];
	constants[3] = ms_Size[cascade];

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(numProbes[0], numProbes[1], numProbes[2]);
}


void CLightField::RayMarchSamples(void* pData)
{
	CTimerManager::GetGPUTimer("Ray March Probe Samples")->Start();

	int cascade = *reinterpret_cast<int*>(pData);

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
	constants[0]	= ms_Center4EngineFlush[cascade];
	constants[0].w	= CRenderer::GetNear4EngineFlush();
	constants[1]	= ms_Size[cascade];
	constants[1].w	= CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(((numProbes[0] + 3) / 4) * 16, ((numProbes[1] + 3) / 4) * 16, (numProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Ray March Probe Samples")->Stop();
}


void CLightField::LightSamples(void* pData)
{
	CTimerManager::GetGPUTimer("Light Probe Samples")->Start();

	int cascade = *reinterpret_cast<int*>(pData);

	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CResourceManager::SetSampler(10, e_ZComparison_Linear_UVW_Clamp);
	CTextureInterface::SetTexture(CSkybox::GetSkyboxTexture(), 11);

	CSDF::SetSDFConstantBuffer(19);
	CLightsManager::SetLightListConstantBuffer(20);
	CLightsManager::SetShadowLightListConstantBuffer(21);

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

	CResourceManager::SetConstantBuffer(22, &sunConstants, sizeof(sunConstants));

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

	CResourceManager::SetConstantBuffer(23, sampleCoords, sizeof(sampleCoords));

	float4 constants[5];
	constants[0] = CLightField::GetCenter(0);
	constants[1] = CLightField::GetSize(0);
	constants[2] = CLightField::GetCenter(1);
	constants[3] = CLightField::GetSize(1);
	constants[4].x = CSkybox::GetSkyLightIntensity();
	constants[4].y = CRenderer::GetNear4EngineFlush();
	constants[4].z = CRenderer::GetFar4EngineFlush();
	constants[4].w = 1.f * cascade;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(((numProbes[0] + 3) / 4) * 16, ((numProbes[1] + 3) / 4) * 16, (numProbes[2] + 3) / 4);

	CTimerManager::GetGPUTimer("Light Probe Samples")->Stop();
}


void CLightField::UpdateLightField(void* pData)
{
	CTimerManager::GetGPUTimer("Update Irradiance Probes")->Start();

	int cascade = *reinterpret_cast<int*>(pData);

	static int index = 1;
	static int reset = 1;

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

	CResourceManager::SetConstantBuffer(4, sampleCoords, sizeof(sampleCoords));

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &reset, sizeof(int));

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(numProbes[0], numProbes[1], numProbes[2] * 2);

	reset = 0;

	CTimerManager::GetGPUTimer("Update Irradiance Probes")->Stop();
}


void CLightField::UpdateLightFieldBorder(void* pData)
{
	int cascade = *reinterpret_cast<int*>(pData);

	int numProbes[3] = { ms_nNumProbes[0], ms_nNumProbes[1], ms_nNumProbes[2] };

	for (int i = 0; i < cascade; i++)
	{
		for (int j = 0; j < 3; j++)
			numProbes[j] = MAX(1, numProbes[j] / 2);
	}

	CDeviceManager::Dispatch(numProbes[0], numProbes[1], numProbes[2]);
}


void CLightField::ShowLightField()
{
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);

	SShowLightFieldConstants constants;

	float4x4 View = CRenderer::GetViewMatrix4EngineFlush();
	View.transpose();

	Copy(constants.m_Right.v(), View.m());
	Copy(constants.m_Up.v(),	View.m() + 4);

	constants.m_Up.w = ms_fProbesDisplaySize;

	constants.m_Center0 = CLightField::GetCenter(0);
	constants.m_Size0	= CLightField::GetSize(0);
	constants.m_Center1 = CLightField::GetCenter(1);
	constants.m_Size1	= CLightField::GetSize(1);
	
	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_FragmentShader, &constants, sizeof(constants));

	CRenderer::SetViewProjConstantBuffer(4);

	CRenderer::RenderQuadScreen(CLightField::ms_nTotalNumProbes);
}


void CLightField::RayTraceReflections()
{
	CSDF::BindSDFs(0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(5);

	CRenderer::SetViewProjConstantBuffer(6);

	CDeviceManager::Dispatch((ms_RayData->GetWidth() + 7) / 8, (ms_RayData->GetHeight() + 7) / 8, 1);
}


void CLightField::ApplyReflections()
{
	CResourceManager::SetSampler(5, e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::SetViewProjConstantBuffer(6);

	unsigned int index = Randi(1, RAND_MAX);

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &index, sizeof(unsigned int));

	CRenderer::RenderQuadScreen();
}


void CLightField::LightReflections()
{
	CSDF::BindSDFs(0);
	CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CResourceManager::SetSampler(12, e_ZComparison_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(14);

	CRenderer::SetViewProjConstantBuffer(15);
	CLightsManager::SetLightListConstantBuffer(16);
	CLightsManager::SetShadowLightListConstantBuffer(17);

	SReflectionConstants constants;
	constants.Center	= CLightField::GetCenter(0);
	constants.Size		= CLightField::GetSize(0);
	constants.Size.w	= tanf(CRenderer::GetFOV4EngineFlush() * (3.1415926f / 360.f));

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();
		constants.SunShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		constants.SunShadowMatrix.transpose();

		constants.SunColor	= float4(desc.m_Color, desc.m_fIntensity);
		constants.SunDir	= desc.m_Dir;
	}

	else
		constants.SunColor.w = 0.f;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((CDeviceManager::GetDeviceWidth() + 7) / 8, (CDeviceManager::GetDeviceHeight() + 7) / 8, 1);
}
