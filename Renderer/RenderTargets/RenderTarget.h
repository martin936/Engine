#ifndef RENDERER_RENDERTARGET_INC
#define RENDERER_RENDERTARGET_INC

#include "Engine/Renderer/Textures/TextureInterface.h"


/*
class CFramebuffer
{
	friend CDeviceManager;

#ifdef __VULKAN__
	friend CPipeline;
#endif

public:

	CFramebuffer();
	~CFramebuffer();

	static void Init();
	static void Terminate();

	static void InitFrame();

	DepthStencil* CreateDepthStencil(int width, int height, ETextureFormat format, bool bShadowMap);
	DepthStencil* CreateCubeMapDepthStencil(int width, int height, ETextureFormat format, bool bShadowMap);

	static DepthStencil* CreateDepthStencil(int width, int height);
	static DepthStencilArray* CreateDepthStencilArray(int width, int height, int arraySize);

	static void BindDepthStencil(DepthStencil* pDepthStencil);
	static void BindDepthStencilArray(DepthStencilArray* pDethStencil, int nSlice);

	static void BindCubeMapDepthStencil(DepthStencil* depthstencil, int nFace);
	void RestoreDepthStencil();

	static void SetDrawBuffers(unsigned int nCount);

	static SRenderTarget* CreateRenderTarget(int width, int height, ETextureFormat format);
	SRenderTarget* CreateRenderTarget(int slot, int width, int height, ETextureFormat format);
	static void BindRenderTarget(unsigned int slot, SRenderTarget* pTarget, int nLayer = 0, int nLevel = 0);
	static void BindCubeMap(unsigned int slot, SRenderTarget* pTarget, int nFace, int nLayer = 0, int nLevel = 0);

	void SetActive();

	SRenderTarget* GetTarget(int idx) const { return m_pTargets[idx]; }
	DepthStencil* GetDepthStencil() const { return m_pDepthStencil; }

	static void SetBackbuffer();

	inline unsigned int GetID() const
	{
		return m_nId;
	};

	static void SetViewport(int xstart, int ystart, int xend, int yend);

private:

	static CFramebuffer* ms_pCurrent;

	struct SViewport
	{
		int m_xstart;
		int m_ystart;
		int m_xend;
		int m_yend;
	};

	static unsigned int ms_nActiveFBO;
	static SViewport	ms_ActiveViewport;
	static int			ms_nDrawBufferCount;

	unsigned int m_nId;
	unsigned int m_nTargets;

	bool m_bCreated[8];
	bool m_bDSCreated;

	SRenderTarget*		m_pTargets[8];
	DepthStencil*		m_pDepthStencil;
	DepthStencilArray*	m_pDepthStencilArray;
	int					m_nDepthStencilSlice;

	DepthStencil* m_pBackupDepthStencil;
};*/


#endif
