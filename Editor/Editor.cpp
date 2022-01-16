#include <time.h>
#include <iostream>
#include "Editor.h"
#include "LightEditor/LightEditor.h"
#include "MaterialEditor/MaterialEditor.h"
#include "Engine/Physics/Physics.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Editor/Adjustables/Adjustables.h"
#include "Externalized/Externalized.h"
#include "Engine/Imgui/imgui.h"
#include "Engine/Timer/Timer.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Renderer/AO/AO.h"
#include "Engine/Renderer/LightField/LightField.h"
#include "Engine/Renderer/Skybox/Skybox.h"
#include "Engine/Renderer/PostFX/DOF/DOF.h"
#include "Engine/Renderer/PostFX/Bloom/Bloom.h"
#include "Engine/Renderer/PostFX/ToneMapping/ToneMapping.h"


std::vector<CEditorItem*> CEditorItem::ms_pItemList;

CEditor::ETransformType				CEditor::ms_eTransformType	= CEditor::e_Transform_Translate;
CEditor::ETransformReferential		CEditor::ms_eTransformRef	= CEditor::e_TransformRef_Local;
std::vector<CEditor::SUndoEvent>	CEditor::ms_History;

CEditorItem*						CEditor::ms_pSelectedItem = nullptr;
CEditorItem							CEditor::ms_pSelectedItemSavedState;

float								CEditor::ms_fGizmoLength = 2.f;
bool								CEditor::ms_bIsUnderModification = false;
bool								CEditor::ms_bIsAxisGrabbed = false;


EXPORT_EXTERN(bool, gs_bFullscreen)
EXPORT_EXTERN(int, gs_nWindowWidth)
EXPORT_EXTERN(int, gs_nWindowHeight)


void CEditor::Init()
{
	CExternalized::ReadVariables();
	CExternalized::WriteVariables();

	//m_pToolBox = new CToolBox(NULL);

	/*
	CSection::ReadAdjustables();*/
}


void CEditor::Terminate()
{
	ClearAll();
}


