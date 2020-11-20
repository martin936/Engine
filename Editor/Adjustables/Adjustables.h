#ifndef __ADJUSTABLES_H__
#define __ADJUSTABLES_H__


#define ADJUSTABLE_PATH "../Data/GD/adjust.cnf"
#include <vector>

class CAdjustable
{
	friend class CSection;
public:

	CAdjustable(const char* pLabel, const char* pcName, float* pVariable, float fValue, float fMin, float fMax, const char* pSection);
	CAdjustable(const char* pLabel, const char* pcName, bool* pVariable, bool bValue, bool bMin, bool bMax, const char* pSection);
	CAdjustable(const char* pLabel, const char* pcName, int* pVariable, int nValue, int nMin, int nMax, const char* pSection);

	inline const char* GetName() const { return m_cName; }
	inline const char* GetLabel() const { return m_cLabel; }

	void ResetValue();
	void SetDefault();

	void Draw();

	//static void WriteAdjustables(FILE* pFile);
	//static void ReadAdjustables(const char* pPath = ADJUSTABLE_PATH);

private:

	enum EAdjustableType
	{
		e_Float,
		e_Bool,
		e_Int
	};

	union UType
	{
		float	m_float;
		int		m_int;
		bool	m_bool;
	};

	EAdjustableType m_eType;
	void*	m_pValue;

	UType	m_uDefaultValue;

	char m_cName[256];
	char m_cLabel[256];

	UType	m_uMin;
	UType	m_uMax;
};


class CAdjustableCategory
{
public:

	CAdjustableCategory::CAdjustableCategory(const char* pcName);
	CAdjustableCategory::~CAdjustableCategory();

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

	void AddAdjustable(CAdjustable* pAdjust);

	char m_cName[256];

	static CAdjustableCategory*			ms_pRoot;

	std::vector<CAdjustableCategory*>	m_pSubCategories;
	std::vector<CAdjustable*>			m_pAdjustables;
};


#define ADJUSTABLE(Label, Type, Name, Value, Min, Max, Section)			\
	Type Name = Value;									\
	CAdjustable Adjust##Name(Label, #Name, &Name, Value, Min, Max, Section);	\


#define EXPORT_ADJUSTABLE(Type, Name)					\
	extern Type Name;									\


#endif
