#ifndef __ANIMATION_SKELETTON_H__
#define __ANIMATION_SKELETTON_H__

#include "AnimationBank.h"
#include "Bones.h"
#include <vector>

class CSkeletton
{
public:

	CSkeletton();
	CSkeletton(const char* pFileName);

	static CSkeletton* Load(const char* pFileName);

	~CSkeletton();

	inline CAnimation* GetAnim(unsigned int nID)
	{
		return m_pAnimations[nID];
	}

private:

	bool	m_bLoaded;

	std::vector<CBone*> m_pBones;

	std::vector<SBoneAnim*>	m_pBoneAnims;
	std::vector<CAnimation*> m_pAnimations;
};


#endif
