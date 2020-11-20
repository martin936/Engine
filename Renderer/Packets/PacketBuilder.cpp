#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Packet.h"

#include "Engine/Imgui/imgui.h"

#ifdef __OPENGL__
#include "Engine/Imgui/OpenGL/imgui_impl_glfw.h"
#include "Engine/Imgui/OpenGL/imgui_impl_opengl3.h"
#else
#include "Engine/Imgui/Vulkan/imgui_impl_vulkan.h"
#include "Engine/Imgui/imgui_impl_win32.h"
#endif


Packet*			CPacketBuilder::ms_pPackets			= NULL;
PacketList*		CPacketBuilder::ms_pPacketLists		= NULL;

unsigned int	CPacketBuilder::ms_nNextVertexIndex		= 0U;
unsigned int	CPacketBuilder::ms_nNextTriangleIndex	= 0U;
unsigned int	CPacketBuilder::ms_nNumVertexToRender	= 0U;
unsigned int	CPacketBuilder::ms_nNumTriangleToRender = 0U;
unsigned int	CPacketBuilder::ms_nMaxNumVertices		= 1000000U;
unsigned int	CPacketBuilder::ms_nMaxTriangles		= 1000000U;
unsigned int	CPacketBuilder::ms_nMaxInstanceDataSize = 1000000U;
unsigned int	CPacketBuilder::ms_nMaxNumPackets		= 1000U;
unsigned int	CPacketBuilder::ms_nNextPacketIndex		= 0U;
unsigned int	CPacketBuilder::ms_nNextInstanceData	= 0U;


#define NUM_BUFFERS 5

BufferId		g_VertexBuffer[NUM_BUFFERS];
BufferId		g_IndexBuffer[NUM_BUFFERS];
BufferId		g_InstanceBuffer[NUM_BUFFERS];

unsigned int	g_CurrentBuffer = 0;

char*			g_MappedVertexBuffer = nullptr;
char*			g_MappedIndexBuffer = nullptr;
char*			g_MappedInstanceBuffer = nullptr;


