#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Engine/Device/DeviceManager.h"


ADJUSTABLE("Internal Energy", float, gs_fInternalEnergy, 1000.f, 50.f, 1000.f, "Physics/Softbodies")
ADJUSTABLE("Damping", float, gs_fDamping, 16.f, 0.f, 30.f,		"Physics/Softbodies")
ADJUSTABLE("Friction", float, gs_fFriction, 0.45f, 0.f, 1.f,	"Physics/Walls")
ADJUSTABLE("Relative Speed Limit", float, gs_fRelativeSpeed, 0.f, 0.f, 20.f, "Gameplay/Speed Limits")
ADJUSTABLE("Absolute Speed Limit", float, gs_fAbsoluteSpeed, 10.f, 0.f, 20.f, "Gameplay/Speed Limits")


CSoftbody::CSoftbody(CPhysicalMesh* pMesh, float fMass, float fSpring, float fDamping, bool bVolumic, bool bGPUAccelerated)
{
	m_bVolumic = bVolumic;

	m_pMesh = pMesh;
	m_fSpring = fSpring;
	m_fDamping = fDamping;
	m_fMass = fMass;

	m_bEnabled = true;

	m_fCenterOfMass = pMesh->m_Center;
	m_Velocity = 0.f;
	m_ExternalForce = 0.f;
	m_bIsInCollision = false;
	m_fLastCollisionTime = 0.f;
	m_fInternalEnergy = 1.f;
	m_LastFreeVelocity = 0.f;
	m_fLastCenterOfMass = pMesh->m_Center;
	m_fReferenceHeight = 0.f;
	m_ReferenceVelocity = 0.f;
	m_fMaxVerticalSpeed = 0.f;
	m_pForces.empty();
	m_nForcesCount = 0;
	m_fLeftOverTime = 0.f;

	m_AttractorPosition = 0.f;
	m_fAttractorStrength = 0.f;

	m_bIsInCollision = false;
	m_bFrozen = false;

	m_bShouldResetVelocities = false;

	int i, j;

	m_pRestLength = new float*[pMesh->m_nVertexCount];
	TVector Edge;
	int index;
	m_nMaxNeighbourCount = 0;

	for (i = 0; i < pMesh->m_nVertexCount; i++)
	{
		m_pRestLength[i] = new float[pMesh->m_pNeighbourCount[i]];

		if ((int)pMesh->m_pNeighbourCount[i] > m_nMaxNeighbourCount)
			m_nMaxNeighbourCount = pMesh->m_pNeighbourCount[i];

		for (j = 0; j < (int)pMesh->m_pNeighbourCount[i]; j++)
		{
			index = pMesh->m_pNeighbourIndex[i][j];
			Subi(Edge, &pMesh->m_pVertexBuffer[i * pMesh->m_nStride], &pMesh->m_pVertexBuffer[index * pMesh->m_nStride]);
			m_pRestLength[i][j] = 0.8f * Length(Edge);
		}
	}

	m_bLoaded = false;
	m_fRestVolume = 4.f * 3.141592f * powf(pMesh->m_fBoundingSphereRadius, 3.f) / 3.f;
	m_fVolume = m_fRestVolume;

	m_bRunOnGPU = bGPUAccelerated;

	if (bGPUAccelerated)
	{
		BuildBuffers();
		m_pMesh->Load();
	}
}


CSoftbody::~CSoftbody()
{
	if (m_pRestLength != NULL)
	{
		for (int i = 0; i < SOFTBODY_MAX_MESH_NEIGHBOURS; i++)
			delete[] m_pRestLength[i];

		delete[] m_pRestLength;

		m_pRestLength = NULL;
	}

	//CDeviceManager::DeleteStorageBuffer(m_nGlobalInfoBufferID);
	//CDeviceManager::DeleteStorageBuffer(m_nNeighbourCountBufferID);
	//CDeviceManager::DeleteStorageBuffer(m_nLocalInfoBufferID);

	m_pMesh->Unload();
}


