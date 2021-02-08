#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Misc/Event.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Renderer/Viewports/Viewports.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Engine/Renderer/AO/AO.h"
#include "LightsManager.h"


std::vector<CLight*>			CLightsManager::ms_pLights;

CTexture*						CLightsManager::ms_DummyTarget;
CTexture*						CLightsManager::ms_pLinkedListHeadPtrTexture;
BufferId						CLightsManager::ms_LinkedListNodeCulling;

CTexture*						CLightsManager::ms_pLinkedListHeadPtrStaticGrid;
BufferId						CLightsManager::ms_StaticLightIndexBuffer;

CTexture*						CLightsManager::ms_pLinkedListHeadPtrGrid;
BufferId						CLightsManager::ms_LinkedListNodeBuffer;
BufferId						CLightsManager::ms_LightIndexBuffer;
CTexture*						CLightsManager::ms_BDRFMap;

BufferId						CLightsManager::ms_LightsListConstantBuffer = INVALIDHANDLE;
BufferId						CLightsManager::ms_ShadowLightsListConstantBuffer = INVALIDHANDLE;

CEvent*							CLightsManager::ms_pLightListReady = nullptr;

std::vector<CLight::SLightDesc> CLightsManager::ms_pVisibleLights[2];

std::vector<CLight::SLightDesc>* CLightsManager::ms_pVisibleLightsToFill	= &CLightsManager::ms_pVisibleLights[0];
std::vector<CLight::SLightDesc>* CLightsManager::ms_pVisibleLightsToFlush	= &CLightsManager::ms_pVisibleLights[1];


extern bool gs_bEnableDiffuseGI_Saved;
extern bool gs_EnableAO_Saved;


struct SSunShadowConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
};


void ClusteredLighting_EntryPoint()
{
	CLightsManager::WaitForLightList();

	CTimerManager::GetGPUTimer("Lighting")->Start();

	CResourceManager::SetSampler(18, e_MinMagMip_Linear_UVW_Clamp);
	CRenderer::SetViewProjConstantBuffer(19);
	CLightsManager::SetLightListConstantBuffer(20);
	CLightsManager::SetShadowLightListConstantBuffer(21);

	float4 constants[7];
	constants[0]	= CLightField::GetCenter(0);
	constants[1]	= CLightField::GetSize(0);
	constants[2]	= CLightField::GetCenter(1);
	constants[3]	= CLightField::GetSize(1);
	constants[4].x	= gs_bEnableDiffuseGI_Saved ? 1.f : 0.f;
	constants[4].y	= gs_EnableAO_Saved ? 1.f : 0.f;
	constants[4].z	= CSkybox::GetSkyLightIntensity();
	constants[4].w	= CRenderer::GetNear4EngineFlush();

	static unsigned int index = 1;

	constants[0].w = *reinterpret_cast<float*>(&index);

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		constants[5] = float4(desc.m_Color, desc.m_fIntensity);
		constants[6] = float4(desc.m_Dir, CRenderer::GetFar4EngineFlush());
	}

	else
	{
		constants[5] = 0.f;
		constants[6].w = CRenderer::GetFar4EngineFlush();
	}

	CResourceManager::SetPushConstant(CShader::e_FragmentShader, constants, sizeof(constants));

	CRenderer::RenderQuadScreen();

	index++;

	CTimerManager::GetGPUTimer("Lighting")->Stop();
}


float VanDerCorput2(unsigned int inBits)
{
	unsigned int bits = inBits;
	bits = (bits << 16U) | (bits >> 16U);
	bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
	bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
	bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
	bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
	return (float)bits * 2.3283064365386963e-10f;
}


float VanDerCorput3(unsigned int inBits)
{
	float f = 1.f;
	float r = 0.f;
	unsigned int i = inBits;

	while (i > 0)
	{
		f /= 3.f;
		r += f * (i % 3U);
		i /= 3;
	}

	return r;
}