const float gs_fSphereVertexBuffer[42 * g_VertexStrideStandard] = 
{
	0.000000f, -1.000000f, 0.000000f,		0.f, 0.f, 0.f,
	0.723607f, -0.447220f, 0.525725f,		0.f, 0.f, 0.f,
	-0.276388f, -0.447220f, 0.850649f,		0.f, 0.f, 0.f,
	-0.894426f, -0.447216f, 0.000000f,		0.f, 0.f, 0.f,
	-0.276388f, -0.447220f, -0.850649f,		0.f, 0.f, 0.f,
	0.723607f, -0.447220f, -0.525725f,		0.f, 0.f, 0.f,
	0.276388f, 0.447220f, 0.850649f,		0.f, 0.f, 0.f,
	-0.723607f, 0.447220f, 0.525725f,		0.f, 0.f, 0.f,
	-0.723607f, 0.447220f, -0.525725f,		0.f, 0.f, 0.f,
	0.276388f, 0.447220f, -0.850649f,		0.f, 0.f, 0.f,
	0.894426f, 0.447216f, 0.000000f,		0.f, 0.f, 0.f,
	0.000000f, 1.000000f, 0.000000f,		0.f, 0.f, 0.f,
	-0.162456f, -0.850654f, 0.499995f,		0.f, 0.f, 0.f,
	0.425323f, -0.850654f, 0.309011f,		0.f, 0.f, 0.f,
	0.262869f, -0.525738f, 0.809012f,		0.f, 0.f, 0.f,
	0.850648f, -0.525736f, 0.000000f,		0.f, 0.f, 0.f,
	0.425323f, -0.850654f, -0.309011f,		0.f, 0.f, 0.f,
	-0.525730f, -0.850652f, 0.000000f,		0.f, 0.f, 0.f,
	-0.688189f, -0.525736f, 0.499997f,		0.f, 0.f, 0.f,
	-0.162456f, -0.850654f, -0.499995f,		0.f, 0.f, 0.f,
	-0.688189f, -0.525736f, -0.499997f,		0.f, 0.f, 0.f,
	0.262869f, -0.525738f, -0.809012f,		0.f, 0.f, 0.f,
	0.951058f, 0.000000f, 0.309013f,		0.f, 0.f, 0.f,
	0.951058f, 0.000000f, -0.309013f,		0.f, 0.f, 0.f,
	0.000000f, 0.000000f, 1.000000f,		0.f, 0.f, 0.f,
	0.587786f, 0.000000f, 0.809017f,		0.f, 0.f, 0.f,
	-0.951058f, 0.000000f, 0.309013f,		0.f, 0.f, 0.f,
	-0.587786f, 0.000000f, 0.809017f,		0.f, 0.f, 0.f,
	-0.587786f, 0.000000f, -0.809017f,		0.f, 0.f, 0.f,
	-0.951058f, 0.000000f, -0.309013f,		0.f, 0.f, 0.f,
	0.587786f, 0.000000f, -0.809017f,		0.f, 0.f, 0.f,
	0.000000f, 0.000000f, -1.000000f,		0.f, 0.f, 0.f,
	0.688189f, 0.525736f, 0.499997f,		0.f, 0.f, 0.f,
	-0.262869f, 0.525738f, 0.809012f,		0.f, 0.f, 0.f,
	-0.850648f, 0.525736f, 0.000000f,		0.f, 0.f, 0.f,
	-0.262869f, 0.525738f, -0.809012f,		0.f, 0.f, 0.f,
	0.688189f, 0.525736f, -0.499997f,		0.f, 0.f, 0.f,
	0.162456f, 0.850654f, 0.499995f,		0.f, 0.f, 0.f,
	0.525730f, 0.850652f, 0.000000f,		0.f, 0.f, 0.f,
	-0.425323f, 0.850654f, 0.309011f,		0.f, 0.f, 0.f,
	-0.425323f, 0.850654f, -0.309011f,		0.f, 0.f, 0.f,
	0.162456f, 0.850654f, -0.499995f
};


const unsigned int gs_nSphereIndexBuffer[240] = 
{
	0U, 13U, 12U,
	1U, 13U, 15U,
	0U, 12U, 17U,
	0U, 17U, 19U,
	0U, 19U, 16U,
	1U, 15U, 22U,
	2U, 14U, 24U,
	3U, 18U, 26U,
	4U, 20U, 28U,
	5U, 21U, 30U,
	1U, 22U, 25U,
	2U, 24U, 27U,
	3U, 26U, 29U,
	4U, 28U, 31U,
	5U, 30U, 23U,
	6U, 32U, 37U,
	7U, 33U, 39U,
	8U, 34U, 40U,
	9U, 35U, 41U,
	10U, 36U, 38U,
	38U, 41U, 11U,
	38U, 36U, 41U,
	36U, 9U, 41U,
	41U, 40U, 11U,
	41U, 35U, 40U,
	35U, 8U, 40U,
	40U, 39U, 11U,
	40U, 34U, 39U,
	34U, 7U, 39U,
	39U, 37U, 11U,
	39U, 33U, 37U,
	33U, 6U, 37U,
	37U, 38U, 11U,
	37U, 32U, 38U,
	32U, 10U, 38U,
	23U, 36U, 10U,
	23U, 30U, 36U,
	30U, 9U, 36U,
	31U, 35U, 9U,
	31U, 28U, 35U,
	28U, 8U, 35U,
	29U, 34U, 8U,
	29U, 26U, 34U,
	26U, 7U, 34U,
	27U, 33U, 7U,
	27U, 24U, 33U,
	24U, 6U, 33U,
	25U, 32U, 6U,
	25U, 22U, 32U,
	22U, 10U, 32U,
	30U, 31U, 9U,
	30U, 21U, 31U,
	21U, 4U, 31U,
	28U, 29U, 8U,
	28U, 20U, 29U,
	20U, 3U, 29U,
	26U, 27U, 7U,
	26U, 18U, 27U,
	18U, 2U, 27U,
	24U, 25U, 6U,
	24U, 14U, 25U,
	14U, 1U, 25U,
	22U, 23U, 10U,
	22U, 15U, 23U,
	15U, 5U, 23U,
	16U, 21U, 5U,
	16U, 19U, 21U,
	19U, 4U, 21U,
	19U, 20U, 4U,
	19U, 17U, 20U,
	17U, 3U, 20U,
	17U, 18U, 3U,
	17U, 12U, 18U,
	12U, 2U, 18U,
	15U, 16U, 5U,
	15U, 13U, 16U,
	13U, 0U, 16U,
	12U, 14U, 2U,
	12U, 13U, 14U,
	13U, 1U, 14U
};

