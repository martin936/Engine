#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "AO.h"


CTexture*		CAO::ms_pSSAOTarget				= NULL;
CTexture*		CAO::ms_pBlendedSSAOTarget		= NULL;
CTexture*		CAO::ms_pLastSSAOTarget			= NULL;
CTexture*		CAO::ms_pBlurredTarget			= NULL;
CTexture*		CAO::ms_pSSAOFinalTarget		= NULL;
CTexture*		CAO::ms_pFinalTarget			= NULL;

float			CAO::ms_fKernel[64]				= { 0 };
float			CAO::ms_fKernelSize				= 1.f;
float			CAO::ms_fStrength				= 1.f;

bool			CAO::ms_bIsInit					= false;
bool			CAO::m_bFirstFrame				= false;

CTexture*		CAO::ms_pInterleavedDepth		= NULL;
CTexture*		CAO::ms_pInterleavedNormals		= NULL;
CTexture*		CAO::ms_pInterleavedHBAO		= NULL;
CTexture*		CAO::ms_pHBAOHistory			= NULL;
CTexture*		CAO::ms_pHBAOFinalTarget		= NULL;
CTexture*		CAO::ms_pHBAOBlendFactor		= NULL;

CAO::ETechnique CAO::ms_eTechnique				= CAO::e_HBAO;



void CAO::Init(ETechnique eTechnique)
{
	if (ms_bIsInit)
		return;

	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pBlendedSSAOTarget	= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R8G8B8A8);
	ms_pSSAOTarget			= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R8G8B8A8);

	ms_pLastSSAOTarget		= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R8G8B8A8);
	ms_pBlurredTarget		= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R8G8B8A8);
	ms_pSSAOFinalTarget		= new CTexture(nWidth / 2, nHeight / 2, ETextureFormat::e_R8G8B8A8);

	for (int i = 0; i < 16; ++i)
	{
		float3 kernel;
		float phi = Randf(0.f, 2.f * 3.141592f);
		float theta = Randf(0.1f, 3.141592f / 2.f);
		
		kernel.x = cosf(phi) * cosf(theta);
		kernel.y = sinf(phi) * cosf(theta);
		kernel.z = sinf(theta);

		float scale = 0.9f * i * i / 256.f + 0.1f;
		kernel = scale * kernel;

		memcpy(&(ms_fKernel[4 * i]), &(kernel.x), 4 * sizeof(float));
	}

	ms_pHBAOHistory			= new CTexture(nWidth, nHeight, ETextureFormat::e_R8, eTextureStorage2D);
	ms_pHBAOFinalTarget		= new CTexture(nWidth, nHeight, ETextureFormat::e_R8, eTextureStorage2D);
	ms_pHBAOBlendFactor		= new CTexture(nWidth, nHeight, ETextureFormat::e_R8_UINT, eTextureStorage2D);

	nWidth = (nWidth + 1) / 2;
	nHeight = (nHeight + 1) / 2;

	ms_pInterleavedDepth	= new CTexture((nWidth + 3) / 4, (nHeight + 3) / 4, 16, ETextureFormat::e_R16_FLOAT,	eTextureStorage2DArray);
	ms_pInterleavedNormals	= new CTexture((nWidth + 3) / 4, (nHeight + 3) / 4, 16, ETextureFormat::e_R8G8,			eTextureStorage2DArray);
	ms_pInterleavedHBAO		= new CTexture((nWidth + 3) / 4, (nHeight + 3) / 4, 16, ETextureFormat::e_R16G16_FLOAT,	eTextureStorage2DArray);

	ms_eTechnique = eTechnique;

	if (eTechnique == e_SSAO)
		ms_pFinalTarget = ms_pSSAOFinalTarget;

	else if (eTechnique == e_HBAO)
		ms_pFinalTarget = ms_pHBAOFinalTarget;

	InitHBAO();

	ms_bIsInit = true;
	m_bFirstFrame = true;
}
