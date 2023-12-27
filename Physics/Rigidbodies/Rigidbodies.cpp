#include "Engine/Engine.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Renderer/Renderer.h"


#define MAX_NUM_RIGIDBODY_TO_BAKE	16


CTexture*	CRigidbody::ms_pDummyTarget = nullptr;

float3		CRigidbody::ms_CurrentCenter;
float3		CRigidbody::ms_CurrentSize;

std::vector<CRigidbody*>	CRigidbody::ms_pRigidbodiesToVoxelize[2] ;
std::vector<CRigidbody*>*	CRigidbody::ms_pRigidbodiesVoxelizeListToFill = &CRigidbody::ms_pRigidbodiesToVoxelize[0];
std::vector<CRigidbody*>*	CRigidbody::ms_pRigidbodiesVoxelizeListToFlush = &CRigidbody::ms_pRigidbodiesToVoxelize[1];

std::vector<CRigidbody*>	CRigidbody::ms_pRigidbodies;


void CRigidbody::Init()
{
	ms_pRigidbodies.clear();

	ms_pDummyTarget = new CTexture(512, 512, ETextureFormat::e_R8, eTexture2D);

	//if (CRenderPass::BeginGraphics("Voxelize Rigid Bodies"))
	//{
	//	// Clear
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::SetNumRWTextures(0, 1);
	//		CRenderPass::SetNumRWBuffers(1, 1);

	//		CRenderPass::BindProgram("Rigidbody_Voxelize_Clear");

	//		CRenderPass::SetMaxNumVersions(MAX_NUM_RIGIDBODY_TO_BAKE);

	//		CRenderPass::SetEntryPoint(Voxelize_Clear);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Voxelize
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::SetNumRWTextures(0, 1);

	//		CRenderPass::BindProgram("Rigidbody_Voxelize", "Rigidbody_Voxelize");

	//		CRenderPass::SetMaxNumVersions(MAX_NUM_RIGIDBODY_TO_BAKE);

	//		CRenderPass::SetEntryPoint(Voxelize);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Compute Particle Count
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::SetNumRWTextures(0, 1);
	//		CRenderPass::SetNumRWBuffers(1, 1);

	//		CRenderPass::BindProgram("Rigidbody_CountVoxels");

	//		CRenderPass::SetEntryPoint(CountVoxels);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Compute Inertia Tensor
	//	/*if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::SetNumRWTextures(0, 1);
	//		CRenderPass::SetNumRWBuffers(1, 1);

	//		CRenderPass::BindProgram("Rigidbody_ComputeInertia");

	//		CRenderPass::SetEntryPoint(ComputeInertiaTensor);

	//		CRenderPass::EndSubPass();
	//	}*/

	//	CRenderPass::End();
	//}
}


CRigidbody::CRigidbody(CMesh* pMesh, float scale, float mass)
{
	m_pPacketList = reinterpret_cast<PacketList*>(pMesh->GetPacketList());
	
	m_fMass = mass;

	m_nNumParticles = 0;
	m_nStartOffset = 0;

	float3 box = pMesh->GetBoundingBox() * scale;

	float3 dim = box / CPhysicsEngine::GetParticleSize();

	for (int i = 0; i < 3; i++)
		dim.v()[i] = ceil(dim.v()[i]);

	m_Size		= dim * CPhysicsEngine::GetParticleSize();
	m_Center	= pMesh->GetCenter();

	m_Rotation = Quaternion(1.f, 0.f, 0.f, 0.f);
	m_LinearMomentum = 0.f;
	m_AngularMomentum = 0.f;

	m_CenterOfMass = 0.f;
	m_CenterOfMassCoords = 0.f;

	m_WorldMatrix.Eye();

	m_NumVoxelsBuffer = CResourceManager::CreateRwBuffer(16 * sizeof(unsigned int), true);

	m_pVoxels = new CTexture((int)dim.x, (int)dim.y, (int)dim.z, ETextureFormat::e_R32_UINT, eTextureStorage3D);

	m_pVoxels->TransitionToState(CRenderPass::e_UnorderedAccess);

	m_Fence = CResourceManager::CreateFence();

	m_bUpToDate = true;
	m_bInserted = false;

	m_nId = static_cast<unsigned int>(ms_pRigidbodies.size());
	ms_pRigidbodies.push_back(this);

	Bake();
}


void CRigidbody::Voxelize_Clear()
{
	int numRigidbodies = MIN(MAX_NUM_RIGIDBODY_TO_BAKE, static_cast<int>(ms_pRigidbodiesVoxelizeListToFlush->size()));

	for (int i = 0; i < numRigidbodies; i++)
	{
		CTexture* pTex0 = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pVoxels;
		BufferId buffer = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_NumVoxelsBuffer;

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_ComputeShader);
		CResourceManager::SetRwBuffer(1, buffer);

		CDeviceManager::Dispatch((pTex0->GetWidth() + 7) / 8, (pTex0->GetHeight() + 7) / 8, (pTex0->GetDepth() + 7) / 8);
	}

	for (int i = 0; i < numRigidbodies; i++)
	{
		CTexture* pTex0 = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pVoxels;
		BufferId buffer = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_NumVoxelsBuffer;

		CFrameBlueprint::TransitionBarrier(pTex0->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier(buffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate, CRenderPass::e_Buffer);
	}

	CFrameBlueprint::FlushBarriers();
}


