#include "Engine/Renderer/Renderer.h"
#include "Shadowmaps.h"


void CShadowMap::Set(ProgramHandle nPID, unsigned int nSlot)
{
	glActiveTexture(GL_TEXTURE0 + nSlot);
	glBindTexture(GL_TEXTURE_2D_ARRAY, ms_pDepthStencilArray->m_nTextureId);

	GLuint uniform = glGetUniformLocation(nPID, "ShadowMap");
	glUniform1i(uniform, nSlot);

	uniform = glGetUniformLocation(nPID, "ShadowMapIndex");
	glUniform1i(uniform, m_nIndex);
}
