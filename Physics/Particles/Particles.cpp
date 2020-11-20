#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Particles/Particles.h"
#include "Engine/Editor/Adjustables/Adjustables.h"


ADJUSTABLE("Speed", float, gs_fSpeed, 1.f, 0.f, 20.f, "Physics/Particles")


void CParticles::InitParticleEngine()
{

}

void CParticles::UnloadParticleEngine()
{

}


CParticles::CParticles(EParticleType eType, unsigned int nMaxCount)
{

	m_nCount = nMaxCount;
	m_nNbActive = 0;
	m_fDispersion = 0.f;
	m_fFlowRate = 0.f;
	m_fSpeed = 0.f;
	m_fLeftOverParticles = 0.f;
	m_nFirstIndex = 0;

	m_SourcePosition = 0.f;
	m_SourceVelocity = 0.f;

	m_bEnabled = false;
	m_bIsIndexBufferReady = false;

	m_eType = eType;

	//m_nVertexBufferID = CDeviceManager::CreateVertexBuffer(NULL, nMaxCount * sizeof(SParticleVertex), true);

	m_eSourceType = e_None;

	SGlobalInfo GlobalInfo = 
	{
		(unsigned int)m_nNbActive,
		{ 0.f, 0.f, 0.f },
		{ 0.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f },
		m_fFlowRate,
		m_fSpeed,
		m_fDispersion
	};

	memcpy(&m_GlobalInfos, &GlobalInfo, sizeof(SGlobalInfo));

	//m_nGlobalBufferID = CDeviceManager::CreateStorageBuffer(&m_GlobalInfos, sizeof(SGlobalInfo));
}


CParticles::~CParticles()
{

}


void CParticles::BindPacket(Packet* pPacket)
{
	//if (m_bIsIndexBufferReady)
	//	CDeviceManager::DeleteStorageBuffer(m_nConvertedIndexBufferID);
	//
	//m_nParentVertexBufferID = pPacket->m_pStream[0]->GetBufferId();
	//m_nParentNormalBufferID = pPacket->m_pStream[1]->GetBufferId();
	//m_nParentIndexBufferID = pPacket->m_nIndexBufferID;
	//
	//m_nMeshTriangleCount = pPacket->m_nTriangleCount;
	//m_nMeshVertexCount = pPacket->m_pStream[0]->GetElementsCount();
	//
	//unsigned int* pIndexBuffer = new unsigned int[3 * m_nMeshTriangleCount];
	//for (unsigned int i = 0; i < 3 * m_nMeshTriangleCount; i++)
	//	pIndexBuffer[i] = (unsigned int)pPacket->m_pIndexBuffer[i];
	//
	//m_nConvertedIndexBufferID = CDeviceManager::CreateStorageBuffer(pIndexBuffer, 3 * m_nMeshTriangleCount * sizeof(unsigned int));
	//
	//delete[] pIndexBuffer;
	//
	//m_bIsIndexBufferReady = true;
}


void CParticles::SetStaticSource(float3& Position)
{
	m_SourcePosition = Position;
	m_SourceVelocity = 0.f;
}


void CParticles::SetRandomSourceOnMesh()
{
	m_nTriangleIndex = Randi(0, m_nMeshTriangleCount);

	m_fBarycentricU = Randf(0.f, 1.f);
	m_fBarycentricV = Randf(0.f, 1.f - m_fBarycentricU);
}


void CParticles::Update(float dt)
{
	float fNewParticles = m_fFlowRate * dt;
	int nNewParticles = 0;

	if (m_fLeftOverParticles > 1.f)
		nNewParticles = (int)m_fLeftOverParticles;

	nNewParticles += (int)fNewParticles;
	m_fLeftOverParticles += fNewParticles - nNewParticles;

	m_nNbActive += nNewParticles;

	WriteGlobalBuffer();

	UpdateSource();

	if (m_nNbActive > 0)
	{
		AddNewParticles(nNewParticles, dt);

		Process(dt);
	}

	ReadGlobalBuffer();
}



