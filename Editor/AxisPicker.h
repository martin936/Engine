#ifndef __AXIS_PICKER_H__
#define __AXIS_PICKER_H__

#include "Engine/Renderer/Renderer.h"
#include "Engine/Maths/Maths.h"

__declspec(align(32)) class CAxisPicker
{
public:

	static void Init();

	CAxisPicker();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

	void operator delete(void* p)
	{
		_mm_free(p);
	}

	void Draw();
	void Update();

	inline void SetSize(float fSize) { m_Size = fSize; }
	inline void SetPosition(float3& vPos) { m_Position = vPos; ComputeModelMatrix(); }

	inline float3 GetPosition() const { return m_Position; }
	inline bool IsGrabbed() const { return m_bGrabbed; }

	//static int UpdateShader(Packet* packet, void* p_pShaderData);

private:

	static CMesh* ms_pMesh;

	void ComputeModelMatrix(void);

	float	m_Size;
	float3	m_Position;
	float3	m_SavedPosition;
	float3	m_Offset;
	int		m_nSelectedAxis;

	PacketList* m_pAxisPackets[4];
	float4x4 m_ModelMatrix;

	bool m_bGrabbed;
};

#endif
