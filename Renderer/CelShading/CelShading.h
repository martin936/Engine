#ifndef __CEL_SHADING_H__
#define __CEL_SHADING_H__


class CCelShading
{
public :
	
	static void Init();
	static void Process();

	static void Terminate();

	inline static SRenderTarget* GetFinalTarget() { return ms_pFinalTarget; }

private:

	static ProgramHandle	ms_SobelPID;
	static ProgramHandle	ms_MergePID;

	static SRenderTarget* ms_pEdgesTarget;
	static SRenderTarget* ms_pFinalTarget;

	static unsigned int ms_nEdgesConstantBuffer;
};



#endif
