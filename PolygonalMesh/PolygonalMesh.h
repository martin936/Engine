#ifndef PolygonalMesh_INC
#define PolygonalMesh_INC

#define MESH_ARM 0
#define MESH_BACKGROUND 1
#define MESH_WALLS 2
#define MESH_MISC 3


#define MAX_NEIGHBOUR_VERTICES 15


class PacketList;


enum VertexElementType
{
	e_FLOAT1,
	e_FLOAT2,
	e_FLOAT3,
	e_FLOAT4,
	e_SBYTE4,
	e_UBYTE4,
	e_SHORT2,
	e_SHORT4,
	e_UBYTE4N,
	e_SHORT2N,
	e_SHORT4N,
	e_USHORT2N,
	e_USHORT4N,
	e_FLOAT16_2,
	e_FLOAT16_4,

	e_MaxVertexElementType,
};


const char gs_VertexElementByteSize[]
{
	4,		// e_FLOAT1
	8,		// e_FLOAT2
	12,		// e_FLOAT3
	16,		// e_FLOAT4
	4,		// e_SBYTE4
	4,		// e_UBYTE4
	4,		// e_SHORT2
	8,		// e_SHORT4
	4,		// e_UBYTE4N
	4,		// e_SHORT2N
	8,		// e_SHORT4N
	4,		// e_USHORT2N
	8,		// e_USHORT4N
	4,		// e_FLOAT16_2
	8,		// e_FLOAT16_4
};


enum VertexElementUsage
{
	e_POSITION					= 0,
	e_NORMAL					= 1,
	e_TANGENT					= 2,
	e_BITANGENT					= 3,
	e_TEXCOORD					= 4,
	e_BLENDWEIGHT				= 5,
	e_BLENDINDICES				= 6,
	e_TEXCOORD2					= 7,
	e_TEXCOORD3					= 8,
	e_TEXCOORD4					= 9,
	e_COLOR						= 10,
	e_COLOR1					= 11,

	e_INSTANCEROW1				= 12,
	e_INSTANCEROW2				= 13,
	e_INSTANCEROW3				= 14,
	e_INSTANCECOLOR				= 15,

	e_MaxVertexElementUsage,
	e_MaxStandardVertexElementUsage = 7,

};

enum VertexElementUsageMask
{
	e_POSITIONMASK				= (1<<e_POSITION),
	e_NORMALMASK				= (1<<e_NORMAL),
	e_TANGENTMASK				= (1<<e_TANGENT),
	e_BITANGENTMASK				= (1<<e_BITANGENT),
	e_TEXCOORDMASK				= (1<<e_TEXCOORD),
	e_BLENDWEIGHTMASK			= (1<<e_BLENDWEIGHT),
	e_BLENDINDICESMASK			= (1<<e_BLENDINDICES),
	e_TEXCOORD2MASK				= (1<<e_TEXCOORD2),
	e_TEXCOORD3MASK				= (1<<e_TEXCOORD3),
	e_TEXCOORD4MASK				= (1<<e_TEXCOORD4),
	e_COLORMASK					= (1<<e_COLOR),
	e_COLOR1MASK				= (1<<e_COLOR1),
	e_INSTANCEROW1MASK			= (1<<e_INSTANCEROW1),
	e_INSTANCEROW2MASK			= (1<<e_INSTANCEROW2),
	e_INSTANCEROW3MASK			= (1<<e_INSTANCEROW3),
	e_INSTANCECOLORMASK			= (1<<e_INSTANCECOLOR)
};


enum InputSlotClass
{
	e_PerVertex,
	e_PerInstance
};


struct SVertexElements
{
	unsigned int			m_StreamID;
	unsigned int			m_Offset;
	VertexElementUsage		m_Usage;
	VertexElementUsageMask	m_StreamMask;
	VertexElementType		m_Type;
	unsigned int			m_SubID;
	const char *			m_ShaderSemantic;
	InputSlotClass			m_InputSlotClass;
};


static const unsigned int g_VertexStrideNoSkin		= 104;
static const unsigned int g_VertexStrideSkin		= 124;

static const unsigned int g_InstanceStride			= 64;

static const unsigned int g_VertexStrideStandard	= 24;

static const unsigned int g_InstanceStrideSkin	= 64;


#include <stdlib.h>
#include <vector>
#include "Engine/Maths/Maths.h"
#include "Engine/Animation/AnimationSkeletton.h"
#include "Engine/Materials/MaterialDefinition.h"


