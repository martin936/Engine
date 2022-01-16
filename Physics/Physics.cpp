#include "Engine/Engine.h"
#include "Physics.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/CommandListManager.h"

std::vector<CSoftbody*>		CPhysicsEngine::ms_pSoftbodies;
std::vector<CRigidbody*>	CPhysicsEngine::ms_pRigidbodies;
std::vector<CMesh*>			CPhysicsEngine::ms_pObstacles;


CTexture*		CPhysicsEngine::ms_pAccelerationStructure;
BufferId		CPhysicsEngine::ms_ParticlePool;
BufferId		CPhysicsEngine::ms_ParticleInitPool;
BufferId		CPhysicsEngine::ms_ContactConstraintBuffer;
BufferId		CPhysicsEngine::ms_IndirectDispatchBuffer;

BufferId		CPhysicsEngine::ms_LinkedListNodeBuffer;
BufferId		CPhysicsEngine::ms_ParticleIndexBuffer;

BufferId		CPhysicsEngine::ms_RigidbodyData[5];
BufferId		CPhysicsEngine::ms_RigidbodyBuffer;

float3			CPhysicsEngine::ms_GridCenter;
float3			CPhysicsEngine::ms_GridSize;

CSDF*			CPhysicsEngine::ms_pStaticCollisionSDF;

unsigned int	CPhysicsEngine::ms_nNumParticles	= 0;
bool			CPhysicsEngine::ms_bIsInit			= false;

float			CPhysicsEngine::ms_fParticleSize	= 0.02f;
unsigned int	CPhysicsEngine::ms_CurrentBufferId	= 0;

std::vector<CRigidbody*>	CPhysicsEngine::ms_pRigidbodiesToInsert[2];
std::vector<CRigidbody*>*	CPhysicsEngine::ms_pRigidbodiesInsertListToFill		= &CPhysicsEngine::ms_pRigidbodiesToInsert[0];
std::vector<CRigidbody*>*	CPhysicsEngine::ms_pRigidbodiesInsertListToFlush	= &CPhysicsEngine::ms_pRigidbodiesToInsert[1];



#define		MAX_NUM_PARTICLES			1000000
#define		MAX_NUM_CONSTRAINTS			100000
#define		MAX_NUM_RIGIDBODY_TO_INSERT 64

#define		NUM_GAUSS_SEIDEL_ITER		20


unsigned int g_PhysicsCommandList = 0;



struct SRigidbodyData
{
	float4		m_Position;
	float4		m_LinearMomentum;
	float4		m_AngularMomentum;
	float4		m_Rotation;

	float4		m_ShapeMatchingData[3];
};



