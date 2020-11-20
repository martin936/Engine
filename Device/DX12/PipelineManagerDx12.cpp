#include "../PipelineManager.h"
#include "../RenderThreads.h"
#include "../CommandListManager.h"
#include "../ResourceManager.h"


extern thread_local CDeviceManager::SInternalEngineState *g_CurrentEngineState;

struct SInputLayout
{
	unsigned int			m_nVertexDeclaration;
	D3D12_INPUT_LAYOUT_DESC	m_pInputLayout;
};

extern std::vector<SInputLayout> gs_VertexDeclarations[CDeviceManager::e_VertexFormatConfig_Max];

std::vector<CPipelineManager::SPipeline*>	CPipelineManager::ms_pPipelines;
std::vector<CPipelineManager::SPipeline*>	CPipelineManager::ms_pCurrentPipeline;

CMutex*								CPipelineManager::ms_pLock = nullptr;

const D3D12_BLEND gs_BlendFuncAssociation[] =
{
	D3D12_BLEND_ZERO,
	D3D12_BLEND_ONE,
	D3D12_BLEND_SRC_COLOR,
	D3D12_BLEND_INV_SRC_COLOR,
	D3D12_BLEND_SRC_ALPHA,
	D3D12_BLEND_INV_SRC_ALPHA,
	D3D12_BLEND_DEST_ALPHA,
	D3D12_BLEND_INV_DEST_ALPHA,
	D3D12_BLEND_DEST_COLOR,
	D3D12_BLEND_INV_DEST_COLOR,
	D3D12_BLEND_SRC_ALPHA_SAT,
	D3D12_BLEND_BLEND_FACTOR,
	D3D12_BLEND_INV_BLEND_FACTOR,
	D3D12_BLEND_SRC1_COLOR,
	D3D12_BLEND_INV_SRC1_COLOR,
	D3D12_BLEND_SRC1_ALPHA,
	D3D12_BLEND_INV_SRC1_ALPHA
};


const D3D12_BLEND_OP gs_BlendOpAssociation[] =
{
	D3D12_BLEND_OP_ADD,
	D3D12_BLEND_OP_SUBTRACT,
	D3D12_BLEND_OP_REV_SUBTRACT,
	D3D12_BLEND_OP_MIN,
	D3D12_BLEND_OP_MAX
};


const D3D12_LOGIC_OP gs_LogicOpAssociation[] =
{
	D3D12_LOGIC_OP_CLEAR,
	D3D12_LOGIC_OP_SET,
	D3D12_LOGIC_OP_COPY,
	D3D12_LOGIC_OP_COPY_INVERTED,
	D3D12_LOGIC_OP_NOOP,
	D3D12_LOGIC_OP_INVERT,
	D3D12_LOGIC_OP_AND,
	D3D12_LOGIC_OP_NAND,
	D3D12_LOGIC_OP_OR,
	D3D12_LOGIC_OP_NOR,
	D3D12_LOGIC_OP_XOR,
	D3D12_LOGIC_OP_EQUIV,
	D3D12_LOGIC_OP_AND_REVERSE,
	D3D12_LOGIC_OP_AND_INVERTED,
	D3D12_LOGIC_OP_OR_REVERSE,
	D3D12_LOGIC_OP_OR_INVERTED
};


const D3D12_FILL_MODE gs_RasterFillModeAssociation[] =
{
	D3D12_FILL_MODE_WIREFRAME,
	D3D12_FILL_MODE_SOLID
};


const D3D12_COMPARISON_FUNC gs_CmpFuncAssociation[] =
{
	D3D12_COMPARISON_FUNC_NEVER,
	D3D12_COMPARISON_FUNC_LESS,
	D3D12_COMPARISON_FUNC_EQUAL,
	D3D12_COMPARISON_FUNC_LESS_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER,
	D3D12_COMPARISON_FUNC_NOT_EQUAL,
	D3D12_COMPARISON_FUNC_GREATER_EQUAL,
	D3D12_COMPARISON_FUNC_ALWAYS
};


const D3D12_STENCIL_OP gs_StencilOpAssociation[] =
{
	D3D12_STENCIL_OP_KEEP,
	D3D12_STENCIL_OP_ZERO,
	D3D12_STENCIL_OP_REPLACE,
	D3D12_STENCIL_OP_INCR_SAT,
	D3D12_STENCIL_OP_DECR_SAT,
	D3D12_STENCIL_OP_INVERT,
	D3D12_STENCIL_OP_INCR,
	D3D12_STENCIL_OP_DECR
};


const D3D12_PRIMITIVE_TOPOLOGY_TYPE gs_TopologyAssociation[] =
{
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
	D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH
};



