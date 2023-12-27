#include "Engine/Engine.h"
#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "../RenderThreads.h"
#include "../CommandListManager.h"
#include "../ResourceManager.h"
#include "../PipelineManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/RTX/RTX_Utils.h"

#include <algorithm>


std::vector<CPipelineManager::SPipeline*>	CPipelineManager::ms_pPipelines;
thread_local CPipelineManager::SPipeline*	CPipelineManager::ms_pCurrentPipeline;

thread_local ProgramHandle					CPipelineManager::ms_nActiveProgram = INVALIDHANDLE;

CMutex*										CPipelineManager::ms_pLock = nullptr;


extern SVertexElements	g_VertexStreamSemantics[];
extern unsigned int		g_VertexStreamOffsetNoSkin[];
extern unsigned int		g_VertexStreamOffsetSkin[];

extern SVertexElements	g_VertexStreamStandardSemantics[];
extern unsigned int		g_VertexStreamStandardOffset[];


const VkBlendFactor gs_BlendFuncAssociation[] =
{
	VK_BLEND_FACTOR_ZERO,
	VK_BLEND_FACTOR_ONE,
	VK_BLEND_FACTOR_SRC_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	VK_BLEND_FACTOR_SRC_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	VK_BLEND_FACTOR_DST_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
	VK_BLEND_FACTOR_DST_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
	VK_BLEND_FACTOR_CONSTANT_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
	VK_BLEND_FACTOR_SRC1_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
	VK_BLEND_FACTOR_SRC1_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
};


const VkBlendOp gs_BlendOpAssociation[] =
{
	VK_BLEND_OP_ADD,
	VK_BLEND_OP_SUBTRACT,
	VK_BLEND_OP_REVERSE_SUBTRACT,
	VK_BLEND_OP_MIN,
	VK_BLEND_OP_MAX
};


const VkLogicOp gs_LogicOpAssociation[] =
{
	VK_LOGIC_OP_CLEAR,
	VK_LOGIC_OP_SET,
	VK_LOGIC_OP_COPY,
	VK_LOGIC_OP_COPY_INVERTED,
	VK_LOGIC_OP_NO_OP,
	VK_LOGIC_OP_INVERT,
	VK_LOGIC_OP_AND,
	VK_LOGIC_OP_NAND,
	VK_LOGIC_OP_OR,
	VK_LOGIC_OP_NOR,
	VK_LOGIC_OP_XOR,
	VK_LOGIC_OP_EQUIVALENT,
	VK_LOGIC_OP_AND_REVERSE,
	VK_LOGIC_OP_AND_INVERTED,
	VK_LOGIC_OP_OR_REVERSE,
	VK_LOGIC_OP_OR_INVERTED
};


const VkPolygonMode gs_RasterFillModeAssociation[] =
{
	VK_POLYGON_MODE_LINE,
	VK_POLYGON_MODE_FILL
};


const VkCullModeFlags gs_RasterCullModeAssociation[] =
{
	VK_CULL_MODE_NONE,
	VK_CULL_MODE_BACK_BIT,
	VK_CULL_MODE_FRONT_BIT
};


const VkCompareOp gs_CmpFuncAssociation[] =
{
	VK_COMPARE_OP_NEVER,
	VK_COMPARE_OP_LESS,
	VK_COMPARE_OP_EQUAL,
	VK_COMPARE_OP_LESS_OR_EQUAL,
	VK_COMPARE_OP_GREATER,
	VK_COMPARE_OP_NOT_EQUAL,
	VK_COMPARE_OP_GREATER_OR_EQUAL,
	VK_COMPARE_OP_ALWAYS
};


const VkStencilOp gs_StencilOpAssociation[] =
{
	VK_STENCIL_OP_KEEP,
	VK_STENCIL_OP_ZERO,
	VK_STENCIL_OP_REPLACE,
	VK_STENCIL_OP_INCREMENT_AND_CLAMP,
	VK_STENCIL_OP_DECREMENT_AND_CLAMP,
	VK_STENCIL_OP_INVERT,
	VK_STENCIL_OP_INCREMENT_AND_WRAP,
	VK_STENCIL_OP_DECREMENT_AND_WRAP
};


const VkPrimitiveTopology gs_TopologyAssociation[] =
{
	VK_PRIMITIVE_TOPOLOGY_MAX_ENUM,
	VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
	VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
	VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
	VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	VK_PRIMITIVE_TOPOLOGY_PATCH_LIST
};



void CPipelineManager::Init()
{
	ms_pPipelines.clear();

	ms_pCurrentPipeline = nullptr;

	ms_pLock = CMutex::Create();
}


void CPipelineManager::Terminate()
{
	std::vector<SPipeline*>::iterator it;

	for (it = ms_pPipelines.begin(); it < ms_pPipelines.end(); it++)
		delete *it;

	ms_pPipelines.clear();
	delete ms_pLock;

	ms_pLock = nullptr;
}


void CPipelineManager::BindShaders(unsigned int nPipelineStateID)
{
	SPipeline* pipeline = ms_pPipelines[nPipelineStateID - 1];

	if (pipeline->m_eType != e_RayTracingPipeline)
		SetActiveProgram(pipeline->m_nProgramID);
}


void CPipelineManager::BindPipeline(unsigned int nCommandListID, unsigned int nPipelineStateID)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	if (nPipelineStateID == INVALIDHANDLE)
		return;

	ASSERT(nPipelineStateID > 0 && nPipelineStateID < ms_pPipelines.size() + 1);

	SPipeline* pipeline = ms_pPipelines[nPipelineStateID - 1];

	if (pipeline->m_pPipelineState == nullptr)
		return;

	BindShaders(nPipelineStateID);

	if (ms_pCurrentPipeline == nullptr || ms_pCurrentPipeline->m_pPipelineState != pipeline->m_pPipelineState)
	{
		ms_pCurrentPipeline = pipeline;

		if (pipeline->m_eType == e_GraphicsPipeline)
			vkCmdBindPipeline((VkCommandBuffer)CCommandListManager::GetCommandListPtr(nCommandListID), VK_PIPELINE_BIND_POINT_GRAPHICS, (VkPipeline)pipeline->m_pPipelineState);

		else if (pipeline->m_eType == e_ComputePipeline)
			vkCmdBindPipeline((VkCommandBuffer)CCommandListManager::GetCommandListPtr(nCommandListID), VK_PIPELINE_BIND_POINT_COMPUTE, (VkPipeline)pipeline->m_pPipelineState);

		else if (pipeline->m_eType == e_RayTracingPipeline)
			vkCmdBindPipeline((VkCommandBuffer)CCommandListManager::GetCommandListPtr(nCommandListID), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, (VkPipeline)pipeline->m_pPipelineState);
	}
}


CPipelineManager::SPipeline* CPipelineManager::GetCurrentPipeline()
{
	return ms_pCurrentPipeline;
}


CPipelineManager::EPipelineType CPipelineManager::GetCurrentPipelineType()
{
	return ms_pCurrentPipeline->m_eType;
}


void CPipelineManager::SetCurrentPipeline(unsigned int nID)
{
	ms_pCurrentPipeline = GetPipelineState(nID);
}


unsigned int CPipelineManager::NewPipeline(EPipelineType eType)
{
	ms_pLock->Take();

	SPipeline* pipeline = new SPipeline(eType);

	unsigned int nID = static_cast<unsigned int>(ms_pPipelines.size());

	ms_pPipelines.push_back(pipeline);

	ms_pLock->Release();

	return nID + 1;
}



CPipelineManager::SPipeline::SPipeline(EPipelineType eType)
{
	m_eType = eType;

	m_nMaxNumVersions = 1;
	m_nCurrentVersion = 0;
	m_nShaderVisibility = CShader::e_FragmentShader;

	m_NumConstantBuffers = 0;
	m_NumTextures.clear();
	m_NumRwTextures.clear();
	m_NumBuffers.clear();
	m_NumRwBuffers.clear();
	m_NumSamplers.clear();

	m_bAliasedPipeline = false;
	m_bVersionNumUpToDate = true;

	for (int i = 0; i < 8; i++)
		m_bRenderTargetBound[i] = false;

	m_pPipelineState = nullptr;
	m_pRootSignature = nullptr;
	m_pDescriptorLayout = nullptr;

	m_nProgramID = INVALIDHANDLE;
}