void ComputeFilteredShadows_EntryPoint()
{
	CLightsManager::WaitForLightList();

	CTimerManager::GetGPUTimer("Compute Shadows")->Start();

	CResourceManager::SetSampler(6, ESamplerState::e_ZComparison_Linear_UVW_Clamp);

	CRenderer::SetViewProjConstantBuffer(8);
	CLightsManager::SetShadowLightListConstantBuffer(9);

	float sampleCoords[32];

	static int index = 1;

	float offset = 0.5f;//VanDerCorput2(index);
	float angle = 2.f * M_PI * VanDerCorput3(index);

	for (int i = 0; i < 16; i++)
	{
		float x = i + offset;
		float r = sqrtf(x / 16.f);
		float theta = 3.88322f * x + angle;

		sampleCoords[2 * i] = r * cosf(theta);
		sampleCoords[2 * i + 1] = r * sinf(theta);
	}

	CResourceManager::SetConstantBuffer(10, sampleCoords, sizeof(sampleCoords));

	SSunShadowConstants sunConstants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		sunConstants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		sunConstants.m_ShadowMatrix.transpose();
		sunConstants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
	}

	else
		sunConstants.m_SunColor = 0.f;

	CResourceManager::SetConstantBuffer(11, &sunConstants, sizeof(sunConstants));

	float NearPlane = CRenderer::GetNear4EngineFlush();
	float FarPlane = CRenderer::GetFar4EngineFlush();
	
	float constant[3];
	constant[0] = NearPlane;
	constant[1] = FarPlane;
	constant[2] = *reinterpret_cast<float*>(&index);

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, constant, 3 * sizeof(float));

	unsigned int nWidth = CDeviceManager::GetDeviceWidth();
	unsigned int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);

	CTimerManager::GetGPUTimer("Compute Shadows")->Stop();

	index++;
}


void ClearGrid_EntryPoint()
{
	CTimerManager::GetGPUTimer("Cull Lights")->Start();

	unsigned int nTexID = CRenderPass::GetWrittenResourceID(1, CRenderPass::e_UnorderedAccess);

	unsigned int width = CTextureInterface::GetTextureWidth(nTexID);
	unsigned int height = CTextureInterface::GetTextureHeight(nTexID);
	unsigned int depth = CTextureInterface::GetTextureDepth(nTexID);

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, (depth + 7) / 8);
}


void ClearGridStatic_EntryPoint()
{
	CTimerManager::GetGPUTimer("Cull Lights Static")->Start();

	unsigned int nTexID = CRenderPass::GetWrittenResourceID(1, CRenderPass::e_UnorderedAccess);

	unsigned int width = CTextureInterface::GetTextureWidth(nTexID);
	unsigned int height = CTextureInterface::GetTextureHeight(nTexID);
	unsigned int depth = CTextureInterface::GetTextureDepth(nTexID);

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, (depth + 7) / 8);
}


void CullLights_EntryPoint()
{
	CRenderer::SetViewProjConstantBuffer(2);

	CRenderer::DrawPackets(ERenderList::e_RenderType_Light);
}


void CullLightsStatic_EntryPoint()
{
	float3 constants[2];
	constants[0] = CLightField::GetCenter(1);
	constants[1] = CLightField::GetSize(1);

	CResourceManager::SetPushConstant(CShader::e_GeometryShader, constants, sizeof(constants));

	CRenderer::DrawPackets(ERenderList::e_RenderType_Light);
}


void FillIndices_EntryPoint()
{
	unsigned int nTexID = CRenderPass::GetWrittenResourceID(2, CRenderPass::e_UnorderedAccess);

	unsigned int width = CTextureInterface::GetTextureWidth(nTexID);
	unsigned int height = CTextureInterface::GetTextureHeight(nTexID);
	unsigned int depth = CTextureInterface::GetTextureDepth(nTexID);

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, (depth + 7) / 8);
}


