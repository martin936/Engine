#include "Engine/Engine.h"
#include "RenderStates.h"

unsigned int		CRenderStates::ms_nWriteMask			= 31;
ERasterizerState	CRenderStates::ms_eRasterState			= e_Solid;
EBlendState			CRenderStates::ms_eBlendState			= e_Opaque;
EBlendState			CRenderStates::ms_eMRTBlendState[8]		= { e_Opaque, e_Opaque, e_Opaque, e_Opaque , e_Opaque, e_Opaque , e_Opaque, e_Opaque };
ECullMode			CRenderStates::ms_eCullMode				= e_CullBackCW;
bool				CRenderStates::ms_bBlendEnabled			= false;
bool				CRenderStates::ms_bMRTBlendEnabled[8]	= { false, false, false, false, false, false, false, false };
bool				CRenderStates::ms_bShouldUpdateBlend[8] = { true, true, true, true, true, true, true, true };
bool				CRenderStates::ms_bSeparateBlending		= false;
bool				CRenderStates::ms_bCullingEnabled		= false;
bool				CRenderStates::ms_bDepthTest			= false;
EDepthFunc			CRenderStates::ms_eDepthFunc			= e_LessEqual;
bool				CRenderStates::ms_bStencilTest			= false;

EStencilOp			CRenderStates::ms_eStencilPassOp		= e_StencilOp_Keep;
EStencilOp			CRenderStates::ms_eStencilFailOp		= e_StencilOp_Keep;
EStencilOp			CRenderStates::ms_eDepthFailOp			= e_StencilOp_Keep;
EStencilFunc		CRenderStates::ms_eStencilFunc			= e_StencilFunc_Always;

unsigned char		CRenderStates::ms_nStencilRef = 0;


struct SStencilOpOGL
{
	EStencilOp	m_Op;
	GLenum		m_OGLOp;
};


SStencilOpOGL gs_StencilOpAssociation[] = 
{
	{e_StencilOp_Keep, GL_KEEP},
	{ e_StencilOp_Zero, GL_ZERO },
	{e_StencilOp_Replace, GL_REPLACE},
	{ e_StencilOp_Incr_Sat, GL_INCR },
	{ e_StencilOp_Incr_Warp, GL_INCR_WRAP },
	{ e_StencilOp_Decr_Sat, GL_DECR },
	{ e_StencilOp_Decr_Warp, GL_DECR_WRAP },
	{e_StencilOp_Invert, GL_INVERT}
};


struct SStencilFuncOGL
{
	EStencilFunc	m_Op;
	GLenum		m_OGLOp;
};


SStencilFuncOGL gs_StencilFuncAssociation[] =
{
	{ e_StencilFunc_Always, GL_ALWAYS },
	{ e_StencilFunc_Never, GL_NEVER },
	{ e_StencilFunc_Less, GL_LESS },
	{ e_StencilFunc_LessEqual, GL_LEQUAL },
	{ e_StencilFunc_Greater, GL_GREATER },
	{ e_StencilFunc_GreaterEqual, GL_GEQUAL },
	{ e_StencilFunc_Equal, GL_EQUAL },
	{ e_StencilFunc_NotEqual, GL_NOTEQUAL }
};


