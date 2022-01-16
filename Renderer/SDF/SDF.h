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

	float3 GetCenter()
	{
		return m_Center;
	}

	float3 GetSize()
	{
		return m_Size;
	}

	unsigned int GetSDFTexture()
	{
		return m_pVolumeSDF->GetID();
	}


	static void ShowSDF();

private:

	int			m_nNumCells[3];
	CTexture*	m_pVolumeSDF;
	CTexture*	m_pVolumeAlbedo;
	CTexture*	m_pInteriorNarrowBand;
	CTexture*	m_pExteriorNarrowBand;
	CTexture*	m_pVoronoiTex[2];
	int			m_nVoronoiIndex;

	bool		m_bIsReady;

	PacketList* m_pPacketList;
	float3		m_Center;
	float3		m_Size;

	static CTexture* ms_pDummyTarget;

	static CSDF* ms_pCurrentSDF;

	static void Clear();
	static void BuildVoronoi();
	static void BuildSDF();
	static void BuildNarrowFields();
	static void BuildNarrowSDF();

	static int NarrowFieldsUpdateShader(Packet* packet, void* pShaderData);

	static std::vector<CSDF*> ms_pSDFToBake[2];
	static std::vector<CSDF*>* ms_pSDFBakeListToFill;
	static std::vector<CSDF*>* ms_pSDFBakeListToFlush;

	static std::vector<CSDF*> ms_pSDFToRender[2];
	static std::vector<CSDF*>* ms_pSDFRenderListToFill;
	static std::vector<CSDF*>* ms_pSDFRenderListToFlush;

	static BufferId	ms_SDFConstantBuffer;
};


#endif
