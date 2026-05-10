#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Renderer/Sprites/TexturedSpriteRenderer.h"
#include "Engine/Misc/FileSystem.h"
#include "Text.h"

// Pull stb_truetype's implementation as static into this TU. ImGui's
// imgui_draw.cpp does the same with its own private copy, so the symbols
// don't collide at link time.
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "Engine/Imgui/imstb_truetype.h"


namespace
{
	const int   kFontPxSize  = 256;	// Bake size; on-screen size is decoupled via fSizeY.
	const int   kAtlasW      = 2048;
	const int   kAtlasH      = 2048;
	const int   kFirstChar   = 32;	// printable ASCII
	const int   kNumChars    = 95;	// 32..126

	stbtt_packedchar gs_PackedChars[kNumChars] = {};
	bool gs_bGlyphsBaked = false;
}


CTexture* CTextRenderer::ms_pFontTex = NULL;


void CTextRenderer::InitFont(const char* path)
{
	if (ms_pFontTex != NULL)
		return;

	FILE* pFile = NULL;
	FileSystem::FOpenS(&pFile, path, "rb");
	if (pFile == NULL)
		return;

	fseek(pFile, 0, SEEK_END);
	long nFileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	if (nFileSize <= 0)
	{
		fclose(pFile);
		return;
	}

	unsigned char* pTtfData = new unsigned char[nFileSize];
	size_t nRead = fread(pTtfData, 1, nFileSize, pFile);
	fclose(pFile);

	if (nRead != (size_t)nFileSize)
	{
		delete[] pTtfData;
		return;
	}

	unsigned char* pAtlas = new unsigned char[kAtlasW * kAtlasH];
	memset(pAtlas, 0, kAtlasW * kAtlasH);

	stbtt_pack_context spc;
	if (stbtt_PackBegin(&spc, pAtlas, kAtlasW, kAtlasH, 0, 1, NULL))
	{
		// 1x oversample keeps the bake fast and the atlas crisp at the chosen
		// 64px size. If text ever needs to scale up significantly, bump to
		// (2,2) and grow the atlas (4x area).
		stbtt_PackSetOversampling(&spc, 1, 1);
		stbtt_PackFontRange(&spc, pTtfData, 0, (float)kFontPxSize, kFirstChar, kNumChars, gs_PackedChars);
		stbtt_PackEnd(&spc);

		gs_bGlyphsBaked = true;
	}

	delete[] pTtfData;

	ms_pFontTex = new CTexture(kAtlasW, kAtlasH, 1, ETextureFormat::e_R8, eTexture2D, pAtlas);

	delete[] pAtlas;
}


