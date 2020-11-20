#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "EnvMaps.h"
#include "SHProbes.h"


ProgramHandle CSHProbe::ms_nGenerateLookupPID		= INVALID_PROGRAM_HANDLE;
ProgramHandle CSHProbe::ms_nComputeIrradiancePID	= INVALID_PROGRAM_HANDLE;
ProgramHandle CSHProbe::ms_nRenderPatchesPID		= INVALID_PROGRAM_HANDLE;

/*CFramebuffer* CSHProbe::ms_pFBO			= NULL;
CFramebuffer* CSHProbe::ms_pLookupFBO	= NULL;
CFramebuffer* CSHProbe::ms_pEnvMapFBO	= NULL;*/

SRenderTarget* CSHProbe::ms_pLookUpSH	= NULL;
SRenderTarget* CSHProbe::ms_pEnvMap		= NULL;
CCamera* CSHProbe::ms_pCamera			= NULL;

unsigned int CSHProbe::ms_nLookupConstantBufferID	= INVALIDHANDLE;
unsigned int CSHProbe::ms_nSHBufferID				= INVALIDHANDLE;
unsigned int CSHProbe::ms_nPatchConstantBufferID	= INVALIDHANDLE;

PacketList* CSHProbe::ms_pPacketList = NULL;

bool CSHProbe::ms_bIsLookupTableReady = false;

int gs_nCamID;


struct SPixelShaderConstants
{
	float	m_ViewProj[16];
	float	m_Eye[4];
	float	m_DiffuseColor[4];

	float	m_LightDir[4];

	float	m_ShadowMatrix[16];
	float	m_ShadowPos[4];
	float	m_ShadowDir[4];

	float	m_NearPlane;
	float	m_FarPlane;
};


struct SSHProbe
{
	float m_fSHParams[4 * 9];
};


struct SSHProbeBuffer
{
	SSHProbe m_Probes[MAX_LIGHT_PROBES];
};


void CSHProbe::Init()
{
	/*ms_nGenerateLookupPID = CShader::LoadProgram(SHADER_PATH("IBLBaker"), "ComputeSHLookupTable", "ComputeSHLookupTable");
	ms_nComputeIrradiancePID = CShader::LoadProgram(SHADER_PATH("IBLBaker"), "ComputeIrradianceSH");
	ms_nRenderPatchesPID = CShader::LoadProgram(SHADER_PATH("Probes"), "Forward", "Forward");

	ms_pLookUpSH	= new SRenderTarget(64, 64, ETextureFormat::e_R32_FLOAT, CTexture::eCubeMapArray, 1, 9);
	ms_pEnvMap		= new SRenderTarget(128, 128, ETextureFormat::e_R16G16B16A16_FLOAT, CTexture::eCubeMap);

	ms_pLookupFBO = new CFramebuffer();
	ms_pLookupFBO->BindCubeMap(0, ms_pLookUpSH, 0, 0);
	ms_pLookupFBO->SetDrawBuffers(1);

	ms_pEnvMapFBO = new CFramebuffer();
	ms_pEnvMapFBO->BindCubeMap(0, ms_pEnvMap, 0);
	ms_pEnvMapFBO->CreateCubeMapDepthStencil(128, 128, ETextureFormat::e_R24_DEPTH_G8_STENCIL, false);
	ms_pEnvMapFBO->SetDrawBuffers(1);

	ms_nLookupConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(int));
	ms_nPatchConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SPixelShaderConstants));

	ms_pCamera = new CStaticCamera();
	gs_nCamID = CRenderer::AddCamera(ms_pCamera);

	SSHProbeBuffer zero;
	ZeroMemory(&zero, sizeof(SSHProbeBuffer));

	zero.m_Probes[0].m_fSHParams[0] = 2.58676f;
	zero.m_Probes[0].m_fSHParams[1] = 2.730808f;
	zero.m_Probes[0].m_fSHParams[2] = 3.152812f;

	zero.m_Probes[0].m_fSHParams[4] = -0.431493f;
	zero.m_Probes[0].m_fSHParams[5] = -0.665128f;
	zero.m_Probes[0].m_fSHParams[6] = -0.969124f;

	zero.m_Probes[0].m_fSHParams[8] = -0.353886f;
	zero.m_Probes[0].m_fSHParams[9] = 0.048348f;
	zero.m_Probes[0].m_fSHParams[10] = 0.672755f;

	zero.m_Probes[0].m_fSHParams[12] = -0.604269f;
	zero.m_Probes[0].m_fSHParams[13] = -0.88623f;
	zero.m_Probes[0].m_fSHParams[14] = -1.298684f;

	zero.m_Probes[0].m_fSHParams[16] = 0.320121f;
	zero.m_Probes[0].m_fSHParams[17] = 0.422942f;
	zero.m_Probes[0].m_fSHParams[18] = 0.541783f;

	zero.m_Probes[0].m_fSHParams[20] = -0.137435f;
	zero.m_Probes[0].m_fSHParams[21] = -0.168666f;
	zero.m_Probes[0].m_fSHParams[22] = -0.229637f;

	zero.m_Probes[0].m_fSHParams[24] = -0.052101f;
	zero.m_Probes[0].m_fSHParams[25] = -0.149999f;
	zero.m_Probes[0].m_fSHParams[26] = -0.232127f;

	zero.m_Probes[0].m_fSHParams[28] = -0.117312f;
	zero.m_Probes[0].m_fSHParams[29] = -0.167151f;
	zero.m_Probes[0].m_fSHParams[30] = -0.265015f;

	zero.m_Probes[0].m_fSHParams[32] = -0.090028f;
	zero.m_Probes[0].m_fSHParams[33] = -0.021071f;
	zero.m_Probes[0].m_fSHParams[34] = 0.08956f;

	ms_nSHBufferID = CDeviceManager::CreateConstantBuffer(&zero, sizeof(SSHProbeBuffer));

	//CMesh* pMesh = CMesh::LoadMesh("../Data/Projects/TestGI/Proxy.mesh");
	//ms_pPacketList = CPacketManager::MeshToPacketList(pMesh);
	//CPacketManager::AddPacketList(ms_pPacketList, true, e_RenderType_DiffusePatches);

	ms_bIsLookupTableReady = false;*/
}



