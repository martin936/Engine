#ifndef __DEBUGDRAW_H__
#define __DEBUGDRAW_H__


#include <vector>
#include "Engine/Maths/Maths.h"

class CDebugDraw
{
public:

	static void Init();

	static void DrawVector(float3& Origin, float3& Vector, float fLength, float4& Color);
	static void DrawCircle(float3& Origin, float3& Normal, float fLength, float4& Color);

	static int UpdateShader(Packet* packet, void* pShaderData);
};


#endif
