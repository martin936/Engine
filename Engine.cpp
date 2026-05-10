#include "Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Imgui/Imgui_engine.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Misc/FileSystem.h"
#include "Engine/Editor/Adjustables/Adjustables.h"


TimeSpan				CEngine::ms_LastTime;
TimeSpan				CEngine::ms_CurrentTime;
TimeSpan				CEngine::ms_FrameTime;

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
	srand(static_cast<unsigned int>(time(nullptr)));
	
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
	srand(static_cast<unsigned int>(time(nullptr)));
	
	m_bRunning = true;

	CEngine::ms_pStartMainGameplayThread->Wait();

	while (!m_bStop)
	{
		CEngine::UpdateFrameDuration();

		// Sample DirectInput on the main (HWND-owning) thread before the
		// gameplay frame reads input state.
		CWindow::PollInputs();

		// Refresh gamepad slot table and per-pad state for this frame.
		CGamepad::RefreshConnected();		
		
		CAdjustable::CommitFrameSnapshot();
		
		CImGui_Impl::Draw();
		
		CEngine::ms_pRenderingIsDone->Wait();
		CEngine::ms_pRenderingIsDone->Reset();

		m_pProcessCallback();
		

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
	srand(static_cast<unsigned int>(time(nullptr)));
	
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
	srand(static_cast<unsigned int>(time(nullptr)));
	
	FileSystem::Init();

	ms_eInitFlags = nFlags;

	CRenderer::Init();
	CTimerManager::Init();

	CImGui_Impl::Init();

	CGamepad::Init();
	CKeyboard::Init();
	
	CAdjustable::ReadAdjustables();
	CAdjustable::CommitFrameSnapshot();

	ms_pStartRendering				= CEvent::Create();
	ms_pRenderingIsDone				= CEvent::Create();
	ms_pRenderingIsDone->Throw();

	ms_pHaltMainGameplayThread		= CEvent::Create();
	ms_pHaltMainRenderingThread		= CEvent::Create();
	
	ms_pStartMainGameplayThread		= CEvent::Create();
	ms_pStartMainRenderingThread	= CEvent::Create();

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

	CKeyboard::Terminate();
	CGamepad::Terminate();

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
	// Prime the frame timer so the first GetFrameDuration() doesn't report all
	// the time spent in CEngine::Init / game-side Init as one giant first frame.
	ms_LastTime    = CWindow::GetTime();
	ms_CurrentTime = ms_LastTime;
	ms_FrameTime   = TimeSpan::Zero();

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


TimeSpan CEngine::GetEngineTime()
{
	return CWindow::GetTime();
}


void CEngine::EndFrame()
{
	CResourceManager::EndFrame();
}

