#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Instance.h"


CInstance::CInstance(CInstancedMesh* pMesh)
{
	if (pMesh != NULL)
	{
		m_nMeshID = pMesh->GetID();

		m_pInstancedMesh = pMesh;
	}

	else
	{
		m_nMeshID = INVALIDHANDLE;

		m_pBones.clear();

		m_pInstancedMesh = NULL;
	}

	m_ModelMatrix.Eye();
	m_pSkeletton = NULL;
}


CInstance::~CInstance()
{
	if (m_pSkeletton != NULL)
		delete m_pSkeletton;

	std::vector<CBone*>::iterator it1;

	for (it1 = m_pBones.begin(); it1 < m_pBones.end(); it1++)
	{
		delete (*it1);
	}

	m_pBones.clear();
}


void CInstance::StartAnim(unsigned int nID)
{
	CAnimation* pAnim = m_pSkeletton->GetAnim(nID);

	m_pAnimStartingTime[nID] = CEngine::GetEngineTime();

	m_pRunningAnims.push_back(pAnim);
}


void CInstance::StopAnim(unsigned int nID)
{
	CAnimation* pAnim = m_pSkeletton->GetAnim(nID);

	std::vector<CAnimation*>::iterator it;

	for (it = m_pRunningAnims.begin(); it < m_pRunningAnims.end(); it++)
	{
		if (*it == pAnim)
		{
			m_pRunningAnims.erase(it);
			break;
		}
	}
}


void CInstance::ProcessAnims()
{
	std::vector<CAnimation*>::iterator it;
	float fTime = CEngine::GetEngineTime();
	unsigned int nID;

	for (it = m_pRunningAnims.begin(); it < m_pRunningAnims.end(); it++)
	{
		nID = (*it)->GetID();

		if (fTime - m_pAnimStartingTime[nID] > (*it)->GetDuration())
		{
			m_pRunningAnims.erase(it);
			if (it > m_pRunningAnims.begin())
				it--;
		}

		else
		{
			ProcessSingleAnim(*it, fTime - m_pAnimStartingTime[nID]);
		}
	}
}


void CInstance::Transform()
{
	ComputeTransformedBones();

	//m_pPacketList->SetTransformedBones(m_pTransformedBones);
}


void CInstance::Draw(ERenderList eType)
{
	m_pInstancedMesh->AddInstance(m_ModelMatrix, eType);
}


void CInstance::Draw(float4& Color, ERenderList eType)
{
	m_pInstancedMesh->AddInstance(m_ModelMatrix, Color, eType);
}


void CInstance::ComputeTransformedBonesInConnexSkeletton(CBone* pRoot)
{
	pRoot->ComputeGlobalTransform();

	int nChildrenCount = pRoot->GetChildrenCount();

	for (int i = 0; i < nChildrenCount; i++)
		ComputeTransformedBonesInConnexSkeletton(pRoot->GetChild(i));
}


void CInstance::ComputeTransformedBones()
{
	std::vector<CBone*>::iterator it;

	for (it = m_pBones.begin(); it < m_pBones.end(); it++)
	{
		if ((*it)->GetParent() == NULL)
		{
			ComputeTransformedBonesInConnexSkeletton(*it);
		}
	}
}


void CInstance::ProcessSingleAnim(CAnimation* pAnim, float fTime)
{
	std::vector<SBoneAnim*>::iterator it;

	for (it = pAnim->m_pChannels.begin(); it < pAnim->m_pChannels.end(); it++)
	{
		CBone* pBone = m_pBones[(*it)->m_nBoneID];
		float3 Pos, Scale;
		Quaternion Rot;

		(*it)->GetKeys(Pos, Rot, Scale, fTime);

		DualQuaternion p(Rot, Pos);

		pBone->SetTransform(p);
	}
}


CInstancedMesh::CInstancedMesh(CMesh* pMesh)
{
	m_pMesh = pMesh;

	m_pPackets = CPacketManager::MeshToPacketList(m_pMesh);

	for (int i = 0; i < MAX_DRAWABLE_LIST_COUNT; i++)
	{
		m_nNbInstances[i] = 0;
		m_InstancedTransformData[i].clear();
	}

	//m_nInstanceBufferID = CDeviceManager::CreateVertexBuffer(NULL, MAX_INSTANCES * sizeof(STransformData), true);
}


CInstancedMesh::~CInstancedMesh()
{
	delete m_pPackets;

	//CDeviceManager::DeleteVertexBuffer(m_nInstanceBufferID);
}


void CInstancedMesh::AddInstance(float4x4 m_World, ERenderList eType)
{
	m_nNbInstances[eType]++;

	STransformData TID;
	TID.m_Color = 1.f;
	TID.m_WorldMatrix = m_World;
	TID.m_WorldMatrix.transpose();

	m_InstancedTransformData[eType].push_back(TID);
}


void CInstancedMesh::AddInstance(float4x4 m_World, float4& Color, ERenderList eType)
{
	m_nNbInstances[eType]++;

	STransformData TID;
	TID.m_Color = Color;
	TID.m_WorldMatrix = m_World;
	TID.m_WorldMatrix.transpose();

	m_InstancedTransformData[eType].push_back(TID);
}


void CInstancedMesh::Draw(ERenderList eList)
{
	if (m_nNbInstances[eList] == 0)
		return;

	/*CDeviceManager::FillVertexBuffer(m_nInstanceBufferID, m_InstancedTransformData[eList].data(), m_nNbInstances[eList] * sizeof(STransformData));

	m_pPackets->m_nNbInstances				= m_nNbInstances[eList];
	m_pPackets->m_nInstanceBufferID			= m_nInstanceBufferID;
	m_pPackets->m_nInstancedBufferStride	= 24 * sizeof(float);

	m_pPackets->m_nInstancedStreamMask = (1 << CStream::e_InstanceWorld0) | (1 << CStream::e_InstanceWorld1) | (1 << CStream::e_InstanceWorld2) | (1 << CStream::e_InstanceWorld3) | (1 << CStream::e_InstanceColor);

	CPacketManager::AddPacketList(m_pPackets, false, eList);*/
}


void CInstancedMesh::Clear()
{
	for (int i = 0; i < MAX_DRAWABLE_LIST_COUNT; i++)
	{
		m_InstancedTransformData->clear();
		m_nNbInstances[i] = 0;
	}
}
