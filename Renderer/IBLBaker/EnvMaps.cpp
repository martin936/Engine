#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Probes/LightProbe.h"
#include "EnvMaps.h"


/*ProgramHandle CIBLBaker::ms_nFilterEnvMapPID = INVALID_PROGRAM_HANDLE;
ProgramHandle CIBLBaker::ms_nPrecomputeBRDFPID = INVALID_PROGRAM_HANDLE;

unsigned int CIBLBaker::ms_nConstantBufferID = INVALIDHANDLE;

CTexture* CIBLBaker::ms_pBakedEnvMap = NULL;
CTexture* CIBLBaker::ms_pBrdfMap = NULL;

SRenderTarget* CIBLBaker::ms_pEnvMap = NULL;
SRenderTarget* CIBLBaker::ms_pFilteredEnvMap = NULL;

PacketList* CIBLBaker::ms_pPacketList = NULL;

int CIBLBaker::ms_nCameraID = INVALIDHANDLE;*/

//int gs_nCamID;


struct SConstants
{
	float	m_fSize;
	float	m_fRoughness;
	float	m_fFace;
	float	m_Padding;
};


void CIBLBaker::Init()
{
	/*ms_nFilterEnvMapPID = CShader::LoadProgram(SHADER_PATH("IBLBaker"), "FilterEnvMap", "FilterEnvMap");
	ms_nPrecomputeBRDFPID = CShader::LoadProgram(SHADER_PATH("IBLBaker"), "PrecomputeBRDF", "PrecomputeBRDF");

	ms_nConstantBufferID = CDeviceManager::CreateConstantBuffer(NULL, sizeof(SConstants));

	ms_pBakedEnvMap = new CTexture("../Data/Environments/EnvMapIBL.dds");
	ms_pBrdfMap = new CTexture("../Data/Environments/BRDF.dds");

	ms_pEnvMap = new SRenderTarget(1024, 1024, ETextureFormat::e_R16G16B16A16_FLOAT, CTexture::eCubeMap);
	ms_pFilteredEnvMap = new SRenderTarget(1024, 1024, ETextureFormat::e_R16G16B16A16_FLOAT, CTexture::eCubeMap, 1, 1, true);

	CCamera* pCamera = new CStaticCamera();
	ms_nCameraID = CRenderer::AddCamera(pCamera);*/

	//CMesh* pMesh = CMesh::LoadMesh("../Data/Projects/TestGI/Proxy.mesh");
	//ms_pPacketList = CPacketManager::MeshToPacketList(pMesh);
	//CPacketManager::AddPacketList(ms_pPacketList, true, e_RenderType_ReflectionPatches);
}


void CIBLBaker::Terminate()
{
	CShader::DeleteProgram(ms_nFilterEnvMapPID);
	CShader::DeleteProgram(ms_nPrecomputeBRDFPID);

	ms_nFilterEnvMapPID = INVALID_PROGRAM_HANDLE;
	ms_nPrecomputeBRDFPID = INVALID_PROGRAM_HANDLE;

	//CDeviceManager::DeleteConstantBuffer(ms_nConstantBufferID);

	//delete ms_pPacketList;
	//delete ms_pEnvMap;
	//delete ms_pFilteredEnvMap;
	//delete ms_pBakedEnvMap;
	//delete ms_pBrdfMap;
}


void CIBLBaker::BakeEnvMap()
{
	/*CRenderer::UseCamera(ms_nCameraID);
	CCamera* pCamera = CRenderer::GetCurrentCamera();

	CPacketManager::ForceShaderHook(CLightProbe::UpdateShader);

	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);
	//CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);

	for (int i = 0; i < 6; i++)
	{
		//CFramebuffer::BindCubeMap(0, ms_pEnvMap, i);
		//CFramebuffer::BindCubeMapDepthStencil(ms_pEnvMapFBO->GetDepthStencil(), i);

		CRenderer::ClearScreen();

		switch (i)
		{
		case 0:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(1.f, 0.f, 2.f));
			break;

		case 1:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(-1.f, 0.f, 2.f));
			break;

		case 2:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 2.f), float3(0.f, 1.f, 2.f));
			break;

		case 3:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 2.f), float3(0.f, -1.f, 2.f));
			break;

		case 4:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(0.f, 0.f, 3.f));
			break;

		case 5:
			pCamera->SetCamera(90.f, 1.f, 0.1f, 50.f, float3(0.f, -1.f, 0.f), float3(0.f, 0.f, 2.f), float3(0.f, 0.f, 1.f));
			break;
		}

		CRenderer::DrawPackets(e_RenderType_ReflectionPatches, CMaterial::e_Deferred);
	}

	CRenderer::ClearColor(0.f, 0.f, 0.f, 0.f);

	CRenderer::UseCamera(2);

	CPacketManager::ForceShaderHook(NULL);*/
}


