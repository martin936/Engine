#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "Text.h"

CTexture* CTextRenderer::ms_pFontTex = NULL;

void CTextRenderer::InitFont(const char* path)
{
	//ms_pFontTex = new CTexture(path, ETextureFormat::e_R8G8B8A8);
}


bool CTextRenderer::ComputeTexCoords(char cLetter, float* pTexCoords)
{
	int nOffset = cLetter - '!';

	if (nOffset < 0)
		return false;

	pTexCoords[0] = 0.06225f * (nOffset % 16);
	pTexCoords[1] = 1.f - 0.125f * (nOffset / 16) - 0.12f;

	pTexCoords[2] = pTexCoords[0];
	pTexCoords[3] = pTexCoords[1] + 0.12f;

	pTexCoords[4] = pTexCoords[0] + 0.064f;
	pTexCoords[5] = pTexCoords[1];

	pTexCoords[6] = pTexCoords[0] + 0.064f;
	pTexCoords[7] = pTexCoords[1] + 0.12f;

	return true;
}


void CTextRenderer::DrawText2D(const char pText[], float x, float y, float fSize, float4& color)
{
	/*int nCharCount = (int)strlen(pText);

	float fAR = CWindow::GetMainWindow()->GetAspectRatio();

	float fX = x * 2.f - 1.f;
	float fY = 1.f - y * 2.f;

	float Size[2] = { fSize, 1.66f * fSize * fAR };
	float fCenter[2] = { fX + 0.5f * Size[0], fY - 0.5f * Size[1] };
	float fTexCoords[8];

	CSpriteEngine::StartCenteredSprite2DSequence(color, ms_pFontTex->m_nID);

	for (int i = 0; i < nCharCount; i++)
	{
		if (pText[i] != ' ')
		{
			ComputeTexCoords(pText[i], fTexCoords);
			CSpriteEngine::AddCenteredSprite2DToSequence(fCenter, Size, fTexCoords);
		}

		fCenter[0] += fSize;
	}

	CSpriteEngine::EndCenteredSprite2DSequence();*/
}


void CTextRenderer::DrawText2D(const char pText[], float x, float y, float fSizeX, float fSizeY, float4& color)
{
	/*int nCharCount = (int)strlen(pText);

	float fAR = CWindow::GetMainWindow()->GetAspectRatio();

	float fX = x * 2.f - 1.f;
	float fY = 1.f - y * 2.f;

	float Size[2] = { fSizeX, fSizeY * fAR };
	float fCenter[2] = { fX + 0.5f * Size[0], fY - 0.5f * Size[1] };
	float fTexCoords[8];

	CSpriteEngine::StartCenteredSprite2DSequence(color, ms_pFontTex->m_nID);

	for (int i = 0; i < nCharCount; i++)
	{
		if (pText[i] != ' ')
		{
			ComputeTexCoords(pText[i], fTexCoords);
			CSpriteEngine::AddCenteredSprite2DToSequence(fCenter, Size, fTexCoords);
		}

		fCenter[0] += fSizeX;
	}

	CSpriteEngine::EndCenteredSprite2DSequence();*/
}


void CTextRenderer::DrawCenteredText2D(const char pText[], float fCenterX, float fCenterY, float fSize, float4& color)
{
	/*int nCharCount = (int)strlen(pText);
	float fAR = CWindow::GetMainWindow()->GetAspectRatio();

	float x = fCenterX - nCharCount * fSize * 0.25f;
	float y = fCenterY - 1.66f * 0.5f * fSize;

	DrawText2D(pText, x, y, fSize, color);*/
}


void CTextRenderer::DrawCenteredText2D(const char pText[], float fCenterX, float fCenterY, float fSizeX, float fSizeY, float4& color)
{
	/*int nCharCount = (int)strlen(pText);
	float fAR = CWindow::GetMainWindow()->GetAspectRatio();

	float x = fCenterX - nCharCount * fSizeX * 0.25f;
	float y = fCenterY - 0.5f * fSizeY;

	DrawText2D(pText, x, y, fSizeX, fSizeY, color);*/
}
