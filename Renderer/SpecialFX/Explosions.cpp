#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "SpecialFX.h"


float CSpecialFX::ms_fStandardTemp		= 298.15f;		// K
float CSpecialFX::ms_fStandardPressure	= 1.013e5f;		// Pa
float CSpecialFX::ms_fEnergyGap			= 13.6f;		// eV
float CSpecialFX::ms_fFreedEnergy		= 1e6f;			// eV

#define kB	8.617e-5f	// eV / K
#define Tau 1e-5f		// s
#define c	3e8f		// m / s


void CSpecialFX::AddExplosion(float3 Center, float fMass)
{
	SExplosion sExplosion;

	sExplosion.m_Center = Center;
	sExplosion.m_fMass = fMass;
	sExplosion.m_nNumSprites = 0;
	sExplosion.m_nFirstSprite = (int)ms_Explosions.size() * ms_nMaxNumSpritePerExplosion;

	ms_Explosions.push_back(sExplosion);
}


void CSpecialFX::ProcessExplosion(SExplosion* pExplosion)
{
	if (pExplosion->m_nNumSprites == 0)
	{
		ms_Sprites[pExplosion->m_nFirstSprite].m = pExplosion->m_fMass;
		ms_Sprites[pExplosion->m_nFirstSprite].T = ms_fStandardTemp;
		ms_Sprites[pExplosion->m_nFirstSprite].n = 1.f;
		ms_Sprites[pExplosion->m_nFirstSprite].r = 0.02f;
		ms_Sprites[pExplosion->m_nFirstSprite].V = 4.f * 3.141592f * powf(ms_Sprites[pExplosion->m_nFirstSprite].r, 3.f) / 3.f;
		ms_Sprites[pExplosion->m_nFirstSprite].rho = pExplosion->m_fMass / ms_Sprites[pExplosion->m_nFirstSprite].V;

		ms_Sprites[pExplosion->m_nFirstSprite].m_Position[0] = pExplosion->m_Center.x;
		ms_Sprites[pExplosion->m_nFirstSprite].m_Position[1] = pExplosion->m_Center.y;
		ms_Sprites[pExplosion->m_nFirstSprite].m_Position[2] = pExplosion->m_Center.z;

		ms_Sprites[pExplosion->m_nFirstSprite].m_Velocity[0] = 0.f;
		ms_Sprites[pExplosion->m_nFirstSprite].m_Velocity[1] = 0.f;
		ms_Sprites[pExplosion->m_nFirstSprite].m_Velocity[2] = 0.f;

		pExplosion->m_nNumSprites++;
	}

	else
	{
		float n, T, dt, V, r, m;
		int nSpriteIndex;

		for (nSpriteIndex = pExplosion->m_nFirstSprite; nSpriteIndex < pExplosion->m_nFirstSprite + pExplosion->m_nNumSprites; nSpriteIndex++)
		{
			dt = 0.001f;
			n = ms_Sprites[nSpriteIndex].n;
			T = ms_Sprites[nSpriteIndex].T;
			V = ms_Sprites[nSpriteIndex].V;
			r = ms_Sprites[nSpriteIndex].r;
			m = ms_Sprites[nSpriteIndex].m;

			float dr = 2.f * kB * T * c * c / (200e9f) - ms_fStandardPressure * V / m;

			ms_Sprites[nSpriteIndex].n = n * (1.f - expf(-ms_fEnergyGap / (kB * T)) * dt / Tau);
			ms_Sprites[nSpriteIndex].r = r + (dr < 0.f ? -sqrtf(-dr) : sqrtf(dr)) * dt;
			ms_Sprites[nSpriteIndex].V = 4.f * 3.151492f * powf(ms_Sprites[nSpriteIndex].r, 3.f) / 3.f;
			ms_Sprites[nSpriteIndex].T = T + 2.f * n * ms_fFreedEnergy * expf(-ms_fEnergyGap / (kB * T)) * ms_fFreedEnergy / (5.f * kB * Tau);
		}
	}


	for (int nSpriteIndex = pExplosion->m_nFirstSprite; nSpriteIndex < pExplosion->m_nFirstSprite + pExplosion->m_nNumSprites; nSpriteIndex++)
	{
		float3 Pos;
		Copy(&Pos.x, ms_Sprites[nSpriteIndex].m_Position);

		float fSize[2];
		fSize[0] = 2.f * ms_Sprites[nSpriteIndex].r;
		fSize[1] = 2.f * ms_Sprites[nSpriteIndex].r;

		CSpriteEngine::AddCenteredSprite3D(Pos, float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), fSize, float4(1.f, 0.f, 0.f, 0.5f), INVALIDHANDLE);
	}
}