void CSoftbody::SetPosition(float3& Pos)
{
	m_fCenterOfMass = Pos;
	m_pMesh->SetCenter(Pos);

	if (m_bRunOnGPU)
	{
		/*glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_pMesh->m_nVertexBufferID);
		void* pData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
		memcpy(pData, m_pMesh->m_pVertexBuffer, m_pMesh->m_nVertexCount * sizeof(CPhysicalMesh::SPhysicalVertex));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);*/
	}
}


void CSoftbody::Update(float dt)
{
	bool bBackupCollision = m_bIsInCollision;

	TVector Zero = { 0.f, 0.f, 0.f };

	if (m_bRunOnGPU)
	{
		Copy(m_GlobalInfos.m_ExternalForces, m_ExternalForce.v());
		Copy(m_GlobalInfos.m_AttractorPosition, m_AttractorPosition.v());
		Copy(m_GlobalInfos.m_MeanPosition, Zero);
		Copy(m_GlobalInfos.m_MeanVelocity, Zero);
		m_GlobalInfos.m_fSpring = m_fSpring;
		m_GlobalInfos.m_fDamping = m_fDamping;
		m_GlobalInfos.m_fFriction = gs_fFriction;
		m_GlobalInfos.m_fVolume = 0.f;
		m_GlobalInfos.m_fInternalEnergy = gs_fInternalEnergy;
		m_GlobalInfos.m_bIsColliding = 0;
		m_GlobalInfos.m_uMutex = 0;
		m_GlobalInfos.m_fRefSpeed = sqrtf(m_fReferenceSpeed * m_fReferenceSpeed + 9.81f * (m_fReferenceHeight - m_fCenterOfMass.z));
		m_GlobalInfos.m_fMaxAbsoluteSpeed = gs_fAbsoluteSpeed;
		m_GlobalInfos.m_fMaxRelativeSpeed = gs_fRelativeSpeed;
		m_GlobalInfos.m_fAttractorStrength = m_fAttractorStrength;
		WriteGlobalInfos(&m_GlobalInfos);
	}

	if (m_bShouldResetVelocities)
	{
		ResetVelocitiesOnGPU();
		m_bShouldResetVelocities = false;
	}

	/*if (CEngine::GetEngineTime() - m_fLastCollisionTime > 0.05)
		m_bRecordVelocity = true;
	else
		m_bRecordVelocity = false;*/


	ComputeMeanValues();
	ComputeVolume();
	Run(dt);


	if (m_bRunOnGPU)
	{
		ReadGlobalInfos();

		Copy(m_fCenterOfMass.v(), m_GlobalInfos.m_MeanPosition);
		Copy(m_Velocity.v(), m_GlobalInfos.m_MeanVelocity);
		m_fVolume = m_GlobalInfos.m_fVolume;
		//m_fRestVolume = m_GlobalInfos.m_fRestVolume;

		/*if (!m_GlobalInfos.m_bIsColliding && m_bRecordVelocity)
		{
			//m_LastFreeVelocity = m_Velocity;
			//printf("%g %g %g\n", m_LastFreeVelocity.x, m_LastFreeVelocity.y, m_LastFreeVelocity.z);
			m_fLastCollisionTime = CEngine::GetEngineTime();
			m_bIsInCollision = false;
		}
		else*/ if (m_GlobalInfos.m_bIsColliding)
		{
			m_bIsInCollision = true;
		}
	}
}


void CSoftbody::ComputeVolume()
{
	if (m_bRunOnGPU)
		ComputeVolumeOnGPU();
	else
		ComputeVolumeOnCPU();
}


void CSoftbody::ComputeMeanValues()
{
	if (m_bRunOnGPU)
		ComputeMeanValuesOnGPU();
	else
		ComputeMeanValuesOnCPU();
}


void CSoftbody::Run(float dt)
{
	if (m_bRunOnGPU)
		RunOnGPU(dt);
	else
		RunOnCPU(dt * 1e-3f);
}


