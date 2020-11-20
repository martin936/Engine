#ifndef _GLFW_USE_OPENGL
#define _GLFW_USE_OPENGL
#endif

#include "Engine/Imgui/imgui.h"
#include "Engine/Imgui/OpenGL/imgui_impl_glfw.h"
#include "Engine/Imgui/OpenGL/imgui_impl_opengl3.h"

#include "Engine/Engine.h"
#include "../Window.h"
#include "Engine/Editor/Externalized/Externalized.h"
#include "Engine/Device/DeviceManager.h"


bool CWindow::ms_bInit = false;
CWindow* CWindow::ms_pMainWindow = NULL;

EXTERNALIZE(bool, gs_bFullscreen, false)
EXTERNALIZE(int, gs_nWindowWidth, 1600)
EXTERNALIZE(int, gs_nWindowHeight, 900)


void CWindow::Initialize(void)
{
	if (!glfwInit()) 
	{
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return;
	}

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // OpenGL 4.4
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__ 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	glDisable(GL_MULTISAMPLE);

	glfwSwapInterval(1);
}


void CWindow::Terminate(void)
{
	if (ms_pMainWindow != NULL)
	{
		delete ms_pMainWindow;
		ms_pMainWindow = NULL;
	}

	glfwTerminate();
}


float CWindow::GetTime()
{
	return (float)glfwGetTime();
}


CWindow::CWindow(void* pHandle)
{
	m_pHandle = pHandle;

	glfwGetWindowSize((GLFWwindow*)pHandle, &m_nWidth, &m_nHeight);
	m_bFullscreen = false;

	if (!ms_bInit)
	{
		CDeviceManager::CreateDevice();

		ImGui::CreateContext();

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL((GLFWwindow*)pHandle, true);
		ImGui_ImplOpenGL3_Init("#version 130");

		ms_bInit = true;
	}
}


CWindow::CWindow(int width, int height, const char* title, bool bFullscreen)
{
	if (!ms_bInit)
		Initialize();

	m_nWidth = width;
	m_nHeight = height;
	m_bFullscreen = bFullscreen;

	GLFWwindow* pWindow = glfwCreateWindow(width, height, title, bFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);

	glfwMakeContextCurrent(pWindow);
	glfwSetInputMode(pWindow, GLFW_STICKY_KEYS, GL_FALSE);

	m_pHandle = (void*)pWindow;

	if (!ms_bInit)
	{
		CDeviceManager::CreateDevice();

		ImGui::CreateContext();

		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForOpenGL(pWindow, true);
		ImGui_ImplOpenGL3_Init("#version 130");

		ms_bInit = true;
	}
}


CWindow::~CWindow()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow((GLFWwindow*)m_pHandle);
}


bool CWindow::ShouldClose(void) const
{
	glfwPollEvents();
	return glfwWindowShouldClose((GLFWwindow*)m_pHandle) ? true : false;
}


void CWindow::SwapBuffers(void) const
{
	ImGui::Render();
#ifdef __OPENGL__
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

	glfwSwapBuffers((GLFWwindow*)m_pHandle);
}