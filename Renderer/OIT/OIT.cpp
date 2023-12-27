#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/SDF/SDF.h"
#include "Engine/Project/Engine/resource.h"
#include "OIT.h"


CTexture* COIT::ms_pAOITColorNodes	= nullptr;
CTexture* COIT::ms_pAOITDepthNodes	= nullptr;
CTexture* COIT::ms_pAOITClearMask	= nullptr;
CTexture* gs_pMarschnerLUT	= nullptr;


extern bool gs_bEnableDiffuseGI_Saved;


void AOIT_EntryPoint();
void AOITHair_EntryPoint();
void AOITClear_EntryPoint();
void AOITResolve_EntryPoint();
void AOITWriteDepth_EntryPoint();


struct SOITSunConstants
{
	float4x4	m_ShadowMatrix;
	float4		m_SunColor;
	float4		m_SunDir;
};


void COIT::Init()
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pAOITColorNodes	= new CTexture(nWidth, nHeight, ETextureFormat::e_R32G32B32A32_UINT, eTextureStorage2D);
	ms_pAOITDepthNodes	= new CTexture(nWidth, nHeight, ETextureFormat::e_R32G32B32A32_FLOAT, eTextureStorage2D);
	ms_pAOITClearMask	= new CTexture(nWidth, nHeight, ETextureFormat::e_R8_UINT, eTextureStorage2D);

	gs_pMarschnerLUT	= new CTexture(IDR_DDS10);

	//if (CRenderPass::BeginGraphics("OIT"))
	//{
	//	// Clear Mask
	//	if (CRenderPass::BeginComputeSubPass())
	//	{
	//		CRenderPass::BindResourceToWrite(0, ms_pAOITClearMask->GetID(), CRenderPass::e_UnorderedAccess);

	//		CRenderPass::BindProgram("AOITClear");

	//		CRenderPass::SetEntryPoint(AOITClear_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Draw Opaque Hair
	//	/*if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(1, CLightsManager::GetLightGridTexture(), CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(2, CLightsManager::GetLightIndexBuffer(), CShader::e_FragmentShader, CRenderPass::e_Buffer);

	//		CRenderPass::EndSubPass();
	//	}*/

	//	// Draw Packets
	//	/*if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(4, CLightsManager::GetLightGridTexture(),		CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(5, CLightsManager::GetLightIndexBuffer(),		CShader::e_FragmentShader, CRenderPass::e_Buffer);
	//		CRenderPass::SetNumTextures(6, 1024);
	//		CRenderPass::SetNumSamplers(7, 1);
	//		CRenderPass::BindResourceToRead(8, CShadowRenderer::GetShadowmapArray(),		CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(9, CShadowRenderer::GetSunShadowmapArray(),		CShader::e_FragmentShader);
	//		CRenderPass::SetNumSamplers(10, 1);
	//		CRenderPass::BindResourceToRead(11, CLightField::GetIrradianceField(0),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(12, CLightField::GetProbeMetadata(0),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(13, CLightField::GetLightFieldOcclusion(0, 0),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(14, CLightField::GetLightFieldOcclusion(0, 1),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(15, CLightField::GetIrradianceField(1),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(16, CLightField::GetProbeMetadata(1),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(17, CLightField::GetLightFieldOcclusion(1, 0),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(18, CLightField::GetLightFieldOcclusion(1, 1),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(19, CLightField::GetIrradianceField(2),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(20, CLightField::GetProbeMetadata(2),			CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(21, CLightField::GetLightFieldOcclusion(2, 0),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(22, CLightField::GetLightFieldOcclusion(2, 1),	CShader::e_FragmentShader);

	//		CRenderPass::BindResourceToWrite(1, ms_pAOITClearMask->GetID(),					CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindResourceToWrite(2, ms_pAOITColorNodes->GetID(),				CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindResourceToWrite(3, ms_pAOITDepthNodes->GetID(),				CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	//		CRenderPass::BindProgram("GBuffer", "AOIT");

	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_Greater, false);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None);

	//		CRenderPass::SetEntryPoint(AOIT_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}*/

	//	// Draw Hair
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::SetNumSamplers(4, 1);
	//		CRenderPass::SetNumSamplers(5, 1);

	//		CRenderPass::BindResourceToRead(6, CLightsManager::GetLightGridTexture(),		CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(7, CLightsManager::GetLightIndexBuffer(),		CShader::e_FragmentShader, CRenderPass::e_Buffer);
	//		CRenderPass::SetNumTextures(8, 1024);
	//		CRenderPass::SetNumTextures(9, 1);
	//		

	//		CRenderPass::BindResourceToWrite(1, ms_pAOITClearMask->GetID(),					CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindResourceToWrite(2, ms_pAOITColorNodes->GetID(),				CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindResourceToWrite(3, ms_pAOITDepthNodes->GetID(),				CRenderPass::e_UnorderedAccess);
	//		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	//		CRenderPass::BindProgram("GBufferHair", "AOITHair");

	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_Greater, false);
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None);

	//		CRenderPass::SetEntryPoint(AOITHair_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Resolve
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::BindResourceToRead(0, ms_pAOITClearMask->GetID(),	CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(1, ms_pAOITColorNodes->GetID(), CShader::e_FragmentShader);
	//		CRenderPass::BindResourceToRead(2, ms_pAOITDepthNodes->GetID(), CShader::e_FragmentShader);

	//		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetMergeTarget(), CRenderPass::e_RenderTarget);

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

	//		CRenderPass::BindProgram("AOITResolve", "AOITResolve");

	//		CRenderPass::DisableDepthStencil();
	//		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_Solid, ERasterCullMode::e_CullMode_None);
	//		CRenderPass::SetBlendState(true, false, EBlendFunc::e_BlendFunc_SrcAlpha, EBlendFunc::e_BlendFunc_InvSrcAlpha, EBlendOp::e_BlendOp_Add, EBlendFunc::e_BlendFunc_SrcAlpha, EBlendFunc::e_BlendFunc_InvSrcAlpha, EBlendOp::e_BlendOp_Add);

	//		CRenderPass::SetEntryPoint(AOITResolve_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	// Write Depth
	//	if (CRenderPass::BeginGraphicsSubPass())
	//	{
	//		CRenderPass::SetNumTextures(1, 1024);
	//		CRenderPass::SetNumSamplers(2, 1);

	//		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

	//		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	//		CRenderPass::BindProgram("AOITWriteDepth", "AOITWriteDepth");

	//		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_Greater, true);

	//		CRenderPass::SetEntryPoint(AOITWriteDepth_EntryPoint);

	//		CRenderPass::EndSubPass();
	//	}

	//	CRenderPass::End();
	//}
}


