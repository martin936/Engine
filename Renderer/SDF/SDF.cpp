#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Timer/Timer.h"
#include "SDF.h"


#define MAX_NUM_SDF_TO_BAKE 16


CTexture*			CSDF::ms_pDummyTarget		= nullptr;
std::vector<CSDF*>	CSDF::ms_pSDFToBake[2];
std::vector<CSDF*>* CSDF::ms_pSDFBakeListToFill		= &CSDF::ms_pSDFToBake[0];
std::vector<CSDF*>* CSDF::ms_pSDFBakeListToFlush	= &CSDF::ms_pSDFToBake[1];

std::vector<CSDF*>	CSDF::ms_pSDFToRender[2];
std::vector<CSDF*>* CSDF::ms_pSDFRenderListToFill	= &CSDF::ms_pSDFToRender[0];
std::vector<CSDF*>* CSDF::ms_pSDFRenderListToFlush	= &CSDF::ms_pSDFToRender[1];

BufferId			CSDF::ms_SDFConstantBuffer		= INVALIDHANDLE;

float3 CSDF::ms_CurrentCenter	= float3(0.f, 0.f, 0.f);
float3 CSDF::ms_CurrentSize		= float3(0.f, 0.f, 0.f);


struct SSDFConstantBuffer
{
	float4	m_Center[64];
	float4	m_Size[64];

	int		m_NumSDFs;
};


void CSDF::Init()
{
	ms_pDummyTarget = new CTexture(1024, 1024, ETextureFormat::e_R8);

	if (CRenderPass::BeginCompute("Bake SDF"))
	{
		// Clear
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);

			CRenderPass::BindProgram("BakeSDF_Clear");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(Clear);

			CRenderPass::EndSubPass();
		}

		// Insert Points
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::BindResourceToWrite(0, ms_pDummyTarget->GetID(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("BakeSDF_InsertPoints", "BakeSDF_InsertPoints");

			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, true);

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(InsertPoints);

			CRenderPass::EndSubPass();
		}

		// Build Voronoi
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::SetNumRWTextures(1, 1);

			CRenderPass::BindProgram("BakeSDF_BuildVoronoi");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(BuildVoronoi);

			CRenderPass::EndSubPass();
		}

		// Build SDF
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::SetNumRWTextures(1, 1);

			CRenderPass::BindProgram("BakeSDF_BuildSDF");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(BuildSDF);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CSDF::Clear()
{
	//CTimerManager::GetGPUTimer("Bake SDF")->Start();

	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeSDF->GetID(), CRenderPass::e_PixelShaderResource, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);

	if (numSDF > 0)
		CFrameBlueprint::FlushBarriers();

	for (int i = 0; i < numSDF; i++)
	{
		CTexture* pTex = (*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0];

		CTextureInterface::SetRWTexture(pTex->GetID(), 0, CShader::e_ComputeShader);

		CDeviceManager::Dispatch((pTex->GetWidth() + 7) / 8, (pTex->GetHeight() + 7) / 8, (pTex->GetDepth() + 7) / 8);
	}
}


void CSDF::InsertPoints()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);

	CFrameBlueprint::FlushBarriers();

	CPacketManager::ForceShaderHook(UpdateShader);
	CRenderer::DisableViewportCheck();

	for (int i = 0; i < numSDF; i++)
	{
		PacketList* packets = (*ms_pSDFBakeListToFlush)[i]->m_pPacketList;

		if (packets == nullptr)
			continue;

		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), 0);

		CRenderer::SShaderData pShaderData;
		pShaderData.m_nInstancedBufferID			= INVALIDHANDLE;
		pShaderData.m_nInstancedBufferByteOffset	= 0;
		pShaderData.m_nInstancedStreamMask			= 0;
		pShaderData.m_nInstancedBufferStride		= 0;
		pShaderData.m_nNbInstances					= 3;

		ms_CurrentCenter	= (*ms_pSDFBakeListToFlush)[i]->m_Center;
		ms_CurrentSize		= (*ms_pSDFBakeListToFlush)[i]->m_Size;

		CRenderer::DrawPacketList(packets, pShaderData, CMaterial::e_Deferred);
	}

	CRenderer::EnableViewportCheck();
	CPacketManager::ForceShaderHook(0);
}


void CSDF::BuildVoronoi()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);

	CFrameBlueprint::FlushBarriers();

	for (int i = 0; i < numSDF; i++)
	{
		CSDF* sdf = (*ms_pSDFBakeListToFlush)[i];

		int passIndex = 0;
		int offset;

		sdf->m_nVoronoiIndex = 0;

		do
		{
			offset = (1 << (static_cast<int>(log2(MAX(sdf->m_pVoronoiTiling[0]->GetWidth(), sdf->m_pVoronoiTiling[0]->GetHeight())) - passIndex - 1)));

			CTextureInterface::SetRWTexture(sdf->m_pVoronoiTiling[sdf->m_nVoronoiIndex]->GetID(), 0, CShader::e_ComputeShader);
			CTextureInterface::SetRWTexture(sdf->m_pVoronoiTiling[(sdf->m_nVoronoiIndex + 1) & 1]->GetID(), 1, CShader::e_ComputeShader);

			float4 constant = float4(sdf->m_Size, offset);

			CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constant, sizeof(constant));

			CDeviceManager::Dispatch((sdf->m_pVoronoiTiling[0]->GetWidth() + 7) / 8, (sdf->m_pVoronoiTiling[0]->GetHeight() + 7) / 8, (sdf->m_pVoronoiTiling[0]->GetDepth() + 7) / 8);

			CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[(sdf->m_nVoronoiIndex + 1) & 1]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
			CFrameBlueprint::FlushBarriers();

			sdf->m_nVoronoiIndex = (sdf->m_nVoronoiIndex + 1) & 1;
			passIndex++;

		} while (offset > 1);
	}
}


