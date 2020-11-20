#include "Engine/Engine.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "Engine/Renderer/Text/Text.h"

#include "Menu.h"


CMenuLabel::CMenuLabel(CMenuContainer* pParent, const char* pText) : CMenuItem(pParent, false)
{
	strcpy_s(m_cText, pText);
	m_bJustified = false;
}


void CMenuLabel::Draw()
{
	size_t nNbChar = strlen(m_cText) - 1;
	nNbChar = nNbChar > 0 ? nNbChar : 1;

	float fWidth;

	if (m_bJustified)
		fWidth = 2.f * m_fSize[0] / nNbChar;

	else
		fWidth = MIN(2.f * m_fSize[0] / nNbChar, m_fSize[1] * 0.667f);

	CTextRenderer::DrawCenteredText2D(m_cText, m_fPosition[0], m_fPosition[1], fWidth, m_fSize[1], m_Color);
}



