#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "SpecialFX.h"


std::vector<CSpecialFX::SExplosion> CSpecialFX::ms_Explosions;
CSpecialFX::SExplosionSprite* CSpecialFX::ms_Sprites = NULL;

int CSpecialFX::ms_nMaxNumSpritePerExplosion = 2000;
int CSpecialFX::ms_nMaxNumExplosions = 1;


void CSpecialFX::Init()
{
	ms_Sprites = new SExplosionSprite[ms_nMaxNumExplosions * ms_nMaxNumSpritePerExplosion];
}


void CSpecialFX::Terminate()
{
	delete[] ms_Sprites;
}


void CSpecialFX::Process()
{
	std::vector<CSpecialFX::SExplosion>::iterator it;

	for (it = ms_Explosions.begin(); it < ms_Explosions.end(); it++)
	{
		ProcessExplosion(&(*it));
	}
}