CPipelineManager::SPipeline::~SPipeline()
{
	if (m_pPipelineState != nullptr)
	{
		if (!m_bAliasedPipeline)
		{
			vkDestroyPipeline(CDeviceManager::GetDevice(), (VkPipeline)m_pPipelineState, nullptr);
			vkDestroyPipelineLayout(CDeviceManager::GetDevice(), (VkPipelineLayout)m_pRootSignature, nullptr);
		}

		vkDestroyDescriptorSetLayout(CDeviceManager::GetDevice(), (VkDescriptorSetLayout)m_pDescriptorLayout, nullptr);

		unsigned int numSets = static_cast<unsigned int>(m_pDescriptorSets.size());

		for (unsigned int i = 0; i < numSets; i++)
		{
			for (unsigned int j = 0; j < m_nMaxNumVersions; j++)
				vkFreeDescriptorSets(CDeviceManager::GetDevice(), (VkDescriptorPool)CResourceManager::GetLocalSRVDecriptorHeap(i), 1, (VkDescriptorSet*)&m_pDescriptorSets[i][j]);

			m_pDescriptorSets[i].clear();
		}

		m_pDescriptorSets.clear();

		m_pPipelineState = nullptr;
		m_pRootSignature = nullptr;
		m_pDescriptorLayout = nullptr;
	}
}


void CPipelineManager::SPipeline::CopyFrom(SPipeline* pSrc)
{
	ASSERT(pSrc != nullptr);

	*this = *pSrc;

	m_pPipelineState = nullptr;
	m_pRootSignature = nullptr;
	m_pDescriptorLayout = nullptr;
}


bool CPipelineManager::SPipeline::BindProgram(const char* computeName)
{
	m_nProgramID = CShader::LoadProgram(computeName);

	return m_nProgramID != INVALIDHANDLE;
}

bool CPipelineManager::SPipeline::BindProgram(const char* vertexName, const char* pixelName)
{
	m_nProgramID = CShader::LoadProgram(vertexName, pixelName);

	return m_nProgramID != INVALIDHANDLE;
}

bool CPipelineManager::SPipeline::BindProgram(const char* vertexName, const char* geomName, const char* pixelName)
{
	m_nProgramID = CShader::LoadProgram(vertexName, geomName, pixelName);

	return m_nProgramID != INVALIDHANDLE;
}

bool CPipelineManager::SPipeline::BindProgram(const char* vertexName, const char* hullName, const char* domainName, const char* pixelName)
{
	m_nProgramID = CShader::LoadProgram(vertexName, hullName, domainName, pixelName);

	return m_nProgramID != INVALIDHANDLE;
}

bool CPipelineManager::SPipeline::BindProgram(const char* vertexName, const char* hullName, const char* domainName, const char* geomName, const char* pixelName)
{
	m_nProgramID = CShader::LoadProgram(vertexName, hullName, domainName, geomName, pixelName);

	return m_nProgramID != INVALIDHANDLE;
}



unsigned int CPipelineManager::SPipeline::AddShaderToLibrary(CShader::SShader* pShader)
{
	uint32_t index		= INVALIDHANDLE;
	uint32_t libSize	= static_cast<uint32_t>(m_ShaderLibrary.size());

	for (uint32_t i = 0; i < libSize; i++)
	{
		if (m_ShaderLibrary[i] == pShader)
		{
			index = i;
			break;
		}
	}

	if (index == INVALIDHANDLE)
	{
		index = libSize;
		m_ShaderLibrary.push_back(pShader);
	}

	return index;
}



bool CPipelineManager::SPipeline::CreateHitGroup(const char* rayGenShader, const char* intersectionShader, const char* anyHitShader, const char* closestHitShader, const char* missShader)
{
	SHitGroup group;
	group.anyHitShader			= INVALIDHANDLE;
	group.closestHitShader		= INVALIDHANDLE;
	group.generalShader			= INVALIDHANDLE;
	group.intersectionShader	= INVALIDHANDLE;

	char name[1024] = "";

	if (anyHitShader != nullptr)
	{
		strcpy(name, anyHitShader);
		strcat(name, "_rahit");

		uint32_t shaderId = CShader::CreateShader(name);
		CShader::SShader* pShader = CShader::GetShader(shaderId);
		pShader->m_eType = CShader::e_AnyHitShader;

		group.anyHitShader = AddShaderToLibrary(pShader);
	}

	if (closestHitShader != nullptr)
	{
		strcpy(name, closestHitShader);
		strcat(name, "_rchit");

		uint32_t shaderId = CShader::CreateShader(name);
		CShader::SShader* pShader = CShader::GetShader(shaderId);
		pShader->m_eType = CShader::e_ClosestHitShader;

		group.closestHitShader = AddShaderToLibrary(pShader);
	}

	if (rayGenShader != nullptr)
	{
		strcpy(name, rayGenShader);
		strcat(name, "_rgen");

		uint32_t shaderId = CShader::CreateShader(name);
		CShader::SShader* pShader = CShader::GetShader(shaderId);
		pShader->m_eType = CShader::e_RayGenShader;

		group.generalShader = AddShaderToLibrary(pShader);
	}

	if (missShader != nullptr)
	{
		strcpy(name, missShader);
		strcat(name, "_rmiss");

		uint32_t shaderId = CShader::CreateShader(name);
		CShader::SShader* pShader = CShader::GetShader(shaderId);
		pShader->m_eType = CShader::e_MissShader;

		group.generalShader = AddShaderToLibrary(pShader);
	}

	if (intersectionShader != nullptr)
	{
		strcpy(name, intersectionShader);
		strcat(name, "_rint");

		uint32_t shaderId = CShader::CreateShader(name);
		CShader::SShader* pShader = CShader::GetShader(shaderId);
		pShader->m_eType = CShader::e_IntersectionShader;

		group.intersectionShader = AddShaderToLibrary(pShader);
	}

	m_HitGroups.push_back(group);

	return true;
}


void CPipelineManager::SPipeline::DisableBlend(unsigned char writeMask)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_bIndependentBlendEnabled = false;

	m_State.m_BlendStates[0].m_bBlendEnabled = false;
	m_State.m_BlendStates[0].m_WriteMask = static_cast<uint8_t>(writeMask);
}



bool CPipelineManager::SPipeline::SetBlendState(bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask, ELogicOp logicOp)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	if (!blendEnable)
	{
		DisableBlend(writeMask);
	}

	else
	{
		m_State.m_bIndependentBlendEnabled			= false;
		m_State.m_BlendStates[0].m_bBlendEnabled	= true;
		m_State.m_BlendStates[0].m_bLogicOpEnabled	= logicOpEnable;
		m_State.m_BlendStates[0].m_SrcFunc			= srcBlend;
		m_State.m_BlendStates[0].m_DstFunc			= dstBlend;
		m_State.m_BlendStates[0].m_BlendOp			= colorOp;
		m_State.m_BlendStates[0].m_SrcAlphaFunc		= srcBlendAlpha;
		m_State.m_BlendStates[0].m_DstAlphaFunc		= dstBlendAlpha;
		m_State.m_BlendStates[0].m_AlphaBlendOp		= alphaOp;
		m_State.m_BlendStates[0].m_WriteMask		= static_cast<uint8_t>(writeMask);
		m_State.m_BlendStates[0].m_LogicOp			= logicOp;
	}

	return true;
}



