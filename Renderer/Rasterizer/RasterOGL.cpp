#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Rasterizer.h"


unsigned int gs_QuadScreenVAO;
unsigned int gs_nQuadScreenVBO;


void CRenderer::ClearScreen()
{
	glStencilMask(0xff);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}


void CRenderer::ClearColorBits()
{
	glClear(GL_COLOR_BUFFER_BIT);
}


void CRenderer::ClearColor(float r, float g, float b, float a)
{
	glClearColor(r, g, b, a);
}


void CRenderer::StencilClearValue(unsigned char value)
{
	glClearStencil(value);
}


void CRenderer::InitQuadScreen()
{
	float data[] = {
		-1.f, -1.f, 0.f,
		1.f, -1.f, 0.f,
		-1.f, 1.f, 0.f,
		-1.f, 1.f, 0.f,
		1.f, -1.f, 0.f,
		1.f, 1.f, 0.f
	};

	gs_nQuadScreenVBO = CDeviceManager::CreateVertexBuffer(data, sizeof(data), false);

	gs_QuadScreenVAO = CDeviceManager::CreateCommandList();
	CDeviceManager::BindCommandList(gs_QuadScreenVAO);
	CDeviceManager::BindVertexBuffer(gs_nQuadScreenVBO, 0, 3, CStream::e_Float);
	CDeviceManager::BindCommandList(0);
}


void CRenderer::TerminateQuadScreen()
{
	CDeviceManager::DeleteVertexBuffer(gs_nQuadScreenVBO);
	CDeviceManager::DeleteCommandList(gs_QuadScreenVAO);
}


void CRenderer::RenderQuadScreen()
{
	glBindVertexArray(gs_QuadScreenVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


