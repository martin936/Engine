#include "Engine/Engine.h"
#include "Engine/Renderer/Lights/LightsManager.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Imgui/imgui.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Renderer/DebugDraw/DebugDraw.h"
#include "Engine/Renderer/Shadows/ShadowRenderer.h"
#include "LightEditor.h"


std::vector<CLightItem*>	CLightItem::ms_pLightList;

bool		CLightEditor::ms_bReloadLights = false;
bool		CLightEditor::ms_bActive = false;
CLightItem* CLightEditor::ms_pSelectedItem = NULL;


CLightItem :: CLightItem() : CEditorItem()
{
	m_eType			= e_LightType_Omni;
	m_fRadius		= 10.f;
	m_fInnerAngle	= 45.f;
	m_fOuterAngle	= 60.f;

	m_bCastShadows	= false;

	ms_pLightList.push_back(this);
}


void CLightItem::Select()
{
	if (CEditor::ms_pSelectedItem != this)
	{
		CLightEditor::ms_pSelectedItem = this;
	}

	CEditorItem::Select();
}


void CLightItem::Update()
{
	CLight::SLightDesc desc = m_pLight->GetDesc();

	if (desc.m_nType == CLight::e_Sun)
		desc.m_Pos		= m_Position - 0.5f * desc.m_fMaxRadius * desc.m_Dir;
	else
		desc.m_Pos		= m_Position;

	desc.m_fIntensity	= m_fPower;
	desc.m_Color		= m_Color;
	desc.m_nCastShadows	= m_bCastShadows ? 1 : 0;
	desc.m_nCastStaticShadows = desc.m_nCastShadows;
	desc.m_nCastDynamicShadows = desc.m_nCastShadows;

	desc.m_Up			= m_Axis[1];
	desc.m_Dir			= m_Axis[2];

	m_pLight->SetDesc(desc);
}


bool CLightItem::ShouldGrab(float* p_fItemDist)
{
	if (CLightEditor::ms_bActive)
	{
		float4x4 ViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
		float4 p = ViewProj * float4(m_Position, 1.f);
		p = (p / p.w) * 0.5f;
		p.x += 0.5f;
		p.y = 0.5f - p.y;

		CMouse* pMouse = CMouse::GetCurrent();

		float pos[2];
		pMouse->GetPos(&pos[0], &pos[1]);

		float3 Eye = CRenderer::GetCurrentCamera()->GetPosition();

		*p_fItemDist = (Eye - m_Position).length();

		if ((pos[0] - p.x) * (pos[0] - p.x) + (pos[1] - p.y) * (pos[1] - p.y) < 1e-4f)
			return true;
	}

	return false;
}


void CLightItem::Draw()
{
	float4 nColor = float4(1.f, 1.f, 0.f, 1.f);

	if (CEditor::ms_pSelectedItem == this)
		nColor = float4(1.f, 1.f, 1.f, 1.f);

	float3 forward = CRenderer::GetCurrentCamera()->GetDirection();

	float3 Eye = CRenderer::GetCurrentCamera()->GetPosition();
	float fov = CRenderer::GetCurrentCamera()->GetFOV();

	float d = (Eye - m_Position).length();

	float fLength = 0.01f * d * tanf(fov * 3.1415926f / 360.f);

	if (m_eType == e_LightType_Omni)
	{
		CDebugDraw::DrawCircle(m_Position, forward, fLength, nColor);
	}

	else if (m_eType == e_LightType_Spot)
	{
		CDebugDraw::DrawCircle(m_Position, forward, fLength, nColor);
		CDebugDraw::DrawVector(m_Position, m_Axis[2], fLength * 10.f, nColor);
	}

	else if (m_eType == e_LightType_Sun)
	{
		CDebugDraw::DrawCircle(m_Position, forward, fLength, nColor);
	}
}


void CLightEditor::Init()
{
	CLightItem::ms_pLightList.clear();
}


void CLightEditor::Terminate()
{

}


void CLightEditor::Update()
{
	if (ms_bReloadLights)
	{
		ReloadLights();
		ms_bReloadLights = false;
	}
}