bool CPipelineManager::SPipeline::SetRenderTargetBlendState(int renderTargetSlot, bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask, ELogicOp logicOp)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_bIndependentBlendEnabled							= true;
	m_State.m_BlendStates[renderTargetSlot].m_bBlendEnabled		= blendEnable;

	if (blendEnable)
	{
		m_State.m_BlendStates[renderTargetSlot].m_bLogicOpEnabled	= logicOpEnable;
		m_State.m_BlendStates[renderTargetSlot].m_SrcFunc			= srcBlend;
		m_State.m_BlendStates[renderTargetSlot].m_DstFunc			= dstBlend;
		m_State.m_BlendStates[renderTargetSlot].m_BlendOp			= colorOp;
		m_State.m_BlendStates[renderTargetSlot].m_SrcAlphaFunc		= srcBlendAlpha;
		m_State.m_BlendStates[renderTargetSlot].m_DstAlphaFunc		= dstBlendAlpha;
		m_State.m_BlendStates[renderTargetSlot].m_AlphaBlendOp		= alphaOp;
		m_State.m_BlendStates[renderTargetSlot].m_WriteMask			= static_cast<uint8_t>(writeMask);
		m_State.m_BlendStates[renderTargetSlot].m_LogicOp			= logicOp;
	}

	return true;
}


bool CPipelineManager::SPipeline::SetRasterizerState(ERasterFillMode fillMode, ERasterCullMode cullMode, bool conservativeRaster, bool enableMultisampling, bool depthClamp, float depthBias, float slopeBias)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_RasterState.m_bConservativeRaster = conservativeRaster;
	m_State.m_RasterState.m_bMultiSampling		= enableMultisampling;
	m_State.m_RasterState.m_bDepthClamp			= depthClamp;

	m_State.m_RasterState.m_DepthBias	= depthBias;
	m_State.m_RasterState.m_SlopeBias	= slopeBias;

	m_State.m_RasterState.m_CullMode	= cullMode;
	m_State.m_RasterState.m_FillMode	= fillMode;

	return true;
}



void CPipelineManager::SPipeline::DisableDepthStencil()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_DepthStencilState.m_bStencilEnable = false;
	m_State.m_DepthStencilState.m_bDepthEnable = false;
	m_State.m_DepthStencilState.m_bEnableWrite = false;
}


bool CPipelineManager::SPipeline::SetDepthState(bool depthEnable, ECmpFunc depthFunc, bool enableWrite)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_DepthStencilState.m_bDepthEnable = depthEnable;
	m_State.m_DepthStencilState.m_bEnableWrite = enableWrite;
	m_State.m_DepthStencilState.m_DepthFunc = depthFunc;

	return true;
}



void CPipelineManager::SPipeline::DisableStencil()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_DepthStencilState.m_bStencilEnable = false;
	m_State.m_DepthStencilState.m_ReadMask = 0;
	m_State.m_DepthStencilState.m_WriteMask = 0;
}


bool CPipelineManager::SPipeline::SetStencilState(bool stencilEnable, int readMask, int writeMask,	EStencilOp frontFail, EStencilOp frontDepthFail, EStencilOp frontPass, ECmpFunc frontFunc, \
																									EStencilOp backFail, EStencilOp backDepthFail, EStencilOp backPass, ECmpFunc backFunc)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	if (!stencilEnable)
	{
		DisableStencil();
	}

	else
	{
		m_State.m_DepthStencilState.m_WriteMask = static_cast<uint8_t>(writeMask);
		m_State.m_DepthStencilState.m_ReadMask = static_cast<uint8_t>(readMask);

		m_State.m_DepthStencilState.m_FrontStencil.m_FailOp = frontFail;
		m_State.m_DepthStencilState.m_FrontStencil.m_DepthFailOp = frontDepthFail;
		m_State.m_DepthStencilState.m_FrontStencil.m_PassOp = frontPass;
		m_State.m_DepthStencilState.m_FrontStencil.m_Func = frontFunc;

		m_State.m_DepthStencilState.m_BackStencil.m_FailOp = backFail;
		m_State.m_DepthStencilState.m_BackStencil.m_DepthFailOp = backDepthFail;
		m_State.m_DepthStencilState.m_BackStencil.m_PassOp = backPass;
		m_State.m_DepthStencilState.m_BackStencil.m_Func = backFunc;
	}

	return true;
}


bool CPipelineManager::SPipeline::SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOp frontFail, EStencilOp frontDepthFail, EStencilOp frontPass, ECmpFunc frontFunc)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	if (!stencilEnable)
	{
		DisableStencil();
	}

	else
	{
		m_State.m_DepthStencilState.m_bStencilEnable = true;
		m_State.m_DepthStencilState.m_WriteMask = static_cast<uint8_t>(writeMask);
		m_State.m_DepthStencilState.m_ReadMask = static_cast<uint8_t>(readMask);

		m_State.m_DepthStencilState.m_FrontStencil.m_FailOp = frontFail;
		m_State.m_DepthStencilState.m_FrontStencil.m_DepthFailOp = frontDepthFail;
		m_State.m_DepthStencilState.m_FrontStencil.m_PassOp = frontPass;
		m_State.m_DepthStencilState.m_FrontStencil.m_Func = frontFunc;

		m_State.m_DepthStencilState.m_BackStencil.m_FailOp = frontFail;
		m_State.m_DepthStencilState.m_BackStencil.m_DepthFailOp = frontDepthFail;
		m_State.m_DepthStencilState.m_BackStencil.m_PassOp = frontPass;
		m_State.m_DepthStencilState.m_BackStencil.m_Func = frontFunc;
	}

	return true;
}


bool CPipelineManager::SPipeline::SetPrimitiveTopology(ETopology eTopology)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_Topology = eTopology;

	return true;
}