void CSHProbe::Terminate()
{
	/*CShader::DeleteProgram(ms_nComputeIrradiancePID);
	CShader::DeleteProgram(ms_nGenerateLookupPID);

	CDeviceManager::DeleteConstantBuffer(ms_nSHBufferID);
	CDeviceManager::DeleteConstantBuffer(ms_nPatchConstantBufferID);

	//delete ms_pPacketList;
	delete ms_pEnvMap;
	delete ms_pLookUpSH;
	delete ms_pLookupFBO;*/
}



void CSHProbe::PrecomputeSHLookupTable()
{
	/*ms_pLookupFBO->SetActive();

	CShader::BindProgram(ms_nGenerateLookupPID);

	ms_pLookupFBO->BindCubeMap(0, ms_pLookUpSH, 0, 0);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetBlendingState(e_Opaque);

	for (int i = 0; i < 6 * 9; i++)
	{
		ms_pLookupFBO->BindCubeMap(0, ms_pLookUpSH, i % 6, i / 6);

		CDeviceManager::FillConstantBuffer(ms_nLookupConstantBufferID, &i, sizeof(int));
		CDeviceManager::BindConstantBuffer(ms_nLookupConstantBufferID, ms_nGenerateLookupPID, 0);

		CRenderer::RenderQuadScreen();
	}

	ms_bIsLookupTableReady = true;*/
}



void CSHProbe::RenderPatches()
{
	/*if (!ms_bIsLookupTableReady)
		PrecomputeSHLookupTable();

	ms_pEnvMapFBO->SetActive();

	int nCamID = CRenderer::GetCurrentCameraIndex();
	CRenderer::UseCamera(gs_nCamID);

	CPacketManager::ForceShaderHook(CSHProbe::UpdateShader);

	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);

	for (int i = 0; i < 6; i++)
	{
		ms_pEnvMapFBO->BindCubeMap(0, ms_pEnvMap, i);
		ms_pEnvMapFBO->BindCubeMapDepthStencil(ms_pEnvMapFBO->GetDepthStencil(), i);

		CRenderer::ClearScreen();

		switch (i)
		{
		case 0:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), m_Position, m_Position + float3(1.f, 0.f, 0.f));
			break;

		case 1:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), m_Position, m_Position + float3(-1.f, 0.f, 0.f));
			break;

		case 2:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, 0.f, 1.f), m_Position, m_Position + float3(0.f, 1.f, 0.f));
			break;

		case 3:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, 0.f, -1.f), m_Position, m_Position + float3(0.f, -1.f, 0.f));
			break;

		case 4:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), m_Position, m_Position + float3(0.f, 0.f, 1.f));
			break;

		case 5:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), m_Position, m_Position + float3(0.f, 0.f, -1.f));
			break;
		}

		CRenderer::DrawPackets(e_RenderType_DiffusePatches, CMaterial::e_Deferred);
	}

	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);

	CRenderer::UseCamera(nCamID);

	CPacketManager::ForceShaderHook(NULL);*/
}



