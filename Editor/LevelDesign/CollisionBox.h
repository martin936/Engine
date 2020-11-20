#ifndef __COLLISION_BOX_H__
#define __COLLISION_BOX_H__


__declspec(align(32)) class CCollisionBox
{
	friend class CCollisionBoxManager;

public:

	CCollisionBox();
	~CCollisionBox();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Draw();
	void Enable();
	void Update();

	void ComputeModelMatrix(void);
	void ApplyModelMatrix();

	static int UpdateShader(Packet* packet, void* p_pShaderData);

private:

	PacketList* m_pPackets;
	CObstacle*	m_pObstacle;
	CObstacle*	m_pTransformedObstacle;

	float3		m_vSize;
	float3		m_vSavedSize;
	float		m_fSavedRadius;

	float3		m_vCenter;

	CAxisPicker* m_pAxisPicker;
	float4x4	m_ModelMatrix;

	bool		m_bSelected;
	bool		m_bEnabled;
	bool		m_bShouldScale;
	bool		m_bGrabbed;
	int			m_nSelectedAxis;
};



class CCollisionBoxManager
{
	friend class CCollisionBox;
public:

	static void Init();
	static void Terminate();

	static void Clear();

	static void SelectBox();
	static void Update();
	static void Draw();

	static void AddBox();
	static void RemoveSelectedBox();

	static void Load(const char* cPath);
	static void Save(const char* cPath);

	static void InitPositionGenerator(float fBorder = 0.f);

	static float3 GetRandomSpawningPosition();

private:

	struct SSegment
	{
		float3	m_Start;
		float3	m_End;
		float	m_fSegmentStart;
		float	m_fSegmentEnd;
	};

	static std::vector<SSegment> ms_Segments;

	struct SBoxList
	{
		CCollisionBox* m_pBox;
		struct SBoxList* m_pNext;
	};

	static SBoxList*	ms_pSelectedBox;

	static SBoxList*	ms_pBoxList;
	static SBoxList*	ms_pLastBox;
	static int			ms_nBoxCount;

	static CMesh*		ms_pBaseMesh;
};




#endif