ETextureFormat ConvertFormat(VkFormat format)
{
	ETextureFormat Format = e_UNKOWN;

	switch (format)
	{
	case VK_FORMAT_B8G8R8A8_UNORM:
		Format = ETextureFormat::e_B8G8R8A8;
		break;
	case VK_FORMAT_R8G8B8A8_UNORM:
		Format = ETextureFormat::e_R8G8B8A8;
		break;
	case VK_FORMAT_R8G8B8A8_SRGB:
		Format = ETextureFormat::e_R8G8B8A8_SRGB;
		break;
	case VK_FORMAT_R8G8B8A8_SNORM:
		Format = ETextureFormat::e_R8G8B8A8_SNORM;
		break;
	case VK_FORMAT_R8G8B8_UNORM:
		Format = ETextureFormat::e_R8G8B8;
		break;
	case VK_FORMAT_R8G8B8_SNORM:
		Format = ETextureFormat::e_R8G8B8_SNORM;
		break;
	case VK_FORMAT_R8G8_UNORM:
		Format = ETextureFormat::e_R8G8;
		break;
	case VK_FORMAT_R8G8_SNORM:
		Format = ETextureFormat::e_R8G8_SNORM;
		break;
	case VK_FORMAT_R8_UNORM:
		Format = ETextureFormat::e_R8;
		break;
	case VK_FORMAT_R16G16B16A16_UNORM:
		Format = ETextureFormat::e_R16G16B16A16_UNORM;
		break;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		Format = ETextureFormat::e_R16G16B16A16_FLOAT;
		break;
	case VK_FORMAT_R16G16B16_SFLOAT:
		Format = ETextureFormat::e_R16G16B16_FLOAT;
		break;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		Format = ETextureFormat::e_R32G32B32A32_FLOAT;
		break;
	case VK_FORMAT_R32G32B32A32_UINT:
		Format = ETextureFormat::e_R32G32B32A32_UINT;
		break;
	case VK_FORMAT_R32G32B32_UINT:
		Format = ETextureFormat::e_R32G32B32_UINT;
		break;
	case VK_FORMAT_R16G16B16A16_UINT:
		Format = ETextureFormat::e_R16G16B16A16_UINT;
		break;
	case VK_FORMAT_R8G8B8A8_UINT:
		Format = ETextureFormat::e_R8G8B8A8_UINT;
		break;
	case VK_FORMAT_R8G8B8A8_SINT:
		Format = ETextureFormat::e_R8G8B8A8_SINT;
		break;
	case VK_FORMAT_R32_SFLOAT:
		Format = ETextureFormat::e_R32_FLOAT;
		break;
	case VK_FORMAT_R16G16_SFLOAT:
		Format = ETextureFormat::e_R16G16_FLOAT;
		break;
	case VK_FORMAT_R16_SFLOAT:
		Format = ETextureFormat::e_R16_FLOAT;
		break;
	case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		Format = ETextureFormat::e_R10G10B10A2;
		break;
	case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
		Format = ETextureFormat::e_R11G11B10_FLOAT;
		break;
	case VK_FORMAT_R8_UINT:
		Format = ETextureFormat::e_R8_UINT;
		break;
	case VK_FORMAT_R16_UINT:
		Format = ETextureFormat::e_R16_UINT;
		break;
	case VK_FORMAT_R32_UINT:
		Format = ETextureFormat::e_R32_UINT;
		break;
	case VK_FORMAT_R32G32_UINT:
		Format = ETextureFormat::e_R32G32_UINT;
		break;
	case VK_FORMAT_R16G16_UINT:
		Format = ETextureFormat::e_R16G16_UINT;
		break;
	case VK_FORMAT_R16G16_UNORM:
		Format = ETextureFormat::e_R16G16_UNORM;
		break;
	case VK_FORMAT_R16G16_SNORM:
		Format = ETextureFormat::e_R16G16_SNORM;
		break;
	case VK_FORMAT_R8G8_UINT:
		Format = ETextureFormat::e_R8G8_UINT;
		break;
	case VK_FORMAT_R8G8B8_SRGB:
		Format = ETextureFormat::e_R8G8B8_SRGB;
		break;
	case VK_FORMAT_D16_UNORM:
		Format = ETextureFormat::e_R16_DEPTH;
		break;
	case VK_FORMAT_D32_SFLOAT:
		Format = ETextureFormat::e_R32_DEPTH;
		break;
	case VK_FORMAT_D24_UNORM_S8_UINT:
		Format = ETextureFormat::e_R24_DEPTH_G8_STENCIL;
		break;
	case VK_FORMAT_D32_SFLOAT_S8_UINT:
		Format = ETextureFormat::e_R32_DEPTH_G8_STENCIL;
		break;
	case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
		Format = ETextureFormat::e_DXT1;
		break;
	case VK_FORMAT_BC2_SRGB_BLOCK:
		Format = ETextureFormat::e_DXT3;
		break;
	case VK_FORMAT_BC3_SRGB_BLOCK:
		Format = ETextureFormat::e_DXT5;
		break;
	case VK_FORMAT_BC7_SRGB_BLOCK:
		Format = ETextureFormat::e_DXT7;
		break;
	case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
		Format = ETextureFormat::e_DXT1_SRGB;
		break;
	case VK_FORMAT_BC2_UNORM_BLOCK:
		Format = ETextureFormat::e_DXT3_SRGB;
		break;
	case VK_FORMAT_BC3_UNORM_BLOCK:
		Format = ETextureFormat::e_DXT5_SRGB;
		break;
	case VK_FORMAT_BC7_UNORM_BLOCK:
		Format = ETextureFormat::e_DXT7_SRGB;
		break;

	default:
		break;
	}

	return Format;
}



VkFormat ConvertFormat(ETextureFormat format)
{
	VkFormat Format = VK_FORMAT_UNDEFINED;

	switch (format)
	{
	case ETextureFormat::e_B8G8R8A8:
		Format = VK_FORMAT_B8G8R8A8_UNORM;
		break;
	case ETextureFormat::e_R8G8B8A8:
		Format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case ETextureFormat::e_R8G8B8A8_SRGB:
		Format = VK_FORMAT_R8G8B8A8_SRGB;
		break;
	case ETextureFormat::e_R8G8B8A8_SNORM:
		Format = VK_FORMAT_R8G8B8A8_SNORM;
		break;
	case ETextureFormat::e_R8G8B8:
		Format = VK_FORMAT_R8G8B8_UNORM;
		break;
	case ETextureFormat::e_R8G8B8_SNORM:
		Format = VK_FORMAT_R8G8B8_SNORM;
		break;
	case ETextureFormat::e_R8G8:
		Format = VK_FORMAT_R8G8_UNORM;
		break;
	case ETextureFormat::e_R8G8_SNORM:
		Format = VK_FORMAT_R8G8_SNORM;
		break;
	case ETextureFormat::e_R8:
		Format = VK_FORMAT_R8_UNORM;
		break;
	case ETextureFormat::e_R16G16B16A16_UNORM:
		Format = VK_FORMAT_R16G16B16A16_UNORM;
		break;
	case ETextureFormat::e_R16G16B16A16_FLOAT:
		Format = VK_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case ETextureFormat::e_R16G16B16_FLOAT:
		Format = VK_FORMAT_R16G16B16_SFLOAT;
		break;
	case ETextureFormat::e_R32G32B32A32_FLOAT:
		Format = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case ETextureFormat::e_R32G32B32A32_UINT:
		Format = VK_FORMAT_R32G32B32A32_UINT;
		break;
	case ETextureFormat::e_R32G32B32_UINT:
		Format = VK_FORMAT_R32G32B32_UINT;
		break;
	case ETextureFormat::e_R16G16B16A16_UINT:
		Format = VK_FORMAT_R16G16B16A16_UINT;
		break;
	case ETextureFormat::e_R8G8B8A8_UINT:
		Format = VK_FORMAT_R8G8B8A8_UINT;
		break;
	case ETextureFormat::e_R8G8B8A8_SINT:
		Format = VK_FORMAT_R8G8B8A8_SINT;
		break;
	case ETextureFormat::e_R32_FLOAT:
		Format = VK_FORMAT_R32_SFLOAT;
		break;
	case ETextureFormat::e_R16G16_FLOAT:
		Format = VK_FORMAT_R16G16_SFLOAT;
		break;
	case ETextureFormat::e_R16_FLOAT:
		Format = VK_FORMAT_R16_SFLOAT;
		break;
	case ETextureFormat::e_R10G10B10A2:
		Format = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		break;
	case ETextureFormat::e_R11G11B10_FLOAT:
		Format = VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		break;
	case ETextureFormat::e_R9G9B9E5_FLOAT:
		Format = VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		break;
	case ETextureFormat::e_R8_UINT:
		Format = VK_FORMAT_R8_UINT;
		break;
	case ETextureFormat::e_R16_UINT:
		Format = VK_FORMAT_R16_UINT;
		break;
	case ETextureFormat::e_R32_UINT:
		Format = VK_FORMAT_R32_UINT;
		break;
	case ETextureFormat::e_R32G32_UINT:
		Format = VK_FORMAT_R32G32_UINT;
		break;
	case ETextureFormat::e_R16G16_UINT:
		Format = VK_FORMAT_R16G16_UINT;
		break;
	case ETextureFormat::e_R16G16_UNORM:
		Format = VK_FORMAT_R16G16_UNORM;
		break;
	case ETextureFormat::e_R16G16:
		Format = VK_FORMAT_R16G16_UNORM;
		break;
	case ETextureFormat::e_R16G16_SNORM:
		Format = VK_FORMAT_R16G16_SNORM;
		break;
	case ETextureFormat::e_R8G8_UINT:
		Format = VK_FORMAT_R8G8_UINT;
		break;
	case ETextureFormat::e_R8G8B8_SRGB:
		Format = VK_FORMAT_R8G8B8_SRGB;
		break;
	case ETextureFormat::e_R16_DEPTH:
		Format = VK_FORMAT_D16_UNORM;
		break;
	case ETextureFormat::e_R24_DEPTH:
		Format = VK_FORMAT_D24_UNORM_S8_UINT;
		break;
	case ETextureFormat::e_R32_DEPTH:
		Format = VK_FORMAT_D32_SFLOAT;
		break;
	case ETextureFormat::e_R24_DEPTH_G8_STENCIL:
		Format = VK_FORMAT_D24_UNORM_S8_UINT;
		break;
	case ETextureFormat::e_R32_DEPTH_G8_STENCIL:
		Format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		break;
	case ETextureFormat::e_DXT1:
		Format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		break;
	case ETextureFormat::e_DXT3:
		Format = VK_FORMAT_BC2_SRGB_BLOCK;
		break;
	case ETextureFormat::e_DXT5:
		Format = VK_FORMAT_BC3_SRGB_BLOCK;
		break;
	case ETextureFormat::e_DXT7:
		Format = VK_FORMAT_BC7_SRGB_BLOCK;
		break;
	case ETextureFormat::e_DXT1_SRGB:
		Format = VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		break;
	case ETextureFormat::e_DXT3_SRGB:
		Format = VK_FORMAT_BC2_UNORM_BLOCK;
		break;
	case ETextureFormat::e_DXT5_SRGB:
		Format = VK_FORMAT_BC3_UNORM_BLOCK;
		break;
	case ETextureFormat::e_DXT7_SRGB:
		Format = VK_FORMAT_BC7_UNORM_BLOCK;
		break;

	default:
		break;
	}

	return Format;
}


