#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/AO/AO.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/IBLBaker/SHProbes.h"
#include "Engine/Renderer/IBLBaker/EnvMaps.h"
#include "LightsManager.h"


CTexture*		CLightsManager::ms_pLightGrid				= NULL;
CTexture*		CLightsManager::ms_pLightIndices			= NULL;
CTexture*		CLightsManager::ms_pControlArray			= NULL;

//SRenderTarget*	CLightsManager::ms_pGridDummyTarget			= NULL;


ProgramHandle gs_MarkFrontFacesPID		= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_FroxelizePID			= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_FixGapsPID				= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_ClusteredLightingPID	= INVALID_PROGRAM_HANDLE;
ProgramHandle gs_ClearGridPID			= INVALID_PROGRAM_HANDLE;

unsigned int	gs_ViewProjConstants	= INVALIDHANDLE;
unsigned int	gs_FroxelizeConstants	= INVALIDHANDLE;


unsigned int	gs_ClusteredConstants	= INVALIDHANDLE;


unsigned int	gs_nBatchID				= 0U;
bool			gs_bShadowPass			= false;


struct SClusteredPixelConstants
{
	float4x4	m_InvViewProj;
	float4		m_Eye;
	float4		m_NearFar;
	float4		m_Params;
};

struct SFroxelizeConstants
{
	float			Near;
	float			Far;
	unsigned int	BatchID;
	unsigned int	Padding;
};


//void CLightsManager::InitClusteredLighting()
//{
	//ms_pLightGrid			= new CTexture(64, 64, e_R32_UINT, CTexture::eTextureStorage3D, 64);
	//ms_pLightIndices		= new CTexture(512, 512, e_R8_UINT, CTexture::eTextureStorage3D, 64);
	//ms_pControlArray		= new CTexture(64, 64, e_R16_UINT, CTexture::eTextureStorage3D, 64);
	//
	//ms_pGridDummyTarget		= new SRenderTarget(64, 64, e_R8, CTexture::eTexture2D);
	//
	//gs_MarkFrontFacesPID	= CShader::LoadProgram(SHADER_PATH("Lights"), "MarkFaces", "MarkFaces", "MarkFrontFaces");
	//gs_FroxelizePID			= CShader::LoadProgram(SHADER_PATH("Lights"), "MarkFaces", "MarkFaces", "Froxelize");
	//gs_ClusteredLightingPID = CShader::LoadProgram(SHADER_PATH("Lights"), "Light", "ClusteredLighting");
	//gs_ClearGridPID			= CShader::LoadProgram(SHADER_PATH("Lights"), "ClearGrid");
	//
	//gs_ViewProjConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(float4x4));
	//gs_FroxelizeConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SFroxelizeConstants));
	//gs_ClusteredConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(SClusteredPixelConstants));
//}



//void CLightsManager::TerminateClusteredLighting()
//{
	//delete ms_pLightGrid;
	//delete ms_pLightIndices;
	//delete ms_pControlArray;
	//
	//delete ms_pGridDummyTarget;
	//
	//CShader::DeleteProgram(gs_MarkFrontFacesPID);
	//CShader::DeleteProgram(gs_FroxelizePID);
	//CShader::DeleteProgram(gs_FixGapsPID);
	//CShader::DeleteProgram(gs_ClusteredLightingPID);
	//CShader::DeleteProgram(gs_ClearGridPID);
	//
	//CDeviceManager::DeleteConstantBuffer(gs_ViewProjConstants);
	//CDeviceManager::DeleteConstantBuffer(gs_FroxelizeConstants);
	//CDeviceManager::DeleteConstantBuffer(gs_ClusteredConstants);
//}



