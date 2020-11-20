#ifndef __MENU_BUTTON_H__
#define __MENU_BUTTON_H__

#include "Menu.h"

class CMenuButton : public CMenuContainer
{
public:

	enum EMenuButtonEventType
	{
		e_First = 0,
		e_OnPress = e_First,
		e_OnRelease,
		e_OnHoverIn,
		e_OnHoverOut,
		e_Last
	};

	CMenuButton(CMenuContainer* pParent, const char* pText);
	~CMenuButton();


	virtual void Process() override;
	virtual void Draw() override;

	inline void SetBackButton()
	{
		m_bIsBackButton = true;
	}

	inline void SetCallback(EMenuButtonEventType eEventType, void(*pCallback)(CMenuButton* pButton, void* pData), void* pData)
	{
		m_pCallback[eEventType] = pCallback;
		m_pCallbackData[eEventType] = pData;
	}


	inline void SetColor(float4& color)
	{
		m_BaseColor = color;
	}


	inline void SetCurrentColor(float4& color)
	{
		m_Color = color;
	}

	
	inline void SetHoverColor(float4& color)
	{
		m_HoverColor = color;
	}


	inline void SetPressedColor(float4& color)
	{
		m_PressedColor = color;
	}


	inline float4 GetColor() const
	{
		return m_Color;
	}

	inline void SetJustified(bool bEnable)
	{
		m_pLabel->SetJustified(bEnable);
	}

	static inline void SetOKButton(int nKey)
	{
		ms_nOKButton = nKey;
	}

	static inline void SetBackButton(int nKey)
	{
		ms_nBackButton = nKey;
	}

protected:

	bool GetEvent(EMenuButtonEventType* eEvent);
	bool GetMouseEvent(EMenuButtonEventType* eEvent);
	bool GetJoystickEvent(EMenuButtonEventType* eEvent);

	void(*m_pCallback[e_Last])(CMenuButton* pButton, void* pData);
	void*		m_pCallbackData[e_Last];

	CMenuLabel* m_pLabel;

	float4		m_BaseColor;
	float4		m_HoverColor;
	float4		m_PressedColor;
	float4		m_Color;

	bool		m_bIsClicked;
	bool		m_bIsHovered;
	bool		m_bIsBackButton;

	static int	ms_nOKButton;
	static int	ms_nBackButton;
};


#endif
