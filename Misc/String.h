#ifndef __STRING_H__
#define __STRING_H__

#include <string.h>

class CString
{
public:

	static char* CropHead(char* pHead, char* pStr);
	static bool CropValue(char* pValue, char* pStr);

	static void GetDirectory(char* pDirectory, const char* pPath);
	static void GetFileName(char* pName, char* pPath);
};


#endif
