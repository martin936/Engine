#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/Renderer/Text/Text.h"
#include "TexturedSpriteRenderer.h"


static void TexturedSprite_EntryPoint()
{
	CResourceManager::SetSampler(1, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::DrawPackets(e_RenderType_TexturedSprites);
}


void CTexturedSpriteRenderer::Init()
{
	CTexture* pFontTex = CTextRenderer::GetFontTexture();

	if (pFontTex == NULL)
		return;

	if (CRenderPass::BeginGraphics(ERenderPassId::e_TexturedSprite2D, "Textured Sprite 2D"))
	{
		CRenderPass::BindResourceToRead(0, pFontTex->GetID(), CShader::e_FragmentShader);
		CRenderPass::SetNumSamplers(1, 1);

		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetToneMappedTarget(), CRenderPass::e_RenderTarget);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("TexturedQuad2D", "TexturedQuad2D");

		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None);
		CRenderPass::DisableDepthStencil();
		CRenderPass::SetBlendState(EBlendState::e_BlendState_AlphaBlend);

		CRenderPass::SetPrimitiveTopology(ETopology::e_Topology_TriangleList);

		CRenderPass::SetEntryPoint(TexturedSprite_EntryPoint);

		CRenderPass::End();
	}
}


int CTexturedSpriteRenderer::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	return 1;
}