const float gs_fConeVertexBuffer[10 * g_VertexStrideStandard] = 
{
	-0.000000f, 1.000000f, -1.000000f,		0.f, 0.f, 0.f,
	-0.707107f, 1.000000f, -0.707107f,		0.f, 0.f, 0.f,
	-1.000000f, 1.000000f, 0.000000f,		0.f, 0.f, 0.f,
	-0.707107f, 1.000000f, 0.707107f,		0.f, 0.f, 0.f,
	0.000000f, 1.000000f, 1.000000f,		0.f, 0.f, 0.f,
	0.000000f, 0.000000f, -0.000000f,		0.f, 0.f, 0.f,
	0.707107f, 1.000000f, 0.707107f,		0.f, 0.f, 0.f,
	1.000000f, 1.000000f, 0.000000f,		0.f, 0.f, 0.f,
	0.707107f, 1.000000f, -0.707107f,		0.f, 0.f, 0.f,
	-0.000000f, 1.000000f, 0.000000f
};

const unsigned int gs_nConeIndexBuffer[48] = 
{
	0U, 5U, 1U,
	1U, 5U, 2U,
	2U, 5U, 3U,
	3U, 5U, 4U,
	4U, 5U, 6U,
	6U, 5U, 7U,
	7U, 5U, 8U,
	8U, 5U, 0U,
	6U, 9U, 4U,
	7U, 9U, 6U,
	8U, 9U, 7U,
	0U, 9U, 8U,
	1U, 9U, 0U,
	2U, 9U, 1U,
	3U, 9U, 2U,
	4U, 9U, 3U
};


void CPacketBuilder::Init()
{
	for (int i = 0; i < NUM_BUFFERS; i++)
	{
		g_VertexBuffer[i]	= CResourceManager::CreateMappableVertexBuffer(ms_nMaxNumVertices * g_VertexStrideStandard, nullptr);
		g_IndexBuffer[i]	= CResourceManager::CreateMappableIndexBuffer(3 * ms_nMaxTriangles * sizeof(unsigned int), nullptr);
		g_InstanceBuffer[i]	= CResourceManager::CreateMappableVertexBuffer(ms_nMaxInstanceDataSize * sizeof(float), nullptr);
	}

	ms_pPackets		= new Packet[ms_nMaxNumPackets];
	ms_pPacketLists = new PacketList[ms_nMaxNumPackets];

	g_CurrentBuffer = 0;

	ms_nNextVertexIndex		= 0U;
	ms_nNextTriangleIndex	= 0U;
	ms_nNumVertexToRender	= 0U;
	ms_nNumTriangleToRender = 0U;
	ms_nMaxNumVertices		= 1000000U;
	ms_nMaxTriangles		= 1000000U;
	ms_nMaxNumPackets		= 1000U;
	ms_nNextPacketIndex		= 0U;

	g_MappedVertexBuffer	= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_VertexBuffer[g_CurrentBuffer]));
	g_MappedIndexBuffer		= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_IndexBuffer[g_CurrentBuffer]));
	g_MappedInstanceBuffer	= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_InstanceBuffer[g_CurrentBuffer]));
}



