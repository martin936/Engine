#ifndef __LIGHT_PROBES_H__
#define __LIGHT_PROBES_H__

#include <vector>
#include "Engine/Maths/Maths.h"
#include "Engine/Renderer/Renderer.h"





class CLightProbe
{
public:

	static void Init();
	static void Terminate();

	CLightProbe() {};
	~CLightProbe();

	void SetPosition(float3& Pos);
	static void Update();

	//inline static CTexture* GetEnvironmentMap() { return ms_pEnvironmentMap->m_pTexture; }
	//inline static CTexture* GetSHIrradiance() { return ms_pSHIrradiance->m_pTexture; }

	static int UpdateShader(Packet* packet, void* p_pShaderData);

private:

	/*static void PrecomputeSHLookupTable();
	static void ComputeSH();

	static CCamera* ms_pCamera;

	static CFramebuffer* ms_pFBO;
	static CFramebuffer* ms_pLookupFBO;

	static SRenderTarget* ms_pEnvironmentMap;

	static SRenderTarget* ms_pSHIrradiance;

	static unsigned int ms_nConstantBufferID;
	static unsigned int ms_nPID;

	static unsigned int ms_nLookupConstantBufferID;
	static unsigned int ms_nGenerateLookupPID;

	static unsigned int ms_nComputeSHPID;
	static unsigned int ms_nResetPID;

	static SRenderTarget* ms_pLookUpSH;

	static bool ms_bIsLookupTableReady;*/
};


#endif
