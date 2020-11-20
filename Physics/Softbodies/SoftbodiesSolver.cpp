#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Editor/Adjustables/Adjustables.h"

EXPORT_ADJUSTABLE(float, gs_fFriction)
EXPORT_ADJUSTABLE(float, gs_fStiffness)
EXPORT_ADJUSTABLE(float, gs_fRelativeSpeed)
EXPORT_ADJUSTABLE(float, gs_fAbsoluteSpeed)


void CSoftbody::BuildBuffers()
{
	if (m_bLoaded)
		return;

	SGlobalInfo GlobalInfos =
	{
		(unsigned int)m_pMesh->m_nVertexCount,
		(unsigned int)m_pMesh->m_nTriangleCount,
		(unsigned int)m_nMaxNeighbourCount,
		0U,
		m_fRestVolume,
		m_fRestVolume,
		m_fInternalEnergy,
		m_fCompressibility,
		m_fMass,
		m_fSpring,
		gs_fFriction,
		m_fDamping,
		0.f,
		gs_fRelativeSpeed,
		gs_fAbsoluteSpeed,
		{ m_pMesh->m_Center.x, m_pMesh->m_Center.y, m_pMesh->m_Center.z },
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, -9.81f },
		{ 0.f, 0.f, 0.f },
		0.f,
		0U
	};

	memcpy(&m_GlobalInfos, &GlobalInfos, sizeof(SGlobalInfo));

	//m_nGlobalInfoBufferID = CDeviceManager::CreateStorageBuffer(&m_GlobalInfos, sizeof(SGlobalInfo));

	SNeighbourInfo* pLocalData = new SNeighbourInfo[m_pMesh->m_nVertexCount * m_nMaxNeighbourCount];

	for (int i = 0; i < m_pMesh->m_nVertexCount; i++)
	{
		for (int j = 0; j < (int)m_pMesh->m_pNeighbourCount[i]; j++)
		{
			pLocalData[i * m_nMaxNeighbourCount + j].m_nIndex = m_pMesh->m_pNeighbourIndex[i][j];
			pLocalData[i * m_nMaxNeighbourCount + j].m_fRestLength = m_pRestLength[i][j];
		}
	}

	//m_nLocalInfoBufferID = CDeviceManager::CreateStorageBuffer(pLocalData, m_pMesh->m_nVertexCount * m_nMaxNeighbourCount * sizeof(SNeighbourInfo));
	//m_nNeighbourCountBufferID = CDeviceManager::CreateStorageBuffer(m_pMesh->m_pNeighbourCount, m_pMesh->m_nVertexCount * sizeof(unsigned int));

	delete[] pLocalData;

	m_bLoaded = true;

	//CDeviceManager::SetMemoryBarrier();
}


void CSoftbody::ComputeVolumeOnGPU()
{

	//GLuint nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Softbody_ComputeVolume];
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalInfoBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_pMesh->m_nVertexBufferID, 1, sizeof(CPhysicalMesh::SPhysicalVertex), 0, m_pMesh->m_nVertexCount);
	//
	//CDeviceManager::BindStorageBuffer(m_pMesh->m_nIndexBufferID, 2, sizeof(unsigned int), 0, 3 * m_pMesh->m_nTriangleCount);
	//
	//CDeviceManager::Dispatch((m_pMesh->m_nTriangleCount + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}

void CSoftbody::RunOnGPU(float dt)
{

	//GLuint nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Softbody_Main];
	//
	//size_t nObstacleCount = CPhysicsEngine::ms_pObstacles.size();
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalInfoBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_pMesh->m_nVertexBufferID, 1, sizeof(CPhysicalMesh::SPhysicalVertex), 0, m_pMesh->m_nVertexCount);
	//
	//CDeviceManager::BindStorageBuffer(m_nLocalInfoBufferID, 2, sizeof(SNeighbourInfo), 0, m_pMesh->m_nVertexCount * m_nMaxNeighbourCount);
	//
	//CDeviceManager::BindStorageBuffer(m_nNeighbourCountBufferID, 3, sizeof(unsigned int), 0, m_pMesh->m_nVertexCount);
	//
	//CDeviceManager::BindStorageBuffer(CPhysicsEngine::ms_nObstacleVertexBufferID, 4, sizeof(CObstacle::SGPUObstacle), 0, PHYSICS_MAX_OBSTACLES);
	//
	//CDeviceManager::SetUniform(nProgramID, "dt", dt * 1e-3f);
	//
	//CDeviceManager::SetUniform(nProgramID, "ObstacleCount", (unsigned int)nObstacleCount);
	//
	//CDeviceManager::Dispatch((m_pMesh->m_nVertexCount + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrier();
	//
	//SGlobalInfo* pMapped = (SGlobalInfo*)CDeviceManager::MapBuffer(m_nGlobalInfoBufferID, CDeviceManager::e_Write);
	//memcpy(pMapped->m_MeanVelocity, pMapped->m_NewMeanVelocity, 3 * sizeof(float));
	//memset(pMapped->m_NewMeanVelocity, 0, 3 * sizeof(float));
	//CDeviceManager::UnmapBuffer(m_nGlobalInfoBufferID);
	//
	//CDeviceManager::SetMemoryBarrier();
}


