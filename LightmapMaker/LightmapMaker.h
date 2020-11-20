#ifndef __LIGHTMAP_MAKER_H__

#include <vector>
#include "Engine/PolygonalMesh/PolygonalMesh.h"


class CMeshCharts
{
public:

	CMeshCharts(CMesh* pMesh, unsigned int nPacketID);
	~CMeshCharts() {};

	void Segment();
	void Parameterize();

	void CreateBuffer();
	static void BindBuffer();

private:

	void ExtractFaces();
	void ConnectFaces();

	void InitSeeds();
	void GrowCharts();
	void UpdateSeeds();
	bool HasConverged();

	unsigned int	PickNextSeed(unsigned int lastFace);
	unsigned int	SelectBestCandidate(unsigned int currentFace);
	unsigned int	SelectWorstCandidate(unsigned int currentFace);
	bool			IsCandidateValid(unsigned int currentFace, unsigned int faceToTest);
	void			AddFace(unsigned int currentFace, unsigned int nextFace);

	void			ComputeChartNormals();
	void			ComputeSum(std::vector<float3>& normal, std::vector<float>& sum);

	void			ComputeSeeds();

	float			Cost(unsigned int faceID, unsigned int chartID);

	float			EdgeLength(unsigned int face1, unsigned int face2);

	__declspec(align(16)) struct SFace
	{
		unsigned int				m_nVertexID[3];
		float						m_fArea;

		unsigned int				m_nNumConnectedFaces;
		unsigned int				m_nConnectedFacesID[3];

		int							m_nChartID;
		float3						m_Normal;

		float3						m_Barycenter;
		float						m_nDistanceToSeed;

		float						m_fPerimeter;
		float3						Padding;
	};

	std::vector<SFace>						m_Faces;
	std::vector<unsigned int>				m_Seeds;
	std::vector<float3>						m_ChartNormal;
	std::vector<float>						m_ChartArea;
	std::vector<float>						m_NumFaces;

	std::vector<std::vector<unsigned int>>	m_SeedsHistory;

	CMesh*									m_pMesh;
	unsigned int							m_nMeshPacketID;

	static unsigned int						ms_nBufferID;
	static unsigned int						ms_nBufferSize;
};


class CAtlas
{
public:

	CAtlas(float paddingSize);
	~CAtlas();

private:

	struct SPackedChart
	{
		CMeshCharts*	m_pChart;

		float			m_fRotatedAngle;
		float			m_fSize;
		float			m_fOffset[2];
	};

	std::vector<SPackedChart*> m_pPackedCharts;
};



class CLightmapMaker
{
	friend CMeshCharts;
	friend CAtlas;
public:

	static void Init();
	static void Terminate();

private:

	static float ms_fMaxNormalAngle;
};


#endif