void CPacketBuilder::Terminate()
{
	for (unsigned int i = 0U; i < ms_nMaxNumPackets; i++)
	{
		ms_pPacketLists[i].m_pPackets.clear();
		ms_pPackets[i].m_VertexBuffer = INVALIDHANDLE;
		ms_pPackets[i].m_IndexBuffer = INVALIDHANDLE;
	}

	delete[] ms_pPacketLists;
	delete[] ms_pPackets;
}



void CPacketBuilder::PrepareForFlush()
{
	CResourceManager::UnmapBuffer(g_VertexBuffer[g_CurrentBuffer]);
	CResourceManager::UnmapBuffer(g_IndexBuffer[g_CurrentBuffer]);
	CResourceManager::UnmapBuffer(g_InstanceBuffer[g_CurrentBuffer]); 

	g_CurrentBuffer = (g_CurrentBuffer + 1) % NUM_BUFFERS;

	g_MappedVertexBuffer	= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_VertexBuffer[g_CurrentBuffer]));
	g_MappedIndexBuffer		= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_IndexBuffer[g_CurrentBuffer]));
	g_MappedInstanceBuffer	= reinterpret_cast<char*>(CResourceManager::MapBuffer(g_InstanceBuffer[g_CurrentBuffer]));
	
	ms_nNumVertexToRender	= ms_nNextVertexIndex;
	ms_nNumTriangleToRender = ms_nNextTriangleIndex;

	ms_nNextVertexIndex		= 0U;
	ms_nNextTriangleIndex	= 0U;
	ms_nNextPacketIndex		= 0U;
	ms_nNextInstanceData	= 0U;
}



PacketList* CPacketBuilder::BuildSphere(float3 Center, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData))
{
	float pVertexBuffer[126];

	for (int i = 0; i < 42; i++)
	{
		for (int j = 0; j < 3; j++)
			pVertexBuffer[3 * i + j] = gs_fSphereVertexBuffer[3 * i + j] * fRadius * 1.0705f + Center[j];
	}

	memcpy(g_MappedVertexBuffer + ms_nNextVertexIndex * g_VertexStrideStandard, pVertexBuffer, 126 * g_VertexStrideStandard);
	memcpy(g_MappedIndexBuffer + 3 * ms_nNextTriangleIndex * sizeof(unsigned int), gs_nSphereIndexBuffer, 240 * sizeof(unsigned int));

	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_VertexBuffer	= g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer	= g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook = pShaderHook;
	pPacket.m_nFirstIndex	= 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumIndex		= 240;

	pPacket.m_nFirstVertex	= 3 * ms_nNextVertexIndex;
	pPacket.m_nNumVertex	= 42;

	pPacket.m_eTopology = e_TriangleList;
	pPacket.m_nTriangleCount = 80;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_nNextVertexIndex += 42;
	ms_nNextTriangleIndex += 80;
	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}



PacketList* CPacketBuilder::BuildInstancedSphereBatch(std::vector<SSphereInfo>& BatchInfos, int(*pShaderHook)(Packet* packet, void* p_pShaderData))
{
	memcpy(g_MappedVertexBuffer + ms_nNextVertexIndex * g_VertexStrideStandard, gs_fSphereVertexBuffer, 42 * g_VertexStrideStandard);
	memcpy(g_MappedIndexBuffer + 3 * ms_nNextTriangleIndex * sizeof(unsigned int), gs_nSphereIndexBuffer, 240 * sizeof(unsigned int));

	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook = pShaderHook;
	pPacket.m_nFirstIndex = 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumIndex = 240;

	pPacket.m_nFirstVertex = ms_nNextVertexIndex;
	pPacket.m_nNumVertex = 42;

	pPacket.m_eTopology = e_TriangleList;
	pPacket.m_nTriangleCount = 80;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_pPacketLists[ms_nNextPacketIndex].m_nInstanceBufferID			= g_InstanceBuffer[g_CurrentBuffer];
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedBufferByteOffset	= ms_nNextInstanceData;
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedStreamMask			= e_INSTANCEROW1MASK | e_INSTANCEROW2MASK | e_INSTANCEROW3MASK;
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedBufferStride		= g_InstanceStride;
	ms_pPacketLists[ms_nNextPacketIndex].m_nNbInstances					= static_cast<unsigned int>(BatchInfos.size());

	int nNbInstances = static_cast<unsigned int>(BatchInfos.size());

	for (int i = 0; i < nNbInstances; i++)
	{
		memcpy((char*)g_MappedInstanceBuffer + ms_nNextInstanceData,		&float4(BatchInfos[i].m_Center, BatchInfos[i].m_fRadius), sizeof(float4));
		memcpy((char*)g_MappedInstanceBuffer + ms_nNextInstanceData + 16,	&float4(0.f, 0.f, 0.f, -1.f), sizeof(float4));
		memcpy((char*)g_MappedInstanceBuffer + ms_nNextInstanceData + 32,	&float4(static_cast<float>(BatchInfos[i].m_nLightID), 1.f, 0.f, 0.f), sizeof(float4));

		ms_nNextInstanceData += g_InstanceStride;
	}

	ms_nNextVertexIndex += 42;
	ms_nNextTriangleIndex += 80;
	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}



