#include "PolygonalMesh.h"
#include "Engine/Renderer/SDF/SDF.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


std::vector<CMesh*> CMesh::ms_pMeshesList;


const char * g_VertexAttributeSemantics[e_MaxVertexElementUsage] =
{
	"POSITION",				// e_POSITION,
	"NORMAL",				// e_NORMAL,
	"TANGENT",				// e_TANGENT,
	"BITANGENT",			// e_BITANGENT,
	"TEXCOORD",				// e_TEXCOORD,	
	"BLENDWEIGHT",			// e_BLENDWEIGHT,	
	"BLENDINDICES",			// e_BLENDINDICES,		
	"TEXCOORD",				// e_TEXCOORD1,	
	"TEXCOORD",				// e_TEXCOORD2,	
	"TEXCOORD",				// e_TEXCOORD3,	
	"COLOR",				// e_COLOR,
	"COLOR",				// e_COLOR1,
	"INSTANCEMATRIX",		// e_INSTANCEROW1
	"INSTANCEMATRIX",		// e_INSTANCEROW2
	"INSTANCEMATRIX",		// e_INSTANCEROW3
	"INSTANCECOLOR",		// e_INSTANCECOLOR
};


SVertexElements g_VertexStreamSemantics[] =
{
	{ 0, 0, e_POSITION,					e_POSITIONMASK,					e_FLOAT3,		0, g_VertexAttributeSemantics[e_POSITION],				e_PerVertex },
	{ 1, 0, e_NORMAL,					e_NORMALMASK,					e_FLOAT3,		0, g_VertexAttributeSemantics[e_NORMAL],				e_PerVertex },
	{ 2, 0, e_TANGENT,					e_TANGENTMASK,					e_FLOAT3,		0, g_VertexAttributeSemantics[e_TANGENT],				e_PerVertex },
	{ 3, 0, e_BITANGENT,				e_BITANGENTMASK,				e_FLOAT3,		0, g_VertexAttributeSemantics[e_BITANGENT],				e_PerVertex },
	{ 4, 0, e_TEXCOORD,					e_TEXCOORDMASK,					e_FLOAT3,		0, g_VertexAttributeSemantics[e_TEXCOORD],				e_PerVertex },
	{ 5, 0, e_BLENDWEIGHT,				e_BLENDWEIGHTMASK,				e_FLOAT4,		0, g_VertexAttributeSemantics[e_BLENDWEIGHT],			e_PerVertex },
	{ 6, 0, e_BLENDINDICES,				e_BLENDINDICESMASK,				e_UBYTE4,		0, g_VertexAttributeSemantics[e_BLENDINDICES],			e_PerVertex },	
	{ 7, 0, e_TEXCOORD2,				e_TEXCOORD2MASK,				e_FLOAT3,		1, g_VertexAttributeSemantics[e_TEXCOORD2],				e_PerVertex },
	{ 8, 0, e_TEXCOORD3,				e_TEXCOORD3MASK,				e_FLOAT3,		2, g_VertexAttributeSemantics[e_TEXCOORD3],				e_PerVertex },
	{ 9, 0, e_TEXCOORD4,				e_TEXCOORD4MASK,				e_FLOAT3,		3, g_VertexAttributeSemantics[e_TEXCOORD4],				e_PerVertex },
	{10, 0, e_COLOR,					e_COLORMASK,					e_UBYTE4N,		0, g_VertexAttributeSemantics[e_COLOR],					e_PerVertex },
	{11, 0, e_COLOR1,					e_COLOR1MASK,					e_UBYTE4N,		1, g_VertexAttributeSemantics[e_COLOR1],				e_PerVertex },
	{12, 0, e_INSTANCEROW1,				e_INSTANCEROW1MASK,				e_FLOAT4,		0, g_VertexAttributeSemantics[e_INSTANCEROW1],			e_PerInstance },
	{13, 0, e_INSTANCEROW2,				e_INSTANCEROW2MASK,				e_FLOAT4,		1, g_VertexAttributeSemantics[e_INSTANCEROW2],			e_PerInstance },
	{14, 0, e_INSTANCEROW3,				e_INSTANCEROW3MASK,				e_FLOAT4,		2, g_VertexAttributeSemantics[e_INSTANCEROW3],			e_PerInstance },
	{15, 0, e_INSTANCECOLOR,			e_INSTANCECOLORMASK,			e_FLOAT4,		0, g_VertexAttributeSemantics[e_INSTANCECOLOR],			e_PerInstance },
};

unsigned int g_VertexStreamSize[] = 
{
	12,		// e_POSITION,
	12,		// e_NORMAL,
	12,		// e_TANGENT,
	12,		// e_BITANGENT,
	12,		// e_TEXCOORD,	
	16,		// e_BLENDWEIGHT,
	4,		// e_BLENDINDICES,	
	12,		// e_TEXCOORD1,	
	12,		// e_TEXCOORD2,	
	12,		// e_TEXCOORD3,	
	4,		// e_COLOR,
	4,		// e_COLOR1,
	16,		// e_INSTANCEROW1
	16,		// e_INSTANCEROW2
	16,		// e_INSTANCEROW3
	16,		// e_INSTANCECOLOR
};

