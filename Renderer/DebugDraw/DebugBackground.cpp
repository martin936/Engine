#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Textures/Textures.h"
#include "Engine/Renderer/Textures/TextureInterface.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "DebugBackground.h"


unsigned int CDebugBackground::ms_nTextureID = INVALIDHANDLE;


static void DebugBackground_EntryPoint()
{
	CTextureInterface::SetTexture(CDebugBackground::GetTextureID(), 0, CShader::e_FragmentShader);
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}


void CDebugBackground::Init()
{	
	if (CRenderPass::BeginGraphics(ERenderPassId::e_Debug_Background, "Debug Background"))
	{
		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetToneMappedTarget(), CRenderPass::e_RenderTarget);

		CRenderPass::SetNumTextures(0, 1);
		CRenderPass::SetNumSamplers(1, 1);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("DebugBackground", "DebugBackground");

		CRenderPass::SetEntryPoint(DebugBackground_EntryPoint);

		CRenderPass::End();
	}
}


void CDebugBackground::SetTexture(CTexture& pTexture)
{
	ms_nTextureID = pTexture.GetID();
}


unsigned int CDebugBackground::GetTextureID()
{
	return ms_nTextureID;
}