void CRenderStates::Init()
{
	glColorMask(ms_nWriteMask & e_Red, ms_nWriteMask & e_Green, ms_nWriteMask & e_Blue, ms_nWriteMask & e_Alpha);
	glDepthMask(ms_nWriteMask & e_Depth);

	if (ms_eRasterState == e_Solid)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else if (ms_eRasterState == e_Wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (ms_eBlendState == e_Opaque)
	{
		glDisable(GL_BLEND);
		ms_bBlendEnabled = false;
	}

	else if (ms_eBlendState == e_Additive)
	{
		glBlendFunc(GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		ms_bBlendEnabled = true;
	}

	else if (ms_eBlendState == e_AlphaBlend)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		ms_bBlendEnabled = true;
	}

	else if (ms_eBlendState == e_Opaque_AdditiveAlpha)
	{
		glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ONE);
		glEnable(GL_BLEND);
		ms_bBlendEnabled = true;
	}

	if (ms_eCullMode == e_CullNone)
	{
		glDisable(GL_CULL_FACE);
		ms_bCullingEnabled = false;
	}

	else if (ms_eCullMode == e_CullBackCW)
	{
		glCullFace(GL_FRONT);
		glEnable(GL_CULL_FACE);
		ms_bCullingEnabled = true;
	}

	else if (ms_eCullMode == e_CullBackCCW)
	{
		glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
		ms_bCullingEnabled = true;
	}

	glDepthFunc(GL_LEQUAL);

	if (ms_bDepthTest)
		glEnable(GL_DEPTH_TEST);

	else
		glDisable(GL_DEPTH_TEST);	

	if (ms_bStencilTest)
		glEnable(GL_STENCIL_TEST);

	else
		glDisable(GL_STENCIL_TEST);

	glStencilOp(gs_StencilOpAssociation[ms_eStencilFailOp].m_OGLOp, gs_StencilOpAssociation[ms_eDepthFailOp].m_OGLOp, gs_StencilOpAssociation[ms_eStencilPassOp].m_OGLOp);
	glStencilFunc(gs_StencilFuncAssociation[ms_eStencilFunc].m_OGLOp, ms_nStencilRef, 0xff);

	glBlendEquation(GL_FUNC_ADD);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}



void CRenderStates::SetWriteMask(unsigned int nFlags)
{
	if ((nFlags & (e_Red | e_Green | e_Blue | e_Alpha)) != (ms_nWriteMask & (e_Red | e_Green | e_Blue | e_Alpha)))
		glColorMask((nFlags & e_Red) ? GL_TRUE : GL_FALSE, (nFlags & e_Green) ? GL_TRUE : GL_FALSE, (nFlags & e_Blue) ? GL_TRUE : GL_FALSE, (nFlags & e_Alpha) ? GL_TRUE : GL_FALSE);

	if ((nFlags & e_Depth) != (ms_nWriteMask & e_Depth))
		glDepthMask((nFlags & e_Depth) ? GL_TRUE : GL_FALSE);

	ms_nWriteMask = nFlags;
}

void CRenderStates::SetRasterizerState(ERasterizerState eState)
{
	if (eState != ms_eRasterState)
	{
		if (eState == e_Solid)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		else if (eState == e_Wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		ms_eRasterState = eState;
	}
}


void CRenderStates::SetIndependentBlendingState(EBlendState eState, int nSlot)
{
	if (eState != ms_eMRTBlendState[nSlot] || ms_bShouldUpdateBlend[nSlot])
	{
		if (eState == e_Opaque && ms_bMRTBlendEnabled[nSlot])
		{
			glDisablei(GL_BLEND, nSlot);
			ms_bMRTBlendEnabled[nSlot] = false;
			ms_eMRTBlendState[nSlot] = e_Opaque;
		}

		else
		{
			if (eState == e_Additive)
			{
				glBlendFunci(nSlot, GL_ONE, GL_ONE);
				ms_eMRTBlendState[nSlot] = e_Additive;
			}

			else if (eState == e_AlphaBlend)
			{
				glBlendFunci(nSlot, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				ms_eMRTBlendState[nSlot] = e_AlphaBlend;
			}

			else if (eState == e_Opaque_AdditiveAlpha)
			{
				glBlendFuncSeparatei(nSlot, GL_ONE, GL_ZERO, GL_ONE, GL_ONE);
				ms_eMRTBlendState[nSlot] = e_Opaque_AdditiveAlpha;
			}

			else if (eState == e_ColInv_AdditiveAlpha)
			{
				glBlendFuncSeparatei(nSlot, GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE);
				ms_eMRTBlendState[nSlot] = e_ColInv_AdditiveAlpha;
			}

			if (!ms_bMRTBlendEnabled[nSlot] || ms_bShouldUpdateBlend[nSlot])
			{
				glEnablei(GL_BLEND, nSlot);
				ms_bMRTBlendEnabled[nSlot] = true;
			}
		}

		ms_bShouldUpdateBlend[nSlot] = false;
		ms_bSeparateBlending = true;
	}
}


void CRenderStates::SetBlendingState(EBlendState eState)
{
	if (ms_bSeparateBlending)
	{
		for (int i = 0; i < 8; i++)
		{
			SetIndependentBlendingState(eState, i);
			ms_bShouldUpdateBlend[i] = true;
		}

		ms_bSeparateBlending = false;
	}

	if (eState != ms_eBlendState)
	{
		if (eState == e_Opaque && ms_bBlendEnabled)
		{
			glDisable(GL_BLEND);
			ms_bBlendEnabled = false;
			ms_eBlendState = e_Opaque;
		}

		else
		{
			if (eState == e_Additive)
			{
				glBlendFunc(GL_ONE, GL_ONE);
				ms_eBlendState = e_Additive;
			}

			else if (eState == e_AlphaBlend)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				ms_eBlendState = e_AlphaBlend;
			}

			else if (eState == e_Opaque_AdditiveAlpha)
			{
				glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ONE);
				ms_eBlendState = e_Opaque_AdditiveAlpha;
			}

			if (!ms_bBlendEnabled)
			{
				glEnable(GL_BLEND);
				ms_bBlendEnabled = true;
			}
		}
	}
}