extern float VanDerCorput2(unsigned int inBits);
extern float VanDerCorput3(unsigned int inBits);


void AOITClear_EntryPoint()
{
	CTimerManager::GetGPUTimer("OIT")->Start();

	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch((nWidth + 15) / 16, (nHeight + 15) / 16, 1);
}


struct SConstants
{
	float4 m_Center0;
	float4 m_Size0;
	float4 m_Center1;
	float4 m_Size1;
	float4 m_Center2;
	float4 m_Size2;
	float4 m_RealCenter;
	float4 m_Eye;

	unsigned int m_FrameIndex;
	float m_Near;
	float m_Far;
	float padding;
};



void AOITHair_EntryPoint()
{
	CPacketManager::ForceShaderHook(COIT::Hair_UpdateShader);

	CResourceManager::SetSampler(4, e_Anisotropic_Linear_UVW_Wrap);
	CResourceManager::SetSampler(5, e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialTextures(8);
	CMaterial::BindMaterialBuffer(12);

	CLightsManager::SetLightListConstantBuffer(10);
	CLightsManager::SetShadowLightListConstantBuffer(11);

	CTextureInterface::SetTexture(gs_pMarschnerLUT->GetID(), 9);

	struct
	{
		float4		m_Eye;
		float4		m_SunColor;
		float4		m_SunDir;		

		float		m_Near;
		float		m_Far;
		float		screenSize[2];
	} constants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		constants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
		constants.m_SunDir = float4(desc.m_Dir, 0.f);
	}

	else
		constants.m_SunColor = 0.f;

	float FOV = CRenderer::GetFOV4EngineFlush();
	float NearPlane = CRenderer::GetNear4EngineFlush();
	float FarPlane = CRenderer::GetFar4EngineFlush();

	constants.m_Eye = CRenderer::GetViewerPosition4EngineFlush();

	constants.m_Near = CRenderer::GetNear4EngineFlush();
	constants.m_Far = CRenderer::GetFar4EngineFlush();

	constants.screenSize[0] = CDeviceManager::GetDeviceWidth();
	constants.screenSize[1] = CDeviceManager::GetDeviceHeight();

	CResourceManager::SetConstantBuffer(13, &constants, sizeof(constants));

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CPacketManager::ForceShaderHook(0);
}



