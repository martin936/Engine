#include "PolygonalMesh.h"
#include "Engine/Renderer/Packets/Stream.h"
#include "Engine/Device/DeviceManager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>


CPhysicalMesh::CPhysicalMesh(CMesh* pMesh)
{
	m_nTriangleCount	= pMesh->m_nTriangleCount;
	m_nVertexCount		= pMesh->m_nVertexCount;
	m_nEdgesCount		= 3 * pMesh->m_nVertexCount;
	m_nStreams			= e_POSITIONMASK | e_NORMALMASK;
	m_nStride			= sizeof(SPhysicalVertex) / sizeof(float);

	m_pVertexBuffer		= new float[m_nStride * m_nVertexCount]();
	m_pIndexBuffer		= new unsigned int[3 * m_nTriangleCount]();

	JoinIdenticalVertices(pMesh);

	m_pNeighbourIndex	= new unsigned int*[m_nVertexCount];
	m_pNeighbourCount	= new unsigned int[m_nVertexCount]();
	m_pEdgesIndex		= new unsigned int[2 * m_nEdgesCount]();

	SPacketInfo PacketInfo;
	PacketInfo.m_bIsUVMapped = false;
	PacketInfo.m_nStartIndex = 0;
	PacketInfo.m_nPacketSize = m_nTriangleCount;
	PacketInfo.m_nStreams = e_POSITIONMASK | e_NORMALMASK;
	PacketInfo.m_pMaterial = CMaterial::GetMaterial("None");
	m_PacketInfo.push_back(PacketInfo);

	for (int i = 0; i < m_nVertexCount; i++)
		m_pNeighbourIndex[i] = NULL;

	FillEdgesBuffer();
	FillNeighbours();

	m_bLoaded = false;

	m_pSkinner = new CSkinner();
}


bool CompareVertices(float* v1, float* v2)
{
	for (int i = 0; i < 3; i++)
	{
		if (fabs(v1[i] - v2[i]) > 1e-5f)
			return false;
	}

	return true;
}


void CPhysicalMesh::JoinIdenticalVertices(CMesh* pMesh)
{
	int* nRemapIndices = new int[m_nVertexCount];

	int nStride = pMesh->m_nStride;
	bool bDuplicate = false;
	int i, j;
	int nVertexCount = 0;

	float Zero[3] = { 0.f, 0.f, 0.f };

	for (i = 0; i < m_nVertexCount; i++)
		nRemapIndices[i] = i;

	for (i = 0; i < m_nVertexCount; i++)
	{
		bDuplicate = false;

		for (j = 0; j < i; j++)
		{
			if (CompareVertices(&pMesh->m_pVertexBuffer[i * nStride], &pMesh->m_pVertexBuffer[j * nStride]))
			{
				bDuplicate = true;
				break;
			}
		}

		if (bDuplicate)
			nRemapIndices[i] = nRemapIndices[j];

		else
		{
			Copy(&m_pVertexBuffer[m_nStride * nVertexCount],		&pMesh->m_pVertexBuffer[nStride * i]);
			Copy(&m_pVertexBuffer[m_nStride * nVertexCount + 3],	&pMesh->m_pVertexBuffer[nStride * i + 3]);
			Copy(&m_pVertexBuffer[m_nStride * nVertexCount + 6],	Zero);
			Copy(&m_pVertexBuffer[m_nStride * nVertexCount + 9],	Zero);

			nRemapIndices[i] = nVertexCount;

			nVertexCount++;
		}
	}

	for (i = 0; i < 3 * m_nTriangleCount; i++)
	{
		m_pIndexBuffer[i] = nRemapIndices[pMesh->m_pIndexBuffer[i]];
	}

	m_nVertexCount = nVertexCount;

	float *pVertexBuffer = new float[m_nStride * m_nVertexCount];

	memcpy(pVertexBuffer, m_pVertexBuffer, m_nVertexCount * m_nStride * sizeof(float));

	delete[] m_pVertexBuffer;
	m_pVertexBuffer = pVertexBuffer;
}


