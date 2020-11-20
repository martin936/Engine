#ifndef __RENDER_PASS_H__
#define __RENDER_PASS_H__

#include <vector>
#include "PipelineManager.h"
#include "Shaders.h"
#include "Engine/Renderer/Textures/TextureInterface.h"
#include <tuple>


#define NUMS_ARGS(...)		(std::tuple_size<decltype(std::make_tuple(__VA_ARGS__))>::value)	
#define SUBPASS_MASK(...)	CRenderPass::GetSubPassMask(NUMS_ARGS(__VA_ARGS__), __VA_ARGS__)


struct SRenderPassTask
{
	class CRenderPass*	m_pRenderPass;
	unsigned int	m_nSubPassMask;
};


class CRenderPass
{
	friend class CFrameBlueprint;
	friend class CPipelineManager;
	friend class CDeviceManager;
public:

	enum EResourceAccessType
	{
		e_Common = 0,
		e_PixelShaderResource = 0x1,
		e_NonPixelShaderResource = 0x2,
		e_ShaderResource = 0x4,
		e_RenderTarget = 0x8,
		e_DepthStencil_Write = 0x10,
		e_DepthStencil_Read = 0x20,
		e_UnorderedAccess = 0x40,
		e_CopyDest = 0x80,
		e_CopySrc = 0x100
	};

	enum EResourceType
	{
		e_Texture,
		e_Buffer
	};

	CRenderPass(const char* pcName, CPipelineManager::EPipelineType eType, bool bLoading = false);
	~CRenderPass();

	static void Reset();

	static void CopyFrom(const char* pcName);

	static bool BeginGraphics(const char* pcName, bool bLoading = false);
	static bool BeginCompute(const char* pcName);
	static void End();

	static bool BeginGraphicsSubPass();
	static bool BeginComputeSubPass();
	static void EndSubPass();

	static void SetEmptyPipeline();

	static unsigned int GetSubPassMask(int numSubPasses, ...);

	static void ChangeSubPassResourceToRead(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, unsigned int nShaderStages = CShader::EShaderType::e_FragmentShader);
	static void ChangeSubPassResourceToRead(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, unsigned int nShaderStages = CShader::EShaderType::e_FragmentShader);

	static void ChangeSubPassResourceToWrite(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType = e_RenderTarget);
	static void ChangeSubPassResourceToWrite(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, EResourceAccessType eType = e_RenderTarget);
	static void ChangeSubPassDepthStencil(const char* pcName, unsigned int nSubPassID, unsigned int nResourceID);

	static unsigned int GetSubPassReadResourceID(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nShaderStages = CShader::EShaderType::e_FragmentShader);
	static unsigned int GetSubPassWrittenResourceID(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, EResourceAccessType eType = e_RenderTarget);

	static void ChangeResourceToRead(const char* pcName, unsigned int nSlot, unsigned int nResourceID, unsigned int nShaderStages = CShader::EShaderType::e_FragmentShader);
	static void ChangeResourceToRead(const char* pcName, unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, unsigned int nShaderStages = CShader::EShaderType::e_FragmentShader);

	static void ChangeResourceToWrite(const char* pcName, unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType = e_RenderTarget);
	static void ChangeResourceToWrite(const char* pcName, unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, EResourceAccessType eType = e_RenderTarget);

	static void ChangeDepthStencil(const char* pcName, unsigned int nResourceID);

	static unsigned int GetReadResourceID(const char* pcName, unsigned int nSlot, unsigned int nShaderStage);
	static unsigned int GetReadResourceID(unsigned int nSlot, unsigned int nShaderStage);

	static unsigned int GetWrittenResourceID(const char* pcName, unsigned int nSlot, EResourceAccessType eType);
	static unsigned int GetWrittenResourceID(unsigned int nSlot, EResourceAccessType eType);

	static void BindResourceToRead(unsigned int nSlot, unsigned int nResourceID, CShader::EShaderType nShaderStages = CShader::EShaderType::e_FragmentShader, EResourceType eType = e_Texture);
	static void BindResourceToRead(unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, CShader::EShaderType nShaderStages = CShader::EShaderType::e_FragmentShader, EResourceType eType = e_Texture);

	static void BindResourceToWrite(unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType = e_RenderTarget, EResourceType eResourceType = e_Texture);
	static void BindResourceToWrite(unsigned int nSlot, unsigned int nResourceID, int nSlice = -1, int nLevel = -1, EResourceAccessType eType = e_RenderTarget, EResourceType eResourceType = e_Texture);

