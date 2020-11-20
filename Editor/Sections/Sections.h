#ifndef __SECTIONS_H__
#define __SECTIONS_H__

#include "Engine/Editor/Adjustables/Adjustables.h"


class CSection
{
	friend class CEditor;
public:

	CSection(const char* pName, const char* pParent);
	CSection(const char* pName, CSection* pParent);

	~CSection() {}

	static CSection* GetSection(const char* cSection, bool bCreate);
	CSection* GetSubSection(const char* cSubSection, bool bCreate);

	inline static CSection* GetMainSection() { return ms_pMainSection; }

	inline void AddAdjustable(CAdjustable* adjustable) { m_Adjustables.push_back(adjustable); }

	void WriteAdjustables(FILE* pFile);

	static void ReadAdjustables(const char* pPath = ADJUSTABLE_PATH);

private:

	inline static void Init() { ms_pMainSection = new CSection("Adjustables", (CSection*)NULL); ms_bInit = true; }

	char m_cName[256];
	char m_cFullPath[512];

	CSection* m_pParentSection;

	std::vector<CSection*> m_SubSections;
	std::vector<CAdjustable*> m_Adjustables;

	static CSection* ms_pMainSection;

	static bool ms_bInit;
};


#endif
