#ifndef PHYSICS_RIGIDBODIES_INC
#define PHYSICS_RIGIDBODIES_INC

#include "Engine/Renderer/Packets/Packet.h"


#define MAX_RIGID_CONTACTS 1000
#define MAX_COLLISION_SPHERES 100

_declspec(align(32)) struct SOrientedBox
{
	float3	m_Position;
	float3	m_Basis[3];
	float	m_Dim[3];

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
};


_declspec(align(32)) struct SSphere
{
	float3	m_Position;
	float3	m_Velocity;
	float	m_fRadius;
	bool	m_bForceCollision;

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
};


_declspec(align(32)) struct SPartialSphere
{
	float3	m_Position;
	float3	m_Velocity;
	float	m_fRadius;
	bool	m_bForceCollision;

	float3	m_Direction;
	float	m_fAngle;

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}
};


_declspec(align(32)) class CRigidbody
{
	friend class CPhysicsEngine;

public:

	enum ECollisionType
	{
		e_Walls = 0,
		e_Rigidbodies = 1,
		e_Spheres = 2
	};

	CRigidbody(CMesh* pMesh, float fMass, bool bIsUniform = true);
	~CRigidbody();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	//inline void BindPacketList(PacketList* pList) { m_pPacketList = pList; }

	inline void SetPosition(float3& Pos) 
	{ 
		m_Position = Pos;
		m_bIsColliding = false;
	}

	inline void SetVelocity(float3& Velocity) 
	{ 
		m_LinearMomentum = Velocity; 
		m_AngularMomentum = 0.f;
		m_bIsColliding = false;
	}

	inline void SetRotation(Quaternion& q) { m_Rotation = q; }

	inline float3 GetMinimumTranslationVector() const
	{
		return m_MTV;
	}

	inline Quaternion GetRotation() const { return m_Rotation; }

	inline float3 GetPosition() const 
	{ 
		return m_Position; 
	}

	inline float3 GetVelocity() const 
	{ 
		return m_LinearMomentum; 
	}

	inline void SetModelMatrix(float4x4& ModelMatrix) 
	{ 
		m_WorldMatrix = ModelMatrix; 
	}

	inline float4x4 GetModelMatrix() const 
	{ 
		return m_WorldMatrix; 
	}

	inline bool IsIdle() const 
	{ 
		return m_bIdle; 
	}

	inline void SetSize(float fSize)
	{
		m_AABB *= fSize / m_fSize;
		m_fSphereRadius *= fSize / m_fSize;
		m_fSize = fSize;
	}

	inline void SetCollisionParams(unsigned int nFlags)
	{
		if (nFlags & e_Rigidbodies)
			m_bShouldCollideWithRigidBodies = true;
		else
			m_bShouldCollideWithRigidBodies = false;

		if (nFlags & e_Spheres)
			m_bShouldCollideWithSpheres = true;
		else
			m_bShouldCollideWithSpheres = false;
	}

	inline bool IsEnabled() const
	{
		return m_bEnabled;
	}

	inline float GetBoundingSphereRadius() const { return m_fSphereRadius; }
	inline float3 GetBoundingBox() const { return m_AABB; }

	inline bool IsColliding() const { return m_bIsColliding; }

	inline void Enable() { m_bEnabled = true; }
	inline void Disable() { m_bEnabled = false; }

	void Update(float dt);

	//void Draw();

	struct SContact
	{
		float3	m_Distance1;
		float3	m_Distance2;
		float3	m_Normal;
		float3	m_Tangent[2];
		float	m_fMultipliers[3];
		float	m_fDepth;
		float3x3 m_fInverseInertia;
		float	m_fMass;
	};

	enum EConstraintType
	{
		e_Lagrange,
		e_KarunKushTucker
	};


	static void AddSphere(float3& Position, float3& Velocity, float fRadius, bool bForceCollision = false);
	static void AddPartialSphere(float3& Position, float3& Velocity, float fRadius, float3& direction, float fAngle, bool bForceCollision = false);

	inline bool ShouldCollideWithSpheres() const { return m_bShouldCollideWithSpheres; }

private:

	//PacketList*	m_pPacketList;

	SContact	m_Contacts[MAX_RIGID_CONTACTS];
	int			m_nContactCounts;

	static float ms_fDefaultSphereRadius;

	float3		m_AABB;
	float3		m_MTV;

	float3		m_Position;
	Quaternion	m_Rotation;
	float3		m_LinearMomentum;
	float3		m_AngularMomentum;

	float3x3	m_OriginalInertiaTensor;
	float3x3	m_InertiaTensor;
	float3x3	m_InverseInertiaTensor;

	bool	m_bShouldCollideWithSpheres;
	bool	m_bShouldCollideWithRigidBodies;

	bool	m_bEnabled;

	float	m_fSize;

	float	m_fSphereRadius;
	bool	m_bIdle;

	bool	m_bIsColliding;

	float4x4 m_WorldMatrix;

	float  m_fMass;

	void ComputeContacts();

	void CheckObstacles(SOrientedBox& Box);
	void CheckRigidbodies(SOrientedBox& Box);
	void CheckSpheres(SOrientedBox& Box);

	void SolveConstraints(float dt);
	float SolveNonPenetrationConstraint(float dt, SContact& Contact);
	float SolveFrictionConstraint(float fFriction, SContact& Contact);

	float ComputeMultiplier(float3& n1, float3& u1, float3& n2, float3& u2, float3x3& InverseInertia, float fBias, EConstraintType eType);

	void ApplyStep(float dt);

	static unsigned int ms_nSphereCount;
	static unsigned int ms_nPartialSphereCount;

	static SSphere			ms_CollisionSpheres[MAX_COLLISION_SPHERES];
	static SPartialSphere	ms_CollisionPartialSpheres[MAX_COLLISION_SPHERES];
};


namespace Collisions
{
	bool GetMinimumTranslationVector(SOrientedBox& A, SOrientedBox& B, float3& Direction, float* pDepth, float* fMinDepth);
	bool GetMinimumTranslationVector(SOrientedBox& A, SSphere& S, float3& Direction, float* pDepth, float* fMinDepth);
	bool GetMinimumTranslationVector(SOrientedBox& A, SPartialSphere& S, float3& Direction, float* pDepth, float* fMinDepth);
	int ExtractContactPoints(SOrientedBox& A, SOrientedBox& B, float3& Axis, float fMinDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass);
	int ExtractContactPoints(SOrientedBox& A, SSphere& S, float3& Axis, float fMinDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass);
}



#endif
