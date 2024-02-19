#ifndef __SHADOW_RENDERER_H__
#define __SHADOW_RENDERER_H__


#define MAX_SHADOWS_PER_FRAME	64


__declspec(align(32)) class CShadowRenderer
{
	friend class CLight;
	friend class CLightItem;

public:

	CShadowRenderer(CLight* pLight);
	~CShadowRenderer();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	static void			Init();

	inline static int	GetShadowmapSize()
	{
		return ms_nShadowmapSize;
	}

	inline static unsigned int GetShadowmapHiZ()
	{
		return ms_pShadowsHiZ->GetID();
	}

	inline static unsigned int GetShadowmapArray()
	{
		return ms_pShadowMapArray->GetID();
	}

	inline static unsigned int GetSunShadowmapArray()
	{
		return ms_pSunShadowMaps->GetID();
	}

	inline static unsigned int GeFilteredShadowArray()
	{
		return ms_pFilteredShadowArray->GetID();
	}

	inline static CTexture* GeFilteredShadowArrayTexture()
	{
		return ms_pFilteredShadowArray;
	}

	int GetDynamicIndex() const { return m_nDynamicIndex; }
	int GetStaticIndex() const { return m_nStaticIndex; }

	int GetDynamicIndex4EngineFlush() const { return m_nRenderingDynamicIndex; }
	int GetStaticIndex4EngineFlush() const { return m_nRenderingStaticIndex; }

	int GetViewport() const
	{
		return m_nViewport;
	}

	CLight* GetLight()
	{
		return m_pLight;
	}

	static void			RenderShadowMaps();

	virtual void		UpdateViewport();
	static void			PrepareForFlush();

	static int		GetNextStaticShadowMapIndex();

	static int		GetNextDynamicShadowMapIndex()
	{
		return ms_nNumDynamicShadowmaps >= ms_nMaxDynamicShadowmaps ? -1 : ms_nNumDynamicShadowmaps;
	}

	inline static void SetDynamicShadowMapMaxCount(unsigned int nNum)
	{
		ms_nMaxDynamicShadowmaps = nNum;
	}

	inline static void SetStaticShadowMapMaxCount(unsigned int nNum)
	{
		ms_nMaxStaticShadowmaps = nNum;
	}

	inline static void SetShadowMapSize(unsigned int nSize)
	{
		ms_nShadowmapSize = nSize;
	}

	static int UpdateShader(Packet* packet, void* pData);

	static unsigned int GetNumShadowViewports4EngineFlush()
	{
		return static_cast<unsigned int>(ms_ViewportsToUpdateToFlush->size());
	}

	static int GetShadowViewportStaticIndex4EngineFlush(unsigned int i)
	{
		return (*ms_ViewportsToUpdateToFlush)[i].m_nStaticIndex;
	}

	static int GetShadowViewportDynamicIndex4EngineFlush(unsigned int i)
	{
		return (*ms_ViewportsToUpdateToFlush)[i].m_nDynamicIndex;
	}

	static bool IsShadowViewportOmni4EngineFlush(unsigned int i)
	{
		return (*ms_ViewportsToUpdateToFlush)[i].m_nOmni == 1;
	}

protected:

	struct SViewportAssociation
	{
		int	m_nViewport;
		int m_nStaticIndex;
		int m_nDynamicIndex;
		int m_nOmni;
	};

	static void	RenderSunShadowMaps();
	static void	RenderSunShadowMapsAlpha();

	static std::vector<SViewportAssociation>	ms_ViewportsToUpdate[2];
	static std::vector<SViewportAssociation>*	ms_ViewportsToUpdateToFill;
	static std::vector<SViewportAssociation>*	ms_ViewportsToUpdateToFlush;

	static CTexture*	ms_pShadowMapArray;
	static CTexture*	ms_pShadowCubeMapArray;
	static CTexture*	ms_pShadowsHiZ;

	static CTexture*	ms_pSunShadowMaps;
	static CTexture*	ms_pSunShadowsHiZ;

	static CTexture*	ms_pFilteredShadowArray;

	static int			ms_nLightIndexArray[MAX_SHADOWS_PER_FRAME];
	static float		ms_fLastActualizationTime[MAX_SHADOWS_PER_FRAME];
	static bool			ms_bUsedInThisFrame[MAX_SHADOWS_PER_FRAME];
	static bool			ms_bShadowmapCleared[MAX_SHADOWS_PER_FRAME];
	static bool			ms_bShouldUpdateHiZ[MAX_SHADOWS_PER_FRAME * 2];

	void				ComputeViewMatrix();
	virtual void		UpdateShadowMatrix() {};

	static int			ms_nMaxStaticShadowmaps;
	static int			ms_nMaxDynamicShadowmaps;
	static int			ms_nNumStaticShadowmaps;
	static int			ms_nNumDynamicShadowmaps;
	static int			ms_nNumStaticShadowmapsInFrame;
	static bool			ms_bAreStaticSMUpdated;

	static int			ms_nShadowmapSize;
	static int			ms_nSunShadowmapSize;

	static unsigned int* ms_nOwner;

	int					m_nViewport;
	int					m_nDynamicIndex;
	int					m_nStaticIndex;
	bool				m_bUpdateStatic;

	bool				m_bForceUpdateStaticShadowMap;

	int					m_nRenderingDynamicIndex;
	int					m_nRenderingStaticIndex;

	int					m_nLastFrame;

	CLight*				m_pLight;

	float3				m_Position;
	float3				m_Position4EngineFlush;
};