void CEditor::Draw()
{
	if (ms_pSelectedItem != nullptr)
		DrawSelectionGizmo();

	ImGuiIO& io = ImGui::GetIO();

	io.ConfigWindowsResizeFromEdges = true;
	io.ConfigWindowsMoveFromTitleBarOnly = true;

	int width = CDeviceManager::GetDeviceWidth();
	int height = CDeviceManager::GetDeviceHeight();

	ImGui::SetNextWindowPos(ImVec2(width * 0.75f, 0.f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(width * 0.25f, 1.f * height), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(0.3f);

	ImGui::Begin("Editor");

	if (ImGui::BeginTabBar("Tabs"))
	{
		if (ImGui::BeginTabItem("Adjustables"))
		{
			CAdjustableCategory::DrawAll();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Lights"))
		{
			CLightEditor::ms_bActive = true;
			CLightEditor::Draw();
			ImGui::EndTabItem();
		}

		else
			CLightEditor::ms_bActive = false;

		if (ImGui::BeginTabItem("Features"))
		{
			DrawFeatures();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Materials"))
		{
			CMaterialEditor::ms_bShouldDraw = true;
			CMaterialEditor::Draw();
			ImGui::EndTabItem();
		}

		else
			CMaterialEditor::ms_bShouldDraw = false;

		ImGui::EndTabBar();
	}

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(0.3f);

	ImGui::Begin("Performance Counters", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	CTimerManager::PrintTimers();

	ImGui::End();
}



void CEditor::DrawFeatures()
{
	if (ImGui::TreeNode("Anti Aliasing"))
	{
		bool bSMAA = CRenderer::IsAAEnabled();
		ImGui::Checkbox("Enable SMAA", &bSMAA);
		CRenderer::EnableAA(bSMAA);

		bool bTAA = CRenderer::IsTAAEnabled();
		ImGui::Checkbox("TAA", &bTAA);
		CRenderer::EnableTAA(bTAA);

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Reflections"))
	{
		bool bSSR = CRenderer::IsSSREnabled();
		ImGui::Checkbox("SSR", &bSSR);
		CRenderer::EnableSSR(bSSR);

		ImGui::TreePop();
	}

	
	if (ImGui::TreeNode("Volumetrics"))
	{
		bool bVolumetrics = CRenderer::IsVolumetricsEnabled();
		ImGui::Checkbox("Enable Volumetrics", &bVolumetrics);
		CRenderer::EnableVolumetrics(bVolumetrics);

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("DOF"))
	{
		bool bDOF = CRenderer::IsDOFEnabled();
		ImGui::Checkbox("Enable DOF", &bDOF);
		CRenderer::EnableDOF(bDOF);

		if (bDOF)
		{
			float dist = CDOF::GetPlaneInFocus();
			ImGui::SliderFloat("Plane in Focus (m)", &dist, 0.f, 50.f);
			CDOF::SetPlaneInFocus(dist);

			float aperture = 100.f * CDOF::GetAperture();
			ImGui::SliderFloat("Aperture (mm)", &aperture, 0.f, 20.f);
			CDOF::SetAperture(aperture * 1e-2f);
		}

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Bloom"))
	{
		bool bBloom = CRenderer::IsBloomEnabled();
		ImGui::Checkbox("Enable Bloom", &bBloom);
		CRenderer::EnableBloom(bBloom);

		if (bBloom)
		{
			float intensity = CBloom::GetIntensity();
			ImGui::SliderFloat("Intensity", &intensity, 0.f, 0.1f);
			CBloom::SetIntensity(intensity);
		}

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Transparency"))
	{
		bool bAlpha = CRenderer::IsTransparencyEnabled();
		ImGui::Checkbox("Enable Transparency", &bAlpha);
		CRenderer::EnableTransparency(bAlpha);

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("AO"))
	{
		bool bAO = CRenderer::IsAOEnabled();
		ImGui::Checkbox("Enable SSRTGI", &bAO);
		CRenderer::EnableAO(bAO);

		if (bAO)
		{
			float size = CAO::GetKernelSize();
			ImGui::SliderFloat("Radius (m)", &size, 0.f, 10.f);
			CAO::SetKernelSize(size);

			float strength = CAO::GetAOStrength();
			ImGui::SliderFloat("Strength", &strength, 0.f, 5.f);
			CAO::SetAOStrength(strength);
		}

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("SSR"))
	{
		bool bSSR = CRenderer::IsSSREnabled();
		ImGui::Checkbox("Enable SSR", &bSSR);
		CRenderer::EnableSSR(bSSR);

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Indirect Diffuse"))
	{
		bool bDiffuseGI = CLightField::IsEnabled();
		ImGui::Checkbox("Enable Indirect Diffuse", &bDiffuseGI);
		CLightField::Enable(bDiffuseGI);

		if (bDiffuseGI)
		{
			bool bShowProbes = CLightField::ShouldShowIrradianceProbes();
			ImGui::Checkbox("Show Irradiance Probes", &bShowProbes);
			CLightField::ShowIrradianceProbes(bShowProbes);

			if (bShowProbes)
			{
				float size = CLightField::GetProbeDisplaySize();
				ImGui::SliderFloat("Probe Size (m)", &size, 0.f, 0.5f);
				CLightField::SetProbeDisplaySize(size);
			}
		}

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Tone Mapping"))
	{
		int nContrastLevel = CToneMapping::GetContrastLevel();
		ImGui::SliderInt("Contrast Level", &nContrastLevel, 0, 6);
		CToneMapping::SetContrastLevel(nContrastLevel);

		float factor = CToneMapping::GetEyeAdaptationFactor();
		ImGui::SliderFloat("Adaptation Speed", &factor, 0.f, 1.f);
		CToneMapping::SetEyeAdaptationFactor(factor);

		float black = CToneMapping::GetLowestBlack();
		ImGui::SliderFloat("Lowest Black", &black, 0.f, 0.1f);
		CToneMapping::SetLowestBlack(black);

		float white = CToneMapping::GetHighestWhite();
		ImGui::SliderFloat("Highest White", &white, black, 200.f);
		CToneMapping::SetHighestWhite(white);

		float bias = CToneMapping::GetEVBias();
		ImGui::SliderFloat("EV Bias", &bias, -5.f, 5.f);
		CToneMapping::SetEVBias(bias);

		ImGui::TreePop();
	}


	if (ImGui::TreeNode("Sky"))
	{
		float fSkyIntensity = CSkybox::GetSkyLightIntensity();
		ImGui::SliderFloat("Intensity", &fSkyIntensity, 0.f, 500.f);
		CSkybox::SetSkyLightIntensity(fSkyIntensity);

		ImGui::TreePop();
	}
}



CEditorItem::CEditorItem()
{
	m_Position	= 0.f;
	m_Scale		= 1.f;

	m_Axis[0] = float3(1.f, 0.f, 0.f);
	m_Axis[1] = float3(0.f, 1.f, 0.f);
	m_Axis[2] = float3(0.f, 0.f, 1.f);

	m_bIsMovable	= true;
	m_bIsScalable	= true;
	m_bHidden		= false;

	ms_pItemList.push_back(this);
}


CEditorItem::~CEditorItem()
{
	std::vector<CEditorItem*>::iterator it;

	for (it = ms_pItemList.begin(); it < ms_pItemList.end(); it++)
	{
		if (*it == this)
		{
			ms_pItemList.erase(it);
			break;
		}
	}
}


void CEditorItem::Copy(const CEditorItem& pItem)
{
	m_Position	= pItem.m_Position;
	m_Scale		= pItem.m_Scale;

	m_Axis[0]	= pItem.m_Axis[0];
	m_Axis[1]	= pItem.m_Axis[1];
	m_Axis[2]	= pItem.m_Axis[2];

	m_Axis[0].normalize();
	m_Axis[1].normalize();
	m_Axis[2].normalize();
}


void CEditorItem::Update()
{

}


void CEditorItem::Draw()
{

}


void CEditorItem::Select()
{
	if (CEditor::ms_pSelectedItem != this)
	{
		CEditor::ms_pSelectedItem = this;

		CEditor::ms_pSelectedItemSavedState.Copy(*this);
	}
}


void CEditor::ClearAll()
{
	std::vector<CEditorItem*>::iterator it;

	for (it = CEditorItem::ms_pItemList.begin(); it < CEditorItem::ms_pItemList.end(); it++)
		delete (*it);

	CEditorItem::ms_pItemList.clear();

	ClearHistory();

	ms_pSelectedItem = nullptr;

	CLightEditor::ClearAll();
}


void CEditor::ClearHistory()
{
	std::vector<SUndoEvent>::iterator it;

	for (it = ms_History.begin(); it < ms_History.end(); it++)
	{
		delete (*it).m_SavedState;
	}

	ms_History.clear();
}


void CEditor::Undo()
{
	if (ms_History.size() > 0)
	{
		SUndoEvent Event = ms_History.back();

		Event.m_ItemRef.Copy(*Event.m_SavedState);
		Event.m_ItemRef.Update();

		Event.m_ItemRef.Select();
		ms_pSelectedItemSavedState.Copy(Event.m_ItemRef);

		delete Event.m_SavedState;

		ms_History.pop_back();
	}
}


void CEditor::Process()
{
	CMouse::GetCurrent()->Process();
	CKeyboard::GetCurrent()->Process();

	CEditor::ProcessMouse();
	CEditor::ProcessKeyboard();

	if (CLightEditor::ms_bActive)
		CLightEditor::Update();

	Draw();
}


void CEditor::ProcessKeyboard()
{
	CKeyboard* pKeyboard = CKeyboard::GetCurrent();

	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	static bool bCanSwitchTransform = true;
	
	if (!bCanSwitchTransform)
		bCanSwitchTransform = pKeyboard->IsReleased(CKeyboard::e_Key_G);


	// ---- Keyboard inputs
	// ---- Ctrl +
	if ((pKeyboard->IsPressed(CKeyboard::e_Key_LControl) || pKeyboard->IsPressed(CKeyboard::e_Key_RControl)))
	{
		if (pKeyboard->IsPressed(CKeyboard::e_Key_W))
		{
			Undo();
		}

		else if (pKeyboard->IsPressed(CKeyboard::e_Key_D))
		{
			if (ms_pSelectedItem != nullptr)
				ms_pSelectedItem->Duplicate();
		}
	}

	else if (bCanSwitchTransform && pKeyboard->IsPressed(CKeyboard::e_Key_G))
	{
		ms_eTransformType = (ETransformType)((ms_eTransformType + 1) % ETransformType::e_Last);
		bCanSwitchTransform = false;
	}

	else if (pKeyboard->IsPressed(CKeyboard::e_Key_Delete))
	{
		if (ms_pSelectedItem != nullptr)
		{
			ms_pSelectedItem->Delete();
			ms_pSelectedItem = nullptr;
		}
	}

	else if (pKeyboard->IsPressed(CKeyboard::e_Key_Escape))
	{
		ms_pSelectedItem = nullptr;
	}
}


void CEditor::ProcessMouse()
{
	CMouse* pMouse = CMouse::GetCurrent();

	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (pMouse && pMouse->IsPressed(CMouse::e_Button_LeftClick))
	{
		if (ms_bIsUnderModification && pMouse->IsPressed(CMouse::e_Button_RightClick))
		{
			ms_pSelectedItem->Copy(ms_pSelectedItemSavedState);
			ms_pSelectedItem->Update();
			ms_bIsAxisGrabbed = false;
		}

		else if (ms_pSelectedItem != nullptr && ProcessSelectionGizmo())
		{
			ms_bIsUnderModification = true;
			ms_pSelectedItem->Update();
		}

		else if (!ms_bIsUnderModification)
		{
			if (!GrabItem() && CMaterialEditor::ms_bShouldDraw)
			{
				CMaterialEditor::ProcessMousePressed();
			}
		}
	}
	else
	{
		CMaterialEditor::ProcessMouseReleased();

		if (ms_bIsUnderModification)
		{
			ms_nSelectedAxis = -1;
			ms_bIsAxisGrabbed = false;
			ms_bIsUnderModification = false;

			if (HasSelectionChanged())
			{
				SUndoEvent Event(*ms_pSelectedItem, ms_pSelectedItemSavedState);

				ms_History.push_back(Event);

				ms_pSelectedItemSavedState.Copy(*ms_pSelectedItem);
			}
		}
	}
}


bool CEditor::HasSelectionChanged()
{
	if ((ms_pSelectedItem->GetPosition() - ms_pSelectedItemSavedState.GetPosition()).length() > 1e-3f)
		return true;

	if ((ms_pSelectedItem->GetScale() - ms_pSelectedItemSavedState.GetScale()).length() > 1e-3f)
		return true;

	for (int i = 0; i < 3; i++)
		if ((ms_pSelectedItem->GetAxis(i) - ms_pSelectedItemSavedState.GetAxis(i)).length() > 1e-3f)
			return true;

	return false;
}


void CEditor::UpdateBeforeFlush()
{
	if (CMaterialEditor::ms_bShouldDraw)
		CMaterialEditor::UpdateBeforeFlush();
}

