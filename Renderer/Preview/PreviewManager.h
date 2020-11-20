#ifndef __PREVIEW_MANAGER__
#define __PREVIEW_MANAGER__


class CPreviewManager
{
public:

	CPreviewManager();
	~CPreviewManager();

	inline static CPreviewManager* CPreviewManager::GetInstance()
	{
		if (ms_pCurrent == NULL)
			ms_pCurrent = new CPreviewManager;

		return ms_pCurrent;
	}

	inline void Enable() { m_bIsEnabled = true; }
	inline void Disable() { m_bIsEnabled = false; }
	inline bool IsEnabled() const { return m_bIsEnabled; }

	void Run();

	void Load();
	void Load(const char* cMeshPath);

	void SetIBL(const char* cEnvironmentMapPath, const char* cIrradianceMapPath);

	void Unload();

private:

	char		m_cScenePath[512];
	char		m_cEnvironmentPath[512];
	char		m_cIrradiancePath[512];

	//CMesh		*m_pScene;
	CTexture	*m_pEnvironmentMap;
	CTexture	*m_pIrradianceMap;

	std::vector<PacketList*> m_pPacketList;

	bool		m_bIsMeshLoaded;
	bool		m_bIsIBLLoaded;

	bool		m_bIsEnabled;

	int			m_nFreecamId;

	static CPreviewManager* ms_pCurrent;
};


#endif