void CPipelineManager::Init()
{
	ms_pPipelines.clear();

	ms_pCurrentPipeline.resize(CRenderWorkerThread::GetNumThreads());

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

	if (pipeline->vertexDeclaration && pipeline->vertexShaderHandle)
	{
		CRenderManager::SetVertexDeclaration(pipeline->vertexDeclaration, pipeline->vertexShaderHandle);
		CRenderManager::SetVertexShaderHandle(pipeline->vertexShaderHandle);
	}

	if (pipeline->hullShaderHandle)
		CRenderManager::SetHullShaderHandle(pipeline->hullShaderHandle);

	if (pipeline->domainShaderHandle)
		CRenderManager::SetDomainShaderHandle(pipeline->domainShaderHandle);

	if (pipeline->geometryShaderHandle)
		CRenderManager::SetGeometryShaderHandle(pipeline->geometryShaderHandle);

	if (pipeline->pixelShaderHandle)
		CRenderManager::SetPixelShaderHandle(pipeline->pixelShaderHandle);

	if (pipeline->computeShaderHandle)
		CRenderManager::SetComputeShaderHandle(pipeline->computeShaderHandle);
}


void CPipelineManager::BindPipeline(unsigned int nCommandListID, unsigned int nPipelineStateID)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	if (nPipelineStateID == INVALIDHANDLE)
		return;

	ASSERT(nPipelineStateID > 0 && nPipelineStateID < ms_pPipelines.size() + 1);

	SPipeline* pipeline = ms_pPipelines[nPipelineStateID - 1];

	if (pipeline->m_gfxDesc.SampleDesc.Count > 1)
		CRenderManager::Set2XMSAASamplePositions();

	else
		CRenderManager::ResetMSAASamplePositions();

	BindShaders(nPipelineStateID);

	if (ms_pCurrentPipeline[nThreadID] != nullptr && ms_pCurrentPipeline[nThreadID]->m_pPipelineState != pipeline->m_pPipelineState)
	{
		ms_pCurrentPipeline[nThreadID] = pipeline;

		((ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nCommandListID))->SetPipelineState((ID3D12PipelineState*)pipeline->m_pPipelineState);

		if (pipeline->m_eType == e_GraphicsPipeline)
			((ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nCommandListID))->SetGraphicsRootSignature((ID3D12RootSignature*)pipeline->m_pRootSignature);

		if (pipeline->m_eType == e_ComputePipeline)
			((ID3D12GraphicsCommandList*)CCommandListManager::GetCommandListPtr(nCommandListID))->SetComputeRootSignature((ID3D12RootSignature*)pipeline->m_pRootSignature);
	}
}


CPipelineManager::SPipeline* CPipelineManager::GetCurrentPipeline()
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	return ms_pCurrentPipeline[nThreadID];
}


CPipelineManager::EPipelineType CPipelineManager::GetCurrentPipelineType()
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	return ms_pCurrentPipeline[nThreadID]->m_eType;
}


void CPipelineManager::SetCurrentPipeline(unsigned int nID)
{
	unsigned int nThreadID = CRenderWorkerThread::GetCurrentThreadWorkerID();

	ms_pCurrentPipeline[nThreadID] = GetPipelineState(nID);
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

	if (eType == e_GraphicsPipeline)
	{
		ZeroMemory(&m_gfxDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
		m_gfxDesc.SampleDesc.Count = 1;
		m_gfxDesc.SampleMask = UINT_MAX;
	}
	else
		ZeroMemory(&m_cmpDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC));

	m_NumCBV = m_NumSRV = m_NumUAV = 0;

	m_pPipelineState = nullptr;
	m_pRootSignature = nullptr;

	vertexDeclaration = vertexShaderHandle = hullShaderHandle = domainShaderHandle = geometryShaderHandle = pixelShaderHandle = computeShaderHandle = nullptr;
}


CPipelineManager::SPipeline::~SPipeline()
{
	if (m_pPipelineState != nullptr)
	{
		((ID3D12PipelineState*)m_pPipelineState)->Release();
		m_pPipelineState = nullptr;
	}
}


void CPipelineManager::SPipeline::CopyFrom(SPipeline* pSrc)
{
	ASSERT(pSrc != nullptr);

	*this = *pSrc;

	m_pPipelineState = nullptr;
	m_pRootSignature = nullptr;
}



