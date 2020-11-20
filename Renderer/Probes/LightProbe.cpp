#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/IBLBaker/EnvMaps.h"
#include "LightProbe.h"

//CFramebuffer* CLightProbe::ms_pFBO = NULL;
//CFramebuffer* CLightProbe::ms_pLookupFBO = NULL;

//SRenderTarget* CLightProbe::ms_pEnvironmentMap = NULL;
//SRenderTarget* CLightProbe::ms_pLookUpSH = NULL;
//SRenderTarget* CLightProbe::ms_pSHIrradiance = NULL;
//
//CCamera* CLightProbe::ms_pCamera = NULL;
//unsigned int CLightProbe::ms_nConstantBufferID = 0;
//unsigned int CLightProbe::ms_nLookupConstantBufferID = 0;
//
//unsigned int CLightProbe::ms_nComputeSHPID = 0;
//unsigned int CLightProbe::ms_nResetPID = 0;
//
//unsigned int CLightProbe::ms_nPID = 0;
//unsigned int CLightProbe::ms_nGenerateLookupPID = 0;
//
//bool CLightProbe::ms_bIsLookupTableReady = false;


#define ENVIRONMENT_MAP_SIZE 512


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


void CLightProbe::Init()
{
	/*CTexture* pEnvironmentMap = new CTexture(ENVIRONMENT_MAP_SIZE, ENVIRONMENT_MAP_SIZE, ETextureFormat::e_R16G16B16A16_FLOAT, CTexture::eCubeMap);
	ms_pEnvironmentMap = new SRenderTarget(pEnvironmentMap);

	ms_pFBO = new CFramebuffer();
	ms_pFBO->BindCubeMap(0, ms_pEnvironmentMap, 0);
	ms_pFBO->CreateCubeMapDepthStencil(ENVIRONMENT_MAP_SIZE, ENVIRONMENT_MAP_SIZE, ETextureFormat::e_R24_DEPTH_G8_STENCIL, false);
	ms_pFBO->SetDrawBuffers(1);

	ms_pCamera = new CCamera();

	ms_nConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SPixelShaderConstants));
	ms_nLookupConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(int));

	ms_nPID = CShader::LoadProgram(SHADER_PATH("Probes"), "Forward", "Forward");
	ms_nGenerateLookupPID = CShader::LoadProgram(SHADER_PATH("Probes"), "ComputeSHLookupTable", "ComputeSHLookupTable", "ComputeSHLookupTable");
	ms_nComputeSHPID = CShader::LoadProgram(SHADER_PATH("Probes"), "ComputeSH", "ComputeSH");
	ms_nResetPID = CShader::LoadProgram(SHADER_PATH("Probes"), "Reset", "Reset");

	CTexture* pLookUpSH = new CTexture(64, 64, ETextureFormat::e_R32_FLOAT, CTexture::eCubeMapArray, 1, 9);
	ms_pLookUpSH = new SRenderTarget(pLookUpSH);

	CTexture* pSHIrradiance = new CTexture(3, 3, ETextureFormat::e_R32_FLOAT, CTexture::eTexture2D);
	ms_pSHIrradiance = new SRenderTarget(pSHIrradiance);

	ms_pLookupFBO = new CFramebuffer();
	ms_pLookupFBO->BindCubeMap(0, ms_pLookUpSH, 0, 0);
	ms_pLookupFBO->SetDrawBuffers(1);

	ms_bIsLookupTableReady = false;*/
}


void CLightProbe::Terminate()
{
	/*CDeviceManager::DeleteConstantBuffer(ms_nConstantBufferID);
	CDeviceManager::DeleteConstantBuffer(ms_nLookupConstantBufferID);

	CShader::DeleteProgram(ms_nPID);
	CShader::DeleteProgram(ms_nGenerateLookupPID);
	CShader::DeleteProgram(ms_nResetPID);
	CShader::DeleteProgram(ms_nComputeSHPID);

	ms_nPID = 0;

	delete ms_pSHIrradiance;
	delete ms_pEnvironmentMap;
	delete ms_pLookUpSH;
	delete ms_pFBO;
	delete ms_pLookupFBO;
	delete ms_pCamera;*/
}


void CLightProbe::Update()
{
	/*if (!ms_bIsLookupTableReady)
		PrecomputeSHLookupTable();

	ms_pFBO->SetActive();*/

	//CCamera* pCamera = CRenderer::GetInstance()->GetCamera();
	//CRenderer::GetInstance()->UseCamera(ms_pCamera);

	//CPacketManager::ForceShaderHook(CLightProbe::UpdateShader);

	//CRenderer::ClearColor(1.f, 1.f, 1.f, 0.f);

	//for (int i = 0; i < 6; i++)
	//{
		//ms_pFBO->BindCubeMap(0, ms_pEnvironmentMap, i);
		//ms_pFBO->BindCubeMapDepthStencil(ms_pFBO->GetDepthStencil(), i);

		//CRenderer::ClearScreen();

		/*switch (i)
		{
		case 0:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(1.f, 0.f, 2.f));
			break;

		case 1:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(-1.f, 0.f, 2.f));
			break;

		case 2:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 2.f), float3(0.f, 1.f, 2.f));
			break;

		case 3:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 2.f), float3(0.f, -1.f, 2.f));
			break;

		case 4:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(0.f, 0.f, 3.f));
			break;

		case 5:
			ms_pCamera->SetCamera(90.f, 1.f, 0.1f, 20.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(0.f, 0.f, 1.f));
			break;
		}

		CRenderer::GetInstance()->DrawPackets(e_RenderType_Preview);
	}

	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);*/

	//CRenderer::GetInstance()->UseCamera(pCamera);

	//CPacketManager::ForceShaderHook(NULL);

	//ComputeSH();
}



/*void CLightProbe::ComputeSH()
{
	ms_pLookupFBO->SetActive();
	ms_pLookupFBO->BindRenderTarget(0, ms_pSHIrradiance);

	CShader::BindProgram(ms_nComputeSHPID);

	CTexture::SetCubeMap(ms_nComputeSHPID, ms_pEnvironmentMap->m_nTextureId, 0);
	CTexture::SetCubeMapArray(ms_nComputeSHPID, ms_pLookUpSH->m_nTextureId, 1);

	CRenderer::RenderQuadScreen();
}*/


/*
void CLightProbe::PrecomputeSHLookupTable()
{
	ms_pLookupFBO->SetActive();

	CShader::BindProgram(ms_nGenerateLookupPID);

	ms_pLookupFBO->BindCubeMap(0, ms_pLookUpSH, 0, 0);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetBlendingState(e_Opaque);

	for (int i = 0; i < 9; i++)
	{
		CDeviceManager::FillConstantBuffer(ms_nLookupConstantBufferID, &i, sizeof(int));
		CDeviceManager::BindConstantBuffer(ms_nLookupConstantBufferID, ms_nGenerateLookupPID, 0);

		CRenderer::RenderQuadScreen();
	}

	ms_bIsLookupTableReady = true;

	ms_pLookupFBO->BindRenderTarget(0, ms_pSHIrradiance);
	CShader::BindProgram(ms_nResetPID);
	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderer::RenderQuadScreen();
}*/



/*int CLightProbe::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;	

	return 1;
}*/