class CShadowOmni : public CShadowRenderer
{
	friend CShadowRenderer;
public:

	struct ShadowInfo
	{
		float4 m_Pos;
		float4 m_NearFar;
	};

	CShadowOmni(CLight* pLight);

	void UpdateViewport() override;

	static BufferId	GetShadowMatricesConstantBuffer()
	{
		return ms_nShadowMatricesBuffer;
	}

	static float4x4 GetShadowMatrix4EngineFlush(int index)
	{
		return (*ms_pShadowMatricesToFlush)[index];
	}

	static BufferId	GetShadowPosConstantBuffer()
	{
		return ms_nShadowPosBuffer;
	}

	static ShadowInfo GetShadowPos4EngineFlush(int index)
	{
		return (*ms_pShadowPosToFlush)[index];
	}

private:

	static std::vector<float4x4>				ms_pShadowMatrices[2];
	static std::vector<float4x4>*				ms_pShadowMatricesToFill;
	static std::vector<float4x4>*				ms_pShadowMatricesToFlush;

	static std::vector<ShadowInfo>				ms_pShadowPos[2];
	static std::vector<ShadowInfo>*				ms_pShadowPosToFill;
	static std::vector<ShadowInfo>*				ms_pShadowPosToFlush;

	static BufferId								ms_nShadowMatricesBuffer;
	static BufferId								ms_nShadowPosBuffer;

	void	ComputeShadowMatrices(float4x4* p_ShadowMatrices);

	float4x4	m_ShadowMatrix[6];
	float		m_Near;
	float		m_Far;
};



class CShadowSpot : public CShadowRenderer
{
	friend CShadowRenderer;
public:

	CShadowSpot(CLight* pLight);

	void PrepareForFlush();

	void UpdateViewport() override;

	static BufferId	GetShadowMatricesConstantBuffer()
	{
		return ms_nShadowMatricesBuffer;
	}

	static float4x4 GetShadowMatrix4EngineFlush(int index)
	{
		return (*ms_pShadowMatricesToFlush)[index];
	}

private:

	static std::vector<float4x4>				ms_pShadowMatrices[2];
	static std::vector<float4x4>*				ms_pShadowMatricesToFill;
	static std::vector<float4x4>*				ms_pShadowMatricesToFlush;

	static BufferId								ms_nShadowMatricesBuffer;

	float4x4	ComputeShadowMatrix();

	float4x4	m_ShadowMatrix;
};


class CShadowDir : public CShadowRenderer
{
	friend CShadowRenderer;
public:

	CShadowDir(CLight* pLight);

	void UpdateViewport() override;

	static BufferId	GetShadowMatricesConstantBuffer()
	{
		return ms_nShadowMatricesBuffer;
	}

	static CShadowDir* GetSunShadowRenderer()
	{
		return ms_pSunShadowRenderer;
	}

	float4x4 GetShadowMatrix4EngineFlush()
	{
		return m_ShadowMatrix4EngineFlush;
	}

	static int UpdateShader(Packet* packet, void* pData);

	static bool		ms_bDrawStatic;
	static bool		ms_bDrawStatic4EngineFlush;

private:

	static BufferId		ms_nShadowMatricesBuffer;

	static CShadowDir*	ms_pSunShadowRenderer;

	float4x4			ComputeShadowMatrix();

	float4x4			m_ShadowMatrix4EngineFlush;
	float4x4			m_ShadowMatrix;
};


#endif
