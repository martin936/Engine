#ifndef __DEBUGDRAW_H__
#define __DEBUGDRAW_H__


#include <vector>
#include "Engine/Maths/Maths.h"

class Packet;

class CDebugDraw
{
public:

	static void Init();

	static void DrawVector(float3& Origin, float3& Vector, float fLength, float4& Color);
	static void DrawCircle(float3& Origin, float3& Normal, float fLength, float4& Color);

	// 2D helpers. Positions are in clip space: x, y in [-1, 1].
	static void DrawLine2D(float2 P1, float2 P2, float4 Color);
	static void DrawVector2D(float2 Origin, float2 Vector, float fLength, float4 Color);
	static void DrawArrow2D(float2 Origin, float2 Vector, float fLength, float4 Color);
	static void DrawCircle2D(float2 Origin, float fRadius, float4 Color, int nSegments = 32);

	static int UpdateShader(Packet* packet, void* pShaderData);
};


#endif
