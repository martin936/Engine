#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/AO/AO.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Engine/Renderer/Preview/PreviewManager.h"
#include "Engine/Renderer/SSS/SSS.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "LightsManager.h"


ADJUSTABLE("Sun Power",				float,	gs_SunPower, 3.f, 0.f, 10.f, "Graphics/Lights/SunLight")
ADJUSTABLE("Ambient Power",			float,	gs_AmbientPower, 0.25f, 0.f, 1.f, "Graphics/Lights/Ambient")
ADJUSTABLE("Cell Shading Factor",	int,	gs_nCelShadingFactor, 3, 0, 10, "Graphics/Cel Shading")
ADJUSTABLE("Enable SSAO",			bool,	gs_bEnableSSAO, true, false, true, "Graphics/Lights/Ambient")
ADJUSTABLE("Enable Shadows",		bool,	gs_bEnableShadows, true, false, true, "Graphics/Lights/SunLight")



CLight::SLightDesc::SLightDesc()
{
	m_Pos			= 0.f;
	m_fIntensity	= 0.f;

	m_Color			= 0.f;
	m_nType			= e_Omni;

	m_Dir			= float3(0.f, 0.f, -1.f);
	m_AreaSize		= 0.f;

	m_Up			= float3(0.f, 1.f, 0.f);

	m_fMinRadius	= 0.01f;
	m_fMaxRadius	= 1.f;
	m_fAngleIn		= 0.f;
	m_fAngleOut		= 0.f;

	m_nIsVolumetric	= 0;
	m_nCastShadows	= 0;
	m_nStaticShadowMapIndex = -1;
	m_nDynamicShadowMapIndex = -1;
}



CLight::CLight(bool bCastShadow)
{
	m_bEnabled = true;
}

CLight::~CLight()
{
	if (m_pShadowRenderer != NULL)
		delete m_pShadowRenderer;
}


void CLight::Init()
{

}


void CLight::Terminate()
{

}



void CLight::RenderShadowMap() const
{
}


CShadowRenderer* CLight::GetShadowRenderer()
{
	if (!m_pShadowRenderer)
		CreateShadowRenderer();

	return m_pShadowRenderer;
}



void COmniLight::Init(float3 vPos, float fStrength, float3 vColor, float fRadius)
{
	m_Desc.m_Pos = vPos;
	m_Desc.m_fIntensity = fStrength;
	m_Desc.m_Color = vColor;
	m_Desc.m_fMaxRadius = fRadius;

	if (m_Desc.m_nCastShadows)
		CreateShadowRenderer();
}


void COmniLight::CreateShadowRenderer()
{
	m_pShadowRenderer = new CShadowOmni(this);
}


void CSpotLight::Init(float3 vPos, float3 vDir, float fStrength, float3 vColor, float fInAngle, float fOutAngle, float fRadius)
{
	vDir.normalize();

	m_Desc.m_Pos = vPos;
	m_Desc.m_Dir = vDir;
	m_Desc.m_fIntensity = fStrength;
	m_Desc.m_Color = vColor;
	m_Desc.m_fMaxRadius = fRadius;
	m_Desc.m_nType = m_Type;

	m_Desc.m_Dir.normalize();

	float cosInner = cosf(fInAngle * 0.5f);
	float cosOuter = cosf(fOutAngle * 0.5f);

	m_Desc.m_fAngleIn = fInAngle;
	m_Desc.m_fAngleOut = fOutAngle;

	if (m_Desc.m_nCastShadows)
		CreateShadowRenderer();
}


void CSpotLight::CreateShadowRenderer()
{
	m_pShadowRenderer = new CShadowSpot(this);
}




void CSunLight::Init(float3 vPos, float3 vDir, float fStrength, float3 vColor, float fRadius)
{
	vDir.normalize();

	m_Desc.m_Pos = vPos - 0.5f * fRadius * vDir;
	m_Desc.m_Dir = vDir;
	m_Desc.m_fIntensity = fStrength;
	m_Desc.m_Color = vColor;
	m_Desc.m_fMaxRadius = fRadius;
	m_Desc.m_nCastShadows = true;
	m_Desc.m_nCastDynamicShadows = true;
	m_Desc.m_nCastStaticShadows = true;
	m_Desc.m_nType = m_Type;

	m_Desc.m_Dir.normalize();

	if (m_Desc.m_nCastShadows)
		CreateShadowRenderer();
}


void CSunLight::CreateShadowRenderer()
{
	m_pShadowRenderer = new CShadowDir(this);
}
