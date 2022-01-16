#include "Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/AO/AO.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Engine/Renderer/DebugDraw/DebugDraw.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SSS/SSS.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Engine/Renderer/PostFX/DOF/DOF.h"
#include "Engine/Renderer/PostFX/Bloom/Bloom.h"
#include "Engine/Renderer/PostFX/ToneMapping/ToneMapping.h"
#include "Engine/Renderer/VolumetricMedia/VolumetricMedia.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "Engine/Renderer/Text/Text.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "Engine/Renderer/TAA/TAA.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Renderer/SDFGI/SDFGI.h"
#include "Engine/Renderer/SSR/SSR.h"
#include "Engine/Renderer/PostFX/DOF/DOF.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Project/Engine/resource.h"
#include "Engine/Physics/Physics.h"


std::vector<CCamera*>	CRenderer::ms_pCameras;
BufferId				CRenderer::ms_ViewProjBuffer = INVALIDHANDLE;

FenceId					CRenderer::ms_FenceFrameFinished = INVALIDHANDLE;

thread_local float4x4		CRenderer::ms_ViewMatrix = 0.f;
thread_local float4x4		CRenderer::ms_ProjMatrix = 0.f;
thread_local float4x4		CRenderer::ms_ViewProjMatrix = 0.f;
thread_local float4x4		CRenderer::ms_InvViewMatrix = 0.f;
thread_local float4x4		CRenderer::ms_InvViewProjMatrix = 0.f;
thread_local float4			CRenderer::ms_EyePosition = float4(0.f, 0.f, 0.f, 0.f);

float4x4					CRenderer::ms_GlobalViewMatrix = 0.f;
float4x4					CRenderer::ms_GlobalProjMatrix = 0.f;
float4x4					CRenderer::ms_GlobalViewProjMatrix = 0.f;
float4x4					CRenderer::ms_GlobalInvViewMatrix = 0.f;
float4x4					CRenderer::ms_GlobalInvViewProjMatrix = 0.f;
float4						CRenderer::ms_GlobalEyePosition = float4(0.f, 0.f, 0.f, 0.f);

float						CRenderer::ms_GlobalFOV		= 0.f;
float						CRenderer::ms_GlobalNear	= 0.f;
float						CRenderer::ms_GlobalFar		= 0.f;

float4x4					CRenderer::ms_LastGlobalViewMatrix = 0.f;
float4x4					CRenderer::ms_LastGlobalProjMatrix = 0.f;
float4x4					CRenderer::ms_LastGlobalViewProjMatrix = 0.f;
float4x4					CRenderer::ms_LastGlobalInvViewMatrix = 0.f;
float4x4					CRenderer::ms_LastGlobalInvViewProjMatrix = 0.f;

CTexture*					CRenderer::ms_pSobolSequence8 = NULL;
CTexture*					CRenderer::ms_pSobolSequence16 = NULL;
CTexture*					CRenderer::ms_pSobolSequence32 = NULL;

CTexture*					CRenderer::ms_pOwenScrambling8 = NULL;
CTexture*					CRenderer::ms_pOwenScrambling16 = NULL;
CTexture*					CRenderer::ms_pOwenScrambling32 = NULL;

CTexture*					CRenderer::ms_pOwenRanking8 = NULL;
CTexture*					CRenderer::ms_pOwenRanking16 = NULL;
CTexture*					CRenderer::ms_pOwenRanking32 = NULL;

BufferId	g_QuadVertexBuffer;

CCamera* CRenderer::ms_pCurrentCamera = NULL;

int		CRenderer::ms_nCurrentCameraIndex	= 0;
bool	CRenderer::ms_bEnableVolumetrics	= false;
bool	CRenderer::ms_bEnableTransparency	= false;
bool	CRenderer::ms_bEnableAA				= false;
bool	CRenderer::ms_bEnableBloom			= false;
bool	CRenderer::ms_bEnableDOF			= false;
bool	CRenderer::ms_bEnableSSR			= false;
bool	CRenderer::ms_bEnableTAA			= false;
bool	CRenderer::ms_bEnableAO				= false;