PacketList* CPacketBuilder::BuildHemisphere(float3 Center, float3 Dir, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData))
{
	float3x3 Transform;

	float3 e1 = float3::cross(Dir, float3(1.f, 0.f, 0.f));
	float3 n = Dir;
	
	if (e1.length() < 1e-3f)
		e1 = float3::cross(Dir, float3(0.f, 1.f, 0.f));

	float3 e2 = float3::cross(e1, Dir);
	e1 = float3::cross(Dir, e2);

	e1.normalize();
	e2.normalize();
	n.normalize();

	float pVertexBuffer[81] =
	{
		0.276388f, 0.447220f, 0.850649f,
		-0.723607f, 0.447220f, 0.525725f,
		-0.723607f, 0.447220f, -0.525725f,
		0.276388f, 0.447220f, -0.850649f,
		0.894426f, 0.447216f, 0.000000f,
		0.000000f, 1.000000f, 0.000000f,
		0.951058f, 0.000000f, 0.309013f,
		0.951058f, 0.000000f, -0.309013f,
		0.000000f, 0.000000f, 1.000000f,
		0.587786f, 0.000000f, 0.809017f,
		-0.951058f, 0.000000f, 0.309013f,
		-0.587786f, 0.000000f, 0.809017f,
		-0.587786f, 0.000000f, -0.809017f,
		-0.951058f, 0.000000f, -0.309013f,
		0.587786f, 0.000000f, -0.809017f,
		0.000000f, 0.000000f, -1.000000f,
		0.688189f, 0.525736f, 0.499997f,
		-0.262869f, 0.525738f, 0.809012f,
		-0.850648f, 0.525736f, 0.000000f,
		-0.262869f, 0.525738f, -0.809012f,
		0.688189f, 0.525736f, -0.499997f,
		0.162456f, 0.850654f, 0.499995f,
		0.525730f, 0.850652f, 0.000000f,
		-0.425323f, 0.850654f, 0.309011f,
		-0.425323f, 0.850654f, -0.309011f,
		0.162456f, 0.850654f, -0.499995f,
		0.000000f, 0.000000f, 0.000000f
	};


	unsigned int nIndexBuffer[150] =
	{
		1U, 17U, 22U, 
		2U, 18U, 24U, 
		3U, 19U, 25U, 
		4U, 20U, 26U, 
		5U, 21U, 23U, 
		23U, 26U, 6U, 
		23U, 21U, 26U, 
		21U, 4U, 26U, 
		26U, 25U, 6U, 
		26U, 20U, 25U, 
		20U, 3U, 25U, 
		25U, 24U, 6U, 
		25U, 19U, 24U, 
		19U, 2U, 24U, 
		24U, 22U, 6U, 
		24U, 18U, 22U, 
		18U, 1U, 22U, 
		22U, 23U, 6U, 
		22U, 17U, 23U, 
		17U, 5U, 23U, 
		8U, 21U, 5U, 
		8U, 15U, 21U, 
		15U, 4U, 21U, 
		16U, 20U, 4U, 
		16U, 13U, 20U, 
		13U, 3U, 20U, 
		14U, 19U, 3U, 
		14U, 11U, 19U, 
		11U, 2U, 19U, 
		12U, 18U, 2U, 
		12U, 9U, 18U, 
		9U, 1U, 18U, 
		10U, 17U, 1U, 
		10U, 7U, 17U, 
		7U, 5U, 17U, 
		15U, 16U, 4U, 
		13U, 14U, 3U, 
		11U, 12U, 2U, 
		9U, 10U, 1U, 
		7U, 8U, 5U, 
		10U, 9U, 27U, 
		7U, 10U, 27U, 
		8U, 7U, 27U, 
		15U, 8U, 27U, 
		16U, 15U, 27U, 
		13U, 16U, 27U, 
		14U, 13U, 27U, 
		11U, 14U, 27U, 
		12U, 11U, 27U, 
		9U, 12U, 27U
	};

	for (int i = 0; i < 26; i++)
	{
		float tmp[3];

		Copy(tmp, pVertexBuffer + 3 * i);

		for (int j = 0; j < 3; j++)
			pVertexBuffer[3 * i + j] = tmp[1] * n[j] + tmp[0] * e1[j] + tmp[2] * e2[j];

		Normalize(pVertexBuffer + 3 * i);
		Scale(pVertexBuffer + 3 * i, fRadius * 1.0705f);
		Addi(pVertexBuffer + 3 * i, Center.v());
	}

	for (int i = 0; i < 150; i++)
		nIndexBuffer[i]--;

	memcpy(g_MappedVertexBuffer + 3 * ms_nNextVertexIndex * sizeof(float), pVertexBuffer, 81 * sizeof(float));
	memcpy(g_MappedIndexBuffer + 3 * ms_nNextTriangleIndex * sizeof(unsigned int), nIndexBuffer, 150 * sizeof(unsigned int));

	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook	= pShaderHook;
	pPacket.m_nFirstIndex	= 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumIndex		= 150;

	pPacket.m_nFirstVertex	= 3 * ms_nNextVertexIndex;
	pPacket.m_nNumVertex	= 26;

	pPacket.m_eTopology		= e_TriangleList;
	pPacket.m_nTriangleCount = 50;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_nNextVertexIndex += 27;
	ms_nNextTriangleIndex += 50;
	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}



