#include "Engine/Engine.h"
#include "Engine/Renderer/Renderer.h"


ProgramHandle gs_nCopyPID = INVALID_PROGRAM_HANDLE;


void CRenderer::CopyInit()
{
	//gs_nCopyPID = CShader::LoadProgram(SHADER_PATH("Copy"), "copy", "copy");
}


void CRenderer::CopyTerminate()
{
	CShader::DeleteProgram(gs_nCopyPID);
}
