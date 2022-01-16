#include "Engine/Renderer/Renderer.h"
#include "Engine/Device/DeviceManager.h"
#include <spirv_cross/spirv_cross.hpp>
#include <string.h>
#include <fstream>

#define STRING(x)			#x
#define STR_VALUE(x)		STRING(x)
#define SHADER_PATH_STRING	STR_VALUE(SHADER_PATH)

#ifdef _WIN32
#include "Windows.h"
#endif

#ifdef strupr
#undef strupr
#endif

ProgramHandle				CShader::ms_nCurrentPID = 0;
std::vector<unsigned int>	CShader::ms_nConstantsBindingPoint;

std::vector<CShader::SProgramDesc>	CShader::ms_ProgramDesc;
std::vector<CShader::SShader>		CShader::ms_Shaders;


extern SVertexElements g_VertexStreamSemantics[];
extern SVertexElements g_VertexStreamStandardSemantics[];


char CShader::ms_pcShaderDirectory[1024] = SHADER_PATH_STRING;


void CShader::BindProgram(ProgramHandle PID)
{

}


void CShader::DeleteProgram(ProgramHandle nPID)
{
}


void CShader::Terminate()
{
	ms_ProgramDesc.clear();

	unsigned int numShaders = static_cast<unsigned int>(ms_Shaders.size());

	for (unsigned int i = 0; i < numShaders; i++)
		vkDestroyShaderModule(CDeviceManager::GetDevice(), (VkShaderModule)ms_Shaders[i].m_pShader, nullptr);
	
	ms_Shaders.clear();
}


static std::vector<uint32_t> readFile(const char* cfilename)
{
	std::string filename(cfilename);
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	ASSERT_MSG(file.is_open(), "Could not open file %s", filename.c_str());

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	file.close();

	return buffer;
}


void CShader::GetShaderPath(char* outPath, const char* shaderName, const char* shaderExt)
{
	sprintf(outPath, "%s/%s.%s", CShader::ms_pcShaderDirectory, shaderName, shaderExt);
}


inline void upr(_Out_writes_z_(p_nMaxSize) char *p_pcDest, _In_ size_t p_nMaxSize, _In_z_ const char* p_pcStr)
{
	size_t i = 0;
	for (; p_pcStr[i] != '\0' && i < p_nMaxSize - 1; ++i)
		p_pcDest[i] = (char)::toupper(static_cast<unsigned char>(p_pcStr[i]));
	p_pcDest[i] = '\0';
}


unsigned int GetVertexDeclarationMask(spirv_cross::Compiler& comp)
{
	spirv_cross::ShaderResources res = comp.get_shader_resources();

	size_t numInputResources = res.stage_inputs.size();

	unsigned int VertexDeclarationMask = 0;

	for (size_t i = 0; i < numInputResources; i++)
	{
		std::string name = res.stage_inputs[i].name;

		char SemanticName[256] = "";
		int index = 0;

		strcpy(SemanticName, name.c_str());
		upr(SemanticName, 256, SemanticName);

		size_t l = strlen(SemanticName);

		if (SemanticName[l - 1] >= '0' && SemanticName[l - 1] <= '9')
		{
			index = atoi(SemanticName + l - 1);
			SemanticName[l - 1] = '\0';
		}

		uint8_t maxVertexElement = (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine) ? e_MaxVertexElementUsage : e_MaxStandardVertexElementUsage;

		for (uint8_t i = 0; i < maxVertexElement; i++)
		{
			if (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine)
			{
				if (g_VertexStreamSemantics[i].m_SubID == index && !strcmp(g_VertexStreamSemantics[i].m_ShaderSemantic, SemanticName))
				{
					VertexDeclarationMask |= g_VertexStreamSemantics[i].m_StreamMask;
					break;
				}
			}

			else
			{
				if (g_VertexStreamStandardSemantics[i].m_SubID == index && !strcmp(g_VertexStreamStandardSemantics[i].m_ShaderSemantic, SemanticName))
				{
					VertexDeclarationMask |= g_VertexStreamStandardSemantics[i].m_StreamMask;
					break;
				}
			}
		}
	}

	return VertexDeclarationMask;
}


