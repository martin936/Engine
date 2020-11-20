#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Maths/Maths.h"
#include "Obstacles.h"
#include <string.h>



CObstacle::CObstacle(float3& Center, float3& AABB)
{
	m_pWalls = NULL;
	m_nWallCount = 0;
	m_bUseOnCPU = true;
	m_bUseOnGPU = false;
	m_bEnabled = true;
	Copy(m_Center, &Center.x);
	m_AABB = AABB;

	m_fBoundingSphereRadius = (Center - 0.5f * float3(AABB.x, AABB.y, AABB.z)).length();
}


CObstacle::CObstacle(CMesh* pMesh, bool bCPU, bool bGPU)
{
	int i, j;
	int nTriangleCount = pMesh->m_nTriangleCount;
	int nIndex;
	TVector Vertex[3];
	TVector Edges[2];
	TVector Normal;
	TVector HintNormal;
	TVector CurrentNormal;

	m_bUseOnCPU = bCPU;
	m_bUseOnGPU = bGPU;

	m_pWalls = new SWall[nTriangleCount];
	memset(m_pWalls, 0, nTriangleCount * sizeof(SWall));
	m_nWallCount = nTriangleCount;

	for (i = 0; i < nTriangleCount; i++)
	{
		memset(HintNormal, 0, 3 * sizeof(float));

		for (j = 0; j < 3; j++)
		{
			nIndex = pMesh->m_pIndexBuffer[3 * i + j];

			memcpy(&m_pWalls[i].m_Vertex[j], &pMesh->m_pVertexBuffer[nIndex * pMesh->m_nStride], 3 * sizeof(float));

			memcpy(CurrentNormal, &pMesh->m_pVertexBuffer[nIndex * pMesh->m_nStride + 3], 3 * sizeof(float));

			Addi(HintNormal, CurrentNormal);
			Copy(Vertex[j], m_pWalls[i].m_Vertex[j]);
		}

		Subi(Edges[0], Vertex[1], Vertex[0]);
		Subi(Edges[1], Vertex[2], Vertex[0]);
		Normalize(Edges[0]);
		Normalize(Edges[1]);

		CrossProduct(Normal, Edges[0], Edges[1]);

		if (DotProduct(Normal, HintNormal) < 0.f)
			Scale(Normal, -1.f);

		Normalize(Normal);
		CrossProduct(Edges[1], Normal, Edges[0]);

		Copy(m_pWalls[i].m_Normal, Normal);
		Copy(m_pWalls[i].m_TangentBasis[0], Edges[0]);
		Copy(m_pWalls[i].m_TangentBasis[1], Edges[1]);
	}

	Copy(m_Center, pMesh->m_Center.v());
	m_fBoundingSphereRadius = pMesh->m_fBoundingSphereRadius;

	m_bEnabled = true;
}


int CObstacle::CheckCollision(TVector CurrentPos, TVector LastPos)
{
	int i;
	TVector Dist;
	Subi(Dist, CurrentPos, m_Center);

	if (Length(Dist) < m_fBoundingSphereRadius)
	{
		for (i = 0; i < m_nWallCount; i++)
		{
			if (Maths::SegTriangleIntersection(LastPos, CurrentPos, GetWall(i)->m_Vertex))
				return i;
		}
	}

	return -1;
}


CObstacle::~CObstacle()
{
	if (m_pWalls != NULL)
		delete[] m_pWalls;

	m_pWalls = NULL;
}


void CObstacle::ComputeBoundingVolumes()
{
	float3 Center = float3(0.f, 0.f, 0.f);
	float3 CurrentPoint;
	float fXMax = -1e9f, fYMax = -1e9f, fZMax = -1e9f;
	float fXMin = 1e9f, fYMin = 1e9f, fZMin = 1e9f;
	float fCurrentRadius;
	m_fBoundingSphereRadius = 0.f;

	Copy(Center.v(), m_Center);

	for (int i = 0; i < m_nWallCount; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Copy(CurrentPoint.v(), m_pWalls[i].m_Vertex[j]);
			
			if (CurrentPoint.x > fXMax)
				fXMax = CurrentPoint.x;

			if (CurrentPoint.x < fXMin)
				fXMin = CurrentPoint.x;

			if (CurrentPoint.y > fYMax)
				fYMax = CurrentPoint.y;

			if (CurrentPoint.y < fYMin)
				fYMin = CurrentPoint.y;

			if (CurrentPoint.z > fZMax)
				fZMax = CurrentPoint.z;

			if (CurrentPoint.z < fZMin)
				fZMin = CurrentPoint.z;
		}
	}

	m_AABB = float3(fXMax - fXMin, fYMax - fYMin, fZMax - fZMin);

	Center = 0.5f * float3(fXMin + fXMax, fYMin + fYMax, fZMin + fZMax);

	Copy(m_Center, Center.v());

	for (int i = 0; i < m_nWallCount; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			Copy(CurrentPoint.v(), m_pWalls[i].m_Vertex[j]);
			fCurrentRadius = (CurrentPoint - Center).length();

			if (fCurrentRadius > m_fBoundingSphereRadius)
				m_fBoundingSphereRadius = fCurrentRadius;
		}
	}

	m_fBoundingSphereRadius += 0.1f;
}