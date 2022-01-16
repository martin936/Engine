#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Sprites/Sprites.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Packet.h"
#include <cassert>


std::vector<std::vector<Drawable>> CPacketManager::m_pDrawables[2];

std::vector<std::vector<Drawable>>* CPacketManager::m_pDrawablesToFill	= &CPacketManager::m_pDrawables[0];
std::vector<std::vector<Drawable>>* CPacketManager::m_pDrawablesToFlush	= &CPacketManager::m_pDrawables[1];

thread_local int (*CPacketManager::m_pShaderHook)(Packet* packet, void* p_pShaderData) = NULL;

extern unsigned int g_VertexStreamOffsetNoSkin[];
extern unsigned int g_VertexStreamSize[];


void CPacketManager::Init()
{
	m_pDrawables[0].resize(MAX_DRAWABLE_LIST_COUNT);
	m_pDrawables[1].resize(MAX_DRAWABLE_LIST_COUNT);
}



void CPacketManager::Terminate()
{

}


void CPacketManager::UpdateBeforeFlush()
{
	std::vector<std::vector<Drawable>>* tmp = CPacketManager::m_pDrawablesToFill;
	CPacketManager::m_pDrawablesToFill = CPacketManager::m_pDrawablesToFlush;
	CPacketManager::m_pDrawablesToFlush = tmp;

	for (int i = 0; i < MAX_DRAWABLE_LIST_COUNT; i++)
		(*CPacketManager::m_pDrawablesToFill)[i].clear();
}



Packet::Packet(void)
{
	m_nVertexDeclaration = 0U;

	m_nFirstIndex = 0;
	m_nNumIndex = 0;
	m_nTriangleCount = 0;
	m_nFirstVertex = 0;
	m_nNumVertex = 0;
	m_nStride = 0U;

	m_VertexBuffer = INVALIDHANDLE;
	m_IndexBuffer = INVALIDHANDLE;

	m_pMaterial = NULL;
	m_pShaderHook = NULL;
	m_pMesh = NULL;
	m_eTopology = e_TriangleList;
	m_Center = 0.f;
	m_fBoundingSphereRadius = 0.f;

	m_nViewportMask = 0xffffffffffffffff;

	m_pTransformedBones = NULL;
	m_nBonesCount = 0;
}



Packet::Packet(CMesh* pMesh, CMesh::SPacketInfo pInfo)
{
	m_eType = e_EnginePacket;

	m_pMesh = pMesh;

	m_nFirstIndex = 0;

	size_t uSize = (pMesh->m_nVertexCount + 7) >> 3;
	unsigned char* bUseVertex = new unsigned char[uSize];
	memset(bUseVertex, 0, uSize);
	int nLastIndex = pInfo.m_nStartIndex + pInfo.m_nPacketSize;
	unsigned int index;
	int nVertexCount = 0;
	unsigned int* pIndexBuffer = new unsigned int[3 * pInfo.m_nPacketSize];

	for (int i = pInfo.m_nStartIndex; i < nLastIndex; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			index = (unsigned int)pMesh->m_pIndexBuffer[3 * i + j];
			if (!(bUseVertex[index >> 3] & (1 << (index % 8))))
				bUseVertex[index >> 3] |= (1 << (index % 8));
		}
	}

	unsigned int* uRemappedIndices = new unsigned int[pMesh->m_nVertexCount];

	for (unsigned int i = 0; i < (unsigned int)pMesh->m_nVertexCount; i++)
	{
		if (bUseVertex[i >> 3] & (1 << (i % 8)))
		{
			uRemappedIndices[i] = nVertexCount;
			nVertexCount++;
		}
	}

	for (int i = pInfo.m_nStartIndex; i < nLastIndex; i++)
		for (int j = 0; j < 3; j++)
			pIndexBuffer[3 * (i - pInfo.m_nStartIndex) + j] = uRemappedIndices[pMesh->m_pIndexBuffer[3 * i + j]];

	m_IndexBuffer = CResourceManager::CreateIndexBuffer(3 * pInfo.m_nPacketSize * sizeof(unsigned int), pIndexBuffer);

	delete[] pIndexBuffer;

	int nOffset = 0;

	int		inputStride = pMesh->m_nStride;
	int		stride		= g_VertexStrideNoSkin;

	char*	pData = new char[stride * nVertexCount];

	m_nVertexDeclaration = pMesh->m_nStreams;

	m_Center = float3(0.f, 0.f, 0.f);
	m_fBoundingSphereRadius = 0.f;
	float3 Point;
	float3 AABB_min(1e8f, 1e8f, 1e8f);
	float3 AABB_max(-1e8f, -1e8f, -1e8f);
	float fDist = 0.f;

	nVertexCount = 0;
	for (unsigned int j = 0; j < (unsigned int)pMesh->m_nVertexCount; j++)
	{
		if (bUseVertex[j >> 3] & (1 << (j % 8)))
		{
			for (int i = 0; i < e_MaxVertexElementUsage; i++)
			{
				if (m_nVertexDeclaration & (1 << i))
				{
					if (i == e_POSITION)
					{
						memcpy(&Point.x, pMesh->m_pVertexBuffer + j * inputStride, 3 * sizeof(float));

						if (Point.x < AABB_min.x)
							AABB_min.x = Point.x;

						if (Point.x > AABB_max.x)
							AABB_max.x = Point.x;

						if (Point.y < AABB_min.y)
							AABB_min.y = Point.y;

						if (Point.y > AABB_max.y)
							AABB_max.y = Point.y;

						if (Point.z < AABB_min.z)
							AABB_min.z = Point.z;

						if (Point.z > AABB_max.z)
							AABB_max.z = Point.z;
					}

					memcpy(pData + stride * nVertexCount + g_VertexStreamOffsetNoSkin[i], (char*)pMesh->m_pVertexBuffer + j * inputStride * sizeof(float) + g_VertexStreamOffsetNoSkin[i], g_VertexStreamSize[i]);
				}
			}

			nVertexCount++;
		}
	}

	m_Center = 0.5f * (AABB_min + AABB_max);

	float3 size = AABB_max - AABB_min;

	m_fBoundingSphereRadius = 0.5f * MAX(size.x, MAX(size.y, size.z));

	m_VertexBuffer = CResourceManager::CreateVertexBuffer(stride * nVertexCount, pData);

	m_nStreamBufferId.resize(e_MaxVertexElementUsage);

	for (int i = 0; i < e_MaxVertexElementUsage; i++)
	{
		if (m_nVertexDeclaration & (1 << i))
			m_nStreamBufferId[i] = CResourceManager::CreateVertexBuffer(m_VertexBuffer, g_VertexStreamOffsetNoSkin[i]);
	}

	delete[] uRemappedIndices;
	delete[] bUseVertex;

	m_nNumVertex = nVertexCount;
	m_nFirstVertex = 0;

	m_nFirstIndex = 0;
	m_nNumIndex = 3 * pInfo.m_nPacketSize;

	m_nStride = stride;

	m_pMaterial = pInfo.m_pMaterial;

	if (m_pMaterial == NULL)
		m_pMaterial = CMaterial::GetMaterial("None");

	m_nTriangleCount = pInfo.m_nPacketSize;

	m_pShaderHook = CDeferredRenderer::UpdateShader;

	m_eTopology = e_TriangleList;

	m_pTransformedBones = NULL;
	m_nBonesCount = 0;
}



