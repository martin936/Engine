#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Timer/Timer.h"
#include "DOF.h"

float CDOF::ms_fFocalLength = 10.f;
float CDOF::ms_fAperture	= 0.1f;


void DOF_ComputeTiles_EntryPoint();
void DOF_ComputeTilesNeighbourhood_EntryPoint();
void DOF_Prefilter_EntryPoint();
void DOF_Filter_EntryPoint();
void DOF_PostFilter_EntryPoint();
void DOF_Merge_EntryPoint();


CTexture* CDOF::ms_pCoCTiles					= nullptr;
CTexture* CDOF::ms_pBlurredCoCTiles				= nullptr;
CTexture* CDOF::ms_pPresortTarget				= nullptr;
CTexture* CDOF::ms_pPrefilteredColor			= nullptr;
CTexture* CDOF::ms_pFilterTarget				= nullptr;

CTexture* CDOF::ms_pPresortTarget_Full			= nullptr;
CTexture* CDOF::ms_pPresortHistoryTarget_Full	= nullptr;
CTexture* CDOF::ms_pPrefilteredColor_Full		= nullptr;
CTexture* CDOF::ms_pFinalTarget_Full			= nullptr;


void CDOF::Init()
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pCoCTiles					= new CTexture((nWidth + 31) / 32, (nHeight + 31) / 32, ETextureFormat::e_R16G16_FLOAT, eTextureStorage2D);
	ms_pBlurredCoCTiles				= new CTexture(ms_pCoCTiles->GetWidth() * 2, ms_pCoCTiles->GetHeight() * 2, ETextureFormat::e_R16G16_FLOAT, eTextureStorage2D);
	ms_pPresortTarget				= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R16G16_FLOAT, eTextureStorage2D);
	ms_pPrefilteredColor			= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pFilterTarget				= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	ms_pPresortTarget_Full			= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pPresortHistoryTarget_Full	= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pPrefilteredColor_Full		= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pFinalTarget_Full			= new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	if (CRenderPass::BeginGraphics("DOF"))
	{
		// Compute Tiles
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(1, ms_pCoCTiles->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("DOF_ComputeTiles");

			CRenderPass::SetEntryPoint(DOF_ComputeTiles_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Compute Tiles Neighbourhood
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pCoCTiles->GetID(),				CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(1, 1);
			CRenderPass::BindResourceToWrite(2, ms_pBlurredCoCTiles->GetID(),		CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("DOF_TilesNeighbourhood");

			CRenderPass::SetEntryPoint(DOF_ComputeTilesNeighbourhood_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Prefilter
		if (CRenderPass::BeginComputeSubPass())
		{			
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetDepthTarget(), CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(2, 1);

			CRenderPass::BindResourceToWrite(3, ms_pPrefilteredColor->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(4, ms_pPresortTarget->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("DOF_Prefilter");

			CRenderPass::SetEntryPoint(DOF_Prefilter_EntryPoint);

			CRenderPass::EndSubPass();
		}
		
		// Filter
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pBlurredCoCTiles->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pPrefilteredColor->GetID(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pPresortTarget->GetID(),			CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(3, ms_pFilterTarget->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("DOF_Filter");

			CRenderPass::SetEntryPoint(DOF_Filter_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Post Filter
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_pFilterTarget->GetID(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(1, ms_pPrefilteredColor->GetID(),		CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("DOF_PostFilter");

			CRenderPass::SetEntryPoint(DOF_PostFilter_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Merge
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pFilterTarget->GetID(),			CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetDepthTarget(), CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(2, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("DOF", "DOF_Merge");

			CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_SrcAlpha, EBlendFunc::e_BlendFunc_InvSrcAlpha, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

			CRenderPass::SetEntryPoint(DOF_Merge_EntryPoint);

			CRenderPass::EndSubPass();
		}
		
		CRenderPass::End();
	}
}


void DOF_ComputeTiles_EntryPoint()
{
	CTimerManager::GetGPUTimer("DOF")->Start();

	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	float FOV	= CRenderer::GetFOV4EngineFlush();

	float maxBlurRadius = CDOF::GetAperture() / (CDOF::GetPlaneInFocus() * 2.f * tanf(FOV * 3.141592f / 180.f));

	float4 constants;
	constants.x = maxBlurRadius;
	constants.y = CDOF::GetPlaneInFocus();
	constants.z = CRenderer::GetNear4EngineFlush();
	constants.w = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(float4));

	CDeviceManager::Dispatch((nWidth + 31) / 32, (nHeight + 31) / 32, 1);
}


void DOF_ComputeTilesNeighbourhood_EntryPoint()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	nWidth = (nWidth + 31) / 32;
	nHeight = (nHeight + 31) / 32;

	nWidth *= 2;
	nHeight *= 2;

	CResourceManager::SetSampler(1, e_MinMagMip_Point_UVW_Clamp);

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);
}



void DOF_Prefilter_EntryPoint()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	nWidth = (nWidth + 1) / 2;
	nHeight = (nHeight + 1) / 2;

	float FOV = CRenderer::GetFOV4EngineFlush();

	float maxBlurRadius = CDOF::GetAperture() / (CDOF::GetPlaneInFocus() * 2.f * tanf(FOV * 3.141592f / 180.f));

	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);

	float4 constants;
	constants.x = maxBlurRadius;
	constants.y = CDOF::GetPlaneInFocus();
	constants.z = CRenderer::GetNear4EngineFlush();
	constants.w = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(float4));

	CDeviceManager::Dispatch((nWidth + 7) / 8, (nHeight + 7) / 8, 1);
}


extern float VanDerCorput2(unsigned int);
extern float VanDerCorput3(unsigned int);



void DOF_Filter_EntryPoint()
{
	float sampleCoords[96];

	static int index = 1;

	float offset = VanDerCorput2(index);
	float angle = 2.f * M_PI * VanDerCorput3(index);

	for (int i = 0; i < 48; i++)
	{
		float x = i + offset;
		float r = sqrtf(x / 48.f);
		float theta = 3.88322f * x + angle;

		sampleCoords[2 * i] = r * cosf(theta);
		sampleCoords[2 * i + 1] = r * sinf(theta);
	}

	CResourceManager::SetConstantBuffer(4, sampleCoords, sizeof(sampleCoords));

	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	nWidth = (nWidth + 1) / 2;
	nHeight = (nHeight + 1) / 2;

	CDeviceManager::Dispatch((nWidth + 7) / 8, (nHeight + 7) / 8, 1);
}



void DOF_PostFilter_EntryPoint()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	nWidth = (nWidth + 1) / 2;
	nHeight = (nHeight + 1) / 2;

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);
}


void DOF_Merge_EntryPoint()
{
	float FOV = CRenderer::GetFOV4EngineFlush();

	float maxBlurRadius = CDOF::GetAperture() / (CDOF::GetPlaneInFocus() * 2.f * tanf(FOV * 3.141592f / 180.f));

	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);

	float4 constants;
	constants.x = maxBlurRadius;
	constants.y = CDOF::GetPlaneInFocus();
	constants.z = CRenderer::GetNear4EngineFlush();
	constants.w = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &constants, sizeof(float4));

	CRenderer::RenderQuadScreen();

	CTimerManager::GetGPUTimer("DOF")->Stop();
}
