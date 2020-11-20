#ifndef __VOLMETRIC_LIGHTMAPS_H__
#define __VOUMETRIC_LIGHTMAPS_H__



class CVolumetricLightMap
{
public:

	CVolumetricLightMap();
	~CVolumetricLightMap();

	void Build();
	void SubdivideGrid();
	void BuildSHList();
	void DisplayProbes(SRenderTarget* pTarget);

private:

	unsigned int	m_nVolumetricLightMapID;
	unsigned int	m_nSHBufferID;

	float3			m_Size;
	float3			m_Center;
	unsigned int	m_nMaxLevel;

	unsigned int	m_nNbSHProbes;
	unsigned int	m_nByteWidth;

	bool			m_bIsReady;

	static int		UpdateShader(Packet* packet, void* p_pShaderData);
};



class CVolumetricLightMapsManager
{
	friend CVolumetricLightMap;

public:

	static void Init();
	static void Terminate();

	static CVolumetricLightMap* GetLightMap()
	{
		return ms_pCurrentMap;
	}

private:

	struct SLightMapNode
	{
		float			m_Data[15];
		unsigned int	m_Addr;
	};

	static CVolumetricLightMap* ms_pCurrentMap;

	static CFramebuffer*	ms_pFBO;
	static SRenderTarget*	ms_pDebugTarget;

	static unsigned int		ms_nMaxLevel;
	static float			ms_fMinCellSize;
};



#endif
