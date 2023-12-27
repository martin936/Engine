#include "Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Imgui/Imgui_engine.h"


float					CEngine::ms_LastTime					= 0.f;
float					CEngine::ms_CurrentTime					= 0.f;
float					CEngine::ms_FrameTime					= 0.f;

int						CEngine::ms_eInitFlags					= CEngine::e_Init_Rendering;

CMainRenderingThread*	CEngine::ms_pMainRenderingThread		= nullptr;
CMainGameplayThread*	CEngine::ms_pMainGameplayThread			= nullptr;
CMainPhysicsThread*		CEngine::ms_pMainPhysicsThread			= nullptr;

CEvent*					CEngine::ms_pHaltMainRenderingThread	= nullptr;
CEvent*					CEngine::ms_pHaltMainGameplayThread		= nullptr;
CEvent*					CEngine::ms_pHaltMainPhysicsThread		= nullptr;

CEvent*					CEngine::ms_pStartMainRenderingThread	= nullptr;
CEvent*					CEngine::ms_pStartMainGameplayThread	= nullptr;
CEvent*					CEngine::ms_pStartMainPhysicsThread		= nullptr;

CEvent*					CEngine::ms_pRenderingIsDone			= nullptr;
CEvent*					CEngine::ms_pStartRendering				= nullptr;


CMainRenderingThread::CMainRenderingThread()
{
	m_nGlobalThreadID	= CThread::GetCurrentThreadId();
	m_bStop				= false;

	CThread* pThread = CThread::Create(1024 * 1024, "Main Rendering Thread");
	AttachToThread(pThread);
	pThread->Run();
}


CMainRenderingThread::~CMainRenderingThread()
{
	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


void CMainRenderingThread::Run()
{
	m_bRunning = true;

	CEngine::ms_pStartMainRenderingThread->Wait();

	while (!m_bStop)
	{
		CEngine::ms_pStartRendering->Wait();
		CEngine::ms_pStartRendering->Reset();

		CEngine::StartFrame();

		CTimerManager::GetCPUTimer("CPU Frame")->Start();
		CRenderer::Process();
		CTimerManager::GetCPUTimer("CPU Frame")->Stop();

		CEngine::EndFrame();

		CDeviceManager::FlipScreen();

		CEngine::ms_pRenderingIsDone->Throw();
	}

	CEngine::ms_pHaltMainRenderingThread->Throw();
}


CMainGameplayThread::CMainGameplayThread(void(*pProcessCallback)()) : m_pProcessCallback(pProcessCallback)
{
	m_nGlobalThreadID = CThread::GetCurrentThreadId();
	m_bStop = false;

	CThread* pThread = CThread::Create(1024 * 1024, "Main Gameplay Thread");
	AttachToThread(pThread);
	pThread->Run();
}


CMainGameplayThread::~CMainGameplayThread()
{
	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}
}


void CMainGameplayThread::Run()
{
	m_bRunning = true;

	CEngine::ms_pStartMainGameplayThread->Wait();

	while (!m_bStop)
	{
		CEngine::UpdateFrameDuration();

		m_pProcessCallback();

		CImGui_Impl::Draw();

		CEngine::ms_pRenderingIsDone->Wait();
		CEngine::ms_pRenderingIsDone->Reset();

		if (CPhysicsEngine::IsInit())
			CPhysicsEngine::UpdateBeforeFlush();

		CRenderer::UpdateBeforeFlush();
		CTimerManager::RefreshTimers();

		CEngine::ms_pStartRendering->Throw();
	}

	CEngine::ms_pHaltMainGameplayThread->Throw();
}


CMainPhysicsThread::CMainPhysicsThread()
{
	m_nGlobalThreadID = CThread::GetCurrentThreadId();
	m_bStop = false;

	CThread* pThread = CThread::Create(1024 * 1024, "Main Physics Thread");
	AttachToThread(pThread);
	pThread->Run();
}