void GetConstantBuffersAndPushConstants(spirv_cross::Compiler& comp, std::vector<CShader::SShaderConstants>& constants, size_t& pushConstantSize)
{
	spirv_cross::ShaderResources res = comp.get_shader_resources();

	size_t numInputResources = res.uniform_buffers.size();

	for (size_t i = 0; i < numInputResources; i++)
	{
		std::string name = res.uniform_buffers[i].name;

		char SemanticName[256] = "";
		int index = 0;

		strcpy(SemanticName, name.c_str());
		upr(SemanticName, 256, SemanticName);

		size_t l = strlen(SemanticName);

		int j = 0;

		while (SemanticName[l - j - 1] >= '0' && SemanticName[l - j - 1] <= '9')
			j++;

		if (j > 0)
		{
			sscanf(SemanticName + l - j, "%d", &index);
			SemanticName[l - j] = '\0';
		}

		if (!strcmp(SemanticName, "CB"))
		{
			CShader::SShaderConstants constant;
			constant.m_nSlot = index;
			constant.m_nSize = static_cast<int>(comp.get_declared_struct_size(comp.get_type(res.uniform_buffers[i].base_type_id)));

			constants.push_back(constant);
		}
	}

	if (res.push_constant_buffers.size() > 0)
	{
		pushConstantSize = comp.get_declared_struct_size(comp.get_type(res.push_constant_buffers[0].base_type_id));
	}

	else
		pushConstantSize = 0;
}



VkShaderModule createShaderModule(const std::vector<uint32_t>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size() * sizeof(uint32_t);
	createInfo.pCode = code.data();

	VkShaderModule shaderModule;
	VkResult res = vkCreateShaderModule(CDeviceManager::GetDevice(), &createInfo, nullptr, &shaderModule);
	ASSERT(res == VK_SUCCESS);

	return shaderModule;
}


unsigned int CShader::CreateShader(const char* shaderName, bool vertexDeclaration)
{
	unsigned int numShaders = static_cast<unsigned int>(ms_Shaders.size());

	for (unsigned int i = 0; i < numShaders; i++)
	{
		if (!strcmp(ms_Shaders[i].m_cName, shaderName))
		{
			return i;
		}
	}

	char shaderPath[1024] = "";
	CShader::GetShaderPath(shaderPath, shaderName, "spv");

	std::vector<uint32_t> shaderCode = readFile(shaderPath);

	SShader shader;

	spirv_cross::Compiler comp(shaderCode);

	if (vertexDeclaration)
		shader.m_nVertexDeclarationMask = GetVertexDeclarationMask(comp);

	GetConstantBuffersAndPushConstants(comp, shader.m_nConstantBuffers, shader.m_nPushConstantSize);

	shader.m_pShader = createShaderModule(shaderCode);
	strcpy(shader.m_cName, shaderName);

	ms_Shaders.push_back(shader);

	return static_cast<unsigned int>(ms_Shaders.size()) - 1;
}


ProgramHandle CShader::LoadProgram(const char* cComputeShaderName)
{
	char path[1024] = "";
	char name[1024] = "";

	strcpy(name, cComputeShaderName);
	strcat(name, "_c");

	SProgramDesc program;
	program.m_nComputeShaderID = CreateShader(name);

	ProgramHandle handle = static_cast<ProgramHandle>(ms_ProgramDesc.size());
	ms_ProgramDesc.push_back(program);

	return handle;
}


