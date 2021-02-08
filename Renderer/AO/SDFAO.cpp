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
	/*if (CRenderPass::BeginCompute("SDFAO"))
	{
		CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(),		CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetNormalTarget(),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(2, CLightField::GetProbeMetadata(),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(3, CLightField::GetLightFieldSH(),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(4, CLightField::GetLightFieldOcclusion(0),	CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(5, CLightField::GetLightFieldOcclusion(1),	CShader::e_ComputeShader);
		CRenderPass::SetNumTextures(6, 1024);
		CRenderPass::SetNumSamplers(7, 1);

		CRenderPass::BindResourceToWrite(8, ms_pSDFAOTarget->GetID(), CRenderPass::e_UnorderedAccess);

		CRenderPass::BindProgram("SDFAO");

		CRenderPass::SetEntryPoint(ComputeSDFAO);

		CRenderPass::End();
	}*/
}


extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);
extern void Encode(float3& v);


void CAO::ComputeSDFAO()
{
	/*CSDF::BindSDFs(6);
	CResourceManager::SetSampler(7, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(9);
	CRenderer::SetViewProjConstantBuffer(10);

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

	CResourceManager::SetConstantBuffer(11, sampleCoords, sizeof(sampleCoords));

	float4 constants[2];
	constants[0] = CLightField::GetCenter();
	constants[1] = CLightField::GetSize();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_pSDFAOTarget->GetWidth() + 7) / 8, (ms_pSDFAOTarget->GetHeight() + 7) / 8, 1);*/
}
