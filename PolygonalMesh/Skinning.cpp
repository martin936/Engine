#include "Engine/Engine.h"
#include "PolygonalMesh.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"




CSkinner::CSkinner()
{
	m_pData.clear();
}


CSkinner::~CSkinner()
{
	std::vector<SData*>::iterator it;

	for (it = m_pData.begin(); it < m_pData.end(); it++)
	{
		delete *it;
	}

	m_pData.clear();
}



void CSkinner::Add(PacketList* pPacketList)
{
	Packet* pPacket = NULL;

	std::vector<Packet>::iterator it;
	std::vector<Packet>& pList = pPacketList->m_pPackets;

	for (it = pList.begin(); it < pList.end(); it++)
	{
		Packet& pPacket = (*it);

		SData* pData					= new SData;
		pData->m_pPacket				= &pPacket;
		pData->m_nStride				= pPacket.m_nStride;

		m_pData.push_back(pData);
	}
}



void CSkinner::SkinAll(CPhysicalMesh* pPhysicalMesh)
{
	std::vector<SData*>::iterator it;

	for (it = m_pData.begin(); it < m_pData.end(); it++)
	{
		SData* pData = *it;

		unsigned int nVertexCount = pData->m_pPacket->m_nNumVertex;//pData->m_pMesh->GetVertexCount();
		unsigned int i, j, k;

		pData->m_pSkinningData = new SSkinningData[nVertexCount];

		TVector4 Triangle[3];
		TVector Pos = { 0.f, 0.f, 0.f };
		TVector Normal;
		TVector Tangent;
		TVector Bitangent;
		TVector NextPos;
		TVector Normal2;

		TVector U, V, W, N, Point;

		float fDist = 0.f;
		float fRadius = 0.f;

		float* pVertexData = nullptr;//pData->m_pPacket->m_pData;
		unsigned int nStride = pData->m_pPacket->m_nStride;

		for (i = 0; i < nVertexCount; i++)
		{
			Copy(NextPos,	&(pVertexData)[nStride * i]);
			Copy(Normal,	&(pVertexData)[nStride * i + 3]);
			Copy(Tangent,	&(pVertexData)[nStride * i + 6]);
			Copy(Bitangent, &(pVertexData)[nStride * i + 9]);

			fRadius = Length(NextPos);

			Normalize(NextPos);
			Scale(NextPos, 1000.f);

			bool bSuccess = false;

			for (j = 0; j < (unsigned int)pPhysicalMesh->m_nTriangleCount; j++)
			{
				for (k = 0; k < 3; k++)
				{
					Copy(Triangle[k], &pPhysicalMesh->m_pVertexBuffer[pPhysicalMesh->m_nStride * pPhysicalMesh->m_pIndexBuffer[3 * j + k]]);
				}

				if (Maths::SegTriangleIntersection(Pos, NextPos, Triangle, &fDist))
				{
					Normalize(NextPos);
					Scale(NextPos, fDist);

					Subi(U, Triangle[1], Triangle[0]);
					Subi(V, Triangle[2], Triangle[0]);
					Subi(Point, NextPos, Triangle[0]);

					CrossProduct(N, U, V);
					CrossProduct(W, U, N);
					Normalize(N);

					float invdet = 1.f / (DotProduct(U, U) * DotProduct(V, W) - DotProduct(U, V) * DotProduct(U, W));

					pData->m_pSkinningData[i].m_nTriangleID = j;
					pData->m_pSkinningData[i].m_fu = (DotProduct(Point, U) * DotProduct(V, W) - (DotProduct(Point, W) * DotProduct(U, V))) * invdet;
					pData->m_pSkinningData[i].m_fv = (DotProduct(Point, W) * DotProduct(U, U) - (DotProduct(Point, U) * DotProduct(U, W))) * invdet;
					pData->m_pSkinningData[i].m_fRadius = fRadius / fDist;

					pData->m_pSkinningData[i].m_fTangent[0] = (DotProduct(Tangent, U) * DotProduct(V, W) - (DotProduct(Tangent, W) * DotProduct(U, V))) * invdet;
					pData->m_pSkinningData[i].m_fTangent[1] = (DotProduct(Tangent, W) * DotProduct(U, U) - (DotProduct(Tangent, U) * DotProduct(U, W))) * invdet;
					pData->m_pSkinningData[i].m_fTangent[2] = DotProduct(Tangent, N);

					pData->m_pSkinningData[i].m_fBitangent[0] = (DotProduct(Bitangent, U) * DotProduct(V, W) - (DotProduct(Bitangent, W) * DotProduct(U, V))) * invdet;
					pData->m_pSkinningData[i].m_fBitangent[1] = (DotProduct(Bitangent, W) * DotProduct(U, U) - (DotProduct(Bitangent, U) * DotProduct(U, W))) * invdet;
					pData->m_pSkinningData[i].m_fBitangent[2] = DotProduct(Bitangent, N);

					CrossProduct(Normal2, Tangent, Bitangent);

					pData->m_pSkinningData[i].m_fNormalSign = DotProduct(Normal2, Normal) > 0.f ? 1.f : -1.f;

					Scale(U, pData->m_pSkinningData[i].m_fu);
					Scale(V, pData->m_pSkinningData[i].m_fv);
					Addi(U, V);
					Addi(U, Triangle[0]);
					Scale(U, fRadius / fDist);

					bSuccess = true;

					break;
				}
			}

			if (!bSuccess)
				bSuccess = false;
		}

		//pData->m_nSkinningDataID = CDeviceManager::CreateVertexBuffer(pData->m_pSkinningData, nVertexCount * sizeof(SSkinningData), false);

		pData->m_nSkinnedVertexCount = nVertexCount;
	}
}