CPhysicalMesh::CPhysicalMesh(CPhysicalMesh::EMeshTemplate eTemplate, int nTesselation)
{
	assert(nTesselation >= 0);
	if (eTemplate != e_Icosphere)
		assert(!"Not implemented");

	m_nTriangleCount	= (int)powf(4.f, 1.f * nTesselation) * 20;
	m_nVertexCount		= m_nTriangleCount / 2 + 2;
	m_nEdgesCount		= m_nVertexCount * 3;
	m_nStreams			= e_POSITIONMASK | e_NORMALMASK;
	m_nStride			= sizeof(SPhysicalVertex) / sizeof(float);

	m_pVertexBuffer		= new float[m_nStride * m_nVertexCount]();
	m_pIndexBuffer		= new unsigned int[3 * m_nTriangleCount]();
	m_pNeighbourIndex	= new unsigned int*[m_nVertexCount];
	m_pNeighbourCount	= new unsigned int[m_nVertexCount]();
	m_pEdgesIndex		= new unsigned int[2 * m_nEdgesCount]();

	SPacketInfo PacketInfo;
	PacketInfo.m_bIsUVMapped = false;
	PacketInfo.m_nStartIndex = 0;
	PacketInfo.m_nPacketSize = m_nTriangleCount;
	PacketInfo.m_nStreams = e_POSITIONMASK | e_NORMALMASK;
	PacketInfo.m_pMaterial = CMaterial::GetMaterial("Player");
	m_PacketInfo.push_back(PacketInfo);

	for (int i = 0; i < m_nVertexCount; i++)
		m_pNeighbourIndex[i] = NULL;

	BuildIcosahedron();

	for (int i = 0; i < nTesselation; i++)
		Tesselate();

	for (int i = 0; i < m_nVertexCount; i++)
		Normalize(&m_pVertexBuffer[m_nStride * i]);

	ComputeNormals();
	FillEdgesBuffer();
	FillNeighbours();

	m_bLoaded = false;

	m_pSkinner = new CSkinner();
}


CPhysicalMesh::CPhysicalMesh()
{
	m_pEdgesIndex = NULL;
	m_pNeighbourIndex = NULL;
	m_pNeighbourCount = NULL;

	m_pSkinner = new CSkinner();
}


CPhysicalMesh::~CPhysicalMesh()
{
	delete[] m_pEdgesIndex;
	delete[] m_pNeighbourCount;

	for (int i = 0; i < m_nVertexCount; i++)
		delete m_pNeighbourIndex[i];

	delete[] m_pNeighbourIndex;

	Unload();

	delete m_pSkinner;

	std::vector<CMesh*>::iterator it;

	for (it = ms_pMeshesList.begin(); it < ms_pMeshesList.end(); it++)
	{
		if (*it == this)
		{
			ms_pMeshesList.erase(it);
			break;
		}
	}
}


void CPhysicalMesh::ComputeNormals()
{
	for (int i = 0; i < m_nVertexCount; i++)
		Normalize(&m_pVertexBuffer[i * m_nStride + 3], &m_pVertexBuffer[i * m_nStride]);
}


void CPhysicalMesh::FillNeighbours()
{
	int i, j, k;
	int index1, index2;
	bool bNewNeighbour;
	int* nCurrentNeighbourCount = new int[m_nVertexCount]();

	for (i = 0; i < m_nVertexCount; i++)
		m_pNeighbourIndex[i] = new unsigned int[m_pNeighbourCount[i]];
	
	for (i = 0; i < m_nTriangleCount; i++)
	{
		for (j = 0; j < 3; j++)
		{
			index1 = m_pIndexBuffer[3 * i + j];
			index2 = m_pIndexBuffer[3 * i + (j + 1) % 3];

			bNewNeighbour = true;
			for (k = 0; k < nCurrentNeighbourCount[index1]; k++)
				if (m_pNeighbourIndex[index1][k] == index2)
				{
					bNewNeighbour = false;
					break;
				}

			if (bNewNeighbour)
			{
				m_pNeighbourIndex[index1][nCurrentNeighbourCount[index1]++] = index2;
				m_pNeighbourIndex[index2][nCurrentNeighbourCount[index2]++] = index1;
			}
		}
	}

	SortNeighbours();
}