	static void BindDepthStencil(unsigned int nResourceID, int nSlice = -1, int nLevel = -1);

	inline static void EnableMemoryBarriers()
	{
		CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

		pCurrent->m_bEnableMemoryBarriers = true;
	}

	inline static bool BindProgram(const char* cComputeShaderPath)
	{
		return ms_pCurrentPipeline->BindProgram(cComputeShaderPath);
	}

	inline static bool BindProgram(const char* cVertexShaderPath, const char* cFragmentShaderPath)
	{
		return ms_pCurrentPipeline->BindProgram(cVertexShaderPath, cFragmentShaderPath);
	}

	inline static bool BindProgram(const char* cVertexShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath)
	{
		return ms_pCurrentPipeline->BindProgram(cVertexShaderPath, cGeometryShaderPath, cFragmentShaderPath);
	}

	inline static bool BindProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cFragmentShaderPath)
	{
		return ms_pCurrentPipeline->BindProgram(cVertexShaderPath, cHullShaderPath, cDomainShaderPath, cFragmentShaderPath);
	}

	inline static bool BindProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath)
	{
		return ms_pCurrentPipeline->BindProgram(cVertexShaderPath, cHullShaderPath, cDomainShaderPath, cGeometryShaderPath, cFragmentShaderPath);
	}

	inline static bool SetBlendState(bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask = 0xf, ELogicOp logicOp = ELogicOp::e_LogicOp_None)
	{
		return ms_pCurrentPipeline->SetBlendState(blendEnable, logicOpEnable, srcBlend, dstBlend, colorOp, srcBlendAlpha, dstBlendAlpha, alphaOp, writeMask, logicOp);
	}

	inline static bool SetRenderTargetBlendState(int renderTargetSlot, bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask = 0xff, ELogicOp logicOp = ELogicOp::e_LogicOp_None)
	{
		return ms_pCurrentPipeline->SetRenderTargetBlendState(renderTargetSlot, blendEnable, logicOpEnable, srcBlend, dstBlend, colorOp, srcBlendAlpha, dstBlendAlpha, alphaOp, writeMask, logicOp);
	}

	inline static void DisableBlend(unsigned char writeMask = 0xf)
	{
		ms_pCurrentPipeline->DisableBlend(writeMask);
	}

	inline static void DisableStencil()
	{
		ms_pCurrentPipeline->DisableStencil();
	}

	inline static void DisableDepthStencil()
	{
		ms_pCurrentPipeline->DisableDepthStencil();
	}

	inline static bool SetRasterizerState(ERasterFillMode fillMode, ERasterCullMode cullMode, bool conservativeRaster = false, bool enableMultisampling = false, bool depthClamp = false, float depthBias = 0.f, float slopeBias = 0.f)
	{
		return ms_pCurrentPipeline->SetRasterizerState(fillMode, cullMode, conservativeRaster, enableMultisampling, depthClamp, depthBias, slopeBias);
	}

	inline static bool SetShadingRate(EShadingRate rate)
	{
		return ms_pCurrentPipeline->SetShadingRate(rate);
	}

	inline static bool SetDepthState(bool depthEnable, ECmpFunc depthFunc, bool enableWrite = true)
	{
		return ms_pCurrentPipeline->SetDepthState(depthEnable, depthFunc, enableWrite);
	}

	inline static bool SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOp frontFail, EStencilOp frontDepthFail, EStencilOp frontPass, ECmpFunc frontFunc, \
		EStencilOp backFail, EStencilOp backDepthFail, EStencilOp backPass, ECmpFunc backFunc)
	{
		return ms_pCurrentPipeline->SetStencilState(stencilEnable, readMask, writeMask, frontFail, frontDepthFail, frontPass, frontFunc, backFail, backDepthFail, backPass, backFunc);
	}

	inline static bool SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOp Fail, EStencilOp depthFail, EStencilOp pass, ECmpFunc func)
	{
		return ms_pCurrentPipeline->SetStencilState(stencilEnable, readMask, writeMask, Fail, depthFail, pass, func);
	}

	inline static bool SetRenderTargetFormat(unsigned int nSlot, ETextureFormat format)
	{
		return ms_pCurrentPipeline->SetRenderTargetFormat(nSlot, format);
	}

	inline static bool SetDepthStencilFormat(ETextureFormat format)
	{
		return ms_pCurrentPipeline->SetDepthStencilFormat(format);
	}

	inline static bool SetPrimitiveTopology(ETopology eTopology)
	{
		return ms_pCurrentPipeline->SetPrimitiveTopology(eTopology);
	}

	inline static void BindSampler(unsigned int nSlot, unsigned int nSamplerID)
	{
		ms_pCurrentPipeline->BindSampler(nSlot, nSamplerID);
	}

	static void SetPipeline(unsigned int nPipelineID)
	{
		CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

		pCurrent->m_nPipelineStateID = nPipelineID;
	}

	static void SetNumSamplers(int nSlot, int numSamplers)
	{
		ms_pCurrentPipeline->SetNumSamplers(nSlot, numSamplers);
	}

	static void SetNumBuffers(int nSlot, int numBuffers)
	{
		ms_pCurrentPipeline->SetNumBuffers(nSlot, numBuffers);
	}

	static void SetNumRWBuffers(int nSlot, int numRWBuffers)
	{
		ms_pCurrentPipeline->SetNumRWBuffers(nSlot, numRWBuffers);
	}

	static void SetNumTextures(int nSlot, int numTextures)
	{
		ms_pCurrentPipeline->SetNumTextures(nSlot, numTextures);
	}

	static void SetNumRWTextures(int nSlot, int numRWTextures)
	{
		ms_pCurrentPipeline->SetNumRWTextures(nSlot, numRWTextures);
	}

	static unsigned int GetCurrentPipeline()
	{
		//CACTUSASSERT(ms_pCurrent != nullptr && "Should be called between Begin and End");

		if (ms_pCurrentSubPass != nullptr)
			return ms_pCurrentSubPass->m_nPipelineStateID;

		return ms_pCurrent->m_nPipelineStateID;
	}

	inline unsigned int GetPipeline(int subpass = 0)
	{
		if (m_SubPasses.size() > subpass)
			return m_SubPasses[subpass]->GetPipeline();

		return m_nPipelineStateID;
	}

	static void SetEntryPoint(void(*pEntryPoint)())
	{
		CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

		pCurrent->m_pEntryPoint = pEntryPoint;
	}

	void Run(unsigned int nCommandListID, void* pData, unsigned int subPassMask = 0xffffffff);


	static CRenderPass* GetRenderPass(const char* pcName)
	{
		unsigned int nID = GetRenderPassID(pcName);

		ASSERT(nID != INVALIDHANDLE);

		return ms_pRenderPasses[nID];
	}

	static SRenderPassTask GetRenderPassTask(const char* pcName, unsigned int subPassMask = 0xffffffff)
	{
		return { GetRenderPass(pcName), subPassMask };
	}

	static CRenderPass* GetRenderPass(unsigned int nID)
	{
		return ms_pRenderPasses[nID];
	}

	static CRenderPass* GetLoadingRenderPass(const char* pcName)
	{
		return ms_pLoadingRenderPasses[GetLoadingRenderPassID(pcName)];
	}

	static SRenderPassTask GetLoadingRenderPassTask(const char* pcName, unsigned int subPassMask = 0xffffffff)
	{
		return { ms_pLoadingRenderPasses[GetLoadingRenderPassID(pcName)], subPassMask };
	}

	static unsigned int GetRenderPassID(const char* pcName);
	static unsigned int GetLoadingRenderPassID(const char* pcName);

	void* GetDeviceRenderPass() const
	{
		return m_pDeviceRenderPass;
	}

	void* GetDeviceFramebuffer(int frame = 0) const
	{
		return m_pFramebuffer[frame];
	}

