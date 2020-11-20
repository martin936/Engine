#ifndef __OIT_H__
#define __OIT_H__

#include "Engine/Renderer/Packets/Packet.h"


class COIT
{
public:

	static void Init();

	static int UpdateShader(Packet* packet, void* pShaderData);
	static int WriteDepth_UpdateShader(Packet* packet, void* pShaderData);

private:

	static CTexture* ms_pAOITColorNodes;
	static CTexture* ms_pAOITDepthNodes;
	static CTexture* ms_pAOITClearMask;
};


#endif
