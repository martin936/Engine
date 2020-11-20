#include "Engine/Renderer/Renderer.h"
#include "Engine/Inputs/Inputs.h"
#include "Engine/Materials/MaterialDefinition.h"
#include "Engine/Imgui/imgui.h"
#include "MaterialEditor.h"


bool						CMaterialEditor::ms_bRayCastRequested = false;
bool						CMaterialEditor::ms_bIsCurrentMaterialModified = false;
unsigned int				CMaterialEditor::ms_nCurrentMatID = INVALIDHANDLE;
bool						CMaterialEditor::ms_bShouldDraw = false;
bool						CMaterialEditor::ms_bShouldReset = true;

std::vector<unsigned int>	CMaterialEditor::ms_ModifiedMaterials;

int															CMaterialEditor::ms_nMaterialIndex[2048];
std::vector<CMaterialEditor::STextureAssociationIds>		CMaterialEditor::ms_TextureAssociation;



void CMaterialEditor::ClearAll()
{
	ms_TextureAssociation.clear();
	ms_bShouldReset = true;
}



void CMaterialEditor::UpdateBeforeFlush()
{
	if (!ms_bShouldDraw)
		return;

	if (ms_nCurrentMatID != INVALIDHANDLE)
	{
		CMaterial::UpdateConstantBuffer(ms_nCurrentMatID);
	}

	if (ms_bIsCurrentMaterialModified)
	{
		AddModifiedMaterial(ms_nCurrentMatID);
	}
}


bool CMaterialEditor::IsMaterialModified(unsigned int nMatID)
{
	unsigned int numMatIDs = static_cast<unsigned int>(ms_ModifiedMaterials.size());

	for (unsigned int i = 0; i < numMatIDs; i++)
		if (ms_ModifiedMaterials[i] == nMatID)
			return true;

	return false;
}


void CMaterialEditor::AddModifiedMaterial(unsigned int nMatID)
{
	unsigned int numMatIDs = static_cast<unsigned int>(ms_ModifiedMaterials.size());

	for (unsigned int i = 0; i < numMatIDs; i++)
		if (ms_ModifiedMaterials[i] == nMatID)
			return;

	ms_ModifiedMaterials.push_back(nMatID);
}


void CMaterialEditor::RemoveModifiedMaterial(unsigned int nMatID)
{
	unsigned int numMatIDs = static_cast<unsigned int>(ms_ModifiedMaterials.size());

	for (unsigned int i = 0; i < numMatIDs; i++)
		if (ms_ModifiedMaterials[i] == nMatID)
		{
			ms_ModifiedMaterials.erase(ms_ModifiedMaterials.begin() + i);
			break;
		}

	if (nMatID == ms_nCurrentMatID)
		ms_bIsCurrentMaterialModified = false;
}


void CMaterialEditor::SaveAll()
{
	for (int i = static_cast<int>(ms_ModifiedMaterials.size()) - 1; i >= 0; i--)
		CMaterial::ExportMaterial(ms_ModifiedMaterials[i]);
}


void CMaterialEditor::ProcessMouse()
{
	CMouse* pMouse = CMouse::GetCurrent();

	float pos[2];
	pMouse->GetPos(&pos[0], &pos[1]);

	if (!ms_bRayCastRequested)
	{
		CDeferredRenderer::RequestRayCastMaterial(pos[0], pos[1]);
		ms_bRayCastRequested = true;
	}
}


void CMaterialEditor::Draw()
{
	if (ms_bShouldReset)
	{
		for (int i = 0; i < 2048; i++)
			ms_nMaterialIndex[i] = -1;

		ms_bShouldReset = false;
	}

	if (!ms_bShouldDraw)
		return;

	if (ms_bRayCastRequested && CDeferredRenderer::GetRayCastMaterial(ms_nCurrentMatID))
		ms_bRayCastRequested = false;

	ImGui::GetIO().MouseDrawCursor = true;

	DrawMaterialSelection();

	DrawMaterialProperties();
}


void CMaterialEditor::DrawMaterialSelection()
{
	ImGui::SetNextItemOpen(true);

	if (ImGui::TreeNode("Modified Materials"))
	{
		if (ImGui::Button("Save All"))
			SaveAll();

		ImGui::Dummy(ImVec2(0.f, 10.f));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0.f, 10.f));

		std::vector<const char*> names;

		unsigned int numMatIDs = static_cast<unsigned int>(ms_ModifiedMaterials.size());

		for (unsigned int i = 0; i < numMatIDs; i++)
			names.push_back(CMaterial::GetMaterialName(ms_ModifiedMaterials[i]));

		int nSelectedMaterialIndex = -1;

		if (ImGui::ListBox("Unsaved materials", &nSelectedMaterialIndex, names.data(), (int)names.size(), 10))
		{
			ms_nCurrentMatID = ms_ModifiedMaterials[nSelectedMaterialIndex];
		}

		ImGui::TreePop();
	}
}