ProgramHandle CShader::LoadProgram(const char* cVertexShaderName, const char* cFragmentShaderName)
{
	char vertexName[1024] = "";
	char pixelName[1024] = "";

	strcpy(vertexName, cVertexShaderName);
	strcat(vertexName, "_v");

	strcpy(pixelName, cFragmentShaderName);
	strcat(pixelName, "_p");

	SProgramDesc program;
	program.m_nVertexShaderID = CreateShader(vertexName, true);
	program.m_nPixelShaderID = CreateShader(pixelName);

	ProgramHandle handle = static_cast<ProgramHandle>(ms_ProgramDesc.size());
	ms_ProgramDesc.push_back(program);

	return handle;
}


ProgramHandle CShader::LoadProgram(const char* cVertexShaderName, const char* cGeometryShaderName, const char* cFragmentShaderName)
{
	char vertexName[1024]	= "";
	char geomName[1024]		= "";
	char pixelName[1024]	= "";

	strcpy(vertexName, cVertexShaderName);
	strcat(vertexName, "_v");

	strcpy(geomName, cGeometryShaderName);
	strcat(geomName, "_g");

	strcpy(pixelName, cFragmentShaderName);
	strcat(pixelName, "_p");

	SProgramDesc program;
	program.m_nVertexShaderID	= CreateShader(vertexName, true);
	program.m_nGeometryShaderID = CreateShader(geomName);
	program.m_nPixelShaderID	= CreateShader(pixelName);

	ProgramHandle handle = static_cast<ProgramHandle>(ms_ProgramDesc.size());
	ms_ProgramDesc.push_back(program);

	return handle;
}

ProgramHandle CShader::LoadProgram(const char* cVertexShaderName, const char* cHullShaderName, const char* cDomainShaderName, const char* cFragmentShaderName)
{
	char vertexName[1024] = "";
	char hullName[1024] = "";
	char domainName[1024] = "";
	char pixelName[1024] = "";

	strcpy(vertexName, cVertexShaderName);
	strcat(vertexName, "_v");

	strcpy(hullName, cHullShaderName);
	strcat(hullName, "_h");

	strcpy(domainName, cDomainShaderName);
	strcat(domainName, "_d");

	strcpy(pixelName, cFragmentShaderName);
	strcat(pixelName, "_p");

	SProgramDesc program;
	program.m_nVertexShaderID	= CreateShader(vertexName, true);
	program.m_nHullShaderID		= CreateShader(hullName);
	program.m_nDomainShaderID	= CreateShader(domainName);
	program.m_nPixelShaderID	= CreateShader(pixelName);

	ProgramHandle handle = static_cast<ProgramHandle>(ms_ProgramDesc.size());
	ms_ProgramDesc.push_back(program);

	return handle;
}

ProgramHandle CShader::LoadProgram(const char* cVertexShaderName, const char* cHullShaderName, const char* cDomainShaderName, const char* cGeometryShaderName, const char* cFragmentShaderName)
{
	char vertexName[1024] = "";
	char hullName[1024] = "";
	char geomName[1024] = "";
	char domainName[1024] = "";
	char pixelName[1024] = "";

	strcpy(vertexName, cVertexShaderName);
	strcat(vertexName, "_v");

	strcpy(hullName, cHullShaderName);
	strcat(hullName, "_h");

	strcpy(domainName, cDomainShaderName);
	strcat(domainName, "_d");

	strcpy(geomName, cGeometryShaderName);
	strcat(geomName, "_g");

	strcpy(pixelName, cFragmentShaderName);
	strcat(pixelName, "_p");

	SProgramDesc program;
	program.m_nVertexShaderID	= CreateShader(vertexName, true);
	program.m_nHullShaderID		= CreateShader(hullName);
	program.m_nGeometryShaderID = CreateShader(geomName);
	program.m_nDomainShaderID	= CreateShader(domainName);
	program.m_nPixelShaderID	= CreateShader(pixelName);

	ProgramHandle handle = static_cast<ProgramHandle>(ms_ProgramDesc.size());
	ms_ProgramDesc.push_back(program);

	return handle;
}



