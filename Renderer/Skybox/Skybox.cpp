#include "Engine/Engine.h"
#include "Engine/Renderer/Probes/LightProbe.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/StencilBits.h"
#include "Skybox.h"


CTexture* CSkybox::ms_pSkybox = NULL;

float CSkybox::ms_fSkyLightIntensity = 200.f;


void Skybox_EntryPoint()
{
	CTextureInterface::SetTexture(CSkybox::GetSkyboxTexture(), 0);
	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::SetViewProjConstantBuffer(2);

	CRenderer::RenderQuadScreen();
}


void CSkybox::Init()
{
	ms_pSkybox = new CTexture("../../Data/Environments/Field.dds");

	if (CRenderPass::BeginGraphics("Skybox"))
	{
		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetAlbedoTarget(),		CRenderPass::e_RenderTarget);
		CRenderPass::BindResourceToWrite(1, CDeferredRenderer::GetNormalTarget(),		CRenderPass::e_RenderTarget);
		CRenderPass::BindResourceToWrite(2, CDeferredRenderer::GetInfoTarget(),			CRenderPass::e_RenderTarget);
		CRenderPass::BindResourceToWrite(3, CDeferredRenderer::GetMotionVectorTarget(), CRenderPass::e_RenderTarget);

		CRenderPass::SetNumTextures(0, 1);
		CRenderPass::SetNumSamplers(1, 1);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("Skybox", "Skybox");

		CRenderPass::SetEntryPoint(Skybox_EntryPoint);

		CRenderPass::End();
	}
}

