#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Imgui/imgui.h"
#include "Imgui_engine.h"


void CImGui_Impl::Draw()
{
	ImGui::Render();
	DrawPackets();
	NewFrame();
}


void CImGui_Impl::DrawPackets()
{
	ImDrawData * pData = ImGui::GetDrawData();
	if (pData == NULL)
		return;

	int nListCount = pData->CmdListsCount;
	int vtx_offset = 0;
	int idx_offset = 0;

	PacketList* packetlist = CPacketBuilder::GetNewPacketList();

	int width = CDeviceManager::GetDeviceWidth();
	int height = CDeviceManager::GetDeviceHeight();

	for (int n = 0; n < nListCount; n++)
	{
		const ImDrawList* cmd_list = pData->CmdLists[n];
		const ImDrawVert * pVertices = cmd_list->VtxBuffer.Data;

		idx_offset = 0;

		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				const ImDrawIdx * pIndices = cmd_list->IdxBuffer.Data + idx_offset;

				Packet* pPacket = CPacketBuilder::GetNewPacket(cmd_list->VtxBuffer.Size, pcmd->ElemCount);

				pPacket->m_eTopology	= EPrimitiveTopology::e_TriangleList;

				pPacket->m_pShaderHook = UpdateShader;

				pPacket->m_ScissorRec.x = MAX(pcmd->ClipRect.x, 0.f);
				pPacket->m_ScissorRec.y = MAX(pcmd->ClipRect.y, 0.f);
				pPacket->m_ScissorRec.z = pcmd->ClipRect.z;
				pPacket->m_ScissorRec.w = pcmd->ClipRect.w;

				pPacket->m_nVertexDeclaration = e_POSITIONMASK | e_TEXCOORDMASK | e_COLORMASK;

				float*			pfPositions		= CPacketBuilder::GetVertexBuffer(pPacket);
				unsigned int*	psPacketIndices	= CPacketBuilder::GetIndexBuffer(pPacket);

				for (int i = 0; i < pPacket->m_nNumVertex; i++)
				{
					const ImDrawVert& pVertex = pVertices[i];
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 0] = 2.f * pVertex.pos.x / width - 1.f;
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 1] = 1.f - 2.f * pVertex.pos.y / height;
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 2] = 0.f;
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 3] = pVertex.uv.x;
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 4] = pVertex.uv.y;
					pfPositions[i * g_VertexStrideStandard / sizeof(float) + 5] = *(float*)(&pVertex.col);
				}

				for (int i = 0; i < pPacket->m_nNumIndex; ++i)
					psPacketIndices[i] = pIndices[i];

				packetlist->m_pPackets.push_back(*pPacket);
			}
			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.Size;
	}

	CPacketManager::AddPacketList(*packetlist, false, e_RenderType_ImGui);
}



int CImGui_Impl::UpdateShader(Packet* packet, void* pData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)pData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	return 1;
}