bool CPipelineManager::SPipeline::SetVertexShader(const char* VertexShaderFile, void** VertexShaderHandle, void** VertexShaderDeclaration)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	bool ret = true;

	if (*VertexShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadVertexShader(VertexShaderFile, VertexShaderDeclaration, VertexShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetVertexDeclaration(*VertexShaderDeclaration, *VertexShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetVertexShaderHandle(*VertexShaderHandle) >= 0;

	vertexShaderHandle = *VertexShaderHandle;
	vertexDeclaration = *VertexShaderDeclaration;

	SVertexShaderHandle* VShader = (SVertexShaderHandle *)g_CurrentEngineState->m_ActiveVertexShader;

	m_gfxDesc.InputLayout.pInputElementDescs = gs_VertexDeclarations[g_CurrentEngineState->m_vertexFormatConfig][g_CurrentEngineState->m_ActiveVertexDeclarationID].m_pInputLayout.pInputElementDescs;
	m_gfxDesc.InputLayout.NumElements = gs_VertexDeclarations[g_CurrentEngineState->m_vertexFormatConfig][g_CurrentEngineState->m_ActiveVertexDeclarationID].m_pInputLayout.NumElements;

	m_gfxDesc.VS.pShaderBytecode = VShader->vShader.pShaderBytecode;
	m_gfxDesc.VS.BytecodeLength = VShader->vShader.BytecodeLength;

	return ret;
}



bool CPipelineManager::SPipeline::SetHullShader(const char* HullShaderFile, void** HullShaderHandle)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	bool ret = true;

	if (*HullShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadHullShader(HullShaderFile, HullShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetHullShaderHandle(*HullShaderHandle) >= 0;

	hullShaderHandle = *HullShaderHandle;

	SHullShaderHandle* HShader = (SHullShaderHandle *)g_CurrentEngineState->m_ActiveHullShader;

	m_gfxDesc.HS.pShaderBytecode = HShader->pShader.pShaderBytecode;
	m_gfxDesc.HS.BytecodeLength = HShader->pShader.BytecodeLength;

	return ret;
}



bool CPipelineManager::SPipeline::SetDomainShader(const char* DomainShaderFile, void** DomainShaderHandle)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	bool ret = true;

	if (*DomainShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadDomainShader(DomainShaderFile, DomainShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetDomainShaderHandle(*DomainShaderHandle) >= 0;

	domainShaderHandle = *DomainShaderHandle;

	SDomainShaderHandle* DShader = (SDomainShaderHandle *)g_CurrentEngineState->m_ActiveDomainShader;

	m_gfxDesc.DS.pShaderBytecode = DShader->pShader.pShaderBytecode;
	m_gfxDesc.DS.BytecodeLength = DShader->pShader.BytecodeLength;

	return ret;
}



bool CPipelineManager::SPipeline::SetGeometryShader(const char* GeometryShaderFile, void** GeometryShaderHandle)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	bool ret = true;

	if (*GeometryShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadGeometryShader(GeometryShaderFile, GeometryShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetGeometryShaderHandle(*GeometryShaderHandle) >= 0;

	geometryShaderHandle = *GeometryShaderHandle;

	SGeometryShaderHandle* GShader = (SGeometryShaderHandle *)g_CurrentEngineState->m_ActiveGeometryShader;

	m_gfxDesc.GS.pShaderBytecode = GShader->pShader.pShaderBytecode;
	m_gfxDesc.GS.BytecodeLength = GShader->pShader.BytecodeLength;

	return ret;
}



bool CPipelineManager::SPipeline::SetPixelShader(const char* PixelShaderFile, void** PixelShaderHandle)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	bool ret = true;

	if (*PixelShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadPixelShader(PixelShaderFile, PixelShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetPixelShaderHandle(*PixelShaderHandle) >= 0;

	pixelShaderHandle = *PixelShaderHandle;

	SPixelShaderHandle* PShader = (SPixelShaderHandle *)g_CurrentEngineState->m_ActivePixelShader;

	m_gfxDesc.PS.pShaderBytecode = PShader->pShader.pShaderBytecode;
	m_gfxDesc.PS.BytecodeLength = PShader->pShader.BytecodeLength;

	return ret;
}



bool CPipelineManager::SPipeline::SetComputeShader(const char* ComputeShaderFile, void** ComputeShaderHandle)
{
	ASSERT(m_eType == e_ComputePipeline);

	bool ret = true;

	if (*ComputeShaderHandle == nullptr)
		ret = CResourceLoaderInterface::LoadComputeShader(ComputeShaderFile, ComputeShaderHandle) >= 0;

	if (ret)
		ret = CRenderManager::SetComputeShaderHandle(*ComputeShaderHandle) >= 0;

	computeShaderHandle = *ComputeShaderHandle;

	SComputeShaderHandle* CShader = (SComputeShaderHandle *)g_CurrentEngineState->m_ActiveComputeShader;

	if (CShader)
	{
		m_cmpDesc.CS.pShaderBytecode = CShader->pShader.pShaderBytecode;
		m_cmpDesc.CS.BytecodeLength = CShader->pShader.BytecodeLength;
	}

	return ret;
}



void CPipelineManager::SPipeline::DisableBlend(unsigned char writeMask)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.BlendState.AlphaToCoverageEnable = false;
	m_gfxDesc.BlendState.IndependentBlendEnable = false;

	m_gfxDesc.BlendState.RenderTarget[0].BlendEnable = false;
	m_gfxDesc.BlendState.RenderTarget[0].LogicOpEnable = false;

	m_gfxDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	m_gfxDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	m_gfxDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	m_gfxDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	m_gfxDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	m_gfxDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	m_gfxDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;

	ASSERT((writeMask & 0xf) == writeMask);

	m_gfxDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = static_cast<UINT8>(writeMask);

	for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		m_gfxDesc.BlendState.RenderTarget[i] = m_gfxDesc.BlendState.RenderTarget[0];
}



bool CPipelineManager::SPipeline::SetBlendState(bool blendEnable, bool logicOpEnable, EBlendFunc::Enum srcBlend, EBlendFunc::Enum dstBlend, EBlendOp::Enum colorOp, EBlendFunc::Enum srcBlendAlpha, EBlendFunc::Enum dstBlendAlpha, EBlendOp::Enum alphaOp, int writeMask, ELogicOp::Enum logicOp)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.BlendState.AlphaToCoverageEnable = false;
	m_gfxDesc.BlendState.IndependentBlendEnable = false;

	m_gfxDesc.BlendState.RenderTarget[0].BlendEnable = blendEnable;
	m_gfxDesc.BlendState.RenderTarget[0].LogicOpEnable = logicOpEnable;

	if (blendEnable)
	{
		m_gfxDesc.BlendState.RenderTarget[0].SrcBlend = gs_BlendFuncAssociation[srcBlend];
		m_gfxDesc.BlendState.RenderTarget[0].DestBlend = gs_BlendFuncAssociation[dstBlend];
		m_gfxDesc.BlendState.RenderTarget[0].BlendOp = gs_BlendOpAssociation[colorOp];

		m_gfxDesc.BlendState.RenderTarget[0].SrcBlendAlpha = gs_BlendFuncAssociation[srcBlendAlpha];
		m_gfxDesc.BlendState.RenderTarget[0].DestBlendAlpha = gs_BlendFuncAssociation[dstBlendAlpha];
		m_gfxDesc.BlendState.RenderTarget[0].BlendOpAlpha = gs_BlendOpAssociation[alphaOp];
	}

	else
	{
		m_gfxDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		m_gfxDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		m_gfxDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

		m_gfxDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		m_gfxDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		m_gfxDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	if (logicOpEnable)
		m_gfxDesc.BlendState.RenderTarget[0].LogicOp = gs_LogicOpAssociation[logicOp];

	else
		m_gfxDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;

	ASSERT((writeMask & 0xf) == writeMask);

	m_gfxDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = static_cast<UINT8>(writeMask);

	for (UINT i = 1; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		m_gfxDesc.BlendState.RenderTarget[i] = m_gfxDesc.BlendState.RenderTarget[0];

	return true;
}



bool CPipelineManager::SPipeline::SetRenderTargetBlendState(int renderTargetSlot, bool blendEnable, bool logicOpEnable, EBlendFunc::Enum srcBlend, EBlendFunc::Enum dstBlend, EBlendOp::Enum colorOp, EBlendFunc::Enum srcBlendAlpha, EBlendFunc::Enum dstBlendAlpha, EBlendOp::Enum alphaOp, int writeMask, ELogicOp::Enum logicOp)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.BlendState.AlphaToCoverageEnable = false;
	m_gfxDesc.BlendState.IndependentBlendEnable = true;

	m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].BlendEnable = blendEnable;
	m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].LogicOpEnable = logicOpEnable;

	if (blendEnable)
	{
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].SrcBlend = gs_BlendFuncAssociation[srcBlend];
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].DestBlend = gs_BlendFuncAssociation[dstBlend];
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].BlendOp = gs_BlendOpAssociation[colorOp];

		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].SrcBlendAlpha = gs_BlendFuncAssociation[srcBlendAlpha];
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].DestBlendAlpha = gs_BlendFuncAssociation[dstBlendAlpha];
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].BlendOpAlpha = gs_BlendOpAssociation[alphaOp];
	}

	else
	{
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].SrcBlend = D3D12_BLEND_ONE;
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].DestBlend = D3D12_BLEND_ZERO;
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].BlendOp = D3D12_BLEND_OP_ADD;

		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].SrcBlendAlpha = D3D12_BLEND_ONE;
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].DestBlendAlpha = D3D12_BLEND_ZERO;
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	}

	if (logicOpEnable)
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].LogicOp = gs_LogicOpAssociation[logicOp];

	else
		m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].LogicOp = D3D12_LOGIC_OP_NOOP;

	m_gfxDesc.BlendState.RenderTarget[renderTargetSlot].RenderTargetWriteMask = static_cast<UINT8>(writeMask);

	return true;
}


