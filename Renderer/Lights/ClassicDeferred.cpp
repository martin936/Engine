#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/AO/AO.h"
#include "LightsManager.h"
#include "Engine/Editor/Adjustables/Adjustables.h"


//SRenderTarget*	CLightsManager::ms_pDiffuseTarget = NULL;
//SRenderTarget*	CLightsManager::ms_pSpecularTarget = NULL;


EXPORT_ADJUSTABLE(bool, gs_bEnableSSAO)



void CLightsManager::InitClassicDeferred()
{
}



void CLightsManager::TerminateClassicDeferred()
{
}



void CLightsManager::ClassicLighting()
{
	/*CFramebuffer::SetDrawBuffers(2);
	CFramebuffer::BindRenderTarget(0, ms_pDiffuseTarget);
	CFramebuffer::BindRenderTarget(1, ms_pSpecularTarget);
	CFramebuffer::BindDepthStencil(NULL);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderer::ClearScreen();

	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetBlendingState(e_Additive);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);

	std::vector<CLight*>::iterator it;
	CLight* pLight = NULL;

	for (it = ms_pLights.begin(); it < ms_pLights.end(); it++)
	{
		pLight = (*it);
		pLight->Draw();
	}


	if (gs_bEnableSSAO)
	{
		CTimerManager::GetGPUTimer("SSAO")->Start();
		CAO::Apply();
		CTimerManager::GetGPUTimer("SSAO")->Stop();
	}

	CTimerManager::GetGPUTimer("Global Ilumination")->Start();
	CLightsManager::DrawGlobalIllumination();
	CTimerManager::GetGPUTimer("Global Ilumination")->Stop();*/
}