void CTextRenderer::DrawText2D(const char pText[], float x, float y, float fSizeX, float fSizeY, float4 color)
{
	if (ms_pFontTex == NULL || !gs_bGlyphsBaked)
		return;

	const int nCharCount = (int)strlen(pText);
	if (nCharCount <= 0)
		return;

	// Stack-buffer the entire string's geometry so the whole label ships as a
	// single packet. 256 chars/call is plenty for HUD labels.
	const int kMaxChars = 256;
	const int nClamped = nCharCount < kMaxChars ? nCharCount : kMaxChars;

	float3 positions[kMaxChars * 4];
	float2 texcoords[kMaxChars * 4];

	int nQuadCount = 0;

	// (x, y) is the top-left of the text in [0,1] window space.
	const float fX = x * 2.f - 1.f;
	const float fY = 1.f - y * 2.f;

	const float fAspect = CWindow::GetMainWindow()->GetAspectRatio();

	// Pixels (font-space) -> clip space. Y uses fSizeY directly; X uses the
	// same per-pixel scale corrected by the window aspect, so glyphs stay
	// visually round regardless of fSizeX. The fSizeX-wide column is only used
	// for cursor advance, not glyph stretching.
	const float fPxToClipY = fSizeY / (float)kFontPxSize;
	const float fPxToClipX = fPxToClipY / fAspect;

	// Vertical placement: the baseline sits in the lower portion of the cell
	// so ascenders fit within fSizeY and descenders hang slightly below.
	const float fCellTop      = fY;
	const float fBaselineClip = fCellTop - 0.8f * fSizeY;

	float fCellLeft = fX;

	for (int i = 0; i < nClamped; i++)
	{
		const char ch = pText[i];

		if (ch != ' ' && ch >= kFirstChar && ch < kFirstChar + kNumChars)
		{
			const stbtt_packedchar& pc = gs_PackedChars[ch - kFirstChar];

			const float fGlyphWidthPx  = pc.xoff2 - pc.xoff;
			const float fGlyphHeightPx = pc.yoff2 - pc.yoff;

			const float fGlyphWidthClip  = fGlyphWidthPx  * fPxToClipX;
			const float fGlyphHeightClip = fGlyphHeightPx * fPxToClipY;

			// Centre the glyph rect horizontally in the fSizeX-wide column.
			const float fLeft  = fCellLeft + 0.5f * fSizeX - 0.5f * fGlyphWidthClip;
			const float fRight = fLeft + fGlyphWidthClip;

			// Vertical: baseline-relative. stbtt yoff is negative for ascenders
			// and yoff2 positive for descenders (Y down in pixel space). In
			// clip space Y is up, so we negate to place top above baseline and
			// bottom below.
			const float fTop    = fBaselineClip + (-pc.yoff)  * fPxToClipY;
			const float fBottom = fTop - fGlyphHeightClip;

			float3* pos = positions + 4 * nQuadCount;
			pos[0] = float3(fLeft,  fBottom, 0.f);	// BL
			pos[1] = float3(fLeft,  fTop,    0.f);	// TL
			pos[2] = float3(fRight, fBottom, 0.f);	// BR
			pos[3] = float3(fRight, fTop,    0.f);	// TR

			const float u0 = (float)pc.x0 / (float)kAtlasW;
			const float v0 = (float)pc.y0 / (float)kAtlasH;
			const float u1 = (float)pc.x1 / (float)kAtlasW;
			const float v1 = (float)pc.y1 / (float)kAtlasH;

			float2* uv = texcoords + 4 * nQuadCount;
			uv[0] = float2(u0, v1);	// BL  (atlas is Y-down: pc.y1 is the visual bottom)
			uv[1] = float2(u0, v0);	// TL
			uv[2] = float2(u1, v1);	// BR
			uv[3] = float2(u1, v0);	// TR

			nQuadCount++;
		}

		fCellLeft += fSizeX;
	}

	if (nQuadCount == 0)
		return;

	PacketList* pPackets = CPacketBuilder::BuildTexturedQuads2D(positions, texcoords, nQuadCount, color, CTexturedSpriteRenderer::UpdateShader);
	CPacketManager::AddPacketList(*pPackets, false, e_RenderType_TexturedSprites);
}


void CTextRenderer::DrawText2D(const char pText[], float x, float y, float fSize, float4 color)
{
	const float fAR = CWindow::GetMainWindow()->GetAspectRatio();
	DrawText2D(pText, x, y, fSize, 1.66f * fSize * fAR, color);
}


void CTextRenderer::DrawCenteredText2D(const char pText[], float fCenterX, float fCenterY, float fSizeX, float fSizeY, float4 color)
{
	const int nCharCount = (int)strlen(pText);

	// fCenterX/Y are in window space [0,1]; fSizeX/Y are in clip space
	// [-1,1]. Window-space distance is half the equivalent clip-space distance,
	// so the half-width offset applied to the window-space anchor is
	// (nChars * fSizeX) / 2 / 2 = 0.25 * nChars * fSizeX.
	const float x = fCenterX - 0.25f * nCharCount * fSizeX;
	const float y = fCenterY - 0.25f * fSizeY;

	DrawText2D(pText, x, y, fSizeX, fSizeY, color);
}


void CTextRenderer::DrawCenteredText2D(const char pText[], float fCenterX, float fCenterY, float fSize, float4 color)
{
	const float fAR = CWindow::GetMainWindow()->GetAspectRatio();
	DrawCenteredText2D(pText, fCenterX, fCenterY, fSize, 1.66f * fSize * fAR, color);
}
