#include "Toolbox.h"
#include "engine/Inputs/inputs.h"
#include "engine/physics/physics.h"

CMesh*										CCollisionBoxManager::ms_pBaseMesh		= NULL;

CCollisionBoxManager::SBoxList*				CCollisionBoxManager::ms_pSelectedBox	= NULL;
CCollisionBoxManager::SBoxList*				CCollisionBoxManager::ms_pBoxList		= NULL;
CCollisionBoxManager::SBoxList*				CCollisionBoxManager::ms_pLastBox		= NULL;
int											CCollisionBoxManager::ms_nBoxCount		= 0;

std::vector<CCollisionBoxManager::SSegment> CCollisionBoxManager::ms_Segments;


CCollisionBox::CCollisionBox()
{
	m_pAxisPicker = new CAxisPicker();

	m_vCenter = 0.f;
	m_vSize = 1.f;

	m_bSelected = false;
	m_bEnabled = false;
	m_bGrabbed = false;

	m_bShouldScale = false;
	m_nSelectedAxis = -1;

	if (CCollisionBoxManager::ms_pBaseMesh == NULL)
		CCollisionBoxManager::ms_pBaseMesh = CMesh::LoadMesh("../Data/Models/Misc/Box.mesh");

	m_pPackets = CPacketManager::MeshToPacketList(CCollisionBoxManager::ms_pBaseMesh);

	m_pObstacle = new CObstacle(CCollisionBoxManager::ms_pBaseMesh, true, true);
	m_pTransformedObstacle = new CObstacle(CCollisionBoxManager::ms_pBaseMesh, true, true);
	
	m_pPackets->m_pPackets[0].m_pShaderHook = CDeferredRenderer::UpdateShader;
	m_pPackets->m_pPackets[0].m_pMaterial = CMaterial::GetMaterial("None");

	m_pAxisPicker->SetPosition(m_vCenter);
	m_vSize = float3(0.5f, 0.5f, 0.5f);

	ComputeModelMatrix();

	ApplyModelMatrix();

	//CPhysicsEngine::Add(m_pTransformedObstacle);
}


CCollisionBox::~CCollisionBox()
{
	delete m_pAxisPicker;

	//CPhysicsEngine::Remove(m_pTransformedObstacle);

	delete m_pObstacle;
	delete m_pPackets;
}


void CCollisionBox::Draw()
{
	float4 Color;
	if (m_bSelected)
		Color = float4(1.f, 1.f, 0.4f, 0.3f);
	else
		Color = float4(1.f, 1.f, 1.f, 1.f); //float4(0.f, 0.23f, 0.435f, 0.6f);

	CPacketManager::AddPacketList(*m_pPackets, false, e_RenderType_Standard);
	
	if (m_bSelected)
		m_pAxisPicker->Draw();
}


void CCollisionBox::ApplyModelMatrix()
{
	int nWallCount = m_pObstacle->GetWallCount();
	int i, j;
	float4 vector;
	float4x4 InverseModelMatrix = inverse(m_ModelMatrix);
	

	for (i = 0; i < nWallCount; i++)
	{
		for (j = 0; j < 3; j++)
		{
			Copy(&vector.x, m_pObstacle->m_pWalls[i].m_Vertex[j]);
			vector.w = 1.f;

			vector = m_ModelMatrix * vector;
			vector = vector / vector.w;
			Copy(m_pTransformedObstacle->m_pWalls[i].m_Vertex[j], &vector.x);
		}

		Copy(&vector.x, m_pObstacle->m_pWalls[i].m_Normal);
		vector.w = 0.f;

		vector = InverseModelMatrix * vector;
		vector.normalize();
		Copy(m_pTransformedObstacle->m_pWalls[i].m_Normal, &vector.x);

		for (j = 0; j < 2; j++)
		{
			Copy(&vector.x, m_pObstacle->m_pWalls[i].m_TangentBasis[j]);
			vector.w = 0.f;

			vector = m_ModelMatrix * vector;
			vector.normalize();
			Copy(m_pTransformedObstacle->m_pWalls[i].m_TangentBasis[j], &vector.x);
		}
	}

	Copy(m_pTransformedObstacle->m_Center, &m_vCenter.x);
	m_pTransformedObstacle->ComputeBoundingVolumes();

	//CPhysicsEngine::ms_bShouldUpdateObstacles = true;
}


