#ifndef PHYSICS_SOFTBODIES_INC
#define PHYSICS_SOFTBODIES_INC


#define SOFTBODY_MAX_MESH_NEIGHBOURS	15

class CSoftbody
{
	friend class CPhysicsEngine;
public:

	CSoftbody(CPhysicalMesh* pMesh, float fMass, float fSpring, float fDamping, bool bVolumic, bool bGPUAccelerated = true);
	~CSoftbody();

	void Update(float dt);

	inline float GetVolume() const { return m_fVolume; }
	inline float3 GetPosition() const { return m_fCenterOfMass; }
	inline float3 GetVelocity() const { return m_Velocity; }
	inline float3 GetLastFreeVelocity() const { return m_LastFreeVelocity; }

	inline void SetCompressibility(float fCompressibility) 
	{ 
		m_fCompressibility = fCompressibility;
		m_fInternalEnergy = 1e2f / m_fCompressibility; 

		m_GlobalInfos.m_fCompressibility = m_fCompressibility;
		m_GlobalInfos.m_fInternalEnergy = m_fInternalEnergy;
	}

	inline void SetStiffness(float fStiffness)
	{
		m_fSpring = fStiffness;
	}

	void ReflectVelocity(float3& Normal, float fScale);

	inline bool IsColliding() const { return m_bIsInCollision; }

	void AddForce(const char* Name, float3 Force, ForceUsage eUsage, float fDuration = 0.f);
	void EditForce(const char* Name, float3 Force);
	void EditForceAdditive(const char* Name, float3 Force);
	void EditForceDuration(const char* Name, float fDuration);
	bool IsForceEnabled(const char* Name);
	void EnableForce(const char* Name);
	void DisableForce(const char* Name);
	void FlushForce(const char* Name);
	void FlushForces();
	void SetForceSpeedTarget(const char* Name, float3 Target);
	void SumForces();

	inline void SetAttractor(float3& Position, float fStrength)
	{
		m_AttractorPosition = Position;
		m_fAttractorStrength = fStrength;
	}

	void ReflectVelocities(float3& Direction);

	inline void SetReferenceSpeed(float fSpeed) { m_fReferenceSpeed = fSpeed; }
	inline void SetReferenceHeight(float fHeight) { m_fReferenceHeight = fHeight; }
	inline void ResetLastFreeVelocity()	{ m_LastFreeVelocity = 0.f; }

	inline void Freeze() { m_bFrozen = true; }
	inline void Unfreeze() { m_bFrozen = false; }

	inline void ResetVelocities() { m_bShouldResetVelocities = true; }

	inline CPhysicalMesh* GetMesh() const { return m_pMesh; }

	inline void SetRestVolume(float fVolume)
	{
		m_fRestVolume = fVolume;
	}

	void SetPosition(float3& Pos);

	inline void AddPacketListToSkin(PacketList* pList)
	{
		m_pMesh->m_pSkinner->Add(pList);
	}

	inline void Skin() 
	{ 
		m_pMesh->m_pSkinner->SkinAll(m_pMesh); 
	}

private:

	void BuildBuffers();

	bool	m_bShouldResetVelocities;

	CPhysicalMesh* m_pMesh;

	float	m_fLeftOverTime;

	bool	m_bEnabled;
	bool	m_bFrozen;

	float	m_fMass;
	float	m_fSpring;
	float	m_fDamping;

	bool	m_bVolumic;

	float	m_fVolume;
	float	m_fRestVolume;
	float	m_fInternalEnergy;
	float	m_fCompressibility;

	float3	m_ExternalForce;
	float3	m_Velocity;
	float3	m_LastFreeVelocity;
	float3	m_ReferenceVelocity;

	float3	m_fCenterOfMass;
	float3	m_fLastCenterOfMass;
	float3	m_BouncingNormal;

	float3	m_AttractorPosition;
	float	m_fAttractorStrength;

