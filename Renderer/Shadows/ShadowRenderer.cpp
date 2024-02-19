#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "ShadowRenderer.h"


CTexture*				CShadowRenderer::ms_pShadowCubeMapArray			= nullptr;
CTexture*				CShadowRenderer::ms_pShadowMapArray				= nullptr;
CTexture*				CShadowRenderer::ms_pFilteredShadowArray		= nullptr;
CTexture*				CShadowRenderer::ms_pShadowsHiZ					= nullptr;

CTexture*				CShadowRenderer::ms_pSunShadowMaps				= nullptr;
CTexture*				CShadowRenderer::ms_pSunShadowsHiZ				= nullptr;

int						CShadowRenderer::ms_nMaxStaticShadowmaps		= 4;
int						CShadowRenderer::ms_nMaxDynamicShadowmaps		= 4;
int						CShadowRenderer::ms_nNumDynamicShadowmaps		= 0;
int						CShadowRenderer::ms_nNumStaticShadowmaps		= 0;
int						CShadowRenderer::ms_nNumStaticShadowmapsInFrame = 0;
int						CShadowRenderer::ms_nShadowmapSize				= 1024;
int						CShadowRenderer::ms_nSunShadowmapSize			= 4096;
bool					CShadowRenderer::ms_bAreStaticSMUpdated			= false;

int						CShadowRenderer::ms_nLightIndexArray[MAX_SHADOWS_PER_FRAME];
float					CShadowRenderer::ms_fLastActualizationTime[MAX_SHADOWS_PER_FRAME];
bool					CShadowRenderer::ms_bUsedInThisFrame[MAX_SHADOWS_PER_FRAME];
bool					CShadowRenderer::ms_bShadowmapCleared[MAX_SHADOWS_PER_FRAME];
bool					CShadowRenderer::ms_bShouldUpdateHiZ[MAX_SHADOWS_PER_FRAME * 2] = { false };

bool					g_OmniShadows = false;

unsigned int			g_ShadowMapsCommandList = 0;

std::vector<CShadowRenderer::SViewportAssociation> CShadowRenderer::ms_ViewportsToUpdate[2];
std::vector<CShadowRenderer::SViewportAssociation>* CShadowRenderer::ms_ViewportsToUpdateToFill		= &CShadowRenderer::ms_ViewportsToUpdate[0];
std::vector<CShadowRenderer::SViewportAssociation>* CShadowRenderer::ms_ViewportsToUpdateToFlush	= &CShadowRenderer::ms_ViewportsToUpdate[1];

void				ComputeShadowMaps_EntryPoint();
void				ComputeShadowMapsAlpha_EntryPoint();
void				ComputeOmniShadowMaps_EntryPoint();
void				ComputeOmniShadowMapsAlpha_EntryPoint();
void				ComputeShadowsHiZ_EntryPoint();
void				CubeToOctahedron_EntryPoint();


