#ifndef __TEXT_H__
#define __TEXT_H__

#include "Engine/Renderer/Textures/Textures.h"
#include "Engine/Renderer/Packets/Packet.h"

#define FONT_PATH(Name) "../Data/Fonts/" Name ".tga"


class CTextRenderer
{
public:

	static void InitFont(const char* path);

	static void DrawText2D(const char pText[], float x, float y, float fSize, float4& color = float4(0.f, 0.f, 0.f, 1.));
	static void DrawText2D(const char pText[], float x, float y, float fSizeX, float fSizeY, float4& color = float4(0.f, 0.f, 0.f, 1.));

	static void DrawCenteredText2D(const char pText[], float x, float y, float fSize, float4& color = float4(0.f, 0.f, 0.f, 1.));
	static void DrawCenteredText2D(const char pText[], float x, float y, float fSizeX, float fSizeY, float4& color = float4(0.f, 0.f, 0.f, 1.));

private:

	static bool ComputeTexCoords(char cLetter, float* pTexCoords);
	static CTexture* ms_pFontTex;
};

#endif
