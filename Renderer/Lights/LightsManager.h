#ifndef RENDERER_LIGHTING_INC
#define RENDERER_LIGHTING_INC


#include "Engine/Maths/Maths.h"
#include "Engine/Renderer/RenderTargets/RenderTarget.h"
#include "Engine/Renderer/Packets/Packet.h"
#include "Lights.h"


class CLightsManager
{
public:

	struct SLight
	{
		float4		m_Pos;
		float4		m_Color;
		float4		m_Dir;
	};


	struct SLightShadow
	{
		SLight		m_Light;

		float4		m_ShadowIndex;
		float4x4	m_ShadowMatrix;
	};

	static void				Init();
	static void				Terminate();

	static void				ComputeLighting();

	static CLight*			AddLight(CLight::ELightType type, bool bCastShadow);

	static void				DeleteLights();
	static void				UpdateLights();

	static void				PrepareForFlush();

	static void				BuildLightList();

	static void SetLightListConstantBuffer(int nSlot);
	static void SetShadowLightListConstantBuffer(int nSlot);

	static unsigned int		GetLightGridTexture()
	{
		return ms_pLinkedListHeadPtrGrid->GetID();
	}

	static unsigned int		GetLightIndexBuffer()
	{
		return ms_LightIndexBuffer;
	}

	static unsigned int		GetStaticLightGridTexture()
	{
		return ms_pLinkedListHeadPtrStaticGrid->GetID();
	}

	static unsigned int		GetStaticLightIndexBuffer()
	{
		return ms_StaticLightIndexBuffer;
	}

	inline static CLight*	GetLight(int nIndex)
	{
		return ms_pLights[nIndex];
	}

	inline static int		GetLightsCount()
	{
		return static_cast<int>(ms_pLights.size());
	}

	inline static unsigned int GetBRDFMap()
	{
		return ms_BDRFMap->GetID();
	}

	static void WaitForLightList();

private:

	static void				BuildLightProxies();

	static int				ClusteredUpdateShader(Packet* packet, void* p_pShaderData);

	static std::vector<CLight*> ms_pLights;

	static std::vector<CLight::SLightDesc> ms_pVisibleLights[2];

	static std::vector<CLight::SLightDesc>* ms_pVisibleLightsToFill;
	static std::vector<CLight::SLightDesc>* ms_pVisibleLightsToFlush;

	static CTexture*			ms_DummyTarget;
	static CTexture*			ms_BDRFMap;

	static CTexture*			ms_pLinkedListHeadPtrTexture;
	static BufferId				ms_LinkedListNodeCulling;

	static CTexture*			ms_pLinkedListHeadPtrGrid;
	static BufferId				ms_LinkedListNodeBuffer;
	static BufferId				ms_LightIndexBuffer;

	static CTexture*			ms_pLinkedListHeadPtrStaticGrid;
	static BufferId				ms_StaticLightIndexBuffer;

	static BufferId				ms_LightsListConstantBuffer;
	static BufferId				ms_ShadowLightsListConstantBuffer;

	static CEvent*				ms_pLightListReady;
};


#endif