void CShadowRenderer::Init()
{
	unsigned int nWidth		= CDeviceManager::GetDeviceWidth();
	unsigned int nHeight	= CDeviceManager::GetDeviceHeight();

	ms_pShadowMapArray		= new CTexture(ms_nShadowmapSize, ms_nShadowmapSize, ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps, ETextureFormat::e_R32_DEPTH_G8_STENCIL, eTextureArray);

	ms_pSunShadowMaps		= new CTexture(ms_nSunShadowmapSize, ms_nSunShadowmapSize, ETextureFormat::e_R32_DEPTH_G8_STENCIL);
	ms_pSunShadowsHiZ		= new CTexture((ms_nSunShadowmapSize + 15) / 16, (ms_nSunShadowmapSize + 15) / 16, 2, ETextureFormat::e_R32_UINT, eTextureStorage2DArray);

	ms_pShadowCubeMapArray	= new CTexture(ms_nShadowmapSize / 4, ms_nShadowmapSize / 4, ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps, ETextureFormat::e_R32_DEPTH_G8_STENCIL, eCubeMapArray);

	ms_pFilteredShadowArray = new CTexture(nWidth, nHeight, MAX(ms_nMaxStaticShadowmaps, ms_nMaxDynamicShadowmaps) + 1, ETextureFormat::e_R8, eTextureStorage2DArray);
	
	ms_nNumDynamicShadowmaps		= 0;
	ms_nNumStaticShadowmaps			= 0;
	ms_nNumStaticShadowmapsInFrame	= 0;

	CShadowSpot::ms_pShadowMatrices[0].resize(ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps);
	CShadowSpot::ms_pShadowMatrices[1].resize(ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps);

	CShadowOmni::ms_pShadowMatrices[0].resize(6 * (ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps));
	CShadowOmni::ms_pShadowMatrices[1].resize(6 * (ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps));

	CShadowOmni::ms_pShadowPos[0].resize(ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps);
	CShadowOmni::ms_pShadowPos[1].resize(ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps);

	ms_ViewportsToUpdate[0].clear();
	ms_ViewportsToUpdate[1].clear();

	for (int i = 0; i < ms_nMaxStaticShadowmaps; i++)
		ms_nLightIndexArray[i] = -1;

	if (!g_ShadowMapsCommandList)
		g_ShadowMapsCommandList = CCommandListManager::CreateCommandList(CCommandListManager::e_Direct);


	if (CRenderPass::BeginGraphics(ERenderPassId::e_Sun_Shadow, "Sun Shadow Map"))
	{
		if (CRenderPass::BeginGraphicsSubPass("Opaque"))
		{
			CRenderPass::BindDepthStencil(ms_pSunShadowMaps->GetID());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("SunShadowMap", "ShadowMap");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, true, 1.f / 65536.f, -4.f);

			CRenderPass::SetEntryPoint(RenderSunShadowMaps);

			CRenderPass::EndSubPass();
		}

		if (CRenderPass::BeginGraphicsSubPass("Alpha"))
		{
			CRenderPass::SetNumTextures(0, 1024);
			CRenderPass::SetNumSamplers(1, 1);

			CRenderPass::BindDepthStencil(ms_pSunShadowMaps->GetID());

			CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

			CRenderPass::BindProgram("SunShadowMapAlpha", "ShadowMapAlpha");

			CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, true, 1.f / 65536.f, -4.f);

			CRenderPass::SetEntryPoint(RenderSunShadowMapsAlpha);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}


	//if (CRenderPass::BeginGraphics("Compute Shadow Maps"))
	//{
	//	// Spot Shadows
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindDepthStencil(ms_pShadowMapArray->GetID());

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	//		CRenderPass::BindProgram("ShadowMap", "ShadowMap", "ShadowMap");

	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_CW, false, false, false, 1.f / 65536.f, -4.f);

	//		CRenderPass::SetEntryPoint(ComputeShadowMaps_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Spot Shadows Alpha
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::SetNumTextures(1, 1024);
	//		CRenderPass::SetNumSamplers(2, 1);

	//		CRenderPass::BindDepthStencil(ms_pShadowMapArray->GetID());

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	//		CRenderPass::BindProgram("ShadowMap", "ShadowMap", "ShadowMapAlpha");

	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_CW, false, false, false, 1.f/ 65536.f, -4.f);

	//		CRenderPass::SetEntryPoint(ComputeShadowMapsAlpha_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Omni Shadows
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindDepthStencil(ms_pShadowCubeMapArray->GetID());
	//	
	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);
	//	
	//		CRenderPass::BindProgram("ShadowMap", "ShadowMap", "ShadowMapOmni");
	//	
	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, false, 1.f / 65536.f, -4.f);
	//	
	//		CRenderPass::SetEntryPoint(ComputeOmniShadowMaps_EntryPoint);
	//	
	//		CRenderPass::EndSubPass();
	//	}

	//	// Omni Shadows Alpha
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::SetNumTextures(1, 1024);
	//		CRenderPass::SetNumSamplers(2, 1);

	//		CRenderPass::BindDepthStencil(ms_pShadowCubeMapArray->GetID());
	//	
	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);
	//	
	//		CRenderPass::BindProgram("ShadowMap", "ShadowMap", "ShadowMapOmniAlpha");
	//	
	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, true);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, false, 1.f / 65536.f, -4.f);
	//	
	//		CRenderPass::SetEntryPoint(ComputeOmniShadowMapsAlpha_EntryPoint);
	//	
	//		CRenderPass::EndSubPass();
	//	}

	//	// Project omni to octahedron map
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0, ms_pShadowCubeMapArray->GetID(), CShader::e_FragmentShader);
	//		CRenderPass::BindDepthStencil(ms_pShadowMapArray->GetID());
	//	
	//		CRenderPass::SetNumSamplers(1, 1);
	//	
	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);
	//	
	//		CRenderPass::BindProgram("CubeToOctahedron", "CubeToOctahedron", "CubeToOctahedron");
	//	
	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_Always, true);
	//	
	//		CRenderPass::SetEntryPoint(CubeToOctahedron_EntryPoint);
	//	
	//		CRenderPass::EndSubPass();
	//	}

	//	// Compute HiZ
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0, ms_pShadowMapArray->GetID(), CShader::e_ComputeShader);
	//		CRenderPass::BindResourceToWrite(1, ms_pShadowsHiZ->GetID(),	CRenderPass::e_UnorderedAccess);

	//		CRenderPass::BindProgram("ComputeShadowsHiZ");

	//		CRenderPass::SetEntryPoint(ComputeShadowsHiZ_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	CRenderPass::End();
	//}
}



