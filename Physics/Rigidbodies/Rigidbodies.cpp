#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"


float CRigidbody::ms_fDefaultSphereRadius = 0.05f;

unsigned int CRigidbody::ms_nSphereCount = 0;
unsigned int CRigidbody::ms_nPartialSphereCount = 0;

SSphere CRigidbody::ms_CollisionSpheres[MAX_COLLISION_SPHERES];
SPartialSphere CRigidbody::ms_CollisionPartialSpheres[MAX_COLLISION_SPHERES];

CRigidbody::CRigidbody(CMesh* pMesh, float fMass, bool bIsUniform)
{
	
	m_Position = pMesh->GetCenter();
	m_Rotation = Quaternion(1.f, 0.f, 0.f, 0.f);

	m_LinearMomentum = 0.f;
	m_AngularMomentum = float3(0.f, 0.f, 0.f);

	m_AABB = pMesh->GetBoundingBox();
	m_fSphereRadius = pMesh->m_fBoundingSphereRadius;

	m_OriginalInertiaTensor = fMass * float3x3(	float3(0.25f * (m_AABB.y * m_AABB.y + m_AABB.z * m_AABB.z) / 3.f, 0.f, 0.f),
												float3(0.f, 0.25f * (m_AABB.x * m_AABB.x + m_AABB.z * m_AABB.z) / 3.f, 0.f),
												float3(0.f, 0.f, 0.25f * (m_AABB.x * m_AABB.x + m_AABB.y * m_AABB.y) / 3.f));

	m_InertiaTensor = m_OriginalInertiaTensor;
	m_fMass = fMass;

	m_bIdle = false;
	m_bShouldCollideWithRigidBodies = false;
	m_bShouldCollideWithSpheres = false;

	m_bIsColliding = false;
	m_fSize = 1.f;

	m_MTV = 0.f;

	m_bEnabled = true;
}


CRigidbody::~CRigidbody()
{
	
}


void CRigidbody::AddSphere(float3& Position, float3& Velocity, float fRadius, bool bForceCollision)
{
	ms_CollisionSpheres[ms_nSphereCount].m_Position = Position;
	ms_CollisionSpheres[ms_nSphereCount].m_Velocity = Velocity;
	ms_CollisionSpheres[ms_nSphereCount].m_fRadius = fRadius;
	ms_CollisionSpheres[ms_nSphereCount].m_bForceCollision = bForceCollision;

	ms_nSphereCount++;
}


void CRigidbody::AddPartialSphere(float3& Position, float3& Velocity, float fRadius, float3& Direction, float fAngle, bool bForceCollision)
{
	ms_CollisionPartialSpheres[ms_nSphereCount].m_Position = Position;
	ms_CollisionPartialSpheres[ms_nSphereCount].m_Velocity = Velocity;
	ms_CollisionPartialSpheres[ms_nSphereCount].m_Direction = Direction;
	ms_CollisionPartialSpheres[ms_nSphereCount].m_fAngle = fAngle;
	ms_CollisionPartialSpheres[ms_nSphereCount].m_fRadius = fRadius;
	ms_CollisionPartialSpheres[ms_nSphereCount].m_bForceCollision = bForceCollision;

	ms_nSphereCount++;
}