int		CRenderer::ms_nCurrentFrame = 0;

bool g_bIsFirstFrame = true;

thread_local EVertexLayout CRenderer::ms_eVertexLayout = e_Vertex_Layout_Engine;


struct SFrameConstantBuffer
{
	float4x4	m_View;
	float4x4	m_Proj;
	float4x4	m_ViewProj;
	float4x4	m_InvView;
	float4x4	m_InvViewProj;

	float4x4	m_LastView;
	float4x4	m_LastProj;
	float4x4	m_LastViewProj;
	float4x4	m_LastInvView;
	float4x4	m_LastInvViewProj;

	float4		m_Eye;
	float4		m_CameraOffset;
};


unsigned int g_2DCommandList = 0;
unsigned int g_CullLightsCommandList = 0;
unsigned int g_ShadowMapCommandList = 0;


bool gs_EnableDOF_Saved = false;
bool gs_EnableBloom_Saved = false;
bool gs_EnableAA_Saved = false;
bool gs_EnableTAA_Saved = false;
bool gs_EnableAO_Saved = false;
bool gs_EnableSSR_Saved = false;
bool gs_EnableSSS_Saved = false;
bool gs_EnableFog_Saved = false;
bool gs_EnableTransparency_Saved = false;
bool gs_bEnableVolumetricMedia_Saved = false;
bool gs_bGenerateLightField_Saved = false;
bool gs_bShowIrradianceProbes_Saved = false;
bool gs_bEnableDiffuseGI_Saved = false;
bool gs_bRequestRayCastMaterial_Saved = false;


void CRenderer::Init()
{
	InitBlueNoiseTextures();

	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pCurrentCamera = new CStaticCamera;
	ms_pCameras.push_back(ms_pCurrentCamera);

	int numProbesX[] = { 16, 16, 8 };
	int numProbesY[] = { 16, 16, 8 };
	int numProbesZ[] = { 8, 8, 4 };

	CLightField::Init(numProbesX, numProbesY, numProbesZ);

	CDeferredRenderer::Init();
	CShadowRenderer::Init();
	CLightsManager::Init();
	CToneMapping::Init();
	CTAA::Init();
	CDebugDraw::Init();
	CForwardRenderer::Init();
	CDOF::Init();
	CBloom::Init();
	CSDF::Init();
	CSSR::Init();
	CAO::Init(CAO::e_SSRTGI);
	CSDFGI::Init();

	InitRenderPasses();

	CPacketManager::Init();
	CTimerManager::Clear();
	CSpriteEngine::Init();
	CPacketBuilder::Init();
	CTextRenderer::InitFont(FONT_PATH("crystal"));

	InitRenderQuadScreen();

	ms_FenceFrameFinished = CResourceManager::CreateFence();

	g_bIsFirstFrame = true;
}



void CRenderer::Terminate()
{
	std::vector<CCamera*>::iterator it;

	for (it = ms_pCameras.begin(); it < ms_pCameras.end(); it++)
		delete (*it);

	CDeferredRenderer::Terminate();
}


void CRenderer::EnableDiffuseGI(bool bEnable)
{
	CLightField::Enable(bEnable);
}



void RenderQuadScreen_RenderPass()
{
	CRenderer::RenderQuadScreen();
}



void CRenderer::InitBlueNoiseTextures()
{
	ms_pOwenRanking8		= new CTexture(IDR_DDS1);
	ms_pOwenRanking16		= new CTexture(IDR_DDS2);
	ms_pOwenRanking32		= new CTexture(IDR_DDS3);
	ms_pOwenScrambling8		= new CTexture(IDR_DDS4);
	ms_pOwenScrambling16	= new CTexture(IDR_DDS5);
	ms_pOwenScrambling32	= new CTexture(IDR_DDS6);
	ms_pSobolSequence8		= new CTexture(IDR_DDS7);
	ms_pSobolSequence16		= new CTexture(IDR_DDS8);
	ms_pSobolSequence32		= new CTexture(IDR_DDS9);
}



