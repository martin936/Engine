#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/SDFGI/SDFGI.h"
#include "Engine/Timer/Timer.h"
#include "SDF.h"


#define MAX_NUM_SDF_TO_BAKE 16


extern bool gs_bEnableDiffuseGI_Saved;
extern bool gs_EnableAO_Saved;


CTexture*			CSDF::ms_pDummyTarget		= nullptr;
std::vector<CSDF*>	CSDF::ms_pSDFToBake[2];
std::vector<CSDF*>* CSDF::ms_pSDFBakeListToFill		= &CSDF::ms_pSDFToBake[0];
std::vector<CSDF*>* CSDF::ms_pSDFBakeListToFlush	= &CSDF::ms_pSDFToBake[1];

std::vector<CSDF*>	CSDF::ms_pSDFToRender[2];
std::vector<CSDF*>* CSDF::ms_pSDFRenderListToFill	= &CSDF::ms_pSDFToRender[0];
std::vector<CSDF*>* CSDF::ms_pSDFRenderListToFlush	= &CSDF::ms_pSDFToRender[1];

BufferId			CSDF::ms_SDFConstantBuffer		= INVALIDHANDLE;
CSDF*				CSDF::ms_pCurrentSDF			= nullptr;


struct SSDFConstantBuffer
{
	float4	m_Center[64];
	float4	m_Size[64];

	int		m_NumSDFs;
};


struct SNarrowFieldsConstants
{
	unsigned int	PositionOffset;
	unsigned int	NormalOffset;
	unsigned int	TexcoordOffset;
	unsigned int	Stride;

	float4			Center;
	float4			Size;
	int				GridSize[4];
};


