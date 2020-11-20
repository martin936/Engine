#include "engine/Renderer/renderer.h"

/*void CRenderer::RenderHUD(SRenderTarget* source)
{

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CRenderer::ClearScreen();

	ProgramHandle pid = BindProgramByName(e_HUDPID);
	CTexture::SetTexture(pid, source->m_nTextureId, 0);
	//CTexture::SetTexture(pid, -1, 1);

	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetDepthStencil(e_Zero, false);

	CRenderer::RenderQuadScreen();
}*/