void CLightEditor::Draw()
{
	std::vector<CLightItem*>::iterator it;

	for (it = CLightItem::ms_pLightList.begin(); it < CLightItem::ms_pLightList.end(); it++)
		(*it)->Draw();

	ImGui::SetNextItemOpen(true);

	if (ImGui::TreeNode("Light list"))
	{
		ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)0), ImVec2(0.f, 200.0f), true, 0);

		std::vector<CLightItem*>::iterator it;

		int nSelected = -1;
		int i = 0;

		if (ms_pSelectedItem != nullptr)
			for (it = CLightItem::ms_pLightList.begin(); it < CLightItem::ms_pLightList.end(); it++, i++)
				if (ms_pSelectedItem == *it)
				{
					nSelected = i;
					break;
				}

		i = 0;

		for (it = CLightItem::ms_pLightList.begin(); it < CLightItem::ms_pLightList.end(); it++, i++)
		{
			char str[256] = "";
			sprintf(str, "Light_%d", i + 1);

			if (ImGui::Selectable(str, nSelected == i))
			{
				nSelected = i;
				(*it)->Select();
			}
		}

		ImGui::EndChild();
		ImGui::TreePop();
	}

	ImGui::Spacing();

	ImGui::SetNextItemOpen(true);

	if (ImGui::TreeNode("Light Properties"))
	{
		if (ms_pSelectedItem != NULL)
		{
			CLight::SLightDesc desc = ms_pSelectedItem->m_pLight->GetDesc();

			if (desc.m_nType == CLight::e_Sun)
			{
				ImGui::ColorEdit3("Color", ms_pSelectedItem->m_Color.v());
				ImGui::InputFloat("Power", &ms_pSelectedItem->m_fPower, 1.f, 10.f, 3);
				ImGui::InputFloat("Culling Radius", &desc.m_fMaxRadius, 0.1f, 1.f, 3);
			}

			else
			{
				bool bEnabled = ms_pSelectedItem->m_pLight->IsEnabled();
				ImGui::Checkbox("Enable", &bEnabled);
				ms_pSelectedItem->m_pLight->Enable(bEnabled);

				ImGui::Spacing();

				ImGui::ColorEdit3("Color", ms_pSelectedItem->m_Color.v());
				ImGui::InputFloat("Power", &ms_pSelectedItem->m_fPower, 1.f, 10.f, 3);
				ImGui::InputFloat("Culling Radius", &desc.m_fMaxRadius, 0.1f, 1.f, 3);

				ImGui::Spacing();

				if (ms_pSelectedItem->m_pLight->m_Type == CLight::e_Spot)
				{
					ImGui::SliderFloat("Inner Angle", &desc.m_fAngleIn, 0.f, 180.f);
					ImGui::SliderFloat("Outer Angle", &desc.m_fAngleOut, 0.f, 180.f);

					ImGui::Spacing();
				}

				ImGui::Checkbox("Cast shadow", &ms_pSelectedItem->m_bCastShadows);

				ImGui::Spacing();

				if (ImGui::TreeNode("Position"))
				{
					ImGui::InputFloat("X", &ms_pSelectedItem->m_Position.x, 0.1f, 0.5f, 3);
					ImGui::InputFloat("Y", &ms_pSelectedItem->m_Position.y, 0.1f, 0.5f, 3);
					ImGui::InputFloat("Z", &ms_pSelectedItem->m_Position.z, 0.1f, 0.5f, 3);

					ImGui::TreePop();

					ImGui::Separator();
					ImGui::Spacing();
				}

				if (ImGui::TreeNode("Type"))
				{
					ImGui::BeginChild(ImGui::GetID((void*)(intptr_t)0), ImVec2(0.f, 50.0f), true, 0);

					int nSelected = ms_pSelectedItem->m_eType;

					if (ImGui::Selectable("Omni", nSelected == CLightItem::e_LightType_Omni))
					{
						nSelected = CLightItem::e_LightType_Omni;
						ms_pSelectedItem->m_eType = CLightItem::e_LightType_Omni;
						ms_pSelectedItem->m_pLight->m_Type = CLight::e_Omni;
						desc.m_nType = CLight::e_Omni;
					}

					if (ImGui::Selectable("Spot", nSelected == CLightItem::e_LightType_Spot))
					{
						nSelected = CLightItem::e_LightType_Spot;
						ms_pSelectedItem->m_eType = CLightItem::e_LightType_Spot;
						ms_pSelectedItem->m_pLight->m_Type = CLight::e_Spot;
						desc.m_nType = CLight::e_Spot;
					}

					ImGui::EndChild();
					ImGui::TreePop();

					ImGui::Separator();
					ImGui::Spacing();
				}
			}

			ms_pSelectedItem->m_pLight->SetDesc(desc);
		}

		ImGui::TreePop();
	}

	if (ms_pSelectedItem != NULL)
		ms_pSelectedItem->Update();
}


