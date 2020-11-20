#include "Engine/Physics/Physics.h"
#include "Rigidbodies.h"
#include "Engine/Renderer/DebugDraw/DebugDraw.h"
#include "Engine/Editor/Adjustables/Adjustables.h"

ADJUSTABLE("Draw Debug", bool, gs_bDrawDebug, false, false, true, "Physics/Rigidbodies")


bool IsPointInBox(float3& Point, SOrientedBox& Box)
{
	float3 Dist = Point - Box.m_Position;

	for (int i = 0; i < 3; i++)
	{
		if (fabs(float3::dotproduct(Dist, Box.m_Basis[i])) > 0.5f * Box.m_Dim[i] + 1e-3f)
			return false;
	}

	return true;
}


void AddContactPoint(float3& Position, SOrientedBox& A, SSphere& S, float3& Normal, float fDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass, int nCount)
{

	pContacts[nCount].m_Distance1 = Position - A.m_Position;
	pContacts[nCount].m_Distance2 = Position - S.m_Position;
	pContacts[nCount].m_Normal = Normal;
	pContacts[nCount].m_Tangent[0] = float3(0.f, 1.f, 0.f);
	pContacts[nCount].m_Tangent[1] = float3::cross(Normal, float3(0.f, 1.f, 0.f));
	pContacts[nCount].m_Tangent[1].normalize();
	pContacts[nCount].m_fDepth = fDepth;
	pContacts[nCount].m_fMultipliers[0] = pContacts[nCount].m_fMultipliers[1] = pContacts[nCount].m_fMultipliers[2] = 0.f;
	pContacts[nCount].m_fMass = fMass;
	pContacts[nCount].m_fInverseInertia = Inertia;

	if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
		CDebugDraw::DrawVector(Position, Normal, 0.2f, float4(0.f, 1.f, 0.f, 1.f));
}



void AddContactPoint(float3& Position, SOrientedBox& A, SOrientedBox& B, float3& Axis, float3& Normal, float fMinDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass, int nCount)
{
	pContacts[nCount].m_Distance1 = Position - A.m_Position;
	pContacts[nCount].m_Distance2 = Position - B.m_Position;
	pContacts[nCount].m_Normal = Normal;
	pContacts[nCount].m_Tangent[0] = float3(0.f, 1.f, 0.f);
	pContacts[nCount].m_Tangent[1] = float3::cross(Normal, float3(0.f, 1.f, 0.f));
	pContacts[nCount].m_Tangent[1].normalize();
	pContacts[nCount].m_fDepth = float3::dotproduct(Position, Axis) - fMinDepth;
	pContacts[nCount].m_fMultipliers[0] = pContacts[nCount].m_fMultipliers[1] = pContacts[nCount].m_fMultipliers[2] = 0.f;
	pContacts[nCount].m_fMass = fMass;
	pContacts[nCount].m_fInverseInertia = Inertia;

	if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
		CDebugDraw::DrawVector(Position, Normal, 0.2f, float4(0.f, 1.f, 0.f, 1.f));
}




