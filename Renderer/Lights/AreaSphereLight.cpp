#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "LightsManager.h"
#include "Lights.h"



ProgramHandle gs_nAreaSphereLightPID = INVALID_PROGRAM_HANDLE;
ProgramHandle gs_nAreaSphereLightShadowPID = INVALID_PROGRAM_HANDLE;



struct AreaSphereLightFragmentShaderConstants
{
	float	InvViewProj[16];
	float	LightPos[3];
	float	DiskRadius;
	float	LightColor[3];
	float	LightPower;
	float	Eye[4];
};



void CAreaSphereLight::Init()
{
	//gs_nAreaSphereLightPID = CShader::LoadProgram(SHADER_PATH("Lights"), "Light", "AreaSphereLight");
	//gs_nAreaSphereLightShadowPID = CShader::LoadProgram(SHADER_PATH("Lights"), "Light", "AreaSphereLightShadow");
}



void CAreaSphereLight::Terminate()
{
	CShader::DeleteProgram(gs_nAreaSphereLightPID);
	CShader::DeleteProgram(gs_nAreaSphereLightShadowPID);
}



void CAreaSphereLight::Init(float3 vPos, float fStrength, float3 vColor, float fDiskRadius, float fRadius)
{
	m_vPos = vPos;
	m_fStrength = fStrength;
	m_vColor = vColor;

	m_fSphereRadius = fDiskRadius;
	m_fRadius = fRadius;

	if (m_bCastShadows)
		CreateShadowRenderer();
}


void CAreaSphereLight::CreateShadowRenderer()
{
	m_pShadowRenderer = new CShadowOmni(this);
}



void CAreaSphereLight::Draw()
{
	/*if (m_bCastShadows)
		DrawScreenShadows();

	GLuint pid;

	if (m_bCastShadows)
		pid = gs_nAreaSphereLightShadowPID;
	else
		pid = gs_nAreaSphereLightPID;


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

	AreaSphereLightFragmentShaderConstants fregisters;
	float4x4 InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	InvViewProj.transpose();

	fregisters.LightPower = m_fStrength;

	memcpy(&fregisters.LightColor, m_vColor.v(), 3 * sizeof(float));

	memcpy(&fregisters.LightPos, m_vPos.v(), 3 * sizeof(float));

	memcpy(&fregisters.InvViewProj, &InvViewProj.m00, 16 * sizeof(float));

	memcpy(&fregisters.Eye, CRenderer::GetCurrentCamera()->GetPosition().v(), 3 * sizeof(float));

	fregisters.DiskRadius = m_fSphereRadius;

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
