#ifndef __SPHERICAL_HARMONICS_PROBES_H__
#define __SPHERICAL_HARMONICS_PROBES_H__


#define MAX_LIGHT_PROBES 400


__declspec(align(32)) class CSHProbe
{
public:

	CSHProbe(unsigned int nIndex, float3& Pos)
	{
		m_Position = Pos;
		m_nIndex = nIndex;
	}

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	static void Init();
	static void Terminate();

	void Update();

	inline static unsigned int GetSHBuffer()
	{
		return ms_nSHBufferID;
	}

private:

	void RenderPatches();
	void ComputeSH();

	unsigned int m_nIndex;
	float3 m_Position;

	static void PrecomputeSHLookupTable();

	static int UpdateShader(Packet* packet, void* p_pShaderData);

	static ProgramHandle ms_nGenerateLookupPID;
	static ProgramHandle ms_nComputeIrradiancePID;
	static ProgramHandle ms_nRenderPatchesPID;

	//static CFramebuffer* ms_pFBO;
	//static CFramebuffer* ms_pLookupFBO;
	//static CFramebuffer* ms_pEnvMapFBO;

	static SRenderTarget* ms_pLookUpSH;
	static SRenderTarget* ms_pEnvMap;
	static CCamera* ms_pCamera;

	static unsigned int ms_nLookupConstantBufferID;
	static unsigned int ms_nSHBufferID;
	static unsigned int ms_nPatchConstantBufferID;

	static PacketList* ms_pPacketList;

	static bool ms_bIsLookupTableReady;
};


__declspec(align(32)) class CSHNetwork
{
public:

	enum EType
	{
		e_Cubic,
		e_Tetrahedric
	};

	static void Init();
	static void Terminate();

	CSHNetwork(EType eType, float3& Center, float3& Dim, int nNumX, int nNumY, int NumZ);
	~CSHNetwork();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Update();

	static void UpdateAll();
	static void Add(EType eType, float3& Center, float3& Dim, int nNumX, int nNumY, int NumZ);

private:

	EType m_eType;
	
	float3 m_Center;
	float3 m_Dim;
	
	int m_nNumProbesX;
	int m_nNumProbesY;
	int m_nNumProbesZ;

	std::vector<CSHProbe*> m_pSHProbes;
	static std::vector<CSHNetwork*> ms_pNetworks;
};


#endif
