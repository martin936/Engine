#ifndef __DEFERRED_H__
#define __DEFERRED_H__


class CDeferredRenderer
{
	friend class CRenderer;
public:

	static void Init();
	static void Terminate();

	static int UpdateShader(Packet* packet, void* p_pShaderData);
	static int MaterialIDUpdateShader(Packet* packet, void* p_pShaderData);

	inline static unsigned int GetAlbedoTarget()
	{
		return ms_pAlbedoTarget->GetID();
	}

	inline static unsigned int GetDepthTarget()
	{
		return ms_pZBuffer->GetID();
	}

	inline static unsigned int GetLastDepthTarget()
	{
		return ms_pLastZBuffer->GetID();
	}

	inline static unsigned int GetNormalTarget()
	{
		return ms_pNormalTarget->GetID();
	}

	inline static unsigned int GetDiffuseTarget()
	{
		return ms_pDiffuseLighting->GetID();
	}

	inline static unsigned int GetSpecularTarget()
	{
		return ms_pSpecularLighting->GetID();
	}

	inline static unsigned int GetInfoTarget()
	{
		return ms_pInfoTarget->GetID();
	}

	inline static unsigned int GetMotionVectorTarget()
	{
		return ms_pMotionVectorTarget->GetID();
	}

	inline static unsigned int GetMergeTarget()
	{
		return ms_pMergeTarget->GetID();
	}

	inline static unsigned int GetFinalTarget()
	{
		return ms_pMergeTarget->GetID();
	}

	inline static unsigned int GetToneMappedTarget()
	{
		return ms_pToneMappedTarget->GetID();
	}

	inline static void RequestRayCastMaterial(const float MouseX, const float MouseY)
	{
		ms_bRequestRaycast = true;
		ms_bRequestReady = false;

		ms_RaycastCoordX = MouseX;
		ms_RaycastCoordY = MouseY;
	}

	inline static bool GetRayCastMaterial(unsigned int& MaterialID)
	{
		if (ms_bRequestReady)
			MaterialID = ms_RequestedMaterialID;

		return ms_bRequestReady;
	}

	static void UpdateBeforeFlush();

	static void DrawDeferred();

private:

	static void GBufferInit();
	static void GBufferTerminate();

	static void MergeInit();
	static void MergeTerminate();

	static void RenderGBuffer();
	static void Merge();

	static void DrawMaterialID();
	static void RayCastMaterialID();

	static CTexture* ms_pAlbedoTarget;
	static CTexture* ms_pNormalTarget;
	static CTexture* ms_pInfoTarget;
	static CTexture* ms_pMotionVectorTarget;
	static CTexture* ms_pZBuffer;
	static CTexture* ms_pLastZBuffer;

	static CTexture* ms_pToneMappedTarget;

	static CTexture* ms_pDiffuseLighting;
	static CTexture* ms_pSpecularLighting;

	static CTexture* ms_pMergeTarget;

	static CTexture* ms_pMaterialTarget;
	static BufferId	 ms_ReadBackMaterialBuffer;

	static bool			ms_bRequestRaycast;
	static bool			ms_bRequestRaycast4EngineFlush;
	static bool			ms_bRequestReady;

	static float		ms_RaycastCoordX;
	static float		ms_RaycastCoordY;

	static unsigned int ms_RequestedMaterialID;
};


#endif