CMainPhysicsThread::~CMainPhysicsThread()
{
	if (m_pThread)
	{
		CThread::Destroy(m_pThread);
		m_pThread = NULL;
	}

	CEngine::ms_pHaltMainPhysicsThread->Throw();
}


void CMainPhysicsThread::Run()
{
	m_bRunning = true;

	CEngine::ms_pStartMainPhysicsThread->Wait();

	while (!m_bStop)
	{
		CThread::ThreadSleep(100);
	}

	CEngine::ms_pHaltMainPhysicsThread->Throw();
}


void CEngine::Init(void(*pGameplayProcessCallback)(void), int nFlags)
{
	srand((unsigned int)time(NULL));

	CRenderer::Init();
	CTimerManager::Init();

	CImGui_Impl::Init();

	ms_pStartRendering				= CEvent::Create();
	ms_pRenderingIsDone				= CEvent::Create();
	ms_pRenderingIsDone->Throw();

	ms_pHaltMainGameplayThread		= CEvent::Create();
	ms_pHaltMainRenderingThread		= CEvent::Create();
	
	ms_pStartMainGameplayThread		= CEvent::Create();
	ms_pStartMainRenderingThread	= CEvent::Create();

	ms_eInitFlags = nFlags;
	
	ms_pMainRenderingThread			= new CMainRenderingThread();
	ms_pMainGameplayThread			= new CMainGameplayThread(pGameplayProcessCallback);

	if (nFlags & e_Init_Rendering_And_Physics)
	{
		CPhysicsEngine::Init();
		ms_pMainPhysicsThread = new CMainPhysicsThread();

		ms_pHaltMainPhysicsThread	= CEvent::Create();
		ms_pStartMainPhysicsThread	= CEvent::Create();
	}
}


void CEngine::Terminate()
{
	ms_pMainGameplayThread->Stop();
	ms_pMainRenderingThread->Stop();

	if (ms_eInitFlags & e_Init_Rendering_And_Physics)
		ms_pMainPhysicsThread->Stop();

	ms_pHaltMainGameplayThread->Wait();
	ms_pHaltMainRenderingThread->Wait();

	delete ms_pMainRenderingThread;
	delete ms_pMainGameplayThread;

	delete ms_pHaltMainRenderingThread;
	delete ms_pHaltMainGameplayThread;

	delete ms_pStartMainRenderingThread;
	delete ms_pStartMainGameplayThread;

	if (ms_eInitFlags & e_Init_Rendering_And_Physics)
	{
		ms_pHaltMainPhysicsThread->Wait();

		delete ms_pMainPhysicsThread;
		delete ms_pHaltMainPhysicsThread;
		delete ms_pStartMainPhysicsThread;
	}

	delete ms_pRenderingIsDone;
	delete ms_pStartRendering;

	CDeviceManager::FlushGPU();

	CImGui_Impl::Terminate();

	CRenderer::Terminate();
	CTimerManager::Terminate();
	CTextureInterface::Terminate();
	CFrameBlueprint::Terminate();
	CShader::Terminate();

	if (CPhysicsEngine::IsInit())
		CPhysicsEngine::Terminate();

	CDeviceManager::DestroyDevice();
}


void CEngine::Launch()
{
	ms_pStartMainGameplayThread->Throw();
	ms_pStartMainRenderingThread->Throw();

	if (ms_eInitFlags & e_Init_Rendering_And_Physics)
		ms_pStartMainPhysicsThread->Throw();
}


void CEngine::StartFrame()
{
	CDeviceManager::InitFrame();
	CResourceManager::BeginFrame();
	CCommandListManager::BeginFrame();
}


void CEngine::UpdateFrameDuration()
{
	ms_CurrentTime = CWindow::GetTime();

	ms_FrameTime = ms_CurrentTime - ms_LastTime;
	ms_LastTime = ms_CurrentTime;
}


float CEngine::GetEngineTime()
{
	return CWindow::GetTime();
}


void CEngine::EndFrame()
{
	CResourceManager::EndFrame();
}

