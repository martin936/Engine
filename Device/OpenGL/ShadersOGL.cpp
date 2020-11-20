#include "Engine/Renderer/Renderer.h"
#include <string.h>

#ifdef _WIN32
#include "Windows.h"
#endif

ProgramHandle CShader::ms_nCurrentPID = 0;

std::vector<unsigned int> CShader::ms_nConstantsBindingPoint;

char CShader::ms_pcShaderDirectory[1024] = "./";


void CShader::BindProgram(ProgramHandle PID)
{
	if (ms_nCurrentPID != PID)
	{
		glUseProgram(PID);
		ms_nCurrentPID = PID;
	}
}


void CShader::DeleteProgram(ProgramHandle nPID)
{
	glDeleteProgram(nPID);
}


void CShader::DeleteAll()
{

}



void CShader::GetFinalName(char* pcPath, const char* pcFolder, const char* pcName, const char* pcExt)
{
	char str[1024] = "";

	strncpy(str, pcFolder, 512);

	char* ptr = str;

	while (*ptr != '\0')
	{
		if (*ptr == '/' || *ptr == '\\' || *ptr == ' ')
			*ptr = '_';

		ptr++;
	}

	if (*(ptr - 1) != '_')
		*(ptr - 1) = '_';

	sprintf(pcPath, "%s%s%s%s", ms_pcShaderDirectory, str, pcName, pcExt);
}



ProgramHandle CShader::LoadProgram(const char* cFolderPath, const char* cComputeShaderName, bool bPhysicsShader)
{
	char**	pcShaderCode = NULL;
	char*	pcOutputMessage = NULL;
	int*	pnLineLength = NULL;
	int		nLineNumber = 0;
	bool	bReturnStatus = false;

	ShaderHandle ComputeShaderID = -1;

	char cComputeShaderPath[1024] = "";
	char cSrcFolderPath[1024] = "";

#ifdef COPY_SHADERS

	char cCopyPath[1024] = "";

	GetFinalName(cCopyPath, cFolderPath, cComputeShaderName, COMPUTE_SHADER_EXTENSION);

	if (bPhysicsShader)
		sprintf(cSrcFolderPath, "%s", PHYSICS_SHADER_ROOT);
	else
		sprintf(cSrcFolderPath, "%s%s%s", SHADER_ROOT, cFolderPath, SHADER_LANG);

	sprintf(cComputeShaderPath, "%s%s%s", cSrcFolderPath, cComputeShaderName, COMPUTE_SHADER_EXTENSION);
#else

	GetFinalName(cComputeShaderPath, cFolderPath, cComputeShaderName, COMPUTE_SHADER_EXTENSION);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cComputeShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		ComputeShaderID = CompileShader(e_ComputeShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cComputeShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

	if (ComputeShaderID < 0)
		return -1;

	ProgramHandle ProgramID = LinkProgram(ComputeShaderID, &pcOutputMessage);

	if (pcOutputMessage != NULL)
	{
		fprintf(stderr, "Compiled compute shader %s\n", cComputeShaderPath);
	}

	HandleErrorMessages(NULL, &pcOutputMessage);

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cCopyPath, FALSE);

	remove("shader_code.glsl");
#endif

	if (ProgramID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(ProgramID + 1);
		ms_nConstantsBindingPoint[ProgramID] = INVALIDHANDLE;
	}

	return ProgramID;
}


