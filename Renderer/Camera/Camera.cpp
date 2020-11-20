#include "Engine/Engine.h"
#include "Engine/Maths/Maths.h"
#include "Camera.h"
#include "Engine/Inputs/Inputs.h"



const float gs_fHalton[16 * 2] =
{
	0.5f, 0.33333f,
	0.25f, 0.666667f,
	0.75f, 0.111111f,
	0.125f, 0.444444f,
	0.625f, 0.7777778f,
	0.375f, 0.222222f,
	0.875f, 0.555556f,
	0.0625f, 0.888889f,
	0.5625f, 0.037037f,
	0.3125f, 0.370370f,
	0.8125f, 0.703704f,
	0.1875f, 0.148148f,
	0.6875f, 0.481481f,
	0.4375f, 0.814815f,
	0.9375f, 0.259259f,
	0.03125f, 0.592593f
};



int CCamera::ms_nCurrentJitterIndex = 0;



CCamera::CCamera()
{
	m_Proj.Eye();
	m_View.Eye();
	m_ViewProj.Eye();
	m_InvViewProj.Eye();

	m_fFOV = 0.f;
	m_vPos = 0.f;
	m_vUp = 0.f;
	m_vRight = 0.f;
	m_vDir = 0.f;
	
	m_fFarPlane = 0.f;
	m_fNearPlane = 0.f;
}


CCamera::~CCamera()
{
}


void CCamera::SetCamera(float fov, float fAspectRatio, float nearPlane, float farPlane, const float3& Up, const float3& Pos, const float3& Target)
{
	m_Proj.Eye();
	m_View.Eye();

	m_vPos = Pos;
	m_vUp = Up;
	m_vDir = Target - Pos;

	m_vDir.normalize();

	m_fNearPlane = nearPlane;
	m_fFarPlane = farPlane;
	m_fFOV = fov;
	m_fAspectRatio = fAspectRatio;

	ComputeViewMatrix();

	ComputeProjectionMatrix();

	m_ViewProj = m_Proj * m_View;
	JitterViewProj(m_ViewProj, ms_nCurrentJitterIndex);

	m_LastViewProj = m_ViewProj;
	m_InvViewProj = inverse(m_ViewProj);

	//CMouse::GetCurrent()->SetPos(0.5f, 0.5f);
}


void CCamera::ComputeProjectionMatrix()
{
	m_Proj.m00 = 1.f / tanf(3.141592f*m_fFOV / 360.f);
	m_Proj.m11 = m_fAspectRatio * m_Proj.m00;
	m_Proj.m22 = 0.5f - 0.5f * (m_fFarPlane + m_fNearPlane) / (m_fFarPlane - m_fNearPlane);
	m_Proj.m32 = 1.f;
	m_Proj.m23 = m_fFarPlane*m_fNearPlane / (m_fFarPlane - m_fNearPlane);
	m_Proj.m33 = 0.f;
}


void CCamera::ComputeViewMatrix()
{
	m_vUp = m_vUp - float3::dotproduct(m_vUp, m_vDir) * m_vDir;
	m_vUp.normalize();

	m_vRight = float3::cross(m_vDir, m_vUp);

	memcpy(m_View.m(), m_vRight.v(), 3 * sizeof(float));
	memcpy(m_View.m() + 4, m_vUp.v(), 3 * sizeof(float));
	memcpy(m_View.m() + 8, m_vDir.v(), 3 * sizeof(float));

	m_View.m()[3] = -float3::dotproduct(m_vRight, m_vPos);
	m_View.m()[7] = -float3::dotproduct(m_vUp, m_vPos);
	m_View.m()[11] = -float3::dotproduct(m_vDir, m_vPos);
}


void CCamera::JitterViewProj(float4x4& ViewProj, int k)
{
	return;

	/*float4x4 JitterMat;
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	JitterMat.Eye();

	JitterMat.m03 = 2.f * gs_fHalton[2 * k] / nWidth;
	JitterMat.m13 = 2.f * gs_fHalton[2 * k + 1] / nHeight;

	ViewProj = JitterMat * ViewProj;*/
}