void CSDF::Init()
{
	ms_pDummyTarget = new CTexture(256, 256, ETextureFormat::e_R8);

	if (CRenderPass::BeginCompute("Bake SDF"))
	{
		// Clear
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::SetNumRWTextures(1, 1);
			CRenderPass::SetNumRWTextures(2, 1);
			CRenderPass::SetNumRWTextures(3, 1);

			CRenderPass::BindProgram("BakeSDF_Clear");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(Clear);

			CRenderPass::EndSubPass();
		}

		// Narrow Fields
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumBuffers(0, 1);
			CRenderPass::SetNumBuffers(1, 1);
			CRenderPass::SetNumRWTextures(2, 1);
			CRenderPass::SetNumRWTextures(3, 1);
			CRenderPass::SetNumRWTextures(4, 1);
			CRenderPass::SetNumRWTextures(5, 1);
			CRenderPass::SetNumTextures(6, 1024);
			CRenderPass::SetNumSamplers(7, 1);

			CRenderPass::BindProgram("BakeSDF_BuildNarrowFields");

			CRenderPass::SetMaxNumVersions(512 * MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(BuildNarrowFields);

			CRenderPass::EndSubPass();
		}

		// Narrow SDF
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::SetNumTextures(0, 1);
			CRenderPass::SetNumTextures(1, 1);
			CRenderPass::SetNumRWTextures(2, 1);

			CRenderPass::BindProgram("BakeSDF_BuildNarrowSDF");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(BuildNarrowSDF);

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
			CRenderPass::SetNumRWTextures(2, 1);

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
	CTimerManager::GetGPUTimer("Bake SDF")->Start();

	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{		 
		CTexture* pTex0 = (*ms_pSDFBakeListToFlush)[i]->m_pExteriorNarrowBand;
		CTexture* pTex1 = (*ms_pSDFBakeListToFlush)[i]->m_pInteriorNarrowBand;
		CTexture* pTex2 = (*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTex[0];

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(pTex1->GetID(), 1, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(pTex2->GetID(), 2, CShader::e_ComputeShader);

		CDeviceManager::Dispatch((pTex0->GetWidth() + 7) / 8, (pTex0->GetHeight() + 7) / 8, (pTex0->GetDepth() + 7) / 8);
	}
}



void CSDF::BuildVoronoi()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTex[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);

	CFrameBlueprint::FlushBarriers();

	for (int i = 0; i < numSDF; i++)
	{
		CSDF* sdf = (*ms_pSDFBakeListToFlush)[i];

		int passIndex = 0;
		int offset;

		sdf->m_nVoronoiIndex = 0;

		do
		{
			offset = (1 << (static_cast<int>(log2(MAX(MAX(sdf->m_pVoronoiTex[0]->GetWidth(), sdf->m_pVoronoiTex[0]->GetHeight()), sdf->m_pVoronoiTex[0]->GetDepth())) - passIndex - 1)));

			CTextureInterface::SetRWTexture(sdf->m_pVoronoiTex[sdf->m_nVoronoiIndex]->GetID(), 0, CShader::e_ComputeShader);
			CTextureInterface::SetRWTexture(sdf->m_pVoronoiTex[(sdf->m_nVoronoiIndex + 1) & 1]->GetID(), 1, CShader::e_ComputeShader);

			float4 constant = float4(sdf->m_Size, 1.f * offset);

			CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constant, sizeof(constant));

			CDeviceManager::Dispatch((sdf->m_pVoronoiTex[0]->GetWidth() + 7) / 8, (sdf->m_pVoronoiTex[0]->GetHeight() + 7) / 8, (sdf->m_pVoronoiTex[0]->GetDepth() + 7) / 8);

			CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTex[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
			CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTex[1]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
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

		CTextureInterface::SetRWTexture(sdf->m_pVoronoiTex[sdf->m_nVoronoiIndex]->GetID(), 0, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pVolumeSDF->GetID(), 1, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pVolumeAlbedo->GetID(), 2, CShader::e_ComputeShader);

		float4 constant = float4(sdf->m_Size, 0.f);

		CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constant, sizeof(constant));

		CDeviceManager::Dispatch((sdf->m_pVolumeSDF->GetWidth() + 7) / 8, (sdf->m_pVolumeSDF->GetHeight() + 7) / 8, (sdf->m_pVolumeSDF->GetDepth() + 7) / 8);

		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeSDF->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_PixelShaderResource, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeAlbedo->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_PixelShaderResource, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();

	CTimerManager::GetGPUTimer("Bake SDF")->Stop();
}


struct SOITSunConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
	float4		m_SunDir;
};


void CSDF::ShowSDF()
{
	CSDF::BindSDFs(0);
	//CSDF::BindVolumeAlbedo(1);
	CResourceManager::SetSampler(2, e_MinMagMip_Linear_UVW_Clamp);
	CSDF::SetSDFConstantBuffer(3);
	CRenderer::SetViewProjConstantBuffer(4);

	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((nWidth + 7) / 8, (nHeight + 7) / 8, 1);
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
	m_Size		= mesh.GetBoundingBox() * float3(1.f + 2.f / numCellX, 1.f + 2.f / numCellY, 1.f + 2.f / numCellZ);

	m_pVolumeSDF			= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_FLOAT, eTextureStorage3D);
	m_pVolumeAlbedo			= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R8G8B8A8,	eTextureStorage3D);
	m_pInteriorNarrowBand	= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);
	m_pExteriorNarrowBand	= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);
	m_pVoronoiTex[0]		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R16G16B16A16_UINT,	eTextureStorage3D);
	m_pVoronoiTex[1]		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R16G16B16A16_UINT,	eTextureStorage3D);

	m_pVolumeSDF->TransitionToState(CRenderPass::e_PixelShaderResource);
	m_pVolumeAlbedo->TransitionToState(CRenderPass::e_PixelShaderResource);
	m_pInteriorNarrowBand->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pExteriorNarrowBand->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pVoronoiTex[0]->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pVoronoiTex[1]->TransitionToState(CRenderPass::e_UnorderedAccess);
}


