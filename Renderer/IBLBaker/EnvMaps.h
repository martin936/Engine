#ifndef __ENV_MAPS_H__
#define __ENV_MAPS_H__

class CIBLBaker
{
public:

	static void Init();
	static void Terminate();

	static void BakeSpecularIBL(const char* cRadiancePath, const char* cBrdfPath, int nSize);

	static void BakeSpecularIBLFromEnvMap();

	static void BakeEnvMap();

	/*inline static unsigned int GetBakedEnvMap()
	{
		return ms_pBakedEnvMap->m_nID;
	}*/

	/*inline static unsigned int GetFilteredEnvMap()
	{
		return ms_pFilteredEnvMap->m_nTextureId;
	}*/

	/*inline static unsigned int GetBrdfMap()
	{
		return ms_pBrdfMap->m_nID;
	}*/

private:

	static ProgramHandle ms_nFilterEnvMapPID;
	static ProgramHandle ms_nPrecomputeBRDFPID;

	static CTexture* ms_pBakedEnvMap;
	static CTexture* ms_pBrdfMap;
	static CTexture* ms_pEnvMap;
	static CTexture* ms_pFilteredEnvMap;

	static int ms_nCameraID;

	static unsigned int ms_nConstantBufferID;

	static PacketList* ms_pPacketList;
};

#endif
