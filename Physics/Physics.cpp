#include "Engine/Engine.h"
#include "Physics.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include <string.h>



std::vector<CSoftbody*>		CPhysicsEngine::ms_pSoftbodies;
std::vector<CRigidbody*>	CPhysicsEngine::ms_pRigidbodies;
std::vector<CObstacle*>		CPhysicsEngine::ms_pObstacles;


bool	CPhysicsEngine::ms_bOncePerRenderedFrame = false;
bool	CPhysicsEngine::ms_bShouldUpdateObstacles = true;


unsigned int	CPhysicsEngine::ms_nObstacleVertexBufferID	= INVALIDHANDLE;
int				CPhysicsEngine::ms_nTotalWallCount			= 0;
ProgramHandle	CPhysicsEngine::ms_ProgramID[32]			= { INVALID_PROGRAM_HANDLE };

CObstacle::SGPUObstacle CPhysicsEngine::ms_pObstacleData[PHYSICS_MAX_OBSTACLES];

bool	CPhysicsEngine::ms_bIsInit = false;


void CPhysicsEngine::Init()
{
	ms_pSoftbodies.clear();
	ms_pRigidbodies.clear();
	ms_pObstacles.clear();

	ms_bShouldUpdateObstacles = true;

	/*ms_ProgramID[e_Softbody_Main]				= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Main", true);
	ms_ProgramID[e_Softbody_ComputeNormals]		= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Normals", true);
	ms_ProgramID[e_Softbody_ComputeMeanValues]	= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_MeanValues", true);
	ms_ProgramID[e_Softbody_ComputeVolume]		= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Volume", true);
	ms_ProgramID[e_Softbody_SkinMesh]			= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_SkinMesh", true);
	ms_ProgramID[e_Softbody_ResetVelocities]		= CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_ResetVelocities", true);
	
	ms_ProgramID[e_Particles_Main]				= CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_Main", true);
	ms_ProgramID[e_Particles_New]				= CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_New", true);
	ms_ProgramID[e_Particles_UpdateMovingSource] = CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_UpdateSource", true);*/


	size_t nWallBufferSize = PHYSICS_MAX_OBSTACLES * sizeof(CObstacle::SGPUObstacle);

	//ms_nObstacleVertexBufferID = CDeviceManager::CreateStorageBuffer(NULL, nWallBufferSize);

	ms_bIsInit = true;
}


void CPhysicsEngine::ReloadShaders()
{
	/*CShader::DeleteProgram(ms_ProgramID[e_Softbody_Main]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeNormals]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_Reflect]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeMeanValues]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeVolume]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_SkinMesh]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ResetVelocities]);

	CShader::DeleteProgram(ms_ProgramID[e_Particles_Main]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_New]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_AddMovingSource]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_UpdateMovingSource]);

	ms_ProgramID[e_Softbody_Main] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Main");
	ms_ProgramID[e_Softbody_ComputeNormals] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Normals");
	ms_ProgramID[e_Softbody_ComputeMeanValues] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_MeanValues");
	ms_ProgramID[e_Softbody_ComputeVolume] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_Volume");
	ms_ProgramID[e_Softbody_SkinMesh] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_SkinMesh");
	ms_ProgramID[e_Softbody_ResetVelocities] = CShader::LoadProgram(SHADER_PATH("Physics"), "Softbody_ResetVelocities");

	ms_ProgramID[e_Particles_Main] = CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_Main");
	ms_ProgramID[e_Particles_New] = CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_New");
	ms_ProgramID[e_Particles_AddMovingSource] = CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_AddSource");
	ms_ProgramID[e_Particles_UpdateMovingSource] = CShader::LoadProgram(SHADER_PATH("Physics"), "Particles_UpdateSource");*/

	ms_bIsInit = false;
}


void CPhysicsEngine::Terminate()
{
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_Main]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeNormals]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_Reflect]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeMeanValues]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ComputeVolume]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_SkinMesh]);
	CShader::DeleteProgram(ms_ProgramID[e_Softbody_ResetVelocities]);

	CShader::DeleteProgram(ms_ProgramID[e_Particles_Main]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_New]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_AddMovingSource]);
	CShader::DeleteProgram(ms_ProgramID[e_Particles_UpdateMovingSource]);


	ms_pSoftbodies.clear();
	ms_pRigidbodies.clear();
	ms_pObstacles.clear();

	//CDeviceManager::DeleteStorageBuffer(ms_nObstacleVertexBufferID);
}



bool CPhysicsEngine::CheckCollision(SSphere& sphere)
{
	CObstacle* pObstacle;
	float3 Center, MTV;
	float fDepth, fMinDepth;

	SOrientedBox Obstacle;

	Obstacle.m_Basis[0] = float3(1.f, 0.f, 0.f);
	Obstacle.m_Basis[1] = float3(0.f, 1.f, 0.f);
	Obstacle.m_Basis[2] = float3(0.f, 0.f, 1.f);

	std::vector<CObstacle*>::iterator it;

	for (it = ms_pObstacles.begin(); it < ms_pObstacles.end(); it++)
	{
		pObstacle = *it;

		if (pObstacle->m_bEnabled && pObstacle->m_bUseOnCPU)
		{
			Copy(&Center.x, pObstacle->m_Center);

			if ((Center - sphere.m_Position).length() < sphere.m_fRadius + pObstacle->m_fBoundingSphereRadius)
			{
				Copy(&Obstacle.m_Position.x, pObstacle->m_Center);
				Copy(Obstacle.m_Dim, pObstacle->m_AABB.v());

				if (Collisions::GetMinimumTranslationVector(Obstacle, sphere, MTV, &fDepth, &fMinDepth))
				{
					return true;
				}
			}
		}
	}

	return false;
}