void CSoftbody::ComputeMeanValuesOnGPU()
{

	//GLuint nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Softbody_ComputeMeanValues];
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalInfoBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_pMesh->m_nVertexBufferID, 1, sizeof(CPhysicalMesh::SPhysicalVertex), 0, m_pMesh->m_nVertexCount);
	//
	//CDeviceManager::Dispatch((m_pMesh->m_nVertexCount + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}


void CSoftbody::SkinPacket()
{
	//if (m_bRunOnGPU)
	//{
	//	GLuint nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Softbody_SkinMesh];
	//
	//	CShader::BindProgram(nProgramID);
	//
	//	CDeviceManager::BindStorageBuffer(m_nGlobalInfoBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//	CDeviceManager::BindStorageBuffer(m_pMesh->m_nVertexBufferID, 1, sizeof(CPhysicalMesh::SPhysicalVertex), 0, m_pMesh->m_nVertexCount);
	//
	//	CDeviceManager::BindStorageBuffer(m_pMesh->m_nIndexBufferID, 2, sizeof(unsigned int), 0, 3 * m_pMesh->m_nTriangleCount);
	//
	//	int nMeshes = m_pMesh->m_pSkinner->GetMeshCount();
	//
	//	for (int i = 0; i < nMeshes; i++)
	//	{
	//		CDeviceManager::SetUniform(nProgramID, "NumVertexToSkin", m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		CDeviceManager::BindStorageBuffer(m_pMesh->m_pSkinner->GetSkinningBufferID(i), 3, sizeof(CSkinner::SSkinningData), 0, m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		CDeviceManager::BindStorageBuffer(m_pMesh->m_pSkinner->GetSkinnedVertexBufferID(i), 4, m_pMesh->m_pSkinner->GetStride(i) * sizeof(float), 0, m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		//CDeviceManager::BindStorageBuffer(m_pMesh->m_pSkinner->GetSkinnedNormalBufferID(i), 5, 3 * sizeof(float), 0, m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		//CDeviceManager::BindStorageBuffer(m_pMesh->m_pSkinner->GetSkinnedTangentBufferID(i), 6, 3 * sizeof(float), 0, m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		//CDeviceManager::BindStorageBuffer(m_pMesh->m_pSkinner->GetSkinnedBitangentBufferID(i), 7, 3 * sizeof(float), 0, m_pMesh->m_pSkinner->GetSkinnedVertexCount(i));
	//
	//		CDeviceManager::Dispatch((m_pMesh->m_pSkinner->GetSkinnedVertexCount(i) + 127) / 128, 1, 1);
	//
	//		CDeviceManager::SetMemoryBarrierOnBufferAccess();
	//	}
	//}
	//
	//else
	//{
	//	/*CPhysicalMesh::SPhysicalVertex* pVertexData = (CPhysicalMesh::SPhysicalVertex*)m_pMesh->m_pVertexBuffer;
	//	float* pMappedVertex = (float*)CDeviceManager::MapBuffer(m_pMesh->m_pSkinner->GetSkinnedVertexBufferID(i), CDeviceManager::e_Write);
	//	float* pMappedNormals = (float*)CDeviceManager::MapBuffer(m_pMesh->m_pSkinner->GetSkinnedNormalBufferID(i), CDeviceManager::e_Write);
	//
	//	for (int i = 0; i < m_pMesh->m_nVertexCount; i++)
	//	{
	//		Copy(&pMappedVertex[3 * i], pVertexData[i].m_Position);
	//		Copy(&pMappedNormals[3 * i], pVertexData[i].m_Normal);
	//	}
	//
	//	CDeviceManager::UnmapBuffer(m_nVertexBufferToSkinID);
	//	CDeviceManager::UnmapBuffer(m_nNormalBufferToSkinID);
	//
	//	CDeviceManager::SetMemoryBarrier();*/
	//}
}


void CSoftbody::ResetVelocitiesOnGPU() const
{
	//GLuint nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Softbody_ResetVelocities];
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalInfoBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_pMesh->m_nVertexBufferID, 1, sizeof(CPhysicalMesh::SPhysicalVertex), 0, m_pMesh->m_nVertexCount);
	//
	//CDeviceManager::Dispatch((m_pMesh->m_nVertexCount + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}


void CSoftbody::WriteGlobalInfos(SGlobalInfo* pInfo) const
{
	//CDeviceManager::SetMemoryBarrier();
	//
	//void* pMapped = CDeviceManager::MapBuffer(m_nGlobalInfoBufferID, CDeviceManager::e_Write);
	//memcpy(pMapped, pInfo, sizeof(SGlobalInfo));
	//CDeviceManager::UnmapBuffer(m_nGlobalInfoBufferID);
	//
	//CDeviceManager::SetMemoryBarrier();
}

void CSoftbody::ReadGlobalInfos()
{
	//CDeviceManager::SetMemoryBarrier();
	//
	//void* pMapped = CDeviceManager::MapBuffer(m_nGlobalInfoBufferID, CDeviceManager::e_Write);
	//memcpy(&m_GlobalInfos, pMapped, sizeof(SGlobalInfo));
	//CDeviceManager::UnmapBuffer(m_nGlobalInfoBufferID);
	//
	//CDeviceManager::SetMemoryBarrier();
}
