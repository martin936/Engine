#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Viewports/Viewports.h"


CRenderer::EDrawInfo CRenderer::ms_eDrawInfo = CRenderer::e_DrawAll;

extern SVertexElements g_VertexStreamSemantics[];
extern SVertexElements g_VertexStreamStandardSemantics[];
extern unsigned int g_VertexStreamStandardOffset[];
extern unsigned int g_VertexStreamOffsetSkin[];

thread_local bool CRenderer::ms_bStaticPacket = false;
thread_local bool CRenderer::ms_bEnableViewportCheck = true;


struct FragmentShaderConstants
{
	float	DiffuseColor[3];

	float	SpecAmplitude;
	float	SpecRoughness;
	float	SSSProfileID;
	float	SSSRadius;
};


void CRenderer::DrawPacket(Packet& packet, SShaderData p_pShaderData, CMaterial::ERenderType eRenderType)
{
	unsigned int nViewport = CViewportManager::GetCurrentViewport();

	if (ms_bEnableViewportCheck && !(packet.m_nViewportMask & (1ULL << nViewport)))
		return;

	if (packet.m_eType == Packet::e_EnginePacket)
		CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	else
		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

	p_pShaderData.m_nCurrentPass = 0;

	int(*pShaderHook)(Packet* packet, void* p_pShaderData) = CPacketManager::m_pShaderHook;
	if (pShaderHook == NULL)
	{
		if (packet.m_pMaterial != NULL)
		{
			if (eRenderType == CMaterial::e_Deferred)
				pShaderHook = packet.m_pMaterial->m_DeferredShaderHook;

			else if (eRenderType == CMaterial::e_Forward)
				pShaderHook = packet.m_pMaterial->m_ForwardShaderHook;

			else
				pShaderHook = packet.m_pShaderHook;
		}

		else
			pShaderHook = packet.m_pShaderHook;
	}
		

	SShaderData* pShaderData = &p_pShaderData;

	int numInstances = 1;

	while (pShaderHook(&packet, pShaderData) > 0)
	{
		numInstances = pShaderData->m_nNbInstances;

		if (numInstances == 0)
		{
			pShaderData->m_nCurrentPass++;
			continue;
		}

		unsigned int nVertexDeclarationMask = CShader::GetProgramVertexDeclarationMask(CPipelineManager::GetActiveProgram());

		std::vector<CDeviceManager::SStream> pStreams;
		unsigned int numStreams = 0;

		uint8_t maxVertexElement = (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine) ? e_MaxVertexElementUsage : e_MaxStandardVertexElementUsage;

		for (uint8_t i = 0; i < maxVertexElement; i++)
		{
			if (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine)
			{
				unsigned int streamMask = g_VertexStreamSemantics[i].m_StreamMask;

				if (nVertexDeclarationMask & streamMask)
				{
					if (g_VertexStreamSemantics[i].m_InputSlotClass == e_PerVertex)
					{
						ASSERT(packet.m_nVertexDeclaration & streamMask);

						pStreams.push_back({ numStreams, packet.m_nStreamBufferId[i], 0 });
					}

					else if (g_VertexStreamSemantics[i].m_InputSlotClass == e_PerInstance)
					{
						ASSERT(pShaderData->m_nInstancedStreamMask & streamMask);

						pStreams.push_back({ numStreams, pShaderData->m_nInstancedBufferID, pShaderData->m_nInstancedBufferByteOffset + g_VertexStreamOffsetSkin[i] });
					}

					numStreams++;
				}
			}

			else
			{
				unsigned int streamMask = g_VertexStreamStandardSemantics[i].m_StreamMask;

				if (nVertexDeclarationMask & streamMask)
				{
					if (g_VertexStreamStandardSemantics[i].m_InputSlotClass == e_PerVertex)
					{
						ASSERT(packet.m_nVertexDeclaration & streamMask);

						pStreams.push_back({ numStreams, packet.m_VertexBuffer, g_VertexStreamStandardOffset[i] });
					}

					else if (g_VertexStreamStandardSemantics[i].m_InputSlotClass == e_PerInstance)
					{
						ASSERT(pShaderData->m_nInstancedStreamMask & streamMask);

						pStreams.push_back({ numStreams, pShaderData->m_nInstancedBufferID, pShaderData->m_nInstancedBufferByteOffset + g_VertexStreamStandardOffset[i] });
					}

					numStreams++;
				}
			}
		}

		CDeviceManager::SetStreams(pStreams);

		if (packet.IsIndexed())
		{
			CDeviceManager::BindIndexBuffer(packet.m_IndexBuffer);
			CDeviceManager::DrawInstancedIndexed(packet.m_nFirstIndex, packet.m_nNumIndex, packet.m_nFirstVertex, 0, numInstances);
		}

		else
			CDeviceManager::DrawInstanced(packet.m_nFirstVertex, packet.m_nNumVertex, 0, numInstances);

		pShaderData->m_nCurrentPass++;
	}
}


void CRenderer::DrawPacketList(PacketList* packetlist, SShaderData& pShaderData, int nRenderFlags)
{
	std::vector<Packet>::iterator it;
	std::vector<Packet>& pList = packetlist->m_pPackets;

	for (it = pList.begin(); it < pList.end(); it++)
	{
		Packet& pPacket = *it;
		pShaderData.m_nPacket = pPacket;

		if (pPacket.m_pMaterial == NULL || (pPacket.m_pMaterial->GetRenderType() & nRenderFlags))
		{
			DrawPacket(pPacket, pShaderData, (nRenderFlags & CMaterial::e_Deferred) ? CMaterial::e_Deferred : CMaterial::e_Forward);
		}
	}
}


void CRenderer::DrawPackets(ERenderList nRenderList, int nRenderFlags)
{
	SShaderData pShaderData;

	std::vector<Drawable>& pList = CPacketManager::GetDrawListToFlush(nRenderList);
	size_t nNumDrawables = pList.size();

	CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	for (size_t i = 0; i < nNumDrawables; i++)
	{
		ms_bStaticPacket = pList[i].m_bIsStatic;

		if (((ms_eDrawInfo & e_DrawStatic) && pList[i].m_bIsStatic) || ((ms_eDrawInfo & e_DrawDynamic) && !pList[i].m_bIsStatic))
		{
			pShaderData.m_nInstancedBufferID			= pList[i].m_pPacketList.m_nInstanceBufferID;
			pShaderData.m_nInstancedBufferByteOffset	= pList[i].m_pPacketList.m_nInstancedBufferByteOffset;
			pShaderData.m_nInstancedStreamMask			= pList[i].m_pPacketList.m_nInstancedStreamMask;
			pShaderData.m_nInstancedBufferStride		= pList[i].m_pPacketList.m_nInstancedBufferStride;
			pShaderData.m_ModelMatrix					= pList[i].m_ModelMatrix;
			pShaderData.m_LastModelMatrix				= pList[i].m_LastModelMatrix;
			pShaderData.m_nNbInstances					= 1;

			if (pShaderData.m_nInstancedBufferID != INVALIDHANDLE)
			{
				pShaderData.m_nNbInstances = pList[i].m_pPacketList.m_nNbInstances;
			}

			DrawPacketList(&pList[i].m_pPacketList, pShaderData, nRenderFlags);
		}
	}
}