void CIBLBaker::BakeSpecularIBLFromEnvMap()
{
	/*CFramebuffer::BindCubeMap(0, ms_pFilteredEnvMap, 0, 0, 0);

	CShader::BindProgram(ms_nFilterEnvMapPID);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetBlendingState(e_Opaque);

	CTexture::SetCubeMap(ms_nFilterEnvMapPID, ms_pEnvMap->m_nTextureId, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	unsigned int nMipMapCount = ms_pFilteredEnvMap->m_pTexture->GetMipMapCount();

	SConstants pregisters;
	pregisters.m_fSize = 1.f * ms_pFilteredEnvMap->m_nWidth;

	for (unsigned int i = 0; i < nMipMapCount; i++)
	{
		for (unsigned int face = 0; face < 6; face++)
		{
			CFramebuffer::BindCubeMap(0, ms_pFilteredEnvMap, face, 0, i);

			pregisters.m_fRoughness = i / (nMipMapCount - 1.f);
			pregisters.m_fFace = 1.f * face;

			CDeviceManager::FillConstantBuffer(ms_nConstantBufferID, &pregisters, sizeof(SConstants));
			CDeviceManager::BindConstantBuffer(ms_nConstantBufferID, ms_nFilterEnvMapPID, 0);

			CRenderer::RenderQuadScreen();
		}

		pregisters.m_fSize /= 2.f;
	}

	CFramebuffer::BindCubeMap(0, NULL, 0);*/
}


void CIBLBaker::BakeSpecularIBL(const char* cRadiancePath, const char* cBrdfPath, int nSize)
{
	/*SRenderTarget* pTarget = new SRenderTarget(nSize, nSize, e_R32G32B32A32_FLOAT, CTexture::eCubeMap, 1, 1, true);
	CTexture* pEnvMap = new CTexture(cRadiancePath);

	CFramebuffer::BindCubeMap(0, pTarget, 0, 0, 0);

	CShader::BindProgram(ms_nFilterEnvMapPID);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetBlendingState(e_Opaque);

	CTexture::SetCubeMap(ms_nFilterEnvMapPID, pEnvMap->m_nID, 0);
	CSampler::BindSamplerState(0, CSampler::e_MinMag_Linear_Mip_None_UVW_Clamp);

	unsigned int nMipMapCount = pTarget->m_pTexture->GetMipMapCount();

	SConstants pregisters;
	pregisters.m_fSize = 1.f * nSize;

	for (unsigned int i = 0; i < nMipMapCount; i++)
	{
		for (unsigned int face = 0; face < 6; face++)
		{
			CFramebuffer::BindCubeMap(0, pTarget, face, 0, i);

			pregisters.m_fRoughness = i / (nMipMapCount - 1.f);
			pregisters.m_fFace = 1.f * face;

			CDeviceManager::FillConstantBuffer(ms_nConstantBufferID, &pregisters, sizeof(SConstants));
			CDeviceManager::BindConstantBuffer(ms_nConstantBufferID, ms_nFilterEnvMapPID, 0);

			CRenderer::RenderQuadScreen();
		}

		pregisters.m_fSize /= 2.f;
	}

	CFramebuffer::BindCubeMap(0, NULL, 0);

	pTarget->m_pTexture->Save("../Data/Environments/EnvMapIBL.dds");

	glFinish();

	delete pEnvMap;
	delete pTarget;

	pTarget = new SRenderTarget(512, 512, e_R32G32B32A32_FLOAT, CTexture::eTexture2D);

	CFramebuffer::BindRenderTarget(0, pTarget);

	CShader::BindProgram(ms_nPrecomputeBRDFPID);

	CRenderer::RenderQuadScreen();

	CFramebuffer::BindRenderTarget(0, NULL);

	pTarget->m_pTexture->Save("../Data/Environments/BRDF.dds");

	glFinish();

	delete pTarget;*/
}
