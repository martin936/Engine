#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "ShadowRenderer.h"


std::vector<float4x4>	CShadowSpot::ms_pShadowMatrices[2];
std::vector<float4x4>*	CShadowSpot::ms_pShadowMatricesToFill	= &CShadowSpot::ms_pShadowMatrices[0];
std::vector<float4x4>*	CShadowSpot::ms_pShadowMatricesToFlush	= &CShadowSpot::ms_pShadowMatrices[1];

BufferId				CShadowSpot::ms_nShadowMatricesBuffer	= INVALIDHANDLE;



CShadowSpot::CShadowSpot(CLight* pLight) : CShadowRenderer(pLight)
{
	
}


void CShadowSpot::UpdateViewport()
{
	float4x4 shadowMatrix = ComputeShadowMatrix();

	m_bForceUpdateStaticShadowMap = false;

	if (memcmp(shadowMatrix.m(), m_ShadowMatrix.m(), sizeof(float4x4)))
		m_bForceUpdateStaticShadowMap = true;

	m_ShadowMatrix = shadowMatrix;	

	CShadowRenderer::UpdateViewport();	

	if (m_nViewport >= 0)
	{
		m_nViewport = CViewportManager::NewViewport();

		if (m_nViewport >= 0)
		{
			SViewportAssociation viewAss;
			viewAss.m_nViewport = m_nViewport;
			viewAss.m_nStaticIndex = m_bUpdateStatic ? m_nStaticIndex : -1;
			viewAss.m_nDynamicIndex = m_nDynamicIndex;
			viewAss.m_nOmni = 0;

			ms_ViewportsToUpdateToFill->push_back(viewAss);

			if (m_nStaticIndex >= 0)
			{
				(*ms_pShadowMatricesToFill)[m_nStaticIndex] = m_ShadowMatrix;
				(*ms_pShadowMatricesToFill)[m_nStaticIndex].transpose();
			}

			if (m_nDynamicIndex >= 0)
			{
				(*ms_pShadowMatricesToFill)[m_nDynamicIndex] = m_ShadowMatrix;
				(*ms_pShadowMatricesToFill)[m_nDynamicIndex].transpose();
			}

			CViewportManager::SetViewport(m_nViewport, m_Position, m_ShadowMatrix);
		}
	}
}


float4x4 CShadowSpot::ComputeShadowMatrix()
{
	CLight::SLightDesc desc = m_pLight->GetDesc();

	float3 up	= desc.m_Up;
	float3 dir	= desc.m_Dir;
	float3 pos	= desc.m_Pos;

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

	view.m()[3]		= -float3::dotproduct(right, pos);
	view.m()[7]		= -float3::dotproduct(up, pos);
	view.m()[11]	= -float3::dotproduct(dir, pos);

	float fov	= desc.m_fAngleOut;
	float Near	= desc.m_fMinRadius;
	float Far	= desc.m_fMaxRadius;

	proj.m00 = 1.f / tanf(3.141592f * fov / 360.f);
	proj.m11 = proj.m00;
	proj.m22 = 0.5f - 0.5f * (Far + Near) / (Far - Near);
	proj.m32 = 1.f;
	proj.m23 = Far * Near / (Far - Near);
	proj.m33 = 0.f;

	return proj * view;
}

