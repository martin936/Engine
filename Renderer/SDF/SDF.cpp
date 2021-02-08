#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/Skybox/Skybox.h"
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

float3	CSDF::ms_CurrentCenter			= float3(0.f, 0.f, 0.f);
float3	CSDF::ms_CurrentSize			= float3(0.f, 0.f, 0.f);
int		CSDF::ms_CurrentProjectionAxis	= 0;


struct SSDFConstantBuffer
{
	float4	m_Center[64];
	float4	m_Size[64];

	int		m_NumSDFs;
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

			CRenderPass::BindProgram("BakeSDF_Clear");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(Clear);

			CRenderPass::EndSubPass();
		}

		// Narrow UDF
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::BindResourceToWrite(0, ms_pDummyTarget->GetID(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("BakeSDF_NarrowUDF", "BakeSDF_NarrowUDF", "BakeSDF_NarrowUDF");

			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false);

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(ComputeNarrowUDF);

			CRenderPass::EndSubPass();
		}

		// Compute Sign And Seed Positions
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::SetNumRWTextures(0, 1);
			CRenderPass::SetNumRWTextures(1, 1);
			CRenderPass::SetNumRWTextures(2, 1);
			CRenderPass::SetNumRWTextures(3, 1);
			CRenderPass::SetNumTextures(4, 1024);
			CRenderPass::SetNumSamplers(5, 1);
			CRenderPass::BindResourceToWrite(0, ms_pDummyTarget->GetID(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("BakeSDF_ComputeSeeds", "BakeSDF_ComputeSeeds", "BakeSDF_ComputeSeeds");

			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false);

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(ComputeSeeds);

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
			CRenderPass::SetNumRWTextures(3, 1);
			CRenderPass::SetNumRWTextures(4, 1);

			CRenderPass::BindProgram("BakeSDF_BuildSDF");

			CRenderPass::SetMaxNumVersions(MAX_NUM_SDF_TO_BAKE);

			CRenderPass::SetEntryPoint(BuildSDF);

			CRenderPass::EndSubPass();
		}


		CRenderPass::End();
	}

	if (CRenderPass::BeginCompute("Show SDF"))
	{
		CRenderPass::BindResourceToRead(0, CLightsManager::GetLightGridTexture(), CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, CLightsManager::GetLightIndexBuffer(), CShader::e_FragmentShader, CRenderPass::e_Buffer);
		CRenderPass::BindResourceToRead(2, CShadowRenderer::GetShadowmapArray(), CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(3, CShadowRenderer::GetSunShadowmapArray(), CShader::e_FragmentShader);
		CRenderPass::SetNumSamplers(4, 1);
		CRenderPass::BindResourceToRead(5, CLightField::GetIrradianceField(0), CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(6, CLightField::GetProbeMetadata(0), CShader::e_FragmentShader);
		CRenderPass::SetNumTextures(7, 1024);
		CRenderPass::SetNumTextures(8, 1024);
		CRenderPass::SetNumSamplers(9, 1);

		CRenderPass::BindResourceToWrite(10, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_UnorderedAccess);

		CRenderPass::BindProgram("ShowSDF");

		CRenderPass::SetEntryPoint(ShowSDF);

		CRenderPass::End();
	}
}


void CSDF::Clear()
{
	CTimerManager::GetGPUTimer("Bake SDF")->Start();

	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeSDF->GetID(), CRenderPass::e_PixelShaderResource, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVolumeAlbedo->GetID(), CRenderPass::e_PixelShaderResource, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
	}

	if (numSDF > 0)
		CFrameBlueprint::FlushBarriers();

	for (int i = 0; i < numSDF; i++)
	{
		CTexture* pTex0 = (*ms_pSDFBakeListToFlush)[i]->m_pNarrowBand;
		CTexture* pTex1 = (*ms_pSDFBakeListToFlush)[i]->m_pNarrowBandSign;
		CTexture* pTex2 = (*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0];

		CTextureInterface::SetRWTexture(pTex0->GetID(), 0, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(pTex1->GetID(), 1, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(pTex2->GetID(), 2, CShader::e_ComputeShader);

		CDeviceManager::Dispatch((pTex0->GetWidth() + 7) / 8, (pTex0->GetHeight() + 7) / 8, (pTex0->GetDepth() + 7) / 8);
	}
}


void CSDF::ComputeNarrowUDF()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);

	CFrameBlueprint::FlushBarriers();

	CPacketManager::ForceShaderHook(NarrowUDFUpdateShader);
	CRenderer::DisableViewportCheck();

	for (int i = 0; i < numSDF; i++)
	{
		PacketList* packets = (*ms_pSDFBakeListToFlush)[i]->m_pPacketList;

		if (packets == nullptr)
			continue;

		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pNarrowBand->GetID(), 0);

		CRenderer::SShaderData pShaderData;
		pShaderData.m_nInstancedBufferID			= INVALIDHANDLE;
		pShaderData.m_nInstancedBufferByteOffset	= 0;
		pShaderData.m_nInstancedStreamMask			= 0;
		pShaderData.m_nInstancedBufferStride		= 0;
		pShaderData.m_nNbInstances					= 9;

		ms_CurrentCenter	= (*ms_pSDFBakeListToFlush)[i]->m_Center;
		ms_CurrentSize		= (*ms_pSDFBakeListToFlush)[i]->m_Size;

		CRenderer::DrawPacketList(packets, pShaderData, CMaterial::e_Deferred | CMaterial::e_Forward);
	}

	CRenderer::EnableViewportCheck();
	CPacketManager::ForceShaderHook(0);
}