void CRenderer::InitRenderQuadScreen()
{
	float vertex_data[5 * 7];
	vertex_data[0]	= -1.f;
	vertex_data[1]	= 1.f;
	vertex_data[2]	= 0.f;

	vertex_data[6]	= -1.f;
	vertex_data[7]	= -1.f;
	vertex_data[8]	= 0.f;

	vertex_data[12]	= 1.f;
	vertex_data[13]	= 1.f;
	vertex_data[14]	= 0.f;

	vertex_data[18]	= -1.f;
	vertex_data[19]	= -1.f;
	vertex_data[20]	= 0.f;

	vertex_data[24]	= 1.f;
	vertex_data[25]	= 1.f;
	vertex_data[26]	= 0.f;

	vertex_data[30] = 1.f;
	vertex_data[31] = -1.f;
	vertex_data[32]	= 0.f;

	g_QuadVertexBuffer = CResourceManager::CreateVertexBuffer(sizeof(vertex_data), vertex_data);
}


void CRenderer::RenderQuadScreen(int numInstances)
{
	std::vector<CDeviceManager::SStream> pStreams;
	pStreams.push_back({ 0, g_QuadVertexBuffer, 0 });

	CDeviceManager::SetStreams(pStreams);
	CDeviceManager::DrawInstanced(0, 6, 0, numInstances);
}



void CRenderer::InitFrame()
{
	CDeviceManager::InitFrame();
	CTexture::InitFrame();
}


void CRenderer::EndFrame()
{
	CPacketManager::EmptyDynamicLists();

	CCamera::UpdateJitterIndex();
}


void CRenderer::InitRenderPasses()
{
	if (!g_2DCommandList)
		g_2DCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	if (!g_CullLightsCommandList)
		g_CullLightsCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	if (!g_ShadowMapCommandList)
		g_ShadowMapCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);

	if (CRenderPass::BeginGraphics("Final Copy"))
	{
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetDepthTarget(), CShader::e_FragmentShader);

			CRenderPass::BindDepthStencil(CDeferredRenderer::GetLastDepthTarget());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("copy", "copyDepth");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_Always, true);

			CRenderPass::SetEntryPoint(RenderQuadScreen_RenderPass);

			CRenderPass::EndSubPass();
		}


		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetToneMappedTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToWrite(0, INVALIDHANDLE, CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("copy", "copy");

			CRenderPass::SetEntryPoint(RenderQuadScreen_RenderPass);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}

	CLightsManager::InitRenderPasses();
}


bool CRenderer::HasFrameStateChanged()
{
	bool bChanged = false;

	if (gs_EnableDOF_Saved != CRenderer::IsDOFEnabled())
	{
		gs_EnableDOF_Saved = CRenderer::IsDOFEnabled();
		bChanged = true;
	}

	if (gs_EnableBloom_Saved != CRenderer::IsBloomEnabled())
	{
		gs_EnableBloom_Saved = CRenderer::IsBloomEnabled();
		bChanged = true;
	}

	if (gs_EnableAA_Saved != CRenderer::IsAAEnabled())
	{
		gs_EnableAA_Saved = CRenderer::IsAAEnabled();
		bChanged = true;
	}

	if (gs_EnableTAA_Saved != CRenderer::IsTAAEnabled())
	{
		gs_EnableTAA_Saved = CRenderer::IsTAAEnabled();
		bChanged = true;
	}

	if (gs_EnableTransparency_Saved != CRenderer::IsTransparencyEnabled())
	{
		gs_EnableTransparency_Saved = CRenderer::IsTransparencyEnabled();
		bChanged = true;
	}
	
	if (gs_EnableAO_Saved != CRenderer::IsAOEnabled())
	{
		gs_EnableAO_Saved = CRenderer::IsAOEnabled();
		bChanged = true;
	}
	
	if (gs_EnableSSR_Saved != CRenderer::IsSSREnabled())
	{
		gs_EnableSSR_Saved = CRenderer::IsSSREnabled();
		bChanged = true;
	}

	if (gs_bShowIrradianceProbes_Saved != CLightField::ShouldShowIrradianceProbes())
	{
		gs_bShowIrradianceProbes_Saved = CLightField::ShouldShowIrradianceProbes();
		bChanged = true;
	}

	if (gs_bEnableDiffuseGI_Saved != CLightField::IsEnabled())
	{
		gs_bEnableDiffuseGI_Saved = CLightField::IsEnabled();
		bChanged = true;
	}

	if (gs_bEnableVolumetricMedia_Saved != CRenderer::IsVolumetricsEnabled())
	{
		gs_bEnableVolumetricMedia_Saved = CRenderer::IsVolumetricsEnabled();
		bChanged = true;
	}

	if (gs_bRequestRayCastMaterial_Saved != CDeferredRenderer::ms_bRequestRaycast4EngineFlush)
	{
		gs_bRequestRayCastMaterial_Saved = CDeferredRenderer::ms_bRequestRaycast4EngineFlush;
		bChanged = true;
	}

	return bChanged;
}