void CCollisionBox::Update()
{
	CMouse* pMouse = CMouse::GetCurrent();

	float4x4 InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	float3 CamPos = CRenderer::GetCurrentCamera()->GetPosition();

	float fMouseX, fMouseY;
	pMouse->GetPos(&fMouseX, &fMouseY);

	float4 vScreenMouse = float4(2.f*fMouseX - 1.f, 1.f - 2.f*fMouseY, 1.f, 1.f);

	float4 vWorldMouse = InvViewProj * vScreenMouse;
	float4 vReducedWorldMouse = vWorldMouse / vWorldMouse.w;

	float3 WorldMouse = float3(vReducedWorldMouse.x, vReducedWorldMouse.y, vReducedWorldMouse.z);
	float3 Dir = WorldMouse - CamPos;

	float t = -CamPos.y / Dir.y;

	float3 NewPos = CamPos + t*Dir;

#ifdef __OPENGL__

	if (m_bSelected)
	{
		CKeyboard* pKeyboard = CKeyboard::GetCurrent();

		if (pKeyboard->IsPressed(GLFW_KEY_S))
		{
			m_bShouldScale = true;
			m_nSelectedAxis = -1;
			m_vSavedSize = m_vSize;
		}

		if (m_bShouldScale)
		{

			if (pKeyboard->IsPressed(GLFW_KEY_X))
			{
				m_nSelectedAxis = 0;
				m_fSavedRadius = (NewPos - m_vCenter).length();
			}

			else if (pKeyboard->IsPressed(GLFW_KEY_Y))
			{
				m_fSavedRadius = (NewPos - m_vCenter).length();
				m_nSelectedAxis = 2;
			}

			else if (pKeyboard->IsPressed(GLFW_KEY_Z))
			{
				m_nSelectedAxis = 1;
				m_fSavedRadius = (NewPos - m_vCenter).length();
			}

			if (m_nSelectedAxis > -1)
			{
				if (pMouse->IsPressed(CMouse::e_Button_LeftClick))
				{
					m_bShouldScale = false;
					m_nSelectedAxis = -1;
				}

				else if (pMouse->IsPressed(CMouse::e_Button_RightClick))
				{
					m_bShouldScale = false;
					m_vSize = m_vSavedSize;
					m_nSelectedAxis = -1;
				}

				else
				{
					float fRadius = (NewPos - m_vCenter).length();
					m_vSize.v()[m_nSelectedAxis] = m_vSavedSize.v()[m_nSelectedAxis] * fRadius / m_fSavedRadius;
				}
			}
		}

		m_pAxisPicker->Update();
		m_vCenter = m_pAxisPicker->GetPosition();

		ComputeModelMatrix();
	}
#endif
}


void CCollisionBox::ComputeModelMatrix(void)
{
	m_ModelMatrix.m00 = m_vSize.x;
	m_ModelMatrix.m01 = 0.f;
	m_ModelMatrix.m02 = 0.f;
	m_ModelMatrix.m03 = m_vCenter.x;
	m_ModelMatrix.m10 = 0.f;
	m_ModelMatrix.m11 = m_vSize.y;
	m_ModelMatrix.m12 = 0.f;
	m_ModelMatrix.m13 = m_vCenter.y;
	m_ModelMatrix.m20 = 0.f;
	m_ModelMatrix.m21 = 0.f;
	m_ModelMatrix.m22 = m_vSize.z;
	m_ModelMatrix.m23 = m_vCenter.z;
	m_ModelMatrix.m30 = m_ModelMatrix.m31 = m_ModelMatrix.m32 = 0.f;
	m_ModelMatrix.m33 = 1.f;
}


/*void CCollisionBoxManager::Reload()
{
	CAxisPicker::Init();
	ms_pBaseMesh = CMesh::LoadMesh("../Data/Models/Misc/Box.mesh");
	SBoxList* pCurrent = m_pBoxList;
	SBoxList* pTmpBox;

	while (pCurrent != NULL)
	{
		pTmpBox = pCurrent;
		delete pCurrent->m_pBox;

		pCurrent = pCurrent->m_pNext;

		delete pTmpBox;
	}

	m_pBoxList = NULL;

	m_Segments.clear();
}*/


void CCollisionBoxManager::Init()
{
	ms_pBoxList = NULL;
	ms_pSelectedBox = NULL;
	ms_nBoxCount = 0;

	CAxisPicker::Init();

	if (ms_pBaseMesh == NULL)
		ms_pBaseMesh = CMesh::LoadMesh("../Data/Models/Misc/Box.mesh");
}


void CCollisionBoxManager::Terminate()
{
	SBoxList* pCurrent = ms_pBoxList;
	SBoxList* pTmpBox;

	while (pCurrent != NULL)
	{
		pTmpBox = pCurrent;
		delete pCurrent->m_pBox;

		pCurrent = pCurrent->m_pNext;

		delete pTmpBox;
	}

	ms_pBoxList = NULL;
	ms_pBaseMesh = NULL;

	ms_Segments.clear();
}


