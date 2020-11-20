#ifndef __MATERIAL_ASSOCIATIONS_H__
#define __MATERIAL_ASSOCIATIONS_H__

#include "Engine/Renderer/Packets/Packet.h"

struct SMaterialAssociation
{
	char	m_cName[256];
	int		(*m_DeferredShaderHook)(Packet* packet, void* p_pShaderData);
	int		(*m_ForwardShaderHook)(Packet* packet, void* p_pShaderData);
};



#endif
