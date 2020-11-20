#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/PostFX/ToneMapping/ToneMapping.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Device/DeviceManager.h"
#include "TAA.h"


CTexture*	CTAA::ms_pHistoryIn		= nullptr;
CTexture*	CTAA::ms_pHistoryOut	= nullptr;


void TAA_EntryPoint()
{
	CTimerManager::GetGPUTimer("TAA")->Start();

	CResourceManager::SetSampler(5, e_MinMagMip_Linear_UVW_Clamp);

	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);
}


void TAA_Copy_EntryPoint()
{
	CRenderer::RenderQuadScreen();

	CTimerManager::GetGPUTimer("TAA")->Stop();
}



void CTAA::Init()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pHistoryIn	= new CTexture(nWidth, nHeight, e_R16G16B16A16_FLOAT);
	ms_pHistoryOut 	= new CTexture(nWidth, nHeight, e_R16G16B16A16_FLOAT, eTextureStorage2D);

	if (CRenderPass::BeginCompute("TAA"))
	{
		// TAA Resolve
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMotionVectorTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pHistoryIn->GetID(),								CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetMergeTarget(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CToneMapping::GetAutoExposureTarget(),				CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(4, ms_pHistoryOut->GetID(),							CRenderPass::e_UnorderedAccess);

			CRenderPass::SetNumSamplers(5, 1);

			CRenderPass::BindProgram("TAA");

			CRenderPass::SetEntryPoint(TAA_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Copy
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pHistoryOut->GetID(), CShader::e_FragmentShader);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(),	CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(1, ms_pHistoryIn->GetID(),					CRenderPass::e_RenderTarget);

			CRenderPass::BindProgram("TAA_Copy", "TAA_Copy");

			CRenderPass::SetEntryPoint(TAA_Copy_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


