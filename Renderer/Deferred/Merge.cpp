#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/StencilBits.h"



void Merge_EntryPoint()
{
	CRenderer::RenderQuadScreen();
}



void CDeferredRenderer::MergeInit()
{
	if (CRenderPass::BeginGraphics("Merge"))
	{
		CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetAlbedoTarget(), CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, CDeferredRenderer::GetDiffuseTarget(), CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetSpecularTarget(), CShader::e_FragmentShader);

		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_RenderTarget);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("Merge", "Merge");

		CRenderPass::SetEntryPoint(Merge_EntryPoint);

		CRenderPass::End();
	}
}


void CDeferredRenderer::MergeTerminate()
{
}


void CDeferredRenderer::Merge()
{
	
}

