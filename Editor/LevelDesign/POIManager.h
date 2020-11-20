#ifndef __POI_MANAGER_H__
#define __POI_MANAGER_H__


#include "Gameplay/POI/POI.h"


class CPOIManager
{
public:

	static void AddPOI();

private:


	struct SPOIPicker
	{
		CPointOfInterest*	m_pPOI;
		CAxisPicker*		m_pAxisPicker;
	};

	static std::vector<SPOIPicker*> m_pPOIs;
};


#endif
