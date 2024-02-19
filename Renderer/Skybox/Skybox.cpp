#include "Engine/Engine.h"
#include "Engine/Renderer/Probes/LightProbe.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/StencilBits.h"
#include "Engine/Renderer/GameRenderPass.h"
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
	ms_pSkybox = new CTexture("../../Data/Environments/BlueSky.dds");

	if (CRenderPass::BeginGraphics(ERenderPassId::e_Skybox, "Skybox"))
	{
		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetAlbedoTarget(),		CRenderPass::e_RenderTarget);

		CRenderPass::SetNumTextures(0, 1);
		CRenderPass::SetNumSamplers(1, 1);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("Skybox", "Skybox");

		CRenderPass::SetEntryPoint(Skybox_EntryPoint);

		CRenderPass::End();
	}
}

