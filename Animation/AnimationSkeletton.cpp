#include "Engine/Engine.h"
#include "AnimationSkeletton.h"




CSkeletton::CSkeletton()
{
	m_pBoneAnims.clear();
	m_pAnimations.clear();
	m_pBones.clear();

	m_bLoaded = false;
}



CSkeletton::CSkeletton(const char* pcFileName)
{
	m_pBoneAnims.clear();
	m_pAnimations.clear();
	m_pBones.clear();

	m_bLoaded = false;


}


CSkeletton* CSkeletton::Load(const char* pcFileName)
{
	CSkeletton* pSkeletton = new CSkeletton(pcFileName);

	if (!pSkeletton->m_bLoaded)
	{
		delete pSkeletton;
		pSkeletton = NULL;
	}

	return pSkeletton;
}


CSkeletton::~CSkeletton()
{
	std::vector<SBoneAnim*>::iterator it1;
	std::vector<CAnimation*>::iterator it2;

	for (it1 = m_pBoneAnims.begin(); it1 < m_pBoneAnims.end(); it1++)
	{
		delete (*it1);
	}

	for (it2 = m_pAnimations.begin(); it2 < m_pAnimations.end(); it2++)
	{
		delete (*it2);
	}

	m_pBoneAnims.clear();
	m_pAnimations.clear();
}