void CSHProbe::ComputeSH()
{
	/*if (!ms_bIsLookupTableReady)
		PrecomputeSHLookupTable();

	CShader::BindProgram(ms_nComputeIrradiancePID);

	CTexture::SetCubeMap(ms_nComputeIrradiancePID, CIBLBaker::GetBakedEnvMap(), 1);
	CSampler::BindSamplerState(1, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CTexture::SetCubeMapArray(ms_nComputeIrradiancePID, ms_pLookUpSH->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	CDeviceManager::BindStorageBuffer(ms_nSHBufferID, 0, sizeof(SSHProbe), m_nIndex, 1);

	CDeviceManager::Dispatch(9, 1, 1);

	CDeviceManager::SetMemoryBarrierOnBufferAccess();

	CDeviceManager::BindStorageBuffer(0, 0, 0, 0, 0);*/
}



void CSHProbe::Update()
{
	RenderPatches();

	ComputeSH();
}



int CSHProbe::UpdateShader(Packet* packet, void* p_pShaderData)
{
	/*if (packet->m_nCurrentPass > 0)
		return -1;

	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	CShader::BindProgram(ms_nRenderPatchesPID);

	CSunLight* pLight = (CSunLight*)CLightsManager::GetLight(0);
	//pLight->GetShadowmap()->Set(ms_nRenderPatchesPID, 1);

	CMaterial* pMaterial = packet->m_pMaterial;

	SPixelShaderConstants pregisters;

	float4x4 mViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	mViewProj.transpose();

	memcpy(&pregisters.m_ViewProj, &(mViewProj.m00), 16 * sizeof(float));
	memcpy(&pregisters.m_Eye, CRenderer::GetCurrentCamera()->GetPosition().v(), 3 * sizeof(float));

	memcpy(&pregisters.m_LightDir, pLight->GetDirection().v(), 3 * sizeof(float));
	pregisters.m_LightDir[3] = pLight->GetPower();

	float4 ModulateColor = pMaterial->m_ModulateDiffuseColor * pShaderData->ModulateColor;
	memcpy(pregisters.m_DiffuseColor, &(ModulateColor.x), 4 * sizeof(float));

	//float4x4 ShadowMatrix = *(pLight->GetShadowmap()->GetViewProjMatrix());
	//ShadowMatrix.transpose();*/

	/*memcpy(&pregisters.m_ShadowMatrix, &ShadowMatrix.m00, 16 * sizeof(float));

	memcpy(&pregisters.m_ShadowPos, pLight->GetPosition().v(), 4 * sizeof(float));
	memcpy(&pregisters.m_ShadowDir, pLight->GetDirection().v(), 4 * sizeof(float));

	pregisters.m_NearPlane = pLight->GetShadowmap()->GetNearPlane();
	pregisters.m_FarPlane = pLight->GetShadowmap()->GetFarPlane();*/

	/*CDeviceManager::FillConstantBuffer(ms_nPatchConstantBufferID, &pregisters, sizeof(SPixelShaderConstants));
	CDeviceManager::BindConstantBuffer(ms_nPatchConstantBufferID, ms_nRenderPatchesPID, 0);

	CDeviceManager::BindConstantBuffer(CSHProbe::GetSHBuffer(), ms_nRenderPatchesPID, 1);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);
	CRenderStates::SetDepthStencil(e_LessEqual, false);
	CRenderStates::SetCullMode(e_CullBackCCW);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetRasterizerState(e_Solid);*/

	return 1;
}