	float	m_fMaxVerticalSpeed;
	int		m_nMaxNeighbourCount;

	float	m_fReferenceHeight;
	float	m_fReferenceSpeed;
	bool	m_bIsInCollision;
	bool	m_bRecordVelocity;
	int*	m_dpIsInCollision;
	double	m_fLastCollisionTime;

	float**	m_pRestLength;

	int						m_nForcesCount;
	std::vector<CForce*>	m_pForces;

	struct SGlobalInfo
	{
		unsigned int	m_nVertexCount;
		unsigned int	m_nTriangleCount;
		unsigned int	m_nMaxNeighbourCount;
		unsigned int	m_bIsColliding;
		float			m_fRestVolume;
		float			m_fVolume;
		float			m_fInternalEnergy;
		float			m_fCompressibility;
		float			m_fMass;
		float			m_fSpring;
		float			m_fFriction;
		float			m_fDamping;
		float			m_fRefSpeed;
		float			m_fMaxAbsoluteSpeed;
		float			m_fMaxRelativeSpeed;
		TVector			m_MeanPosition;
		TVector			m_MeanVelocity;
		TVector			m_NewMeanVelocity;
		TVector			m_ExternalForces;
		TVector			m_AttractorPosition;
		float			m_fAttractorStrength;
		unsigned int	m_uMutex;
	};

	SGlobalInfo m_GlobalInfos;

	struct SNeighbourInfo
	{
		unsigned int		m_nIndex;
		float				m_fRestLength;
	};


	unsigned int m_nGlobalInfoBufferID;
	unsigned int m_nLocalInfoBufferID;
	unsigned int m_nNeighbourCountBufferID;

	/*unsigned int m_nVertexBufferToSkinID;
	unsigned int m_nNormalBufferToSkinID;
	unsigned int m_nTangentBufferToSkinID;
	unsigned int m_nBitangentBufferToSkinID;*/

	bool m_bLoaded;
	bool m_bRunOnGPU;

	/*void*	m_pGPUPool;

	inline float* GetVelocitiesPointer() const { return (float*)m_pGPUPool; }
	inline float* GetForcesPointer() const { return (float*)m_pGPUPool + 3 * m_pMesh->nvert; }
	inline float* GetCenterOfMassPointer() const { return (float*)m_pGPUPool + (6 + SOFTBODY_MAX_MESH_NEIGHBOURS) * m_pMesh->nvert; }
	inline float* GetMeanVelocityPointer() const { return (float*)m_pGPUPool + (6 + SOFTBODY_MAX_MESH_NEIGHBOURS) * m_pMesh->nvert + 3; }
	inline float* GetVolumePointer() const { return (float*)m_pGPUPool + (6 + SOFTBODY_MAX_MESH_NEIGHBOURS) * m_pMesh->nvert + 6; }

	inline float* GetPositionPointer() const { return (float*)m_pMesh->gpu_pool; }
	inline int*	GetMutexPointer() const { return (int*)((float*)m_pGPUPool + (6 + SOFTBODY_MAX_MESH_NEIGHBOURS) * m_pMesh->nvert + 11); }*/

	static void Sim_ResetVelocities();
	static void Sim_ComputeStatistics();
	static void Sim_ComputeVolume();
	static void Sim_UpdateSoftbody();

	void Run(float dt);
	void ComputeVolume();
	void ComputeMeanValues();

	void RunOnCPU(float dt);
	void ComputeVolumeOnCPU();
	void ComputeMeanValuesOnCPU();

	void RunOnGPU(float dt);
	void ComputeVolumeOnGPU();
	void ComputeMeanValuesOnGPU();

	void ResetVelocitiesOnGPU() const;

	void SkinPacket();

	void WriteGlobalInfos(SGlobalInfo* pInfo) const;
	void ReadGlobalInfos();

	void SetVelocities(float3 Velocity);
};


#endif