void CRigidbody::CountVoxels()
{
	int numRigidbodies = MIN(MAX_NUM_RIGIDBODY_TO_BAKE, static_cast<int>(ms_pRigidbodiesVoxelizeListToFlush->size()));

	for (int i = 0; i < numRigidbodies; i++)
	{
		CTexture* pTex0 = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pVoxels;
		BufferId buffer = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_NumVoxelsBuffer;

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_ComputeShader);
		CResourceManager::SetRwBuffer(1, buffer);

		float4 constants[2];
		constants[0] = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_Center;
		constants[1] = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_Size;

		CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

		CDeviceManager::Dispatch((pTex0->GetWidth() + 7) / 8, (pTex0->GetHeight() + 7) / 8, (pTex0->GetDepth() + 7) / 8);

		CFrameBlueprint::TransitionBarrier(pTex0->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier(buffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate, CRenderPass::e_Buffer);
	}

	for (int i = 0; i < numRigidbodies; i++)
	{
		CRigidbody& body = *(*ms_pRigidbodiesVoxelizeListToFlush)[i];

		CTexture* pTex0 = body.m_pVoxels;
		BufferId buffer = body.m_NumVoxelsBuffer;

		CFrameBlueprint::TransitionBarrier(pTex0->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier(buffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate, CRenderPass::e_Buffer);

		CResourceManager::SubmitFence(body.m_Fence);

		body.m_bUpToDate = false;
	}

	CFrameBlueprint::FlushBarriers();
}


void CRigidbody::ComputeInertiaTensor()
{
	int numRigidbodies = MIN(MAX_NUM_RIGIDBODY_TO_BAKE, static_cast<int>(ms_pRigidbodiesVoxelizeListToFlush->size()));

	for (int i = 0; i < numRigidbodies; i++)
	{
		CRigidbody& body = *(*ms_pRigidbodiesVoxelizeListToFlush)[i];

		CTexture* pTex0 = body.m_pVoxels;
		BufferId buffer = body.m_NumVoxelsBuffer;

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_ComputeShader);
		CResourceManager::SetRwBuffer(1, buffer);

		float4 constants[2];
		constants[0] = body.m_Center;
		constants[1] = body.m_Size;

		CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

		CDeviceManager::Dispatch((pTex0->GetWidth() + 7) / 8, (pTex0->GetHeight() + 7) / 8, (pTex0->GetDepth() + 7) / 8);

		CResourceManager::SubmitFence(body.m_Fence);

		body.m_bUpToDate = false;
	}
}


void CRigidbody::Bake()
{
	ms_pRigidbodiesVoxelizeListToFill->push_back(this);
}