void CShadowRenderer::RenderSunShadowMaps()
{
	CDeviceManager::ClearDepthStencil(0.f);

	CPacketManager::ForceShaderHook(CShadowDir::UpdateShader);

	CRenderer::DisableViewportCheck();

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}



void CShadowRenderer::RenderSunShadowMapsAlpha()
{
	CPacketManager::ForceShaderHook(CShadowDir::UpdateShader);

	CRenderer::DisableViewportCheck();

	CMaterial::BindMaterialTextures(0);
	CResourceManager::SetSampler(1, e_Anisotropic_Linear_UVW_Wrap);
	CMaterial::BindMaterialBuffer(2);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}



void ComputeShadowMaps_EntryPoint()
{

	std::vector<unsigned int> slicesToClear;

	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (!CShadowRenderer::IsShadowViewportOmni4EngineFlush(i))
		{
			int staticIndex = CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
			int dynamicIndex = CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

			if (staticIndex >= 0)
				slicesToClear.push_back(staticIndex);

			if (dynamicIndex >= 0)
				slicesToClear.push_back(dynamicIndex);
		}
	}

	if (slicesToClear.size() == 0)
		return;

	g_OmniShadows = false;

	CDeviceManager::ClearDepth(slicesToClear, 0.f);

	CPacketManager::ForceShaderHook(CShadowRenderer::UpdateShader);

	CRenderer::DisableViewportCheck();

	CResourceManager::SetConstantBuffer(0, CShadowSpot::GetShadowMatricesConstantBuffer());

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}


void ComputeShadowMapsAlpha_EntryPoint()
{
	bool bRender = false;

	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (!CShadowRenderer::IsShadowViewportOmni4EngineFlush(i))
		{
			int staticIndex = CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
			int dynamicIndex = CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

			if (staticIndex >= 0 || dynamicIndex >= 0)
				bRender = true;
		}
	}

	if (!bRender)
		return;

	CPacketManager::ForceShaderHook(CShadowRenderer::UpdateShader);

	CRenderer::DisableViewportCheck();

	CResourceManager::SetConstantBuffer(0, CShadowSpot::GetShadowMatricesConstantBuffer());
	CMaterial::BindMaterialTextures(1);
	CResourceManager::SetSampler(2, e_Anisotropic_Linear_UVW_Wrap);
	CMaterial::BindMaterialBuffer(3);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}