bool CPipelineManager::SPipeline::SetRasterizerState(ERasterFillMode::Enum fillMode, ERasterCullMode::Enum cullMode, bool conservativeRaster, bool enableMultisampling)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.RasterizerState.FillMode = gs_RasterFillModeAssociation[fillMode];

	if (cullMode == ERasterCullMode::e_CullMode_None)
		m_gfxDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	else
		m_gfxDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

	if (cullMode == ERasterCullMode::e_CullMode_CW)
		m_gfxDesc.RasterizerState.FrontCounterClockwise = false;

	else if (cullMode == ERasterCullMode::e_CullMode_CCW)
		m_gfxDesc.RasterizerState.FrontCounterClockwise = true;

	m_gfxDesc.RasterizerState.ConservativeRaster = conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	m_gfxDesc.RasterizerState.MultisampleEnable = enableMultisampling;

	return true;
}



void CPipelineManager::SPipeline::DisableDepthStencil()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DepthStencilState.DepthEnable = false;
	m_gfxDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	m_gfxDesc.DepthStencilState.StencilEnable = false;
}


bool CPipelineManager::SPipeline::SetDepthState(bool depthEnable, ECmpFunc::Enum depthFunc, bool enableWrite)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DepthStencilState.DepthEnable = depthEnable;
	m_gfxDesc.DepthStencilState.DepthWriteMask = enableWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;

	if (depthEnable)
		m_gfxDesc.DepthStencilState.DepthFunc = gs_CmpFuncAssociation[depthFunc];

	return true;
}



