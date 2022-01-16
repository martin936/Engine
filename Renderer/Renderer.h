#ifndef RENDERER_INC
#define RENDERER_INC


#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include "Engine/Engine.h"
#include "Engine/Renderer/Lights/Lights.h"
#include "Engine/Device/Shaders.h"
#include "Engine/Renderer/RenderTargets/RenderTarget.h"
#include "Engine/Renderer/Packets/Packet.h"
#include "Engine/Renderer/Packets/Stream.h"
#include "Engine/Renderer/Camera/Camera.h"
#include "Engine/Instance/Instance.h"

#include "Engine/Materials/MaterialDefinition.h"
#include "Engine/PolygonalMesh/PolygonalMesh.h"
#include "Engine/Renderer/Deferred/Deferred.h"
#include "Engine/Renderer/Forward/Forward.h"



enum EVertexLayout
{
	e_Vertex_Layout_Standard,
	e_Vertex_Layout_Engine
};


class CRenderer
{
	friend class CDeferredRenderer;
	friend class CForwardRenderer;

public:

	__declspec(align(32)) struct SShaderData
	{
		unsigned int				m_nNbInstances;
		unsigned int				m_nInstancedBufferID;
		unsigned int				m_nInstancedStreamMask;
		unsigned int				m_nInstancedBufferByteOffset;

		unsigned int				m_nInstancedBufferStride;
		unsigned int				m_nCurrentPass;
		unsigned int				m_nPadding[2];

		Packet						m_nPacket;

		float3x4					m_ModelMatrix;
		float3x4					m_LastModelMatrix;
	};

	enum EDrawInfo
	{
		e_DrawStatic = 1,
		e_DrawDynamic = 2,
		e_DrawAll = 3
	};


	static void Init();
	static void Terminate();

	static void InitRenderPasses();

	static void InitFrame();
	static void EndFrame();

	static void InitRenderQuadScreen();
	static void InitBlueNoiseTextures();

	static void RenderQuadScreen(int numInstances = 1);

	static void DrawPacket(Packet& packet, SShaderData pShaderData, CMaterial::ERenderType eRenderType);
	static void DrawPacketList(PacketList* packetlist, SShaderData& pShaderData, int nRenderFlags);
	static void DrawPackets(ERenderList nRenderList, int nRenderFlags = 0);

	static void Process();

	static void ClearScreen();
	static void ClearColorBits();
	static void ClearColor(float r, float g, float b, float a);
	static void StencilClearValue(unsigned char value);

	inline static CCamera* GetCamera(int nIndex) 
	{ 
		return ms_pCameras[nIndex]; 
	}

	inline static void UseCamera(int nIndex) 
	{ 
		ms_pCurrentCamera = ms_pCameras[nIndex]; 
		ms_nCurrentCameraIndex = nIndex;
	}

	inline static CCamera* GetCurrentCamera() 
	{ 
		return ms_pCurrentCamera; 
	}

	inline static int GetCurrentCameraIndex()
	{
		return ms_nCurrentCameraIndex;
	}

	static int AddCamera(CCamera* pCamera)
	{
		ms_pCameras.push_back(pCamera);

		return (int)ms_pCameras.size() - 1;
	}

	static EDrawInfo ms_eDrawInfo;

	inline static void EnableVolumetrics(bool bEnable)
	{
		ms_bEnableVolumetrics = bEnable;
	}

	inline static bool IsVolumetricsEnabled()
	{
		return ms_bEnableVolumetrics;
	}

	inline static void EnableTransparency(bool bEnable)
	{
		ms_bEnableTransparency = bEnable;
	}

	inline static bool IsAOEnabled()
	{
		return ms_bEnableAO;
	}

	inline static void EnableAO(bool bEnable)
	{
		ms_bEnableAO = bEnable;
	}

	inline static bool IsTransparencyEnabled()
	{
		return ms_bEnableTransparency;
	}

	inline static void EnableAA(bool bEnable)
	{
		ms_bEnableAA = bEnable;
	}

	inline static bool IsAAEnabled()
	{
		return ms_bEnableAA;
	}

	inline static void EnableBloom(bool bEnable)
	{
		ms_bEnableBloom = bEnable;
	}

	inline static bool IsBloomEnabled()
	{
		return ms_bEnableBloom;
	}

	inline static void EnableDOF(bool bEnable)
	{
		ms_bEnableDOF = bEnable;
	}

	inline static bool IsDOFEnabled()
	{
		return ms_bEnableDOF;
	}

	inline static void EnableSSR(bool bEnable)
	{
		ms_bEnableSSR = bEnable;
	}

	inline static bool IsSSREnabled()
	{
		return ms_bEnableSSR;
	}

	inline static void EnableTAA(bool bEnable)
	{
		ms_bEnableTAA = bEnable;
	}

	static void EnableDiffuseGI(bool bEnable);

	inline static bool IsTAAEnabled()
	{
		return ms_bEnableTAA;
	}