void CPhysicsEngine::Run()
{
	UpdateObstacles();

	UpdateSoftbodies();

	UpdateRigidbodies();
	
	UpdateParticles();
}


void CPhysicsEngine::UpdateSoftbodies()
{
	static float fLeftOver = 0.f;
	float tframe = MIN(CEngine::GetFrameDuration(), PHYSICS_MAX_FRAME_COUNT * PHYSICS_SOFTBODY_TIMESTEP_MS);

	float stable_dt = PHYSICS_SOFTBODY_TIMESTEP_MS;
	float t = 0.f;

	if (tframe + fLeftOver < stable_dt)
		return;

	std::vector<CSoftbody*>::iterator it;
	CSoftbody* pSoftbody;

	for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
	{
		pSoftbody = *it;

		if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
		{
			pSoftbody->m_bIsInCollision = false;
			pSoftbody->SumForces();
		}
	}

	if (fLeftOver > stable_dt)
	{
		tframe += (int)(fLeftOver / stable_dt) * stable_dt;
		fLeftOver -= (int)(fLeftOver / stable_dt) * stable_dt;
	}

	ms_bOncePerRenderedFrame = true;

	do
	{
		if (t + stable_dt > tframe)
		{
			fLeftOver += tframe - t;
			break;
		}

		for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
		{
			pSoftbody = *it;

			if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
				pSoftbody->Update(stable_dt);
		}

		t += stable_dt;

		ms_bOncePerRenderedFrame = false;

	} while (t < tframe);


	for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
	{
		pSoftbody = *it;

		if (pSoftbody->m_bEnabled && !pSoftbody->IsColliding())
			pSoftbody->m_LastFreeVelocity = pSoftbody->m_Velocity;

		if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
			pSoftbody->SkinPacket();
	}
}


void CPhysicsEngine::UpdateRigidbodies()
{
	static float fLeftOver = 0.f;
	float tframe = MIN(CEngine::GetFrameDuration(), PHYSICS_MAX_FRAME_COUNT * PHYSICS_RIGIDBODY_TIMESTEP_MS);

	float stable_dt = PHYSICS_RIGIDBODY_TIMESTEP_MS;
	float t = 0.f;

	if (tframe + fLeftOver < stable_dt)
		return;

	if (fLeftOver > stable_dt)
	{
		tframe += (int)(fLeftOver / stable_dt) * stable_dt;
		fLeftOver -= (int)(fLeftOver / stable_dt) * stable_dt;
	}

	ms_bOncePerRenderedFrame = true;

	std::vector<CRigidbody*>::iterator it;
	CRigidbody* pRigidbody;

	do
	{
		if (t + stable_dt > tframe)
		{
			fLeftOver += tframe - t;
			break;
		}


		for (it = ms_pRigidbodies.begin(); it < ms_pRigidbodies.end(); it++)
		{
			pRigidbody = *it;

			if (pRigidbody->m_bEnabled && !pRigidbody->IsIdle())
				pRigidbody->Update(stable_dt * 1e-3f);
		}

		t += stable_dt;

		ms_bOncePerRenderedFrame = false;

	} while (t < tframe);

	CRigidbody::ms_nSphereCount = 0;
}


void CPhysicsEngine::UpdateParticles()
{
	
}


void CPhysicsEngine::UpdateObstacles()
{
	std::vector<CObstacle*>::iterator it;
	CObstacle* pObstacle;
	size_t nWallSize = 0;
	CObstacle::SGPUObstacle* pObstacles = ms_pObstacleData;
	int i = 0;

	if (ms_bShouldUpdateObstacles)
		ms_nTotalWallCount = 0;

	for (it = ms_pObstacles.begin(); it < ms_pObstacles.end(); it++)
	{
		pObstacle = *it;

		if (pObstacle->m_bEnabled && pObstacle->m_bUseOnGPU)
		{
			pObstacle->Update();
			if (ms_bShouldUpdateObstacles)
			{
				pObstacles[i].m_nWallCount = pObstacle->m_nWallCount;
				pObstacles[i].m_fRadius = pObstacle->m_fBoundingSphereRadius;
				Copy(pObstacles[i].m_Center, pObstacle->m_Center);
				memcpy(&(pObstacles[i].m_Walls), pObstacle->m_pWalls, pObstacle->m_nWallCount * sizeof(CObstacle::SWall));
				ms_nTotalWallCount += pObstacle->m_nWallCount;
			}
			i++;
		}
	}



	if (ms_bShouldUpdateObstacles)
	{
		/*void* pBuffer = glMapNamedBuffer(ms_nObstacleVertexBufferID, GL_WRITE_ONLY);
		memcpy(pBuffer, ms_pObstacleData, ms_pObstacles.size() * sizeof(CObstacle::SGPUObstacle));
		glUnmapNamedBuffer(ms_nObstacleVertexBufferID);
		ms_bShouldUpdateObstacles = false;*/
	}
}







