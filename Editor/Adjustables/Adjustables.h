#ifndef __ADJUSTABLES_H__
#define __ADJUSTABLES_H__


#define ADJUSTABLE_PATH "Data/GD/adjust.cnf"
#include <vector>
#include <stdio.h>

class CAdjustableCategory;

class CAdjustable
{
	friend class CSection;
	friend class CAdjustableCategory;
public:

	CAdjustable(const char* pLabel, const char* pcName, float* pVariable, float fValue, float fMin, float fMax, const char* pSection);
	CAdjustable(const char* pLabel, const char* pcName, bool* pVariable, bool bValue, bool bMin, bool bMax, const char* pSection);
	CAdjustable(const char* pLabel, const char* pcName, int* pVariable, int nValue, int nMin, int nMax, const char* pSection);
	CAdjustable(const char* pLabel, const char* pcName, void (*pCallback)(), const char* pSection);

	inline const char* GetName() const { return m_cName; }
	inline const char* GetLabel() const { return m_cLabel; }

	void ResetValue();
	void SetDefault();

	void Draw();

	// Persists every adjustable's current value to disk (INI-like format) and
	// restores them. Buttons are skipped — they have no state. Both return
	// false on I/O failure (file open failed, etc.).
	static bool WriteAdjustables(const char* pPath = ADJUSTABLE_PATH);
	static bool ReadAdjustables (const char* pPath = ADJUSTABLE_PATH);

	// Commits editor-side changes to the live variables. Driven by the engine
	// at the start of each gameplay frame; do not call directly. Within a
	// frame the live values are stable, which makes them safe to read from
	// any thread that runs strictly between frame boundaries.
	static void CommitFrameSnapshot();

	// O(1) lookup by full key in the form "<category-path>/<name>", or just
	// "<name>" for adjustables registered at the root. Returns nullptr if no
	// adjustable matches.
	static CAdjustable* Find(const char* pcFullKey);

private:

	void WriteValue(FILE* pFile) const;
	void SetValueFromString(const char* pcValue);

	enum EAdjustableType
	{
		e_Float,
		e_Bool,
		e_Int,
		e_Button
	};

	union UType
	{
		float	m_float;
		int		m_int;
		bool	m_bool;
	};

	EAdjustableType m_eType;
	void*	m_pValue;            // live variable; updated only at frame start.
	UType	m_uStaging;          // edited by ImGui; copied to *m_pValue by CommitFrameSnapshot.
	void  (*m_pCallback)();

	UType	m_uDefaultValue;

	char m_cName[256];
	char m_cLabel[256];
	char m_cFullKey[512];        // "<category-path>/<name>", used as hash-map key.

	UType	m_uMin;
	UType	m_uMax;
};


class CAdjustableCategory
{
	friend class CAdjustable;
public:

	CAdjustableCategory(const char* pcName);
	~CAdjustableCategory();

	static void InsertAdjustable(CAdjustable* pAdjust, const char* pcPath);

	static void DrawAll();

	void Draw();

	static void Terminate()
	{
		if (ms_pRoot != NULL)
			delete ms_pRoot;
	}

private:

	CAdjustableCategory * GetSubCategory(const char* pcName);
	CAdjustableCategory * FindSubCategoryByPath(const char* pcPath); // exact match, no insert.

	void AddAdjustable(CAdjustable* pAdjust);
	CAdjustable* FindAdjustable(const char* pcName);                 // direct children only.

	void WriteRecursive(FILE* pFile, const char* pcPathPrefix);
	static CAdjustable* FindByPathAndName(const char* pcPath, const char* pcName);

	char m_cName[256];

	static CAdjustableCategory*			ms_pRoot;

	std::vector<CAdjustableCategory*>	m_pSubCategories;
	std::vector<CAdjustable*>			m_pAdjustables;
};


#define ADJUSTABLE(Label, Type, Name, Value, Min, Max, Section)			\
	Type Name = Value;									\
	CAdjustable Adjust##Name(Label, #Name, &Name, Value, Min, Max, Section);	\


#define ADJUSTABLE_BUTTON(Label, Name, Callback, Section)			\
	CAdjustable AdjustButton##Name(Label, #Name, Callback, Section);	\


#define EXPORT_ADJUSTABLE(Type, Name)					\
	extern Type Name;									\


#endif
