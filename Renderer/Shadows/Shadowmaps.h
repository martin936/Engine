#ifndef RENDERER_SHADOWMAPS_INC
#define RENDERER_SHADOWMAPS_INC

#include "Engine/Renderer/RenderTargets/RenderTarget.h"
#include "Engine/Renderer/Packets/Packet.h"

typedef struct {

  int type;
  float pos[3], fw[3];
  float power, angle;
  unsigned int color;
  int fading;
  
}Light;


class CShadowMap
{
public:

	static void Init();
	static void Terminate();

	CShadowMap();
	CShadowMap(int width, int height);
	~CShadowMap();

	void ComputeViewMatrix();

	void ComputePerspectiveProjectionMatrix();
	void ComputeOrthographicProjectionMatrix();

	void SetOrthoShadowMap(float3 vPos, float3 vDir, float3 vUp, float fNearPlane, float fFarPlane, float fViewportWidth, float fWiewportHeight);
	void SetPerspectiveShadowMap(float3 vPos, float3 vDir, float3 vUp, float fNearPlane, float fFarPlane, float fFOV, float fAspectRatio);
	inline void SetDirection(const float3& vDir) { m_vDir = vDir; m_vDir.normalize(); }
	inline void SetPosition(const float3& vPos) { m_vPos = vPos; }

	inline float4x4* GetProjMatrix() { return &m_Proj; }
	inline float4x4* GetViewMatrix() { return &m_View; }
	inline float4x4* GetViewProjMatrix() { return &m_ViewProj; }
	inline float4x4* GetInvViewProjMatrix() { return &m_InvViewProj; }

	float GetNearPlane() const { return m_fNearPlane; }
	float GetFarPlane() const { return m_fFarPlane; }
	int GetWidth() const { return ms_nWidth; }
	int GetHeight() const { return ms_nHeight; }

	void Draw();

	void Set(ProgramHandle nPID, unsigned int nSlot);

	static int UpdateShader(Packet* packet, void* p_pShaderData);

	static void BuildArray();

private:

	static DepthStencilArray* ms_pDepthStencilArray;
	int	m_nIndex;

	int m_nWidth;
	int m_nHeight;

	static int ms_nWidth;
	static int ms_nHeight;

	static int ms_nNumShadowMaps;

	float4x4 m_View;
	float4x4 m_Proj;

	float4x4 m_ViewProj;
	float4x4 m_InvViewProj;

	float3	m_vPos;
	float3	m_vDir;
	float3	m_vUp;

	float m_fNearPlane;
	float m_fFarPlane;

	float m_fFOV;
	float m_fAspectRatio;

	float m_fViewportWidth;
	float m_fViewportHeight;
};


#endif