void CMaterialEditor::DrawMaterialProperties()
{
	ImGui::SetNextItemOpen(true);

	if (ImGui::TreeNode("Material Properties"))
	{
		if (ms_nCurrentMatID != INVALIDHANDLE)
		{
			ms_bIsCurrentMaterialModified = IsMaterialModified(ms_nCurrentMatID);

			RegisterTextures();

			DrawHeader();

			DrawTexturesSection();

			DrawSpecularSection();

			DrawEmissiveSection();

			DrawSSSSection();

			DrawTransparencySection();
		}

		ImGui::TreePop();
	}
}


void CMaterialEditor::RegisterTextures()
{
	if (ms_nMaterialIndex[ms_nCurrentMatID] < 0)
	{
		CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

		unsigned int	nDiffuseTextureID	= mat.m_DiffuseTextureID;
		unsigned int	nNormalTextureID	= mat.m_NormalTextureID;
		unsigned int	nInfoTextureID		= mat.m_InfoTextureID;

		STextureAssociationIds texAsso;
		texAsso.m_nDiffuseID = nDiffuseTextureID;
		texAsso.m_nNormalID = nNormalTextureID;
		texAsso.m_nInfoID = nInfoTextureID;
		texAsso.m_nFlags = 0xffffffff;

		ms_nMaterialIndex[ms_nCurrentMatID] = static_cast<int>(ms_TextureAssociation.size());
		ms_TextureAssociation.push_back(texAsso);
	}
}


void CMaterialEditor::DrawHeader()
{
	char matName[512] = "";
	strcpy(matName, CMaterial::GetMaterialName(ms_nCurrentMatID));

	if (ms_bIsCurrentMaterialModified)
		strcat(matName, "*");

	ImGui::Text("%s", matName);

	char matPath[512] = "";
	strcpy(matPath, CMaterial::GetMaterialFullPath(ms_nCurrentMatID));
	ImGui::TextWrapped("Path:\n\t%s", matPath);

	ImGui::Separator();
	ImGui::Dummy(ImVec2(0.f, 20.f));

	if (ImGui::Button("Reload"))
	{
		CMaterial::ReloadMaterial(ms_nCurrentMatID);
	}

	ImGui::SameLine();

	if (ImGui::Button("Save"))
	{
		CMaterial::ExportMaterial(ms_nCurrentMatID);
	}

	ImGui::Dummy(ImVec2(0.f, 10.f));

	ImGui::Separator();

	ImGui::Dummy(ImVec2(0.f, 10.f));

	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	bool bIsMetallic		= mat.m_Metalness == 1;
	bool bSavedMetallicity	= bIsMetallic;

	bool isDielectric = !bIsMetallic;
	ImGui::Checkbox("Dielectric", &isDielectric);
	bIsMetallic = !isDielectric;

	ImGui::SameLine();

	ImGui::Checkbox("Metallic", &bIsMetallic);

	if (bIsMetallic != bSavedMetallicity)
	{
		mat.m_Metalness = bIsMetallic ? 1 : 0;
	}

	ImGui::Dummy(ImVec2(0.f, 10.f));
}


void CMaterialEditor::DrawTexturesSection()
{
	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	if (ImGui::TreeNode("Textures"))
	{
		bool bUseNormal = ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags & 1U;

		if (ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nNormalID != INVALIDHANDLE)
		{
			if (ImGui::Checkbox("Use Normal Map", &bUseNormal))
			{
				unsigned int nTextureID = bUseNormal ? ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nNormalID : INVALIDHANDLE;

				if (nTextureID != mat.m_NormalTextureID)
				{
					ms_bIsCurrentMaterialModified = true;
					mat.m_NormalTextureID = nTextureID;
				}
			}

			if (bUseNormal)
			{
				float fBumpHeight = mat.m_BumpHeight;

				ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags |= 1U;
				if (ImGui::SliderFloat("Bump Strength", &fBumpHeight, 0.f, 10.f))
				{
					ms_bIsCurrentMaterialModified = true;
					mat.m_BumpHeight = fBumpHeight;
				}
			}

			else
				ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags &= ~1U;
		}

		if (bUseNormal && ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nInfoID != INVALIDHANDLE)
		{
			bool bUseInfo = ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags & 2U;
			if (ImGui::Checkbox("Use Info Map", &bUseInfo))
			{
				unsigned int nTextureID = bUseInfo ? ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nInfoID : INVALIDHANDLE;

				if (nTextureID != mat.m_InfoTextureID)
				{
					ms_bIsCurrentMaterialModified = true;
					mat.m_InfoTextureID = nTextureID;
				}
			}

			if (bUseInfo)
				ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags |= 2U;

			else
				ms_TextureAssociation[ms_nMaterialIndex[ms_nCurrentMatID]].m_nFlags &= ~2U;
		}

		ImGui::Dummy(ImVec2(0.f, 10.f));

		ImGui::TreePop();
	}
}