PacketList* CPacketBuilder::BuildCone(float3 Center, float3 Dir, float fAngle, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData))
{
	float3 e1 = float3::cross(Dir, float3(1.f, 0.f, 0.f));
	float3 n = Dir;

	if (e1.length() < 1e-3f)
		e1 = float3::cross(Dir, float3(0.f, 1.f, 0.f));

	float3 e2 = float3::cross(e1, Dir);
	e1 = float3::cross(Dir, e2);

	e1.normalize();
	e2.normalize();
	n.normalize();


	float pVertexBuffer[30];

	float fSize = 1.09f * tanf(fAngle * 3.141592f / 360.f);

	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 3; j++)
			pVertexBuffer[3 * i + j] = gs_fConeVertexBuffer[3 * i + j];

		for (int j = 0; j < 3; j += 2)
			pVertexBuffer[3 * i + j] *= fSize;
	}


	for (int i = 0; i < 10; i++)
	{
		float tmp[3];

		Copy(tmp, pVertexBuffer + 3 * i);

		for (int j = 0; j < 3; j++)
			pVertexBuffer[3 * i + j] = tmp[1] * n[j] + tmp[0] * e1[j] + tmp[2] * e2[j];

		Normalize(pVertexBuffer + 3 * i);
		Scale(tmp, n.v(), -0.15f);
		Addi(pVertexBuffer + 3 * i, tmp);
		Scale(pVertexBuffer + 3 * i, fRadius * 1.35f);
		Addi(pVertexBuffer + 3 * i, &Center.x);
	}

	memcpy(g_MappedVertexBuffer + 3 * ms_nNextVertexIndex * sizeof(float), pVertexBuffer, 30 * sizeof(float));
	memcpy(g_MappedIndexBuffer + 3 * ms_nNextTriangleIndex * sizeof(unsigned int), gs_nConeIndexBuffer, 48 * sizeof(unsigned int));

	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook	= pShaderHook;
	pPacket.m_nFirstIndex	= 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumIndex		= 48;

	pPacket.m_nFirstVertex	= 3 * ms_nNextVertexIndex;
	pPacket.m_nNumVertex	= 10;

	pPacket.m_eTopology		= e_TriangleList;
	pPacket.m_nTriangleCount = 16;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_nNextVertexIndex += 10;
	ms_nNextTriangleIndex += 16;
	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}