VkFormat ConvertVertexFormat(VertexElementType type)
{
	VkFormat format = VK_FORMAT_UNDEFINED;

	switch (type)
	{
	case VertexElementType::e_FLOAT1:
		format = VK_FORMAT_R32_SFLOAT;
		break;

	case VertexElementType::e_FLOAT16_2:
		format = VK_FORMAT_R16G16_SFLOAT;
		break;

	case VertexElementType::e_FLOAT16_4:
		format = VK_FORMAT_R16G16B16A16_SFLOAT;
		break;

	case VertexElementType::e_FLOAT2:
		format = VK_FORMAT_R32G32_SFLOAT;
		break;

	case VertexElementType::e_FLOAT3:
		format = VK_FORMAT_R32G32B32_SFLOAT;
		break;

	case VertexElementType::e_FLOAT4:
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;

	case VertexElementType::e_SBYTE4:
		format = VK_FORMAT_R8_SINT;
		break;

	case VertexElementType::e_SHORT2:
		format = VK_FORMAT_R16G16_SINT;
		break;

	case VertexElementType::e_SHORT2N:
		format = VK_FORMAT_R16G16_SNORM;
		break;

	case VertexElementType::e_SHORT4:
		format = VK_FORMAT_R16G16B16A16_SINT;
		break;

	case VertexElementType::e_SHORT4N:
		format = VK_FORMAT_R16G16B16A16_SNORM;
		break;

	case VertexElementType::e_UBYTE4:
		format = VK_FORMAT_R8G8B8A8_UINT;
		break;

	case VertexElementType::e_UBYTE4N:
		format = VK_FORMAT_R8G8B8A8_UNORM;
		break;

	case VertexElementType::e_USHORT2N:
		format = VK_FORMAT_R16G16_UNORM;
		break;

	case VertexElementType::e_USHORT4N:
		format = VK_FORMAT_R16G16B16A16_UNORM;
		break;

	default:
		break;
	}

	return format;
}



bool CPipelineManager::SPipeline::SetRenderTargetFormat(unsigned int nSlot, ETextureFormat format)
{
	ASSERT(m_eType == e_GraphicsPipeline);
	ASSERT(nSlot < 8);

	m_bRenderTargetBound[nSlot] = true;
	m_State.m_RenderTargetFormat[nSlot] = format;

	return true;
}



bool CPipelineManager::SPipeline::SetSampleDesc(int nSampleCount, int nSampleQuality)
{
	m_State.m_RasterState.m_SampleCount = nSampleCount;
	m_State.m_RasterState.m_SampleQuality = nSampleQuality;

	return true;
}



bool CPipelineManager::SPipeline::SetDepthStencilFormat(ETextureFormat format)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_State.m_DepthStencilFormat = format;

	return true;
}


void CPipelineManager::SPipeline::BindSampler(unsigned int nSlot, unsigned int nSamplerID)
{
	if (m_nSamplerIDs.size() < nSlot + 1)
		m_nSamplerIDs.resize(nSlot + 1);

	m_nSamplerIDs[nSlot] = nSamplerID;
}


bool CPipelineManager::SPipeline::SetShadingRate(EShadingRate eRate)
{
	return true;
}


unsigned int CPipelineManager::SPipeline::GetNumRenderTargets()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	return m_State.m_NumRenderTargets;
}


VkShaderStageFlags ConvertShaderStages(int shaderStage)
{
	VkShaderStageFlags stageFlags = 0;

	if (shaderStage & CShader::e_ComputeShader)
		stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

	if (shaderStage & CShader::e_VertexShader)
		stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;

	if (shaderStage & CShader::e_HullShader)
		stageFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

	if (shaderStage & CShader::e_DomainShader)
		stageFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	if (shaderStage & CShader::e_GeometryShader)
		stageFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;

	if (shaderStage & CShader::e_FragmentShader)
		stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;

	if (shaderStage & CShader::e_RayGenShader)
		stageFlags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	if (shaderStage & CShader::e_MissShader)
		stageFlags |= VK_SHADER_STAGE_MISS_BIT_KHR;

	if (shaderStage & CShader::e_ClosestHitShader)
		stageFlags |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

	if (shaderStage & CShader::e_IntersectionShader)
		stageFlags |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

	if (shaderStage & CShader::e_AnyHitShader)
		stageFlags |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

	return stageFlags;
}


VkShaderStageFlagBits ConvertShaderStage(CShader::EShaderType eType)
{
	switch (eType)
	{
	case CShader::e_ComputeShader:
			return VK_SHADER_STAGE_COMPUTE_BIT;

		case CShader::e_VertexShader:
			return VK_SHADER_STAGE_VERTEX_BIT;

		case CShader::e_HullShader:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

		case CShader::e_DomainShader:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

		case CShader::e_GeometryShader:
			return VK_SHADER_STAGE_GEOMETRY_BIT;

		case CShader::e_FragmentShader:
			return VK_SHADER_STAGE_FRAGMENT_BIT;

		case CShader::e_RayGenShader:
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		case CShader::e_MissShader:
			return VK_SHADER_STAGE_MISS_BIT_KHR;

		case CShader::e_ClosestHitShader:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

		case CShader::e_IntersectionShader:
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;

		case CShader::e_AnyHitShader:
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

		default:
			break;
	}

	return VK_SHADER_STAGE_ALL;
}


