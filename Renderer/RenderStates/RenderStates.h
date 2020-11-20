#ifndef __RENDER_STATES_H__
#define __RENDER_STATES_H__

#include "Engine/Engine.h"

/*enum EWriteMask
{
	e_Red = 1,
	e_Green = 2,
	e_Blue = 4,
	e_Alpha = 8,
	e_Depth = 16,
	e_Stencil = 32
};

enum ERasterizerState
{
	e_Wireframe,
	e_Solid
};

enum EBlendState
{
	e_Opaque,
	e_AlphaBlend,
	e_Additive,
	e_Opaque_AdditiveAlpha,
	e_ColInv_AdditiveAlpha
};

enum ECullMode
{
	e_CullNone,
	e_CullBackCW,
	e_CullBackCCW
};


enum EDepthFunc
{
	e_Zero,
	e_LessEqual,
	e_Less,
	e_GreaterEqual,
	e_Greater,
	e_Equal
};


enum EStencilFunc
{
	e_StencilFunc_Always,
	e_StencilFunc_Never,
	e_StencilFunc_Less,
	e_StencilFunc_LessEqual,
	e_StencilFunc_Greater,
	e_StencilFunc_GreaterEqual,
	e_StencilFunc_Equal,
	e_StencilFunc_NotEqual
};


enum EStencilOp
{
	e_StencilOp_Keep,
	e_StencilOp_Zero,
	e_StencilOp_Replace,
	e_StencilOp_Incr_Sat,
	e_StencilOp_Incr_Warp,
	e_StencilOp_Decr_Sat,
	e_StencilOp_Decr_Warp,
	e_StencilOp_Invert
};
*/
/*
class CRenderStates
{
public:

	static void Init();

	static void SetWriteMask(unsigned int nFlags);
	static void SetRasterizerState(ERasterizerState eState);
	static void SetBlendingState(EBlendState eState);
	static void SetIndependentBlendingState(EBlendState eState, int nSlot);
	static void SetCullMode(ECullMode eState);
	static void SetDepthStencil(EDepthFunc eDepth, bool bStencil);

	static void SetStencil(EStencilOp stencilPass, EStencilOp stencilFail, EStencilOp depthFail, EStencilFunc stencilFunc, unsigned char writeMask, unsigned char readMask, unsigned char ref);

private:

	static unsigned int		ms_nWriteMask;
	static ERasterizerState	ms_eRasterState;
	static EBlendState		ms_eBlendState;
	static EBlendState		ms_eMRTBlendState[8];
	static bool				ms_bBlendEnabled;
	static bool				ms_bMRTBlendEnabled[8];
	static bool				ms_bShouldUpdateBlend[8];
	static bool				ms_bSeparateBlending;
	static ECullMode		ms_eCullMode;
	static bool				ms_bCullingEnabled;
	static EDepthFunc		ms_eDepthFunc;
	static bool				ms_bDepthTest;
	static bool				ms_bStencilTest;

	static EStencilOp		ms_eStencilPassOp;
	static EStencilOp		ms_eStencilFailOp;
	static EStencilOp		ms_eDepthFailOp;
	static EStencilFunc		ms_eStencilFunc;

	static unsigned char	ms_nStencilRef;
};*/

#endif
