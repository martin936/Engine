#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Preview/PreviewManager.h"
#include "Shadowmaps.h"


ProgramHandle gs_nShadowPID = INVALID_PROGRAM_HANDLE;
ProgramHandle gs_nTessShadowPID = INVALID_PROGRAM_HANDLE;
ProgramHandle gs_nAlphaShadowPID = INVALID_PROGRAM_HANDLE;

ProgramHandle gs_nInstancedShadowPID = INVALID_PROGRAM_HANDLE;
ProgramHandle gs_nInstancedAlphaShadowPID = INVALID_PROGRAM_HANDLE;

DepthStencilArray* CShadowMap::ms_pDepthStencilArray = NULL;

int CShadowMap::ms_nWidth = 4096;
int CShadowMap::ms_nHeight = 4096;
int CShadowMap::ms_nNumShadowMaps = 0;

unsigned int  gs_ShadowTessConstantBuffer = 0;


struct SShadowTessConstants
{
	float	m_Eye[3];
	float	m_TanFOV;
};


CShadowMap::CShadowMap()
{
	m_Proj = float4x4(0.f);
	m_View = float4x4(0.f);
	m_ViewProj = float4x4(0.f);
	m_InvViewProj = float4x4(0.f);

	m_nIndex = ms_nNumShadowMaps;
	ms_nNumShadowMaps++;
}


CShadowMap::CShadowMap(int width, int height) : CShadowMap()
{
	m_nWidth = width;
	m_nHeight = height;
}


CShadowMap::~CShadowMap()
{
}


void CShadowMap::Init()
{
	gs_nShadowPID = CShader::LoadProgram(SHADER_PATH("Shadows"), "Shadows", "Shadows");
	gs_nInstancedShadowPID = CShader::LoadProgram(SHADER_PATH("Shadows"), "InstancedShadows", "Shadows");
	gs_nTessShadowPID = CShader::LoadProgram(SHADER_PATH("Shadows"), "TessShadows", "TessShadows", "TessShadows", "Shadows");
	gs_nAlphaShadowPID = CShader::LoadProgram(SHADER_PATH("Shadows"), "AlphaShadows", "AlphaShadows");
	gs_nInstancedAlphaShadowPID = CShader::LoadProgram(SHADER_PATH("Shadows"), "InstancedAlphaShadows", "AlphaShadows");

	gs_ShadowTessConstantBuffer = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SShadowTessConstants));
}


void CShadowMap::Terminate()
{
	delete ms_pDepthStencilArray;

	CShader::DeleteProgram(gs_nShadowPID);
	CShader::DeleteProgram(gs_nInstancedShadowPID);
	CShader::DeleteProgram(gs_nTessShadowPID);
	CShader::DeleteProgram(gs_nAlphaShadowPID);
	CShader::DeleteProgram(gs_nInstancedAlphaShadowPID);
	CDeviceManager::DeleteConstantBuffer(gs_ShadowTessConstantBuffer);
}


void CShadowMap::BuildArray()
{
	ms_pDepthStencilArray = CFramebuffer::CreateDepthStencilArray(ms_nWidth, ms_nHeight, ms_nNumShadowMaps);
}


void CShadowMap::ComputeViewMatrix()
{
	m_vUp = m_vUp - float3::dotproduct(m_vUp, m_vDir) * m_vDir;
	m_vUp.normalize();

	float3 vRight = float3::cross(m_vDir, m_vUp);

	memcpy(m_View.m(), vRight.v(), 3 * sizeof(float));
	memcpy(m_View.m() + 4, m_vUp.v(), 3 * sizeof(float));
	memcpy(m_View.m() + 8, m_vDir.v(), 3 * sizeof(float));

	m_View.m()[3] = -float3::dotproduct(vRight, m_vPos);
	m_View.m()[7] = -float3::dotproduct(m_vUp, m_vPos);
	m_View.m()[11] = -float3::dotproduct(m_vDir, m_vPos);
	m_View.m33 = 1.f;
}


void CShadowMap::ComputeOrthographicProjectionMatrix()
{
	m_Proj = 0.f;

	m_Proj.m00 = 2.f / m_fViewportWidth;
	m_Proj.m11 = 2.f / m_fViewportHeight;
	m_Proj.m22 = 1.f / (m_fFarPlane - m_fNearPlane);
	m_Proj.m33 = 1.f;

	m_Proj.m23 = -m_fNearPlane / (m_fFarPlane - m_fNearPlane);
}


void CShadowMap::ComputePerspectiveProjectionMatrix()
{
	m_Proj = 0.f;

	m_Proj.m00 = 1.f / tanf(3.141592f*m_fFOV / 360.f);
	m_Proj.m11 = m_fAspectRatio*m_Proj.m00;
	m_Proj.m22 = (m_fFarPlane + m_fNearPlane) / (m_fFarPlane - m_fNearPlane);
	m_Proj.m32 = 1.f;
	m_Proj.m23 = -m_fFarPlane*m_fNearPlane / (m_fFarPlane - m_fNearPlane);
	m_Proj.m33 = 0.f;
}


