#ifndef __MENU_H__
#define __MENU_H__

#include "Engine/Maths/Maths.h"

class CMenuContainer;


class CMenuItem
{
public:

	CMenuItem(CMenuContainer* pParent, bool bSelectable);
	~CMenuItem();

	virtual void Draw() = 0;
	virtual void Select(CMenuItem* pCaller = nullptr);
	virtual void Process();

	inline float GetWidth() const
	{
		return m_fSize[0];
	}

	inline float GetHeight() const
	{
		return m_fSize[1];
	}

	inline void SetRelativeWidth(float width)
	{
		m_fRelativeSize[0] = width;
		m_bIsWidthFixed = false;
		m_bShouldUpdate = true;
	}

	inline void SetRelativeHeight(float height)
	{
		m_fRelativeSize[1] = height;
		m_bIsWidthFixed = false;
		m_bShouldUpdate = true;
	}

	inline void SetFixedWidth(float width)
	{
		m_fSize[0] = width;
		m_bIsWidthFixed = true;
		m_bShouldUpdate = true;
	}

	inline void SetFixedHeight(float height)
	{
		m_fSize[1] = height;
		m_bIsHeightFixed = true;
		m_bShouldUpdate = true;
	}

	inline void GetRelativePosition(float* x, float* y) const
	{
		*x = m_fRelativePosition[0];
		*y = m_fRelativePosition[1];
	}

	inline void SetRelativePosition(float x, float y)
	{
		m_fRelativePosition[0] = x;
		m_fRelativePosition[1] = y;
		m_bShouldUpdate = true;
	}

	inline void GetAbsolutePosition(float* x, float* y) const
	{
		*x = m_fPosition[0];
		*y = m_fPosition[1];
	}

	inline void SetAbsolutePosition(float x, float y)
	{
		m_fPosition[0] = x;
		m_fPosition[1] = y;
		m_bShouldUpdate = true;
	}

	inline int GetHierarchyLevel() const
	{
		return m_nHierarchyLevel;
	}

	inline bool IsSelectable() const
	{
		return m_bSelectable;
	}

	inline bool IsCentered() const
	{
		return m_bIsCentered;
	}

	bool IsSelected() const;

	inline void SetCentered(bool bCentered)
	{
		m_bIsCentered = bCentered;
	}
	
	inline void LinkItem(CMenuItem* pItem, int nAction)
	{
		m_pLinkedItems.push_back({pItem, nAction});
	}

	static inline void SetInputFetchingCallback(bool(*pIsActionRequired)(int nAction))
	{
		ms_pIsActionRequired = pIsActionRequired;
	}

protected:

	float		m_fSize[2];
	float		m_fRelativeSize[2];

	float		m_fRelativePosition[2];	
	float		m_fPosition[2];

	bool		m_bSelectable;
	bool		m_bIsSelected;
	bool		m_bIsWidthFixed;
	bool		m_bIsHeightFixed;

	bool		m_bIsCentered;
	bool		m_bShouldUpdate;

	int			m_nHierarchyLevel;

	struct SMenuLink
	{
		CMenuItem*	m_pItem;
		int			m_nAction;
	};

	static bool(*ms_pIsActionRequired)(int nAction);

	static bool	ms_bActionTaken;

	CMenuContainer*	m_pParent;

	std::vector<SMenuLink> m_pLinkedItems;
};



class CMenuContainer : public CMenuItem
{
	friend CMenuItem;

public:

	CMenuContainer(CMenuContainer* pParent);
	~CMenuContainer();

	virtual void Draw() override;
	virtual void Process() override;

	virtual CMenuItem* GetChild(unsigned int i)
	{
		if (i < m_pChildren.size())
			return m_pChildren[i];

		return NULL;
	}

	virtual void AddChild(CMenuItem* pChild);

protected:

	std::vector<CMenuItem*>	m_pChildren;
};



class CMenuTile	: public CMenuContainer
{
public:

	CMenuTile(CMenuContainer* pParent);
	~CMenuTile();

	virtual void Draw() override;

	virtual void Process() override;

	virtual void AddChild(CMenuItem* pChild) override;

	inline void SetColor(float4& color)
	{
		m_Color = color;
	}

	inline void SetActiveState(int nState)
	{
		m_nActiveState = nState;
	}

protected:

	float4		m_Color;
	int			m_nActiveState;

	std::vector<int> m_pChildrenState;
};


#include "MenuLabel.h"
#include "MenuButton.h"


#endif
