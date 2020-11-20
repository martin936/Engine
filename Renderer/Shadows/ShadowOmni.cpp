#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "ShadowRenderer.h"



std::vector<float4x4>	CShadowOmni::ms_pShadowMatrices[2];
std::vector<float4x4>*	CShadowOmni::ms_pShadowMatricesToFill	= &CShadowOmni::ms_pShadowMatrices[0];
std::vector<float4x4>*	CShadowOmni::ms_pShadowMatricesToFlush	= &CShadowOmni::ms_pShadowMatrices[1];

std::vector<CShadowOmni::ShadowInfo>		CShadowOmni::ms_pShadowPos[2];
std::vector<CShadowOmni::ShadowInfo>*		CShadowOmni::ms_pShadowPosToFill = &CShadowOmni::ms_pShadowPos[0];
std::vector<CShadowOmni::ShadowInfo>*		CShadowOmni::ms_pShadowPosToFlush = &CShadowOmni::ms_pShadowPos[1];

BufferId				CShadowOmni::ms_nShadowMatricesBuffer	= INVALIDHANDLE;
BufferId				CShadowOmni::ms_nShadowPosBuffer	= INVALIDHANDLE;



CShadowOmni::CShadowOmni(CLight* pLight) : CShadowRenderer(pLight)
{
	
}



void CShadowOmni::ComputeShadowMatrices(float4x4* p_ShadowMatrices)
{
	CLight::SLightDesc desc = m_pLight->GetDesc();

	float Near = desc.m_fMinRadius;
	float Far = desc.m_fMaxRadius;

	m_Near = Near;
	m_Far = Far;

	float d = 1.f / (Far - Near);
	float a = 0.5f - 0.5f * (Far + Near) * d;
	float b = (Far * Near) * d;

	p_ShadowMatrices[0] = float4x4(float4(0.f, 0.f, -1.f, desc.m_Pos.z), float4(0.f, -1.f, 0.f, desc.m_Pos.y), float4(a, 0.f, 0.f, b - a * desc.m_Pos.x), float4(1.f, 0.f, 0.f, -desc.m_Pos.x));
	p_ShadowMatrices[1] = float4x4(float4(0.f, 0.f, 1.f, -desc.m_Pos.z), float4(0.f, -1.f, 0.f, desc.m_Pos.y), float4(-a, 0.f, 0.f, b + a * desc.m_Pos.x), float4(-1.f, 0.f, 0.f, desc.m_Pos.x));
	p_ShadowMatrices[2] = float4x4(float4(1.f, 0.f, 0.f, -desc.m_Pos.x), float4(0.f, 0.f, -1.f, desc.m_Pos.z), float4(0.f, -a, 0.f, b + a * desc.m_Pos.y), float4(0.f, -1.f, 0.f, desc.m_Pos.y));
	p_ShadowMatrices[3] = float4x4(float4(1.f, 0.f, 0.f, -desc.m_Pos.x), float4(0.f, 0.f, 1.f, -desc.m_Pos.z), float4(0.f, a, 0.f, b - a * desc.m_Pos.y), float4(0.f, 1.f, 0.f, -desc.m_Pos.y));
	p_ShadowMatrices[4] = float4x4(float4(1.f, 0.f, 0.f, -desc.m_Pos.x), float4(0.f, -1.f, 0.f, desc.m_Pos.y), float4(0.f, 0.f, a, b - a * desc.m_Pos.z), float4(0.f, 0.f, 1.f, -desc.m_Pos.z));
	p_ShadowMatrices[5] = float4x4(float4(-1.f, 0.f, 0.f, desc.m_Pos.x), float4(0.f, -1.f, 0.f, desc.m_Pos.y), float4(0.f, 0.f, -a, b + a * desc.m_Pos.z), float4(0.f, 0.f, -1.f, desc.m_Pos.z));
}



void CShadowOmni::UpdateViewport()
{
	float4x4 shadowMatrix[6];
	
	ComputeShadowMatrices(shadowMatrix);

	m_bForceUpdateStaticShadowMap = false;

	if (memcmp(shadowMatrix[0].m(), m_ShadowMatrix[0].m(), sizeof(float4x4)))
		m_bForceUpdateStaticShadowMap = true;

	for (int i = 0; i < 6; i++)
		m_ShadowMatrix[i] = shadowMatrix[i];

	CShadowRenderer::UpdateViewport();

	if (m_nViewport >= 0)
	{
		m_nViewport = CViewportManager::NewViewport();

		if (m_nViewport >= 0)
		{
			SViewportAssociation viewAss;
			viewAss.m_nViewport		= m_nViewport;
			viewAss.m_nStaticIndex	= m_bUpdateStatic ? m_nStaticIndex : -1;
			viewAss.m_nDynamicIndex = m_nDynamicIndex;
			viewAss.m_nOmni = 1;

			ms_ViewportsToUpdateToFill->push_back(viewAss);

			for (int i = 0; i < 6; i++)
			{
				if (m_nStaticIndex >= 0)
				{
					(*ms_pShadowMatricesToFill)[6 * m_nStaticIndex + i] = m_ShadowMatrix[i];
					(*ms_pShadowMatricesToFill)[6 * m_nStaticIndex + i].transpose();

					(*ms_pShadowPosToFill)[m_nStaticIndex].m_Pos = m_Position;
					(*ms_pShadowPosToFill)[m_nStaticIndex].m_NearFar = float4(m_Near, m_Far, 0.f, 0.f);
				}

				if (m_nDynamicIndex >= 0)
				{
					(*ms_pShadowMatricesToFill)[6 * m_nDynamicIndex + i] = m_ShadowMatrix[i];
					(*ms_pShadowMatricesToFill)[6 * m_nDynamicIndex + i].transpose();

					(*ms_pShadowPosToFill)[m_nDynamicIndex].m_Pos = m_Position;
					(*ms_pShadowPosToFill)[m_nDynamicIndex].m_NearFar = float4(m_Near, m_Far, 0.f, 0.f);
				}
			}

			CViewportManager::SetViewportOmni(m_nViewport, m_Position, m_Far);
		}
	}
}


