#ifndef __FORWARD_H__
#define __FORWARD_H__

#include "Engine/Renderer/OIT/OIT.h"

class CForwardRenderer
{
	friend class COIT;

public:

	static void Init();

	static void DrawForward();

private:

};


#endif