void CShadowMap::SetOrthoShadowMap(float3 vPos, float3 vDir, float3 vUp, float fNearPlane, float fFarPlane, float fViewportWidth, float fViewportHeight)
{
	m_vPos	= vPos;
	m_vDir	= vDir;
	m_vUp	= vUp;

	m_vDir.normalize();
	m_vUp.normalize();

	m_fNearPlane		= fNearPlane;
	m_fFarPlane			= fFarPlane;
	m_fViewportWidth	= fViewportWidth;
	m_fViewportHeight	= fViewportHeight;

	ComputeViewMatrix();
	ComputeOrthographicProjectionMatrix();

	m_ViewProj = m_Proj * m_View;
}


void CShadowMap::SetPerspectiveShadowMap(float3 vPos, float3 vDir, float3 vUp, float fNearPlane, float fFarPlane, float fFOV, float fAspectRatio)
{
	m_vPos = vPos;
	m_vDir = vDir;
	m_vUp = vUp;

	m_vDir.normalize();
	m_vUp.normalize();

	m_fNearPlane = fNearPlane;
	m_fFarPlane = fFarPlane;
	m_fFOV = m_fFOV;
	m_fAspectRatio = fAspectRatio;

	ComputeViewMatrix();
	ComputePerspectiveProjectionMatrix();

	m_ViewProj = m_Proj * m_View;
}


void CShadowMap::Draw()
{
	CFramebuffer::BindDepthStencilArray(ms_pDepthStencilArray, m_nIndex);

	CRenderStates::SetWriteMask(e_Depth);
	CRenderer::ClearScreen();

	ComputeViewMatrix();

	m_ViewProj = m_Proj * m_View;

	float4x4 ViewProj_old = CRenderer::GetCurrentCamera()->GetViewProjMatrix();

	CRenderer::GetCurrentCamera()->SetViewProjMatrix(m_ViewProj);

	CPacketManager::ForceShaderHook(CShadowMap::UpdateShader);

	if (CPreviewManager::GetInstance()->IsEnabled())
		CRenderer::DrawPackets(e_RenderType_Preview, CMaterial::e_Deferred | CMaterial::e_Mixed | CMaterial::e_Forward);
	else
		CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred | CMaterial::e_Forward | CMaterial::e_Mixed);

	CPacketManager::ForceShaderHook(NULL);

	CRenderer::GetCurrentCamera()->SetViewProjMatrix(ViewProj_old);
}


int CShadowMap::UpdateShader(Packet* packet, void* p_pShaderData)
{
	if (packet->m_nCurrentPass > 0)
		return -1;

	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;
	CMaterial* pMaterial = packet->m_pMaterial;

	ProgramHandle nPID;

	bool bIsInstanced = pShaderData->m_nInstancedBufferID != 0;

	if (pMaterial->m_bUseTessellation && pMaterial->m_nInfoMapID != INVALID_HANDLE)
	{
		nPID = gs_nTessShadowPID;
		CShader::BindProgram(nPID);

		CTexture::SetTexture(nPID, pMaterial->m_nInfoMapID, 0);

		SShadowTessConstants tregisters;
		float fov = CRenderer::GetCurrentCamera()->GetFOV();

		memcpy(tregisters.m_Eye, CRenderer::GetCurrentCamera()->GetPosition().v(), 3 * sizeof(float));
		tregisters.m_TanFOV = powf(tanf(fov * 3.141592f / 360.f), 2.f);

		CDeviceManager::FillConstantBuffer(gs_ShadowTessConstantBuffer, &tregisters, sizeof(tregisters));
		CDeviceManager::BindConstantBuffer(gs_ShadowTessConstantBuffer, nPID, 0);
	}

	else if (pMaterial->GetRenderType() != CMaterial::e_Deferred && pMaterial->m_nDiffuseMapID != INVALID_HANDLE)
	{
		nPID = bIsInstanced ? gs_nInstancedAlphaShadowPID : gs_nAlphaShadowPID;
		CShader::BindProgram(nPID);

		CTexture::SetTexture(nPID, pMaterial->m_nDiffuseMapID, 0);
	}

	else
	{
		nPID = bIsInstanced ? gs_nInstancedShadowPID : gs_nShadowPID;
		CShader::BindProgram(nPID);
	}
		

	float4x4 WorldViewProj = pShaderData->World;
	WorldViewProj.transpose();
	WorldViewProj = pShaderData->WorldViewProj * WorldViewProj;

	GLuint uniform = glGetUniformLocation(nPID, "WorldViewProj");
	glUniformMatrix4fv(uniform, 1, GL_TRUE, WorldViewProj.m());

	CRenderStates::SetCullMode(e_CullBackCW);
	CRenderStates::SetWriteMask(e_Depth);
	CRenderStates::SetDepthStencil(e_LessEqual, false);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetRasterizerState(e_Solid);

	return 1;
}
