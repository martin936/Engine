#ifndef __MENU_LABEL_H__
#define __MENU_LABEL_H__


class CMenuLabel : public CMenuItem
{
public:

	CMenuLabel(CMenuContainer* pParent, const char* pText);
	~CMenuLabel() {}

	virtual void Draw() override;

	inline void SetText(const char* cText)
	{
		strncpy(m_cText, cText, 256);
	}

	inline void SetColor(float4& color)
	{
		m_Color = color;
	}

	inline float4 GetColor() const
	{
		return m_Color;
	}

	inline void SetJustified(bool bEnabled)
	{
		m_bJustified = bEnabled;
	}

protected:

	char	m_cText[256];

	float4	m_Color;
	bool	m_bJustified;
};


#endif
