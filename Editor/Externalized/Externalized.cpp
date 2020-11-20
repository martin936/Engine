#include "Externalized.h"


std::vector<CExternalized*> CExternalized::ms_pExternalizedVar;


EXTERNALIZE(int, gs_nPlayerCount, 2)
EXTERNALIZE(bool, gs_bLaunchPreview, false)
EXTERNALIZE(float, gs_fRadius, 0.2f)
EXTERNALIZE(int, gs_nPlayerTesselation, 2)
EXTERNALIZE(float, gs_fBounceForceDuration, 0.08f)



CExternalized::CExternalized(const char* pcName, float* pVariable, float fValue)
{
	m_eType = e_Float;
	m_pValue = pVariable;

	sprintf_s(m_cName, "%s", pcName);

	ms_pExternalizedVar.push_back(this);
}


CExternalized::CExternalized(const char* pcName, int* pVariable, int nValue)
{
	m_eType = e_Int;
	m_pValue = pVariable;

	sprintf_s(m_cName, "%s", pcName);

	ms_pExternalizedVar.push_back(this);
}


CExternalized::CExternalized(const char* pcName, bool* pVariable, bool bValue)
{
	m_eType = e_Bool;
	m_pValue = pVariable;

	sprintf_s(m_cName, "%s", pcName);

	ms_pExternalizedVar.push_back(this);
}


void CExternalized::WriteVariables()
{
	FILE* pFile = NULL;

	fopen_s(&pFile, EXTERNALIZED_PATH, "w+");

	if (pFile == NULL)
		return;

	std::vector<CExternalized*>::iterator it;

	for (it = ms_pExternalizedVar.begin(); it < ms_pExternalizedVar.end(); it++)
	{
		switch ((*it)->m_eType)
		{
		case e_Float:
			fprintf_s(pFile, "%s:%.3f\n", (*it)->m_cName, *(float*)((*it)->m_pValue));
			break;

		case e_Int:
			fprintf_s(pFile, "%s:%d\n", (*it)->m_cName, *(int*)((*it)->m_pValue));
			break;

		case e_Bool:
			fprintf_s(pFile, "%s:%s\n", (*it)->m_cName, *(bool*)((*it)->m_pValue) ? "true" : "false");
			break;

		default:
			break;
		}
	}

	fclose(pFile);
}


void CExternalized::ReadVariables(const char* pPath)
{
	FILE* pFile = NULL;

	fopen_s(&pFile, pPath, "r");

	if (pFile == NULL)
		return;

	char str[256] = "";
	char* pValue = NULL;

	std::vector<CExternalized*>::iterator it;

	while (fgets(str, 256, pFile) != NULL)
	{
		pValue = strchr(str, ':');
		if (pValue != NULL)
		{
			*pValue = '\0';
			pValue++;

			for (it = ms_pExternalizedVar.begin(); it < ms_pExternalizedVar.end(); it++)
			{
				if (strstr(str, (*it)->m_cName) != NULL)
				{
					switch ((*it)->m_eType)
					{
					case e_Float:
						sscanf_s(pValue, "%f", (float*)((*it)->m_pValue));
						break;

					case e_Int:
						sscanf_s(pValue, "%d", (int*)((*it)->m_pValue));
						break;

					case e_Bool:
						if (strstr(pValue, "true") != NULL)
							*(bool*)((*it)->m_pValue) = true;

						else if (strstr(pValue, "false") != NULL)
							*(bool*)((*it)->m_pValue) = false;
						break;

					default:
						break;
					}
				}
			}
		}
	}

	fclose(pFile);
}