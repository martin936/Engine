#ifndef __TOOLBOX_H__
#define __TOOLBOX_H__

#include "Engine/Editor/AxisPicker.h"
#include "CollisionBox.h"

class CToolBox
{
public:

	CToolBox();
	~CToolBox();

	void Update();
	void DrawAxisPicker();
	inline CCollisionBoxManager* GetCollisionBoxManager() const { return m_pCollisionBoxManager; }

	inline void ReloadCollisionBoxManager()
	{
		delete m_pCollisionBoxManager;
		m_pCollisionBoxManager = new CCollisionBoxManager;
	}

private:

	CCollisionBoxManager* m_pCollisionBoxManager;
};


#endif
