#include "Engine/Renderer/Renderer.h"
#include "Engine/Inputs/Inputs.h"
#include "AxisPicker.h"

CMesh* CAxisPicker::ms_pMesh = NULL;


void CAxisPicker::Init()
{
	ms_pMesh = NULL;// CMesh::LoadMesh("../Data/Models/Misc/Axis.mesh");
}


CAxisPicker::CAxisPicker()
{
	PacketList* pPackets = NULL;// CPacketManager::MeshToPacketList(ms_pMesh, false);

	/*m_pAxisPackets[0] = pPackets;
	m_pAxisPackets[1] = pPackets->next;
	m_pAxisPackets[2] = m_pAxisPackets[1]->next;
	m_pAxisPackets[3] = m_pAxisPackets[2]->next;

	for (int i = 0; i < 4; i++)
	{
		m_pAxisPackets[i]->packet->m_pShaderHook = CForwardRenderer::UpdateShader;
		m_pAxisPackets[i]->next = NULL;
	}*/

	m_ModelMatrix.Eye();

	m_Position = 0.f;
	m_Size = 0.3f;
	m_bGrabbed = false;
	m_nSelectedAxis = 0;

	ComputeModelMatrix();
}


void CAxisPicker::Update()
{
	CMouse* pMouse = CMouse::GetCurrent();

	if (m_bGrabbed && pMouse->IsPressed(CMouse::e_Button_RightClick))
	{
		m_Position = m_SavedPosition;
		m_bGrabbed = false;
	}

	if (!pMouse->IsPressed(CMouse::e_Button_LeftClick))
	{
		m_bGrabbed = false;
		return;
	}

	float4x4 InvViewProj = CRenderer::GetCurrentCamera()->GetInvViewProjMatrix();
	float3 CamPos = CRenderer::GetCurrentCamera()->GetPosition();
	float3 CurrentPos = m_Position;
	bool bShouldSave = false;
	
	float fMouseX, fMouseY;
	pMouse->GetPos(&fMouseX, &fMouseY);

	float4 vScreenMouse = float4(2.f*fMouseX - 1.f, 1.f - 2.f*fMouseY, 1.f, 1.f);

	float4 vWorldMouse = InvViewProj * vScreenMouse;
	float4 vReducedWorldMouse = vWorldMouse / vWorldMouse.w;

	float3 WorldMouse = float3(vReducedWorldMouse.x, vReducedWorldMouse.y, vReducedWorldMouse.z);
	float3 Dir = WorldMouse - CamPos;

	float t = -CamPos.y / Dir.y;

	float3 NewPos = CamPos + t*Dir;

	for (int i = 0; i < 4; i++)
	{
		float3 Dir = float3(0.f, 0.f, 0.f);
		if (i < 3)
			Dir.v()[(i+1)%3] = 1.f;

		if ((NewPos - m_Position - 0.8f*m_Size*Dir).length() < m_Size * 0.15f || (m_bGrabbed && m_nSelectedAxis == i))
		{
			bShouldSave = !m_bGrabbed;
			if (bShouldSave)
				m_Offset = m_Position - NewPos;

			if (bShouldSave || m_nSelectedAxis == i)
			{
				m_bGrabbed = true;
				m_nSelectedAxis = i;

				if (i < 3)
					m_Position.v()[(i + 1) % 3] = (NewPos + m_Offset).v()[(i + 1) % 3];
				else
					m_Position = NewPos + m_Offset;
			}
		}
	}

	if (bShouldSave)
		m_SavedPosition = CurrentPos;

	ComputeModelMatrix();
}


void CAxisPicker::ComputeModelMatrix(void)
{
	m_ModelMatrix.m00 = m_Size;
	m_ModelMatrix.m01 = 0.f;
	m_ModelMatrix.m02 = 0.f;
	m_ModelMatrix.m03 = m_Position.x;
	m_ModelMatrix.m10 = 0.f;
	m_ModelMatrix.m11 = m_Size;
	m_ModelMatrix.m12 = 0.f;
	m_ModelMatrix.m13 = m_Position.y;
	m_ModelMatrix.m20 = 0.f;
	m_ModelMatrix.m21 = 0.f;
	m_ModelMatrix.m22 = m_Size;
	m_ModelMatrix.m23 = m_Position.z;
	m_ModelMatrix.m30 = m_ModelMatrix.m31 = m_ModelMatrix.m32 = 0.f;
	m_ModelMatrix.m33 = 1.f;
}


void CAxisPicker::Draw()
{
	int i,j;
	float4 Color[4];

	for (i = 0; i < 4; i++)
	{
		Color[i].v()[i] = Color[i].v()[3] = 1.f;
		for (j = 0; j < 3; j++)
			if (j != i)
				Color[i].v()[j] = (i == m_nSelectedAxis) && m_bGrabbed ? 0.8f : 0.3f;

		CPacketManager::AddPacketList(*m_pAxisPackets[i], false, ERenderList::e_RenderType_3D_Debug);
	}
}


/*int CAxisPicker::UpdateShader(Packet* packet, void* p_pShaderData)
{
	if (packet->m_nCurrentPass > 0)
		return -1;

	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;
	CRenderer* pCurrent = CRenderer::GetInstance();

	//pCurrent->SetMaterial(packet->m_pMaterial);

	GLuint uniform = glGetUniformLocation(packet->m_pMaterial->ProgramId, "WorldViewProj");
	if (uniform > 0)
		glUniformMatrix4fv(uniform, 1, GL_TRUE, pShaderData->WorldViewProj->m);

	uniform = glGetUniformLocation(packet->m_pMaterial->ProgramId, "World");
	if (uniform > 0)
		glUniformMatrix4fv(uniform, 1, GL_TRUE, pShaderData->World->m);

	uniform = glGetUniformLocation(packet->m_pMaterial->ProgramId, "ModulateColor");
	if (uniform > 0)
		glUniform4fv(uniform, 1, &(pShaderData->ModulateColor->x));

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha | e_Depth);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetCullMode(e_CullBackCCW);
	CRenderStates::SetBlendingState(e_Opaque);
	CRenderStates::SetRasterizerState(e_Solid);

	return 1;
}*/