void CLightsManager::ClusteredLighting()
{
	//CFramebuffer::BindRenderTarget(0, ms_pDiffuseTarget);
	//CFramebuffer::BindRenderTarget(1, ms_pSpecularTarget);
	//CFramebuffer::BindDepthStencil(NULL);
	//CFramebuffer::SetDrawBuffers(2);
	//
	//CShader::BindProgram(gs_ClusteredLightingPID);
	//
	//int nUnit = 0;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CRenderer::GetDepthStencil()->m_nTextureId, nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CDeferredRenderer::GetNormalTarget()->m_nTextureId, nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CDeferredRenderer::GetInfoTarget()->m_nTextureId, nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CDeferredRenderer::GetFresnelTarget()->m_nTextureId, nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTextureArray(gs_ClusteredLightingPID, CShadowRenderer::GetShadowmapArray(), nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_ZComparison_Linear_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CAO::GetFinalTarget(), nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Point_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetTexture(gs_ClusteredLightingPID, CIBLBaker::GetBrdfMap(), nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);
	//
	//nUnit++;
	//CTexture::SetCubeMap(gs_ClusteredLightingPID, CIBLBaker::GetBakedEnvMap(), nUnit);
	//CSampler::BindSamplerState(nUnit, CSampler::e_MinMagMip_Linear_UVW_Clamp);
	//
	//ms_pLightGrid->BindImage(0, CTexture::e_ReadOnly);
	//ms_pLightIndices->BindImage(1, CTexture::e_ReadOnly);
	//
	//SClusteredPixelConstants pregisters;
	//pregisters.m_InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	//pregisters.m_InvViewProj.transpose();
	//
	//pregisters.m_Eye = float4(CRenderer::GetCurrentCamera()->GetPosition(), 1.f);
	//
	//float NearPlane = CRenderer::GetCurrentCamera()->GetNearPlane();
	//float FarPlane = CRenderer::GetCurrentCamera()->GetFarPlane();
	//
	//pregisters.m_NearFar.x = NearPlane;
	//pregisters.m_NearFar.y = FarPlane;
	//pregisters.m_NearFar.z = 1.f / CShadowRenderer::GetShadowmapSize();
	//pregisters.m_NearFar.w = 1.f / CShadowRenderer::GetShadowmapSize();
	//
	//pregisters.m_Params.x = CRenderer::IsAOEnabled() ? 1.f : 0.f;
	//pregisters.m_Params.y = CAO::GetAOStrength();
	//
	//CDeviceManager::FillConstantBuffer(gs_ClusteredConstants, &pregisters, sizeof(pregisters));
	//
	//CDeviceManager::BindConstantBuffer(gs_ClusteredConstants, gs_ClusteredLightingPID, 0);
	//CDeviceManager::BindConstantBuffer(ms_nLightsConstantBuffer, gs_ClusteredLightingPID, 1);
	//CDeviceManager::BindConstantBuffer(CSHProbe::GetSHBuffer(), gs_ClusteredLightingPID, 2);
	//
	//CRenderStates::SetBlendingState(e_Opaque);
	//CRenderStates::SetCullMode(e_CullNone);
	//CRenderStates::SetDepthStencil(e_Zero, false);
	//CRenderStates::SetRasterizerState(e_Solid);
	//CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	//
	//CRenderer::RenderQuadScreen();
}



void CLightsManager::BuildGrid()
{
	/*CShader::BindProgram(gs_ClearGridPID);
	ms_pLightGrid->BindImage(0, CTexture::e_WriteOnly);
	CDeviceManager::DispatchDraw(8, 8, 8);

	CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindRenderTarget(0, ms_pGridDummyTarget);
	CFramebuffer::BindRenderTarget(1, NULL);
	CFramebuffer::BindRenderTarget(3, NULL);
	CFramebuffer::BindRenderTarget(4, NULL);
	CFramebuffer::BindRenderTarget(5, NULL);
	CFramebuffer::BindDepthStencil(NULL);

	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetWriteMask(e_Red);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderer::ClearScreen();

	gs_nBatchID = 0U;

	gs_bShadowPass = false;
	CRenderer::DrawPackets(e_RenderType_Light);

	gs_bShadowPass = true;
	CRenderer::DrawPackets(e_RenderType_LightShadow);

	CDeviceManager::SetMemoryBarrierOnBufferAccess();*/
}



int CLightsManager::MarkFrontfaces()
{
	/*CShader::BindProgram(gs_MarkFrontFacesPID);

	ms_pControlArray->BindImage(0, CTexture::e_WriteOnly);

	float4x4 ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	ViewProj.transpose();

	CDeviceManager::BindConstantBuffer(gs_ViewProjConstants, 0, 0);
	CDeviceManager::FillConstantBuffer(gs_ViewProjConstants, &ViewProj.m00, 16 * sizeof(float));

	float NearPlane = CRenderer::GetCurrentCamera()->GetNearPlane();
	float FarPlane = CRenderer::GetCurrentCamera()->GetFarPlane();

	SFroxelizeConstants Params = { NearPlane, FarPlane, gs_nBatchID, gs_bShadowPass ? 1U : 0U };

	CDeviceManager::FillConstantBuffer(gs_FroxelizeConstants, &Params, sizeof(SFroxelizeConstants));

	CDeviceManager::BindConstantBuffer(gs_ViewProjConstants, gs_MarkFrontFacesPID, 0);
	CDeviceManager::BindConstantBuffer(gs_FroxelizeConstants, gs_MarkFrontFacesPID, 1);

	CRenderStates::SetCullMode(e_CullBackCCW);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetDepthStencil(e_Zero, false);

	return 1;*/
	return 1;
}



int CLightsManager::Froxelize()
{
	/*CShader::BindProgram(gs_FroxelizePID);

	ms_pLightGrid->BindImage(0, CTexture::e_ReadWrite);
	ms_pLightIndices->BindImage(1, CTexture::e_WriteOnly);
	ms_pControlArray->BindImage(2, CTexture::e_ReadOnly);

	CDeviceManager::BindConstantBuffer(gs_ViewProjConstants, gs_FroxelizePID, 0);
	CDeviceManager::BindConstantBuffer(gs_FroxelizeConstants, gs_FroxelizePID, 1);

	CRenderStates::SetCullMode(e_CullBackCW);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetDepthStencil(e_Zero, false);*/

	return 1;
}


int CLightsManager::ClusteredUpdateShader(Packet* packet, void* p_pShaderData)
{/*
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (packet->m_nCurrentPass > 1)
	{
		gs_nBatchID++;

		CDeviceManager::SetMemoryBarrierOnBufferAccess();
		return -1;
	}

	else if (packet->m_nCurrentPass > 0)
		return Froxelize();

	return MarkFrontfaces();*/
	return -1;
}