void CSDF::BuildSDF()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{
		CSDF* sdf = (*ms_pSDFBakeListToFlush)[i];

		CTextureInterface::SetRWTexture(sdf->m_pVoronoiTiling[sdf->m_nVoronoiIndex]->GetID(), 0, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pVolumeSDF->GetID(), 1, CShader::e_ComputeShader);

		float4 constant = float4(sdf->m_Size, 0.f);

		CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constant, sizeof(constant));

		CDeviceManager::Dispatch((sdf->m_pVolumeSDF->GetWidth() + 7) / 8, (sdf->m_pVolumeSDF->GetHeight() + 7) / 8, (sdf->m_pVolumeSDF->GetDepth() + 7) / 8);

		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeSDF->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_PixelShaderResource, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();

	//CTimerManager::GetGPUTimer("Bake SDF")->Stop();
}


CSDF::CSDF(CMesh& mesh, int numCellX, int numCellY, int numCellZ)
{
	m_nNumCells[0] = numCellX;
	m_nNumCells[1] = numCellY;
	m_nNumCells[2] = numCellZ;

	m_nVoronoiIndex = 0;
	m_bIsReady = false;

	m_pPacketList = reinterpret_cast<PacketList*>(mesh.GetPacketList());
	m_Center	= mesh.GetCenter();
	m_Size		= mesh.GetBoundingBox();

	m_pVolumeSDF		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R16_FLOAT, eTextureStorage3D);
	m_pVoronoiTiling[0] = new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32G32B32A32_FLOAT, eTextureStorage3D);
	m_pVoronoiTiling[1] = new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32G32B32A32_FLOAT, eTextureStorage3D);

	m_pVolumeSDF->TransitionToState(CRenderPass::e_PixelShaderResource);
	m_pVoronoiTiling[0]->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pVoronoiTiling[1]->TransitionToState(CRenderPass::e_UnorderedAccess);
}


int CSDF::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	float4 constants[2];
	constants[0] = ms_CurrentCenter;
	constants[1] = ms_CurrentSize;

	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_FragmentShader, constants, sizeof(constants));

	return 1;
}


void CSDF::Bake()
{
	ms_pSDFBakeListToFill->push_back(this);
	m_bIsReady = true;
}


void CSDF::Render()
{
	ms_pSDFRenderListToFill->push_back(this);
}


void CSDF::BindSDFs(unsigned int nSlot)
{
	std::vector<CSDF*>& SDFs = *ms_pSDFRenderListToFlush;

	int numSDFs = static_cast<int>(SDFs.size());

	std::vector<CTexture*> pTextures(numSDFs);

	for (int i = 0; i < numSDFs; i++)
		pTextures[i] = SDFs[i]->m_pVolumeSDF;

	CResourceManager::SetTextures(nSlot, pTextures);
}


void CSDF::SetSDFConstantBuffer(unsigned int nSlot)
{
	CResourceManager::SetConstantBuffer(nSlot, ms_SDFConstantBuffer);
}


void CSDF::UpdateBeforeFlush()
{
	std::vector<CSDF*>* tmp		= ms_pSDFBakeListToFill;
	ms_pSDFBakeListToFill		= ms_pSDFBakeListToFlush;
	ms_pSDFBakeListToFlush		= tmp;

	ms_pSDFBakeListToFill->clear();

	tmp = ms_pSDFRenderListToFill;
	ms_pSDFRenderListToFill		= ms_pSDFRenderListToFlush;
	ms_pSDFRenderListToFlush	= tmp;

	ms_pSDFRenderListToFill->clear();

	SSDFConstantBuffer constants;
	constants.m_NumSDFs = static_cast<int>(ms_pSDFRenderListToFlush->size());

	for (int i = 0; i < constants.m_NumSDFs; i++)
	{
		constants.m_Center[i]	= (*ms_pSDFRenderListToFlush)[i]->m_Center;
		constants.m_Size[i]		= (*ms_pSDFRenderListToFlush)[i]->m_Size;
	}

	if (ms_SDFConstantBuffer == INVALIDHANDLE)
		ms_SDFConstantBuffer = CResourceManager::CreateFrameConstantBuffer(&constants, sizeof(constants));
	else
		CResourceManager::UpdateFrameConstantBuffer(ms_SDFConstantBuffer, &constants);
}