unsigned int g_VertexStreamOffsetNoSkin[] = 
{
	0,		// e_POSITION,
	12,		// e_NORMAL,
	24,		// e_TANGENT,
	36,		// e_BITANGENT,
	48,		// e_TEXCOORD,	
	60,		// e_BLENDWEIGHT,
	60,		// e_BLENDINDICES,	
	60,		// e_TEXCOORD1,	
	72,		// e_TEXCOORD2,	
	84,		// e_TEXCOORD3,	
	96,		// e_COLOR,
	100,	// e_COLOR1,
	0,		// e_INSTANCEROW1
	16,		// e_INSTANCEROW2
	32,		// e_INSTANCEROW3
	48,		// e_INSTANCECOLOR
};

unsigned int g_VertexStreamOffsetSkin[] =
{
	0,		// e_POSITION,
	12,		// e_NORMAL,
	24,		// e_TANGENT,
	36,		// e_BITANGENT,
	48,		// e_TEXCOORD,	
	64,		// e_BLENDWEIGHT,
	68,		// e_BLENDINDICES,	
	80,		// e_TEXCOORD1,	
	92,		// e_TEXCOORD2,	
	104,	// e_TEXCOORD3,	
	116,	// e_COLOR,
	120,	// e_COLOR1,
	0,		// e_INSTANCEROW1
	16,		// e_INSTANCEROW2
	32,		// e_INSTANCEROW3
	48,		// e_INSTANCECOLOR
};


SVertexElements g_VertexStreamStandardSemantics[] =
{
	{ 0, 0, e_POSITION,					e_POSITIONMASK,					e_FLOAT3,		0, g_VertexAttributeSemantics[e_POSITION],				e_PerVertex },
	{ 1, 0, e_TEXCOORD,					e_TEXCOORDMASK,					e_FLOAT2,		0, g_VertexAttributeSemantics[e_TEXCOORD],				e_PerVertex },
	{ 2, 0, e_COLOR,					e_COLORMASK,					e_UBYTE4N,		0, g_VertexAttributeSemantics[e_COLOR],					e_PerVertex },
	{ 3, 0, e_INSTANCEROW1,				e_INSTANCEROW1MASK,				e_FLOAT4,		0, g_VertexAttributeSemantics[e_INSTANCEROW1],			e_PerInstance },
	{ 4, 0, e_INSTANCEROW2,				e_INSTANCEROW2MASK,				e_FLOAT4,		1, g_VertexAttributeSemantics[e_INSTANCEROW2],			e_PerInstance },
	{ 5, 0, e_INSTANCEROW3,				e_INSTANCEROW3MASK,				e_FLOAT4,		2, g_VertexAttributeSemantics[e_INSTANCEROW3],			e_PerInstance },
	{ 6, 0, e_INSTANCECOLOR,			e_INSTANCECOLORMASK,			e_FLOAT4,		0, g_VertexAttributeSemantics[e_INSTANCECOLOR],			e_PerInstance },
};


unsigned int g_VertexStreamStandardSize[] =
{
	12,		// e_POSITION,
	8,		// e_TEXCOORD,
	4,		// e_COLOR,
	16,		// e_INSTANCEROW1
	16,		// e_INSTANCEROW2
	16,		// e_INSTANCEROW3
	16,		// e_INSTANCECOLOR
};

unsigned int g_VertexStreamStandardOffset[] =
{
	0,		// e_POSITION,
	12,		// e_TEXCOORD,
	20,		// e_COLOR,
	0,		// e_INSTANCEROW1
	16,		// e_INSTANCEROW2
	32,		// e_INSTANCEROW3
	48,		// e_INSTANCECOLOR
};


CMesh::CMesh()
{
	m_pVertexBuffer = NULL;
	m_pIndexBuffer = NULL;
	m_nStride = 0;
	m_nStreams = 0;
	m_Center = 0.f;
	m_AABB = 0.f;
	m_fBoundingSphereRadius = 0.f;
	m_pSkeletton = NULL;
	m_pSDF = NULL;

	m_nVertexCount = 0;
	m_nTriangleCount = 0;

	m_nID = (unsigned int)ms_pMeshesList.size();

	ms_pMeshesList.push_back(this);

	m_pPacketList = nullptr;
}



CMesh::~CMesh()
{
	delete[] m_pVertexBuffer;
	delete[] m_pIndexBuffer;

	if (m_pSDF)
		delete m_pSDF;
}



void CMesh::EnableSDF()
{
	CSDF* pSDF = new CSDF(*this, 128, 128, 128);
	pSDF->Bake();

	m_pSDF = pSDF;
}



void CMesh::RefreshSDF()
{
	ASSERT(m_pSDF != nullptr);

	((CSDF*)m_pSDF)->Bake();
}



void CMesh::SetCenter(float3& Center)
{
	for (int i = 0; i < m_nVertexCount; i++)
		Addi(&m_pVertexBuffer[i * m_nStride], (Center - m_Center).v());

	m_Center = Center;
}


float3 CMesh::GetVertexNormal(int i) const
{
	return float3(m_pVertexBuffer[i * m_nStride + 3], m_pVertexBuffer[i * m_nStride + 4], m_pVertexBuffer[i * m_nStride + 5]);
}



void CMesh::ClearAll()
{
	std::vector<CMesh*>::iterator it;

	for (it = ms_pMeshesList.begin(); it < ms_pMeshesList.end(); it++)
	{
		delete (*it);
	}

	ms_pMeshesList.clear();
}
