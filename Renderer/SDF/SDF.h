#ifndef __SDF_H__
#define __SDF_H__


#include "Engine/Renderer/Textures/Textures.h"


class CSDF
{
public:

	CSDF(CMesh& mesh, int numCellX, int numCellY, int numCellZ);

	void Bake();
	void Render();

	bool IsReady()
	{
		return m_bIsReady;
	}

	static void Init();
	static void UpdateBeforeFlush();

	static void BindSDFs(unsigned int nSlot);
	static void BindVolumeAlbedo(unsigned int nSlot);
	static void SetSDFConstantBuffer(unsigned int nSlot);

private:

	int			m_nNumCells[3];
	CTexture*	m_pVolumeSDF;
	CTexture*	m_pVolumeAlbedo;
	CTexture*	m_pNarrowBand;
	CTexture*	m_pNarrowBandSign;
	CTexture*	m_pVoronoiTiling[2];
	int			m_nVoronoiIndex;

	bool		m_bIsReady;

	PacketList* m_pPacketList;
	float3		m_Center;
	float3		m_Size;

	static float3	ms_CurrentCenter;
	static float3	ms_CurrentSize;
	static int		ms_CurrentProjectionAxis;

	static CTexture* ms_pDummyTarget;

	static void Clear();
	static void ComputeNarrowUDF();
	static void ComputeSeeds();
	static void BuildVoronoi();
	static void BuildSDF();
	static void ShowSDF();

	static int NarrowUDFUpdateShader(Packet* packet, void* pShaderData);
	static int SeedsUpdateShader(Packet* packet, void* pShaderData);

	static std::vector<CSDF*> ms_pSDFToBake[2];
	static std::vector<CSDF*>* ms_pSDFBakeListToFill;
	static std::vector<CSDF*>* ms_pSDFBakeListToFlush;

	static std::vector<CSDF*> ms_pSDFToRender[2];
	static std::vector<CSDF*>* ms_pSDFRenderListToFill;
	static std::vector<CSDF*>* ms_pSDFRenderListToFlush;

	static BufferId	ms_SDFConstantBuffer;
};


#endif
