#ifndef PHYSICS_FORCES_INC
#define PHYSICS_FORCES_INC


typedef enum
{
  FORCE_USAGE_ALWAYS,
  FORCE_USAGE_TEMPORARY
}ForceUsage;


typedef struct {

  char name[256];
  float3 m_Vector;
  int pending;
  int active;
  int speed_target;
  ForceUsage usage;

  float duration;
  float time_left;

}Force;


__declspec(align(32)) class CForce
{
	friend class CSoftbody;
	friend class CParticles;

public:

	CForce(const char* cName, float3 Force, ForceUsage Usage, float fDuration);
	~CForce() {};

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Enable();
	void Disable();

	inline const char* GetName() const { return m_cName; }

private:

	char	m_cName[256];
	float3	m_Force;

	bool	m_bPending;
	bool	m_bActive;
	bool	m_bSpeedTarget;

	ForceUsage m_Usage;

	float	m_fDuration;
	float	m_fTimeLeft;
};


class CForceField
{
	friend class CSoftbody;
	friend class CParticles;

public:

	CForceField(const char* cName, float(*pFieldFunc)(float3& Position, float3& Velocity));
	~CForceField();

	void Enable();
	void Disable();

	inline const char* GetName() const { return m_cName; }

private:

	char m_cName[256];
	float(*m_pFieldFunc)(float3& Position, float3& Velocity);

	bool m_bPending;
	bool m_bActive;
	
	float m_fDuration;
	float m_fTimeLeft;
};


#endif
