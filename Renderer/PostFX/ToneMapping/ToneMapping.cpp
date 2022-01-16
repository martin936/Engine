#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/RenderPass.h"
#include "Engine/Renderer/Renderer.h"
#include "ToneMapping.h"

CTexture*		CToneMapping::ms_p3DLUT				= NULL;
CTexture*		CToneMapping::ms_pContrastLUT[7]	= { NULL };
CTexture*		CToneMapping::ms_pHDHTarget			= NULL;
CTexture*		CToneMapping::ms_pAETarget			= NULL;

int				CToneMapping::ms_nCurrentContrast	= 2;

float			CToneMapping::ms_fEyeAdaptation = 0.3f;
float			CToneMapping::ms_fLowestBlack	= 0.f;
float			CToneMapping::ms_fHighestWhite	= 500.f;
float			CToneMapping::ms_fEVBias		= 0.f;

void ComputeHistogram_EntryPoint();
void ReduceHistogram_EntryPoint();
void ComputeAE_EntryPoint();
void ToneMapping_EntryPoint();


void CToneMapping::Init()
{
	int nWidth = CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();	

	ms_pHDHTarget	= new CTexture(128, (nHeight + 3) / 4, ETextureFormat::e_R32_UINT, eTextureStorage2D);
	ms_pAETarget	= new CTexture(1, 1, ETextureFormat::e_R16G16_FLOAT, eTextureStorage2D);

	ms_p3DLUT		= new CTexture("../../Data/LUTs/desat65cube.dds");

	ms_pContrastLUT[0] = new CTexture("../../Data/LUTs/Filmic_to_0-35_1-30.dds");
	ms_pContrastLUT[1] = new CTexture("../../Data/LUTs/Filmic_to_0-48_1-09.dds");
	ms_pContrastLUT[2] = new CTexture("../../Data/LUTs/Filmic_to_0-60_1-04.dds");
	ms_pContrastLUT[3] = new CTexture("../../Data/LUTs/Filmic_to_0-70_1-03.dds");
	ms_pContrastLUT[4] = new CTexture("../../Data/LUTs/Filmic_to_0-85_1-011.dds");
	ms_pContrastLUT[5] = new CTexture("../../Data/LUTs/Filmic_to_0.99_1-0075.dds");
	ms_pContrastLUT[6] = new CTexture("../../Data/LUTs/Filmic_to_1.20_1-00.dds");

	if (CRenderPass::BeginGraphics("ToneMapping"))
	{
		// Compute Histogram
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(), CShader::e_ComputeShader);

			CRenderPass::BindResourceToWrite(1, ms_pHDHTarget->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("BuildHistogram");

			CRenderPass::SetEntryPoint(ComputeHistogram_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Reduce Histogram
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_pHDHTarget->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("ReduceHistogram");

			CRenderPass::SetEntryPoint(ReduceHistogram_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Compute Auto-Exposure
		if (CRenderPass::BeginComputeSubPass())
		{
			CRenderPass::BindResourceToWrite(0, ms_pHDHTarget->GetID(), CRenderPass::e_UnorderedAccess);
			CRenderPass::BindResourceToWrite(1, ms_pAETarget->GetID(), CRenderPass::e_UnorderedAccess);

			CRenderPass::BindProgram("ComputeAutoExposure");

			CRenderPass::SetEntryPoint(ComputeAE_EntryPoint);

			CRenderPass::EndSubPass();
		}

		// Tone Mapping
		if (CRenderPass::BeginGraphicsSubPass())
		{
			CRenderPass::BindResourceToRead(0, CDeferredRenderer::GetMergeTarget(), CShader::e_FragmentShader);
			CRenderPass::BindResourceToRead(1, ms_pAETarget->GetID(),				CShader::e_FragmentShader);
			CRenderPass::SetNumTextures(2, 1);
			CRenderPass::SetNumTextures(3, 1);
			CRenderPass::SetNumSamplers(4, 1);

			CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetToneMappedTarget(), CRenderPass::e_RenderTarget);

			CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

			CRenderPass::BindProgram("ToneMapping", "ToneMapping");

			CRenderPass::SetEntryPoint(ToneMapping_EntryPoint);

			CRenderPass::EndSubPass();
		}

		CRenderPass::End();
	}
}



void ComputeHistogram_EntryPoint()
{
	int nHeight = CDeviceManager::GetDeviceHeight();

	CDeviceManager::Dispatch(1, (nHeight + 3) / 4, 1);
}


void ReduceHistogram_EntryPoint()
{
	unsigned int nHeight = CDeviceManager::GetDeviceHeight();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, &nHeight, sizeof(unsigned int));

	CDeviceManager::Dispatch(128, 1, 1);
}