	static void SetVertexLayout(EVertexLayout eVertexLayout)
	{
		ms_eVertexLayout = eVertexLayout;
	}

	static EVertexLayout GetVertexLayout()
	{
		return ms_eVertexLayout;
	}

	static void UpdateBeforeFlush();

	static void UpdateLocalMatrices();

	static float4x4 GetLastViewMatrix4EngineFlush()
	{
		return ms_LastGlobalViewMatrix;
	}

	static float4x4 GetLastProjMatrix4EngineFlush()
	{
		return ms_LastGlobalProjMatrix;
	}

	static float4x4 GetLastViewProjMatrix4EngineFlush()
	{
		return ms_LastGlobalViewProjMatrix;
	}

	static float4x4 GetLastInvViewMatrix4EngineFlush()
	{
		return ms_LastGlobalInvViewMatrix;
	}

	static float4x4 GetLastInvViewProjMatrix4EngineFlush()
	{
		return ms_LastGlobalInvViewProjMatrix;
	}

	static float4x4 GetViewMatrix4EngineFlush()
	{
		return ms_ViewMatrix;
	}

	static float4x4 GetProjMatrix4EngineFlush()
	{
		return ms_ProjMatrix;
	}

	static float4x4 GetViewProjMatrix4EngineFlush()
	{
		return ms_ViewProjMatrix;
	}

	static float4x4 GetInvViewMatrix4EngineFlush()
	{
		return ms_InvViewMatrix;
	}

	static float4x4 GetInvViewProjMatrix4EngineFlush()
	{
		return ms_InvViewProjMatrix;
	}

	static float GetFOV4EngineFlush()
	{
		return ms_GlobalFOV;
	}

	static float GetNear4EngineFlush()
	{
		return ms_GlobalNear;
	}

	static float4 GetViewerPosition4EngineFlush()
	{
		return ms_GlobalEyePosition;
	}

	static float GetFar4EngineFlush()
	{
		return ms_GlobalFar;
	}

	static bool IsPacketStatic()
	{
		return ms_bStaticPacket;
	}

	static void SetViewProjConstantBuffer(unsigned int nSlot);

	static int GetCurrentFrame()
	{
		return ms_nCurrentFrame;
	}

	static void EnableViewportCheck()
	{
		ms_bEnableViewportCheck = true;
	}

	static void DisableViewportCheck()
	{
		ms_bEnableViewportCheck = false;
	}

	static CTexture*				ms_pSobolSequence8;
	static CTexture*				ms_pSobolSequence16;
	static CTexture*				ms_pSobolSequence32;

	static CTexture*				ms_pOwenScrambling8;
	static CTexture*				ms_pOwenScrambling16;
	static CTexture*				ms_pOwenScrambling32;

	static CTexture*				ms_pOwenRanking8;
	static CTexture*				ms_pOwenRanking16;
	static CTexture*				ms_pOwenRanking32;

private:

	thread_local static float4x4	ms_ViewMatrix;
	thread_local static float4x4	ms_ProjMatrix;
	thread_local static float4x4	ms_ViewProjMatrix;
	thread_local static float4x4	ms_InvViewMatrix;
	thread_local static float4x4	ms_InvViewProjMatrix;
	thread_local static float4		ms_EyePosition;

	static float4x4					ms_GlobalViewMatrix;
	static float4x4					ms_GlobalProjMatrix;
	static float4x4					ms_GlobalViewProjMatrix;
	static float4x4					ms_GlobalInvViewMatrix;
	static float4x4					ms_GlobalInvViewProjMatrix;
	static float4					ms_GlobalEyePosition;
	static float					ms_GlobalFOV;
	static float					ms_GlobalNear;
	static float					ms_GlobalFar;

	static float4x4					ms_LastGlobalViewMatrix;
	static float4x4					ms_LastGlobalProjMatrix;
	static float4x4					ms_LastGlobalViewProjMatrix;
	static float4x4					ms_LastGlobalInvViewMatrix;
	static float4x4					ms_LastGlobalInvViewProjMatrix;

	static BufferId					ms_ViewProjBuffer;

	static FenceId					ms_FenceFrameFinished;

	static void UpdateGlobalMatrices();
	static bool HasFrameStateChanged();

	thread_local static EVertexLayout ms_eVertexLayout;

	thread_local static bool ms_bStaticPacket;
	thread_local static bool ms_bEnableViewportCheck;

	static bool ms_bEnableVolumetrics;
	static bool ms_bEnableSSR;
	static bool ms_bEnableTransparency;
	static bool ms_bEnableAA;
	static bool ms_bEnableTAA;
	static bool ms_bEnableDOF;
	static bool ms_bEnableBloom;
	static bool ms_bEnableAO;

	static void CopyInit();
	static void CopyTerminate();

	static void Render();

	static std::vector<CCamera*> ms_pCameras;

	static CCamera* ms_pCurrentCamera;
	static int ms_nCurrentCameraIndex;

	static int		ms_nCurrentFrame;
};




#endif