void CCollisionBoxManager::Save(const char* cPath)
{
	FILE* pFile;
	SBoxList* pCurrent = ms_pBoxList;
	fopen_s(&pFile, cPath, "wb+");

	fwrite(&ms_nBoxCount, sizeof(int), 1, pFile);
	
	for (int i = 0; i < ms_nBoxCount && pCurrent != NULL; i++)
	{
		fwrite(pCurrent->m_pBox->m_vCenter.v(), sizeof(float), 3, pFile);
		fwrite(pCurrent->m_pBox->m_vSize.v(), sizeof(float), 3, pFile);

		pCurrent = pCurrent->m_pNext;
	}

	fclose(pFile);
}


void CCollisionBoxManager::Load(const char* cPath)
{
	SBoxList* pCurrent = ms_pBoxList;
	SBoxList* pTmpBox;

	while (pCurrent != NULL)
	{
		pTmpBox = pCurrent;
		delete pCurrent->m_pBox;

		pCurrent = pCurrent->m_pNext;

		delete pTmpBox;
	}

	ms_pBoxList = NULL;
	ms_pLastBox = NULL;
	ms_pSelectedBox = NULL;
	ms_nBoxCount = 0;

	FILE* pFile;
	float3 Center = float3(0.f, 0.f, 0.f);
	float3 Size = float3(0.f, 0.f, 0.f);
	int nBoxCount = 0;
	
	fopen_s(&pFile, cPath, "rb");

	fread(&nBoxCount, sizeof(int), 1, pFile);

	for (int i = 0; i < nBoxCount; i++)
	{
		fread(&Center.x, sizeof(float), 3, pFile);
		fread(&Size.x, sizeof(float), 3, pFile);

		AddBox();
		ms_pLastBox->m_pBox->m_vCenter = Center;
		ms_pLastBox->m_pBox->m_vSize = Size;
		ms_pLastBox->m_pBox->m_pAxisPicker->SetPosition(Center);

		ms_pLastBox->m_pBox->ComputeModelMatrix();
		ms_pLastBox->m_pBox->ApplyModelMatrix();
	}

	ms_nBoxCount = nBoxCount;

	fclose(pFile);
}


void CCollisionBoxManager::AddBox()
{
	if (ms_pBoxList == NULL)
	{
		ms_pBoxList = new SBoxList;
		ms_pBoxList->m_pBox = new CCollisionBox;
		ms_pBoxList->m_pNext = NULL;
		ms_nBoxCount++;

		ms_pLastBox = ms_pBoxList;
	}

	else
	{
		ms_pLastBox->m_pNext = new SBoxList;
		ms_pLastBox = ms_pLastBox->m_pNext;

		ms_pLastBox->m_pBox = new CCollisionBox;
		ms_pLastBox->m_pNext = NULL;
		ms_nBoxCount++;
	}
}



void CCollisionBoxManager::RemoveSelectedBox()
{
	if (ms_pSelectedBox != NULL)
	{
		SBoxList* pCurrent = ms_pBoxList;

		if (ms_pBoxList == ms_pSelectedBox)
			ms_pBoxList = ms_pBoxList->m_pNext;

		else
		{
			while (pCurrent != NULL)
			{
				if (pCurrent->m_pNext == ms_pSelectedBox)
					break;

				pCurrent = pCurrent->m_pNext;
			}

			if (pCurrent != NULL)
			{
				pCurrent->m_pNext = ms_pSelectedBox->m_pNext;
				ms_pLastBox = ms_pSelectedBox->m_pNext ? ms_pSelectedBox->m_pNext : pCurrent;
			}
		}

		delete ms_pSelectedBox->m_pBox;
		delete ms_pSelectedBox;
		ms_pSelectedBox = NULL;

		ms_nBoxCount--;
	}
}


void CCollisionBoxManager::Update()
{
	SBoxList* pCurrent = ms_pBoxList;
	SelectBox();

	while (pCurrent != NULL)
	{
		pCurrent->m_pBox->Update();
		pCurrent = pCurrent->m_pNext;
	}
}


void CCollisionBoxManager::Draw()
{
	SBoxList* pCurrent = ms_pBoxList;

	while (pCurrent != NULL)
	{
		pCurrent->m_pBox->Draw();
		pCurrent = pCurrent->m_pNext;
	}
}


void CCollisionBoxManager::Clear()
{
	SBoxList* pCurrent = ms_pBoxList;

	while (pCurrent != NULL)
	{
		SBoxList* pToDelete = pCurrent;
		pCurrent = pCurrent->m_pNext;

		delete pToDelete;
	}

	ms_pBoxList = NULL;

	ms_pBaseMesh = NULL;
}