private:

	void CreateFramebuffer();
	void BeginRenderPass();
	void EndRenderPass();

	struct SReadOnlyResource
	{
		unsigned int			m_nSlot;
		unsigned int			m_nResourceID;
		EResourceType			m_eType;
		int						m_nSlice;
		int						m_nLevel;
		CShader::EShaderType	m_nShaderStages;
	};

	struct SWriteResource
	{
		unsigned int		m_nSlot;
		unsigned int		m_nResourceID;
		EResourceType		m_eResourceType;
		int					m_nSlice;
		int					m_nLevel;
		EResourceAccessType	m_eType;
	};

	void(*m_pEntryPoint)();

	std::vector<CRenderPass*>			m_SubPasses;
	CRenderPass*						m_pParentPass;

	std::vector<void*>					m_pFramebuffer;
	void*								m_pDeviceRenderPass;

	std::vector<SReadOnlyResource>		m_nReadResourceID;
	std::vector<SWriteResource>			m_nWritenResourceID;
	unsigned int						m_nDepthStencilID;
	unsigned int						m_nDepthStencilSlice;
	unsigned int						m_nDepthStencilLevel;
	unsigned int						m_nPipelineStateID;

	bool								m_bLoadingPass;
	bool								m_bEnableMemoryBarriers;
	bool								m_bIsGraphicsRenderPassRunning;
	unsigned int						m_nID;
	unsigned int						m_nSortedID;
	char								m_cName[512];

	static CRenderPass*					ms_pCurrentSubPass;
	static CRenderPass*					ms_pCurrent;
	static CPipelineManager::SPipeline* ms_pCurrentPipeline;
	static std::vector<CRenderPass*>	ms_pRenderPasses;
	static std::vector<CRenderPass*>	ms_pSortedRenderPasses;
	static std::vector<CRenderPass*>	ms_pLoadingRenderPasses;
};