PacketList* CPacketBuilder::BuildInstancedConeBatch(std::vector<SConeInfo>& BatchInfo, int(*pShaderHook)(Packet* packet, void* p_ShaderData))
{
	memcpy(g_MappedVertexBuffer + ms_nNextVertexIndex * g_VertexStrideStandard, gs_fConeVertexBuffer, 10 * g_VertexStrideStandard);
	memcpy(g_MappedIndexBuffer + 3 * ms_nNextTriangleIndex * sizeof(unsigned int), gs_nConeIndexBuffer, 48 * sizeof(unsigned int));

	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook = pShaderHook;
	pPacket.m_nFirstIndex = 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumIndex = 48;

	pPacket.m_nFirstVertex = ms_nNextVertexIndex;
	pPacket.m_nNumVertex = 10;

	pPacket.m_eTopology = e_TriangleList;
	pPacket.m_nTriangleCount = 16;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_pPacketLists[ms_nNextPacketIndex].m_nInstanceBufferID			= g_InstanceBuffer[g_CurrentBuffer];
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedBufferByteOffset	= ms_nNextInstanceData;
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedStreamMask			= e_INSTANCEROW1MASK | e_INSTANCEROW2MASK | e_INSTANCEROW3MASK;
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstancedBufferStride		= g_InstanceStride;
	ms_pPacketLists[ms_nNextPacketIndex].m_nNbInstances					= static_cast<unsigned int>(BatchInfo.size());

	int nNbInstances = static_cast<unsigned int>(BatchInfo.size());

	for (int i = 0; i < nNbInstances; i++)
	{
		memcpy(g_MappedInstanceBuffer + ms_nNextInstanceData,		&float4(BatchInfo[i].m_Origin, BatchInfo[i].m_fRadius), sizeof(float4));
		memcpy(g_MappedInstanceBuffer + ms_nNextInstanceData + 16,	&float4(BatchInfo[i].m_Direction, 1.09f * tanf(BatchInfo[i].m_fAngle * 3.141592f / 360.f)), sizeof(float4));
		memcpy(g_MappedInstanceBuffer + ms_nNextInstanceData + 32,	&float4(static_cast<float>(BatchInfo[i].m_nLightID), 0.f, 0.f, 0.f), sizeof(float4));
		ms_nNextInstanceData += g_InstanceStride;
	}

	ms_nNextVertexIndex += 10;
	ms_nNextTriangleIndex += 16;
	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}



PacketList* CPacketBuilder::BuildLine(float3& P1, float3& P2, float4& Color, int(*pShaderHook)(Packet* packet, void* p_ShaderData))
{
	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];
	pPacket.m_nFirstVertex = ms_nNextVertexIndex;
	pPacket.m_nNumVertex = 2;

	unsigned int nColor = ((unsigned int)(255.f * Color.w) << 24) | ((unsigned int)(255.f * Color.z) << 16) | ((unsigned int)(255.f * Color.y) << 8) | ((unsigned int)(255.f * Color.x));

	memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard], &P1.x, sizeof(float3));
	memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard + 5 * sizeof(float)], &nColor, sizeof(unsigned int));
	ms_nNextVertexIndex++;

	memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard], &P2.x, sizeof(float3));
	memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard] + 5 * sizeof(float), &nColor, sizeof(unsigned int));
	ms_nNextVertexIndex++;

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = INVALIDHANDLE;

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook = pShaderHook;

	pPacket.m_eTopology = e_LineList;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK | e_COLORMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_pPacketLists[ms_nNextPacketIndex].m_nInstanceBufferID = INVALIDHANDLE;

	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}