void CRigidbody::Voxelize()
{
	int numRigidbodies = MIN(MAX_NUM_RIGIDBODY_TO_BAKE, static_cast<int>(ms_pRigidbodiesVoxelizeListToFlush->size()));

	CPacketManager::ForceShaderHook(VoxelizeUpdateShader);
	CRenderer::DisableViewportCheck();

	for (int i = 0; i < numRigidbodies; i++)
	{
		CTexture* pTex0 = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pVoxels;

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_FragmentShader);

		CRenderer::SShaderData pShaderData;
		pShaderData.m_nInstancedBufferID			= INVALIDHANDLE;
		pShaderData.m_nInstancedBufferByteOffset	= 0;
		pShaderData.m_nInstancedStreamMask			= 0;
		pShaderData.m_nInstancedBufferStride		= 0;
		pShaderData.m_nNbInstances					= 3;

		ms_CurrentCenter	= (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_Center;
		ms_CurrentSize		= (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_Size;

		PacketList* packets = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pPacketList;

		CRenderer::DrawPacketList(packets, pShaderData, CMaterial::e_Deferred | CMaterial::e_Forward);
	}

	CRenderer::EnableViewportCheck();
	CPacketManager::ForceShaderHook(0);

	for (int i = 0; i < numRigidbodies; i++)
	{
		CTexture* pTex0 = (*ms_pRigidbodiesVoxelizeListToFlush)[i]->m_pVoxels;
		CFrameBlueprint::TransitionBarrier(pTex0->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();
}


void CRigidbody::UpdateBeforeFlush()
{
	int numRigidbodies = static_cast<int>(ms_pRigidbodiesVoxelizeListToFlush->size());

	for (int i = 0; i < numRigidbodies; i++)
	{
		CRigidbody& body = *(*ms_pRigidbodiesVoxelizeListToFlush)[i];

		if (!body.m_bUpToDate)
		{
			CResourceManager::WaitForFence(body.m_Fence, 0);
			body.m_bUpToDate = true;

			void* pData = CResourceManager::MapBuffer(body.m_NumVoxelsBuffer);

			memcpy(body.m_CenterOfMass.v(), reinterpret_cast<float*>(pData), 3 * sizeof(float));

			body.m_nNumParticles = *(reinterpret_cast<unsigned int*>(pData) + 3);

			memcpy(body.m_InertiaTensor.m(), reinterpret_cast<float*>(pData) + 4, 3 * sizeof(float));
			memcpy(body.m_InertiaTensor.m() + 3, reinterpret_cast<float*>(pData) + 8, 3 * sizeof(float));
			memcpy(body.m_InertiaTensor.m() + 6, reinterpret_cast<float*>(pData) + 12, 3 * sizeof(float));

			CResourceManager::UnmapBuffer(body.m_NumVoxelsBuffer);

			body.m_CenterOfMass			/= 1.f * body.m_nNumParticles;
			body.m_InertiaTensor		= (1.f / body.m_nNumParticles) * body.m_InertiaTensor;

			//body.m_CenterOfMass = (body.m_CenterOfMassCoords - float3(0.5f)) * body.m_Size + body.m_Center;

			body.m_CenterOfMassToBoxCenter = 0.f - body.m_CenterOfMass;
		}
	}

	std::vector<CRigidbody*>* tmp = ms_pRigidbodiesVoxelizeListToFill;
	ms_pRigidbodiesVoxelizeListToFill = ms_pRigidbodiesVoxelizeListToFlush;
	ms_pRigidbodiesVoxelizeListToFlush = tmp;

	ms_pRigidbodiesVoxelizeListToFill->clear();
}


int CRigidbody::VoxelizeUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	float4 constants[2];
	constants[0] = ms_CurrentCenter;
	constants[1] = ms_CurrentSize;

	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_FragmentShader, constants, 2 * sizeof(float4));

	return 1;
}


//#include "Engine/Engine.h"
//#include "Engine/Physics/Physics.h"
//
//
//float CRigidbody::ms_fDefaultSphereRadius = 0.05f;
//
//unsigned int CRigidbody::ms_nSphereCount = 0;
//unsigned int CRigidbody::ms_nPartialSphereCount = 0;
//
//SSphere CRigidbody::ms_CollisionSpheres[MAX_COLLISION_SPHERES];
//SPartialSphere CRigidbody::ms_CollisionPartialSpheres[MAX_COLLISION_SPHERES];
//
//CRigidbody::CRigidbody(CMesh* pMesh, float fMass, bool bIsUniform)
//{
//	
//	m_Position = pMesh->GetCenter();
//	m_Rotation = Quaternion(1.f, 0.f, 0.f, 0.f);
//
//	m_LinearMomentum = 0.f;
//	m_AngularMomentum = float3(0.f, 0.f, 0.f);
//
//	m_AABB = pMesh->GetBoundingBox();
//	m_fSphereRadius = pMesh->m_fBoundingSphereRadius;
//
//	m_OriginalInertiaTensor = fMass * float3x3(	float3(0.25f * (m_AABB.y * m_AABB.y + m_AABB.z * m_AABB.z) / 3.f, 0.f, 0.f),
//												float3(0.f, 0.25f * (m_AABB.x * m_AABB.x + m_AABB.z * m_AABB.z) / 3.f, 0.f),
//												float3(0.f, 0.f, 0.25f * (m_AABB.x * m_AABB.x + m_AABB.y * m_AABB.y) / 3.f));
//
//	m_InertiaTensor = m_OriginalInertiaTensor;
//	m_fMass = fMass;
//
//	m_bIdle = false;
//	m_bShouldCollideWithRigidBodies = false;
//	m_bShouldCollideWithSpheres = false;
//
//	m_bIsColliding = false;
//	m_fSize = 1.f;
//
//	m_MTV = 0.f;
//
//	m_bEnabled = true;
//}
//
//
//CRigidbody::~CRigidbody()
//{
//	
//}
//
//
//void CRigidbody::AddSphere(float3& Position, float3& Velocity, float fRadius, bool bForceCollision)
//{
//	ms_CollisionSpheres[ms_nSphereCount].m_Position = Position;
//	ms_CollisionSpheres[ms_nSphereCount].m_Velocity = Velocity;
//	ms_CollisionSpheres[ms_nSphereCount].m_fRadius = fRadius;
//	ms_CollisionSpheres[ms_nSphereCount].m_bForceCollision = bForceCollision;
//
//	ms_nSphereCount++;
//}
//
//
//void CRigidbody::AddPartialSphere(float3& Position, float3& Velocity, float fRadius, float3& Direction, float fAngle, bool bForceCollision)
//{
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_Position = Position;
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_Velocity = Velocity;
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_Direction = Direction;
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_fAngle = fAngle;
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_fRadius = fRadius;
//	ms_CollisionPartialSpheres[ms_nSphereCount].m_bForceCollision = bForceCollision;
//
//	ms_nSphereCount++;
//}
//
//