void CSoftbody::ComputeMeanValuesOnCPU()
{
	int i = 0;
	int nVertexCount = m_pMesh->m_nVertexCount;
	CPhysicalMesh::SPhysicalVertex* pVertexBuffer = (CPhysicalMesh::SPhysicalVertex*)(m_pMesh->m_pVertexBuffer);
	memset(m_fCenterOfMass.v(), 0, 3 * sizeof(float));
	memset(m_Velocity.v(), 0, 3 * sizeof(float));

	for (i = 0; i < nVertexCount; i++)
	{
		Addi(m_fCenterOfMass.v(), pVertexBuffer[i].m_Position);
		Addi(m_Velocity.v(), pVertexBuffer[i].m_Velocity);
	}

	Scale(m_fCenterOfMass.v(), 1.f / nVertexCount);
	Scale(m_Velocity.v(), 1.f / nVertexCount);

	if (fabs(m_fCenterOfMass.y) > 1e-3f || fabs(m_Velocity.y) > 1e-3f)
	{
		for (i = 0; i < nVertexCount; i++)
		{
			pVertexBuffer[i].m_Position[1] -= m_fCenterOfMass.y;
			pVertexBuffer[i].m_Velocity[1] -= m_Velocity.y;
		}
	}
}


void CSoftbody::ComputeVolumeOnCPU()
{
	int i = 0, j = 0;
	int nTriangleCount = m_pMesh->m_nTriangleCount;
	CPhysicalMesh::SPhysicalVertex* pVertexBuffer = (CPhysicalMesh::SPhysicalVertex*)m_pMesh->m_pVertexBuffer;
	unsigned int* pIndexBuffer = m_pMesh->m_pIndexBuffer;
	int nStride = m_pMesh->m_nStride;
	float fVolume = 0.f;
	TVector vertices[3];
	TVector Cross;
	TVector Radius;
	TVector Edges[2];
	TVector Center;

	Copy(Center, m_fCenterOfMass.v());

	for (i = 0; i < nTriangleCount; i++)
	{
		for (j = 0; j < 3; j++)
			Copy(vertices[j], pVertexBuffer[pIndexBuffer[3 * i + j]].m_Position);

		Subi(Edges[0], vertices[1], vertices[0]);
		Subi(Edges[1], vertices[2], vertices[0]);
		Subi(Radius, vertices[0], Center);
		CrossProduct(Cross, Edges[0], Edges[1]);

		fVolume += fabs(DotProduct(Radius, Cross));
	}

	fVolume /= 6.f;

	m_fVolume = fVolume;
}


