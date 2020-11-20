#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "VolumetricMedia.h"


CTexture*		CVolumetricMedia::ms_pScatteringValue	= nullptr;
CTexture*		CVolumetricMedia::ms_pScatteredLight	= nullptr;
CTexture*		CVolumetricMedia::ms_pIntegratedLight	= nullptr;


void CVolumetricMedia::Init()
{
	int nWidth	= CDeviceManager::GetDeviceWidth();
	int nHeight = CDeviceManager::GetDeviceHeight();

	ms_pScatteringValue = new CTexture(192, 128, 64, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);
	ms_pScatteredLight	= new CTexture(192, 128, 64, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage3D);
	ms_pIntegratedLight = new CTexture(nWidth, nHeight, ETextureFormat::e_R16G16B16A16_FLOAT, eTextureStorage2D);

	/*if (CRenderPass::BeginCompute("Fill Scattering Grid"))
	{
		CRenderPass::BindResourceToWrite(0, )

		CRenderPass::End();
	}*/
}




void CVolumetricMedia::FillGrid()
{
	
}



void CVolumetricMedia::ApplyLights()
{
	
}



void CVolumetricMedia::IntegrateRays()
{
	
}



void CVolumetricMedia::Merge()
{

}
