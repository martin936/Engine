//#include "Engine/Engine.h"
//#include "Engine/Physics/Physics.h"
//#include "Rigidbodies.h"
//
//
//void CRigidbody::Update(float dt)
//{
//	ComputeContacts();
//
//	SolveConstraints(dt);
//
//	ApplyStep(dt);
//}
//
//
//
//void CRigidbody::ComputeContacts()
//{
//	
//	float3 Center, Normal;
//	float fDepth = 0.f, fMinDepth = 0.f;
//
//	float3 MTV;
//	SOrientedBox Box;
//	Box.m_Position = m_Position;
//	Box.m_Dim[0] = m_AABB.x;
//	Box.m_Dim[1] = m_AABB.y;
//	Box.m_Dim[2] = m_AABB.z;
//
//	float3x3 P = Quaternion::GetMatrix(m_Rotation);
//	Box.m_Basis[0] = float3(P.m00, P.m10, P.m20);
//	Box.m_Basis[1] = float3(P.m01, P.m11, P.m21);
//	Box.m_Basis[2] = float3(P.m02, P.m12, P.m22);
//
//	m_nContactCounts = 0;
//	int nCount = 0;
//
//	CheckObstacles(Box);
//
//	CheckRigidbodies(Box);
//
//	CheckSpheres(Box);
//}
//
//
//
//void CRigidbody::ApplyStep(float dt)
//{
//	float	theta = m_AngularMomentum.length() * dt;
//	float3	a = m_AngularMomentum / MAX(1e-3f, m_AngularMomentum.length());
//	float	C = cosf(theta * 0.5f);
//	float	S = sinf(theta * 0.5f);
//
//	Quaternion H(C, a.x * S, a.y * S, a.z * S);
//
//	m_Position.y = 0.f;
//	m_Position += dt * m_LinearMomentum;
//	m_Rotation = H * m_Rotation;
//
//	H = m_Rotation;
//	H.Normalize();
//
//	m_WorldMatrix = Quaternion::GetMatrix(H, m_Position);
//	m_WorldMatrix.m00 *= m_fSize;
//	m_WorldMatrix.m01 *= m_fSize;
//	m_WorldMatrix.m02 *= m_fSize;
//	m_WorldMatrix.m10 *= m_fSize;
//	m_WorldMatrix.m11 *= m_fSize;
//	m_WorldMatrix.m12 *= m_fSize;
//	m_WorldMatrix.m20 *= m_fSize;
//	m_WorldMatrix.m21 *= m_fSize;
//	m_WorldMatrix.m22 *= m_fSize;
//
//	float3x3 P = Quaternion::GetMatrix(H);
//	float3x3 tP(P);
//	tP.transpose();
//
//	m_InertiaTensor = P * (m_OriginalInertiaTensor * tP);
//}
//
//
//
//void CRigidbody::CheckObstacles(SOrientedBox& Box)
//{
//	std::vector<CObstacle*>::iterator it;
//	CObstacle* pObstacle;
//	float3 Center, MTV;
//	float fDepth, fMinDepth;
//	int nCount;
//	m_bIsColliding = false;
//
//	SOrientedBox Obstacle;
//
//	Obstacle.m_Basis[0] = float3(1.f, 0.f, 0.f);
//	Obstacle.m_Basis[1] = float3(0.f, 1.f, 0.f);
//	Obstacle.m_Basis[2] = float3(0.f, 0.f, 1.f);
//
//	/*for (it = CPhysicsEngine::ms_pObstacles.begin(); it < CPhysicsEngine::ms_pObstacles.end(); it++)
//	{
//		pObstacle = *it;
//
//		if (pObstacle->m_bEnabled && pObstacle->m_bUseOnCPU)
//		{
//			Copy(&Center.x, pObstacle->m_Center);
//
//			if ((Center - m_Position).length() < m_fSphereRadius + pObstacle->m_fBoundingSphereRadius)
//			{
//				Copy(&Obstacle.m_Position.x, pObstacle->m_Center);
//				Copy(Obstacle.m_Dim, pObstacle->m_AABB.v());
//
//				if (Collisions::GetMinimumTranslationVector(Box, Obstacle, MTV, &fDepth, &fMinDepth))
//				{
//					nCount = Collisions::ExtractContactPoints(Box, Obstacle, MTV, fMinDepth, &m_Contacts[m_nContactCounts], float3x3(0.f), 1e8f);
//					m_nContactCounts += nCount;
//
//					m_bIsColliding = true;
//					m_MTV = MTV;
//				}
//			}
//		}
//	}*/
//}
//
//
//void CRigidbody::CheckRigidbodies(SOrientedBox& Box)
//{
//	if (!m_bShouldCollideWithRigidBodies)
//		return;
//
//	std::vector<CRigidbody*>::iterator it;
//	CRigidbody* pRigidbody;
//
//	SOrientedBox Obstacle;
//	float3 MTV;
//	float3x3 P;
//	float fDepth, fMinDepth;
//	int nCount;
//
//	/*for (it = CPhysicsEngine::ms_pRigidbodies.begin(); it < CPhysicsEngine::ms_pRigidbodies.end(); it++)
//	{
//		pRigidbody = *it;
//
//		if (pRigidbody->m_bEnabled && pRigidbody != this && pRigidbody->m_bShouldCollideWithRigidBodies)
//		{
//			if ((pRigidbody->m_Position - m_Position).length() < m_fSphereRadius + pRigidbody->m_fSphereRadius)
//			{
//				Obstacle.m_Position = pRigidbody->m_Position;
//				Copy(Obstacle.m_Dim, pRigidbody->m_AABB.v());
//
//				P = Quaternion::GetMatrix(pRigidbody->m_Rotation);
//				Obstacle.m_Basis[0] = float3(P.m00, P.m10, P.m20);
//				Obstacle.m_Basis[1] = float3(P.m01, P.m11, P.m21);
//				Obstacle.m_Basis[2] = float3(P.m02, P.m12, P.m22);
//
//				if (Collisions::GetMinimumTranslationVector(Box, Obstacle, MTV, &fDepth, &fMinDepth))
//				{
//					nCount = Collisions::ExtractContactPoints(Box, Obstacle, MTV, fMinDepth, &m_Contacts[m_nContactCounts], pRigidbody->m_InverseInertiaTensor, pRigidbody->m_fMass);
//
//					pRigidbody->m_bIdle = false;
//
//					m_nContactCounts += nCount;
//				}
//			}
//		}
//	}*/
//}
//
//
//void CRigidbody::CheckSpheres(SOrientedBox& Box)
//{
//	float3 MTV;
//	float fDepth, fMinDepth;
//	int nCount;
//
//	for (unsigned int i = 0; i < ms_nSphereCount; i++)
//	{
//		if (m_bShouldCollideWithSpheres || ms_CollisionSpheres[i].m_bForceCollision)
//		{
//			if (Collisions::GetMinimumTranslationVector(Box, ms_CollisionSpheres[i], MTV, &fDepth, &fMinDepth))
//			{
//				nCount = Collisions::ExtractContactPoints(Box, ms_CollisionSpheres[i], MTV, fDepth, &m_Contacts[m_nContactCounts], float3x3(0.f), 1e8f);
//				m_nContactCounts += nCount;
//			}
//		}
//	}
//
//
//	for (unsigned int i = 0; i < ms_nPartialSphereCount; i++)
//	{
//		if (m_bShouldCollideWithSpheres || ms_CollisionPartialSpheres[i].m_bForceCollision)
//		{
//			if (Collisions::GetMinimumTranslationVector(Box, ms_CollisionPartialSpheres[i], MTV, &fDepth, &fMinDepth))
//			{
//				nCount = Collisions::ExtractContactPoints(Box, (SSphere&)(ms_CollisionPartialSpheres[i]), MTV, fDepth, &m_Contacts[m_nContactCounts], float3x3(0.f), 1e8f);
//				m_nContactCounts += nCount;
//			}
//		}
//	}
//}
//
