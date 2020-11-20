#include "Engine/Engine.h"
#include "Engine/Maths/Maths.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Renderer/Sprites/Sprites.h"

#include "Menu.h"



int CMenuButton::ms_nOKButton	= 0;
int CMenuButton::ms_nBackButton = 0;


void EmptyCallback(CMenuButton* pButton, void* pData)
{

}



CMenuButton::CMenuButton(CMenuContainer* pParent, const char* pText) : CMenuContainer(pParent)
{
	m_pLabel = new CMenuLabel(this, pText);

	m_pLabel->SetRelativePosition(0.f, 0.f);
	m_pLabel->SetRelativeWidth(0.8f);
	m_pLabel->SetRelativeHeight(0.8f);
	m_pLabel->SetColor(float4(0.f, 0.f, 0.f, 1.f));
	m_pLabel->SetCentered(true);

	m_bIsClicked	= false;
	m_bIsHovered	= false;
	m_bIsBackButton = false;

	m_Color = 1.f;
	m_BaseColor = 1.f;
	m_PressedColor = 0.5f;
	m_HoverColor = 0.8f;

	for (int i = e_First; i < e_Last; i++)
	{
		m_pCallback[i] = EmptyCallback;
	}
}


CMenuButton::~CMenuButton()
{
	if (m_pLabel != nullptr)
		delete m_pLabel;
}


void CMenuButton::Process()
{
	CMenuItem::Process();

	m_pLabel->Process();

	EMenuButtonEventType eEvent;

	if (GetEvent(&eEvent))
	{
		if (eEvent == e_OnPress)
		{
			m_bIsClicked = true;
			m_Color = m_PressedColor;
		}

		else if (eEvent == e_OnRelease)
		{
			m_bIsClicked = false;
			m_Color = m_BaseColor;
		}

		else if (eEvent == e_OnHoverIn)
		{
			m_bIsHovered = true;
			m_Color = m_HoverColor;
		}

		else if (eEvent == e_OnHoverOut)
		{
			m_bIsHovered = false;
			m_Color = m_BaseColor;
		}

		m_pCallback[eEvent](this, m_pCallbackData[eEvent]);
	}
}


bool CMenuButton::GetMouseEvent(EMenuButtonEventType* eEvent)
{
	float fMousePos[2];
	float fRelativePos[2];

	CMouse::GetCurrent()->GetPos(&fMousePos[0], &fMousePos[1]);

	bool bIsMousePressed = CMouse::GetCurrent()->IsPressed(CMouse::e_Button_LeftClick);

	for (int i = 0; i < 2; i++)
	{
		fRelativePos[i] = fMousePos[i] - (m_fPosition[i] - 0.5f * m_fSize[i]);
	}

	if (fRelativePos[0] >= 0.f && fRelativePos[0] < m_fSize[0] && fRelativePos[1] >= 0.f && fRelativePos[1] < m_fSize[1])
	{
		if (bIsMousePressed)
		{
			if (!m_bIsClicked)
			{
				*eEvent = CMenuButton::e_OnPress;
				return true;
			}
		}

		else if (m_bIsClicked)
		{
			*eEvent = CMenuButton::e_OnRelease;
			return true;
		}

		else if (!m_bIsHovered)
		{
			*eEvent = CMenuButton::e_OnHoverIn;
			return true;
		}

	}

	else
	{
		if (m_bIsClicked && !bIsMousePressed)
		{
			*eEvent = CMenuButton::e_OnRelease;
			return true;
		}

		if (m_bIsHovered)
		{
			*eEvent = CMenuButton::e_OnHoverOut;
			return true;
		}
	}

	return false;
}


bool CMenuButton::GetJoystickEvent(EMenuButtonEventType* eEvent)
{
	bool selected = IsSelected();

	bool click = (selected && ms_pIsActionRequired(ms_nOKButton) || (m_bIsBackButton && ms_pIsActionRequired(ms_nBackButton)));

	if (!ms_bActionTaken && click)
	{
		ms_bActionTaken = true;
		*eEvent = e_OnPress;
		return true;
	}

	else if (!click && m_bIsClicked)
	{
		*eEvent = e_OnRelease;
		true;
	}

	return false;
}


bool CMenuButton::GetEvent(EMenuButtonEventType* eEvent)
{
	return GetMouseEvent(eEvent) || GetJoystickEvent(eEvent);
}


void CMenuButton::Draw()
{
	float4 color = m_Color;

	if (IsSelected())
		color = float4(0.8f, 1.f, 0.8f, 1.f);

	CSpriteEngine::AddCenteredSprite2D(m_fPosition, m_fSize, color);

	m_pLabel->Draw();
}

