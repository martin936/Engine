#ifndef __ANIMATION_BANK_H__
#define __ANIMATION_BANK_H__

#include "AnimationKeys.h"

class CAnimation
{
	friend class CInstance;

public:

	CAnimation(std::vector<CAnimation*>& AnimBank);

	inline float	GetDuration() const
	{
		return m_fDuration;
	}

	inline unsigned int GetID() const
	{
		return m_nID;
	}

	inline int GetChannelCount() const
	{
		return (int)m_pChannels.size();
	}

private:

	unsigned int	m_nID;

	float	m_fDuration;
	float	m_fTime;

	std::vector<SBoneAnim*> m_pChannels;
};


#endif
