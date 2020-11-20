#ifndef __IMGUI_H__
#define __IMGUI_H__

#include "Engine/Renderer/Packets/Packet.h"

class CImGui_Impl
{
public:

	static void Init();
	static void Terminate();

	static void Draw();
	static void DrawPackets();
	static void NewFrame();

	static int UpdateShader(Packet* packet, void* pData);
};


#endif