#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/TAA/TAA.h"
#include "AO.h"


struct SHBAOSplitConstants
{
	float4x4	m_InvView;
	float4		m_NearFar;
};


struct SHBAOConstants
{
	float4x4	m_InvView;
	float4x4	m_InvViewProj;
	float		m_Jitters[32];

	float		m_wsSphereRadius;
	float		m_ssSphereRadius;
	float		m_temporalFactor;
	float		m_temporalOffset;

	float		m_FullRes[4];

	float		m_FovX;
	float		m_FovY;
	float		m_Near;
	float		m_Far;
};


struct SHBAODenoiseConstants
{
	float4x4	m_InvViewProj;
	float4x4	m_LastInvViewProj;

	float4		m_Params;
};



void SplitInputs_EntryPoint()
{
	CTimerManager::GetGPUTimer("HBAO")->Start();
	CAO::SplitInputs();
}


void HBAO_EntryPoint()
{
	CAO::ComputeHBAO();
}


void DenoiseUpscale_EntryPoint()
{
	CAO::DenoiseAndUpscale();
}


void HBAOCopy_EntryPoint()
{
	CAO::HBAOCopy();
	CTimerManager::GetGPUTimer("HBAO")->Stop();
}



void CAO::InitHBAO()
{
	if (CRenderPass::BeginCompute("HBAO"))
	{
		// Split Inputs
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(2, ms_pInterleavedDepth->GetID(),			CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(3, ms_pInterleavedNormals->GetID(),		CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("HBAO_SplitInputs");

			CRenderPass::SetEntryPoint(SplitInputs_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Compute HBAO
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pInterleavedDepth->GetID(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pInterleavedNormals->GetID(),			CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(2, ms_pInterleavedHBAO->GetID(),			CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("HBAO");

			CRenderPass::SetEntryPoint(HBAO_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Denoise & Upscale
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pInterleavedHBAO->GetID(),				CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_pHBAOHistory->GetID(),					CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetMotionVectorTarget(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetLastDepthTarget(),		CShader::e_ComputeShader);
			CRenderPass::SetNumSamplers(5, 1);

			CRenderPass::BindResourceToWrite(6, ms_pHBAOFinalTarget->GetID(),				CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(7, ms_pHBAOBlendFactor->GetID(),				CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("HBAO_DenoiseUpscale");

			CRenderPass::SetEntryPoint(DenoiseUpscale_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Copy Last Frame HBAO
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pHBAOFinalTarget->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(1, ms_pHBAOHistory->GetID(),		CRenderPass::e_UnorderedAccess);
			
			CRenderPass::BindProgram("HBAO_Copy");

			CRenderPass::SetEntryPoint(HBAOCopy_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CAO::SplitInputs()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);
}



float radicalInverse_VdC(unsigned int inBits)
{
	unsigned int bits = inBits;
	bits = (bits << 16U) | (bits >> 16U);
	bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
	bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
	bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
	bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
	return (float)bits * 2.3283064365386963e-10f;
}



void CAO::ComputeHBAO()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	float AspectRatio	= 1.f * nWidth / nHeight;
	float fov			= CRenderer::GetFOV4EngineFlush();

	SHBAOConstants constants;

	constants.m_InvView		= CRenderer::GetInvViewMatrix4EngineFlush();
	constants.m_InvViewProj	= CRenderer::GetInvViewProjMatrix4EngineFlush();
	constants.m_Near		= CRenderer::GetNear4EngineFlush();
	constants.m_Far			= CRenderer::GetFar4EngineFlush();
	constants.m_FovX		= tanf(fov * 3.1415926f / 360.f);
	constants.m_FovY		= constants.m_FovX / AspectRatio;

	static int nOffset = 0;

	for (int i = 0; i < 16; i++)
	{
		float angle = (3.1415926f / 2.f) * radicalInverse_VdC((unsigned int)nOffset);

		constants.m_Jitters[i * 2 + 0] = cos(angle);
		constants.m_Jitters[i * 2 + 1] = sin(angle);

		nOffset++;
	}

	constants.m_FullRes[0]		= static_cast<float>(nWidth);
	constants.m_FullRes[1]		= static_cast<float>(nHeight);
	constants.m_wsSphereRadius	= ms_fKernelSize;
	constants.m_ssSphereRadius	= 200.f;
	constants.m_temporalFactor	= m_bFirstFrame ? 1.f : 0.1f;
	constants.m_temporalOffset	= 1.f * nOffset;

	CResourceManager::SetConstantBuffer(3, &constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pInterleavedDepth->GetWidth() + 7) / 8, (ms_pInterleavedDepth->GetHeight() + 7) / 8, 16);

	nOffset++;
}



void CAO::DenoiseAndUpscale()
{
	CResourceManager::SetSampler(5, e_MinMagMip_Linear_UVW_Clamp);

	SHBAODenoiseConstants constants;
	constants.m_InvViewProj = CRenderer::GetInvViewProjMatrix4EngineFlush();
	constants.m_LastInvViewProj = CRenderer::GetLastInvViewProjMatrix4EngineFlush();

	constants.m_Params.x = m_bFirstFrame ? 1.f : 0.f;
	constants.m_Params.y = CRenderer::GetNear4EngineFlush();
	constants.m_Params.z = CRenderer::GetFar4EngineFlush();
	constants.m_Params.w = ms_fStrength;

	CResourceManager::SetConstantBuffer(8, &constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pHBAOFinalTarget->GetWidth() + 7) / 8, (ms_pHBAOFinalTarget->GetHeight() + 7) / 8, 1);

	m_bFirstFrame = false;
}



void CAO::HBAOCopy()
{
	CDeviceManager::Dispatch((ms_pHBAOFinalTarget->GetWidth() + 7) / 8, (ms_pHBAOFinalTarget->GetHeight() + 7) / 8, 1);
}

