#ifndef ENGINE_INC
#define ENGINE_INC

#ifdef _WIN32
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#else
#include <chrono>
#endif

#ifdef __VULKAN__
#include "vulkan/vulkan.h"
#elif __OPENGL__
#include "GL/glew.h"
#endif

#ifndef _WIN32
#include "GLFW/glfw3.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <iostream>

#ifndef M_PI
#define M_PI 3.141592f
#endif

#define MAX(a, b)		((a) > (b) ? (a) : (b))
#define MIN(a, b)		((a) < (b) ? (a) : (b))
#define CLAMP(x, a, b)	MAX(a, MIN(b, x))

#define SAFE_DELETE(x) if(x != NULL) delete x;

#define ENABLE_CHECKS

#define lengthof(ARRAY) (sizeof(ARRAY) / sizeof(*ARRAY))


#include "Engine/Maths/Maths.h"
#include "Engine/Misc/Assert.h"
#include "Engine/Threads/Thread.h"
#include "Engine/Misc/Event.h"
#include "Engine/Misc/Semaphore.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#ifdef clamp
#undef clamp
#endif


class CMainRenderingThread : public CThreadWorker
{
public:

	CMainRenderingThread();
	~CMainRenderingThread();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {}

private:

	ThreadId		m_nGlobalThreadID;
	bool			m_bStop;
};


class CMainGameplayThread : public CThreadWorker
{
public:

	CMainGameplayThread(void (*pProcessCallback)());
	~CMainGameplayThread();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {}

private:

	void			(*m_pProcessCallback)();

	ThreadId		m_nGlobalThreadID;
	bool			m_bStop;
};


class CMainPhysicsThread : public CThreadWorker
{
public:

	CMainPhysicsThread();
	~CMainPhysicsThread();

	void Stop() { m_bStop = true; }
	void Run();
	void Cancel() {}

private:

	ThreadId		m_nGlobalThreadID;
	bool			m_bStop;
};



class CEngine
{
	friend CMainRenderingThread;
	friend CMainGameplayThread;
	friend CMainPhysicsThread;

public:

	enum EInitFeature
	{
		e_Init_Rendering = 1,
		e_Init_Rendering_And_Physics = 2
	};

	static void Init(void (*pGameplayProcessCallback)(void), int nFlags = e_Init_Rendering);

	static void Launch();

	static void Terminate();

	static void StartFrame();

	static void UpdateFrameDuration();

	static void EndFrame();

	inline static float GetFrameDuration() 
	{
		return ms_FrameTime;
	}

	static float GetEngineTime();

private:

	static float ms_LastTime;
	static float ms_CurrentTime;
	static float ms_FrameTime;

	static int						ms_eInitFlags;

	static CEvent*					ms_pHaltMainRenderingThread;
	static CEvent*					ms_pHaltMainGameplayThread;
	static CEvent*					ms_pHaltMainPhysicsThread;

	static CEvent*					ms_pStartMainRenderingThread;
	static CEvent*					ms_pStartMainGameplayThread;
	static CEvent*					ms_pStartMainPhysicsThread;

	static CEvent*					ms_pRenderingIsDone;
	static CEvent*					ms_pStartRendering;

	static CMainRenderingThread*	ms_pMainRenderingThread;
	static CMainGameplayThread*		ms_pMainGameplayThread;
	static CMainPhysicsThread*		ms_pMainPhysicsThread;

};



#endif