void CPipelineManager::SPipeline::DisableStencil()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DepthStencilState.StencilEnable = false;
}


bool CPipelineManager::SPipeline::SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOpDX12::Enum frontFail, EStencilOpDX12::Enum frontDepthFail, EStencilOpDX12::Enum frontPass, ECmpFunc::Enum frontFunc, \
	EStencilOpDX12::Enum backFail, EStencilOpDX12::Enum backDepthFail, EStencilOpDX12::Enum backPass, ECmpFunc::Enum backFunc)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DepthStencilState.StencilEnable = stencilEnable;

	if (stencilEnable)
	{
		m_gfxDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(readMask);
		m_gfxDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(writeMask);

		m_gfxDesc.DepthStencilState.FrontFace.StencilFailOp = gs_StencilOpAssociation[frontFail];
		m_gfxDesc.DepthStencilState.FrontFace.StencilDepthFailOp = gs_StencilOpAssociation[frontDepthFail];
		m_gfxDesc.DepthStencilState.FrontFace.StencilPassOp = gs_StencilOpAssociation[frontPass];
		m_gfxDesc.DepthStencilState.FrontFace.StencilFunc = gs_CmpFuncAssociation[frontFunc];

		m_gfxDesc.DepthStencilState.BackFace.StencilFailOp = gs_StencilOpAssociation[backFail];
		m_gfxDesc.DepthStencilState.BackFace.StencilDepthFailOp = gs_StencilOpAssociation[backDepthFail];
		m_gfxDesc.DepthStencilState.BackFace.StencilPassOp = gs_StencilOpAssociation[backPass];
		m_gfxDesc.DepthStencilState.BackFace.StencilFunc = gs_CmpFuncAssociation[backFunc];
	}

	return true;
}


bool CPipelineManager::SPipeline::SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOpDX12::Enum frontFail, EStencilOpDX12::Enum frontDepthFail, EStencilOpDX12::Enum frontPass, ECmpFunc::Enum frontFunc)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DepthStencilState.StencilEnable = stencilEnable;

	if (stencilEnable)
	{
		m_gfxDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(readMask);
		m_gfxDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(writeMask);

		m_gfxDesc.DepthStencilState.FrontFace.StencilFailOp = gs_StencilOpAssociation[frontFail];
		m_gfxDesc.DepthStencilState.FrontFace.StencilDepthFailOp = gs_StencilOpAssociation[frontDepthFail];
		m_gfxDesc.DepthStencilState.FrontFace.StencilPassOp = gs_StencilOpAssociation[frontPass];
		m_gfxDesc.DepthStencilState.FrontFace.StencilFunc = gs_CmpFuncAssociation[frontFunc];

		m_gfxDesc.DepthStencilState.BackFace.StencilFailOp = gs_StencilOpAssociation[frontFail];
		m_gfxDesc.DepthStencilState.BackFace.StencilDepthFailOp = gs_StencilOpAssociation[frontDepthFail];
		m_gfxDesc.DepthStencilState.BackFace.StencilPassOp = gs_StencilOpAssociation[frontPass];
		m_gfxDesc.DepthStencilState.BackFace.StencilFunc = gs_CmpFuncAssociation[frontFunc];
	}

	return true;
}


