#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/TAA/TAA.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "AO.h"


CTexture* CAO::ms_pSDFAOTarget = nullptr;


void CAO::InitSDFAO()
{
	if (CRenderPass::BeginCompute("SDFAO"))
	{
		CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),		CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(2, CLightField::GetLightFieldSH(0),				CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(3, CLightField::GetProbeMetadata(0),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(4, CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(5, CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(6, CLightField::GetLightFieldSH(1),				CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(7, CLightField::GetProbeMetadata(1),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(8, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(9, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(10, CLightField::GetLightFieldSH(2),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(11, CLightField::GetProbeMetadata(2),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(12, CLightField::GetLightFieldOcclusion(2, 0),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(13, CLightField::GetLightFieldOcclusion(2, 1),	CShader::e_ComputeShader);
		CRenderPass::SetNumTextures(14, 1024);
		CRenderPass::SetNumSamplers(15, 1);

		CRenderPass::BindResourceToWrite(16, ms_pSDFAOTarget->GetID(), CRenderPass::e_UnorderedAccess);

		CRenderPass::BindProgram("SDFAO");

		CRenderPass::SetEntryPoint(ComputeSDFAO);

		CRenderPass::End();
	}
}


extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);
extern void Encode(float3& v);


void CAO::ComputeSDFAO()
{
	CSDF::BindSDFs(14);
	CResourceManager::SetSampler(15, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(17);
	CRenderer::SetViewProjConstantBuffer(18);

	float3 sampleCoords[8];
	static int index = 1;

	float angle = VanDerCorput3(index);
	float offset = VanDerCorput2(index);

	for (int i = 0; i < 8; i++)
	{
		float x = (i + offset) / 8.f;
		float y = 2.f * 3.1415926f * (1.618034f * i + angle);

		float cos_th = sqrtf(MAX(0.f, 1.f - 0.49f * x * x));
		float sin_th = 0.7f * x;

		sampleCoords[i] = float3(cos_th * cosf(y), cos_th * sinf(y), sin_th);
	}

	index++;

	CResourceManager::SetConstantBuffer(19, sampleCoords, sizeof(sampleCoords));

	float4 constants[8];
	constants[0] = CLightField::GetCenter(0);
	constants[1] = CLightField::GetSize(0);
	constants[2] = CLightField::GetCenter(1);
	constants[3] = CLightField::GetSize(1);
	constants[4] = CLightField::GetCenter(2);
	constants[5] = CLightField::GetSize(2);
	constants[6] = CLightField::GetRealCenter();
	constants[7] = float4(ms_fKernelSize, ms_fBias, 0.f, 0.f);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pSDFAOTarget->GetWidth() + 7) / 8, (ms_pSDFAOTarget->GetHeight() + 7) / 8, 1);
}