void CRenderStates::SetCullMode(ECullMode eState)
{
	if (eState != ms_eCullMode)
	{
		if (eState == e_CullNone && ms_bCullingEnabled)
		{
			glDisable(GL_CULL_FACE);
			ms_bCullingEnabled = false;
			ms_eCullMode = e_CullNone;
		}

		else
		{
			if (eState == e_CullBackCW)
			{
				glCullFace(GL_FRONT);
				ms_eCullMode = e_CullBackCW;
			}

			else if (eState == e_CullBackCCW)
			{
				glCullFace(GL_BACK);
				ms_eCullMode = e_CullBackCCW;
			}

			if (!ms_bCullingEnabled)
			{
				glEnable(GL_CULL_FACE);
				ms_bCullingEnabled = true;
			}
		}
	}
}


void CRenderStates::SetDepthStencil(EDepthFunc eDepth, bool bStencil)
{
	if (eDepth != ms_eDepthFunc)
	{
		if (eDepth == e_Zero && ms_bDepthTest)
		{
			glDisable(GL_DEPTH_TEST);
			ms_bDepthTest = false;
		}

		else if (eDepth != e_Zero)
		{
			if (!ms_bDepthTest)
			{
				glEnable(GL_DEPTH_TEST);
				ms_bDepthTest = true;
			}

			if (eDepth == e_Less)
				glDepthFunc(GL_LESS);

			else if (eDepth == e_LessEqual)
				glDepthFunc(GL_LEQUAL);

			else if (eDepth == e_Greater)
				glDepthFunc(GL_GREATER);

			else if (eDepth == e_GreaterEqual)
				glDepthFunc(GL_GEQUAL);

			else if (eDepth == e_Equal)
				glDepthFunc(GL_EQUAL);
		}

		ms_eDepthFunc = eDepth;
	}

	if (bStencil != ms_bStencilTest)
	{
		if (bStencil)
			glEnable(GL_STENCIL_TEST);

		else
			glDisable(GL_STENCIL_TEST);

		ms_bStencilTest = bStencil;
	}
}


void CRenderStates::SetStencil(EStencilOp stencilPass, EStencilOp stencilFail, EStencilOp depthFail, EStencilFunc stencilFunc, unsigned char writeMask, unsigned char readMask, unsigned char ref)
{
	if (ms_eStencilPassOp != stencilPass || ms_eStencilFailOp != stencilFail || ms_eDepthFailOp != depthFail)
	{
		ms_eStencilPassOp = stencilPass;
		ms_eStencilFailOp = stencilFail;
		ms_eDepthFailOp = depthFail;

		glStencilOp(gs_StencilOpAssociation[stencilFail].m_OGLOp, gs_StencilOpAssociation[depthFail].m_OGLOp, gs_StencilOpAssociation[stencilPass].m_OGLOp);
	}

	if (ms_eStencilFunc != stencilFunc || ms_nStencilRef != ref)
	{
		ms_eStencilFunc = stencilFunc;
		ms_nStencilRef = ref;

		glStencilFunc(gs_StencilFuncAssociation[stencilFunc].m_OGLOp, ref, readMask);
		glStencilMask(writeMask);
	}
}

