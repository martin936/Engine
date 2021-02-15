#include "Engine/Engine.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/PostFX/ToneMapping/ToneMapping.h"
#include "Engine/Device/DeviceManager.h"
#include "Bloom.h"


CTexture*	CBloom::ms_pDownscaleTargets	=	NULL;
float		CBloom::ms_fIntensity			= 1.f;


void CBloom::Init()
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pDownscaleTargets = new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D, 1, true);

	if (CRenderPass::BeginGraphics("Bloom"))
	{
		// Extract Downscale
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CToneMapping::GetAutoExposureTarget(),	CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(2, 1);

			CRenderPass::BindResourceToWrite(3, ms_pDownscaleTargets->GetID(), -1, 0,	CRenderPass::e_UnorderedAccess);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("Bloom_ExtractDownscale");

			CRenderPass::SetEntryPoint(CBloom::DownscaleExtract);

			CRenderPass::EndSubPass();
		}

		// Downscale
		for (unsigned int i = 1; i < ms_pDownscaleTargets->GetMipMapCount() - 3; i++)
		{
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pDownscaleTargets->GetID(), -1, i - 1,	CShader::e_ComputeShader);				
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(2, ms_pDownscaleTargets->GetID(), -1, i, CRenderPass::e_UnorderedAccess);

				CRenderPass::BindProgram("Bloom_Downscale");

				CRenderPass::SetEntryPoint(CBloom::Downscale, &i, sizeof(unsigned int));

				CRenderPass::EndSubPass();
			}
		}

		// Upscale
		for (int i = ms_pDownscaleTargets->GetMipMapCount() - 5; i >= 0; i--)
		{
			if (CRenderPass::BeginGraphicsSubPass())
			{
				CRenderPass::BindResourceToRead(0, ms_pDownscaleTargets->GetID(), -1, i + 1,	CShader::e_FragmentShader);				
				CRenderPass::SetNumSamplers(1, 1);

				CRenderPass::BindResourceToWrite(0, ms_pDownscaleTargets->GetID(), -1, i,		CRenderPass::e_RenderTarget);

				CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

				CRenderPass::BindProgram("Bloom", "Bloom_Upscale");

				CRenderPass::SetEntryPoint(CBloom::Upscale);

				CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

				CRenderPass::EndSubPass();
			}
		}

		// Merge
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pDownscaleTargets->GetID(), -1, 0, CShader::e_FragmentShader);
			CRenderPass::SetNumSamplers(1, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("Bloom", "Bloom_Upscale");

			CRenderPass::SetEntryPoint(CBloom::Upscale);

			CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_One, EBlendFunc::e_BlendFunc_One, EBlendOp::e_BlendOp_Add);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CBloom::DownscaleExtract()
{
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &ms_fIntensity, sizeof(float));

	CDeviceManager::Dispatch((ms_pDownscaleTargets->GetWidth() + 7) / 8, (ms_pDownscaleTargets->GetHeight() + 7) / 8, 1);
}


void CBloom::Downscale(void* pData)
{
	unsigned int nWidth		= ms_pDownscaleTargets->GetWidth();
	unsigned int nHeight	= ms_pDownscaleTargets->GetHeight();

	unsigned int n = *reinterpret_cast<unsigned int*>(pData);

	for (unsigned int i = 0; i < n; i++)
	{
		nWidth /= 2;
		nHeight /= 2;
	}

	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);

	CDeviceManager::Dispatch((nWidth + 7) / 8, (nHeight + 7) / 8, 1);
}


void CBloom::Upscale()
{
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}
