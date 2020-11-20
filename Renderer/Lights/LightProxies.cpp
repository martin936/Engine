#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"
#include "LightsManager.h"



void CLightsManager::BuildLightProxies()
{
	PacketList*	pPacket	= NULL;

	std::vector<CPacketBuilder::SSphereInfo>	SphereBatchInfos;
	std::vector<CPacketBuilder::SConeInfo>		SpotBatchInfos;

	CPacketBuilder::SSphereInfo					sphereInfos;
	CPacketBuilder::SConeInfo					coneInfos;

	unsigned int nNbLights = 0;
	unsigned int nNbShadowLights = 0;

	unsigned int numVisibleLights = static_cast<unsigned int>(ms_pVisibleLightsToFill->size());

	for (unsigned int i = 0; i < numVisibleLights; i++)
	{
		CLight::SLightDesc& desc = (*ms_pVisibleLightsToFill)[i];

		switch (desc.m_nType)
		{
		case CLight::e_Omni:
		
			sphereInfos.m_Center	= desc.m_Pos;
			sphereInfos.m_fRadius	= desc.m_fMaxRadius * 1.1f;
			
			if (desc.m_nCastShadows)
			{
				sphereInfos.m_nLightID = (1 << 15) | nNbShadowLights;
				nNbShadowLights++;
			}

			else
			{
				sphereInfos.m_nLightID = nNbLights;				
				nNbLights++;
			}

			SphereBatchInfos.push_back(sphereInfos);
			break;
		
			
		case CLight::e_Spot:

			coneInfos.m_Origin		= desc.m_Pos;
			coneInfos.m_fRadius		= desc.m_fMaxRadius * 1.415f;
			coneInfos.m_Direction	= desc.m_Dir;
			coneInfos.m_fAngle		= desc.m_fAngleOut;

			if (desc.m_nCastShadows)
			{
				coneInfos.m_nLightID = (1 << 15) | nNbShadowLights;
				nNbShadowLights++;
			}

			else
			{
				coneInfos.m_nLightID = nNbLights;
				nNbLights++;
			}

			SpotBatchInfos.push_back(coneInfos);
			break;

		default:
			break;
		}
	}

	if (SphereBatchInfos.size() > 0)
	{
		pPacket = CPacketBuilder::BuildInstancedSphereBatch(SphereBatchInfos, CLightsManager::ClusteredUpdateShader);
		CPacketManager::AddPacketList(*pPacket, false, e_RenderType_Light);

		SphereBatchInfos.clear();
	}
	
	if (SpotBatchInfos.size() > 0)
	{
		pPacket = CPacketBuilder::BuildInstancedConeBatch(SpotBatchInfos, CLightsManager::ClusteredUpdateShader);
		CPacketManager::AddPacketList(*pPacket, false, e_RenderType_Light);

		SpotBatchInfos.clear();
	}
}