void CCollisionBoxManager::SelectBox()
{
	CMouse* pMouse = CMouse::GetCurrent();

	float4x4 InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	float3 CamPos = CRenderer::GetCurrentCamera()->GetPosition();

	float fMouseX, fMouseY;
	pMouse->GetPos(&fMouseX, &fMouseY);

	float4 vScreenMouse = float4(2.f*fMouseX - 1.f, 1.f - 2.f*fMouseY, 1.f, 1.f);

	float4 vWorldMouse = InvViewProj * vScreenMouse;
	float4 vReducedWorldMouse = vWorldMouse / vWorldMouse.w;

	float3 WorldMouse = float3(vReducedWorldMouse.x, vReducedWorldMouse.y, vReducedWorldMouse.z);
	float3 Dir = WorldMouse - CamPos;

	float t = -CamPos.y / Dir.y;

	float3 NewPos = CamPos + t*Dir;


	if (pMouse->IsPressed(CMouse::e_Button_LeftClick))
	{
		SBoxList* pCurrent = ms_pBoxList;
		SBoxList* pSelectedBox = NULL;
		CCollisionBox* pCurrentBox;
		float fMinLength = 1e9f, fTmpLength;

		while (pCurrent != NULL)
		{
			pCurrentBox = pCurrent->m_pBox;
			if (!pCurrentBox->m_bShouldScale)
			{
				if (fabs(NewPos.x - pCurrentBox->m_vCenter.x) < pCurrentBox->m_vSize.x && fabs(NewPos.z - pCurrentBox->m_vCenter.z) < pCurrentBox->m_vSize.z)
				{
					fTmpLength = (NewPos - pCurrentBox->m_vCenter).length();
					if (fTmpLength < fMinLength)
					{
						fMinLength = fTmpLength;
						pSelectedBox = pCurrent;
					}
				}
			}

			pCurrent = pCurrent->m_pNext;
		}

		if (pSelectedBox != NULL)
		{
			if (ms_pSelectedBox != NULL)
			{
				ms_pSelectedBox->m_pBox->m_bSelected = false;
				ms_pSelectedBox->m_pBox->m_bShouldScale = false;
			}

			ms_pSelectedBox = pSelectedBox;
			ms_pSelectedBox->m_pBox->m_bSelected = true;
			ms_pSelectedBox->m_pBox->m_bGrabbed = true;
		}
	}

	else if (pMouse->IsPressed(CMouse::e_Button_RightClick))
	{
		if (ms_pSelectedBox != NULL)
		{
			if (!ms_pSelectedBox->m_pBox->m_bShouldScale)
			{
				ms_pSelectedBox->m_pBox->m_bSelected = false;
				ms_pSelectedBox->m_pBox->m_bGrabbed = false;
				ms_pSelectedBox->m_pBox->ApplyModelMatrix();
				ms_pSelectedBox = NULL;
			}
		}
	}

	else if (ms_pSelectedBox != NULL)
		ms_pSelectedBox->m_pBox->m_bGrabbed = false;
}



void CCollisionBoxManager::InitPositionGenerator(float fBorder)
{
	SBoxList* pBox = ms_pBoxList;
	float fOffset = 0.f;

	while (pBox != NULL)
	{
		float3 Center = pBox->m_pBox->m_vCenter;
		float3 Size = pBox->m_pBox->m_vSize;

		SSegment seg;
		seg.m_Start = Center + float3(-Size.x + fBorder, 0.f, Size.z);
		seg.m_End = Center + float3(Size.x - fBorder, 0.f, Size.z);
		seg.m_fSegmentStart = fOffset;
		seg.m_fSegmentEnd = fOffset + (seg.m_End - seg.m_Start).length();

		fOffset += seg.m_fSegmentEnd - seg.m_fSegmentStart;

		if (seg.m_End.x - seg.m_Start.x > 0.f)
			ms_Segments.push_back(seg);

		pBox = pBox->m_pNext;
	}

	std::vector<SSegment>::iterator it;

	for (it = ms_Segments.begin(); it < ms_Segments.end(); it++)
	{
		(*it).m_fSegmentStart /= fOffset;
		(*it).m_fSegmentEnd /= fOffset;
	}
}



float3 CCollisionBoxManager::GetRandomSpawningPosition()
{
	float nb = 1.f * std::rand() / RAND_MAX;

	std::vector<SSegment>::iterator it;

	for (it = ms_Segments.begin(); it < ms_Segments.end(); it++)
	{
		if (nb > (*it).m_fSegmentStart && nb < (*it).m_fSegmentEnd)
		{
			float t = ((*it).m_fSegmentEnd - nb) / ((*it).m_fSegmentEnd - (*it).m_fSegmentStart);

			return (1.f - t) * (*it).m_Start + t * (*it).m_End;
		}
	}

	return float3(0.f, 0.f, 0.f);
}

