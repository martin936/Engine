#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/StencilBits.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/LightmapMaker/LightmapMaker.h"


unsigned int g_GBufferCommandList = 0;


void GBuffer_EntryPoint()
{
	CDeviceManager::ClearDepthStencil(0.f);

	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialBuffer(1);
	CMaterial::BindMaterialTextures(2);
	CResourceManager::SetSampler(3, e_Anisotropic_Linear_UVW_Wrap);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred);
}


void GBufferAlpha_EntryPoint()
{
	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialBuffer(1);
	CMaterial::BindMaterialTextures(2);
	CResourceManager::SetSampler(3, e_Anisotropic_Linear_UVW_Wrap);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);
}


void CDeferredRenderer::GBufferInit()
{
	g_GBufferCommandList		= CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	if (CRenderPass::BeginGraphics(ERenderPassId::e_GBuffer, "GBuffer"))
	{
		if (CRenderPass::BeginGraphicsSubPass("Opaque"))
		{
			CRenderPass::BindResourceToWrite(0, ms_pAlbedoTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(1, ms_pNormalTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(2, ms_pInfoTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(3, ms_pEmissiveTarget->GetID(),		CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(4, ms_pMotionVectorTarget->GetID(),	CRenderPass::e_RenderTarget);

			CRenderPass::SetNumTextures(2, 1024);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindDepthStencil(ms_pZBuffer->GetID());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("GBuffer", "GBuffer");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_CW);

			CRenderPass::SetEntryPoint(GBuffer_EntryPoint);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginGraphicsSubPass("Alpha"))
		{
			CRenderPass::BindResourceToWrite(0, ms_pAlbedoTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(1, ms_pNormalTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(2, ms_pInfoTarget->GetID(),			CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(3, ms_pEmissiveTarget->GetID(),		CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(4, ms_pMotionVectorTarget->GetID(),	CRenderPass::e_RenderTarget);

			CRenderPass::SetNumTextures(2, 1024);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindDepthStencil(ms_pZBuffer->GetID());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("GBuffer", "GBufferAlpha");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_CW);

			CRenderPass::SetEntryPoint(GBufferAlpha_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}



void CDeferredRenderer::RenderGBuffer()
{
	if (CSchedulerThread::BeginRenderTaskDeclaration())
	{
		CSchedulerThread::AddRenderPass(ERenderPassId::e_Skybox);
		CSchedulerThread::AddRenderPass(ERenderPassId::e_GBuffer);

		CSchedulerThread::EndRenderTaskDeclaration();
	}

	CSchedulerThread::ProcessRenderTask(g_GBufferCommandList);
}



void CDeferredRenderer::GBufferTerminate()
{

}



int CDeferredRenderer::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;
	
	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	CMaterial::BindMaterial(1, packet->m_pMaterial->GetID());

	float3x4 constants[2];
	constants[0] = pShaderData->m_ModelMatrix;
	constants[1] = pShaderData->m_LastModelMatrix;

	CResourceManager::SetPushConstant(CShader::e_VertexShader, &constants, sizeof(constants));

	return 1;
}