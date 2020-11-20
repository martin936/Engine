#ifndef __INSTANCE_H__
#define __INSTANCE_H__

#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "Engine/Animation/Bones.h"
#include "Engine/Renderer/Packets/Packet.h"


#define MAX_INSTANCES 1000


__declspec(align(32)) class CInstancedMesh
{
	friend class CInstance;
public:

	CInstancedMesh(CMesh* pMesh);
	~CInstancedMesh();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline unsigned int GetID()
	{
		return m_pMesh->GetID();
	}

	inline CMesh* GetMesh()
	{
		return m_pMesh;
	}

	void AddInstance(float4x4 m_World, ERenderList eType = e_RenderType_Standard);
	void AddInstance(float4x4 m_World, float4& Color, ERenderList eType = e_RenderType_Standard);

	void Draw(ERenderList eType = e_RenderType_Standard);

	void Clear();

private:

	CMesh*			m_pMesh;
	PacketList*		m_pPackets;

	unsigned int	m_nNbInstances[MAX_DRAWABLE_LIST_COUNT];

	std::vector<STransformData> m_InstancedTransformData[MAX_DRAWABLE_LIST_COUNT];

	unsigned int	m_nInstanceBufferID;
};



_declspec(align(32)) class CInstance
{
public:

	CInstance(CInstancedMesh* pMesh);
	~CInstance();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void StartAnim(unsigned int nID);
	void StopAnim(unsigned int nID);

	void Transform();

	void ProcessAnims();

	void ProcessSingleAnim(CAnimation* pAnim, float fTime);

	void Draw(ERenderList eType = e_RenderType_Standard);
	void Draw(float4& Color, ERenderList eType = e_RenderType_Standard);

protected:

	void ComputeTransformedBones();
	void ComputeTransformedBonesInConnexSkeletton(CBone* pRoot);

	unsigned int m_nMeshID;

	float4x4	m_ModelMatrix;

	CSkeletton* m_pSkeletton;

	std::vector<CBone*> m_pBones;
	std::vector<float>	m_pAnimStartingTime;

	std::vector<CAnimation*> m_pRunningAnims;

	CInstancedMesh*		m_pInstancedMesh;

	std::vector<DualQuaternion> m_pTransformedBones;
};


#endif

