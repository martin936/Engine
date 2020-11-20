#ifndef __PIPELINE_MANAGER_DX12_H__
#define __PIPELINE_MANAGER_DX12_H__


#include <vector>
#include "Shaders.h"
#include "Engine/Renderer/Textures/TextureInterface.h"
#include "Engine/Misc/Mutex.h"


enum EBlendOp
{
	e_BlendOp_Add = 0,
	e_BlendOp_Sub,
	e_BlendOp_RevSub,
	e_BlendOp_Min,
	e_BlendOp_Max
};

enum EBlendFunc
{
	e_BlendFunc_Zero = 0,
	e_BlendFunc_One,
	e_BlendFunc_SrcCol,
	e_BlendFunc_InvSrcCol,
	e_BlendFunc_SrcAlpha,
	e_BlendFunc_InvSrcAlpha,
	e_BlendFunc_DstAlpha,
	e_BlendFunc_InvDstAlpha,
	e_BlendFunc_DstCol,
	e_BlendFunc_InvDstCol,
	e_BlendFunc_SrcAlpha_Sat,
	e_BlendFunc_BlendFactor,
	e_BlendFunc_InvBlendFactor,
	e_BlendFunc_Src1Col,
	e_BlendFunc_InvSrc1Col,
	e_BlendFunc_Src1Alpha,
	e_BlendFunc_InvSrc1Alpha
};

enum ELogicOp
{
	e_LogicOp_Clear = 0,
	e_LogicOp_Set,
	e_LogicOp_Copy,
	e_LogicOp_InvCopy,
	e_LogicOp_None,
	e_LogicOp_Inv,
	e_LogicOp_And,
	e_LogicOp_Nand,
	e_LogicOp_Or,
	e_LogicOp_Nor,
	e_LogicOp_Xor,
	e_LogicOp_Equiv,
	e_LogicOp_And_Rev,
	e_LogicOp_And_Inv,
	e_LogicOp_Or_Rev,
	e_LogicOp_Or_Inv
};

enum ERasterFillMode
{
	e_FillMode_WireFrame = 0,
	e_FillMode_Solid
};

enum ERasterCullMode
{
	e_CullMode_None = 0,
	e_CullMode_CW,
	e_CullMode_CCW
};

enum ECmpFunc
{
	e_CmpFunc_Never = 0,
	e_CmpFunc_Less,
	e_CmpFunc_Equal,
	e_CmpFunc_LEqual,
	e_CmpFunc_Greater,
	e_CmpFunc_NotEqual,
	e_CmpFunc_GEqual,
	e_CmpFunc_Always
};

enum EStencilOp
{
	e_StencilOp_Keep = 0,
	e_StencilOp_Zero,
	e_StencilOp_Replace,
	e_StencilOp_IncrSat,
	e_StencilOp_DecrSat,
	e_StencilOp_Inv,
	e_StencilOp_Incr,
	e_StencilOp_Decr
};


enum ETopology
{
	e_Topology_Undefined = 0,
	e_Topology_PointList,
	e_Topology_LineList,
	e_Topology_LineStrip,
	e_Topology_TriangleList,
	e_Topology_TriangleFan,
	e_Topology_TriangleStrip,
	e_Topology_Patch,

	e_Topology_Point = e_Topology_PointList,
	e_Topology_Line = e_Topology_LineList,
	e_Topology_Triangle = e_Topology_TriangleList,
};

enum EShadingRate
{
	e_PerPixel,
	e_PerSample
};


class CPipelineManager
{
public:

	static void Init();
	static void Terminate();

	enum EPipelineType
	{
		e_InvalidPipeline = -1,
		e_GraphicsPipeline,
		e_ComputePipeline,

	};

	struct SPipeline
	{
		EPipelineType m_eType;

		struct SBlendState
		{
			bool		m_bBlendEnabled;
			bool		m_bLogicOpEnabled;

			EBlendFunc	m_SrcFunc;
			EBlendFunc	m_DstFunc;
			EBlendOp	m_BlendOp;

			EBlendFunc	m_SrcAlphaFunc;
			EBlendFunc	m_DstAlphaFunc;
			EBlendOp	m_AlphaBlendOp;

			ELogicOp	m_LogicOp;

			uint8_t		m_WriteMask;

			SBlendState()
			{
				m_bBlendEnabled = false; 
				m_bLogicOpEnabled = false;
				m_SrcFunc = e_BlendFunc_One;		m_DstFunc = e_BlendFunc_Zero;		m_BlendOp = e_BlendOp_Add;
				m_SrcAlphaFunc = e_BlendFunc_One;	m_DstAlphaFunc = e_BlendFunc_Zero;	m_AlphaBlendOp = e_BlendOp_Add;
				m_LogicOp = e_LogicOp_None; 
				m_WriteMask = 0xf;
			}
		};