void CSDF::ComputeSeeds()
{
	int numSDF = MIN(MAX_NUM_SDF_TO_BAKE, static_cast<int>(ms_pSDFBakeListToFlush->size()));

	for (int i = 0; i < numSDF; i++)
	{
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pNarrowBand->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
		CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
	}

	CFrameBlueprint::FlushBarriers();

	CPacketManager::ForceShaderHook(SeedsUpdateShader);
	CRenderer::DisableViewportCheck();

	for (int i = 0; i < numSDF; i++)
	{
		PacketList* packets = (*ms_pSDFBakeListToFlush)[i]->m_pPacketList;

		if (packets == nullptr)
			continue;

		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pNarrowBand->GetID(), 0);
		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pNarrowBandSign->GetID(), 1);
		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), 2);
		CTextureInterface::SetRWTexture((*ms_pSDFBakeListToFlush)[i]->m_pVolumeAlbedo->GetID(), 3);
		CMaterial::BindMaterialTextures(4);
		CResourceManager::SetSampler(5, e_Anisotropic_Linear_UVW_Wrap);
		CMaterial::BindMaterialBuffer(6);

		CRenderer::SShaderData pShaderData;
		pShaderData.m_nInstancedBufferID			= INVALIDHANDLE;
		pShaderData.m_nInstancedBufferByteOffset	= 0;
		pShaderData.m_nInstancedStreamMask			= 0;
		pShaderData.m_nInstancedBufferStride		= 0;
		pShaderData.m_nNbInstances					= 9;

		ms_CurrentCenter	= (*ms_pSDFBakeListToFlush)[i]->m_Center;
		ms_CurrentSize		= (*ms_pSDFBakeListToFlush)[i]->m_Size;

		CRenderer::DrawPacketList(packets, pShaderData, CMaterial::e_Deferred | CMaterial::e_Forward);
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

			float4 constant = float4(sdf->m_Size, 1.f * offset);

			CResourceManager::SetPushConstant(CShader::e_ComputeShader, &constant, sizeof(constant));

			CDeviceManager::Dispatch((sdf->m_pVoronoiTiling[0]->GetWidth() + 7) / 8, (sdf->m_pVoronoiTiling[0]->GetHeight() + 7) / 8, (sdf->m_pVoronoiTiling[0]->GetDepth() + 7) / 8);

			CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[0]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
			CFrameBlueprint::TransitionBarrier((*ms_pSDFBakeListToFlush)[i]->m_pVoronoiTiling[1]->GetID(), CRenderPass::e_UnorderedAccess, CRenderPass::e_UnorderedAccess, CFrameBlueprint::e_Immediate);
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
		CTextureInterface::SetRWTexture(sdf->m_pNarrowBand->GetID(), 1, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pNarrowBandSign->GetID(), 2, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pVolumeSDF->GetID(), 3, CShader::e_ComputeShader);
		CTextureInterface::SetRWTexture(sdf->m_pVolumeAlbedo->GetID(), 4, CShader::e_ComputeShader);

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
	CResourceManager::SetSampler(4, e_ZComparison_Linear_UVW_Clamp);
	CSDF::BindSDFs(7);
	CSDF::BindVolumeAlbedo(8);
	CResourceManager::SetSampler(9, e_MinMagMip_Linear_UVW_Clamp);
	CRenderer::SetViewProjConstantBuffer(11);
	CLightsManager::SetLightListConstantBuffer(12);
	CLightsManager::SetShadowLightListConstantBuffer(13);
	CSDF::SetSDFConstantBuffer(14);

	SOITSunConstants sunConstants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		sunConstants.m_ShadowMatrix.transpose();
		sunConstants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
		sunConstants.m_SunDir = float4(desc.m_Dir, 0.f);
	}

	else
		sunConstants.m_SunColor = 0.f;

	CResourceManager::SetConstantBuffer(15, &sunConstants, sizeof(sunConstants));

	float4 constants[3];
	constants[0] = CLightField::GetCenter(0);
	constants[0].w = CRenderer::GetNear4EngineFlush();
	constants[1] = CLightField::GetSize(0);
	constants[1].w = CRenderer::GetFar4EngineFlush();
	constants[2].x = gs_bEnableDiffuseGI_Saved ? 1.f : 0.f;
	constants[2].y = gs_EnableAO_Saved ? 1.f : 0.f;
	constants[2].z = CSkybox::GetSkyLightIntensity();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constants, sizeof(constants));

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

	m_pVolumeSDF		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R16_FLOAT, eTextureStorage3D);
	m_pVolumeAlbedo		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R8G8B8A8,	eTextureStorage3D);
	m_pNarrowBand		= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);
	m_pNarrowBandSign	= new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);
	m_pVoronoiTiling[0] = new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);
	m_pVoronoiTiling[1] = new CTexture(m_nNumCells[0], m_nNumCells[1], m_nNumCells[2], ETextureFormat::e_R32_UINT,	eTextureStorage3D);

	m_pVolumeSDF->TransitionToState(CRenderPass::e_PixelShaderResource);
	m_pVolumeAlbedo->TransitionToState(CRenderPass::e_PixelShaderResource);
	m_pNarrowBand->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pNarrowBandSign->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pVoronoiTiling[0]->TransitionToState(CRenderPass::e_UnorderedAccess);
	m_pVoronoiTiling[1]->TransitionToState(CRenderPass::e_UnorderedAccess);
}


int CSDF::NarrowUDFUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	float4 constants[3];
	constants[0] = ms_CurrentCenter;
	constants[1] = ms_CurrentSize;
	constants[2] = 128.f;

	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_GeometryShader | CShader::e_FragmentShader, constants, sizeof(constants));

	return 1;
}


int CSDF::SeedsUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	float4 constants[3];
	constants[0] = ms_CurrentCenter;
	constants[1] = ms_CurrentSize;
	constants[2] = 128.f;

	constants[2].w = packet->m_pMaterial->GetRenderType() == CMaterial::e_Forward ? 1.f : 0.f;

	CResourceManager::SetPushConstant(CShader::e_VertexShader | CShader::e_GeometryShader | CShader::e_FragmentShader, constants, sizeof(constants));

	CMaterial::BindMaterial(6, packet->m_pMaterial->GetID());

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