void ComputeOmniShadowMaps_EntryPoint()
{
	std::vector<unsigned int> slicesToClear;

	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (CShadowRenderer::IsShadowViewportOmni4EngineFlush(i))
		{
			int staticIndex		= CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
			int dynamicIndex	= CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

			if (staticIndex >= 0)
			{
				for (int j = 0; j < 6; j++)
					slicesToClear.push_back(staticIndex * 6 + j);
			}

			if (dynamicIndex >= 0)
			{
				for (int j = 0; j < 6; j++)
					slicesToClear.push_back(dynamicIndex);
			}
		}
	}

	if (slicesToClear.size() == 0)
		return;

	g_OmniShadows = true;

	CDeviceManager::ClearDepth(slicesToClear, 0.f);

	CPacketManager::ForceShaderHook(CShadowRenderer::UpdateShader);

	CRenderer::DisableViewportCheck();

	CResourceManager::SetConstantBuffer(0, CShadowOmni::GetShadowMatricesConstantBuffer());
	CResourceManager::SetConstantBuffer(1, CShadowOmni::GetShadowPosConstantBuffer());

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}


void ComputeOmniShadowMapsAlpha_EntryPoint()
{
	bool bRender = false;

	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (CShadowRenderer::IsShadowViewportOmni4EngineFlush(i))
		{
			int staticIndex = CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
			int dynamicIndex = CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

			if (staticIndex >= 0 || dynamicIndex >= 0)
				bRender = true;
		}
	}

	if (!bRender)
		return;

	CPacketManager::ForceShaderHook(CShadowRenderer::UpdateShader);

	CRenderer::DisableViewportCheck();

	CResourceManager::SetConstantBuffer(0, CShadowOmni::GetShadowMatricesConstantBuffer());
	CMaterial::BindMaterialTextures(1);
	CResourceManager::SetSampler(2, e_Anisotropic_Linear_UVW_Wrap);
	CMaterial::BindMaterialBuffer(3);
	CResourceManager::SetConstantBuffer(4, CShadowOmni::GetShadowPosConstantBuffer());

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CRenderer::EnableViewportCheck();

	CPacketManager::ForceShaderHook(0);
}


void CubeToOctahedron_EntryPoint()
{
	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();
	int numSlices = 0;
	unsigned int viewportMask = 0;
	bool usedIndex[32] = { false };

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (!CShadowRenderer::IsShadowViewportOmni4EngineFlush(i))
			continue;

		int staticIndex = CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
		int dynamicIndex = CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

		if (staticIndex >= 0 && !usedIndex[staticIndex])
		{
			usedIndex[dynamicIndex] = true;
			viewportMask |= 1 << (staticIndex);
			numSlices++;
		}

		if (dynamicIndex >= 0 && !usedIndex[dynamicIndex])
		{
			usedIndex[dynamicIndex] = true;
			viewportMask |= 1 << (dynamicIndex);
			numSlices++;
		}
	}

	if (numSlices == 0)
		return;

	CResourceManager::SetSampler(1, e_MinMagMip_Linear_UVW_Clamp);

	CResourceManager::SetPushConstant(CShader::e_VertexShader, &viewportMask, sizeof(unsigned int));

	CRenderer::RenderQuadScreen(numSlices);
}


void ComputeShadowsHiZ_EntryPoint()
{
	int indices[32] = { 0 };
	bool usedIndex[32] = { false };

	unsigned int numViewports = CShadowRenderer::GetNumShadowViewports4EngineFlush();
	int numSlices = 0;

	for (unsigned int i = 0; i < numViewports; i++)
	{
		int staticIndex = CShadowRenderer::GetShadowViewportStaticIndex4EngineFlush(i);
		int dynamicIndex = CShadowRenderer::GetShadowViewportDynamicIndex4EngineFlush(i);

		if (staticIndex >= 0 && !usedIndex[staticIndex])
		{
			usedIndex[staticIndex] = true;
			indices[numSlices] = staticIndex;
			numSlices++;
		}

		if (dynamicIndex >= 0 && !usedIndex[dynamicIndex])
		{
			usedIndex[dynamicIndex] = true;
			indices[numSlices] = dynamicIndex;
			numSlices++;
		}
	}

	if (numSlices == 0)
	{
		CTimerManager::GetGPUTimer("Shadow Maps")->Stop();
		return;
	}

	CResourceManager::SetConstantBuffer(2, indices, sizeof(indices));

	CDeviceManager::Dispatch((CShadowRenderer::GetShadowmapSize() + 15) / 16, (CShadowRenderer::GetShadowmapSize() + 15) / 16, numSlices);

	CTimerManager::GetGPUTimer("Shadow Maps")->Stop();
}