void CSoftbody::RunOnCPU(float dt)
{
	/*int i = 0;
	int index;
	int Counter = 0;
	unsigned int j;
	int nVertexCount = m_pMesh->m_nVertexCount;
	unsigned int nNeighbourCount;
	CPhysicalMesh::SPhysicalVertex* pVertexBuffer = (CPhysicalMesh::SPhysicalVertex*)(m_pMesh->m_pVertexBuffer);
	unsigned int** pNeighbours = m_pMesh->m_pNeighbourIndex;
	CPhysicsEngine::SObstacleList* pObstacles = CPhysicsEngine::GetInstance()->m_pObstacles;
	CPhysicsEngine::SObstacleList* pCurrentObstacle;
	int nObstacleCount = CPhysicsEngine::GetInstance()->m_nObstaclesCount;
	CObstacle::SWall CurrentWall;

	TVector Force;
	TVector Normal;
	TVector NormalTmp;
	TVector CurrentPos;
	TVector Velocity;
	TVector NeighbourDir;
	TVector LastNeighbour;
	TVector Fluctuations;

	float fRestLength;
	float fLength;
	float fFluctuationAmpl;
	float h;
	float dx;
	float alpha;
	float beta;

	bool bShouldUpdate;
	bool bCollision = false;

	for (i = 0; i < nVertexCount; i++)
	{
		nNeighbourCount = m_pMesh->m_pNeighbourCount[i];
		Copy(Force, m_ExternalForce.v);
		Scale(Force, m_fMass);
		Copy(CurrentPos, pVertexBuffer[i].m_Position);
		Copy(Velocity, pVertexBuffer[i].m_Velocity);
		memset(Normal, 0, 3 * sizeof(float));

		for (j = 0; j < nNeighbourCount; j++)
		{
			fRestLength = m_pRestLength[i][j];
			Subi(NeighbourDir, pVertexBuffer[pNeighbours[i][j]].m_Position, CurrentPos);

			fLength = Length(NeighbourDir);

			if (j > 0)
			{
				CrossProduct(NormalTmp, LastNeighbour, NeighbourDir);
				Normalize(NormalTmp);
				Addi(Normal, NormalTmp);
			}

			Copy(LastNeighbour, NeighbourDir);
			Scale(NeighbourDir, 0.5f * m_fSpring * (fLength - fRestLength) / fLength);
			Addi(Force, NeighbourDir);
		}

		Normalize(Normal);

		Scale(NormalTmp, Normal, m_fInternalEnergy * powf(m_fVolume, 2.f / 3.f) * (1.f - m_fRestVolume / m_fVolume) / nVertexCount);
		Subi(Force, NormalTmp);

		Subi(NormalTmp, Velocity, m_Velocity.v);
		Scale(NormalTmp, m_fMass * m_fDamping);
		Subi(Force, NormalTmp);

		for (j = 0; j < 3; j++)
			Velocity[j] += 0.5f * dt * (Force[j] + pVertexBuffer[i].m_Forces[j]) / m_fMass;

		Subi(Fluctuations, Velocity, m_Velocity.v);

		fFluctuationAmpl = Length(Fluctuations);
		if (fFluctuationAmpl > 1.5f)
			for (j = 0; j < 3; j++)
				Velocity[j] = 1.5f * Fluctuations[j] / fFluctuationAmpl + m_Velocity.v[j];

		Copy(pVertexBuffer[i].m_Normal, Normal);
		Copy(pVertexBuffer[i].m_Velocity, Velocity);
		Copy(pVertexBuffer[i].m_Forces, Force);
	}

	for (i = 0; i < nVertexCount; i++)
	{
		Copy(CurrentPos, pVertexBuffer[i].m_Position);

		for (j = 0; j < 3; j++)
			CurrentPos[j] += pVertexBuffer[i].m_Velocity[j] * dt + 0.5f * pVertexBuffer[i].m_Forces[j] * dt * dt / m_fMass;

		Counter = 0;
		index = 0;
		pCurrentObstacle = pObstacles;
		bShouldUpdate = false;

		while (Counter++ < 1)
		{
			while (pCurrentObstacle != NULL)
			{
				index = pCurrentObstacle->m_pObstacle->CheckCollision(CurrentPos, pVertexBuffer[i].m_Position);
				if (index >= 0)
				{
					memcpy(&CurrentWall, pCurrentObstacle->m_pObstacle->GetWall(index), sizeof(CObstacle::SWall));

					h = DotProduct(CurrentWall.m_Normal, CurrentWall.m_Vertex[0]) - DotProduct(CurrentWall.m_Normal, CurrentPos);
					Subi(Velocity, CurrentPos, pVertexBuffer[i].m_Position);
					dx = Length(Velocity);
					alpha = DotProduct(pVertexBuffer[i].m_Velocity, CurrentWall.m_Normal);
					beta = -DotProduct(Velocity, CurrentWall.m_Normal);
					Copy(Velocity, pVertexBuffer[i].m_Velocity);

					beta = dx / beta;

					for (j = 0; j < 3; j++)
					{
						CurrentPos[j] += (h + beta * gs_fFriction * h + 1e-5f) * CurrentWall.m_Normal[j];
						Velocity[j] = gs_fFriction * (Velocity[j] - 2.f * alpha * CurrentWall.m_Normal[j]);
					}

					bShouldUpdate = true;
				}

				pCurrentObstacle = pCurrentObstacle->m_pNext;
			}

			if (index < 0)
				break;
		}

		if (bShouldUpdate)
		{
			for (j = 0; j < 3; j++)
				pVertexBuffer[i].m_Forces[j] = (Velocity[j] - pVertexBuffer[i].m_Velocity[j]) / dt;

			Copy(pVertexBuffer[i].m_Velocity, Velocity);
			bCollision = true;
		}

		Copy(pVertexBuffer[i].m_Position, CurrentPos);
	}

	if (!bCollision && m_bRecordVelocity)
	{
		m_LastFreeVelocity = m_Velocity;
		m_fLastCollisionTime = CEngine::GetEngineTime();
		m_bIsInCollision = false;
	}
	else if (bCollision)
		m_bIsInCollision = true;*/
}