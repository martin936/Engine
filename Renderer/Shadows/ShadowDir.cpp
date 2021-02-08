#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "ShadowRenderer.h"


bool CShadowDir::ms_bDrawStatic				= true;
bool CShadowDir::ms_bDrawStatic4EngineFlush = true;

BufferId		CShadowDir::ms_nShadowMatricesBuffer	= INVALIDHANDLE;
CShadowDir*		CShadowDir::ms_pSunShadowRenderer		= nullptr;


CShadowDir::CShadowDir(CLight* pLight) : CShadowRenderer(pLight)
{
	ms_pSunShadowRenderer = this;

	m_ShadowMatrix = 0.f;
}



void CShadowDir::UpdateViewport()
{
	float4x4 shadowMatrix = ComputeShadowMatrix();

	ms_bDrawStatic = false;

	if (memcmp(shadowMatrix.m(), m_ShadowMatrix.m(), sizeof(float4x4)))
		ms_bDrawStatic = true;

	m_ShadowMatrix = shadowMatrix;

	m_nViewport = CViewportManager::NewViewport();

	if (m_nViewport >= 0)
	{
		CViewportManager::SetViewport(m_nViewport, m_Position, m_ShadowMatrix);
	}
}


float4x4 CShadowDir::ComputeShadowMatrix()
{
	CLight::SLightDesc desc = m_pLight->GetDesc();

	float3 up = desc.m_Up;
	float3 dir = desc.m_Dir;
	float3 pos = desc.m_Pos;

	up = up - float3::dotproduct(up, dir) * dir;
	up.normalize();

	float3 right = float3::cross(dir, up);

	float4x4 view;
	float4x4 proj;

	view.Eye();
	proj.Eye();

	memcpy(view.m(), right.v(), 3 * sizeof(float));
	memcpy(view.m() + 4, up.v(), 3 * sizeof(float));
	memcpy(view.m() + 8, dir.v(), 3 * sizeof(float));

	view.m()[3] = -float3::dotproduct(right, pos);
	view.m()[7] = -float3::dotproduct(up, pos);
	view.m()[11] = -float3::dotproduct(dir, pos);

	float Far = desc.m_fMaxRadius;

	proj.m00 = 1.f / 50.f;
	proj.m11 = 1.f / 50.f;
	proj.m22 = -1.f / Far;
	proj.m23 = 1.f;
	proj.m33 = 1.f;

	return proj * view;
}



int CShadowDir::UpdateShader(Packet* packet, void* pData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)pData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	if (CRenderer::IsPacketStatic() && !CShadowDir::ms_bDrawStatic4EngineFlush)
		return -1;

	unsigned int viewportMask[6] = { 0U };

	viewportMask[0] = CRenderer::IsPacketStatic() ? 2 : 1;

	CResourceManager::SetPushConstant(CShader::e_VertexShader, viewportMask, 6 * sizeof(unsigned int));

	return 1;
}