bool CPipelineManager::SPipeline::SetPrimitiveTopology(ETopology::Enum eTopology)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.PrimitiveTopologyType = gs_TopologyAssociation[eTopology];

	return true;
}


DXGI_FORMAT ConvertFormat(CBitmapInterface::Format format)
{
	DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;

	switch (format)
	{
	case CBitmapInterface::e_TrueColor:
		Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		break;
	case CBitmapInterface::e_10BitsRGB2BitsA:
		Format = DXGI_FORMAT_R10G10B10A2_UNORM;
		break;
	case CBitmapInterface::e_8BitsRed:
		Format = DXGI_FORMAT_R8_UNORM;
		break;
	case CBitmapInterface::e_8BitsRG:
		Format = DXGI_FORMAT_R8G8_UNORM;
		break;
	case CBitmapInterface::e_8BitsRG_SNORM:
		Format = DXGI_FORMAT_R8G8_SNORM;
		break;
	case CBitmapInterface::e_16BitsRed:
		Format = DXGI_FORMAT_R8G8_B8G8_UNORM;
		break;
	case CBitmapInterface::e_Float16BitsRed:
		Format = DXGI_FORMAT_R16_FLOAT;
		break;
	case CBitmapInterface::e_16BitsRG:
		Format = DXGI_FORMAT_R16G16_UNORM;
		break;
	case CBitmapInterface::e_16BitsRG_SNORM:
		Format = DXGI_FORMAT_R16G16_SNORM;
		break;
	case CBitmapInterface::e_16BitsRGBA:
		Format = DXGI_FORMAT_R16G16B16A16_UNORM;
		break;
	case CBitmapInterface::e_Float16BitsRG:
		Format = DXGI_FORMAT_R16G16_FLOAT;
		break;
	case CBitmapInterface::e_Float16BitsRGBA:
		Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		break;
	case CBitmapInterface::e_Float32BitsRed:
		Format = DXGI_FORMAT_R32_FLOAT;
		break;
	case CBitmapInterface::e_Float32BitsRG:
		Format = DXGI_FORMAT_R32G32_FLOAT;
		break;
	case CBitmapInterface::e_Float32BitsRGBA:
		Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		break;
	case CBitmapInterface::e_Float11BitsRG10BitsB:
		Format = DXGI_FORMAT_R11G11B10_FLOAT;
		break;
	case CBitmapInterface::e_Unsigned8BitsR:
		Format = DXGI_FORMAT_R8_UINT;
		break;
	case CBitmapInterface::e_Unsigned16BitsR:
		Format = DXGI_FORMAT_R16_UINT;
		break;
	case CBitmapInterface::e_Unsigned32BitsR:
		Format = DXGI_FORMAT_R32_UINT;
		break;
	case CBitmapInterface::e_Unsigned32BitsRGBA:
		Format = DXGI_FORMAT_R32G32B32A32_UINT;
		break;
	case CBitmapInterface::e_DepthStencil:
		Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		break;
	case CBitmapInterface::e_Depth:
		Format = DXGI_FORMAT_D32_FLOAT;
		break;

	default:
		break;
	}

	return Format;
}



bool CPipelineManager::SPipeline::SetRenderTargetFormat(unsigned int nSlot, CBitmapInterface::Format format)
{
	ASSERT(m_eType == e_GraphicsPipeline);
	ASSERT(nSlot < 8);

	m_gfxDesc.RTVFormats[nSlot] = ConvertFormat(format);

	return true;
}



bool CPipelineManager::SPipeline::SetSampleDesc(int nSampleCount, int nSampleQuality)
{
	m_gfxDesc.SampleDesc.Count = nSampleCount;
	m_gfxDesc.SampleDesc.Quality = 0;

	return true;
}



bool CPipelineManager::SPipeline::SetDepthStencilFormat(CBitmapInterface::Format format)
{
	ASSERT(m_eType == e_GraphicsPipeline);

	m_gfxDesc.DSVFormat = ConvertFormat(format);

	return true;
}