void CPhysicalMesh::FillEdgesBuffer()
{
	int i, j, k;
	int index1, index2;
	bool bEdgeExists = false;
	m_nEdgesCount = 0;

	for (i = 0; i < m_nTriangleCount; i++)
	{
		for (j = 0; j < 3; j++)
		{
			index1 = m_pIndexBuffer[3 * i + j];
			index2 = m_pIndexBuffer[3 * i + (j + 1) % 3];

			bEdgeExists = false;

			for (k = 0; k < m_nEdgesCount; k++)
			{
				if (m_pEdgesIndex[k * 2] != index1 && m_pEdgesIndex[k * 2 + 1] != index1)
					continue;

				if (m_pEdgesIndex[k * 2] != index2 && m_pEdgesIndex[k * 2 + 1] != index2)
					continue;

				bEdgeExists = true;
				break;
			}

			if (!bEdgeExists)
			{
				m_pEdgesIndex[m_nEdgesCount * 2] = index1;
				m_pEdgesIndex[m_nEdgesCount * 2 + 1] = index2;
				m_pNeighbourCount[index1]++;
				m_pNeighbourCount[index2]++;
				m_nEdgesCount++;
			}
		}
	}
}


void CPhysicalMesh::BuildIcosahedron()
{
	float t = (1.f + sqrtf(5.f)) / 2.f;
	int nCurrentVertex = 0;

	m_nVertexCount = 12;
	m_nTriangleCount = 20;

	SetVertex(0, float3(-1.f, t, 0.f));
	SetVertex(1, float3(1.f, t, 0.f));
	SetVertex(2, float3(-1.f, -t, 0.f));
	SetVertex(3, float3(1.f, -t, 0.f));

	SetVertex(4, float3(0.f, -1.f, t));
	SetVertex(5, float3(0.f, 1.f, t));
	SetVertex(6, float3(0.f, -1.f, -t));
	SetVertex(7, float3(0.f, 1.f, -t));

	SetVertex(8, float3(t, 0.f, -1.f));
	SetVertex(9, float3(t, 0.f, 1.f));
	SetVertex(10, float3(-t, 0.f, -1.f));
	SetVertex(11, float3(-t, 0.f, 1.f));

	SetTriangle(0, 0, 11, 5);
	SetTriangle(1, 0, 5, 1);
	SetTriangle(2, 0, 1, 7);
	SetTriangle(3, 0, 7, 10);
	SetTriangle(4, 0, 10, 11);

	SetTriangle(5, 1, 5, 9);
	SetTriangle(6, 5, 11, 4);
	SetTriangle(7, 11, 10, 2);
	SetTriangle(8, 10, 7, 6);
	SetTriangle(9, 7, 1, 8);

	SetTriangle(10, 3, 9, 4);
	SetTriangle(11, 3, 4, 2);
	SetTriangle(12, 3, 2, 6);
	SetTriangle(13, 3, 6, 8);
	SetTriangle(14, 3, 8, 9);

	SetTriangle(15, 4, 9, 5);
	SetTriangle(16, 2, 4, 11);
	SetTriangle(17, 6, 2, 10);
	SetTriangle(18, 8, 6, 7);
	SetTriangle(19, 9, 8, 1);

	Resize(1.f);
}


void CPhysicalMesh::Tesselate()
{
	int i, j, k;
	float3 Vertices[3];
	int NewIndex[3];
	int index1, index2;
	int max_index;
	int nFormerTriangleCount = m_nTriangleCount;
	bool bVertexExists = false;
	TVector Dist;

	for (i = 0; i < nFormerTriangleCount; i++)
	{
		for (j = 0; j < 3; j++)
		{
			index1 = m_pIndexBuffer[3 * i + j];
			index2 = m_pIndexBuffer[3 * i + (j + 1) % 3];

			for (k = 0; k < 3; k++)
				Vertices[j].v()[k] = 0.5f * (m_pVertexBuffer[index1 * m_nStride + k] + m_pVertexBuffer[index2 * m_nStride + k]);

			max_index = index1 > index2 ? index1 : index2;
			bVertexExists = false;

			for (k = max_index; k < m_nVertexCount; k++)
			{
				Subi(Dist, Vertices[j].v(), &m_pVertexBuffer[k * m_nStride]);
				if (Length(Dist) < 1e-6f)
				{
					bVertexExists = true;
					NewIndex[j] = k;
					break;
				}
			}

			if (!bVertexExists)
			{
				NewIndex[j] = m_nVertexCount;
				SetVertex(m_nVertexCount++, Vertices[j]);
			}
		}

		SetTriangle(m_nTriangleCount++, m_pIndexBuffer[3 * i], NewIndex[0], NewIndex[2]);
		SetTriangle(m_nTriangleCount++, m_pIndexBuffer[3 * i + 1], NewIndex[1], NewIndex[0]);
		SetTriangle(m_nTriangleCount++, m_pIndexBuffer[3 * i + 2], NewIndex[2], NewIndex[1]);

		for (j = 0; j < 3; j++)
			m_pIndexBuffer[3 * i + j] = NewIndex[j];
	}
}