void CRenderer::Process()
{
	if (HasFrameStateChanged() || g_bIsFirstFrame)
	{
		CFrameBlueprint::PrepareForSort();

		Render();

		CFrameBlueprint::SortRenderPasses();
		CFrameBlueprint::BakeFrame();
	}

	Render();

	g_bIsFirstFrame = false;
}


void CRenderer::Render()
{
	if (CPhysicsEngine::IsInit())
		CPhysicsEngine::Run();

	CLightsManager::BuildLightList();

	CSchedulerThread::AddRenderTask(g_ShadowMapCommandList, CRenderPass::GetRenderPassTask("Compute Sun Shadow Map"));

	std::vector<SRenderPassTask> renderPasses;
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Compute Shadow Maps"));
	//renderPasses.push_back(CRenderPass::GetRenderPassTask("Bake SDF"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Light Grid"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Static Light Grid"));

	if (gs_bEnableDiffuseGI_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("Update Light Field"));

	CSchedulerThread::AddRenderTask(g_CullLightsCommandList, renderPasses);

	std::vector<unsigned int> kickoff(2);
	kickoff[0] = g_ShadowMapCommandList;
	kickoff[1] = g_CullLightsCommandList;
	
	CCommandListManager::ScheduleDeferredKickoff(kickoff);

	CDeferredRenderer::DrawDeferred();

	//if (gs_EnableTransparency_Saved)
	//	CForwardRenderer::DrawForward();

	renderPasses.clear();
	//renderPasses.push_back(CRenderPass::GetRenderPassTask("Show SDF"));

	if (gs_EnableTAA_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("TAA"));
	
	if (gs_EnableDOF_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("DOF"));
	
	if (gs_EnableBloom_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("Bloom"));
	
	renderPasses.push_back(CRenderPass::GetRenderPassTask("ToneMapping"));
	
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Debug Draw"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Final Copy"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Imgui"));

	if (gs_bRequestRayCastMaterial_Saved)
		renderPasses.push_back(CRenderPass::GetRenderPassTask("Ray Cast Material"));

	CSchedulerThread::AddRenderTask(g_2DCommandList, renderPasses);
	CCommandListManager::ScheduleDeferredKickoff(g_2DCommandList);

	CCommandListManager::LaunchDeferredKickoffs();

	if (gs_bRequestRayCastMaterial_Saved)
		CResourceManager::SubmitFence(ms_FenceFrameFinished);
}



