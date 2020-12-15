#include "Engine/Engine.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/AO/AO.h"
#include "Engine/Editor/Adjustables/Adjustables.h"


ADJUSTABLE("Enable SSS", bool, gs_bEnableSSS, true, false, true, "Graphics/PostFX/SSS")

extern bool gs_bShowIrradianceProbes_Saved;
extern bool gs_EnableAO_Saved;

bool			CDeferredRenderer::ms_bRequestRaycast = false;
bool			CDeferredRenderer::ms_bRequestRaycast4EngineFlush = false;
bool			CDeferredRenderer::ms_bRequestReady = true;
	  
float			CDeferredRenderer::ms_RaycastCoordX = 0.f;
float			CDeferredRenderer::ms_RaycastCoordY = 0.f;
unsigned int	CDeferredRenderer::ms_RequestedMaterialID = 0;

CTexture* CDeferredRenderer::ms_pAlbedoTarget		= NULL;
CTexture* CDeferredRenderer::ms_pNormalTarget		= NULL;
CTexture* CDeferredRenderer::ms_pInfoTarget			= NULL;
CTexture* CDeferredRenderer::ms_pMotionVectorTarget = NULL;
CTexture* CDeferredRenderer::ms_pZBuffer			= NULL;
CTexture* CDeferredRenderer::ms_pLastZBuffer		= NULL;

CTexture* CDeferredRenderer::ms_pToneMappedTarget	= NULL;
CTexture* CDeferredRenderer::ms_pMaterialTarget		= NULL;

CTexture* CDeferredRenderer::ms_pDiffuseLighting	= NULL;
CTexture* CDeferredRenderer::ms_pSpecularLighting	= NULL;

CTexture* CDeferredRenderer::ms_pMergeTarget		= NULL;

BufferId CDeferredRenderer::ms_ReadBackMaterialBuffer = INVALIDHANDLE;

unsigned int g_LightingMergeCommandList = 0;


void CDeferredRenderer::Init()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pAlbedoTarget		= new CTexture(nWidth, nHeight, e_R8G8B8A8);
	ms_pNormalTarget		= new CTexture(nWidth, nHeight, e_R10G10B10A2);
	ms_pInfoTarget			= new CTexture(nWidth, nHeight, e_R8G8B8A8);
	ms_pMotionVectorTarget	= new CTexture(nWidth, nHeight, e_R16G16_SNORM);

	ms_pZBuffer				= new CTexture(nWidth, nHeight, e_R32_DEPTH_G8_STENCIL);
	ms_pLastZBuffer			= new CTexture(nWidth, nHeight, e_R32_DEPTH_G8_STENCIL);

	ms_pDiffuseLighting		= new CTexture(nWidth, nHeight, e_R16G16B16A16_FLOAT);
	ms_pSpecularLighting	= new CTexture(nWidth, nHeight, e_R16G16B16A16_FLOAT);

	ms_pMergeTarget			= new CTexture(nWidth, nHeight, e_R16G16B16A16_FLOAT, eTextureStorage2D);

	ms_pToneMappedTarget	= new CTexture(nWidth, nHeight, ETextureFormat::e_R8G8B8A8);

	ms_pMaterialTarget		= new CTexture(nWidth, nHeight, ETextureFormat::e_R16_UINT);

	ms_ReadBackMaterialBuffer = CResourceManager::CreateRwBuffer(sizeof(unsigned int), true);

	g_LightingMergeCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	GBufferInit();
	MergeInit();

	CSkybox::Init();

	CAO::Init(CAO::e_HBAO);

	if (CRenderPass::BeginGraphics("Ray Cast Material"))
	{
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_pMaterialTarget->GetID(), CRenderPass::e_RenderTarget);
			CRenderPass::BindDepthStencil(ms_pZBuffer->GetID());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("GBuffer", "DrawMaterialID");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);

			CRenderPass::SetEntryPoint(DrawMaterialID);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pMaterialTarget->GetID(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToWrite(1, ms_ReadBackMaterialBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("RayCastMatID");

			CRenderPass::SetEntryPoint(RayCastMaterialID);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}



void CDeferredRenderer::Terminate()
{
	
}



void CDeferredRenderer::RayCastMaterialID()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	unsigned int constants[2];
	constants[0] = static_cast<unsigned int>(ms_RaycastCoordX * nWidth);
	constants[1] = static_cast<unsigned int>(ms_RaycastCoordY * nHeight);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch(1, 1, 1);
}



void CDeferredRenderer::DrawMaterialID()
{
	CPacketManager::ForceShaderHook(MaterialIDUpdateShader);

	CRenderer::SetViewProjConstantBuffer(0);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred | CMaterial::e_Forward);

	CPacketManager::ForceShaderHook(0);
}



int CDeferredRenderer::MaterialIDUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	unsigned int nMatID = packet->m_pMaterial->GetID();

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, &nMatID, sizeof(nMatID));

	return 1;
}



void CDeferredRenderer::UpdateBeforeFlush()
{
	if (!ms_bRequestReady)
	{
		if (CResourceManager::WaitForFence(CRenderer::ms_FenceFrameFinished, 0))
		{
			ms_RequestedMaterialID = *static_cast<unsigned int*>(CResourceManager::MapBuffer(ms_ReadBackMaterialBuffer));
			CResourceManager::UnmapBuffer(ms_ReadBackMaterialBuffer);

			ms_bRequestReady = true;
		}
	}

	if (ms_bRequestRaycast4EngineFlush)
		ms_bRequestRaycast = false;

	ms_bRequestRaycast4EngineFlush = ms_bRequestRaycast;

	if (ms_bRequestRaycast4EngineFlush)
		ms_bRequestReady = false;
}



void CDeferredRenderer::DrawDeferred()
{
	RenderGBuffer();

	std::vector<SRenderPassTask> renderPasses;
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Compute Shadows"));

	if (gs_EnableAO_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("HBAO"));

	renderPasses.push_back(CRenderPass::GetRenderPassTask("Lighting"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Merge"));
	//renderPasses.push_back(CRenderPass::GetRenderPassTask("Ray Trace Light Field"));

	if (gs_bShowIrradianceProbes_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("Show Light Field"));
	
	CSchedulerThread::AddRenderTask(g_LightingMergeCommandList, renderPasses);
	CCommandListManager::ScheduleDeferredKickoff(g_LightingMergeCommandList);
}

