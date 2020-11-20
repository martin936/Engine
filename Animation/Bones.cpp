#include "Engine/Engine.h"
#include "Bones.h"


std::vector<CBone*> CBone::ms_pBonesPalette;


CBone::CBone()
{
	m_nID = (unsigned int)ms_pBonesPalette.size();

	m_pParent = NULL;
	m_pChild.clear();

	ms_pBonesPalette.push_back(this);
}


CBone::CBone(CBone* pParent)
{
	m_nID = (unsigned int)ms_pBonesPalette.size();

	m_pParent = pParent;
	m_pChild.clear();

	ms_pBonesPalette.push_back(this);

	pParent->m_pChild.push_back(this);
}


void CBone::ClearAll()
{
	std::vector<CBone*>::iterator it;

	for (it = ms_pBonesPalette.begin(); it < ms_pBonesPalette.end(); it++)
	{
		delete (*it);
	}

	ms_pBonesPalette.clear();
}
