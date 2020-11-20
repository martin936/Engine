#include "Engine/Engine.h"
#include "Engine/Maths/Maths.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "Menu.h"


bool CMenuItem::ms_bActionTaken = false;

bool (*CMenuItem::ms_pIsActionRequired)(int nAction) = nullptr;


CMenuItem::CMenuItem(CMenuContainer* pParent, bool bSelectable)
{
	m_fPosition[0] = m_fPosition[1] = 0.f;
	m_fSize[0] = m_fSize[1] = 0.f;

	m_pParent = pParent;

	m_bIsSelected = false;
	m_bSelectable = bSelectable;
	m_bIsHeightFixed = false;
	m_bIsWidthFixed = false;

	m_bShouldUpdate = true;
	m_bIsCentered = false;

	if (pParent != nullptr)
	{
		pParent->AddChild(this);
		m_nHierarchyLevel = pParent->GetHierarchyLevel() + 1;
	}

	else
		m_nHierarchyLevel = 0;
}


CMenuItem::~CMenuItem()
{

}


void CMenuItem::Process()
{
	if (m_pParent == nullptr)
		ms_bActionTaken = false;

	if (IsSelected() && ms_pIsActionRequired != nullptr && !ms_bActionTaken)
	{
		std::vector<SMenuLink>::iterator it;

		for (it = m_pLinkedItems.begin(); it < m_pLinkedItems.end(); it++)
		{
			if (ms_pIsActionRequired((*it).m_nAction))
			{
				ms_bActionTaken = true;
				(*it).m_pItem->Select(this);
				break;
			}
		}
	}

	if (!m_bShouldUpdate)
		return;

	if (m_pParent != nullptr)
	{
		for (int i = 0; i < 2; i++)
		{
			if ((!m_bIsWidthFixed && i == 0) || (!m_bIsHeightFixed && i == 1))
				m_fSize[i] = m_pParent->m_fSize[i] * m_fRelativeSize[i];

			if (m_bIsCentered)
				m_fPosition[i] = m_pParent->m_fPosition[i] + m_pParent->m_fSize[i] * m_fRelativePosition[i];

			else
				m_fPosition[i] = m_pParent->m_fPosition[i] + m_pParent->m_fSize[i] * (m_fRelativePosition[i] - 0.5f) + 0.5f * m_fSize[i];
		}
	}

	else
	{
		for (int i = 0; i < 2; i++)
		{
			if ((!m_bIsWidthFixed && i == 0) || (!m_bIsHeightFixed && i == 1))
				m_fSize[i] = m_fRelativeSize[i];

			if (m_bIsCentered)
				m_fPosition[i] = 0.5f + m_fRelativePosition[i];

			else
				m_fPosition[i] = m_fRelativePosition[i] + 0.5f * m_fSize[i];
		}
	}

	m_bShouldUpdate = false;
}


void CMenuItem::Select(CMenuItem* pCaller)
{
	if (pCaller == nullptr || m_pParent == pCaller)
		m_bIsSelected = true;

	else
	{
		CMenuItem* pAncestors[2];

		pAncestors[0] = this;
		pAncestors[1] = pCaller;

		while (pAncestors[0]->m_nHierarchyLevel != pAncestors[1]->m_nHierarchyLevel)
		{
			if (pAncestors[0]->m_nHierarchyLevel > pAncestors[1]->m_nHierarchyLevel)
				pAncestors[0] = pAncestors[0]->m_pParent;

			else
				pAncestors[1] = pAncestors[1]->m_pParent;
		}

		while (pAncestors[0] != pAncestors[1])
		{
			pAncestors[0]->m_bIsSelected = true;
			pAncestors[1]->m_bIsSelected = false;

			pAncestors[0] = pAncestors[0]->m_pParent;
			pAncestors[1] = pAncestors[1]->m_pParent;
		}
	}
}


bool CMenuItem::IsSelected() const
{
	if (!m_bIsSelected || !m_bSelectable)
		return false;

	if (m_pParent == nullptr)
		return true;

	return m_pParent->IsSelected();
}


CMenuContainer::CMenuContainer(CMenuContainer* pParent) : CMenuItem(pParent, true)
{

}


CMenuContainer::~CMenuContainer()
{
	std::vector<CMenuItem*>::iterator it;

	for (it = m_pChildren.begin(); it < m_pChildren.end(); it++)
		delete (*it);
}


void CMenuContainer::AddChild(CMenuItem* pChild)
{
	m_pChildren.push_back(pChild);
}


void CMenuContainer::Draw()
{
	std::vector<CMenuItem*>::iterator it;

	for (it = m_pChildren.begin(); it < m_pChildren.end(); it++)
		(*it)->Draw();
}


void CMenuContainer::Process()
{
	CMenuItem::Process();

	std::vector<CMenuItem*>::iterator it;

	bool bIsAChildrenSelected = false;

	for (it = m_pChildren.begin(); it < m_pChildren.end(); it++)
	{
		if ((*it)->IsSelected())
			bIsAChildrenSelected = true;
	}

	if (m_bIsSelected && !bIsAChildrenSelected)
		for (it = m_pChildren.begin(); it < m_pChildren.end(); it++)
		{
			if ((*it)->IsSelectable())
			{
				(*it)->Select(this);
				break;
			}
		}

	for (it = m_pChildren.begin(); it < m_pChildren.end(); it++)
		(*it)->Process();
}


CMenuTile::CMenuTile(CMenuContainer* pParent) : CMenuContainer(pParent)
{
	m_Color = 1.f;
	m_nActiveState = 0;
}


CMenuTile::~CMenuTile()
{
	
}


void CMenuTile::AddChild(CMenuItem* pChild)
{
	m_pChildren.push_back(pChild);
	m_pChildrenState.push_back(m_nActiveState);
}


void CMenuTile::Process()
{
	CMenuItem::Process();

	size_t nNbChildren = m_pChildren.size();

	bool bIsAChildrenSelected = false;

	for (size_t i = 0; i < nNbChildren; i++)
	{
		if (m_pChildrenState[i] == m_nActiveState && m_pChildren[i]->IsSelected())
			bIsAChildrenSelected = true;
	}

	if (m_bIsSelected && !bIsAChildrenSelected)
		for (size_t i = 0; i < nNbChildren; i++)
		{
			if (m_pChildrenState[i] == m_nActiveState && m_pChildren[i]->IsSelectable())
			{
				m_pChildren[i]->Select(this);
				break;
			}
		}

	for (size_t i = 0; i < nNbChildren; i++)
	{
		if (m_pChildrenState[i] == m_nActiveState)
			m_pChildren[i]->Process();
	}
}


void CMenuTile::Draw()
{
	float4 color = m_Color;

	if (IsSelected())
		color *= 0.6f;

	CSpriteEngine::AddCenteredSprite2D(m_fPosition, m_fSize, color);

	size_t nNbChildren = m_pChildren.size();

	for (size_t i = 0; i < nNbChildren; i++)
	{
		if (m_pChildrenState[i] == m_nActiveState)
			m_pChildren[i]->Draw();
	}
}
