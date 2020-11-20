#ifndef __SMAA_H__
#define __SMAA_H__

class CSMAA
{
public:

	static void Init();

	static void Process(CTexture* pColorTarget);

	static void Terminate();

	inline static CTexture* GetFinalTarget() { return ms_pFinalTarget; }

private:

	static CTexture* ms_pEdgesTarget;
	static CTexture* ms_pWeightsTarget;
	static CTexture* ms_pFinalTarget;

	static CTexture*	ms_pAreaTex;
	static CTexture*	ms_pSearchTex;

	static ProgramHandle	ms_ComputeVelocities;
	static ProgramHandle	ms_EdgesPID;
	static ProgramHandle	ms_WeightPID;
	static ProgramHandle	ms_BlendPID;

	static unsigned int		ms_nConstantBuffer;
};


#endif