void CSDF::BuildNarrowFields()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pInteriorNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pExteriorNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();

	CPacketManager::ForceShaderHook(NarrowFieldsUpdateShader);
	CRenderer::DisableViewportCheck();

	for (int i = 0; i < numSDF; i++)
	{
		PacketList* packets = (*ms_pSDFBakeListToFlush)[i]->m_pPacketList;

		if (packets == nullptr)
			continue;

		CRenderer::SShaderData pShaderData;
		pShaderData.m_nInstancedBufferID = INVALIDHANDLE;
		pShaderData.m_nInstancedBufferByteOffset = 0;
		pShaderData.m_nInstancedStreamMask = 0;
		pShaderData.m_nInstancedBufferStride = 0;
		pShaderData.m_nNbInstances = 1;

		ms_pCurrentSDF = (*ms_pSDFBakeListToFlush)[i];

		CRenderer::DrawPacketList(packets, pShaderData, CMaterial::e_Deferred);
	}

	CRenderer::EnableViewportCheck();
	CPacketManager::ForceShaderHook(0);
}


void CSDF::BuildNarrowSDF()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pInteriorNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pExteriorNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();

	for (int i = 0; i < numSDF; i++)
	{
		CTexture* pSDF = (*ms_pSDFBakeListToFlush)[i]->m_pVolumeSDF;

		CTextureInterface::SetTexture((*ms_pSDFBakeListToFlush)[i]->m_pInteriorNarrowBand->GetID(), 0);
		CTextureInterface::SetTexture((*ms_pSDFBakeListToFlush)[i]->m_pExteriorNarrowBand->GetID(), 1);
		CTextureInterface::SetRWTexture(pSDF->GetID(), 2);

		float3 constants = (*ms_pSDFBakeListToFlush)[i]->m_Size;
		CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

		CDeviceManager::Dispatch((pSDF->GetWidth() + 7) / 8, (pSDF->GetHeight() + 7) / 8, (pSDF->GetDepth() + 7) / 8);
	}
}


int CSDF::NarrowFieldsUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	CResourceManager::SetBuffer(0, pShaderData->m_nPacket.m_IndexBuffer);
	CResourceManager::SetBuffer(1, pShaderData->m_nPacket.m_VertexBuffer);
	CTextureInterface::SetRWTexture(ms_pCurrentSDF->m_pExteriorNarrowBand->GetID(), 2);
	CTextureInterface::SetRWTexture(ms_pCurrentSDF->m_pInteriorNarrowBand->GetID(), 3);
	CTextureInterface::SetRWTexture(ms_pCurrentSDF->m_pVoronoiTex[0]->GetID(), 4);
	CTextureInterface::SetRWTexture(ms_pCurrentSDF->m_pVolumeAlbedo->GetID(), 5);

	CMaterial::BindMaterialTextures(6);
	CResourceManager::SetSampler(7, ESamplerState::e_MinMagMip_Linear_UVW_Wrap);

	CMaterial::BindMaterialBuffer(8);

	CMaterial::BindMaterial(8, packet->m_pMaterial->GetID());

	SNarrowFieldsConstants constants;
	constants.Center = ms_pCurrentSDF->m_Center;
	constants.Size = ms_pCurrentSDF->m_Size;
	constants.PositionOffset = 0;
	constants.NormalOffset = 3;
	constants.TexcoordOffset = 16;
	constants.Stride = pShaderData->m_nPacket.m_nStride / sizeof(float);

	constants.Center.w = MIN(constants.Size.x / CSDF::ms_pCurrentSDF->m_nNumCells[0], MIN(constants.Size.y / CSDF::ms_pCurrentSDF->m_nNumCells[1], constants.Size.z / CSDF::ms_pCurrentSDF->m_nNumCells[2]));
	constants.Center.w *= 0.1f * constants.Center.w;

	for (int i = 0; i < 3; i++)
		constants.GridSize[i] = CSDF::ms_pCurrentSDF->m_nNumCells[i];

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constants, sizeof(constants));

	CDeviceManager::Dispatch(pShaderData->m_nPacket.m_nNumIndex / 3, 1, 1);

	return -1;
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


void CSDF::BindVolumeAlbedo(unsigned int nSlot)
{
	std::vector<CSDF*>& SDFs = *ms_pSDFRenderListToFlush;

	int numSDFs = static_cast<int>(SDFs.size());

	std::vector<CTexture*> pTextures(numSDFs);

	for (int i = 0; i < numSDFs; i++)
		pTextures[i] = SDFs[i]->m_pVolumeAlbedo;

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
