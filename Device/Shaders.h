#ifndef __SHADERS_H__
#define __SHADERS_H__


typedef unsigned int ProgramHandle;

#define INVALID_PROGRAM_HANDLE 0xffffffff


#include "Engine/Engine.h"
#include <vector>


class CShader
{
	friend class CDeviceManager;
	friend class CPipelineManager;

public:

	enum EShaderType
	{
		e_VertexShader		= 1,
		e_HullShader		= 2,
		e_DomainShader		= 4,
		e_GeometryShader	= 8,
		e_FragmentShader	= 16,
		e_ComputeShader		= 32
	};


	struct SShaderConstants
	{
		unsigned int	m_nSlot;
		unsigned int	m_nSize;
	};


	struct SShader
	{
		void*							m_pShader;
		unsigned int					m_nVertexDeclarationMask;
		std::vector<SShaderConstants>	m_nConstantBuffers;
		size_t							m_nPushConstantSize;
		char							m_cName[256];
	};


	struct SProgramDesc
	{
		unsigned int	m_nVertexShaderID;
		unsigned int	m_nHullShaderID;
		unsigned int	m_nDomainShaderID;
		unsigned int	m_nGeometryShaderID;
		unsigned int	m_nPixelShaderID;
		unsigned int	m_nComputeShaderID;

		SProgramDesc()
		{
			m_nVertexShaderID	= -1;
			m_nHullShaderID		= -1;
			m_nDomainShaderID	= -1;
			m_nGeometryShaderID = -1;
			m_nPixelShaderID	= -1;
			m_nComputeShaderID	= -1;
		}
	};


	static void BindProgram(ProgramHandle PID);

	static ProgramHandle LoadProgram(const char* cComputeShaderPath);
	static ProgramHandle LoadProgram(const char* cVertexShaderPath, const char* cFragmentShaderPath);
	static ProgramHandle LoadProgram(const char* cVertexShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath);
	static ProgramHandle LoadProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cFragmentShaderPath);
	static ProgramHandle LoadProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath);

	static void DeleteProgram(ProgramHandle nPID);

	static void Terminate();

	inline static unsigned int GetProgramVertexDeclarationMask(ProgramHandle nProgramID)
	{
		ASSERT(nProgramID < ms_ProgramDesc.size());
		ASSERT(ms_ProgramDesc[nProgramID].m_nVertexShaderID < ms_Shaders.size());

		return ms_Shaders[ms_ProgramDesc[nProgramID].m_nVertexShaderID].m_nVertexDeclarationMask;
	}

	inline static void SetShaderDirectory(const char* pcPath)
	{
		strcpy_s(ms_pcShaderDirectory, pcPath);
	}

	static ProgramHandle ms_nCurrentPID;

private:

	static std::vector<SProgramDesc>	ms_ProgramDesc;
	static std::vector<SShader>			ms_Shaders;

	static char ms_pcShaderDirectory[1024];

	static void GetShaderPath(char* outPath, const char* shaderName, const char* shaderExt);
	static unsigned int CreateShader(const char* shaderName, bool vertexDeclaration = false);

	static bool GetShaderCode(const char* cFolderPath, const char* cPath, char*** pcShaderCode, int** pnCodeLength, int* pnLineNumber);
	static bool HandleIncludes(const char* cFolderPath, const char* cPath);

#ifdef __OPENGL__
	typedef unsigned int ShaderHandle;

	static ShaderHandle CompileShader(EShaderType type, char** pcShaderCode, int* nLineLength, int nLineNumber, char** pOutputMessage);
	static void HandleErrorMessages(const char* cPath, char** pOutputMessage);
	
	static ProgramHandle LinkProgram(ShaderHandle ComputeShaderID, char** pOutputMessage);
	static ProgramHandle LinkProgram(ShaderHandle VertexShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage);
	static ProgramHandle LinkProgram(ShaderHandle VertexShaderID, ShaderHandle GeometryShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage);
	static ProgramHandle LinkProgram(ShaderHandle VertexShaderID, ShaderHandle HullShaderID, ShaderHandle DomainShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage);
	static ProgramHandle LinkProgram(ShaderHandle VertexShaderID, ShaderHandle HullShaderID, ShaderHandle DomainShaderID, ShaderHandle GeometryShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage);
#endif

	static std::vector<unsigned int> ms_nConstantsBindingPoint;
};


#endif
