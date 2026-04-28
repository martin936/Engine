#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/GameRenderPass.h"
#include "Engine/Renderer/Camera/CameraBase.h"
#include "Engine/Renderer/Window/Window.h"
#include "DebugDraw.h"
#include <math.h>




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


// Transform a clip-space point into world space using the current camera.
// The 3D debug pass re-applies ViewProj, so the point will land back on the
// intended clip-space location.
static float3 ClipToWorld(float2 clipXY, float clipZ)
{
	CCamera* pCamera = CRenderer::GetCurrentCamera();
	float4x4 invVP = pCamera->GetInvViewProjMatrix();

	float4 clip(clipXY.x, clipXY.y, clipZ, 1.f);
	float4 hw = invVP * clip;

	float invW = 1.f / hw.w;
	return float3(hw.x * invW, hw.y * invW, hw.z * invW);
}


void CDebugDraw::DrawLine2D(float2 P1, float2 P2, float4 Color)
{
	// Near plane under reverse-Z is z=1; use a value that is safely visible.
	const float kClipZ = 0.5f;

	float3 W1 = ClipToWorld(P1, kClipZ);
	float3 W2 = ClipToWorld(P2, kClipZ);

	PacketList* packets = CPacketBuilder::BuildLine(W1, W2, Color, CDebugDraw::UpdateShader);
	CPacketManager::AddPacketList(*packets, false, e_RenderType_3D_Debug);
}


void CDebugDraw::DrawVector2D(float2 Origin, float2 Vector, float fLength, float4 Color)
{
	float2 End(Origin.x + Vector.x * fLength, Origin.y + Vector.y * fLength);
	DrawLine2D(Origin, End, Color);
}


void CDebugDraw::DrawArrow2D(float2 Origin, float2 Vector, float fLength, float4 Color)
{
	float2 End(Origin.x + Vector.x * fLength, Origin.y + Vector.y * fLength);
	DrawLine2D(Origin, End, Color);

	// Arrowhead: two short segments from the tip, 150 deg off the direction.
	const float headLen = fLength * 0.25f;
	const float cosA    = -0.8660254f;  // cos(150 deg)
	const float sinA    =  0.5f;        // sin(150 deg)

	float2 head1(Vector.x * cosA - Vector.y * sinA, Vector.x * sinA + Vector.y * cosA);
	float2 head2(Vector.x * cosA + Vector.y * sinA, -Vector.x * sinA + Vector.y * cosA);

	float2 Tip1(End.x + head1.x * headLen, End.y + head1.y * headLen);
	float2 Tip2(End.x + head2.x * headLen, End.y + head2.y * headLen);

	DrawLine2D(End, Tip1, Color);
	DrawLine2D(End, Tip2, Color);
}


void CDebugDraw::DrawCircle2D(float2 Origin, float fRadius, float4 Color, int nSegments)
{
	if (nSegments < 3)
		nSegments = 3;

	float aspectRatio = CWindow::GetMainWindow()->GetAspectRatio();
	float2 prev(Origin.x + fRadius, Origin.y);
	for (int i = 1; i <= nSegments; ++i)
	{
		float a = (float)i / (float)nSegments * 6.28318530718f;
		float2 cur(Origin.x + cosf(a) * fRadius, Origin.y + sinf(a) * fRadius * aspectRatio);
		DrawLine2D(prev, cur, Color);
		prev = cur;
	}
}


int CDebugDraw::UpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	return 1;
}