CShadowRenderer::CShadowRenderer(CLight* pLight)
{
	m_nViewport = -1;
	m_nStaticIndex = -1;
	m_nDynamicIndex = -1;

	m_nLastFrame = -1;

	m_pLight = pLight;
}


CShadowRenderer::~CShadowRenderer()
{

}


void CShadowRenderer::PrepareForFlush()
{
	std::vector<CShadowRenderer::SViewportAssociation>* tmp = ms_ViewportsToUpdateToFill;
	ms_ViewportsToUpdateToFill = ms_ViewportsToUpdateToFlush;
	ms_ViewportsToUpdateToFlush = tmp;

	ms_ViewportsToUpdateToFill->clear();

	if (CShadowSpot::ms_nShadowMatricesBuffer == INVALIDHANDLE)
		CShadowSpot::ms_nShadowMatricesBuffer = CResourceManager::CreateFrameConstantBuffer(CShadowSpot::ms_pShadowMatricesToFill->data(), (ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps) * sizeof(float4x4));
	else
		CResourceManager::UpdateFrameConstantBuffer(CShadowSpot::ms_nShadowMatricesBuffer, CShadowSpot::ms_pShadowMatricesToFill->data());

	std::vector<float4x4>* tmp1 = CShadowSpot::ms_pShadowMatricesToFill;
	CShadowSpot::ms_pShadowMatricesToFill = CShadowSpot::ms_pShadowMatricesToFlush;
	CShadowSpot::ms_pShadowMatricesToFlush = tmp1;

	if (CShadowOmni::ms_nShadowMatricesBuffer == INVALIDHANDLE)
		CShadowOmni::ms_nShadowMatricesBuffer = CResourceManager::CreateFrameConstantBuffer(CShadowOmni::ms_pShadowMatricesToFill->data(), 6 * (ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps) * sizeof(float4x4));
	else
		CResourceManager::UpdateFrameConstantBuffer(CShadowOmni::ms_nShadowMatricesBuffer, CShadowOmni::ms_pShadowMatricesToFill->data());

	tmp1 = CShadowOmni::ms_pShadowMatricesToFill;
	CShadowOmni::ms_pShadowMatricesToFill = CShadowOmni::ms_pShadowMatricesToFlush;
	CShadowOmni::ms_pShadowMatricesToFlush = tmp1;

	if (CShadowOmni::ms_nShadowPosBuffer == INVALIDHANDLE)
		CShadowOmni::ms_nShadowPosBuffer = CResourceManager::CreateFrameConstantBuffer(CShadowOmni::ms_pShadowPosToFill->data(), (ms_nMaxDynamicShadowmaps + ms_nMaxStaticShadowmaps) * sizeof(CShadowOmni::ShadowInfo));
	else
		CResourceManager::UpdateFrameConstantBuffer(CShadowOmni::ms_nShadowPosBuffer, CShadowOmni::ms_pShadowPosToFill->data());

	std::vector<CShadowOmni::ShadowInfo>* tmp2 = CShadowOmni::ms_pShadowPosToFill;
	CShadowOmni::ms_pShadowPosToFill = CShadowOmni::ms_pShadowPosToFlush;
	CShadowOmni::ms_pShadowPosToFlush = tmp2;

	CShadowDir::ms_bDrawStatic4EngineFlush = CShadowDir::ms_bDrawStatic;

	float4x4 matrices[2];

	if (CShadowDir::GetSunShadowRenderer() != nullptr)
	{
		matrices[0] = CShadowDir::GetSunShadowRenderer()->m_ShadowMatrix;
		matrices[0].transpose();
		matrices[1] = matrices[0];

		CShadowDir::GetSunShadowRenderer()->m_ShadowMatrix4EngineFlush = CShadowDir::GetSunShadowRenderer()->m_ShadowMatrix;
	}

	if (CShadowDir::ms_nShadowMatricesBuffer == INVALIDHANDLE)
		CShadowDir::ms_nShadowMatricesBuffer = CResourceManager::CreateFrameConstantBuffer(matrices, 2 * sizeof(float4x4));
	else
		CResourceManager::UpdateFrameConstantBuffer(CShadowDir::ms_nShadowMatricesBuffer, matrices);

	ms_nNumDynamicShadowmaps = 0;
}


