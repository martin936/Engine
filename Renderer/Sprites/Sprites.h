#ifndef __SPRITES_H__
#define __SPRITES_H__


#include "Engine/Renderer/Renderer.h"

#define SPRITES_PATH(Name) "../Data/Sprites/" Name ".tga"

#define SPRITE_BAM_ID	CSpriteEngine::ms_nSpriteID[0]
#define SPRITE_BANG_ID	CSpriteEngine::ms_nSpriteID[1]
#define SPRITE_POW_ID	CSpriteEngine::ms_nSpriteID[2]
#define SPRITE_ZAP_ID	CSpriteEngine::ms_nSpriteID[3]
#define SPRITE_BOUM_ID	CSpriteEngine::ms_nSpriteID[4]


class CSpriteEngine
{
public:

	static void Init();
	static void Terminate();

	static void LoadSprites();

	static void AddCenteredSprite2D(float fCenter[2], float fSize[2], float4& Color, unsigned int nTextureID = INVALIDHANDLE);
	static void AddCenteredSprite2D(float fCenter[2], float fSize[2], float Texcoords[8], float4& Color, unsigned int nTextureID = INVALIDHANDLE);
	static void AddCenteredSprite3D(float3& Center, float3& XAxis, float3& YAxis, float fSize[2], float4& Color, unsigned int nTextureID = INVALIDHANDLE);

	static void StartCenteredSprite2DSequence(float4& Color, unsigned int nTextureID = INVALIDHANDLE);
	static void AddCenteredSprite2DToSequence(float fCenter[2], float fSize[2]);
	static void AddCenteredSprite2DToSequence(float fCenter[2], float fSize[2], float Texcoords[8]);
	static void EndCenteredSprite2DSequence();

	static void AddSprite(float3 Position[4], float Texcoords[8], unsigned int nTextureID, float4& Color, int(*pShaderHook)(Packet* packet, void* p_pShaderData));

	static void AddStickingCenteredSprite2D(float fCenter[2], float fSize[2], float4& Color, unsigned int nTextureID, float fStickingTime);

	static void PrepareForFlush();
	static void ProcessStickingSprites();

	static void RenderSprites();

	static unsigned int ms_nSpriteID[1000];

private:

	static unsigned int ms_nVertexBufferID;
	static unsigned int ms_nTexcoordBufferID;

	static float*	ms_pVertexData;
	static float*	ms_pTexcoordData;

	static Packet* ms_pPackets;
	static PacketList* ms_pPacketLists;

	struct SSpriteDesc
	{
		float3			m_Position[4];
		float			m_fTexcoords[8];
		float4			m_Color;
		unsigned int	m_nTextureID;
		float			m_fTimeLeft;
	};


	static std::vector<SSpriteDesc> ms_StickingSprites;

	static unsigned int ms_nNextVertexIndex;
	static unsigned int ms_nNextPacketIndex;
	static unsigned int ms_nNumVertexToRender;
	static unsigned int ms_nMaxNumSprites;

	static ProgramHandle ms_nSprite2DPID;
	static ProgramHandle ms_nTexturedSprite2DPID;

	static ProgramHandle ms_nSprite3DPID;
	static ProgramHandle ms_nTexturedSprite3DPID;

	static int	Sprite2DUpdateShader(Packet* packet, void* p_pShaderData);
	static int	Sprite3DUpdateShader(Packet* packet, void* p_pShaderData);

	static int			ms_nCurrentSequenceSize;
	static float4		ms_CurrentSequenceColor;
	static unsigned int ms_nCurrentSequenceTextureID;
};


#endif