void CLightEditor::ClearAll()
{
	std::vector<CLightItem*>::iterator it;

	for (it = CLightItem::ms_pLightList.begin(); it < CLightItem::ms_pLightList.end(); it++)
		delete *it;
	
	CLightItem::ms_pLightList.clear();

	ms_pSelectedItem = NULL;
}


void CLightEditor::ReloadLights()
{
	ClearAll();

	int nNumLights = CLightsManager::GetLightsCount();

	for (int i = 0; i < nNumLights; i++)
	{
		CLightItem* pLightItem = new CLightItem();
			
		CLight* pLight = CLightsManager::GetLight(i);

		if (pLight->GetType() == CLight::e_Spot)
			pLightItem->m_eType = CLightItem::e_LightType_Spot;
		
		else if (pLight->GetType() == CLight::e_Omni)
			pLightItem->m_eType = CLightItem::e_LightType_Omni;
		
		else if (pLight->GetType() == CLight::e_Sun)
			pLightItem->m_eType = CLightItem::e_LightType_Sun;

		CLight::SLightDesc desc = pLight->GetDesc();

		if (desc.m_nType == CLight::e_Sun)
			pLightItem->m_Position	= desc.m_Pos + 0.5f * desc.m_fMaxRadius * desc.m_Dir;
		else
			pLightItem->m_Position	= desc.m_Pos;

		pLightItem->m_Color			= desc.m_Color;
		pLightItem->m_fPower		= desc.m_fIntensity;
		pLightItem->m_bCastShadows	= desc.m_nCastShadows == 1 ? true : false;

		if (pLightItem->m_eType == CLightItem::e_LightType_Spot)
		{
			pLightItem->m_Axis[2]		= desc.m_Dir;
			float3 ex(1.f, 0.f, 0.f);
			if (float3::dotproduct(ex, pLightItem->m_Axis[0]) > 0.95f)
				ex = float3(0.f, 1.f, 0.f);

			pLightItem->m_Axis[1] = float3::normalize(float3::cross(pLightItem->m_Axis[2], ex));
			pLightItem->m_Axis[0] = float3::normalize(float3::cross(pLightItem->m_Axis[1], pLightItem->m_Axis[2]));

			pLightItem->m_fInnerAngle	= desc.m_fAngleIn;
			pLightItem->m_fOuterAngle	= desc.m_fAngleOut;
		}

		else if (pLightItem->m_eType == CLightItem::e_LightType_Sun)
		{
			pLightItem->m_Axis[2] = desc.m_Dir;
			float3 ex(1.f, 0.f, 0.f);
			if (float3::dotproduct(ex, pLightItem->m_Axis[0]) > 0.95f)
				ex = float3(0.f, 1.f, 0.f);

			pLightItem->m_Axis[1] = float3::normalize(float3::cross(pLightItem->m_Axis[2], ex));
			pLightItem->m_Axis[0] = float3::normalize(float3::cross(pLightItem->m_Axis[1], pLightItem->m_Axis[2]));
		}

		pLightItem->m_pLight = pLight;
	}
}