int CShadowRenderer::GetNextStaticShadowMapIndex()
{
	int nIndex = -1;
	ms_nNumStaticShadowmapsInFrame++;

	if (ms_nNumStaticShadowmaps < ms_nMaxStaticShadowmaps)
	{
		nIndex = ms_nMaxDynamicShadowmaps + ms_nNumStaticShadowmaps;
		ms_nNumStaticShadowmaps++;

		return nIndex;
	}

	else
	{
		float fMin = 1e8f;

		for (int i = 0; i < ms_nMaxStaticShadowmaps; i++)
		{
			if (ms_fLastActualizationTime[i] < fMin && !ms_bUsedInThisFrame[i])
			{
				fMin = ms_fLastActualizationTime[i];
				nIndex = ms_nMaxDynamicShadowmaps + i;
			}
		}

		return nIndex;
	}
}



void CShadowRenderer::UpdateViewport()
{
	m_nViewport = -1;

	if (!m_pLight)
		return;

	CLight::SLightDesc desc = m_pLight->GetDesc();

	bool bDrawStatic = false;
	bool bDrawDynamic = false;

	m_Position = desc.m_Pos;

	if (desc.m_nType != CLight::e_Sun)
	{
		m_nDynamicIndex = desc.m_nCastDynamicShadows ? GetNextDynamicShadowMapIndex() : -1;

		if (m_nDynamicIndex >= 0)
		{
			bDrawDynamic = true;
			ms_nNumDynamicShadowmaps++;
		}

		if (ms_nNumStaticShadowmapsInFrame < ms_nMaxStaticShadowmaps && desc.m_nCastStaticShadows)
		{
			int nStaticIndex = m_nStaticIndex;
			bDrawStatic = m_bForceUpdateStaticShadowMap;

			if (nStaticIndex < 0 || ms_nLightIndexArray[nStaticIndex - ms_nMaxDynamicShadowmaps] != m_pLight->GetID())
			{
				nStaticIndex = GetNextStaticShadowMapIndex();
				if (nStaticIndex >= 0)
				{
					ms_nLightIndexArray[nStaticIndex - ms_nMaxDynamicShadowmaps] = m_pLight->GetID();
					m_nStaticIndex = nStaticIndex;

					ms_fLastActualizationTime[nStaticIndex - ms_nMaxDynamicShadowmaps] = CEngine::GetEngineTime();

					bDrawStatic = true;
				}
			}

			if (nStaticIndex >= 0)
				ms_bUsedInThisFrame[nStaticIndex - ms_nMaxDynamicShadowmaps] = true;

			else
				m_nStaticIndex = -1;
		}

		else
			m_nStaticIndex = -1;

		if (!bDrawStatic && !bDrawDynamic)
		{
			m_nViewport = -1;
			return;
		}

		if (bDrawStatic)
			ms_bAreStaticSMUpdated = true;
	}

	if (m_nLastFrame < CRenderer::GetCurrentFrame())
	{
		m_bUpdateStatic = bDrawStatic;
		m_nViewport = 0;
	}

	desc.m_nStaticShadowMapIndex = m_nStaticIndex;
	desc.m_nDynamicShadowMapIndex = m_nDynamicIndex;

	m_pLight->SetDesc(desc);

	m_nLastFrame = CRenderer::GetCurrentFrame();
}


