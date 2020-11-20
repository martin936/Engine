#ifndef PHYSICS_TYPES
#define PHYSICS_TYPES

#include <stdlib.h>
#include <math.h>

//#define PHYSICS_ENGINE_USE_CUDA


#define PHYSICS_SOFTBODY_TIMESTEP_MS	5.f
#define PHYSICS_RIGIDBODY_TIMESTEP_MS	5.f
#define PHYSICS_MAX_FRAME_COUNT			10

typedef enum {
	PHYSICS_BALL,
	PHYSICS_SOFTBODY,
	PHYSICS_STREAMS,
	PHYSICS_RIGIDBODY
}PhyEnum;


#define PHYSICS_WALL_VERTICES 1
#define PHYSICS_WALL_GAMEPLAY 2
#define PHYSICS_WALL_PARTICLES 4
#define PHYSICS_WALL_ALL 7

#define PHYSICS_MAX_OBSTACLES 128


#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "Engine/Maths/Maths.h"

#include "Engine/Physics/Forces/Forces.h"
#include "Engine/Physics/Rigidbodies/Rigidbodies.h"
#include "Engine/Physics/Softbodies/Softbodies.h"
#include "Engine/Physics/Obstacles/Obstacles.h"
#include "Engine/Physics/Particles/Particles.h"

#include "Engine/Renderer/Packets/Packet.h"
#include "Engine/Device/Shaders.h"


#define PHYSICS_SHADER_PATH "../../Engine/Physics/Shaders/"


class CPhysicsEngine
{
	friend class CSoftbody;
	friend class CRigidbody;
	friend class CParticles;
	friend class CObstacle;

public:

	static void Init();
	static void Terminate();

	inline static bool IsInit() { return ms_bIsInit; }

	static bool ms_bOncePerRenderedFrame;
	static bool ms_bShouldUpdateObstacles;

	static void ReloadShaders();

	inline static void Add(CSoftbody* pSoftbody)
	{
		ms_pSoftbodies.push_back(pSoftbody);
	}

	inline static void Add(CRigidbody* pRigidbody)
	{
		ms_pRigidbodies.push_back(pRigidbody);
	}

	inline static void Add(CObstacle* pObstacle)
	{
		ms_pObstacles.push_back(pObstacle);
	}

	inline static void Remove(CSoftbody* pSoftbody)
	{
		std::vector<CSoftbody*>::iterator it;

		for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
		{
			if (*it == pSoftbody)
			{
				ms_pSoftbodies.erase(it);
				break;
			}
		}
	}

	inline static void Remove(CRigidbody* pRigidbody)
	{
		std::vector<CRigidbody*>::iterator it;

		for (it = ms_pRigidbodies.begin(); it < ms_pRigidbodies.end(); it++)
		{
			if (*it == pRigidbody)
			{
				ms_pRigidbodies.erase(it);
				break;
			}
		}
	}

	inline static void Remove(CObstacle* pObstacle)
	{
		std::vector<CObstacle*>::iterator it;

		for (it = ms_pObstacles.begin(); it < ms_pObstacles.end(); it++)
		{
			if (*it == pObstacle)
			{
				ms_pObstacles.erase(it);
				break;
			}
		}
	}


	inline static void RemoveAll()
	{
		ms_pObstacles.clear();
		ms_pRigidbodies.clear();
		ms_pSoftbodies.clear();

		ms_nTotalWallCount = 0;
	}


	static void Run();

	static bool CheckCollision(SSphere& sphere);

	enum PhysicsProgramID
	{
		e_Softbody_Main,
		e_Softbody_ComputeNormals,
		e_Softbody_Reflect,
		e_Softbody_ComputeMeanValues,
		e_Softbody_ComputeVolume,
		e_Softbody_SkinMesh,
		e_Softbody_ResetVelocities,

		e_Particles_Main,
		e_Particles_New,
		e_Particles_AddMovingSource,
		e_Particles_UpdateMovingSource
	};


	static std::vector<CSoftbody*>	ms_pSoftbodies;
	static std::vector<CRigidbody*>	ms_pRigidbodies;
	static std::vector<CObstacle*>	ms_pObstacles;


private:

	static void UpdateObstacles();
	static void UpdateSoftbodies();
	static void UpdateRigidbodies();
	static void UpdateParticles();

	static unsigned int ms_nObstacleVertexBufferID;
	static int ms_nTotalWallCount;
	static CObstacle::SGPUObstacle ms_pObstacleData[PHYSICS_MAX_OBSTACLES];

	static ProgramHandle ms_ProgramID[32];

	static bool ms_bIsInit;
};


#endif