PacketList* CPacketBuilder::GetNewPacketList()
{
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_nInstanceBufferID = INVALIDHANDLE;

	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}


Packet* CPacketBuilder::GetNewPacket(int numVertices, int numIndices)
{
	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];

	pPacket.m_nFirstVertex = ms_nNextVertexIndex;
	pPacket.m_nFirstIndex = 3 * ms_nNextTriangleIndex;
	pPacket.m_nNumVertex = numVertices;
	pPacket.m_nNumIndex = numIndices;

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = g_IndexBuffer[g_CurrentBuffer];

	pPacket.m_eType = Packet::e_StandardPacket;

	ms_nNextPacketIndex++;
	ms_nNextVertexIndex += numVertices;
	ms_nNextTriangleIndex += numIndices / 3;

	return &pPacket;
}


float* CPacketBuilder::GetVertexBuffer(Packet* packet)
{
	return reinterpret_cast<float*>(g_MappedVertexBuffer + packet->m_nFirstVertex * g_VertexStrideStandard);
}


unsigned int* CPacketBuilder::GetIndexBuffer(Packet* packet)
{
	return reinterpret_cast<unsigned int*>(g_MappedIndexBuffer + packet->m_nFirstIndex * sizeof(unsigned int));
}


PacketList* CPacketBuilder::BuildCircle(float3& Origin, float3& Normal, float fRadius, float4& Color, int(*pShaderHook)(Packet* packet, void* p_ShaderData))
{
	Packet& pPacket = ms_pPackets[ms_nNextPacketIndex];
	pPacket.m_nFirstVertex = ms_nNextVertexIndex;
	pPacket.m_nNumVertex = 32;

	float3 points[16];
	float angle = 0.f;

	float3 ex(1.f, 0.f, 0.f);

	if (float3::dotproduct(ex, Normal) > 0.95f)
		ex = float3(0.f, 1.f, 0.f);

	float3 ey = float3::normalize(float3::cross(Normal, ex));
	ex = float3::cross(ey, Normal);

	for (int i = 0; i < 16; i++)
	{
		angle = 2.f * i * 3.1415926f / 16.f;
		points[i] = Origin + fRadius * (cosf(angle) * ex + sinf(angle) * ey);
	}

	unsigned int nColor = ((unsigned int)(255.f * Color.w) << 24) | ((unsigned int)(255.f * Color.z) << 16) | ((unsigned int)(255.f * Color.y) << 8) | ((unsigned int)(255.f * Color.x));

	for (int i = 0; i < 16; i++)
	{
		memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard], &points[i].x, sizeof(float3));
		memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard + 5 * sizeof(float)], &nColor, sizeof(unsigned int));
		ms_nNextVertexIndex++;

		memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard], &points[(i + 1) % 16].x, sizeof(float3));
		memcpy(&g_MappedVertexBuffer[ms_nNextVertexIndex * g_VertexStrideStandard] + 5 * sizeof(float), &nColor, sizeof(unsigned int));
		ms_nNextVertexIndex++;
	}	

	pPacket.m_VertexBuffer = g_VertexBuffer[g_CurrentBuffer];
	pPacket.m_IndexBuffer = INVALIDHANDLE;

	pPacket.m_eType = Packet::e_StandardPacket;

	pPacket.m_pShaderHook = pShaderHook;

	pPacket.m_eTopology = e_LineList;

	pPacket.m_nVertexDeclaration = e_POSITIONMASK | e_COLORMASK;

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	ms_pPacketLists[ms_nNextPacketIndex].m_nInstanceBufferID = INVALIDHANDLE;

	ms_nNextPacketIndex++;

	return &ms_pPacketLists[ms_nNextPacketIndex - 1];
}
