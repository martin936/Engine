#ifndef __OBSTACLE_H__
#define __OBSTACLE_H__


__declspec(align(32)) class CObstacle
{
	friend class CCollisionBox;
	friend class CPhysicsEngine;
	friend class CSoftbody;
	friend class CRigidbody;

public:

	CObstacle(CMesh* pMesh, bool bCPU, bool bGPU);
	CObstacle(float3& Center, float3& AABB);
	~CObstacle();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Update() {};

	inline int GetWallCount() const { return m_nWallCount; }

	struct SWall
	{
		TVector4 m_Vertex[3];
		TVector4 m_Normal;
		TVector4 m_TangentBasis[2];
	};

	struct SGPUObstacle
	{
		TVector			m_Center;
		float			m_fRadius;
		unsigned int	m_nWallCount;
		TVector			m_padding;
		SWall			m_Walls[2048];
	};

	inline SWall* GetWall(int idx) const { return m_pWalls + idx; }

	int CheckCollision(TVector CurrentPos, TVector LastPos);

	void ComputeBoundingVolumes();

	inline bool ShouldUseInGameplay() const { return m_bUseOnGPU; }

private:

	int m_nWallCount;
	SWall* m_pWalls;

	TVector m_Center;
	float m_fBoundingSphereRadius;
	float3 m_AABB;

	bool	m_bEnabled;

	bool	m_bUseOnCPU;
	bool	m_bUseOnGPU;
};


#endif
