#ifndef __SSAO_H__
#define __SSAO_H__


class CAO
{
public:

	enum ETechnique
	{
		e_SSAO,
		e_HBAO,
		e_SDFAO
	};

	static void Init(ETechnique eTechnique);

	static void SetTechnique(ETechnique eTechnique) 
	{ 
		if (eTechnique == e_SSAO)
			ms_pFinalTarget = ms_pSSAOFinalTarget;

		else if (eTechnique == e_HBAO)
			ms_pFinalTarget = ms_pHBAOFinalTarget;

		ms_eTechnique = eTechnique; 
	}

	static ETechnique GetTechnique() { return ms_eTechnique; }

	inline static unsigned int GetFinalTarget() { return ms_pSDFAOTarget->GetID(); }

	inline static float GetKernelSize()
	{
		return ms_fKernelSize;
	}

	inline static void SetKernelSize(float size)
	{
		ms_fKernelSize = size;
	}

	inline static float GetAOStrength()
	{
		return ms_fStrength;
	}

	inline static void SetAOStrength(float strength)
	{
		ms_fStrength = strength;
	}


	// HBAO
	static void				ApplyHBAO();
	static void				SplitInputs();
	static void				ComputeHBAO();
	static void				DenoiseAndUpscale();
	static void				HBAOCopy();

	// SDFAO
	static void				ComputeSDFAO();

private:

	static void InitHBAO();
	static void InitSSAO();
	static void InitSDFAO();

	static CTexture*		ms_pFinalTarget;
	static ETechnique		ms_eTechnique;

	// SSAO
	static void				ApplySSAO();

	static CTexture*		ms_pSSAOTarget;
	static CTexture*		ms_pBlendedSSAOTarget;
	static CTexture*		ms_pLastSSAOTarget;
	static CTexture*		ms_pBlurredTarget;
	static CTexture*		ms_pSSAOFinalTarget;

	static float			ms_fKernel[64];
	static float			ms_fKernelSize;
	static float			ms_fStrength;

	struct SSSAOFragmentConstants
	{
		float m_InvViewProj[16];
		float m_ViewProj[16];
		float m_LastViewProj[16];
		float m_Kernel[64];
		float m_FrameTick;
		float m_KernelSize;
		float m_Padding[2];
	};

	struct SBlurFragmentConstants
	{
		float m_InvViewProj[16];
		float m_CamDir[4];
		float m_Dir[2];
		float m_Pixel[2];
	};

	static bool				ms_bIsInit;
	static bool				m_bFirstFrame;

	// HBAO
	static CTexture*		ms_pInterleavedDepth;
	static CTexture*		ms_pInterleavedNormals;
	static CTexture*		ms_pInterleavedHBAO;
	static CTexture*		ms_pHBAOHistory;
	static CTexture*		ms_pHBAOFinalTarget;
	static CTexture*		ms_pHBAOBlendFactor;

	// SDFAO
	static CTexture*		ms_pSDFAOTarget;
};


#endif