void CMaterialEditor::DrawSpecularSection()
{
	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	if (ImGui::TreeNode("Specular"))
	{
		bool	bIsMetallic		= mat.m_Metalness == 1.f;
		float4	pfFresnel_Metal = mat.m_Fresnel;
		float	fFresnel		= mat.m_Reflectivity;
		float	fRoughness		= mat.m_Roughness;

		if (bIsMetallic)
		{
			if (ImGui::ColorPicker4("Reflectance", pfFresnel_Metal.v()))
			{
				ms_bIsCurrentMaterialModified = true;
				mat.m_Fresnel = pfFresnel_Metal;
			}
		}

		else
		{
			if (ImGui::SliderFloat("Reflectance", &fFresnel, 0.f, 1.f))
			{
				ms_bIsCurrentMaterialModified = true;
				mat.m_Reflectivity = fFresnel;
			}
		}

		if (ImGui::SliderFloat("Roughness", &fRoughness, 0.f, 1.f))
		{
			ms_bIsCurrentMaterialModified = true;
			mat.m_Roughness = fRoughness;
		}

		ImGui::Dummy(ImVec2(0.f, 10.f));

		ImGui::TreePop();
	}
}


void CMaterialEditor::DrawEmissiveSection()
{
	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	if (ImGui::TreeNode("Emissive"))
	{
		float	Emissive = mat.m_Emissive;

		if (ImGui::SliderFloat("Emissive Value", &Emissive, 0.f, 2500.f))
		{
			ms_bIsCurrentMaterialModified = true;
			mat.m_Emissive = Emissive;
		}

		ImGui::Dummy(ImVec2(0.f, 10.f));

		ImGui::TreePop();
	}
}


void CMaterialEditor::DrawSSSSection()
{
	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	if (ImGui::TreeNode("SSS"))
	{
		int		SSSProfile = mat.m_SSSProfileID;
		float	fSSSRadius = mat.m_SSSRadius;
		float	fThickness = mat.m_SSSThickness;

		bool bUseSSS = SSSProfile > 0;
		ImGui::Checkbox("Use SSS", &bUseSSS);

		if (bUseSSS)
		{
			if (ImGui::SliderInt("SSS Profile", &SSSProfile, 1, 4) || SSSProfile == 0)
			{
				if (SSSProfile == 0)
					SSSProfile = 1;

				ms_bIsCurrentMaterialModified = true;
				mat.m_SSSProfileID = SSSProfile;
			}

			if (ImGui::SliderFloat("SSS Radius", &fSSSRadius, 0.f, 10.f))
			{
				ms_bIsCurrentMaterialModified = true;
				mat.m_SSSRadius = fSSSRadius;
			}

			if (ImGui::SliderFloat("Thickness", &fThickness, 0.f, 10.f))
			{
				ms_bIsCurrentMaterialModified = true;
				mat.m_SSSThickness = fThickness;
			}
		}

		else
		{
			if (SSSProfile > 0)
			{
				ms_bIsCurrentMaterialModified = true;
				mat.m_SSSProfileID = 0;
			}
		}

		ImGui::Dummy(ImVec2(0.f, 10.f));

		ImGui::TreePop();
	}
}


void CMaterialEditor::DrawTransparencySection()
{
	CMaterial& mat = *CMaterial::ms_pMaterials[ms_nCurrentMatID];

	if (ImGui::TreeNode("Transparency"))
	{
		bool	bUseAlpha = mat.m_eRenderType == CMaterial::e_Forward;

		if (ImGui::Checkbox("Use Transparency", &bUseAlpha))
		{
			ms_bIsCurrentMaterialModified = true;			
			mat.m_eRenderType = bUseAlpha ? CMaterial::e_Forward : CMaterial::e_Deferred;
		}

		ImGui::Dummy(ImVec2(0.f, 10.f));

		ImGui::TreePop();
	}
}