class CFrameBlueprint
{
	friend CRenderPass;
public:

	enum EBarrierFlags
	{
		e_Immediate,
		e_Split_Begin,
		e_Split_End
	};

	static void Init();
	static void Terminate();

	static void PrepareForRenderPass(CRenderPass* renderPass);
	static void FinishRenderPass(CRenderPass* renderPass);

	static void EndFrame();

	static void BakeFrame();

	static bool IsCurrentRenderPass(const char* pcName);

	static unsigned int GetCurrentSubPass();

	static bool IsSorting() { return ms_bIsSorting; }

	static void PrepareForSort();

	static void SortRenderPasses();

	static void TransitionResourcesToFirstState(unsigned int renderpass);

	static void SetNextRenderPass(SRenderPassTask renderpass);

	static void TransitionBarrier(unsigned int nResourceID, unsigned int eCurrentState, unsigned int eNextState, EBarrierFlags eFlags, CRenderPass::EResourceType eType = CRenderPass::e_Texture);
	static void UnorderedAccessBarrier(unsigned int nResourceID, EBarrierFlags eFlags, CRenderPass::EResourceType eType = CRenderPass::e_Texture);
	static void InsertMemoryBarrier();
	static void FlushBarriers();

	static CRenderPass*	GetRunningRenderPass()
	{
		return ms_pCurrentRenderPass;
	}

private:

	enum EEventType
	{
		e_PipelineChange,
		e_Barrier
	};

	enum EQueueID
	{
		e_GraphicsQueue,
		e_AsyncComputeQueue,
		e_CopyQueue
	};

	enum EBarrierType
	{
		e_Barrier_ResourceTransition,
		e_Barrier_Aliasing,
		e_Barrier_UAV,
		e_Barrier_Memory
	};

	struct SResourceBarrier
	{
		unsigned int m_nRenderPassID;
		bool		 m_bExecuteBeforeDrawCall;

		EBarrierType				m_eType;
		EBarrierFlags				m_eFlags;
		CRenderPass::EResourceType	m_eResourceType;

		union
		{
			struct
			{
				unsigned int m_nResourceID;
				int m_nLevel;
				int m_nSlice;
				unsigned int m_nCurrentState;
				unsigned int m_nNextState;
				unsigned int m_nSrcAccess;
				unsigned int m_nDstAccess;
				unsigned int m_nBeforeStage;
				unsigned int m_nAfterStage;
			} m_ResourceTransition;

			struct
			{
				unsigned int m_nPreviousResourceID;
				unsigned int m_nNextResourceID;
			} m_Aliasing;

			struct
			{
				unsigned int m_nResourceID;
			} m_UAV;
		};
	};

	struct SResourceUsage
	{
		unsigned int				m_nResourceID;
		int							m_nLevel;
		int							m_nSlice;
		CRenderPass::EResourceType	m_eType;

		std::vector<unsigned int>	m_nRenderPassID;
		std::vector<CRenderPass::EResourceAccessType> m_nUsage;
	};

	static bool								ms_bIsSorting;
	static unsigned int						ms_nNextPassSortedID;

	static std::vector<std::vector<SResourceBarrier>>	ms_EventList;
	static std::vector<SResourceUsage>					ms_ResourceUsage;

	static std::vector<std::vector<SResourceBarrier>>	ms_BarrierCache;

	static std::vector<SResourceBarrier>	ms_TransitionsToFirstState;

	static thread_local CRenderPass*		ms_pCurrentRenderPass;

	static unsigned int GetResourceIndex(unsigned int nResourceID, CRenderPass::EResourceType eType);

	static void ExecuteBarrier(unsigned int nRenderPassID, unsigned int nEventID);
	static void FlushBarriers(unsigned int nRenderPassID);
};


#endif
