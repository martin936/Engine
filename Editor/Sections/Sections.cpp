#include "Sections.h"
#include "Engine/Editor/Editor.h"
#include "Engine/Physics/Physics.h"

CSection* CSection::ms_pMainSection = NULL;
bool CSection::ms_bInit = false;


CSection::CSection(const char* pName, const char* pParent)
{
	if (!ms_bInit)
		Init();

	CSection* pParentSection = GetSection(pParent, true);

	sprintf_s(m_cName, "%s", pName);

	m_SubSections.clear();
	m_Adjustables.clear();

	if (pParentSection != NULL)
		pParentSection->m_SubSections.push_back(this);

	if (pParentSection == NULL || pParentSection == ms_pMainSection)
		sprintf_s(m_cFullPath, "%s", m_cName);
	else
		sprintf_s(m_cFullPath, "%s/%s", pParentSection->m_cFullPath, m_cName);

	m_pParentSection = pParentSection;
}


CSection::CSection(const char* pName, CSection* pParent)
{
	if (pParent == NULL && !ms_bInit)
		ms_pMainSection = this;

	if (pParent != NULL)
		pParent->m_SubSections.push_back(this);

	sprintf_s(m_cName, "%s", pName);

	if (pParent == NULL || pParent == ms_pMainSection)
		sprintf_s(m_cFullPath, "%s", m_cName);
	else
		sprintf_s(m_cFullPath, "%s/%s", pParent->m_cFullPath, m_cName);

	m_SubSections.clear();
	m_Adjustables.clear();

	m_pParentSection = pParent;
}



void CSection::ReadAdjustables(const char* pPath)
{
	FILE* pFile = NULL;

	fopen_s(&pFile, pPath, "r");
	if (pFile == NULL)
		return;

	char str[512] = "";
	char* pcValue;
	CSection* pSection = NULL;
	std::vector<CAdjustable*>::iterator it;

	while (fgets(str, 512, pFile) != NULL)
	{
		pcValue = strchr(str, ':');

		if (pcValue != NULL)
		{
			*pcValue = '\0';
			pcValue++;

			char *ptr1, *ptr2 = str;

			while ((ptr1 = strchr(ptr2, '/')) != NULL)
			{
				ptr2 = ptr1 + 1;
			}

			if (ptr2 != str)
			{
				*(ptr2 - 1) = '\0';
				pSection = GetSection(str, false);
				
				for (it = pSection->m_Adjustables.begin(); it < pSection->m_Adjustables.end(); it++)
				{
					if (!strcmp((*it)->m_cName, ptr2))
					{
						switch ((*it)->m_eType)
						{
						case CAdjustable::e_Float:
							sscanf_s(pcValue, "%f", (float*)((*it)->m_pValue));
							(*it)->m_uDefaultValue.m_float = *(float*)((*it)->m_pValue);
							break;

						case CAdjustable::e_Int:
							sscanf_s(pcValue, "%d", (int*)((*it)->m_pValue));
							(*it)->m_uDefaultValue.m_int = *(int*)((*it)->m_pValue);
							break;

						case CAdjustable::e_Bool:
							if (strstr(pcValue, "true") != NULL)
								*(bool*)((*it)->m_pValue) = true;

							else if (strstr(pcValue, "false") != NULL)
								*(bool*)((*it)->m_pValue) = false;

							(*it)->m_uDefaultValue.m_bool = *(bool*)((*it)->m_pValue);
							break;

						default:
							break;
						}

						break;
					}
				}
			}
		}
	}

	fclose(pFile);
}



void CSection::WriteAdjustables(FILE* p_pFile)
{
	FILE* pFile = p_pFile;

	if (pFile == NULL)
		fopen_s(&pFile, ADJUSTABLE_PATH, "w+");

	std::vector<CAdjustable*>::iterator it;
	std::vector<CSection*>::iterator it2;

	for (it = m_Adjustables.begin(); it < m_Adjustables.end(); it++)
	{
		switch ((*it)->m_eType)
		{
		case CAdjustable::e_Float:
			fprintf_s(pFile, "%s/%s:%.3f\n", m_cFullPath, (*it)->m_cName, *(float*)((*it)->m_pValue));
			break;

		case CAdjustable::e_Int:
			fprintf_s(pFile, "%s/%s:%d\n", m_cFullPath, (*it)->m_cName, *(int*)((*it)->m_pValue));
			break;

		case CAdjustable::e_Bool:
			fprintf_s(pFile, "%s/%s:%s\n", m_cFullPath, (*it)->m_cName, *(bool*)((*it)->m_pValue) ? "true" : "false");
			break;

		default:
			break;
		}
	}

	for (it2 = m_SubSections.begin(); it2 < m_SubSections.end(); it2++)
	{
		(*it2)->WriteAdjustables(pFile);
	}

	if (p_pFile == NULL)
		fclose(pFile);
}



CSection* CSection::GetSection(const char* pcSection, bool bCreate)
{
	if (!ms_bInit)
		Init();

	if (pcSection == NULL)
		return ms_pMainSection;

	else if (pcSection[0] == '\0')
		return ms_pMainSection;

	std::vector<CSection*>::iterator it;
	const char* pPtr = strchr(pcSection, '/');
	char cHead[256] = "";
	int i = 0;

	if (pPtr != NULL)
		pPtr++;

	for (i = 0; i < 256; i++)
	{
		if (pcSection[i] == '/' || pcSection[i] == '\0')
			break;
		
		cHead[i] = pcSection[i];
	}

	cHead[i] = '\0';

	for (it = ms_pMainSection->m_SubSections.begin(); it < ms_pMainSection->m_SubSections.end(); it++)
	{
		if (!strcmp(cHead, (*it)->m_cName))
		{
			return (*it)->GetSubSection(pPtr, bCreate);
		}
	}

	if (bCreate)
	{
		CSection* pSection = new CSection(cHead, ms_pMainSection);
		return pSection->GetSubSection(pPtr, true);
	}

	return ms_pMainSection;
}


CSection* CSection::GetSubSection(const char* pcSubSection, bool bCreate)
{
	if (pcSubSection == NULL)
		return this;

	else if (pcSubSection[0] == '\0')
		return this;

	std::vector<CSection*>::iterator it;
	const char* pPtr = strchr(pcSubSection, '/');
	char cHead[256] = "";
	int i = 0;

	if (pPtr != NULL)
		pPtr++;

	for (i = 0; i < 256; i++)
	{
		if (pcSubSection[i] == '/' || pcSubSection[i] == '\0')
			break;

		cHead[i] = pcSubSection[i];
	}

	cHead[i] = '\0';

	for (it = m_SubSections.begin(); it < m_SubSections.end(); it++)
	{
		if (!strcmp(cHead, (*it)->m_cName))
		{
			return (*it)->GetSubSection(pPtr, bCreate);
		}
	}

	if (bCreate)
	{
		CSection* pSection = new CSection(cHead, m_cFullPath);
		return pSection->GetSubSection(pPtr, true);
	}
	else
		return this;
}