Packet::~Packet()
{
}


PacketList::~PacketList()
{
	m_pPackets.clear();
}


/*void PacketList::SetTransformedBones(std::vector<DualQuaternion>& Bones)
{
	PacketList* pPacketList = this;

	while (pPacketList != NULL)
	{
		pPacketList->packet->m_pTransformedBones = Bones.data();
		pPacketList->packet->m_nBonesCount = (int)Bones.size();

		pPacketList = pPacketList->next;
	}
}*/



PacketList* CPacketManager::MeshToPacketList(CMesh* pMesh)
{
	std::vector<CMesh::SPacketInfo>::iterator it;

	PacketList* list = new PacketList;
	int count = 0;

	for(it = pMesh->m_PacketInfo.begin() ; it < pMesh->m_PacketInfo.end(); it++ )
	{
		if( it->m_nPacketSize > 0)
		{
			Packet current_packet( pMesh, *it);

			list->m_pPackets.push_back(current_packet);
		}
	}

	return list;
}


void CPacketManager::AddPacketList(CMesh* pMesh, bool bIsStatic, ERenderList nRenderType)
{
	ASSERT(pMesh->m_pPacketList != nullptr);

	AddPacketList(*(PacketList*)pMesh->m_pPacketList, pMesh->GetModelMatrix(), pMesh->m_LastModelMatrix, bIsStatic, nRenderType);

	if (pMesh->m_pSDF != nullptr)
		((CSDF*)pMesh->m_pSDF)->Render();
}


void CPacketManager::AddPacketList(PacketList& packetlist, bool bIsStatic, ERenderList nRenderType)
{
	ASSERT(nRenderType < MAX_DRAWABLE_LIST_COUNT && nRenderType >= 0);

	std::vector<Drawable>& list = CPacketManager::GetDrawListToFill(nRenderType);

	Drawable drawable;
	drawable.m_bIsStatic		= bIsStatic;
	drawable.m_pPacketList		= packetlist;
	drawable.m_ModelMatrix.Eye();
	drawable.m_LastModelMatrix.Eye();
	list.push_back(drawable);
}


void CPacketManager::AddPacketList(PacketList& packetlist, float3x4 ModelMatrix, float3x4 LastModelMatrix, bool bIsStatic, ERenderList nRenderType)
{
	ASSERT(nRenderType < MAX_DRAWABLE_LIST_COUNT && nRenderType >= 0);

	std::vector<Drawable>& list = CPacketManager::GetDrawListToFill(nRenderType);

	Drawable drawable;
	drawable.m_bIsStatic		= bIsStatic;
	drawable.m_pPacketList		= packetlist;
	drawable.m_ModelMatrix		= ModelMatrix;
	drawable.m_LastModelMatrix	= LastModelMatrix;
	list.push_back(drawable);
}


void CPacketManager::EmptyDynamicLists()
{
	for (int i = 0; i < MAX_DRAWABLE_LIST_COUNT; i++)
	{
		m_pDrawables[i].clear();
	}
}


void CPacketManager::Reset()
{
	for (int i = 0; i < MAX_DRAWABLE_LIST_COUNT; i++)
	{
		m_pDrawables[i].clear();
	}
}