void CPhysicsEngine::Init()
{
	ms_bIsInit = true;

	ms_pSoftbodies.clear();
	ms_pRigidbodies.clear();
	ms_pObstacles.clear();

	ms_ParticlePool				= CResourceManager::CreateRwBuffer(MAX_NUM_PARTICLES * sizeof(SPhysicsParticle));
	ms_ParticleInitPool			= CResourceManager::CreateRwBuffer(MAX_NUM_PARTICLES * sizeof(float4));
	ms_ContactConstraintBuffer	= CResourceManager::CreateRwBuffer(MAX_NUM_CONSTRAINTS * 2 * sizeof(float4));

	ms_LinkedListNodeBuffer		= CResourceManager::CreateRwBuffer(10 * 1024 * 1024);
	ms_ParticleIndexBuffer		= CResourceManager::CreateRwBuffer(5 * 1024 * 1024);

	for (int i = 0; i < 5; i++)
		ms_RigidbodyData[i]		= CResourceManager::CreateRwBuffer(8192 * 2 * sizeof(float4), true);

	ms_RigidbodyBuffer			= CResourceManager::CreateRwBuffer(8192 * sizeof(SRigidbodyData), true);

	ms_IndirectDispatchBuffer	= CResourceManager::CreateRwBuffer(sizeof(float4));

	ms_CurrentBufferId = 0;

	ms_pAccelerationStructure	= new CTexture(128, 128, 128, ETextureFormat::e_R32_UINT, eTextureStorage3D);

	CRigidbody::Init();

	if (!g_PhysicsCommandList)
		g_PhysicsCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);


	if (CRenderPass::BeginCompute("Insert Rigid Body Particles"))
	{
		CRenderPass::BindResourceToRead(0, ms_ParticleInitPool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
		CRenderPass::BindResourceToRead(1, ms_ParticlePool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
		CRenderPass::SetNumRWTextures(2, 1);

		CRenderPass::BindResourceToWrite(3, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

		CRenderPass::BindProgram("Physics_Rigidbody_Insert");

		CRenderPass::SetMaxNumVersions(MAX_NUM_RIGIDBODY_TO_INSERT);

		CRenderPass::SetEntryPoint(InsertRigidbodies);

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute("Build Physics Acceleration Structure"))
	{
		// Clear Buffers
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_LinkedListNodeBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1, ms_ParticleIndexBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(2, ms_pAccelerationStructure->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(3, ms_ContactConstraintBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(4, ms_IndirectDispatchBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_AccGrid_Clear");

			CRenderPass::SetEntryPoint(ClearAccelerationStructure);

			CRenderPass::EndSubPass();
		}

		// Insert Particles in Linked List
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_ParticlePool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1, ms_pAccelerationStructure->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_LinkedListNodeBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_AccGrid_Insert");

			CRenderPass::SetEntryPoint(BuildAccelerationStructure);

			CRenderPass::EndSubPass();
		}

		// Compactify Particle Indices
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_LinkedListNodeBuffer, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1, ms_ParticleIndexBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(2, ms_pAccelerationStructure->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("Physics_AccGrid_Compactify");

			CRenderPass::SetEntryPoint(CompactifyAccelerationStructure);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}


	if (CRenderPass::BeginCompute("Run Physics"))
	{
		// Apply Step With External Forces
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_RigidbodyBuffer, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1, ms_ParticleInitPool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(2, ms_ParticlePool, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_FreeStep");

			CRenderPass::SetEntryPoint(ApplyFreeStep);

			CRenderPass::EndSubPass();
		}

		// Compute Constraints
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_ParticlePool,						CShader::e_ComputeShader,	CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1, ms_pAccelerationStructure->GetID(),	CShader::e_ComputeShader);
			CRenderPass::SetNumTextures(2, 1);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindResourceToWrite(4, ms_ContactConstraintBuffer,			CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(5, ms_IndirectDispatchBuffer,			CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_ComputeConstraints");

			CRenderPass::SetEntryPoint(ComputeConstraints);

			CRenderPass::EndSubPass();
		}


		for (int i = 0; i < NUM_GAUSS_SEIDEL_ITER; i++)
		{
			// Update Multipliers
			if (CRenderPass::BeginComputeSubPass())
			{
				CRenderPass::BindResourceToWrite(0, ms_ParticlePool, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
				CRenderPass::BindResourceToWrite(1, ms_ContactConstraintBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

				CRenderPass::BindProgram("Physics_UpdateMultipliers");

				CRenderPass::SetEntryPoint(UpdateMultipliers);

				CRenderPass::EndSubPass();
			}

			if (i < NUM_GAUSS_SEIDEL_ITER - 1)
			{
				// Update Constraints
				if (CRenderPass::BeginComputeSubPass())
				{
					CRenderPass::BindResourceToRead(0, ms_ParticlePool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
					CRenderPass::SetNumTextures(1, 1);
					CRenderPass::SetNumSamplers(2, 1);

					CRenderPass::BindResourceToWrite(3, ms_ContactConstraintBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

					CRenderPass::BindProgram("Physics_UpdateConstraints");

					CRenderPass::SetEntryPoint(UpdateConstraints);

					CRenderPass::EndSubPass();
				}
			}
		}

		// Compute Center Of Mass 
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0,	ms_ParticlePool,	CShader::e_ComputeShader,		CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(1, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_ComputeCenterOfMass");

			CRenderPass::SetEntryPoint(ComputeCenterOfMass);

			CRenderPass::EndSubPass();
		}

		// Compute Shape Matching Matrix
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_ParticlePool,		CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1, ms_ParticleInitPool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(2, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_ComputeShapeMatchingMatrix");

			CRenderPass::SetEntryPoint(ComputeShapeMatchingMatrix);

			CRenderPass::EndSubPass();
		}

		// Rigid Bodies Shape Matching
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::SetNumRWBuffers(1, 1);

			CRenderPass::BindProgram("Physics_ShapeMatching");

			CRenderPass::SetEntryPoint(ShapeMatching);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}


	/*if (CRenderPass::BeginCompute("Run Physics"))
	{
		// Compute Forces
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pAccelerationStructure->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_ParticlePool,						CShader::e_ComputeShader,		CRenderPass::e_Buffer);
			CRenderPass::SetNumTextures(2, 1);
			CRenderPass::SetNumSamplers(3, 1);

			CRenderPass::BindResourceToWrite(4, ms_ParticleForcesPool,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_ComputeForces");

			CRenderPass::SetEntryPoint(ComputeForces);

			CRenderPass::EndSubPass();
		}

		// Integrate Momentum
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_ParticlePool,			CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1, ms_ParticleForcesPool,	CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(2, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("Physics_IntegrateMomentum");

			CRenderPass::SetEntryPoint(IntegrateMomentum);

			CRenderPass::EndSubPass();
		}

		// Update Center of Mass
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_RigidbodyBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::SetNumRWBuffers(1, 1);

			CRenderPass::BindProgram("Physics_UpdateCenterOfMass");

			CRenderPass::SetEntryPoint(UpdateCenterOfMass);

			CRenderPass::EndSubPass();
		}

		// Update Particles
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_ParticleInitPool, CShader::e_ComputeShader, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToRead(1, ms_RigidbodyBuffer, CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(2, ms_ParticlePool,		CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
		
			CRenderPass::BindProgram("Physics_UpdateParticles");
		
			CRenderPass::SetEntryPoint(UpdateParticles);
		
			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}*/
}


void CPhysicsEngine::Terminate()
{

}


void CPhysicsEngine::Run()
{
	std::vector<SRenderPassTask> renderPasses;
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Voxelize Rigid Bodies"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Insert Rigid Body Particles"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Build Physics Acceleration Structure"));
	renderPasses.push_back(CRenderPass::GetRenderPassTask("Run Physics"));

	CSchedulerThread::AddRenderTask(g_PhysicsCommandList, renderPasses);
	CCommandListManager::ScheduleDeferredKickoff(g_PhysicsCommandList);
}


void CPhysicsEngine::ClearAccelerationStructure()
{
	CDeviceManager::Dispatch((ms_pAccelerationStructure->GetWidth() + 7) / 8, (ms_pAccelerationStructure->GetHeight() + 7) / 8, (ms_pAccelerationStructure->GetDepth() + 7) / 8);
}


void CPhysicsEngine::BuildAccelerationStructure()
{
	if (ms_nNumParticles == 0)
		return;

	float4 constants[2];
	constants[0]	= ms_GridCenter;
	constants[0].w	= ms_fParticleSize;
	constants[1]	= ms_GridSize;
	constants[1].w	= *reinterpret_cast<float*>(&ms_nNumParticles);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 1023) / 1024, 1, 1);
}


void CPhysicsEngine::CompactifyAccelerationStructure()
{
	CDeviceManager::Dispatch((ms_pAccelerationStructure->GetWidth() + 7) / 8, (ms_pAccelerationStructure->GetHeight() + 7) / 8, (ms_pAccelerationStructure->GetDepth() + 7) / 8);
}


void CPhysicsEngine::ApplyFreeStep()
{
	if (ms_nNumParticles == 0)
		return;

	float constants[2];
	constants[0] = *reinterpret_cast<float*>(&ms_nNumParticles);
	constants[1] = CEngine::GetFrameDuration() * 0.01f;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::ComputeConstraints()
{
	if (ms_nNumParticles == 0)
		return;

	if (ms_pStaticCollisionSDF != nullptr)
		CTextureInterface::SetTexture(ms_pStaticCollisionSDF->GetSDFTexture(), 2, CShader::e_ComputeShader);

	CResourceManager::SetSampler(3, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	float4 constants[4];
	constants[0] = ms_GridCenter;
	constants[0].w = ms_fParticleSize;
	constants[1] = ms_GridSize;
	constants[1].w = *reinterpret_cast<float*>(&ms_nNumParticles);
	constants[2] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetCenter();
	constants[3] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetSize();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::UpdateMultipliers()
{
	if (ms_nNumParticles == 0)
		return;

	float dt = CEngine::GetFrameDuration() * 0.01f;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &dt, sizeof(dt));

	CDeviceManager::DispatchIndirect(ms_IndirectDispatchBuffer, 0);
}


void CPhysicsEngine::UpdateConstraints()
{
	if (ms_nNumParticles == 0)
		return;

	if (ms_pStaticCollisionSDF != nullptr)
		CTextureInterface::SetTexture(ms_pStaticCollisionSDF->GetSDFTexture(), 1, CShader::e_ComputeShader);

	CResourceManager::SetSampler(2, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	float4 constants[2];
	constants[0] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetCenter();
	constants[1] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetSize();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::DispatchIndirect(ms_IndirectDispatchBuffer, 0);
}


void CPhysicsEngine::ComputeCenterOfMass()
{
	if (ms_nNumParticles == 0)
		return;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &ms_nNumParticles, sizeof(ms_nNumParticles));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::ComputeShapeMatchingMatrix()
{
	if (ms_nNumParticles == 0)
		return;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &ms_nNumParticles, sizeof(ms_nNumParticles));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::ShapeMatching()
{
	if (ms_nNumParticles == 0)
		return;

	CResourceManager::SetRwBuffer(1, ms_RigidbodyData[ms_CurrentBufferId]);

	unsigned int numRigidbodies = static_cast<unsigned int>(ms_pRigidbodies.size());

	float dt = CEngine::GetFrameDuration() * 0.01f;

	unsigned int constants[2];
	constants[0] = numRigidbodies;
	constants[1] = *reinterpret_cast<unsigned int*>(&dt);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((numRigidbodies + 63) / 64, 1, 1);
}


/*
void CPhysicsEngine::ComputeForces()
{
	if (ms_nNumParticles == 0)
		return;

	CResourceManager::SetSampler(3, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	if (ms_pStaticCollisionSDF != nullptr)
	{
		CTextureInterface::SetTexture(ms_pStaticCollisionSDF->GetSDFTexture(), 2, CShader::e_ComputeShader);
	}

	float4 constants[4];
	constants[0] = ms_GridCenter;
	constants[0].w = ms_fParticleSize;
	constants[1] = ms_GridSize;
	constants[1].w = *reinterpret_cast<float*>(&ms_nNumParticles);
	constants[2] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetCenter();
	constants[3] = (ms_pStaticCollisionSDF == nullptr) ? float3(0.f, 0.f, 0.f) : ms_pStaticCollisionSDF->GetSize();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::IntegrateMomentum()
{
	if (ms_nNumParticles == 0)
		return;

	float dt = MIN(1e-2f, CEngine::GetFrameDuration());

	unsigned int constants[2];
	constants[0] = ms_nNumParticles;
	constants[1] = *reinterpret_cast<unsigned int*>(&dt);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}


void CPhysicsEngine::UpdateCenterOfMass()
{
	if (ms_nNumParticles == 0)
		return;

	CResourceManager::SetRwBuffer(1, ms_RigidbodyData[ms_CurrentBufferId]);

	float dt = MIN(1e-2f, CEngine::GetFrameDuration());

	unsigned int numRigidbodies = static_cast<unsigned int>(ms_pRigidbodies.size());

	unsigned int constants[2];
	constants[0] = numRigidbodies;
	constants[1] = *reinterpret_cast<unsigned int*>(&dt);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

	CDeviceManager::Dispatch((numRigidbodies + 63) / 64, 1, 1);
}


void CPhysicsEngine::UpdateParticles()
{
	if (ms_nNumParticles == 0)
		return;

	unsigned int constants = ms_nNumParticles;

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch((ms_nNumParticles + 255) / 256, 1, 1);
}*/


void CPhysicsEngine::InsertRigidbodies()
{
	int numRigidbodies = MIN(MAX_NUM_RIGIDBODY_TO_INSERT, static_cast<int>(ms_pRigidbodiesInsertListToFlush->size()));

	for (int i = 0; i < numRigidbodies; i++)
	{
		CRigidbody& body = *(*ms_pRigidbodiesInsertListToFlush)[i];

		if (!body.m_bUpToDate)
			continue;

		CResourceManager::SetRwBuffer(0, ms_ParticleInitPool);
		CResourceManager::SetRwBuffer(1, ms_ParticlePool);
		CTextureInterface::SetRWTexture(body.m_pVoxels->GetID(), 2, CShader::e_ComputeShader);
		CResourceManager::SetRwBuffer(3, ms_RigidbodyBuffer);

		body.m_nStartOffset = ms_nNumParticles;
		ms_nNumParticles += body.m_nNumParticles;

		float4 constants[4];
		constants[0]	= body.m_Center;
		constants[0].w	= body.m_fMass;
		constants[1]	= body.m_Size;
		constants[1].w	= *reinterpret_cast<float*>(&body.m_nStartOffset);
		constants[2]	= body.m_CenterOfMass;
		constants[2].w	= *reinterpret_cast<float*>(&body.m_nId);
		constants[3].w	= *reinterpret_cast<float*>(&body.m_nNumParticles);

		CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

		CDeviceManager::Dispatch((body.m_pVoxels->GetWidth() + 7) / 8, (body.m_pVoxels->GetHeight() + 7) / 8, (body.m_pVoxels->GetDepth() + 7) / 8);

		body.m_bInserted = true;
	}
}


void CPhysicsEngine::UpdateBeforeFlush()
{
	int numRigidbodies = static_cast<int>(ms_pRigidbodiesInsertListToFlush->size());

	for (int i = 0; i < numRigidbodies; i++)
		if (!(*ms_pRigidbodiesInsertListToFlush)[i]->m_bInserted)
			ms_pRigidbodiesInsertListToFill->push_back((*ms_pRigidbodiesInsertListToFlush)[i]);

	std::vector<CRigidbody*>* tmp = ms_pRigidbodiesInsertListToFill;
	ms_pRigidbodiesInsertListToFill = ms_pRigidbodiesInsertListToFlush;
	ms_pRigidbodiesInsertListToFlush = tmp;

	ms_pRigidbodiesInsertListToFill->clear();

	ms_CurrentBufferId = (ms_CurrentBufferId + 1) % 5;

	numRigidbodies = static_cast<unsigned int>(ms_pRigidbodies.size());

	void* pData = CResourceManager::MapBuffer(ms_RigidbodyData[ms_CurrentBufferId]);

	for (int i = 0; i < numRigidbodies; i++)
	{
		float4 CdM	= ((float4*)pData)[2 * i];
		float4 Q	= ((float4*)pData)[2 * i + 1];

		ms_pRigidbodies[i]->m_CenterOfMass	= float3(CdM.x, CdM.y, CdM.z);
		ms_pRigidbodies[i]->m_Rotation		= Quaternion(Q.w, Q.x, Q.y, Q.z);

		float3 center = ms_pRigidbodies[i]->m_CenterOfMassToBoxCenter;

		float3 u = float3(Q.x, Q.y, Q.z);
		float s = Q.w;
		center = 2.0f * float3::dotproduct(u, center) * u + (s * s - float3::dotproduct(u, u)) * center + 2.0f * s * float3::cross(u, center);

		ms_pRigidbodies[i]->m_WorldMatrix	= Quaternion::GetMatrix(ms_pRigidbodies[i]->m_Rotation, ms_pRigidbodies[i]->m_CenterOfMass + center);
	}

	CResourceManager::UnmapBuffer(ms_RigidbodyData[ms_CurrentBufferId]);

	CRigidbody::UpdateBeforeFlush();
}


//#include "Engine/Engine.h"
//#include "Physics.h"
//#include "Engine/Renderer/Renderer.h"
//#include "Engine/Device/DeviceManager.h"
//#include <string.h>
//
//
//
//std::vector<CSoftbody*>		CPhysicsEngine::ms_pSoftbodies;
//std::vector<CRigidbody*>	CPhysicsEngine::ms_pRigidbodies;
//std::vector<CObstacle*>		CPhysicsEngine::ms_pObstacles;
//
//
//bool	CPhysicsEngine::ms_bOncePerRenderedFrame = false;
//bool	CPhysicsEngine::ms_bShouldUpdateObstacles = true;
//
//
//unsigned int	CPhysicsEngine::ms_nObstacleVertexBufferID	= INVALIDHANDLE;
//int				CPhysicsEngine::ms_nTotalWallCount			= 0;
//ProgramHandle	CPhysicsEngine::ms_ProgramID[32]			= { INVALID_PROGRAM_HANDLE };
//
//CObstacle::SGPUObstacle CPhysicsEngine::ms_pObstacleData[PHYSICS_MAX_OBSTACLES];
//
//bool	CPhysicsEngine::ms_bIsInit = false;
//
//
//void CPhysicsEngine::Init()
//{
//	ms_pSoftbodies.clear();
//	ms_pRigidbodies.clear();
//	ms_pObstacles.clear();
//
//	ms_bShouldUpdateObstacles = true;
//
//	/*if (CRenderPass::BeginCompute("Softbody Physics"))
//	{
//		// Reset Velocities
//		if (CRenderPass::BeginComputeSubPass())
//		{
//			CRenderPass::SetNumRWBuffers(0, 1);
//
//			CRenderPass::BindProgram("Softbody_ResetVelocities");
//
//			CRenderPass::SetEntryPoint(CSoftbody::Sim_ResetVelocities);
//
//			CRenderPass::EndSubPass();
//		}
//
//		// Compute Statistics
//		if (CRenderPass::BeginComputeSubPass())
//		{
//			CRenderPass::SetNumRWBuffers(0, 1);
//			CRenderPass::SetNumRWBuffers(1, 1);
//
//			CRenderPass::BindProgram("Softbody_MeanValues");
//
//			CRenderPass::SetEntryPoint(CSoftbody::Sim_ComputeStatistics);
//
//			CRenderPass::EndSubPass();
//		}
//
//		// Compute Volume
//		if (CRenderPass::BeginComputeSubPass())
//		{
//			CRenderPass::SetNumRWBuffers(0, 1);
//			CRenderPass::SetNumRWBuffers(1, 1);
//
//			CRenderPass::BindProgram("Softbody_Volume");
//
//			CRenderPass::SetEntryPoint(CSoftbody::Sim_ComputeVolume);
//
//			CRenderPass::EndSubPass();
//		}
//
//		// Update Softbody
//		if (CRenderPass::BeginComputeSubPass())
//		{
//			CRenderPass::SetNumRWBuffers(0, 1);
//			CRenderPass::SetNumRWBuffers(1, 1);
//
//			CRenderPass::BindProgram("Softbody_Main");
//
//			CRenderPass::SetEntryPoint(CSoftbody::Sim_UpdateSoftbody);
//
//			CRenderPass::EndSubPass();
//		}
//
//		CRenderPass::End();
//	}*/
//
//	size_t nWallBufferSize = PHYSICS_MAX_OBSTACLES * sizeof(CObstacle::SGPUObstacle);
//
//	ms_nObstacleVertexBufferID = CResourceManager::CreateRwBuffer(nWallBufferSize, true);
//
//	ms_bIsInit = true;
//}
//
//
//void CPhysicsEngine::ReloadShaders()
//{
//	
//
//	ms_bIsInit = false;
//}
//
//
//void CPhysicsEngine::Terminate()
//{
//	ms_pSoftbodies.clear();
//	ms_pRigidbodies.clear();
//	ms_pObstacles.clear();
//}
//
//
//
//bool CPhysicsEngine::CheckCollision(SSphere& sphere)
//{
//	CObstacle* pObstacle;
//	float3 Center, MTV;
//	float fDepth, fMinDepth;
//
//	SOrientedBox Obstacle;
//
//	Obstacle.m_Basis[0] = float3(1.f, 0.f, 0.f);
//	Obstacle.m_Basis[1] = float3(0.f, 1.f, 0.f);
//	Obstacle.m_Basis[2] = float3(0.f, 0.f, 1.f);
//
//	std::vector<CObstacle*>::iterator it;
//
//	for (it = ms_pObstacles.begin(); it < ms_pObstacles.end(); it++)
//	{
//		pObstacle = *it;
//
//		if (pObstacle->m_bEnabled && pObstacle->m_bUseOnCPU)
//		{
//			Copy(&Center.x, pObstacle->m_Center);
//
//			if ((Center - sphere.m_Position).length() < sphere.m_fRadius + pObstacle->m_fBoundingSphereRadius)
//			{
//				Copy(&Obstacle.m_Position.x, pObstacle->m_Center);
//				Copy(Obstacle.m_Dim, pObstacle->m_AABB.v());
//
//				if (Collisions::GetMinimumTranslationVector(Obstacle, sphere, MTV, &fDepth, &fMinDepth))
//				{
//					return true;
//				}
//			}
//		}
//	}
//
//	return false;
//}
//
//
//
//void CPhysicsEngine::Run()
//{
//	UpdateObstacles();
//
//	//UpdateSoftbodies();
//
//	UpdateRigidbodies();
//	
//	//UpdateParticles();
//}
//
//
//void CPhysicsEngine::UpdateSoftbodies()
//{
//	static float fLeftOver = 0.f;
//	float tframe = MIN(CEngine::GetFrameDuration(), PHYSICS_MAX_FRAME_COUNT * PHYSICS_SOFTBODY_TIMESTEP_MS);
//
//	float stable_dt = PHYSICS_SOFTBODY_TIMESTEP_MS;
//	float t = 0.f;
//
//	if (tframe + fLeftOver < stable_dt)
//		return;
//
//	std::vector<CSoftbody*>::iterator it;
//	CSoftbody* pSoftbody;
//
//	for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
//	{
//		pSoftbody = *it;
//
//		if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
//		{
//			pSoftbody->m_bIsInCollision = false;
//			pSoftbody->SumForces();
//		}
//	}
//
//	if (fLeftOver > stable_dt)
//	{
//		tframe += (int)(fLeftOver / stable_dt) * stable_dt;
//		fLeftOver -= (int)(fLeftOver / stable_dt) * stable_dt;
//	}
//
//	ms_bOncePerRenderedFrame = true;
//
//	do
//	{
//		if (t + stable_dt > tframe)
//		{
//			fLeftOver += tframe - t;
//			break;
//		}
//
//		for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
//		{
//			pSoftbody = *it;
//
//			if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
//				pSoftbody->Update(stable_dt);
//		}
//
//		t += stable_dt;
//
//		ms_bOncePerRenderedFrame = false;
//
//	} while (t < tframe);
//
//
//	for (it = ms_pSoftbodies.begin(); it < ms_pSoftbodies.end(); it++)
//	{
//		pSoftbody = *it;
//
//		if (pSoftbody->m_bEnabled && !pSoftbody->IsColliding())
//			pSoftbody->m_LastFreeVelocity = pSoftbody->m_Velocity;
//
//		if (pSoftbody->m_bEnabled && !pSoftbody->m_bFrozen)
//			pSoftbody->SkinPacket();
//	}
//}
//
//
//void CPhysicsEngine::UpdateRigidbodies()
//{
//	static float fLeftOver = 0.f;
//	float tframe = MIN(CEngine::GetFrameDuration(), PHYSICS_MAX_FRAME_COUNT * PHYSICS_RIGIDBODY_TIMESTEP_MS);
//
//	float stable_dt = PHYSICS_RIGIDBODY_TIMESTEP_MS;
//	float t = 0.f;
//
//	if (tframe + fLeftOver < stable_dt)
//		return;
//
//	if (fLeftOver > stable_dt)
//	{
//		tframe += (int)(fLeftOver / stable_dt) * stable_dt;
//		fLeftOver -= (int)(fLeftOver / stable_dt) * stable_dt;
//	}
//
//	ms_bOncePerRenderedFrame = true;
//
//	std::vector<CRigidbody*>::iterator it;
//	CRigidbody* pRigidbody;
//
//	do
//	{
//		if (t + stable_dt > tframe)
//		{
//			fLeftOver += tframe - t;
//			break;
//		}
//
//
//		for (it = ms_pRigidbodies.begin(); it < ms_pRigidbodies.end(); it++)
//		{
//			pRigidbody = *it;
//
//			if (pRigidbody->m_bEnabled && !pRigidbody->IsIdle())
//				pRigidbody->Update(stable_dt * 1e-3f);
//		}
//
//		t += stable_dt;
//
//		ms_bOncePerRenderedFrame = false;
//
//	} while (t < tframe);
//
//	CRigidbody::ms_nSphereCount = 0;
//}
//
//
//void CPhysicsEngine::UpdateParticles()
//{
//	
//}
//
//
//void CPhysicsEngine::UpdateObstacles()
//{
//	std::vector<CObstacle*>::iterator it;
//	CObstacle* pObstacle;
//	size_t nWallSize = 0;
//	CObstacle::SGPUObstacle* pObstacles = ms_pObstacleData;
//	int i = 0;
//
//	if (ms_bShouldUpdateObstacles)
//		ms_nTotalWallCount = 0;
//
//	for (it = ms_pObstacles.begin(); it < ms_pObstacles.end(); it++)
//	{
//		pObstacle = *it;
//
//		if (pObstacle->m_bEnabled && pObstacle->m_bUseOnGPU)
//		{
//			pObstacle->Update();
//			if (ms_bShouldUpdateObstacles)
//			{
//				pObstacles[i].m_nWallCount = pObstacle->m_nWallCount;
//				pObstacles[i].m_fRadius = pObstacle->m_fBoundingSphereRadius;
//				Copy(pObstacles[i].m_Center, pObstacle->m_Center);
//				memcpy(&(pObstacles[i].m_Walls), pObstacle->m_pWalls, pObstacle->m_nWallCount * sizeof(CObstacle::SWall));
//				ms_nTotalWallCount += pObstacle->m_nWallCount;
//			}
//			i++;
//		}
//	}
//
//
//
//	if (ms_bShouldUpdateObstacles)
//	{
//		/*void* pBuffer = glMapNamedBuffer(ms_nObstacleVertexBufferID, GL_WRITE_ONLY);
//		memcpy(pBuffer, ms_pObstacleData, ms_pObstacles.size() * sizeof(CObstacle::SGPUObstacle));
//		glUnmapNamedBuffer(ms_nObstacleVertexBufferID);
//		ms_bShouldUpdateObstacles = false;*/
//	}
//}







