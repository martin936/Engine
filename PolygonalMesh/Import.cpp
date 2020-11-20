#include "PolygonalMesh.h"
#include "Engine/Materials/MaterialDefinition.h"
#include "Engine/Misc/String.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>


int GetHead(char* pDest, int nSize, const char* pSource)
{
	int count = 0;
	int i = 0;

	while (pSource[i] != '\0' && count < 255 && pSource[i] != '\n')
	{
		if (pSource[i] != ' ' && pSource[i] != '\t')
		{
			pDest[count] = pSource[i];
			count++;
		}
		else if (count > 0)
			break;

		i++;
	}

	pDest[count] = '\0';

	return count + 1;
}


void CMesh::Save(const char* cFilePath)
{
	FILE* pFile = NULL;
	pFile = fopen(cFilePath, "wb+");
	if (pFile == NULL)
	{
		printf("Error %d\n", errno);
	}

	else
	{
		int nPacketCount = (int)m_PacketInfo.size();
		std::vector<SPacketInfo>::iterator it;

		fwrite(&m_nVertexCount, sizeof(int), 1, pFile);
		fwrite(&m_nTriangleCount, sizeof(int), 1, pFile);
		fwrite(&m_nStride, sizeof(int), 1, pFile);
		fwrite(&m_nStreams, sizeof(int), 1, pFile);
		fwrite(&nPacketCount, sizeof(int), 1, pFile);

		fwrite(m_pVertexBuffer, m_nStride * sizeof(float), m_nVertexCount, pFile); 
		fwrite(m_pIndexBuffer, sizeof(unsigned int), 3 * m_nTriangleCount, pFile);

		SSavedPacketInfo SavedPacketInfo;

		for (it = m_PacketInfo.begin(); it < m_PacketInfo.end(); it++)
		{
			SavedPacketInfo.m_nStartIndex	= it->m_nStartIndex;
			SavedPacketInfo.m_nPacketSize	= it->m_nPacketSize;
			SavedPacketInfo.m_bIsUVMapped	= it->m_bIsUVMapped ? 1 : 0;
			SavedPacketInfo.m_nStreams		= it->m_nStreams;

			sprintf_s(SavedPacketInfo.m_cMaterial, "%s", it->m_cName);

			fwrite(&SavedPacketInfo, sizeof(SSavedPacketInfo), 1, pFile);
		}

		fclose(pFile);
	}
}


CMesh* CMesh::LoadMesh(const char* cFilePath)
{
	CMesh* pMesh = new CMesh;

	pMesh->Load(cFilePath);

	return pMesh;
}


void CMesh::Load(const char* cFilePath)
{
	FILE* pFile = NULL;
	errno = 0;
	m_bIsLoaded = false;

	char cDirectory[512] = "";
	CString::GetDirectory(cDirectory, cFilePath);

	CMaterial::SetDirectory(cDirectory);

	pFile = fopen(cFilePath, "r");
	if (pFile == NULL)
	{
		printf("Error %d\n", errno);
	}

	if (strstr(cFilePath, ".mesh") != NULL)
	{
		fclose(pFile);
		pFile = fopen(cFilePath, "rb");
		int nPacketCount;

		fread_s(&m_nVertexCount, sizeof(int), sizeof(int), 1, pFile);
		fread_s(&m_nTriangleCount, sizeof(int), sizeof(int), 1, pFile);
		fread_s(&m_nStride, sizeof(int), sizeof(int), 1, pFile);
		fread_s(&m_nStreams, sizeof(int), sizeof(int), 1, pFile);
		fread_s(&nPacketCount, sizeof(int), sizeof(int), 1, pFile);

		m_pVertexBuffer = new float[m_nStride * m_nVertexCount];
		m_pIndexBuffer = new unsigned int[3 * m_nTriangleCount];

		fread(m_pVertexBuffer, m_nStride * sizeof(float), m_nVertexCount, pFile);
		fread(m_pIndexBuffer, sizeof(unsigned int), 3 * m_nTriangleCount, pFile);

		SSavedPacketInfo SavedPacketInfo;
		SPacketInfo PacketInfo;

		for (int i = 0; i < nPacketCount; i++)
		{
			fread_s(&SavedPacketInfo, sizeof(SSavedPacketInfo), sizeof(SSavedPacketInfo), 1, pFile);

			PacketInfo.m_nStartIndex = SavedPacketInfo.m_nStartIndex;
			PacketInfo.m_nPacketSize = SavedPacketInfo.m_nPacketSize;
			PacketInfo.m_bIsUVMapped = SavedPacketInfo.m_bIsUVMapped ? true : false;
			PacketInfo.m_nStreams = SavedPacketInfo.m_nStreams;

			PacketInfo.m_pMaterial = CMaterial::GetMaterial(SavedPacketInfo.m_cMaterial);

			m_PacketInfo.push_back(PacketInfo);
		}

		fclose(pFile);

		float3 CurrentPoint;
		float fXMax = -1e9f, fYMax = -1e9f, fZMax = -1e9f;
		float fXMin = 1e9f, fYMin = 1e9f, fZMin = 1e9f;
		float fCurrentRadius;
		m_fBoundingSphereRadius = 0.f;

		for (int i = 0; i < m_nVertexCount; i++)
		{
			memcpy(CurrentPoint.v(), &m_pVertexBuffer[m_nStride * i], 3 * sizeof(float));

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

		m_AABB = float3(fXMax - fXMin, fYMax - fYMin, fZMax - fZMin);

		m_Center = 0.5f * float3(fXMin + fXMax, fYMin + fYMax, fZMin + fZMax);

		for (int i = 0; i < m_nVertexCount; i++)
		{
			memcpy(CurrentPoint.v(), &m_pVertexBuffer[m_nStride * i], 3 * sizeof(float));
			fCurrentRadius = (CurrentPoint - m_Center).length();

			if (fCurrentRadius > m_fBoundingSphereRadius)
				m_fBoundingSphereRadius = fCurrentRadius;
		}

		m_bIsLoaded = true;
	}


	char cAnimFileName[256];
	char* ptr;

	strcpy_s(cAnimFileName, cFilePath);
	ptr = strchr(cAnimFileName, '.');
	sprintf(ptr, ".anim");

	m_pSkeletton = CSkeletton::Load(cAnimFileName);
}