void SortIndices_EntryPoint()
{
	unsigned int nTexID = CRenderPass::GetWrittenResourceID(1, CRenderPass::e_UnorderedAccess);

	unsigned int width = CTextureInterface::GetTextureWidth(nTexID);
	unsigned int height = CTextureInterface::GetTextureHeight(nTexID);
	unsigned int depth = CTextureInterface::GetTextureDepth(nTexID);

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, (depth + 7) / 8);

	CTimerManager::GetGPUTimer("Cull Lights")->Stop();
}


void SortIndicesStatic_EntryPoint()
{
	unsigned int nTexID = CRenderPass::GetWrittenResourceID(1, CRenderPass::e_UnorderedAccess);

	unsigned int width = CTextureInterface::GetTextureWidth(nTexID);
	unsigned int height = CTextureInterface::GetTextureHeight(nTexID);
	unsigned int depth = CTextureInterface::GetTextureDepth(nTexID);

	CDeviceManager::Dispatch((width + 7) / 8, (height + 7) / 8, (depth + 7) / 8);

	CTimerManager::GetGPUTimer("Cull Lights Static")->Stop();
}




void CLightsManager::Init()
{
	ms_DummyTarget					= new CTexture(64, 60, ETextureFormat::e_R8);

	ms_pLinkedListHeadPtrGrid		= new CTexture(64, 60, 64, ETextureFormat::e_R32_UINT, eTextureStorage3D);
	ms_pLinkedListHeadPtrStaticGrid = new CTexture(64, 64, 16, ETextureFormat::e_R32_UINT, eTextureStorage3D);
	ms_pLinkedListHeadPtrTexture	= new CTexture(64, 60, ETextureFormat::e_R32_UINT, eTextureStorage2D);

	ms_LinkedListNodeCulling		= CResourceManager::CreateRwBuffer(1 * 1024 * 1024);
	ms_LinkedListNodeBuffer			= CResourceManager::CreateRwBuffer(10 * 1024 * 1024);	
	ms_LightIndexBuffer				= CResourceManager::CreateRwBuffer(5 * 1024 * 1024);

	ms_pLinkedListHeadPtrStaticGrid	= new CTexture(64, 64, 16, ETextureFormat::e_R32_UINT, eTextureStorage3D);
	ms_StaticLightIndexBuffer		= CResourceManager::CreateRwBuffer(5 * 1024 * 1024);

	ms_BDRFMap						= new CTexture("../../Data/Environments/BRDF.dds");

	ms_pLightListReady				= CEvent::Create();

	if (CRenderPass::BeginGraphics("Lighting"))
	{
		CRenderPass::BindResourceToRead(0, ms_pLinkedListHeadPtrGrid->GetID(),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(1, ms_LightIndexBuffer,							CShader::e_FragmentShader, CRenderPass::e_Buffer);
		CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(3, CDeferredRenderer::GetAlbedoTarget(),		CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(4, CDeferredRenderer::GetNormalTarget(),		CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(5, CDeferredRenderer::GetInfoTarget(),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(6, CShadowRenderer::GeFilteredShadowArray(),	CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(7, CAO::GetFinalTarget(),						CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(8, CLightField::GetIrradianceField(0),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(9, CLightField::GetProbeMetadata(0),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(10, CLightField::GetLightFieldSH(0),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(11, CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(12, CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(13, CLightField::GetIrradianceField(1),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(14, CLightField::GetProbeMetadata(1),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(15, CLightField::GetLightFieldSH(1),			CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(16, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_FragmentShader);
		CRenderPass::BindResourceToRead(17, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_FragmentShader);
		CRenderPass::SetNumSamplers(18, 1);
		CRenderPass::BindResourceToRead(19, ms_BDRFMap->GetID(),						CShader::e_FragmentShader);

		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetDiffuseTarget(),	CRenderPass::e_RenderTarget);
		CRenderPass::BindResourceToWrite(1, CDeferredRenderer::GetSpecularTarget(),	CRenderPass::e_RenderTarget);

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("ComputeLighting", "ComputeLighting");

		CRenderPass::SetEntryPoint(ClusteredLighting_EntryPoint);

		CRenderPass::End();
	}

	if (CRenderPass::BeginCompute("Compute Shadows"))
	{
		CRenderPass::BindResourceToRead(0, CLightsManager::GetLightGridTexture(),		CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(1, CLightsManager::GetLightIndexBuffer(),		CShader::e_ComputeShader, CRenderPass::e_Buffer);
		CRenderPass::BindResourceToRead(2, CDeferredRenderer::GetDepthTarget(),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(3, CShadowRenderer::GetShadowmapHiZ(),			CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(4, CShadowRenderer::GetShadowmapArray(),		CShader::e_ComputeShader);
		CRenderPass::BindResourceToRead(5, CShadowRenderer::GetSunShadowmapArray(),		CShader::e_ComputeShader);

		CRenderPass::BindResourceToWrite(7, CShadowRenderer::GeFilteredShadowArray(),	CRenderPass::e_UnorderedAccess);

		CRenderPass::SetNumSamplers(6, 1);

		CRenderPass::BindProgram("ComputeFilteredShadows");

		CRenderPass::SetEntryPoint(ComputeFilteredShadows_EntryPoint);

		CRenderPass::End();
	}

	if (CRenderPass::BeginGraphics("Light Grid"))
	{
		// Clear Grid
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_pLinkedListHeadPtrTexture->GetID(),	CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(1, ms_pLinkedListHeadPtrGrid->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_LinkedListNodeCulling,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(3, ms_LinkedListNodeBuffer,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(4, ms_LightIndexBuffer,					CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("ClearGrid");

			CRenderPass::SetEntryPoint(ClearGrid_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Light Culling
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_DummyTarget->GetID(),				CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(0, ms_pLinkedListHeadPtrTexture->GetID(),	CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(1, ms_LinkedListNodeCulling,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("CullLights", "CullLights", "CullLights");

			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, true);

			CRenderPass::SetEntryPoint(CullLights_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Fill Indices
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pLinkedListHeadPtrTexture->GetID(),	CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_LinkedListNodeCulling,				CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(2, ms_pLinkedListHeadPtrGrid->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(3, ms_LinkedListNodeBuffer,				CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("FillLightIndices");

			CRenderPass::SetEntryPoint(FillIndices_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Compact and Sort Indices
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_LinkedListNodeBuffer,					CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(1, ms_pLinkedListHeadPtrGrid->GetID(),		CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_LightIndexBuffer,					CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("SortLightIndices");

			CRenderPass::SetEntryPoint(SortIndices_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}

	if (CRenderPass::BeginGraphics("Static Light Grid"))
	{
		// Clear Grid
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_pLinkedListHeadPtrTexture->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(1, ms_pLinkedListHeadPtrStaticGrid->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_LinkedListNodeCulling, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(3, ms_LinkedListNodeBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);
			CRenderPass::BindResourceToWrite(4, ms_StaticLightIndexBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("ClearGrid");

			CRenderPass::SetEntryPoint(ClearGridStatic_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Light Culling
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_DummyTarget->GetID(), CRenderPass::e_RenderTarget);
			CRenderPass::BindResourceToWrite(0, ms_pLinkedListHeadPtrTexture->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(1, ms_LinkedListNodeCulling, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("CullLights", "CullLightsStatic", "CullLightsStatic");

			CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None, false, false, true);

			CRenderPass::SetEntryPoint(CullLightsStatic_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Fill Indices
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_pLinkedListHeadPtrTexture->GetID(), CShader::e_ComputeShader);
			CRenderPass::BindResourceToRead(1, ms_LinkedListNodeCulling, CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(2, ms_pLinkedListHeadPtrStaticGrid->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(3, ms_LinkedListNodeBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("FillLightIndices");

			CRenderPass::SetEntryPoint(FillIndices_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Compact and Sort Indices
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, ms_LinkedListNodeBuffer, CShader::e_ComputeShader, CRenderPass::e_Buffer);

			CRenderPass::BindResourceToWrite(1, ms_pLinkedListHeadPtrStaticGrid->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(2, ms_StaticLightIndexBuffer, CRenderPass::e_UnorderedAccess, CRenderPass::e_Buffer);

			CRenderPass::BindProgram("SortLightIndices");

			CRenderPass::SetEntryPoint(SortIndicesStatic_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}


void CLightsManager::Terminate()
{
	delete ms_pLightListReady;
}

CLight* CLightsManager::AddLight(CLight::ELightType type, bool bCastShadow)
{
	CLight* pLight = NULL;

	switch (type)
	{
	case CLight::e_Omni:
		pLight = new COmniLight(bCastShadow);
		break;

	case CLight::e_Sun:
		pLight = new CSunLight(bCastShadow);
		break;

	case CLight::e_Spot:
		pLight = new CSpotLight(bCastShadow);
		break;

	default:
		break;
	}
	
	if (pLight != NULL)
		pLight->m_Desc.m_LightID = static_cast<unsigned int>(ms_pLights.size());

	ms_pLights.push_back(pLight);

	return pLight;
}


void CLightsManager::UpdateLights()
{
	std::vector<CLight*>::iterator it;
	CLight* pLight;

	ms_pVisibleLightsToFill->clear();

	for (it = ms_pLights.begin(); it < ms_pLights.end(); it++)
	{
		pLight = *it;

		if (!pLight->m_bEnabled)
			continue;

		CLight::SLightDesc desc = pLight->GetDesc();

		if (pLight->m_Type == CLight::e_Sun)
			pLight->m_bVisible = true;

		else
		{
			float3 center;
			float radius;

			if (pLight->m_Type == CLight::e_Omni)
			{
				center = desc.m_Pos;
				radius = desc.m_fMaxRadius;
			}

			else if (pLight->m_Type == CLight::e_Spot)
			{
				float h = desc.m_fMaxRadius * 0.5f;
				float a = desc.m_fAngleOut * 3.1415926f / 360.f;

				radius = h / (2.f * cosf(a));
				center = desc.m_Pos + radius * desc.m_Dir;
			}
				
			pLight->m_bVisible = true;//CViewportManager::IsVisible(0, center, radius);

			if (!pLight->m_bVisible)
				continue;

			ms_pVisibleLightsToFill->push_back(desc);
		}		

		if (pLight->CastShadow())
			pLight->GetShadowRenderer()->UpdateViewport();
	}

	BuildLightProxies();
}


void CLightsManager::BuildLightList()
{
	if (!CFrameBlueprint::IsSorting())
	{
		ms_pLightListReady->Reset();

		int numLights = static_cast<int>(ms_pVisibleLightsToFlush->size());

		std::vector<CLight::SLightDesc>& lightDesc = *ms_pVisibleLightsToFlush;

		std::vector<SLight>			lights;
		std::vector<SLightShadow>	shadowedLights;
		lights.reserve(128);
		shadowedLights.reserve(128);

		float cosOuter, cosInner;

		SLight light;
		SLightShadow shadowedLight;

		for (int i = 0; i < numLights; i++)
		{
			light.m_Pos = lightDesc[i].m_Pos;
			light.m_Pos.w = 1.f / (lightDesc[i].m_fMaxRadius * lightDesc[i].m_fMaxRadius);
			light.m_Color = lightDesc[i].m_Color * lightDesc[i].m_fIntensity;
			light.m_Dir = lightDesc[i].m_Dir;

			light.m_Color.w = 1.f;
			light.m_Dir.w = 2.f;

			switch (lightDesc[i].m_nType)
			{
			case CLight::e_Spot:

				cosInner = cosf(lightDesc[i].m_fAngleIn * 3.141592f / 360.f);
				cosOuter = cosf(lightDesc[i].m_fAngleOut * 3.141592f / 360.f);

				light.m_Color.w = 1.f / MAX(0.001f, cosInner - cosOuter);
				light.m_Dir.w = -cosOuter;
				break;

			default:
				break;
			}

			if (lightDesc[i].m_nCastShadows)
			{
				shadowedLight.m_Light = light;
				shadowedLight.m_ShadowIndex.x = 1.f * lightDesc[i].m_nStaticShadowMapIndex;
				shadowedLight.m_ShadowIndex.y = 1.f * lightDesc[i].m_nDynamicShadowMapIndex;

				shadowedLight.m_ShadowIndex.z = lightDesc[i].m_nType == CLight::e_Spot ? 0.f : 1.f;
				shadowedLight.m_ShadowIndex.w = 1.f / CShadowRenderer::GetShadowmapSize();

				if (lightDesc[i].m_nType == CLight::e_Spot)
				{
					if (shadowedLight.m_ShadowIndex.x >= 0)
						shadowedLight.m_ShadowMatrix = CShadowSpot::GetShadowMatrix4EngineFlush(lightDesc[i].m_nStaticShadowMapIndex);

					else if (shadowedLight.m_ShadowIndex.y >= 0)
						shadowedLight.m_ShadowMatrix = CShadowSpot::GetShadowMatrix4EngineFlush(lightDesc[i].m_nDynamicShadowMapIndex);
				}

				else
				{
					shadowedLight.m_ShadowMatrix.m00 = lightDesc[i].m_fMinRadius;
					shadowedLight.m_ShadowMatrix.m10 = lightDesc[i].m_fMaxRadius;
				}

				shadowedLights.push_back(shadowedLight);
			}

			else
				lights.push_back(light);
		}

		if (ms_LightsListConstantBuffer == INVALIDHANDLE)
			ms_LightsListConstantBuffer = CResourceManager::CreateFrameConstantBuffer(lights.data(), 128 * sizeof(SLight));
		else
			CResourceManager::UpdateFrameConstantBuffer(ms_LightsListConstantBuffer, lights.data(), 128 * sizeof(SLight));

		if (ms_ShadowLightsListConstantBuffer == INVALIDHANDLE)
			ms_ShadowLightsListConstantBuffer = CResourceManager::CreateFrameConstantBuffer(shadowedLights.data(), 128 * sizeof(SLightShadow));
		else
			CResourceManager::UpdateFrameConstantBuffer(ms_ShadowLightsListConstantBuffer, shadowedLights.data(), 128 * sizeof(SLightShadow));

		ms_pLightListReady->Throw();
	}
}


void CLightsManager::WaitForLightList()
{
	ms_pLightListReady->Wait();
}


void CLightsManager::PrepareForFlush()
{
	std::vector<CLight::SLightDesc>* tmp = ms_pVisibleLightsToFlush;
	ms_pVisibleLightsToFlush = ms_pVisibleLightsToFill;
	ms_pVisibleLightsToFill = tmp;
}


void CLightsManager::SetLightListConstantBuffer(int nSlot)
{
	CResourceManager::SetConstantBuffer(nSlot, ms_LightsListConstantBuffer);
}


void CLightsManager::SetShadowLightListConstantBuffer(int nSlot)
{
	CResourceManager::SetConstantBuffer(nSlot, ms_ShadowLightsListConstantBuffer);
}


void CLightsManager::DeleteLights()
{
	std::vector<CLight*>::iterator it_light;

	for (it_light = ms_pLights.begin(); it_light < ms_pLights.end(); it_light++)
		delete (*it_light);

	ms_pLights.clear();
}


void CLightsManager::ComputeLighting()
{

}


int	CLightsManager::ClusteredUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	if (CFrameBlueprint::IsCurrentRenderPass("Light Grid"))
	{
		float FOV = CRenderer::GetFOV4EngineFlush();
		float NearPlane = CRenderer::GetNear4EngineFlush();
		float FarPlane = CRenderer::GetFar4EngineFlush();

		float3 constant;

		constant.x = FOV / (NearPlane - (FarPlane - NearPlane) / (NearPlane * FarPlane));
		constant.y = NearPlane;
		constant.z = FarPlane;

		CResourceManager::SetPushConstant(CShader::e_GeometryShader | CShader::e_FragmentShader, &constant.x, 3 * sizeof(float));
	}

	return 1;
}




