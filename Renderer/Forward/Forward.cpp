#include "Engine/Engine.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/OIT/OIT.h"


unsigned int g_ForwardCommandList = 0;


void CForwardRenderer::Init()
{
	g_ForwardCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	COIT::Init();
}



void CForwardRenderer::DrawForward()
{
	//CSchedulerThread::AddRenderTask(g_ForwardCommandList, CRenderPass::GetRenderPassTask("OIT"));
	//CCommandListManager::ScheduleDeferredKickoff(g_ForwardCommandList);
}
