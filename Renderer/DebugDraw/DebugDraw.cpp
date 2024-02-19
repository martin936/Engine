#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/Device/DeviceManager.h"
#include "DebugDraw.h"




void DebugDraw_EntryPoint()
{
	CRenderer::SetViewProjConstantBuffer(0);

	CRenderer::DrawPackets(e_RenderType_3D_Debug);
}


void CDebugDraw::Init()
{
	if (CRenderPass::BeginGraphics(ERenderPassId::e_DebugDraw, "Debug Draw"))
	{
		CRenderPass::BindResourceToWrite(0, CDeferredRenderer::GetToneMappedTarget(), CRenderPass::e_RenderTarget);
		CRenderPass::BindDepthStencil(CDeferredRenderer::GetDepthTarget());

		CRenderer::SetVertexLayout(e_Vertex_Layout_Standard);

		CRenderPass::BindProgram("DebugDraw", "DebugDraw");

		CRenderPass::SetRasterizerState(ERasterFillMode::e_FillMode_WireFrame, ERasterCullMode::e_CullMode_None);
		CRenderPass::SetDepthState(true, ECmpFunc::e_CmpFunc_GEqual, false);

		CRenderPass::SetPrimitiveTopology(ETopology::e_Topology_LineList);

		CRenderPass::SetEntryPoint(DebugDraw_EntryPoint);

		CRenderPass::End();
	}
}


void CDebugDraw::DrawVector(float3& Origin, float3& Vector, float fLength, float4& Color)
{
	PacketList* packets = CPacketBuilder::BuildLine(Origin, Origin + fLength * Vector, Color, CDebugDraw::UpdateShader);
	CPacketManager::AddPacketList(*packets, false, e_RenderType_3D_Debug);
}


void CDebugDraw::DrawCircle(float3& Origin, float3& Normal, float fRadius, float4& Color)
{
	PacketList* packets = CPacketBuilder::BuildCircle(Origin, Normal, fRadius, Color, CDebugDraw::UpdateShader);
	CPacketManager::AddPacketList(*packets, false, e_RenderType_3D_Debug);
}


int CDebugDraw::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	return 1;
}
