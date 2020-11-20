#ifndef PHYSICS_PARTICLES_INC
#define PHYSICS_PARTICLES_INC

#define MAX_PARTICLE_COUNT 1000000

class CParticles
{
	friend class CPhysicsEngine;

private:

	static void InitParticleEngine();
	static void UnloadParticleEngine();

public:

	enum EParticleType
	{
		e_BillBoard,
		e_IcoSphere,
		e_Mesh
	};

	enum ESourceType
	{
		e_None,
		e_Static,
		e_Dynamic
	};

	CParticles(EParticleType eType, unsigned int nMaxCount);
	~CParticles();

	void SetStaticSource(float3& Position);
	void SetRandomSourceOnMesh();

	void SetDispersion(float fDispersion)	{ m_fDispersion = fDispersion; }
	void SetFlowRate(float fRate)			{ m_fFlowRate = fRate; }
	void SetSpeed(float fSpeed)				{ m_fSpeed = fSpeed; }
	void SetLifeTime(float fLifeTime)		{ m_fLifeTime = fLifeTime; }

	void Enable() { m_bEnabled = true; }
	void Disable() { m_bEnabled = false; }

	void Update(float dt);

	void Draw();

	void SetSprite(unsigned int nTexID);

	void BindPacket(Packet* pPacket);

	struct SParticleVertex
	{
		TVector	m_Position;
		float	m_fElapsedTime;
		TVector	m_Velocity;
	};

private:

	void UpdateSource();
	void AddNewParticles(int nCount, float dt);
	void Process(float dt);

	void WriteGlobalBuffer();
	void ReadGlobalBuffer();	

	EParticleType	m_eType;
	ESourceType		m_eSourceType;
	unsigned int	m_nSpriteID;

	unsigned int	m_nVertexBufferID;
	unsigned int	m_nGlobalBufferID;

	unsigned int	m_nParentVertexBufferID;
	unsigned int	m_nParentIndexBufferID;
	unsigned int	m_nParentNormalBufferID;
	unsigned int	m_nConvertedIndexBufferID;

	unsigned int	m_nMeshTriangleCount;
	unsigned int	m_nMeshVertexCount;

	Packet*	m_pPacket;

	int		m_nCount;
	int		m_nNbActive;
	int		m_nFirstIndex;

	float	m_fDispersion;
	float	m_fFlowRate;
	float	m_fSpeed;
	float	m_fLifeTime;

	float	m_fLeftOverParticles;

	float3	m_SourcePosition;
	float3	m_SourceVelocity;

	int		m_nTriangleIndex;
	float	m_fBarycentricU;
	float	m_fBarycentricV;

	bool	m_bEnabled;
	bool	m_bIsIndexBufferReady;

	struct SGlobalInfo
	{
		unsigned int	m_nParticleCount;
		TVector			m_ExternalForces;
		TVector			m_SourcePos;
		TVector			m_SourceNormal;
		float			m_fFlowRate;
		float			m_fSpeed;
		float			m_fDispersion;
	};

	SGlobalInfo m_GlobalInfos;

};


#endif