void CPipelineManager::SPipeline::BindSampler(unsigned int nSlot, unsigned int nSamplerID)
{
	if (m_nSamplerIDs.size() < nSlot + 1)
		m_nSamplerIDs.resize(nSlot + 1);

	CResourceManager::CreateStaticSampler(&m_nSamplerIDs[nSlot], nSamplerID);

	m_nSamplerIDs[nSlot].RegisterSpace = 0;
	m_nSamplerIDs[nSlot].ShaderRegister = nSlot;
	m_nSamplerIDs[nSlot].ShaderVisibility = m_eType == e_GraphicsPipeline ? (D3D12_SHADER_VISIBILITY)m_nShaderVisibility : D3D12_SHADER_VISIBILITY_ALL;
}



void CPipelineManager::SPipeline::CreateRootSignature(unsigned int nNumCBV, unsigned int nNumSRV, unsigned int nNumUAV, unsigned int nNumSamplers, unsigned int nShaderVisibility)
{
	m_NumCBV = nNumCBV;
	m_NumSRV = nNumSRV;
	m_NumUAV = nNumUAV;
	m_NumSamplers = nNumSamplers;

	if (nShaderVisibility == CBitmapInterface::e_VertexShader)
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	if (nShaderVisibility == CBitmapInterface::e_HullShader)
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;

	if (nShaderVisibility == CBitmapInterface::e_DomainShader)
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;

	if (nShaderVisibility == CBitmapInterface::e_GeometryShader)
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;

	if (nShaderVisibility == CBitmapInterface::e_PixelShader)
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	else
		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}


bool CPipelineManager::SPipeline::SetShadingRate(EShadingRate::Enum eRate)
{
	return true;
}


unsigned int CPipelineManager::SPipeline::GetNumRenderTargets()
{
	ASSERT(m_eType == e_GraphicsPipeline);

	return m_gfxDesc.NumRenderTargets;
}