void CPhysicalMesh::Resize(float fRadius)
{
	float3 CurrentPoint, Direction;
	float fCurrentRadius;
	m_Center = 0.f;
	m_fBoundingSphereRadius = 1e30f;

	for (int i = 0; i < m_nVertexCount; i++)
	{
		for (int j = 0; j < 3; j++)
			m_Center.v()[j] += m_pVertexBuffer[m_nStride * i + j];
	}

	m_Center = m_Center / (float)m_nVertexCount;

	for (int i = 0; i < m_nVertexCount; i++)
	{
		memcpy(CurrentPoint.v(), &m_pVertexBuffer[m_nStride * i], 3 * sizeof(float));
		fCurrentRadius = (CurrentPoint - m_Center).length();

		if (fCurrentRadius < m_fBoundingSphereRadius)
			m_fBoundingSphereRadius = fCurrentRadius;
	}

	for (int i = 0; i < m_nVertexCount; i++)
	{
		memcpy(CurrentPoint.v(), &m_pVertexBuffer[m_nStride * i], 3 * sizeof(float));
		CurrentPoint = m_Center + (fRadius / m_fBoundingSphereRadius) * (CurrentPoint - m_Center);
		memcpy(&m_pVertexBuffer[m_nStride * i], CurrentPoint.v(), 3 * sizeof(float));
	}

	m_fBoundingSphereRadius = fRadius;
}


void CPhysicalMesh::SortNeighbours()
{
	int* list;
	int nSortedNeighbours;
	int nNeighbourIndex, nConnexIndex = 0;
	bool bLinked;
	TVector Normal, Edge1, Edge2;

	for (int i = 0; i < m_nVertexCount; i++)
	{
		list = new int[m_pNeighbourCount[i]];
		nSortedNeighbours = 1;

		list[0] = m_pNeighbourIndex[i][0];
		while (nSortedNeighbours < (int)m_pNeighbourCount[i])
		{
			for (int j = 0; j < (int)m_pNeighbourCount[i]; j++)
			{
				nNeighbourIndex = m_pNeighbourIndex[i][j];

				if (nNeighbourIndex == list[nSortedNeighbours - 1])
					continue;

				bLinked = false;
				for (int k = 0; k < (int)m_pNeighbourCount[nNeighbourIndex]; k++)
				{
					if (m_pNeighbourIndex[nNeighbourIndex][k] == list[nSortedNeighbours - 1])
					{
						Subi(Edge1, &m_pVertexBuffer[list[nSortedNeighbours - 1] * m_nStride], &m_pVertexBuffer[i * m_nStride]);
						Subi(Edge2, &m_pVertexBuffer[nNeighbourIndex * m_nStride], &m_pVertexBuffer[i * m_nStride]);
						CrossProduct(Normal, Edge1, Edge2);
						Normalize(Normal);

						if (DotProduct(Normal, &m_pVertexBuffer[i * m_nStride + 3]) > 0.f)
						{
							list[nSortedNeighbours] = nNeighbourIndex;
							nSortedNeighbours++;
							bLinked = true;
						}

						else
							nConnexIndex = nNeighbourIndex;

						break;
					}
				}

				if (bLinked)
					break;
			}

			if (!bLinked)
			{
				list[nSortedNeighbours] = nConnexIndex;
				nSortedNeighbours++;
			}
		}

		for (int j = 0; j < (int)m_pNeighbourCount[i]; j++)
			m_pNeighbourIndex[i][j] = list[j];

		delete[] list;
	}
}


void CPhysicalMesh::Load()
{
	if (!m_bLoaded)
	{
		//m_nVertexBufferID = CDeviceManager::CreateStorageBuffer(m_pVertexBuffer, m_nStride * m_nVertexCount * sizeof(float));
		//m_nIndexBufferID = CDeviceManager::CreateStorageBuffer(m_pIndexBuffer, 3 * m_nTriangleCount * sizeof(unsigned int));

		m_bLoaded = true;
	}
}


void CPhysicalMesh::Unload()
{
	if (m_bLoaded)
	{
		//CDeviceManager::DeleteStorageBuffer(m_nVertexBufferID);
		//CDeviceManager::DeleteStorageBuffer(m_nIndexBufferID);
		m_bLoaded = false;
	}
}

