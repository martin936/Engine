#ifndef __EXTERNALIZED_H__
#define __EXTERNALIZED_H__


#define EXTERNALIZED_PATH	"../Data/GD/fixed.cnf"


#include <vector>

class CExternalized
{
public:

	CExternalized(const char* pcName, float* pVariable, float fValue);
	CExternalized(const char* pcName, bool* pVariable, bool bValue);
	CExternalized(const char* pcName, int* pVariable, int nValue);

	static void WriteVariables();
	static void ReadVariables(const char* pPath = EXTERNALIZED_PATH);

private:

	static std::vector<CExternalized*> ms_pExternalizedVar;
	static bool ms_bIsVectorDefined;

	enum EType
	{
		e_Float,
		e_Bool,
		e_Int
	};

	EType m_eType;

	char m_cName[256];

	void*	m_pValue;
};


#define EXTERNALIZE(Type, Name, Value)		\
	Type Name = Value;						\
	CExternalized gs_Ext##Name(#Name, &Name, Value); \

#define EXPORT_EXTERN(Type, Name)			\
	extern Type Name;						\


#endif
