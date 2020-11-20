#include "Engine/Imgui/imgui.h"
#include "Adjustables.h"


CAdjustableCategory*	CAdjustableCategory::ms_pRoot = NULL;



CAdjustable::CAdjustable(const char* pLabel, const char* pcName, float* pVariable, float fValue, float fMin, float fMax, const char* pcSection)
{
	m_eType		= e_Float;
	m_pValue	= pVariable;

	m_uMin.m_float = fMin;
	m_uMax.m_float = fMax;

	m_uDefaultValue.m_float = *(float*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


CAdjustable::CAdjustable(const char* pLabel, const char* pcName, int* pVariable, int nValue, int nMin, int nMax, const char* pcSection)
{
	m_eType = e_Int;
	m_pValue = pVariable;

	m_uMin.m_int = nMin;
	m_uMax.m_int = nMax;

	m_uDefaultValue.m_int = *(int*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


CAdjustable::CAdjustable(const char* pLabel, const char* pcName, bool* pVariable, bool bValue, bool bMin, bool bMax, const char* pcSection)
{
	m_eType = e_Bool;
	m_pValue = pVariable;

	m_uMin.m_bool = bMin;
	m_uMax.m_bool = bMax;

	m_uDefaultValue.m_bool = *(bool*)m_pValue;

	sprintf_s(m_cName, "%s", pcName);
	sprintf_s(m_cLabel, "%s", pLabel);

	CAdjustableCategory::InsertAdjustable(this, pcSection);
}


void CAdjustable::ResetValue()
{
	switch (m_eType)
	{
	case e_Float:
		*(float*)m_pValue = m_uDefaultValue.m_float;
		break;

	case e_Int:
		*(int*)m_pValue = m_uDefaultValue.m_int;
		break;

	case e_Bool:
		*(bool*)m_pValue = m_uDefaultValue.m_bool;
		break;

	default:
		break;
	}
}


void CAdjustable::SetDefault()
{
	switch (m_eType)
	{
	case e_Float:
		m_uDefaultValue.m_float = *(float*)m_pValue;
		break;

	case e_Int:
		m_uDefaultValue.m_int = *(int*)m_pValue;
		break;

	case e_Bool:
		m_uDefaultValue.m_bool = *(bool*)m_pValue;
		break;

	default:
		break;
	}
}


void CAdjustable::Draw()
{
	switch (m_eType)
	{
	case e_Bool:
		ImGui::Checkbox(m_cLabel, (bool*)m_pValue);
		break;

	case e_Int:
		ImGui::SliderInt(m_cLabel, (int*)m_pValue, m_uMin.m_int, m_uMax.m_int);
		break;

	case e_Float:
		ImGui::SliderFloat(m_cLabel, (float*)m_pValue, m_uMin.m_float, m_uMax.m_float);
		break;

	default:
		break;
	}
}


CAdjustableCategory::CAdjustableCategory(const char* pcName)
{
	m_pSubCategories.clear();
	m_pAdjustables.clear();

	strcpy_s<256>(m_cName, pcName);
}


CAdjustableCategory::~CAdjustableCategory()
{
	std::vector<CAdjustableCategory*>::iterator it;

	for (it = m_pSubCategories.begin(); it < m_pSubCategories.end(); it++)
		delete *it;

	m_pSubCategories.clear();
	m_pAdjustables.clear();
}


CAdjustableCategory* CAdjustableCategory::GetSubCategory(const char* pcName)
{
	std::vector<CAdjustableCategory*>::iterator it;

	for (it = m_pSubCategories.begin(); it < m_pSubCategories.end(); it++)
	{
		if (!strcmp((*it)->m_cName, pcName))
			return *it;
	}

	CAdjustableCategory* pCategory = new CAdjustableCategory(pcName);
	m_pSubCategories.push_back(pCategory);

	return pCategory;
}


void CAdjustableCategory::AddAdjustable(CAdjustable* pAdjust)
{
	m_pAdjustables.push_back(pAdjust);
}


void CAdjustableCategory::InsertAdjustable(CAdjustable* pAdjust, const char* pcPath)
{
	if (ms_pRoot == NULL)
		ms_pRoot = new CAdjustableCategory("Root");

	if (pcPath == NULL || *pcPath == '\0')
		ms_pRoot->AddAdjustable(pAdjust);

	char path[256];
	strcpy_s<256>(path, pcPath);

	char* start = path;
	char* end;

	CAdjustableCategory* pCurrentCategory = ms_pRoot;

	do
	{
		end = strchr(start, '/');

		if (end != NULL)
			*end = '\0';

		pCurrentCategory = pCurrentCategory->GetSubCategory(start);

		if (end != NULL)
			start = end + 1;

	} while (end != NULL);

	pCurrentCategory->AddAdjustable(pAdjust);
}


void CAdjustableCategory::DrawAll()
{
	if (ms_pRoot != NULL)
	{
		std::vector<CAdjustableCategory*>::iterator it_cat;
		std::vector<CAdjustable*>::iterator it_adj;

		for (it_cat = ms_pRoot->m_pSubCategories.begin(); it_cat < ms_pRoot->m_pSubCategories.end(); it_cat++)
			(*it_cat)->Draw();

		if (ms_pRoot->m_pAdjustables.size() > 0)
			ImGui::Separator();

		for (it_adj = ms_pRoot->m_pAdjustables.begin(); it_adj < ms_pRoot->m_pAdjustables.end(); it_adj++)
			(*it_adj)->Draw();
	}
}


void CAdjustableCategory::Draw()
{
	if (ImGui::TreeNode(m_cName))
	{
		std::vector<CAdjustableCategory*>::iterator it_cat;
		std::vector<CAdjustable*>::iterator it_adj;

		for (it_cat = m_pSubCategories.begin(); it_cat < m_pSubCategories.end(); it_cat++)
			(*it_cat)->Draw();

		if (m_pAdjustables.size() > 0)
			ImGui::Separator();

		for (it_adj = m_pAdjustables.begin(); it_adj < m_pAdjustables.end(); it_adj++)
			(*it_adj)->Draw();

		ImGui::TreePop();

		if (m_pAdjustables.size() > 0)
			ImGui::Separator();
	}
}