void CRenderer::UpdateGlobalMatrices()
{
	ms_LastGlobalViewMatrix			= ms_GlobalViewMatrix;
	ms_LastGlobalProjMatrix			= ms_GlobalProjMatrix;
	ms_LastGlobalViewProjMatrix		= ms_GlobalViewProjMatrix;
	ms_LastGlobalInvViewMatrix		= ms_GlobalInvViewMatrix;
	ms_LastGlobalInvViewProjMatrix	= ms_GlobalInvViewProjMatrix;

	ms_GlobalViewMatrix		= GetCurrentCamera()->GetViewMatrix();
	ms_GlobalProjMatrix		= GetCurrentCamera()->GetProjMatrix();
	ms_GlobalEyePosition	= GetCurrentCamera()->GetPosition();
	ms_GlobalFOV			= GetCurrentCamera()->GetFOV();
	ms_GlobalNear			= GetCurrentCamera()->GetNearPlane();
	ms_GlobalFar			= GetCurrentCamera()->GetFarPlane();

	ms_GlobalViewProjMatrix = ms_GlobalProjMatrix * ms_GlobalViewMatrix;

	ms_GlobalInvViewMatrix = inverse(ms_GlobalViewMatrix);
	ms_GlobalInvViewProjMatrix = inverse(ms_GlobalViewProjMatrix);

	ms_GlobalViewMatrix.transpose();
	ms_GlobalProjMatrix.transpose();
	ms_GlobalViewProjMatrix.transpose();
	ms_GlobalInvViewMatrix.transpose();
	ms_GlobalInvViewProjMatrix.transpose();

	static int jitterOffset = 0;

	static float samples[8][2] = 
	{
		{0.5f, 1.f / 3.f},
		{0.25f, 2.f / 3.f},
		{0.75f, 1.f / 9.f},
		{0.125f, 4.f / 9.f},
		{0.625f, 7.f / 9.f},
		{0.375f, 2.f / 9.f},
		{0.875f, 5.f / 9.f},
		{0.0625f, 8.f / 9.f}
	};

	float4 offsets(0.f, 0.f, 0.f, 0.f);

	if (gs_EnableTAA_Saved)
	{
		offsets.x = (samples[jitterOffset][0] - 0.5f) / CDeviceManager::GetDeviceWidth();
		offsets.y = (samples[jitterOffset][1] - 0.5f) / CDeviceManager::GetDeviceHeight();

		jitterOffset = (jitterOffset + 1) % 8;
	}

	SFrameConstantBuffer buffer =
	{
		ms_GlobalViewMatrix,
		ms_GlobalProjMatrix,
		ms_GlobalViewProjMatrix,
		ms_GlobalInvViewMatrix,
		ms_GlobalInvViewProjMatrix,

		ms_LastGlobalViewMatrix,
		ms_LastGlobalProjMatrix,
		ms_LastGlobalViewProjMatrix,
		ms_LastGlobalInvViewMatrix,
		ms_LastGlobalInvViewProjMatrix,

		ms_GlobalEyePosition,
		offsets
	};

	if (ms_ViewProjBuffer == INVALIDHANDLE)
		ms_ViewProjBuffer = CResourceManager::CreateFrameConstantBuffer(&buffer, sizeof(buffer));
	else
		CResourceManager::UpdateFrameConstantBuffer(ms_ViewProjBuffer, &buffer);
}



void CRenderer::SetViewProjConstantBuffer(unsigned int nSlot)
{
	CResourceManager::SetConstantBuffer(nSlot, ms_ViewProjBuffer);
}



void CRenderer::UpdateLocalMatrices()
{
	ms_ViewMatrix			= ms_GlobalViewMatrix;
	ms_ProjMatrix			= ms_GlobalProjMatrix;
	ms_ViewProjMatrix		= ms_GlobalViewProjMatrix;
	ms_InvViewMatrix		= ms_GlobalInvViewMatrix;
	ms_InvViewProjMatrix	= ms_GlobalInvViewProjMatrix;

	ms_EyePosition			= ms_GlobalEyePosition;
}



void CRenderer::UpdateBeforeFlush()
{
	CEditor::UpdateBeforeFlush();

	UpdateGlobalMatrices();

	CDeferredRenderer::UpdateBeforeFlush();

	CPacketManager::UpdateBeforeFlush();
	CPacketBuilder::PrepareForFlush();
	CLightsManager::PrepareForFlush();
	CViewportManager::UpdateBeforeFlush();
	CShadowRenderer::PrepareForFlush();
	CLightField::UpdateBeforeFlush();
	CSDF::UpdateBeforeFlush();

	ms_nCurrentFrame++;
}


