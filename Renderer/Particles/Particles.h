#ifndef __PARTICLE_RENDER_H__
#define __PARTICLE_RENDER_H__

#define MAX_PARTICLE_SYSTEMS 100

class CParticleRenderer
{
public:

	static void Init();

	static void Terminate();

	//static void Process(SRenderTarget* pTarget, DepthStencil* pDepthStencil);

	static void AddSystem(unsigned int nVertexBufferID, unsigned int nParticleCount);

	struct SBufferIDs
	{
		unsigned int m_nVertexBufferID;
		unsigned int m_nParticleCount;
	};

private:

	static SBufferIDs ms_nBufferIDs[MAX_PARTICLE_SYSTEMS];

	static CTexture*	ms_pNormalBlurTarget;
	static CTexture*	ms_pNormalTarget;
	static CTexture*	ms_pDepthBlurTarget;
	static CTexture*	ms_pDepthTarget;

	static unsigned int ms_nParticleSystemCount;

	static ProgramHandle ms_nProgramID;
	static ProgramHandle ms_nBlurPID;
	static ProgramHandle ms_nMergePID;

	static unsigned int ms_nConstantBufferID;
	static unsigned int ms_nBlurConstantBufferID;

	static unsigned int ms_nCommandListID;

	//static CFramebuffer* ms_pFramebuffer;
};


#endif