void FindContactPointOnEdge(float3& Point, float3& Edge, SOrientedBox& A, SOrientedBox& B, float3& Axis, float3& Normal, float fMinDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass, int* nCount)
{
	float3 Dir, TestPoint, u;
	float t, dot, tmax = 0.f, tmin = 1e8f;
	bool contact = false;

	u = Edge;
	u.normalize();
	float dim = Edge.length();

	for (int i = 0; i < 3; i++)
	{
		if ((fabs(float3::dotproduct(Point - B.m_Position, B.m_Basis[i])) - 0.5f * B.m_Dim[i]) *
			(fabs(float3::dotproduct(Point + Edge - B.m_Position, B.m_Basis[i])) - 0.5f * B.m_Dim[i]) > 0.f)
			continue;

		dot = float3::dotproduct(B.m_Basis[i], u);

		if (dot < 0.f)
			Dir = -1.f * B.m_Basis[i];

		else
			Dir = B.m_Basis[i];

		for (int j = -1; j <= 1; j += 2)
		{
			t = (j * 0.5f * B.m_Dim[i] - float3::dotproduct(Point - B.m_Position, Dir)) / fabs(dot);

			if (t < 0.f || t > dim)
				continue;

			TestPoint = Point + t * u;

			if (fabs(float3::dotproduct(TestPoint - B.m_Position, B.m_Basis[(i + 1) % 3])) > 0.5f * B.m_Dim[(i + 1) % 3])
				continue;

			if (fabs(float3::dotproduct(TestPoint - B.m_Position, B.m_Basis[(i + 2) % 3])) > 0.5f * B.m_Dim[(i + 2) % 3])
				continue;

			if (t < tmin)
				tmin = t;

			if (t > tmax)
				tmax = t;

			contact = true;
		}
	}

	if (contact)
	{
		if (tmin > 0.f && tmin < dim)
		{
			AddContactPoint(Point + tmin * u, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, *nCount);
			(*nCount)++;

			if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
				CDebugDraw::DrawVector(Point, u, tmin, float4(1.f, 0.f, 0.f, 1.f));
		}

		if (tmax > 0.f && tmax < dim)
		{
			AddContactPoint(Point + tmax * u, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, *nCount);
			(*nCount)++;

			if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
				CDebugDraw::DrawVector(Point, u, tmax, float4(1.f, 0.f, 0.f, 1.f));
		}
	}
}



int Collisions::ExtractContactPoints(SOrientedBox& A, SOrientedBox& B, float3& Axis, float fMinDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass)
{
	int nCount = 0;


	float3 InnerPoint = A.m_Position;
	float3 ClosestEdge, ClosestFace, Offset;
	int i, j;
	float fMinEdge = 1.f, fMinFace = 1.f, fDot;

	for (i = 0; i < 3; i++)
	{
		fDot = float3::dotproduct(A.m_Basis[i], Axis);

		if (fDot > 0.f)
			InnerPoint += 0.5f * A.m_Dim[i] * A.m_Basis[i];

		else
			InnerPoint -= 0.5f * A.m_Dim[i] * A.m_Basis[i];

		fDot = fabs(fDot);

		if (fDot < fMinEdge)
		{
			fMinEdge = fDot;
			ClosestEdge = A.m_Dim[i] * A.m_Basis[i];
			j = i;
		}
	}

	for (i = 0; i < 3; i++)
	{
		fDot = fabs(float3::dotproduct(A.m_Basis[i], Axis));

		if (fDot >= fMinEdge && fDot < fMinFace && j != i)
		{
			fMinFace = fDot;
			ClosestFace = A.m_Dim[i] * A.m_Basis[i];
		}
	}

	if (float3::dotproduct(ClosestEdge, A.m_Position - InnerPoint) < 0.f)
		ClosestEdge = -1.f * ClosestEdge;

	if (float3::dotproduct(ClosestFace, A.m_Position - InnerPoint) < 0.f)
		ClosestFace = -1.f * ClosestFace;

	float3 FourthPoint = InnerPoint + ClosestEdge + ClosestFace;

	ClosestEdge += InnerPoint;
	ClosestFace += InnerPoint;

	bool bIsFaceAligned = false;

	if (fabs(float3::dotproduct(InnerPoint, Axis) - float3::dotproduct(ClosestEdge, Axis)) < 1e-3f && fabs(float3::dotproduct(InnerPoint, Axis) - float3::dotproduct(ClosestFace, Axis)) < 1e-3f)
		bIsFaceAligned = true;

	fMinEdge = 0.f;
	float3 Normal, Edge;

	for (i = 0; i < 3; i++)
	{
		fDot = fabs(float3::dotproduct(B.m_Basis[i], Axis));

		if (fDot > fMinEdge)
		{
			fMinEdge = fDot;
			Normal = (float3::dotproduct(B.m_Basis[i], A.m_Position - B.m_Position) > 0.f ? 1.f : -1.f) * B.m_Basis[i];
		}
	}

	Normal.normalize();

	bool bInside[4] = { false };

	if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
	{
		//CDebugDraw::DrawPoint(InnerPoint, 0.005f, float4(0.f, 0.f, 1.f, 1.f));
		//CDebugDraw::DrawPoint(ClosestEdge, 0.005f, float4(0.f, 0.f, 1.f, 1.f));
		//CDebugDraw::DrawPoint(ClosestFace, 0.005f, float4(0.f, 0.f, 1.f, 1.f));
		//CDebugDraw::DrawPoint(FourthPoint, 0.005f, float4(0.f, 0.f, 1.f, 1.f));
	}

	if (IsPointInBox(InnerPoint, B))
	{
		AddContactPoint(InnerPoint, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, nCount++);
		bInside[0] = true;
	}

	if (IsPointInBox(ClosestEdge, B))
	{
		AddContactPoint(ClosestEdge, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, nCount++);
		bInside[1] = true;
	}

	if (IsPointInBox(ClosestFace, B))
	{
		AddContactPoint(ClosestFace, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, nCount++);
		bInside[2] = true;
	}

	if (IsPointInBox(FourthPoint, B))
	{
		AddContactPoint(FourthPoint, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, nCount++);
		bInside[3] = true;
	}

	Edge = ClosestEdge - InnerPoint;
	FindContactPointOnEdge(InnerPoint, Edge, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, &nCount);

	Edge = ClosestFace - InnerPoint;
	FindContactPointOnEdge(InnerPoint, Edge, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, &nCount);

	Edge = FourthPoint - ClosestFace;
	FindContactPointOnEdge(ClosestFace, Edge, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, &nCount);

	Edge = FourthPoint - ClosestEdge;
	FindContactPointOnEdge(ClosestEdge, Edge, A, B, Axis, Normal, fMinDepth, pContacts, Inertia, fMass, &nCount);

	return nCount;
}


