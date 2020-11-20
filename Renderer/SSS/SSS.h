#ifndef __SSS_H__
#define __SSS_H__


__declspec(align(32)) class CSSS
{
public:

	static void Init();
	static void Terminate();

	struct SProfile
	{
		SProfile()
		{
			m_Near = 0.f;
			m_Far = 0.f;
			m_Ratio = 0.f;
		};

		float3 m_Near;
		float3 m_Far;
		float3 m_Ratio;
	};

	static void AddProfile(SProfile& Profile);

	static void Apply();

	//static unsigned int GetProfileTexture() { return ms_pSSSLinearProfiles->m_nTextureId; }

	/*inline static unsigned int GetDepthCopy()
	{
		return ms_pDepthCopy->m_nTextureId;
	}*/

private:

	static bool ms_bShouldComputeProfiles;

	static int	ms_nNbSamples;
	static int	ms_nMaxNbProfiles;
	static int	ms_nNbProfiles;

	static ProgramHandle ms_nComputeProfilesPID;
	static ProgramHandle ms_nSSSPID;
	static ProgramHandle ms_nSSSFollowSurfacePID;
	static ProgramHandle ms_nCopyDepthPID;

	static unsigned int ms_nComputeProfilesConstantBufferID;
	static unsigned int ms_nSSSConstantBufferID;

	static SProfile* ms_pProfiles;

	static CTexture* ms_pTranslucencyMap;
	static CTexture* ms_pSSSProfiles;
	static CTexture* ms_pSSSLinearProfiles;
	static CTexture* ms_pSeparableSSSTarget;
	static CTexture* ms_pDepthCopy;

public:

	static void ComputeProfiles();
};


#endif
