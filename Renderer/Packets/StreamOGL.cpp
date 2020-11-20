#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Stream.h"


const CStream::SStreamDesc CStream::ms_StreamDescription[] = 
{
	{	e_Position,				e_Float,	3	},
	{	e_Normal,				e_Float,	3	},
	{	e_Tangent,				e_Float,	3	},
	{	e_Bitangent,			e_Float,	3	},
	{	e_Texcoord,				e_Float,	3	},
	{	e_BoneIndices,			e_Int,		4	},
	{	e_BoneWeights,			e_Float,	4	},
	{	e_Color,				e_Float,	3	},
	{	e_InstancePosition,		e_Float,	3	},
	{	e_InstanceWorld0,		e_Float,	4	},
	{	e_InstanceWorld1,		e_Float,	4	},
	{	e_InstanceWorld2,		e_Float,	4	},
	{	e_InstanceWorld3,		e_Float,	4	},
	{	e_InstanceColor,		e_Float,	4	}
};


CStream::CStream(EDataType eType, int nNbComponents, size_t nSize, size_t nStride)
{
	m_nStride = nStride;
	m_nSize = nSize;
	m_eType = eType;
	m_nNbComponents = nNbComponents;

	m_pData = new char[nSize];

	m_nBufferID = 0;
}


CStream::~CStream()
{
	if (m_pData != NULL)
	{
		delete[] m_pData;
	}

	CDeviceManager::DeleteVertexBuffer(m_nBufferID);
}


void CStream::Load(void* pData, unsigned int nFlags)
{
	if (pData != NULL)
		memcpy(m_pData, pData, m_nSize);

	if (nFlags & STREAM_DATA_STATIC)
		m_nBufferID = CDeviceManager::CreateVertexBuffer(m_pData, m_nSize, false);

	else if (nFlags & STREAM_DATA_DYNAMIC)
		m_nBufferID = CDeviceManager::CreateVertexBuffer(m_pData, m_nSize, false);
}


void CStream::Set(unsigned int nSlot)
{
	if (m_pData)
		CDeviceManager::BindVertexBuffer(m_nBufferID, nSlot, m_nNbComponents, m_eType);
}



void CStream::SetMultiStreams(unsigned int nBufferID, unsigned int nStride, unsigned int nLayoutMask)
{
	glBindBuffer(GL_ARRAY_BUFFER, nBufferID);
	unsigned int nOffset = 0U;

	for (unsigned int i = 0; i < MAX_STREAM_COUNT; i++)
	{
		if (nLayoutMask & (1U << i))
		{
			CStream::EStreamID eID = CStream::ms_StreamDescription[i].m_eID;
			CStream::EDataType eType = CStream::ms_StreamDescription[i].m_eType;
			int nNbComponents = CStream::ms_StreamDescription[i].m_nComponents;
	
			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, nNbComponents, GL_FLOAT, GL_FALSE, nStride * sizeof(float), (GLvoid*)((char*)nullptr + nOffset * sizeof(GL_FLOAT)));

			nOffset += nNbComponents;
		}
	}
}


void CStream::SetMultiStreamsInstanced(unsigned int nBufferID, unsigned int nStride, unsigned int nByteOffset, unsigned int nLayoutMask)
{
	unsigned int nOffset = nByteOffset;

	for (unsigned int i = 8U; i < MAX_STREAM_COUNT; i++)
	{
		if (nLayoutMask & (1U << i))
		{
			CStream::EStreamID eID		= CStream::ms_StreamDescription[i].m_eID;
			CStream::EDataType eType	= CStream::ms_StreamDescription[i].m_eType;
			int nNbComponents			= CStream::ms_StreamDescription[i].m_nComponents;

			glEnableVertexAttribArray(i);
			glBindBuffer(GL_ARRAY_BUFFER, nBufferID);
			glVertexAttribPointer(i, nNbComponents, GL_FLOAT, GL_FALSE, nStride, (GLvoid*)((char*)nullptr + nOffset));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glVertexAttribDivisor(i, 1);

			nOffset += nNbComponents * sizeof(GL_FLOAT);
		}
	}
}