__declspec(align(32)) class CMesh
{
	friend class CObstacle;
	friend class Packet;
	friend class CPacketManager;
	friend class CPhysicalMesh;
	friend class CSoftbody;
	friend class CRigidbody;
	friend class CSkinner;

public:

	CMesh();

	void Load(const char* cFilePath);
	void Save(const char* cFilePath);

	static CMesh* LoadMesh(const char* cFilePath);

	~CMesh();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void SetCenter(float3& Center);

	inline unsigned int GetID() const
	{
		return m_nID;
	}

	inline bool IsLoaded() const { return m_bIsLoaded; }

	inline int GetVertexCount()			const { return m_nVertexCount;			}
	inline int GetTriangleCount()		const { return m_nTriangleCount;		}
	inline float3 GetCenter()			const { return m_Center;				}
	inline float3 GetBoundingBox()		const { return m_AABB;					}
	inline float GetBoundingSphere()	const { return m_fBoundingSphereRadius; }

	inline float3 GetVertexPosition(int i) const 
	{ 
		return float3(m_pVertexBuffer[i * m_nStride], m_pVertexBuffer[i * m_nStride + 1], m_pVertexBuffer[i * m_nStride + 2]); 
	}

	float3 GetVertexNormal(int i) const;

	inline void GetTriangleIndices(int nPacketID, int face, unsigned int* pTri) const
	{
		int index  = m_PacketInfo[nPacketID].m_nStartIndex + face * 3;

		if (m_pIndexBuffer != nullptr)
		{
			for (int i = 0; i < 3; i++)
				pTri[i] = m_pIndexBuffer[index + i];
		}

		else
		{
			for (int i = 0; i < 3; i++)
				pTri[i] = index + i;
		}
	}

	inline unsigned int GetNumTrianglesInPacket(int nPacketID)
	{
		return m_PacketInfo[nPacketID].m_nPacketSize;
	}

	inline CSkeletton* GetSkeletton() const
	{
		return m_pSkeletton;
	}

	struct SPacketInfo
	{
		int			m_nStartIndex;
		int			m_nPacketSize;
		bool		m_bIsUVMapped;
		int			m_nStreams;
		CMaterial*	m_pMaterial;
		char		m_cName[256];
	};

	static void ClearAll();

private:

	struct SSavedPacketInfo
	{
		int			m_nStartIndex;
		int			m_nPacketSize;
		int			m_bIsUVMapped;
		int			m_nStreams;
		char		m_cMaterial[256];
	};

	void ImportReadObjects(const char* cLine);
	void ImportReadVertices(const char* cLine, void* pData);
	void ImportCheckMaterial(const char* cLine);
	void ImportReadFace(const char* cLine, void* pData, void* pDataOut);
	void ImportAddFace(int* nTriangle, void* pData, bool bUseNormals, bool bUseTexCoords, void* pDataOut);
	void DefragBuffers(void* pData);

	CSkeletton*				m_pSkeletton;

	float*					m_pVertexBuffer;
	unsigned int*			m_pIndexBuffer;
	int						m_nStride;
	int						m_nStreams;

	unsigned int			m_nID;

	float3		m_Center;
	float3		m_AABB;
	float		m_fBoundingSphereRadius;

	int			m_nVertexCount;
	int			m_nVertexCounter;
	int			m_nTriangleCount;

	bool		m_bUseTexCoords;
	bool		m_bIsLoaded;

	std::vector<SPacketInfo> m_PacketInfo;

	static std::vector<CMesh*> ms_pMeshesList;
};



class CSkinner
{
public:

	CSkinner();
	~CSkinner();

	void Add(PacketList* pPacketList);

	void SkinAll(CPhysicalMesh* pMesh);

	inline int GetMeshCount() const
	{
		return (int)m_pData.size();
	}

	inline int GetSkinningBufferID(int i) const
	{
		return m_pData[i]->m_nSkinningDataID;
	}

	inline unsigned int GetSkinnedVertexBufferID(int i) const
	{
		return m_pData[i]->m_nVertexBufferToSkinID;
	}

	inline unsigned int GetStride(int i) const
	{
		return m_pData[i]->m_nStride;
	}

	inline unsigned int GetSkinnedVertexCount(int i) const
	{
		return m_pData[i]->m_nSkinnedVertexCount;
	}

	struct SSkinningData
	{
		unsigned int	m_nTriangleID;
		float			m_fu;
		float			m_fv;
		float			m_fRadius;
		float			m_fTangent[3];
		float			m_fBitangent[3];
		float			m_fNormalSign;
		float			m_fPadding;
	};

private:

	struct SData
	{
		Packet*	m_pPacket;
		SSkinningData* m_pSkinningData;
		unsigned int m_nSkinningDataID;
		unsigned int m_nSkinnedVertexCount;

		unsigned int m_nVertexBufferToSkinID;
		unsigned int m_nStride;
	};

	std::vector<SData*> m_pData;
};



class CPhysicalMesh : public CMesh
{
	friend class CSoftbody;
	friend class CParticles;
	friend class CSkinner;

public:

	enum EMeshTemplate
	{
		e_Icosphere
	};

	CPhysicalMesh(EMeshTemplate eTemplate, int nTesselation);
	CPhysicalMesh(CMesh* pMesh);
	CPhysicalMesh();

	~CPhysicalMesh();

	void Load();
	void Unload();

	void Resize(float fRadius);

private:

	struct SPhysicalVertex
	{
		float	m_Position[3];
		float	m_Normal[3];
		float	m_Velocity[3];
		float	m_Forces[3];
	};

	CSkinner* m_pSkinner;

	void JoinIdenticalVertices(CMesh* pMesh);

	void Tesselate();
	void ComputeNormals();
	void FillEdgesBuffer();
	void FillNeighbours();
	void SortNeighbours();
	void BuildIcosahedron();

	inline void SetVertex(int index, float3& data) const { memcpy(&m_pVertexBuffer[index * m_nStride], data.v(), 3 * sizeof(float)); }
	inline void SetTriangle(int index, int v1, int v2, int v3) const 
	{
		m_pIndexBuffer[3 * index] = v1;
		m_pIndexBuffer[3 * index + 1] = v2;
		m_pIndexBuffer[3 * index + 2] = v3;
	}

	unsigned int*		m_pEdgesIndex;
	unsigned int**		m_pNeighbourIndex;
	unsigned int*		m_pNeighbourCount;

	int		m_nEdgesCount;

	unsigned int m_nVertexBufferID;
	unsigned int m_nIndexBufferID;
	bool	m_bLoaded;
};




int GetHead(char* pDest, int nSize, const char* pSource);


#endif