void ComputeAE_EntryPoint()
{
	float params[5];
	params[0] = CToneMapping::GetEVBias();
	params[1] = CToneMapping::GetLowestBlack();
	params[2] = CToneMapping::GetHighestWhite();
	params[3] = CToneMapping::GetEyeAdaptationFactor();
	params[4] = CEngine::GetFrameDuration();

	CResourceManager::SetPushConstant(CShader::e_ComputeShader, params, sizeof(params));

	CDeviceManager::Dispatch(1, 1, 1);
}


void ToneMapping_EntryPoint()
{
	CTextureInterface::SetTexture(CToneMapping::ms_p3DLUT->GetID(), 2);
	CTextureInterface::SetTexture(CToneMapping::ms_pContrastLUT[CToneMapping::GetContrastLevel()]->GetID(), 3);
	CResourceManager::SetSampler(4, ESamplerState::e_MinMagMip_Linear_UVW_Clamp);

	CRenderer::RenderQuadScreen();
}



void CToneMapping::LoadSPI3D(const char* pcFileName)
{
	FILE* pFile = NULL;
	fopen_s(&pFile, pcFileName, "r");

	char str[512] = "";

	for (int i = 0; i < 3; i++)
		fgets(str, 512, pFile);

	int sizeX, sizeY, sizeZ;

	sscanf(str, " %d %d %d", &sizeX, &sizeY, &sizeZ);

	float4* pData = new float4[sizeX * sizeY * sizeZ];

	int i = 0;

	while (fgets(str, 512, pFile) != NULL)
	{
		sscanf_s(str, " %*d %*d %*d %f %f %f", &pData[i].x, &pData[i].y, &pData[i].z);
		pData[i].w = 0.f;
		
		i++;
	}

	CTexture* pTex = new CTexture(sizeX, sizeY, sizeZ, ETextureFormat::e_R32G32B32A32_FLOAT, eTexture3D, pData);

	char Filename[1024] = "";
	strcpy(Filename, pcFileName);

	char* ptr = Filename + strlen(Filename) - 1;

	while (*ptr != '.' && ptr > Filename)
		ptr--;

	strcpy(ptr, ".dds\0");

	pTex->Save(Filename);
}



void CToneMapping::LoadSPI1D(const char* pcFileName)
{
	FILE* pFile = NULL;
	fopen_s(&pFile, pcFileName, "r");

	char str[512] = "";

	for (int i = 0; i < 3; i++)
		fgets(str, 512, pFile);

	int size;

	sscanf(str, "%*[^ ] %d", &size);

	float* pData = new float[size];

	int i = 0;

	while (fgets(str, 512, pFile) != NULL)
	{
		if (sscanf_s(str, " %f", &pData[i]) > 0)		
			i++;
	}

	CTexture* pTex = new CTexture(size, 1, 1, ETextureFormat::e_R32_FLOAT, eTexture2D, pData);

	char Filename[1024] = "";
	strcpy(Filename, pcFileName);

	char* ptr = Filename + strlen(Filename) - 1;

	while (*ptr != '.' && ptr > Filename)
		ptr--;

	strcpy(ptr, ".dds\0");

	pTex->Save(Filename);
}