		struct SRasterState
		{
			bool			m_bConservativeRaster;
			bool			m_bMultiSampling;
			bool			m_bDepthClamp;

			float			m_DepthBias;
			float			m_SlopeBias;

			ERasterFillMode	m_FillMode;
			ERasterCullMode	m_CullMode;

			int				m_SampleCount;
			int				m_SampleQuality;

			SRasterState()
			{
				m_bConservativeRaster = false;
				m_bMultiSampling = false;
				m_bDepthClamp = false;
				m_DepthBias = 0.f;
				m_SlopeBias = 0.f;
				m_FillMode = e_FillMode_Solid;
				m_CullMode = e_CullMode_None;
				m_SampleCount = 1;
				m_SampleQuality = 0;
			}
		};

		struct SStencilState
		{
			EStencilOp	m_FailOp;
			EStencilOp	m_DepthFailOp;
			EStencilOp	m_PassOp;
			ECmpFunc	m_Func;

			SStencilState()
			{
				m_FailOp		= e_StencilOp_Keep;
				m_DepthFailOp	= e_StencilOp_Keep;
				m_PassOp		= e_StencilOp_Keep;
				m_Func			= e_CmpFunc_Always;
			}
		};

		struct SDepthStencilState
		{
			bool			m_bStencilEnable;
			uint8_t			m_WriteMask;
			uint8_t			m_ReadMask;
			SStencilState	m_FrontStencil;
			SStencilState	m_BackStencil;

			bool			m_bDepthEnable;
			bool			m_bEnableWrite;
			ECmpFunc		m_DepthFunc;

			SDepthStencilState()
			{
				m_bStencilEnable = false;
				m_WriteMask		 = 0xff;
				m_ReadMask		 = 0xff;
				
				m_bDepthEnable	 = false;
				m_bEnableWrite	 = false;
				m_DepthFunc		 = e_CmpFunc_Always;
			}
		};

		struct SPipelineState
		{
			bool				m_bIndependentBlendEnabled;
			SBlendState			m_BlendStates[8];
			SRasterState		m_RasterState;
			SDepthStencilState	m_DepthStencilState;
			ETopology			m_Topology;
			EShadingRate		m_ShadingRate;
			unsigned int		m_NumRenderTargets;
			ETextureFormat		m_RenderTargetFormat[8];
			ETextureFormat		m_DepthStencilFormat;

			SPipelineState()
			{
				m_bIndependentBlendEnabled	= false;
				m_Topology					= ETopology::e_Topology_TriangleList;
				m_ShadingRate				= EShadingRate::e_PerPixel;
				m_NumRenderTargets			= 0;
				m_DepthStencilFormat		= ETextureFormat::e_UNKOWN;
			}
		};

		bool m_bRenderTargetBound[8];

		SPipelineState						m_State;
		static thread_local SPipelineState	ms_CurrentState;
		static thread_local bool			ms_bResetState;
		static thread_local bool			ms_bReBindShaders;
		static thread_local bool			ms_bUpdateStencilMask;
		static thread_local unsigned int	ms_nStencilRef;

		ProgramHandle	m_nProgramID;

		struct SBindingDescriptor
		{
			int slot;
			int count;
		};

		unsigned int	m_NumConstantBuffers;
		std::vector<SBindingDescriptor>	m_NumTextures;
		std::vector<SBindingDescriptor>	m_NumBuffers;
		std::vector<SBindingDescriptor>	m_NumRwTextures;
		std::vector<SBindingDescriptor>	m_NumRwBuffers;
		std::vector<SBindingDescriptor>	m_NumSamplers;
		unsigned int	m_nShaderVisibility;

		std::vector<unsigned int> m_nSamplerIDs;

		void*			m_pPipelineState;
		void*			m_pRootSignature;
		void*			m_pDescriptorLayout;

		bool			m_bAliasedPipeline;

		std::vector<void*>			m_pDescriptorSets;
		std::vector<unsigned int>	m_nDynamicOffsets;
		std::vector<unsigned int>	m_nDynamicBufferBinding;

		SPipeline(EPipelineType eType);
		~SPipeline();

		void CopyFrom(SPipeline* pSrc);

		unsigned int GetNumRenderTargets();

		bool BindProgram(const char* cComputeShaderPath);
		bool BindProgram(const char* cVertexShaderPath, const char* cFragmentShaderPath);
		bool BindProgram(const char* cVertexShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath);
		bool BindProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cFragmentShaderPath);
		bool BindProgram(const char* cVertexShaderPath, const char* cHullShaderPath, const char* cDomainShaderPath, const char* cGeometryShaderPath, const char* cFragmentShaderPath);

