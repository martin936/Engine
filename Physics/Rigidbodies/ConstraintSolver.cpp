//#include "Engine/Engine.h"
//#include "Rigidbodies.h"
//#include "Engine/Editor/Adjustables/Adjustables.h"
//
//ADJUSTABLE("Friction", float, gs_Friction, 1.f, 0.f, 1.f, "Physics/Rigidbodies")
//
//
//
//void CRigidbody::SolveConstraints(float dt)
//{
//	float3 Gravity = float3(0.f, 0.f, -9.81f);
//
//	m_LinearMomentum += dt * Gravity;
//
//	float fFriction = powf(gs_Friction, 4.f);
//
//	int nMaxIter = 64;
//	int i, j;
//	float fTotalError = 0.f;
//
//	m_InverseInertiaTensor = inverse(m_InertiaTensor);
//
//	for (i = 0; i < nMaxIter; i++)
//	{
//		fTotalError = 0.f;
//
//		for (j = 0; j < m_nContactCounts; j++)
//		{
//			fTotalError += SolveNonPenetrationConstraint(dt, m_Contacts[j]);
//			fTotalError += SolveFrictionConstraint(fFriction, m_Contacts[j]);
//		}
//
//		if (fTotalError < 1e-4f)
//			break;
//	}
//
//	if (fTotalError > 1e-4f)
//		fTotalError = 0.f;
//
//	m_AngularMomentum.x = m_AngularMomentum.z = 0.f;
//	m_LinearMomentum.y = 0.f;
//}
//
//
//float CRigidbody::SolveNonPenetrationConstraint(float dt, SContact& Contact)
//{
//	float3 Normal = Contact.m_Normal;
//	float3 R1crossN = float3::cross(Contact.m_Distance1, Normal);
//	float3 R2crossN = float3::cross(Contact.m_Distance2, Normal);
//
//	float fBias = -0.4f * (Contact.m_fDepth - 0.01f) / dt;
//	if (fBias > 0.f)
//		fBias = 0.f;
//
//	float fSavedMultiplier = Contact.m_fMultipliers[0];
//	Contact.m_fMultipliers[0] = ComputeMultiplier(Normal / m_fMass, -1.f * R1crossN, -1.f * Normal / Contact.m_fMass, R2crossN, Contact.m_fInverseInertia, fBias, e_KarunKushTucker);
//
//	float fError = (Contact.m_fMultipliers[0] - fSavedMultiplier);
//
//	m_LinearMomentum += Normal * fError;
//	m_AngularMomentum -= m_InverseInertiaTensor * R1crossN * fError;
//
//	return fError * fError;
//}
//
//
//float CRigidbody::SolveFrictionConstraint(float fFriction, SContact& Contact)
//{
//	float3 R1crossU1 = float3::cross(Contact.m_Distance1, Contact.m_Tangent[0]);
//	float3 R2crossU1 = float3::cross(Contact.m_Distance2, Contact.m_Tangent[0]);
//	float3 R1crossU2 = float3::cross(Contact.m_Distance1, Contact.m_Tangent[1]);
//	float3 R2crossU2 = float3::cross(Contact.m_Distance2, Contact.m_Tangent[1]);
//
//	float fSavedMultiplier1 = Contact.m_fMultipliers[1];
//	Contact.m_fMultipliers[1] = ComputeMultiplier(Contact.m_Tangent[0] / m_fMass, -1.f * R1crossU1, -1.f * Contact.m_Tangent[0] / Contact.m_fMass, R2crossU1, Contact.m_fInverseInertia, 0.f, e_Lagrange);
//
//	float fError1 = (Contact.m_fMultipliers[1] - fSavedMultiplier1);
//
//	m_LinearMomentum += fFriction * Contact.m_Tangent[0] * fError1;
//	m_AngularMomentum -= fFriction * m_InverseInertiaTensor * R1crossU1 * fError1;
//
//	float fSavedMultiplier2 = Contact.m_fMultipliers[2];
//	Contact.m_fMultipliers[2] = ComputeMultiplier(Contact.m_Tangent[1] / m_fMass, -1.f * R1crossU2, -1.f * Contact.m_Tangent[1] / Contact.m_fMass, R2crossU2, Contact.m_fInverseInertia, 0.f, e_Lagrange);
//
//	float fError2 = (Contact.m_fMultipliers[2] - fSavedMultiplier2);
//
//	m_LinearMomentum += fFriction * Contact.m_Tangent[1] * fError2;
//	m_AngularMomentum -= fFriction * m_InverseInertiaTensor * R1crossU2 * fError2;
//
//	return fFriction * (fError1 * fError1 + fError2 * fError2);
//}
//
//
//float CRigidbody::ComputeMultiplier(float3& n1, float3& u1, float3& n2, float3& u2, float3x3& InverseInertia, float fBias, EConstraintType eType)
//{
//	float K = -(float3::dotproduct(n1, n1) + float3::dotproduct(n2, n2) + float3::dotproduct(u1, (m_InverseInertiaTensor * u1)) + float3::dotproduct(u2, (InverseInertia * u2)));
//
//	float Lambda = (float3::dotproduct(n1, m_LinearMomentum) + float3::dotproduct(u1, m_AngularMomentum) + fBias) / K;
//
//	if (eType == e_KarunKushTucker)
//		Lambda = MAX(0.f, Lambda);
//
//	return Lambda;
//}
