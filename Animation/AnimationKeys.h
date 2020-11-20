#ifndef __ANIMATIONKEYS_H__
#define __ANIMATIONKEYS_H__


#include "Engine/Maths/Maths.h"
#include <vector>

struct SVectorKey
{
	float	m_fTime;
	float3	m_Value;
};


struct SRotationKey
{
	float		m_fTime;
	Quaternion	m_Value;
};


enum EAnimBehaviour
{
	e_Behaviour_Default,
	e_Behaviour_Constant,
	e_Behaviour_Repeat
};


struct SBoneAnim
{
	unsigned int m_nBoneID;
	
	std::vector<SVectorKey>		m_PositionKeys;
	std::vector<SRotationKey>	m_RotationKeys;
	std::vector<SVectorKey>		m_ScalingKeys;

	EAnimBehaviour	m_ePreState;
	EAnimBehaviour	m_ePostState;

	void GetKeys(float3& pPosition, Quaternion& pRot, float3& pScale, float fTime)
	{
		std::vector<SVectorKey>::iterator it;
		int nIndex = 0;
		float fLerp = 0.f;

		for (it = m_PositionKeys.begin(); it < m_PositionKeys.end(); it++)
		{
			if (it < m_PositionKeys.end() - 1 && (*it).m_fTime < fTime && (*(it+1)).m_fTime > fTime)
			{
				fLerp = (fTime - (*it).m_fTime) / ((*(it + 1)).m_fTime - (*it).m_fTime);
				break;
			}

			nIndex++;
		}

		if (nIndex < m_PositionKeys.size())
		{
			pPosition = (1.f - fLerp) * m_PositionKeys[nIndex].m_Value + fLerp * m_PositionKeys[nIndex + 1].m_Value;
			pRot = (1.f - fLerp) * m_RotationKeys[nIndex].m_Value + fLerp * m_RotationKeys[nIndex + 1].m_Value;
			pScale = (1.f - fLerp) * m_ScalingKeys[nIndex].m_Value + fLerp * m_ScalingKeys[nIndex + 1].m_Value;
		}

		else
		{
			pPosition = m_PositionKeys.back().m_Value;
			pRot = m_RotationKeys.back().m_Value;
			pScale = m_ScalingKeys.back().m_Value;
		}
	}
};


#endif
