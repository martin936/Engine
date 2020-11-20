#include "Engine/Engine.h"
#include "AnimationBank.h"


CAnimation::CAnimation(std::vector<CAnimation*>& AnimBank)
{
	m_nID = (unsigned int)AnimBank.size();

	m_fDuration = 0.f;
	m_pChannels.clear();

	AnimBank.push_back(this);
}