void AOIT_EntryPoint()
{
	CPacketManager::ForceShaderHook(COIT::UpdateShader);

	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialTextures(6);
	CResourceManager::SetSampler(7, e_Anisotropic_Linear_UVW_Wrap);
	CResourceManager::SetSampler(10, e_ZComparison_Linear_UVW_Clamp);
	CMaterial::BindMaterialBuffer(23);
	CLightsManager::SetLightListConstantBuffer(24);
	CLightsManager::SetShadowLightListConstantBuffer(25);

	float sampleCoords[32];

	static int index = 1;

	float offset = VanDerCorput2(index);
	float angle = 2.f * M_PI * VanDerCorput3(index);

	for (int i = 0; i < 16; i++)
	{
		float x = i + offset;
		float r = sqrtf(x / 16.f);
		float theta = 3.88322f * x + angle;

		sampleCoords[2 * i] = r * cosf(theta);
		sampleCoords[2 * i + 1] = r * sinf(theta);
	}

	CResourceManager::SetConstantBuffer(26, sampleCoords, sizeof(sampleCoords));

	struct
	{
		float4x4	m_ShadowMatrix;
		float4		m_SunColor;
		float4		m_SunDir;

		float4		m_Center0;
		float4		m_Size0;
		float4		m_Center1;
		float4		m_Size1;
		float4		m_Center2;
		float4		m_Size2;
		float4		m_RealCenter;
		float4		m_Eye;

		unsigned	m_FrameIndex;
		float		m_Near;
		float		m_Far;
		float		padding;
	} constants;

	if (CShadowDir::GetSunShadowRenderer())
	{
		CLight::SLightDesc desc = CShadowDir::GetSunShadowRenderer()->GetLight()->GetDesc();

		constants.m_ShadowMatrix = CShadowDir::GetSunShadowRenderer()->GetShadowMatrix4EngineFlush();
		constants.m_ShadowMatrix.transpose();
		constants.m_SunColor = float4(desc.m_Color, desc.m_fIntensity);
		constants.m_SunDir = float4(desc.m_Dir, 0.f);
	}

	else
		constants.m_SunColor = 0.f;

	float FOV = CRenderer::GetFOV4EngineFlush();
	float NearPlane = CRenderer::GetNear4EngineFlush();
	float FarPlane = CRenderer::GetFar4EngineFlush();

	constants.m_Center0		= CLightField::GetCenter(0);
	constants.m_Size0		= CLightField::GetSize(0);
	constants.m_Center1		= CLightField::GetCenter(1);
	constants.m_Size1		= CLightField::GetSize(1);
	constants.m_Center2		= CLightField::GetCenter(2);
	constants.m_Size2		= CLightField::GetSize(2);
	constants.m_RealCenter	= CLightField::GetRealCenter();
	constants.m_Eye			= CRenderer::GetViewerPosition4EngineFlush();
	constants.m_Eye.w		= gs_bEnableDiffuseGI_Saved ? 1.f : 0.f;

	constants.m_FrameIndex = index;
	constants.m_Near = CRenderer::GetNear4EngineFlush();
	constants.m_Far = CRenderer::GetFar4EngineFlush();

	CResourceManager::SetConstantBuffer(27, &constants, sizeof(constants));

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CPacketManager::ForceShaderHook(0);
}


void AOITResolve_EntryPoint()
{
	CRenderer::RenderQuadScreen();
}


void AOITWriteDepth_EntryPoint()
{
	CPacketManager::ForceShaderHook(COIT::WriteDepth_UpdateShader);

	CRenderer::SetViewProjConstantBuffer(0);
	CMaterial::BindMaterialTextures(1);
	CResourceManager::SetSampler(2, e_Anisotropic_Linear_UVW_Wrap);
	CMaterial::BindMaterialBuffer(3);

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Forward);

	CPacketManager::ForceShaderHook(0);

	CTimerManager::GetGPUTimer("OIT")->Stop();
}



int COIT::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	CMaterial::BindMaterial(23, packet->m_pMaterial->GetID());

	float3x4 constants[2];
	constants[0] = pShaderData->m_ModelMatrix;
	constants[1] = pShaderData->m_LastModelMatrix;

	CResourceManager::SetPushConstant(CShader::e_VertexShader, &constants, sizeof(constants));

	return 1;
}



int COIT::Hair_UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	CMaterial::BindMaterial(12, packet->m_pMaterial->GetID());

	float3x4 constants[2];
	constants[0] = pShaderData->m_ModelMatrix;
	constants[1] = pShaderData->m_LastModelMatrix;

	CResourceManager::SetPushConstant(CShader::e_VertexShader, &constants, sizeof(constants));

	return 1;
}



int COIT::WriteDepth_UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	CMaterial::BindMaterial(3, packet->m_pMaterial->GetID());

	return 1;
}
