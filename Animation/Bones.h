#ifndef __BONES_H__
#define __BONES_H__

#include <vector>
#include "Engine/Maths/Maths.h"

class CBone
{

public:

	CBone();
	CBone(CBone* pParent);

	inline void SetTransform(DualQuaternion& H)
	{
		m_LocalTransform = H;
	}

	inline DualQuaternion GetTransform() const
	{
		return m_LocalTransform;
	}

	inline unsigned int GetID() const
	{
		return m_nID;
	}

	inline static CBone* GetBone(unsigned int nID)
	{
		return ms_pBonesPalette[nID];
	}

	inline CBone* GetParent() const
	{
		return m_pParent;
	}

	inline int GetChildrenCount() const
	{
		return (int)m_pChild.size();
	}

	inline CBone* GetChild(int i) const
	{
		return m_pChild[i];
	}

	inline static int GetBonesCount()
	{
		return (int)ms_pBonesPalette.size();
	}

	inline void ComputeGlobalTransform()
	{
		if (m_pParent != NULL)
			m_GlobalTransform = m_LocalTransform * m_pParent->m_GlobalTransform;

		else
			m_GlobalTransform = m_LocalTransform;
	}

	static void ClearAll();

private:

	unsigned int	m_nID;
	
	CBone*	m_pParent;
	std::vector<CBone*> m_pChild;

	DualQuaternion	m_LocalTransform;
	DualQuaternion	m_GlobalTransform;

	static std::vector<CBone*> ms_pBonesPalette;
};


#endif