void CParticles::WriteGlobalBuffer()
{

	//m_GlobalInfos.m_nParticleCount = m_nNbActive > m_nCount ? m_nCount : m_nNbActive;
	//m_GlobalInfos.m_ExternalForces[0] = 0.f;
	//m_GlobalInfos.m_ExternalForces[1] = 0.f;
	//m_GlobalInfos.m_ExternalForces[2] = -9.81f;
	//m_GlobalInfos.m_fFlowRate = m_fFlowRate;
	//m_GlobalInfos.m_fSpeed = gs_fSpeed;
	//m_GlobalInfos.m_fDispersion = m_fDispersion;
	//
	//SGlobalInfo* pGlobalBuffer = (SGlobalInfo*)CDeviceManager::MapBuffer(m_nGlobalBufferID, CDeviceManager::e_Write);
	//memcpy(pGlobalBuffer, &m_GlobalInfos, sizeof(SGlobalInfo));
	//CDeviceManager::UnmapBuffer(m_nGlobalBufferID);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferMapping();
}


void CParticles::ReadGlobalBuffer()
{
	//SGlobalInfo* pGlobalBuffer = (SGlobalInfo*)CDeviceManager::MapBuffer(m_nGlobalBufferID, CDeviceManager::e_Write);
	//memcpy(&m_GlobalInfos, pGlobalBuffer, sizeof(SGlobalInfo));
	//CDeviceManager::UnmapBuffer(m_nGlobalBufferID);
	//
	//Copy(&m_SourcePosition.x, m_GlobalInfos.m_SourcePos);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferMapping();
}


void CParticles::AddNewParticles(int nCount, float dt)
{
	//ProgramHandle nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Particles_New];
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_nVertexBufferID, 1, sizeof(SParticleVertex), 0, m_nNbActive);
	//
	//CDeviceManager::SetUniform(nProgramID, "FirstIndex", (unsigned int)m_nFirstIndex);
	//CDeviceManager::SetUniform(nProgramID, "NewParticleCount", (unsigned int)nCount);
	//
	//CDeviceManager::Dispatch((nCount + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
	//
	//if (m_nNbActive > m_nCount)
	//	m_nNbActive = m_nCount;
	//
	//m_nFirstIndex += nCount;
	//if (m_nFirstIndex > m_nCount)
	//	m_nFirstIndex -= m_nCount;
}



void CParticles::UpdateSource()
{
	//ProgramHandle nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Particles_UpdateMovingSource];
	//
	//CShader::BindProgram(nProgramID);
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_nParentVertexBufferID, 1, 3 * sizeof(float), 0, m_nMeshVertexCount);
	//
	//CDeviceManager::BindStorageBuffer(m_nParentNormalBufferID, 2, 3 * sizeof(float), 0, m_nMeshVertexCount);
	//
	//CDeviceManager::BindStorageBuffer(m_nConvertedIndexBufferID, 3, 3 * sizeof(unsigned int), 0, m_nMeshTriangleCount);
	//
	//CDeviceManager::SetUniform(nProgramID, "m_nTriangleIndex", (unsigned int)m_nTriangleIndex);
	//CDeviceManager::SetUniform(nProgramID, "m_fUCoord", m_fBarycentricU);
	//CDeviceManager::SetUniform(nProgramID, "m_fVCoord", m_fBarycentricV);
	//
	//CDeviceManager::Dispatch(1, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}


void CParticles::Process(float dt)
{
	//ProgramHandle nProgramID = CPhysicsEngine::ms_ProgramID[CPhysicsEngine::e_Particles_Main];
	//
	//CShader::BindProgram(nProgramID);
	//
	//size_t nObstaclesCount = CPhysicsEngine::ms_pObstacles.size();
	//
	//CDeviceManager::BindStorageBuffer(m_nGlobalBufferID, 0, sizeof(SGlobalInfo), 0, 1);
	//
	//CDeviceManager::BindStorageBuffer(m_nVertexBufferID, 1, sizeof(SParticleVertex), 0, m_nNbActive);
	//
	//CDeviceManager::BindStorageBuffer(CPhysicsEngine::ms_nObstacleVertexBufferID, 2, sizeof(CObstacle::SGPUObstacle), 0, PHYSICS_MAX_OBSTACLES);
	//
	//CDeviceManager::SetUniform(nProgramID, "dt", dt);
	//
	//CDeviceManager::SetUniform(nProgramID, "ObstacleCount", (unsigned int)nObstaclesCount);
	//
	//CDeviceManager::Dispatch((m_nNbActive + 127) / 128, 1, 1);
	//
	//CDeviceManager::SetMemoryBarrierOnBufferAccess();
}


void CParticles::Draw()
{
	CParticleRenderer::AddSystem(m_nVertexBufferID, m_nNbActive);
}
