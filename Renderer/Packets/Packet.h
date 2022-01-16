#ifndef RENDERER_PACKET_INC
#define RENDERER_PACKET_INC

#include "Engine/Device/ResourceManager.h"
#include "Engine/PolygonalMesh/PolygonalMesh.h"


#define MAX_DRAWABLE_LIST_COUNT 32

enum ERenderList
{
	e_RenderType_Standard,

	e_RenderType_ImGui,
	e_RenderType_3D_Debug,
	e_RenderType_3D_Debug_Alpha,

	e_RenderType_Light
};

enum EPrimitiveTopology
{
	e_PointList,
	e_TriangleList,
	e_TriangleStrip,
	e_TriangleFan,
	e_LineList,

	e_TrianglePatch
};


__declspec(align(32)) struct STransformData
{
	float3x4	m_WorldMatrix;
	float4		m_Color;

	STransformData operator=(STransformData const& data)
	{
		m_WorldMatrix = data.m_WorldMatrix;
		m_Color = data.m_Color;
	}
};


_declspec(align(32)) class Packet
{
	friend class CPacketManager;
	friend class CPacketBuilder;
	friend class CSpriteEngine;

public:

	enum EPacketType
	{
		e_EnginePacket,
		e_StandardPacket
	};

	Packet();
	Packet(CMesh* pMesh, CMesh::SPacketInfo pInfo);

	~Packet();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void* operator new[](size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void operator delete[](void* p)
	{
		_mm_free(p);
	}

	bool IsIndexed() const
	{
		return m_IndexBuffer != INVALIDHANDLE;
	}

	EPacketType				m_eType;

	int						m_nTriangleCount;
	int						m_nFirstVertex;
	int						m_nNumVertex;
	int						m_nNumIndex;
	int						m_nFirstIndex;

	float4					m_ScissorRec;

	CMesh*					m_pMesh;
	CMaterial* 				m_pMaterial;
	int						(*m_pShaderHook)(Packet* packet, void* p_pShaderData);

	unsigned int			m_nVertexDeclaration;

	BufferId				m_IndexBuffer;
	BufferId				m_VertexBuffer;

	std::vector<BufferId>	m_nStreamBufferId;

	EPrimitiveTopology		m_eTopology;

	unsigned int			m_nStride;

	DualQuaternion*			m_pTransformedBones;
	int						m_nBonesCount;

	float3					m_Center;
	float					m_fBoundingSphereRadius;

	unsigned long long int	m_nViewportMask;
};


_declspec(align(32)) class PacketList
{
public:

	PacketList() 
	{
		m_nNbInstances = 0;
		m_nInstanceBufferID = INVALIDHANDLE;
		m_nInstancedBufferByteOffset = 0;
		m_nInstancedStreamMask = 0;
		m_nInstancedBufferStride = 0;
	}

	~PacketList();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void* operator new[](size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void operator delete[](void* p)
	{
		_mm_free(p);
	}

	//void SetTransformedBones(std::vector<DualQuaternion>& Bones);

	unsigned int					m_nNbInstances;
	BufferId						m_nInstanceBufferID;
	unsigned int					m_nInstancedStreamMask;
	unsigned int					m_nInstancedBufferByteOffset;

	unsigned int					m_nInstancedBufferStride;

	std::vector<Packet>				m_pPackets;
};


class Drawable
{
public:

	bool			m_bIsStatic;
	PacketList		m_pPacketList;
	float3x4		m_ModelMatrix;
	float3x4		m_LastModelMatrix;
};



class CPacketManager
{
public:

	static void Init();
	static void Terminate();

	static void AddPacketList(CMesh* pMesh, bool bIsStatic = false, ERenderList nRenderType = ERenderList::e_RenderType_Standard);
	static void AddPacketList(PacketList& list, bool bIsStatic = false, ERenderList nRenderType = ERenderList::e_RenderType_Standard);
	static void AddPacketList(PacketList& list, float3x4 ModelMatrix, float3x4 LastModelMatrix, bool bIsStatic = false, ERenderList nRenderType = ERenderList::e_RenderType_Standard);

	static void EmptyDynamicLists();
	static void Reset();

	static PacketList* MeshToPacketList(CMesh* mesh);

	static std::vector<Drawable>& GetDrawListToFill(ERenderList nRenderType) { return (*m_pDrawablesToFill)[nRenderType]; }
	static std::vector<Drawable>& GetDrawListToFlush(ERenderList nRenderType) { return (*m_pDrawablesToFlush)[nRenderType]; }

	static void ForceShaderHook(int(*pShaderHook)(Packet* packet, void* p_pShaderData)) { m_pShaderHook = pShaderHook; }
	
	thread_local static int(*m_pShaderHook)(Packet* packet, void* p_pShaderData);

	static void UpdateBeforeFlush();

private:

	static std::vector<std::vector<Drawable>>	m_pDrawables[2];

	static std::vector<std::vector<Drawable>>*	m_pDrawablesToFill;
	static std::vector<std::vector<Drawable>>*	m_pDrawablesToFlush;
};



class CPacketBuilder
{
public:

	static void Init();
	static void Terminate();

	static void PrepareForFlush();

	static PacketList*		BuildSphere(float3 Center, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData));
	static PacketList*		BuildCone(float3 Pos, float3 Dir, float fAngle, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData));
	static PacketList*		BuildHemisphere(float3 Center, float3 Dir, float fRadius, int(*pShaderHook)(Packet* packet, void* p_pShaderData));

	struct SSphereInfo
	{
		float3				m_Center;
		float				m_fRadius;

		unsigned int		m_nLightID;
		float3				m_Padding;
	};

	struct SConeInfo
	{
		float3				m_Origin;
		float				m_fRadius;

		float3				m_Direction;
		float				m_fAngle;

		unsigned int		m_nLightID;
		float3				m_Padding;
	};

	static PacketList*		BuildInstancedSphereBatch(std::vector<SSphereInfo>& BatchInfo, int(*pShaderHook)(Packet* packet, void* p_ShaderData));
	static PacketList*		BuildInstancedConeBatch(std::vector<SConeInfo>& BatchInfo, int(*pShaderHook)(Packet* packet, void* p_ShaderData));

	static PacketList*		BuildLine(float3& P1, float3& P2, float4& Color, int(*pShaderHook)(Packet* packet, void* p_ShaderData));
	static PacketList*		BuildCircle(float3& Origin, float3& Normal, float fRadius, float4& Color, int(*pShaderHook)(Packet* packet, void* p_ShaderData));

	static PacketList*		GetNewPacketList();
	static Packet*			GetNewPacket(int numVertices, int numIndices);

	static float*			GetVertexBuffer(Packet* pPacket);
	static unsigned int*	GetIndexBuffer(Packet* pPacket);

private:

	static Packet*			ms_pPackets;
	static PacketList*		ms_pPacketLists;

	static unsigned int		ms_nNextVertexIndex;
	static unsigned int		ms_nNextTriangleIndex;
	static unsigned int		ms_nNumVertexToRender;
	static unsigned int		ms_nNumTriangleToRender;
	static unsigned int		ms_nNextInstanceData;

	static unsigned int		ms_nNextPacketIndex;

	static unsigned int		ms_nMaxNumVertices;
	static unsigned int		ms_nMaxTriangles;
	static unsigned int		ms_nMaxInstanceDataSize;
	static unsigned int		ms_nMaxNumPackets;
};


#endif
