#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Inputs/Inputs.h"
#include "Camera.h"


CFreeCamera::CFreeCamera() : CCamera()
{

}


CFreeCamera::~CFreeCamera()
{

}


void CFreeCamera::Update()
{
	float fPosX, fPosY;
	float fLastPosX, fLastPosY;
	static bool bFreeze = true;
	static bool bUpdateFreeze = true;
	float3 Move = float3(0.f, 0.f, 0.f);

	CKeyboard* pKeyboard = CKeyboard::GetCurrent();
	CMouse* pMouse = CMouse::GetCurrent();

	pMouse->GetPos(&fPosX, &fPosY);
	pMouse->GetLastPos(&fLastPosX, &fLastPosY);

	if (pKeyboard->IsPressed(CKeyboard::e_Key_Space) && bUpdateFreeze)
	{
		bFreeze = !bFreeze;
		bUpdateFreeze = false;
	}

	if (pKeyboard->IsReleased(CKeyboard::e_Key_Space))
		bUpdateFreeze = true;

	if (bFreeze)
		return;

	m_vDir += 5.f * (fLastPosX - fPosX) * m_vRight + 5.f * (fPosY - fLastPosY) * m_vUp;
	m_vDir.normalize();

	m_vUp = float3(0.f, 0.f, 1.f);

	if (pKeyboard->IsPressed(CKeyboard::e_Key_W))
		Move += m_vDir;

	if (pKeyboard->IsPressed(CKeyboard::e_Key_S))
		Move -= m_vDir;

	if (pKeyboard->IsPressed(CKeyboard::e_Key_A))
		Move -= m_vRight;

	if (pKeyboard->IsPressed(CKeyboard::e_Key_D))
		Move += m_vRight;

	if (Move.length() > 1e-3f)
		Move.normalize();

	m_vPos += 5.f * CEngine::GetFrameDuration() * Move;

	ComputeViewMatrix();

	m_LastViewProj = m_ViewProj;
	m_ViewProj = m_Proj * m_View;
	//JitterViewProj(m_ViewProj, ms_nCurrentJitterIndex);

	m_InvViewProj = inverse(m_ViewProj);
}