struct SShadowConstants
{
	float3x4		m_ModelMatrix;
	unsigned int	m_ViewportMask[6];
};


void CShadowRenderer::RenderShadowMaps()
{
	if (CSchedulerThread::BeginRenderTaskDeclaration())
	{
		CSchedulerThread::AddRenderPass(ERenderPassId::e_Sun_Shadow);

		CSchedulerThread::EndRenderTaskDeclaration();
	}

	CSchedulerThread::ProcessRenderTask(g_ShadowMapsCommandList);
}


int CShadowRenderer::UpdateShader(Packet* packet, void* pData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)pData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	SShadowConstants constants;

	constants.m_ModelMatrix = pShaderData->m_ModelMatrix;

	unsigned int numViewports = static_cast<unsigned int>(ms_ViewportsToUpdateToFlush->size());
	unsigned int numInstances = 0;

	int omni = g_OmniShadows ? 1 : 0;

	for (unsigned int i = 0; i < numViewports; i++)
	{
		if (((*ms_ViewportsToUpdateToFlush)[i].m_nOmni == omni) && (packet->m_nViewportMask & (1ULL << (*ms_ViewportsToUpdateToFlush)[i].m_nViewport)))
		{
			int staticViewport = -1, dynamicViewport = -1;

			if ((*ms_ViewportsToUpdateToFlush)[i].m_nStaticIndex >= 0 && CRenderer::IsPacketStatic())
				staticViewport = (*ms_ViewportsToUpdateToFlush)[i].m_nStaticIndex;

			if ((*ms_ViewportsToUpdateToFlush)[i].m_nDynamicIndex >= 0 && !CRenderer::IsPacketStatic())
				dynamicViewport = (*ms_ViewportsToUpdateToFlush)[i].m_nDynamicIndex;

			if (omni)
			{
				for (unsigned int j = 0; j < 6; j++)
				{
					if (staticViewport >= 0)
					{
						float4 pos = CShadowOmni::GetShadowPos4EngineFlush(staticViewport).m_Pos;
						float4x4 matrix = CShadowOmni::GetShadowMatrix4EngineFlush(staticViewport * 6 + j);
						matrix.transpose();

						if (CViewportManager::IsVisible(float3(pos.x, pos.y, pos.z), matrix, packet->m_Center, packet->m_fBoundingSphereRadius))
						{
							int viewportID = 6 * staticViewport + j;
							constants.m_ViewportMask[viewportID / 32] |= (1UL << (viewportID & 31));
							numInstances++;
						}
					}

					if (dynamicViewport >= 0)
					{
						float4 pos = CShadowOmni::GetShadowPos4EngineFlush(dynamicViewport).m_Pos;
						float4x4 matrix = CShadowOmni::GetShadowMatrix4EngineFlush(dynamicViewport * 6 + j);
						matrix.transpose();

						if (CViewportManager::IsVisible(float3(pos.x, pos.y, pos.z), matrix, packet->m_Center, packet->m_fBoundingSphereRadius))
						{
							int viewportID = 6 * dynamicViewport + j;
							constants.m_ViewportMask[viewportID / 32] |= (1UL << (viewportID & 31));
							numInstances++;
						}
					}
				}
			}

			else
			{
				if (staticViewport > 0)
				{
					constants.m_ViewportMask[0] |= (1UL << staticViewport);
					numInstances++;
				}

				if (dynamicViewport > 0)
				{
					constants.m_ViewportMask[0] |= (1UL << dynamicViewport);
					numInstances++;
				}
			}
		}
	}

	if (numInstances == 0)
		return -1;

	pShaderData->m_nNbInstances = numInstances;

	CResourceManager::SetPushConstant(CShader::e_VertexShader, &constants, sizeof(constants));

	return 1;
}