ProgramHandle CShader::LoadProgram(const char* cFolderPath, const char* cVertexShaderName, const char* cFragmentShaderName)
{
	char**	pcShaderCode = NULL;
	char*	pcOutputMessage = NULL;
	int*	pnLineLength = NULL;
	int	nLineNumber = 0;
	bool	bReturnStatus = false;

	char cVertexShaderPath[1024] = "";
	char cFragmentShaderPath[1024] = "";

	char cSrcFolderPath[1024] = "";

#ifdef COPY_SHADERS

	char cVertexCopyPath[1024] = "";
	char cFragmentCopyPath[1024] = "";
	

	GetFinalName(cVertexCopyPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cFragmentCopyPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);

	sprintf(cSrcFolderPath, "%s%s%s", SHADER_ROOT, cFolderPath, SHADER_LANG);
	sprintf(cVertexShaderPath, "%s%s%s", cSrcFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	sprintf(cFragmentShaderPath, "%s%s%s", cSrcFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#else

	GetFinalName(cVertexShaderPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cFragmentShaderPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#endif

	ShaderHandle VertexShaderID = -1;
	ShaderHandle FragmentShaderID = -1;

	bReturnStatus = GetShaderCode(cSrcFolderPath, cVertexShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		VertexShaderID = CompileShader(e_VertexShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cVertexShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cVertexCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cFragmentShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		FragmentShaderID = CompileShader(e_FragmentShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cFragmentShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

	if (VertexShaderID < 0 || FragmentShaderID < 0)
		return -1;

	ProgramHandle ProgramID = LinkProgram(VertexShaderID, FragmentShaderID, &pcOutputMessage);

	if (pcOutputMessage != NULL)
	{
		fprintf(stderr, "Compiled vertex shader %s\n", cVertexShaderPath);
		fprintf(stderr, "Compiled fragment shader %s\n", cFragmentShaderPath);
	}

	HandleErrorMessages(NULL, &pcOutputMessage);

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cFragmentCopyPath, FALSE);

	remove("shader_code.glsl");
#endif

	if (ProgramID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(ProgramID + 1);
		ms_nConstantsBindingPoint[ProgramID] = INVALIDHANDLE;
	}

	return ProgramID;
}


ProgramHandle CShader::LoadProgram(const char* cFolderPath, const char* cVertexShaderName, const char* cGeometryShaderName, const char* cFragmentShaderName)
{
	char**	pcShaderCode = NULL;
	char*	pcOutputMessage = NULL;
	int*	pnLineLength = NULL;
	int	nLineNumber = 0;
	bool	bReturnStatus = false;

	ShaderHandle VertexShaderID = -1;
	ShaderHandle GeometryShaderID = -1;
	ShaderHandle FragmentShaderID = -1;

	char cVertexShaderPath[1024] = "";
	char cGeometryShaderPath[1024] = "";
	char cFragmentShaderPath[1024] = "";

	char cSrcFolderPath[1024] = "";

#ifdef COPY_SHADERS

	char cVertexCopyPath[1024] = "";
	char cGeometryCopyPath[1024] = "";
	char cFragmentCopyPath[1024] = "";

	GetFinalName(cVertexCopyPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cGeometryCopyPath, cFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	GetFinalName(cFragmentCopyPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);

	sprintf(cSrcFolderPath, "%s%s%s", SHADER_ROOT, cFolderPath, SHADER_LANG);
	sprintf(cVertexShaderPath, "%s%s%s", cSrcFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	sprintf(cGeometryShaderPath, "%s%s%s", cSrcFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	sprintf(cFragmentShaderPath, "%s%s%s", cSrcFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#else

	GetFinalName(cVertexShaderPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cGeometryShaderPath, cFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	GetFinalName(cFragmentShaderPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cVertexShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		VertexShaderID = CompileShader(e_VertexShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cVertexShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cVertexCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cGeometryShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		GeometryShaderID = CompileShader(e_GeometryShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cGeometryShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cGeometryCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cFragmentShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		FragmentShaderID = CompileShader(e_FragmentShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cFragmentShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

	if (VertexShaderID < 0 || FragmentShaderID < 0)
		return -1;

	ProgramHandle ProgramID = LinkProgram(VertexShaderID, GeometryShaderID, FragmentShaderID, &pcOutputMessage);

	if (pcOutputMessage != NULL)
	{
		fprintf(stderr, "Compiled vertex shader %s\n", cVertexShaderPath);
		fprintf(stderr, "Compiled geometry shader %s\n", cGeometryShaderPath);
		fprintf(stderr, "Compiled fragment shader %s\n", cFragmentShaderPath);
	}

	HandleErrorMessages(NULL, &pcOutputMessage);

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cFragmentCopyPath, FALSE);

	remove("shader_code.glsl");
#endif

	if (ProgramID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(ProgramID + 1);
		ms_nConstantsBindingPoint[ProgramID] = INVALIDHANDLE;
	}

	return ProgramID;
}

ProgramHandle CShader::LoadProgram(const char* cFolderPath, const char* cVertexShaderName, const char* cHullShaderName, const char* cDomainShaderName, const char* cFragmentShaderName)
{
	char**	pcShaderCode = NULL;
	char*	pcOutputMessage = NULL;
	int*	pnLineLength = NULL;
	int	nLineNumber = 0;
	bool	bReturnStatus = false;

	ShaderHandle VertexShaderID = -1;
	ShaderHandle HullShaderID = -1;
	ShaderHandle DomainShaderID = -1;
	ShaderHandle FragmentShaderID = -1;

	char cVertexShaderPath[1024] = "";
	char cHullShaderPath[1024] = "";
	char cDomainShaderPath[1024] = "";
	char cFragmentShaderPath[1024] = "";

	char cSrcFolderPath[1024] = "";

#ifdef COPY_SHADERS

	char cVertexCopyPath[1024] = "";
	char cHullCopyPath[1024] = "";
	char cDomainCopyPath[1024] = "";
	char cFragmentCopyPath[1024] = "";

	GetFinalName(cVertexCopyPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cHullCopyPath, cFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	GetFinalName(cDomainCopyPath, cFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	GetFinalName(cFragmentCopyPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);

	sprintf(cSrcFolderPath, "%s%s%s", SHADER_ROOT, cFolderPath, SHADER_LANG);
	sprintf(cVertexShaderPath, "%s%s%s", cSrcFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	sprintf(cHullShaderPath, "%s%s%s", cSrcFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	sprintf(cDomainShaderPath, "%s%s%s", cSrcFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	sprintf(cFragmentShaderPath, "%s%s%s", cSrcFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#else

	GetFinalName(cVertexShaderPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cHullShaderPath, cFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	GetFinalName(cDomainShaderPath, cFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	GetFinalName(cFragmentShaderPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cVertexShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		VertexShaderID = CompileShader(e_VertexShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cVertexShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cVertexCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cHullShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		HullShaderID = CompileShader(e_HullShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cHullShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cHullCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cDomainShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		DomainShaderID = CompileShader(e_DomainShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cDomainShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cDomainCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cFragmentShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		FragmentShaderID = CompileShader(e_FragmentShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cFragmentShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

	if (VertexShaderID < 0 || FragmentShaderID < 0)
		return -1;

	ProgramHandle ProgramID = LinkProgram(VertexShaderID, HullShaderID, DomainShaderID, FragmentShaderID, &pcOutputMessage);

	if (pcOutputMessage != NULL)
	{
		fprintf(stderr, "Compiled vertex shader %s\n", cVertexShaderPath);
		fprintf(stderr, "Compiled hull shader %s\n", cHullShaderPath);
		fprintf(stderr, "Compiled domain shader %s\n", cDomainShaderPath);
		fprintf(stderr, "Compiled fragment shader %s\n", cFragmentShaderPath);
	}

	HandleErrorMessages(NULL, &pcOutputMessage);

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cFragmentCopyPath, FALSE);

	remove("shader_code.glsl");
#endif

	if (ProgramID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(ProgramID + 1);
		ms_nConstantsBindingPoint[ProgramID] = INVALIDHANDLE;
	}

	return ProgramID;
}

ProgramHandle CShader::LoadProgram(const char* cFolderPath, const char* cVertexShaderName, const char* cHullShaderName, const char* cDomainShaderName, const char* cGeometryShaderName, const char* cFragmentShaderName)
{
	char**	pcShaderCode = NULL;
	char*	pcOutputMessage = NULL;
	int*	pnLineLength = NULL;
	int	nLineNumber = 0;
	bool	bReturnStatus = false;

	ShaderHandle VertexShaderID = -1;
	ShaderHandle HullShaderID = -1;
	ShaderHandle DomainShaderID = -1;
	ShaderHandle GeometryShaderID = -1;
	ShaderHandle FragmentShaderID = -1;

	char cVertexShaderPath[1024] = "";
	char cHullShaderPath[1024] = "";
	char cDomainShaderPath[1024] = "";
	char cGeometryShaderPath[1024] = "";
	char cFragmentShaderPath[1024] = "";

	char cSrcFolderPath[1024] = "";

#ifdef COPY_SHADERS

	char cVertexCopyPath[1024] = "";
	char cHullCopyPath[1024] = "";
	char cDomainCopyPath[1024] = "";
	char cGeometryCopyPath[1024] = "";
	char cFragmentCopyPath[1024] = "";

	GetFinalName(cVertexCopyPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cHullCopyPath, cFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	GetFinalName(cDomainCopyPath, cFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	GetFinalName(cGeometryCopyPath, cFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	GetFinalName(cFragmentCopyPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);

	sprintf(cSrcFolderPath, "%s%s%s", SHADER_ROOT, cFolderPath, SHADER_LANG);
	sprintf(cVertexShaderPath, "%s%s%s", cSrcFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	sprintf(cHullShaderPath, "%s%s%s", cSrcFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	sprintf(cDomainShaderPath, "%s%s%s", cSrcFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	sprintf(cGeometryShaderPath, "%s%s%s", cSrcFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	sprintf(cFragmentShaderPath, "%s%s%s", cSrcFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#else

	GetFinalName(cVertexShaderPath, cFolderPath, cVertexShaderName, VERTEX_SHADER_EXTENSION);
	GetFinalName(cHullShaderPath, cFolderPath, cHullShaderName, HULL_SHADER_EXTENSION);
	GetFinalName(cDomainShaderPath, cFolderPath, cDomainShaderName, DOMAIN_SHADER_EXTENSION);
	GetFinalName(cGeometryShaderPath, cFolderPath, cGeometryShaderName, GEOMETRY_SHADER_EXTENSION);
	GetFinalName(cFragmentShaderPath, cFolderPath, cFragmentShaderName, FRAGMENT_SHADER_EXTENSION);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cVertexShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		VertexShaderID = CompileShader(e_VertexShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cVertexShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cVertexCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cHullShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		HullShaderID = CompileShader(e_HullShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cHullShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cHullCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cDomainShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		DomainShaderID = CompileShader(e_DomainShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cDomainShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cDomainCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cGeometryShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		GeometryShaderID = CompileShader(e_GeometryShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cGeometryShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cGeometryCopyPath, FALSE);
#endif

	bReturnStatus = GetShaderCode(cSrcFolderPath, cFragmentShaderPath, &pcShaderCode, &pnLineLength, &nLineNumber);
	if (bReturnStatus)
	{
		FragmentShaderID = CompileShader(e_FragmentShader, pcShaderCode, pnLineLength, nLineNumber, &pcOutputMessage);
		HandleErrorMessages(cFragmentShaderPath, &pcOutputMessage);
	}
	else
		fprintf(stderr, "Aborting compilation\n\n");

	if (VertexShaderID < 0 || FragmentShaderID < 0)
		return -1;

	ProgramHandle ProgramID = LinkProgram(VertexShaderID, HullShaderID, DomainShaderID, GeometryShaderID, FragmentShaderID, &pcOutputMessage);

	if (pcOutputMessage != NULL)
	{
		fprintf(stderr, "Compiled vertex shader %s\n", cVertexShaderPath);
		fprintf(stderr, "Compiled hull shader %s\n", cHullShaderPath);
		fprintf(stderr, "Compiled domain shader %s\n", cDomainShaderPath);
		fprintf(stderr, "Compiled geometry shader %s\n", cGeometryShaderPath);
		fprintf(stderr, "Compiled fragment shader %s\n", cFragmentShaderPath);
	}

	HandleErrorMessages(NULL, &pcOutputMessage);

#ifdef COPY_SHADERS
	CopyFile("shader_code.glsl", cFragmentCopyPath, FALSE);

	remove("shader_code.glsl");
#endif

	if (ProgramID >= ms_nConstantsBindingPoint.size())
	{
		ms_nConstantsBindingPoint.resize(ProgramID + 1);
		ms_nConstantsBindingPoint[ProgramID] = INVALIDHANDLE;
	}

	return ProgramID;
}


bool WriteNextLine(FILE* pSrc, FILE* pDest, const char* cFolderPath)
{
	char cLine[1024] = "";
	char cFileName[1024] = "";
	char cFilePath[1024] = "";
	char cNewFolderPath[1024] = "";
	char* ptr = NULL;
	char* endPtr = NULL;
	char* startPtr = NULL;

	int i;

	if (fgets(cLine, 1024, pSrc) != NULL)
	{
		if ((ptr = strstr(cLine, "#include ")) == NULL)
		{
			fputs(cLine, pDest);
		}

		else
		{
			startPtr = strchr(ptr, '"');
			if (startPtr == NULL)
				return true;

			startPtr++;
			endPtr = strchr(startPtr, '"');
			if (endPtr == NULL)
				return true;

			ptr = startPtr;
			i = 0;
			while (ptr < endPtr)
			{
				cFileName[i] = *ptr;
				ptr++;
				i++;
			}

			cFileName[i] = '\0';

			sprintf(cFilePath, "%s%s", cFolderPath, cFileName);

			FILE* pIncFile = fopen(cFilePath, "r");
			if (pIncFile == NULL)
				return true;

			startPtr = cFilePath;
			endPtr = cFilePath + strlen(cFilePath) - 1;
			while (endPtr > startPtr && *endPtr != '/' && *endPtr != '\\')
				endPtr--;

			ptr = startPtr;
			i = 0;
			while (ptr <= endPtr)
			{
				cNewFolderPath[i] = *ptr;
				ptr++;
				i++;
			}

			cNewFolderPath[i] = '\0';

			while (WriteNextLine(pIncFile, pDest, cNewFolderPath));

			fclose(pIncFile);
		}

		return true;
	}

	return false;
}


bool CShader::HandleIncludes(const char* cFolderPath, const char* cPath)
{
	FILE* pMain = fopen(cPath, "r");

	if (pMain == NULL)
	{
		fprintf(stderr, "Could not find shader %s\n", cPath);
		return false;
	}

	FILE* pDest = fopen("shader_code.glsl", "w+");

	while (WriteNextLine(pMain, pDest, cFolderPath));

	fputs("\n", pDest);

	fclose(pMain);
	fclose(pDest);

	return true;
}




bool CShader::GetShaderCode(const char* cFolderPath, const char* cPath, char*** pcShaderCode, int** pnCodeLength, int* pnLineNumber)
{
#ifdef COPY_SHADERS
	if (!HandleIncludes(cFolderPath, cPath))
		return false;

	FILE* pFile = fopen("shader_code.glsl", "r");

#else

	FILE* pFile = fopen(cPath, "r");

#endif

	char cLine[1024] = "";
	*pnLineNumber = 0;
	int nLineSize;
	*pcShaderCode = NULL;
	
	while (fgets(cLine, 1024, pFile) != NULL)
		(*pnLineNumber)++;

	*pcShaderCode = new char*[*pnLineNumber+1];
	*pnCodeLength = new int[*pnLineNumber+1];

	rewind(pFile);
	*pnLineNumber = 0;

	while (fgets(cLine, 1024, pFile) != NULL)
	{
		nLineSize = (int)strlen(cLine);
		(*pnCodeLength)[*pnLineNumber] = nLineSize;
		(*pcShaderCode)[*pnLineNumber] = new char[nLineSize + 1];
		strncpy((*pcShaderCode)[*pnLineNumber], cLine, nLineSize+1);

		(*pnLineNumber)++;
	}

	fclose(pFile);

	return true;
}


void CShader::HandleErrorMessages(const char* cPath, char** pOutputMessage)
{
	if (pOutputMessage != NULL)
	{
		if (*pOutputMessage != NULL)
		{
			if (cPath != NULL)
				fprintf(stderr, "Error compiling shader %s : %s\n\n", cPath, *pOutputMessage);

			else
				fprintf(stderr, "Error linking program : %s\n\n", *pOutputMessage);

			delete[] * pOutputMessage;
			*pOutputMessage = NULL;
		}
	}
}


CShader::ShaderHandle CShader::CompileShader(EShaderType type, char** pcShaderCode, int* nLineLength, int nLineNumber, char** pOutputMessage)
{
	CShader::ShaderHandle ShaderID;

	switch (type)
	{
	case e_VertexShader:
		ShaderID = glCreateShader(GL_VERTEX_SHADER);
		break;

	case e_HullShader:
		ShaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
		break;

	case e_DomainShader:
		ShaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
		break;

	case e_GeometryShader:
		ShaderID = glCreateShader(GL_GEOMETRY_SHADER);
		break;

	case e_FragmentShader:
		ShaderID = glCreateShader(GL_FRAGMENT_SHADER);
		break;

	case e_ComputeShader:
		ShaderID = glCreateShader(GL_COMPUTE_SHADER);
		break;

	default:
		fprintf(stderr, "Invalid shader type\n");
		return -1;
	}

	glShaderSource(ShaderID, nLineNumber, pcShaderCode, nLineLength);
	glCompileShader(ShaderID);

	delete[] nLineLength;

	for (int i = 0; i < nLineNumber; i++)
		delete[] pcShaderCode[i];

	delete[] pcShaderCode;

	if (pOutputMessage != NULL)
	{
		GLint CompileStatus = GL_FALSE;
		int MessageLength = 0;
		glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &CompileStatus);
		glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (CompileStatus != GL_TRUE)
		{
			*pOutputMessage = new char[MessageLength];
			glGetShaderInfoLog(ShaderID, MessageLength, NULL, *pOutputMessage);
			return -1;
		}
	}

	return ShaderID;
}


ProgramHandle CShader::LinkProgram(CShader::ShaderHandle ComputeShaderID, char** pOutputMessage)
{
	ProgramHandle ProgramID = glCreateProgram();
	glAttachShader(ProgramID, ComputeShaderID);
	glLinkProgram(ProgramID);

	if (pOutputMessage != NULL)
	{
		int CompileStatus = 0;
		int MessageLength = 0;
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &CompileStatus);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (MessageLength > 0)
		{
			*pOutputMessage = new char[MessageLength];
			glGetProgramInfoLog(ProgramID, MessageLength, NULL, *pOutputMessage);
		}
	}

	return ProgramID;
}


ProgramHandle CShader::LinkProgram(CShader::ShaderHandle VertexShaderID, CShader::ShaderHandle FragmentShaderID, char** pOutputMessage)
{
	ProgramHandle ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	if (pOutputMessage != NULL)
	{
		int CompileStatus = 0;
		int MessageLength = 0;
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &CompileStatus);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (MessageLength > 0)
		{
			*pOutputMessage = new char[MessageLength];
			glGetProgramInfoLog(ProgramID, MessageLength, NULL, *pOutputMessage);
		}
	}

	return ProgramID;
}


ProgramHandle CShader::LinkProgram(ShaderHandle VertexShaderID, ShaderHandle GeometryShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage)
{
	ProgramHandle ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, GeometryShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	if (pOutputMessage != NULL)
	{
		int CompileStatus = 0;
		int MessageLength = 0;
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &CompileStatus);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (MessageLength > 0)
		{
			*pOutputMessage = new char[MessageLength];
			glGetProgramInfoLog(ProgramID, MessageLength, NULL, *pOutputMessage);
		}
	}

	return ProgramID;
}


ProgramHandle CShader::LinkProgram(ShaderHandle VertexShaderID, ShaderHandle HullShaderID, ShaderHandle DomainShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage)
{
	ProgramHandle ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, HullShaderID);
	glAttachShader(ProgramID, DomainShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	if (pOutputMessage != NULL)
	{
		int CompileStatus = 0;
		int MessageLength = 0;
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &CompileStatus);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (MessageLength > 0)
		{
			*pOutputMessage = new char[MessageLength];
			glGetProgramInfoLog(ProgramID, MessageLength, NULL, *pOutputMessage);
		}
	}

	return ProgramID;
}


ProgramHandle CShader::LinkProgram(ShaderHandle VertexShaderID, ShaderHandle HullShaderID, ShaderHandle DomainShaderID, ShaderHandle GeometryShaderID, ShaderHandle FragmentShaderID, char** pOutputMessage)
{
	ProgramHandle ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, HullShaderID);
	glAttachShader(ProgramID, DomainShaderID);
	glAttachShader(ProgramID, GeometryShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	if (pOutputMessage != NULL)
	{
		int CompileStatus = 0;
		int MessageLength = 0;
		glGetProgramiv(ProgramID, GL_LINK_STATUS, &CompileStatus);
		glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &MessageLength);
		*pOutputMessage = NULL;

		if (MessageLength > 0)
		{
			*pOutputMessage = new char[MessageLength];
			glGetProgramInfoLog(ProgramID, MessageLength, NULL, *pOutputMessage);
		}
	}

	return ProgramID;
}