int Collisions::ExtractContactPoints(SOrientedBox& A, SSphere& S, float3& Axis, float fDepth, CRigidbody::SContact* pContacts, float3x3& Inertia, float fMass)
{
	float3 u, Point, Dir;

	if (float3::dotproduct(Axis, S.m_Position - A.m_Position) < 0.f)
		u = -1.f * Axis;

	else
		u = Axis;

	float dot, t;

	u.normalize();
	Point = S.m_Position - S.m_fRadius * u;

	for (int i = 0; i < 3; i++)
	{
		if ((fabs(float3::dotproduct(Point - A.m_Position, A.m_Basis[i])) - 0.5f * A.m_Dim[i]) *
			(fabs(float3::dotproduct(S.m_Position - A.m_Position, A.m_Basis[i])) - 0.5f * A.m_Dim[i]) > 0.f)
			continue;

		dot = float3::dotproduct(A.m_Basis[i], u);

		if (dot < 0.f)
			Dir = -1.f * A.m_Basis[i];

		else
			Dir = A.m_Basis[i];

		for (int j = -1; j <= 1; j += 2)
		{
			t = (j * 0.5f * A.m_Dim[i] - float3::dotproduct(Point - A.m_Position, Dir)) / fabs(dot);

			if (t < 0.f || t > S.m_fRadius)
				continue;

			AddContactPoint(Point + t * u, A, S, -1.f *u, fDepth, pContacts, Inertia, fMass, 0);
			AddContactPoint(Point + t * u, A, S, -1.f *u, fDepth, pContacts, Inertia, fMass, 1);

			if (gs_bDrawDebug && CPhysicsEngine::ms_bOncePerRenderedFrame)
				CDebugDraw::DrawVector(Point, u, t, float4(1.f, 0.f, 0.f, 1.f));

			return 2;
		}
	}

	return 0;
}