bool CPipelineManager::SPipeline::Create()
{
	SPipeline* existingPipeline = CPipelineManager::FindSimilarPipeline(this);

	if (existingPipeline != nullptr)
	{
		m_pPipelineState = existingPipeline->m_pPipelineState;
		m_pRootSignature = existingPipeline->m_pRootSignature;
		return true;
	}

	if (m_eType == e_GraphicsPipeline)
	{
		m_gfxDesc.NumRenderTargets = 0;

		for (int i = 0; i < 8; i++)
		{
			if (m_gfxDesc.RTVFormats[i] != 0)
				m_gfxDesc.NumRenderTargets++;

			else
				break;
		}
	}

	UINT numCB = 0;

	std::vector<D3D12_ROOT_PARAMETER> parameters;

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};

	unsigned int nCommandListID = CRenderWorkerThread::GetCurrentThreadCommandListID();

	if (m_eType == e_GraphicsPipeline)
	{
		SVertexShaderHandle*	VShader = (SVertexShaderHandle *)g_CurrentEngineState->m_ActiveVertexShader;
		SHullShaderHandle*		HShader = (SHullShaderHandle *)g_CurrentEngineState->m_ActiveHullShader;
		SDomainShaderHandle*	DShader = (SDomainShaderHandle *)g_CurrentEngineState->m_ActiveDomainShader;
		SGeometryShaderHandle*	GShader = (SGeometryShaderHandle *)g_CurrentEngineState->m_ActiveGeometryShader;
		SPixelShaderHandle*		PShader = (SPixelShaderHandle *)g_CurrentEngineState->m_ActivePixelShader;

		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		if (VShader && VShader->constantBufferId)
		{
			VShader->constantBufferSlot = numCB;
			numCB++;

			D3D12_ROOT_PARAMETER param;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			parameters.push_back(param);
		}

		if (HShader && HShader->constantBufferId)
		{
			HShader->constantBufferSlot = numCB;
			numCB++;

			D3D12_ROOT_PARAMETER param;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;

			parameters.push_back(param);
		}
		else if (!HShader)
			rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

		if (DShader && DShader->constantBufferId)
		{
			DShader->constantBufferSlot = numCB;
			numCB++;

			D3D12_ROOT_PARAMETER param;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;

			parameters.push_back(param);
		}
		else if (!DShader)
			rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

		if (GShader && GShader->constantBufferId)
		{
			GShader->constantBufferSlot = numCB;
			numCB++;

			D3D12_ROOT_PARAMETER param;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;

			parameters.push_back(param);
		}
		else if (!GShader)
			rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		if (PShader && PShader->constantBufferId)
		{
			PShader->constantBufferSlot = numCB;
			numCB++;

			D3D12_ROOT_PARAMETER param;
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			parameters.push_back(param);
		}
		else if (!PShader)
			rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
	}

	else
	{
		SComputeShaderHandle*		CShader = (SComputeShaderHandle *)g_CurrentEngineState->m_ActiveComputeShader;

		m_nShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
		rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
		rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
		rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
		rootDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

		if (CShader && CShader->constantBufferId)
		{
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.RegisterSpace = 0;
			param.Descriptor.ShaderRegister = 0;

			parameters.push_back(param);
		}
	}

	m_nIndexSRV = m_nIndexCBV = m_nIndexUAV = m_nIndexSamplers = -1;

	D3D12_DESCRIPTOR_RANGE srvRange = {};
	D3D12_DESCRIPTOR_RANGE cbvRange = {};
	D3D12_DESCRIPTOR_RANGE uavRange = {};
	D3D12_DESCRIPTOR_RANGE samplerRange = {};

	if (m_NumSRV > 0)
	{
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.BaseShaderRegister = 0;
		srvRange.RegisterSpace = 0;
		srvRange.NumDescriptors = m_NumSRV;
		srvRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &srvRange;
		param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)m_nShaderVisibility;

		m_nIndexSRV = static_cast<unsigned int>(parameters.size());

		parameters.push_back(param);
	}

	if (m_NumCBV > 0)
	{
		cbvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRange.BaseShaderRegister = 0;
		cbvRange.RegisterSpace = 0;
		cbvRange.NumDescriptors = m_NumCBV;
		cbvRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &cbvRange;
		param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)m_nShaderVisibility;

		m_nIndexCBV = static_cast<unsigned int>(parameters.size());

		parameters.push_back(param);
	}

	if (m_NumUAV > 0)
	{
		uavRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		uavRange.BaseShaderRegister = m_eType == e_GraphicsPipeline ? m_gfxDesc.NumRenderTargets : 0;
		uavRange.RegisterSpace = 0;
		uavRange.NumDescriptors = m_NumUAV;
		uavRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &uavRange;
		param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)m_nShaderVisibility;

		m_nIndexUAV = static_cast<unsigned int>(parameters.size());

		parameters.push_back(param);
	}

	if (m_NumSamplers > 0)
	{
		samplerRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		samplerRange.BaseShaderRegister = 0;
		samplerRange.RegisterSpace = 0;
		samplerRange.NumDescriptors = m_NumSamplers;
		samplerRange.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		param.DescriptorTable.NumDescriptorRanges = 1;
		param.DescriptorTable.pDescriptorRanges = &samplerRange;
		param.ShaderVisibility = (D3D12_SHADER_VISIBILITY)m_nShaderVisibility;

		m_nIndexSamplers = static_cast<unsigned int>(parameters.size());

		parameters.push_back(param);
	}

	rootDesc.NumParameters = static_cast<UINT>(parameters.size());
	rootDesc.pParameters = parameters.data();

	rootDesc.NumStaticSamplers = static_cast<UINT>(m_nSamplerIDs.size()) - m_NumSamplers;
	rootDesc.pStaticSamplers = m_nSamplerIDs.data() + m_NumSamplers;

#ifdef EKOPLF_X2_DEFINE
	CCommandListManager::LockCommandLists();
#endif

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &error);
	ASSERT(hr == S_OK);

	hr = CDisplay::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS((ID3D12RootSignature**)(&m_pRootSignature)));
	ASSERT(hr == S_OK);

	if (m_eType == e_GraphicsPipeline)
	{
		m_gfxDesc.pRootSignature = (ID3D12RootSignature*)m_pRootSignature;

		hr = CDisplay::GetDevice()->CreateGraphicsPipelineState(&m_gfxDesc, __uuidof(ID3D12PipelineState), &m_pPipelineState);
	}

	else
	{
		m_cmpDesc.pRootSignature = (ID3D12RootSignature*)m_pRootSignature;

		hr = CDisplay::GetDevice()->CreateComputePipelineState(&m_cmpDesc, __uuidof(ID3D12PipelineState), &m_pPipelineState);
	}

#ifdef EKOPLF_X2_DEFINE
	CCommandListManager::ReleaseCommandLists();
#endif

	ASSERT(hr == S_OK);

	return hr == S_OK;
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

		if (pipeline->m_eType == e_GraphicsPipeline)
		{
			if (memcmp(&pipeline->m_gfxDesc, &currentPipeline->m_gfxDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC)) == 0)
			{
				bFound = true;
				break;
			}
		}

		else
		{
			if (memcmp(&pipeline->m_cmpDesc, &currentPipeline->m_cmpDesc, sizeof(D3D12_COMPUTE_PIPELINE_STATE_DESC)) == 0)
			{
				bFound = true;
				break;
			}
		}
	}

	return bFound ? currentPipeline : nullptr;
}