bool CPipelineManager::SPipeline::Create()
{
	SPipeline* existingPipeline = CPipelineManager::FindSimilarPipeline(this);

	if (existingPipeline != nullptr)
	{
		m_pPipelineState = existingPipeline->m_pPipelineState;
		m_pRootSignature = existingPipeline->m_pRootSignature;
		m_pDescriptorLayout = existingPipeline->m_pDescriptorLayout;

		m_bAliasedPipeline = true;
	}

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	if (m_eType == e_RayTracingPipeline)
	{
		VkDescriptorSetLayoutBinding binding{};
		binding.binding = m_RTAccelerationStructureSlot;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		binding.descriptorCount = 1;
		binding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		layoutBindings.push_back(binding);
	}

	if (m_NumTextures.size() > 0)
	{
		int numBinding = static_cast<int>(m_NumTextures.size());

		for (int i = 0; i < numBinding; i++)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = m_NumTextures[i].slot;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			binding.descriptorCount = m_NumTextures[i].count;
			binding.stageFlags = ConvertShaderStages(m_NumTextures[i].stage);

			layoutBindings.push_back(binding);
		}
	}

	if (m_NumBuffers.size() > 0)
	{
		int numBinding = static_cast<int>(m_NumBuffers.size());

		for (int i = 0; i < numBinding; i++)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = m_NumBuffers[i].slot;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = m_NumBuffers[i].count;
			binding.stageFlags = ConvertShaderStages(m_NumBuffers[i].stage);

			layoutBindings.push_back(binding);
		}
	}

	if (m_NumRwTextures.size() > 0)
	{
		int numBinding = static_cast<int>(m_NumRwTextures.size());

		for (int i = 0; i < numBinding; i++)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = m_NumRwTextures[i].slot;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = m_NumRwTextures[i].count;
			binding.stageFlags = ConvertShaderStages(m_NumRwTextures[i].stage);

			layoutBindings.push_back(binding);
		}
	}

	if (m_NumRwBuffers.size() > 0)
	{
		int numBinding = static_cast<int>(m_NumRwBuffers.size());

		for (int i = 0; i < numBinding; i++)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = m_NumRwBuffers[i].slot;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			binding.descriptorCount = m_NumRwBuffers[i].count;
			binding.stageFlags = ConvertShaderStages(m_NumRwBuffers[i].stage);

			layoutBindings.push_back(binding);
		}
	}

	if (m_NumSamplers.size() > 0)
	{
		int numBinding = static_cast<int>(m_NumSamplers.size());

		for (int i = 0; i < numBinding; i++)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = m_NumSamplers[i].slot;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			binding.descriptorCount = m_NumSamplers[i].count;
			binding.stageFlags = ConvertShaderStages(m_NumSamplers[i].stage);

			layoutBindings.push_back(binding);
		}
	}

	VkPipelineShaderStageCreateInfo shaderStages[6] = {};

	m_nDynamicOffsets.clear();
	m_nDynamicBufferBinding.clear();

	int numStages = 0;

	std::vector<VkPushConstantRange>					pushConstants;
	std::vector<VkPipelineShaderStageCreateInfo>		stages;
	std::vector<VkRayTracingShaderGroupCreateInfoKHR>	hitGroups;

	uint32_t missCount = 0;
	uint32_t hitCount = 0;

	if (m_eType == e_RayTracingPipeline)
	{
		for (CShader::SShader* pShader : m_ShaderLibrary)
		{
			VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
			stage.pName = "main";
			stage.module = (VkShaderModule)pShader->m_pShader;
			stage.stage = ConvertShaderStage(pShader->m_eType);

			stages.push_back(stage);

			if (pShader->m_nPushConstantSize > 0)
			{
				VkPushConstantRange range{};
				range.stageFlags = stage.stage;
				range.offset = 0;
				range.size = static_cast<uint32_t>(pShader->m_nPushConstantSize);

				pushConstants.push_back(range);
			}

			unsigned int numConstantBuffers = static_cast<unsigned int>(pShader->m_nConstantBuffers.size());
			m_NumConstantBuffers += numConstantBuffers;

			for (unsigned int i = 0; i < numConstantBuffers; i++)
			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = pShader->m_nConstantBuffers[i].m_nSlot;
				binding.descriptorCount = 1;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				binding.stageFlags = stage.stage;

				layoutBindings.push_back(binding);
				m_nDynamicOffsets.push_back(0);
				m_nDynamicBufferBinding.push_back(binding.binding);
			}

			std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
		}

		for (SHitGroup& hitGroup : m_HitGroups)
		{
			VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
			
			if (hitGroup.generalShader == INVALIDHANDLE)
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			else
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;

			group.anyHitShader			= hitGroup.anyHitShader			== INVALIDHANDLE ? VK_SHADER_UNUSED_KHR : hitGroup.anyHitShader;
			group.closestHitShader		= hitGroup.closestHitShader		== INVALIDHANDLE ? VK_SHADER_UNUSED_KHR : hitGroup.closestHitShader;
			group.generalShader			= hitGroup.generalShader		== INVALIDHANDLE ? VK_SHADER_UNUSED_KHR : hitGroup.generalShader;
			group.intersectionShader	= hitGroup.intersectionShader	== INVALIDHANDLE ? VK_SHADER_UNUSED_KHR : hitGroup.intersectionShader;

			hitGroups.push_back(group);

			if (group.generalShader != INVALIDHANDLE && m_ShaderLibrary[group.generalShader]->m_eType == CShader::e_MissShader)
				missCount++;

			if (group.anyHitShader != INVALIDHANDLE || group.closestHitShader != INVALIDHANDLE)
				hitCount++;
		}
	}

	else
	{
		if (m_nProgramID != INVALIDHANDLE)
		{
			CShader::SProgramDesc program = CShader::ms_ProgramDesc[m_nProgramID];

			m_NumConstantBuffers = 0;

			if (program.m_nComputeShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nComputeShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_COMPUTE_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}

			if (program.m_nVertexShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nVertexShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_VERTEX_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}

			if (program.m_nHullShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nHullShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}

			if (program.m_nDomainShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nDomainShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}

			if (program.m_nGeometryShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nGeometryShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}

			if (program.m_nPixelShaderID != INVALIDHANDLE)
			{
				CShader::SShader& shader = CShader::ms_Shaders[program.m_nPixelShaderID];

				shaderStages[numStages].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStages[numStages].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				shaderStages[numStages].module = (VkShaderModule)(shader.m_pShader);
				shaderStages[numStages].pName = "main";
				numStages++;

				if (shader.m_nPushConstantSize > 0)
				{
					VkPushConstantRange range{};
					range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					range.offset = 0;
					range.size = static_cast<uint32_t>(shader.m_nPushConstantSize);

					pushConstants.push_back(range);
				}

				unsigned int numConstantBuffers = static_cast<unsigned int>(shader.m_nConstantBuffers.size());
				m_NumConstantBuffers += numConstantBuffers;

				for (unsigned int i = 0; i < numConstantBuffers; i++)
				{
					VkDescriptorSetLayoutBinding binding{};
					binding.binding = shader.m_nConstantBuffers[i].m_nSlot;
					binding.descriptorCount = 1;
					binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

					layoutBindings.push_back(binding);
					m_nDynamicOffsets.push_back(0);
					m_nDynamicBufferBinding.push_back(binding.binding);
				}

				std::sort(m_nDynamicBufferBinding.begin(), m_nDynamicBufferBinding.end());
			}
		}

		else
			return true;
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<unsigned int>(layoutBindings.size());
	layoutInfo.pBindings	= layoutBindings.data();

	VkResult res = vkCreateDescriptorSetLayout(CDeviceManager::GetDevice(), &layoutInfo, nullptr, (VkDescriptorSetLayout*)&m_pDescriptorLayout);
	ASSERT(res == VK_SUCCESS);

	for (unsigned int i = 0; i < CDeviceManager::ms_FrameCount; i++)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = (VkDescriptorPool)CResourceManager::GetLocalSRVDecriptorHeap(i);
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = (VkDescriptorSetLayout*)&m_pDescriptorLayout;

		std::vector<void*> sets;

		for (unsigned int j = 0; j < m_nMaxNumVersions; j++)
		{
			VkDescriptorSet descriptorSet;

			if (layoutBindings.size() > 0)
			{
				res = vkAllocateDescriptorSets(CDeviceManager::GetDevice(), &allocInfo, &descriptorSet);
				ASSERT(res == VK_SUCCESS);
			}

			else
				descriptorSet = nullptr;

			sets.push_back(descriptorSet);
		}

		m_pDescriptorSets.push_back(sets);
	}

	if (existingPipeline != nullptr)
		return true;

	VkPipelineLayoutCreateInfo	pipelineLayoutInfo{};
	pipelineLayoutInfo.sType					= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount			= 1;
	pipelineLayoutInfo.pSetLayouts				= (VkDescriptorSetLayout*)&m_pDescriptorLayout;
	pipelineLayoutInfo.pushConstantRangeCount	= static_cast<uint32_t>(pushConstants.size());
	pipelineLayoutInfo.pPushConstantRanges		= pushConstants.data();

	res = vkCreatePipelineLayout(CDeviceManager::GetDevice(), &pipelineLayoutInfo, nullptr, (VkPipelineLayout*)&m_pRootSignature);
	ASSERT(res == VK_SUCCESS);
		
	if (m_eType == e_GraphicsPipeline)
	{
		std::vector<VkVertexInputAttributeDescription> vertexDescArray;
		std::vector<VkVertexInputBindingDescription> vertexBindingArray;

		int numStreams = 0;
		unsigned int vertexDeclarationMask = 0;
		
		if (m_nProgramID != INVALIDHANDLE)
			vertexDeclarationMask = CShader::GetProgramVertexDeclarationMask(m_nProgramID);

		bool usesSkinning = (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine) && (vertexDeclarationMask & g_VertexStreamSemantics[e_BLENDWEIGHT].m_StreamMask);
		
		uint8_t maxVertexElement = (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine) ? e_MaxVertexElementUsage : e_MaxStandardVertexElementUsage;

		for (uint8_t i = 0; i < maxVertexElement; i++)
		{
			unsigned int streamMask = CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine ? g_VertexStreamSemantics[i].m_StreamMask : g_VertexStreamStandardSemantics[i].m_StreamMask;

			if (vertexDeclarationMask & streamMask)
			{
				VkVertexInputAttributeDescription vertexDesc;
				vertexDesc.binding	= numStreams;
				vertexDesc.location = numStreams;

				VkVertexInputBindingDescription bindingDesc;
				bindingDesc.binding = numStreams;
				
				if (CRenderer::GetVertexLayout() == e_Vertex_Layout_Engine)
				{
					vertexDesc.format = ConvertVertexFormat(g_VertexStreamSemantics[i].m_Type);
					vertexDesc.offset = 0;

					bindingDesc.inputRate = g_VertexStreamSemantics[i].m_InputSlotClass == e_PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;

					if (bindingDesc.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
						bindingDesc.stride = usesSkinning ? g_VertexStrideSkin : g_VertexStrideNoSkin;
					else
						bindingDesc.stride = g_InstanceStride;
				}

				else
				{
					vertexDesc.format = ConvertVertexFormat(g_VertexStreamStandardSemantics[i].m_Type);
					vertexDesc.offset = 0;

					bindingDesc.inputRate = g_VertexStreamStandardSemantics[i].m_InputSlotClass == e_PerVertex ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
					
					if (bindingDesc.inputRate == VK_VERTEX_INPUT_RATE_VERTEX)
						bindingDesc.stride = g_VertexStrideStandard;
					else
						bindingDesc.stride = g_InstanceStride;
				}

				vertexDescArray.push_back(vertexDesc);
				vertexBindingArray.push_back(bindingDesc);

				numStreams++;
			}
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexAttributeDescriptionCount		= static_cast<unsigned int>(vertexDescArray.size());
		vertexInputInfo.vertexBindingDescriptionCount		= static_cast<unsigned int>(vertexBindingArray.size());
		vertexInputInfo.pVertexAttributeDescriptions		= vertexDescArray.data();
		vertexInputInfo.pVertexBindingDescriptions			= vertexBindingArray.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology					= gs_TopologyAssociation[m_State.m_Topology];
		inputAssembly.primitiveRestartEnable	= VK_FALSE;

		m_State.m_NumRenderTargets = 0;

		int numAttachments = 0;
		for (int i = 0; i < 8; i++)
		{
			if (m_bRenderTargetBound[i])
			{
				m_State.m_NumRenderTargets++;
			}
		}

		numAttachments = m_State.m_NumRenderTargets;

		if (m_State.m_DepthStencilFormat != ETextureFormat::e_UNKOWN)
		{
			numAttachments++;
		}

		VkViewport viewport{};
		VkRect2D scissor{};

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable			= m_State.m_RasterState.m_bDepthClamp ? VK_TRUE : VK_FALSE;
		rasterizer.rasterizerDiscardEnable	= VK_FALSE;
		rasterizer.cullMode					= gs_RasterCullModeAssociation[m_State.m_RasterState.m_CullMode];
		rasterizer.frontFace				= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable			= ((m_State.m_RasterState.m_DepthBias > 0.f) || (m_State.m_RasterState.m_SlopeBias > 0.f)) ? VK_TRUE : VK_FALSE;
		rasterizer.depthBiasConstantFactor	= m_State.m_RasterState.m_DepthBias;
		rasterizer.depthBiasSlopeFactor		= m_State.m_RasterState.m_SlopeBias;
		rasterizer.polygonMode				= gs_RasterFillModeAssociation[m_State.m_RasterState.m_FillMode];
		rasterizer.lineWidth				= 2.f;

		
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable	= m_State.m_RasterState.m_SampleCount > 1 ? VK_TRUE : VK_FALSE;
		multisampling.rasterizationSamples	= (VkSampleCountFlagBits)(1 << (m_State.m_RasterState.m_SampleCount - 1));
		multisampling.minSampleShading		= 1.f;
		multisampling.pSampleMask			= nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable		= VK_FALSE;


		VkPipelineDepthStencilStateCreateInfo depthstencil{};
		depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthstencil.depthTestEnable		= m_State.m_DepthStencilState.m_bDepthEnable ? VK_TRUE : VK_FALSE; 
		depthstencil.depthWriteEnable		= m_State.m_DepthStencilState.m_bEnableWrite ? VK_TRUE : VK_FALSE; 
		depthstencil.stencilTestEnable		= m_State.m_DepthStencilState.m_bStencilEnable ? VK_TRUE : VK_FALSE; 
		depthstencil.depthBoundsTestEnable	= VK_FALSE;
		depthstencil.minDepthBounds			= 0.f;
		depthstencil.maxDepthBounds			= 1.f;
		depthstencil.depthCompareOp			= gs_CmpFuncAssociation[m_State.m_DepthStencilState.m_DepthFunc];

		depthstencil.front.compareMask		= m_State.m_DepthStencilState.m_ReadMask;
		depthstencil.front.writeMask		= m_State.m_DepthStencilState.m_WriteMask;
		depthstencil.front.compareOp		= gs_CmpFuncAssociation[m_State.m_DepthStencilState.m_FrontStencil.m_Func];
		depthstencil.front.passOp			= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_FrontStencil.m_PassOp];
		depthstencil.front.failOp			= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_FrontStencil.m_FailOp];
		depthstencil.front.depthFailOp		= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_FrontStencil.m_DepthFailOp];
		depthstencil.front.reference		= 0;

		depthstencil.back.compareMask		= m_State.m_DepthStencilState.m_ReadMask;
		depthstencil.back.writeMask			= m_State.m_DepthStencilState.m_WriteMask;
		depthstencil.back.compareOp			= gs_CmpFuncAssociation[m_State.m_DepthStencilState.m_BackStencil.m_Func];
		depthstencil.back.passOp			= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_BackStencil.m_PassOp];
		depthstencil.back.failOp			= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_BackStencil.m_FailOp];
		depthstencil.back.depthFailOp		= gs_StencilOpAssociation[m_State.m_DepthStencilState.m_BackStencil.m_DepthFailOp];
		depthstencil.back.reference			= 0;


		VkPipelineColorBlendAttachmentState colorBlendAttachment[8]{};

		for (unsigned int i = 0; i < m_State.m_NumRenderTargets; i++)
		{
			int index = m_State.m_bIndependentBlendEnabled ? i : 0;
			colorBlendAttachment[i].colorWriteMask			= (VkColorComponentFlagBits)(m_State.m_BlendStates[index].m_WriteMask);
			colorBlendAttachment[i].blendEnable				= m_State.m_BlendStates[index].m_bBlendEnabled ? VK_TRUE : VK_FALSE;
			colorBlendAttachment[i].srcColorBlendFactor		= gs_BlendFuncAssociation[m_State.m_BlendStates[index].m_SrcFunc];
			colorBlendAttachment[i].dstColorBlendFactor		= gs_BlendFuncAssociation[m_State.m_BlendStates[index].m_DstFunc];
			colorBlendAttachment[i].colorBlendOp			= gs_BlendOpAssociation[m_State.m_BlendStates[index].m_BlendOp];
			colorBlendAttachment[i].srcAlphaBlendFactor		= gs_BlendFuncAssociation[m_State.m_BlendStates[index].m_SrcAlphaFunc];
			colorBlendAttachment[i].dstAlphaBlendFactor		= gs_BlendFuncAssociation[m_State.m_BlendStates[index].m_DstAlphaFunc];
			colorBlendAttachment[i].alphaBlendOp			= gs_BlendOpAssociation[m_State.m_BlendStates[index].m_AlphaBlendOp];
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable			= m_State.m_BlendStates[0].m_bLogicOpEnabled ? VK_TRUE : VK_FALSE;
		colorBlending.logicOp				= gs_LogicOpAssociation[m_State.m_BlendStates[0].m_LogicOp];
		colorBlending.attachmentCount		= m_State.m_NumRenderTargets;
		colorBlending.pAttachments			= colorBlendAttachment;
		colorBlending.blendConstants[0]		= 0.f;
		colorBlending.blendConstants[1]		= 0.f;
		colorBlending.blendConstants[2]		= 0.f;
		colorBlending.blendConstants[3]		= 0.f;


		VkPipelineTessellationStateCreateInfo tessState{};
		tessState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;


		VkDynamicState dynamicStates[] = 
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_STENCIL_REFERENCE
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = lengthof(dynamicStates);
		dynamicState.pDynamicStates = dynamicStates;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount				= numStages;
		pipelineInfo.pStages				= shaderStages;
		pipelineInfo.pVertexInputState		= &vertexInputInfo;
		pipelineInfo.pInputAssemblyState	= &inputAssembly;
		pipelineInfo.pTessellationState		= &tessState;
		pipelineInfo.pViewportState			= &viewportState;
		pipelineInfo.pRasterizationState	= &rasterizer;
		pipelineInfo.pMultisampleState		= &multisampling;
		pipelineInfo.pDepthStencilState		= &depthstencil;
		pipelineInfo.pColorBlendState		= &colorBlending;
		pipelineInfo.pDynamicState			= &dynamicState;

		CRenderPass* pCurrent = CRenderPass::ms_pCurrentSubPass == nullptr ? CRenderPass::ms_pCurrent : CRenderPass::ms_pCurrentSubPass;

		VkPipelineRenderingCreateInfo pipeline_rendering_create_info{};
		pipeline_rendering_create_info.sType					= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipeline_rendering_create_info.colorAttachmentCount		= pCurrent->m_nNumColorAttachments;
		pipeline_rendering_create_info.pColorAttachmentFormats	= (VkFormat*)pCurrent->m_pColorAttachmentFormats;

		if (pCurrent->m_pDepthAttachment)
			pipeline_rendering_create_info.depthAttachmentFormat	= ConvertFormat(pCurrent->m_DepthFormat);

		if (pCurrent->m_pStencilAttachment)
			pipeline_rendering_create_info.stencilAttachmentFormat	= ConvertFormat(pCurrent->m_StencilFormat);

		pipelineInfo.layout					= (VkPipelineLayout)m_pRootSignature;
		pipelineInfo.basePipelineHandle		= VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex		= -1;
		pipelineInfo.renderPass				= VK_NULL_HANDLE; 
		pipelineInfo.pNext					= &pipeline_rendering_create_info;

		res = vkCreateGraphicsPipelines(CDeviceManager::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, (VkPipeline*)&m_pPipelineState);
		ASSERT(res == VK_SUCCESS);
	}

	else if (m_eType == e_ComputePipeline)
	{
		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage				= shaderStages[0];
		pipelineInfo.layout				= (VkPipelineLayout)m_pRootSignature;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex	= -1;

		res = vkCreateComputePipelines(CDeviceManager::GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, (VkPipeline*)&m_pPipelineState);
		ASSERT(res == VK_SUCCESS);
	}

	else
	{
		VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{};
		rayPipelineInfo.sType		= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayPipelineInfo.stageCount	= static_cast<uint32_t>(stages.size());
		rayPipelineInfo.pStages		= stages.data();
		rayPipelineInfo.groupCount	= static_cast<uint32_t>(hitGroups.size());
		rayPipelineInfo.pGroups		= hitGroups.data();

		rayPipelineInfo.layout		= (VkPipelineLayout)m_pRootSignature;
		rayPipelineInfo.maxPipelineRayRecursionDepth = 1;

		CDeviceManager::vkCreateRayTracingPipelinesKHR(CDeviceManager::GetDevice(), {}, {}, 1, & rayPipelineInfo, nullptr, (VkPipeline*)&m_pPipelineState);
		
		uint32_t handleCount	= 1 + missCount + hitCount;
		uint32_t handleSize		= CRTX::ms_Properties.shaderGroupHandleSize;

		// The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
		uint32_t handleSizeAligned = (handleSize + (CRTX::ms_Properties.shaderGroupHandleAlignment - 1)) & ~(CRTX::ms_Properties.shaderGroupHandleAlignment - 1);
		
		m_rgenRegion.stride = (handleSizeAligned + (CRTX::ms_Properties.shaderGroupBaseAlignment - 1)) & ~(CRTX::ms_Properties.shaderGroupBaseAlignment - 1);
		m_rgenRegion.size   = m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
		m_missRegion.stride = handleSizeAligned;
		m_missRegion.size   = (missCount * handleSizeAligned + (CRTX::ms_Properties.shaderGroupBaseAlignment - 1)) & ~(CRTX::ms_Properties.shaderGroupBaseAlignment - 1);
		m_hitRegion.stride  = handleSizeAligned;
		m_hitRegion.size    = (hitCount * handleSizeAligned + (CRTX::ms_Properties.shaderGroupBaseAlignment - 1)) & ~(CRTX::ms_Properties.shaderGroupBaseAlignment - 1);
		m_callRegion.stride = handleSizeAligned;
		m_callRegion.size	= 0;

		uint32_t             dataSize = handleCount * handleSize;
		std::vector<uint8_t> handles(dataSize);

		VkResult result = CDeviceManager::vkGetRayTracingShaderGroupHandlesKHR(CDeviceManager::GetDevice(), (VkPipeline)m_pPipelineState, 0, handleCount, dataSize, handles.data());
		ASSERT(result == VK_SUCCESS);

		VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;

		m_rtSBTBuffer = CResourceManager::CreateShaderBindingTableBuffer(sbtSize);

		VkDeviceAddress sbtAddress	= (VkDeviceAddress)CResourceManager::GetBufferDeviceAddress(m_rtSBTBuffer);
		m_rgenRegion.deviceAddress	= sbtAddress;
		m_missRegion.deviceAddress	= sbtAddress + m_rgenRegion.size;
		m_hitRegion.deviceAddress	= sbtAddress + m_rgenRegion.size + m_missRegion.size;
		m_callRegion.deviceAddress	= sbtAddress + m_rgenRegion.size + m_missRegion.size + m_hitRegion.size;

		void* pSBTBuffer = CResourceManager::MapBuffer(m_rtSBTBuffer);
		uint32_t handleIndex = 0;

		uint8_t* pData = reinterpret_cast<uint8_t*>(pSBTBuffer);

		memcpy(pData, handles.data() + handleIndex * handleSize, handleSize);
		handleIndex++;
		pData += m_rgenRegion.size;

		for (uint32_t i = 0; i < missCount; i++)
		{
			memcpy(pData, handles.data() + handleIndex * handleSize, handleSize);
			handleIndex++;
			pData += m_missRegion.size;
		}

		for (uint32_t i = 0; i < hitCount; i++)
		{
			memcpy(pData, handles.data() + handleIndex * handleSize, handleSize);
			handleIndex++;
			pData += m_hitRegion.size;
		}

		CResourceManager::UnmapBuffer(m_rtSBTBuffer);
	}

	return true;
}



CPipelineManager::SPipeline* CPipelineManager::FindSimilarPipeline(CPipelineManager::SPipeline* pipeline)
{
	std::vector<SPipeline*>::iterator it;
	SPipeline* currentPipeline = nullptr;
	bool bFound = false;

	for (it = ms_pPipelines.begin(); it < ms_pPipelines.end(); it++)
	{
		currentPipeline = *it;

		if (currentPipeline == pipeline)
			continue;

		if (currentPipeline->m_eType != pipeline->m_eType)
			continue;

		if (memcmp(&pipeline->m_State, &currentPipeline->m_State, sizeof(pipeline->m_State)) == 0)
		{
			if (pipeline->m_nProgramID != INVALIDHANDLE && currentPipeline->m_nProgramID != INVALIDHANDLE && !memcmp(&CShader::ms_ProgramDesc[currentPipeline->m_nProgramID], &CShader::ms_ProgramDesc[pipeline->m_nProgramID], sizeof(CShader::SProgramDesc)))
			{
				bFound = true;
				break;
			}
		}
	}

	return bFound ? currentPipeline : nullptr;
}