		void SetNumSamplers(int nSlot, int numSamplers)
		{
			m_NumSamplers.push_back({ nSlot, numSamplers });
		}

		void SetNumBuffers(int nSlot, int numBuffers)
		{
			m_NumBuffers.push_back({ nSlot, numBuffers });
		}

		void SetNumRWBuffers(int nSlot, int numRWBuffers)
		{
			m_NumRwBuffers.push_back({ nSlot, numRWBuffers });
		}

		void SetNumTextures(int nSlot, int numTextures)
		{
			m_NumTextures.push_back({ nSlot, numTextures });
		}

		void SetNumRWTextures(int nSlot, int numRWTextures)
		{
			m_NumRwTextures.push_back({ nSlot, numRWTextures });
		}

		void DisableBlend(unsigned char writeMask = 0xf);
		void DisableStencil();
		void DisableDepthStencil();

		bool SetBlendState(bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask = 0xf, ELogicOp logicOp = ELogicOp::e_LogicOp_None);
		bool SetRenderTargetBlendState(int renderTargetSlot, bool blendEnable, bool logicOpEnable, EBlendFunc srcBlend, EBlendFunc dstBlend, EBlendOp colorOp, EBlendFunc srcBlendAlpha, EBlendFunc dstBlendAlpha, EBlendOp alphaOp, int writeMask = 0xff, ELogicOp logicOp = ELogicOp::e_LogicOp_None);

		bool SetRasterizerState(ERasterFillMode fillMode, ERasterCullMode cullMode, bool conservativeRaster = false, bool enableMultisampling = false, bool depthClamp = false, float depthBias = 0.f, float slopeBias = 0.f);
		bool SetDepthState(bool depthEnable, ECmpFunc depthFunc, bool enableWrite = true);

		bool SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOp frontFail, EStencilOp frontDepthFail, EStencilOp frontPass, ECmpFunc frontFunc, \
			EStencilOp backFail, EStencilOp backDepthFail, EStencilOp backPass, ECmpFunc backFunc);

		bool SetStencilState(bool stencilEnable, int readMask, int writeMask, EStencilOp Fail, EStencilOp depthFail, EStencilOp pass, ECmpFunc func);

		bool SetRenderTargetFormat(unsigned int nSlot, ETextureFormat format);
		bool SetSampleDesc(int nSampleCount, int nSampleQuality);
		bool SetShadingRate(EShadingRate eRate);
		bool SetDepthStencilFormat(ETextureFormat format);

		bool SetPrimitiveTopology(ETopology eTopology);

		void CreateRootSignature(unsigned int nNumCBV, unsigned int nNumSRV, unsigned int nNumUAV, unsigned int nNumSamplers = 0, unsigned int nShaderVisibility = CShader::EShaderType::e_FragmentShader);

		void BindSampler(unsigned int nSlot, unsigned int nSamplerID);

		bool Create();
	};


	static unsigned int NewPipeline(EPipelineType eType);

	static void DelayPipelineCreation() { ms_pLock->Take(); }
	static void ResumePipelineCreation() { ms_pLock->Release(); }

	static SPipeline* GetPipelineState(unsigned int nID)
	{
		if (nID == INVALIDHANDLE)
			return nullptr;

		ASSERT(nID > 0 && nID <= ms_pPipelines.size());

		return ms_pPipelines[nID - 1];
	}

	static void* GetPipelineRootSignature(unsigned int nID)
	{
		ASSERT(nID > 0 && nID <= ms_pPipelines.size());

		return ms_pPipelines[nID - 1]->m_pRootSignature;
	}

	static EPipelineType GetPipelineType(unsigned int nID)
	{
		ASSERT(nID > 0 && nID <= ms_pPipelines.size());

		return ms_pPipelines[nID - 1]->m_eType;
	}

	static void BindPipeline(unsigned int nCommandListID, unsigned int nPipelineStateID);
	static void BindShaders(unsigned int nPipelineStateID);

	static void SetStencilRef(unsigned int nRef);

	static SPipeline*	GetCurrentPipeline();

	static ProgramHandle	GetActiveProgram() { return ms_nActiveProgram; }

	static void			SetCurrentPipeline(unsigned int nID);

	static EPipelineType GetCurrentPipelineType();

private:

	thread_local static SPipeline*		ms_pCurrentPipeline;
	thread_local static ProgramHandle	ms_nActiveProgram;

	static void SetActiveProgram(ProgramHandle pid)
	{
		ms_nActiveProgram = pid;
	}

	static SPipeline* FindSimilarPipeline(SPipeline* pipeline);

	static std::vector<SPipeline*> ms_pPipelines;

	static CMutex* ms_pLock;
};


#endif
