#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "LightsManager.h"
#include "Lights.h"


ProgramHandle gs_nAreaSpotLightPID = INVALID_PROGRAM_HANDLE;
ProgramHandle gs_nAreaSpotLightShadowPID = INVALID_PROGRAM_HANDLE;



struct AreaSpotLightFragmentShaderConstants
{
	float	InvViewProj[16];
	float 	LightDir[3];
	float	LightPower;
	float	LightPos[3];
	float	DiskRadius;
	float	LightColor[4];
	float	Eye[4];
};



void CAreaSpotLight::Init()
{
	//gs_nAreaSpotLightPID = CShader::LoadProgram(SHADER_PATH("Lights"), "Light", "AreaSpotLight");
	//gs_nAreaSpotLightShadowPID = CShader::LoadProgram(SHADER_PATH("Lights"), "Light", "AreaSpotLightShadow");
}



void CAreaSpotLight::Terminate()
{
	CShader::DeleteProgram(gs_nAreaSpotLightPID);
	CShader::DeleteProgram(gs_nAreaSpotLightShadowPID);
}



void CAreaSpotLight::Init(float3 vPos, float3 vDir, float fStrength, float3 vColor, float fInAngle, float fOutAngle, float fDiskRadius, float fRadius)
{
	m_vPos = vPos;
	m_vDir = vDir;
	m_fStrength = fStrength;
	m_vColor = vColor;

	m_fInAngle = fInAngle;
	m_fOutAngle = fOutAngle;

	m_fDiskRadius = fDiskRadius;
	m_fRadius = fRadius;

	if (m_bCastShadows)
		CreateShadowRenderer();
}


void CAreaSpotLight::CreateShadowRenderer()
{
	m_pShadowRenderer = new CShadowSpot(this);
}



void CAreaSpotLight::Draw()
{
	/*if (m_bCastShadows)
		DrawScreenShadows();

	GLuint pid;

	if (m_bCastShadows)
		pid = gs_nAreaSpotLightShadowPID;
	else
		pid = gs_nAreaSpotLightPID;


	CShader::BindProgram(pid);

	SRenderTarget* normal = CDeferredRenderer::GetNormalTarget();
	SRenderTarget* info = CDeferredRenderer::GetInfoTarget();

	CTexture::SetTexture(pid, CRenderer::GetDepthStencil()->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(pid, normal->m_nTextureId, 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	CTexture::SetTexture(pid, info->m_nTextureId, 2);
	CSampler::BindSamplerState(2, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);

	if (m_bCastShadows)
	{
		CTexture::SetTexture(pid, CLightsManager::ms_pFinalShadowTarget->m_nTextureId, 3);
		CSampler::BindSamplerState(3, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	}

	AreaSpotLightFragmentShaderConstants fregisters;
	float4x4 InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	InvViewProj.transpose();

	fregisters.LightPower = m_fStrength;
	fregisters.DiskRadius = m_fDiskRadius;

	memcpy(&fregisters.LightColor, m_vColor.v(), 3 * sizeof(float));

	memcpy(&fregisters.LightPos, m_vPos.v(), 3 * sizeof(float));

	memcpy(&fregisters.LightDir, m_vDir.v(), 3 * sizeof(float));

	memcpy(&fregisters.InvViewProj, &InvViewProj.m00, 16 * sizeof(float));

	memcpy(&fregisters.Eye, CRenderer::GetCurrentCamera()->GetPosition().v(), 3 * sizeof(float));

	if (!m_bIsConstantBufferInit)
	{
		m_nConstantBufferId = CDeviceManager::CreateConstantBuffer(&fregisters, sizeof(fregisters));
		m_bIsConstantBufferInit = true;
	}
	else
		CDeviceManager::FillConstantBuffer(m_nConstantBufferId, &fregisters, sizeof(fregisters));

	CDeviceManager::BindConstantBuffer(m_nConstantBufferId, pid, 0);

	CRenderer::RenderQuadScreen();*/
}
