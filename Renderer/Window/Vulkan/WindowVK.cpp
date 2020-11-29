#include "Engine/Engine.h"
#include "../Window.h"
#include "Engine/Editor/Externalized/Externalized.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Imgui/imgui.h"
#include "Engine/Imgui/Vulkan/imgui_impl_vulkan.h"
#include "Engine/Imgui/imgui_impl_win32.h"


bool CWindow::ms_bInit = false;
CWindow* CWindow::ms_pMainWindow = NULL;

extern HINSTANCE g_hInstance;
LARGE_INTEGER g_appStartTime;

EXTERNALIZE(bool, gs_bFullscreen, false)
EXTERNALIZE(int, gs_nWindowWidth, 1600)
EXTERNALIZE(int, gs_nWindowHeight, 900)


void CWindow::Terminate(void)
{
}


float CWindow::GetTime()
{
	LARGE_INTEGER time;
	LARGE_INTEGER frequ;

	QueryPerformanceCounter(&time);
	QueryPerformanceFrequency(&frequ);

	double ret;

	_int64 *itime, *oldtime;
	itime = (_int64 *)&time;
	oldtime = (_int64 *)&g_appStartTime;
	_int64	Elapse = *itime - *oldtime;
	double delapse = (double)Elapse;
	double dbase = (double)*((_int64*)&frequ);
	ret = delapse / dbase;

	return (float)ret;
}


CWindow::CWindow(void* pHandle)
{
	m_pHandle = pHandle;
	m_nWidth = m_nHeight = 0;
	
	m_bFullscreen = false;
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}


CWindow::CWindow(int width, int height, const char* title, bool bFullscreen)
{
	m_pHandle = nullptr;
	m_nWidth = width;
	m_nHeight = height;
	m_bFullscreen = bFullscreen;

	WNDCLASSEX wc;

	//Step 1: Registering the Window Class
	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= g_hInstance;
	wc.hIcon			= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= title;
	wc.hIconSm			= LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	int screenWidth		= GetSystemMetrics(SM_CXSCREEN);
	int screenHeight	= GetSystemMetrics(SM_CYSCREEN);
	int windowLeft		= screenWidth / 2 - width / 2;
	int windowTop		= screenHeight / 2 - height / 2;

	DWORD dwStyle;

	if (bFullscreen)
		dwStyle = WS_POPUP;
	else
		dwStyle = WS_OVERLAPPEDWINDOW;

	// Step 2: Creating the Window
	m_pHandle = CreateWindow(
								title,
								title,
								dwStyle,
								windowLeft, windowTop, width, height,
								NULL, NULL, g_hInstance, NULL);

	if (m_pHandle == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	ShowWindow((HWND)m_pHandle, SW_SHOW);
	SetForegroundWindow((HWND)m_pHandle);
	SetFocus((HWND)m_pHandle);

	QueryPerformanceCounter(&g_appStartTime);

	if (ms_pMainWindow == nullptr)
		SetMainWindow();

	CDeviceManager::CreateDevice();
}


CWindow::~CWindow()
{
}


void CWindow::MainLoop() const
{
	MSG Msg;

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}


void CWindow::SwapBuffers(void) const
{
	CDeviceManager::FlipScreen();
}