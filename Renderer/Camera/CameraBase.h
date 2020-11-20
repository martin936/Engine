#ifndef __CAMERA_BASE_H__
#define __CAMERA_BASE_H__


__declspec(align(32)) class CCamera
{
	friend class CCameraEditor;

public:

	CCamera();
	~CCamera();

	void* operator new(size_t i)
	{
		return _mm_malloc(i, 32);
	}

		void operator delete(void* p)
	{
		_mm_free(p);
	}

	inline float4x4 GetProjMatrix() { return m_Proj; }
	inline float4x4 GetViewMatrix() { return m_View; }
	inline float4x4 GetViewProjMatrix() { return m_ViewProj; }
	inline float4x4 GetInvViewProjMatrix() { return m_InvViewProj; }
	inline float4x4 GetLastFrameViewProjMatrix() { return m_LastViewProj; }

	inline float3 GetDirection()	{ return m_vDir; }
	inline float3 GetUp()			{ return m_vUp; }
	inline float3 GetRight()		{ return m_vRight; }

	inline void SetViewProjMatrix(float4x4& ViewProj) { m_ViewProj = ViewProj; }

	void SetCamera(float fov, float fAspectRatio, float nearPlane, float farPlane, const float3& Up, const float3& Pos, const float3& Target);
	void ComputeViewMatrix();
	void ComputeProjectionMatrix();

	inline float GetNearPlane() const { return m_fNearPlane; }
	inline float GetFarPlane() const { return m_fFarPlane; }

	inline float GetFOV() const { return m_fFOV; }
	inline float GetAspectRatio() const { return m_fAspectRatio; }

	static void JitterViewProj(float4x4& ViewProj, int k);

	inline static void UpdateJitterIndex() 
	{ 
		ms_nCurrentJitterIndex = (ms_nCurrentJitterIndex + 1) % 16; 
	}

	inline void SetPosition(float3& Pos)
	{
		m_vPos = Pos;
		ComputeViewMatrix();

		m_ViewProj = m_Proj * m_View;
		JitterViewProj(m_ViewProj, ms_nCurrentJitterIndex);

		m_InvViewProj = inverse(m_ViewProj);
	}

	inline void SetFieldOfView(float fFOV)
	{
		m_fFOV = fFOV;
		ComputeProjectionMatrix();

		m_ViewProj = m_Proj * m_View;
		JitterViewProj(m_ViewProj, ms_nCurrentJitterIndex);

		m_InvViewProj = inverse(m_ViewProj);
	}

	inline float3 GetPosition() { return m_vPos; }

	virtual void Update() {};

	inline static int GetCurrentJitterIndex() { return ms_nCurrentJitterIndex; }

protected:

	float4x4 m_Proj;
	float4x4 m_View;
	float4x4 m_ViewProj;
	float4x4 m_InvViewProj;
	float4x4 m_LastViewProj;

	float	m_fFOV;
	float	m_fAspectRatio;

	float3	m_vPos;
	float3	m_vUp;
	float3	m_vRight;
	float3	m_vDir;

	float	m_fFarPlane;
	float	m_fNearPlane;

	static int		ms_nCurrentJitterIndex;
};


#endif
