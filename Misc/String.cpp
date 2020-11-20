#include "String.h"


char* CString::CropHead(char* pHead, char* pStr)
{
	int i = 0;
	int count = 0;
	int nSize = 256;

	while (i < nSize && pStr[i] != '\0' && pStr[i] != '\n')
	{
		if (pStr[i] != ' ' && pStr[i] != '\t')
		{
			pHead[count] = pStr[i];
			count++;
		}
		else if (count > 0)
			break;

		i++;
	}

	pHead[count] = '\0';

	return pStr + i;
}


bool CString::CropValue(char* pValue, char* pStr)
{
	int i = 0;
	int count = 0;
	int nSize = 256;

	bool bStart = false;

	while (i < nSize && pStr[i] != '\0' && pStr[i] != '\n')
	{
		if (bStart && pStr[i] != ' ' && pStr[i] != '\t')
		{
			pValue[count] = pStr[i];
			count++;
		}
		else if (count > 0)
			break;

		if (pStr[i] == '=' || pStr[i] == ':')
			bStart = true;

		i++;
	}

	pValue[count] = '\0';

	return bStart;
}


void CString::GetDirectory(char* pDirectory, const char* pPath)
{
	strncpy(pDirectory, pPath, 512);

	char* ptr = pDirectory;
	char* pTmp1 = pDirectory;
	char* pTmp = ptr;

	while (ptr != NULL)
	{
		pTmp = ptr;
		pTmp1 = strchr(ptr, '/');

		if (pTmp1 == NULL)
			ptr = strchr(ptr, '\\');

		else
			ptr = pTmp1;

		if (ptr != NULL)
			ptr++;
	}

	*pTmp = '\0';
}


void CString::GetFileName(char* pName, char* pPath)
{
	char* ptr = pPath;
	char* ptrTest = pPath;

	do
	{
		ptr = ptrTest;
		ptrTest = strchr(ptr, '/');

		if (ptrTest == NULL)
			ptrTest = strchr(ptr, '\\');

		if (ptrTest != NULL)
			ptrTest++;

	} while (ptrTest != NULL);

	strncpy(pName, ptr, 512);
}
