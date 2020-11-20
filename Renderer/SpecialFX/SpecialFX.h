#ifndef __SPECIAL_FX_H__
#define __SPECIAL_FX_H__


class CSpecialFX
{
public:

	static void Init();
	static void Terminate();

	static void Process();

	static void AddExplosion(float3 Center, float fMass);

private:

	struct SExplosion
	{
		float3	m_Center;

		float	m_fMass;
		int		m_nNumSprites;
		int		m_nFirstSprite;
	};

	struct SExplosionSprite
	{
		float	T;
		float	n;
		float	V;
		float	m;

		float	rho;
		float	r;

		float	m_Position[3];
		float	m_Velocity[3];
	};

	static void ProcessExplosion(SExplosion* pExplosion);

	static SExplosionSprite* ms_Sprites;
	static std::vector<SExplosion> ms_Explosions;

	static int ms_nMaxNumSpritePerExplosion;
	static int ms_nMaxNumExplosions;

	static float ms_fStandardTemp;
	static float ms_fStandardPressure;
	static float ms_fEnergyGap;
	static float ms_fFreedEnergy;
};


#endif
