#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/TAA/TAA.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SSR/SSR.h"
#include "AO.h"


CTexture* CAO::ms_pRadiance;
CTexture* CAO::ms_pLastFrameRadiance;
CTexture* CAO::ms_pSSRTGI;
CTexture* CAO::ms_pSSRTGIHistoryIn;
CTexture* CAO::ms_pSSRTGIHistoryOut;


void CAO::InitSSRTGI()
{
	int width = CDeviceManager::GetDeviceWidth();
	int height = CDeviceManager::GetDeviceHeight();

	ms_pLastFrameRadiance	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pRadiance			= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D, 1, true);
	ms_pSSRTGI				= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pSSRTGIHistoryIn		= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);
	ms_pSSRTGIHistoryOut	= new CTexture(width, height, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);


	if (CRenderPass::BeginGraphics("Save Frame Radiance"))
	{
		CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(),				CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetMotionVectorTarget(),		CShader::e_FragmentShader);

		CRenderPass::BindResourceToWrite(0, ms_pLastFrameRadiance->GetID(),					CRenderPass::e_RenderTarget);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("copy", "SaveFrameRadiance");

		CRenderPass::SetEntryPoint(SaveFrameRadiance);

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute("SSRTGI"))
	{
		// Reproject Radiance
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pLastFrameRadiance->GetID(),				CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetMotionVectorTarget(),	CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(2, 1);

			CRenderPass::BindResourceToWrite(0, ms_pRadiance->GetID(), -1, 0,				CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("copy", "ReprojectRadiance");

			CRenderPass::SetEntryPoint(ReprojectRadiance);

			CRenderPass::EndSubPass();
		}

		// Push Radiance Mips
		int numMips = ms_pRadiance->GetMipMapCount();
		for (int i = 1; i < numMips; i++)
		{
			if (CRenderPass::BeginGraphicsSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pRadiance->GetID(), -1, i - 1, CShader::e_FragmentShader);
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(0, ms_pRadiance->GetID(), -1, i, CRenderPass::e_RenderTarget);

				CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

				CRenderPass::BindProgram("Bloom", "PushRadianceMips");

				CRenderPass::SetEntryPoint(PushRadianceMip);

				CRenderPass::EndSubPass();
			}
		}

		// Pull Radiance Mips
		for (int i = numMips - 2; i >= 0; i--)
		{
			if (CRenderPass::BeginGraphicsSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pRadiance->GetID(), -1, i + 1, CShader::e_FragmentShader);
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(0, ms_pRadiance->GetID(), -1, i, CRenderPass::e_RenderTarget);

				CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

				CRenderPass::BindProgram("Bloom", "PullRadianceMips");

				CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_InvDstAlpha, EBlendFunc::e_BlendFunc_DstAlpha, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_Zero, EBlendFunc::e_BlendFunc_Zero, EBlendOp::e_BlendOp_Add, 7);

				CRenderPass::SetEntryPoint(PullRadianceMip);

				CRenderPass::EndSubPass();
			}
		}

		// SSRTGI
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CSSR::GetZMips(),							CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pRadiance->GetID(),						CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetNormalTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CLightField::GetLightFieldSH(0),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(5, CLightField::GetProbeMetadata(0),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(6, CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(7, CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(8, CLightField::GetLightFieldSH(1),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(9, CLightField::GetProbeMetadata(1),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(10, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(11, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(12, CLightField::GetLightFieldSH(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(13, CLightField::GetProbeMetadata(2),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(14, CLightField::GetLightFieldOcclusion(2, 0),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(15, CLightField::GetLightFieldOcclusion(2, 1),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(16, CRenderer::ms_pSobolSequence32->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(17, CRenderer::ms_pOwenScrambling32->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(18, CRenderer::ms_pOwenRanking32->GetID(),		CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(19, 1);

			CRenderPass::BindResourceToWrite(20, ms_pSSRTGI->GetID(),						CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SSRTGI");

			CRenderPass::SetEntryPoint(SSRTGI);

			CRenderPass::EndSubPass();
		}

		// TAA
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMotionVectorTarget(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pSSRTGIHistoryIn->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, ms_pSSRTGI->GetID(),							CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindResourceToWrite(4, ms_pSSRTGIHistoryOut->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("SSRTGI_TAA");

			CRenderPass::SetEntryPoint(TAA);

			CRenderPass::EndSubPass();
		}

		// TAA Copy
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pSSRTGIHistoryOut->GetID(),	CShader::e_FragmentShader);

			CRenderPass::BindResourceToWrite(0, ms_pSSRTGIHistoryIn->GetID(),	CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("copy", "copy");

			CRenderPass::SetEntryPoint(TAA_Copy);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}



void CAO::SaveFrameRadiance()
{
	CRenderer::RenderQuadScreen();
}


void CAO::ReprojectRadiance()
{
	CResourceManager::SetSampler(2, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}


void CAO::PushRadianceMip()
{
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}


void CAO::PullRadianceMip()
{
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}


extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);


void CAO::SSRTGI()
{
	CResourceManager::SetSampler(19, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);
	CRenderer::SetViewProjConstantBuffer(21);

	float3 sampleCoords[4];
	static int index = 1;

	float angle = VanDerCorput3(index);
	float offset = VanDerCorput2(index);

	for (int i = 0; i < 4; i++)
	{
		float x = (i + offset) / 4.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = sqrtf(MAX(0.f, 1.f - 0.49f * x * x));
		float sin_th = 0.7f * x;

		sampleCoords[i] = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);
	}

	CResourceManager::SetConstantBuffer(22, sampleCoords, sizeof(sampleCoords));

	float4 constants[8];
	constants[0] = CLightField::GetCenter(0);
	constants[1] = CLightField::GetSize(0);
	constants[2] = CLightField::GetCenter(1);
	constants[3] = CLightField::GetSize(1);
	constants[4] = CLightField::GetCenter(2);
	constants[5] = CLightField::GetSize(2);
	constants[6] = CLightField::GetRealCenter();
	constants[7] = float4(CRenderer::GetNear4EngineFlush(), CRenderer::GetFar4EngineFlush(), ms_fKernelSize, 1.f * (index & 31u));

	index++;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pSSRTGI->GetWidth() + 7) / 8, (ms_pSSRTGI->GetHeight() + 7) / 8, 1);
}


void CAO::TAA()
{
	CResourceManager::SetSampler(3, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CDeviceManager::Dispatch((ms_pSSRTGI->GetWidth() + 15) / 16, (ms_pSSRTGI->GetHeight() + 15) / 16, 1);
}


void CAO::TAA_Copy()
{
	CRenderer::RenderQuadScreen();
}
