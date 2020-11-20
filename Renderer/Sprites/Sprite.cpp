#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "Sprites.h"


unsigned int CSpriteEngine::ms_nNextVertexIndex = 0U;
unsigned int CSpriteEngine::ms_nNextPacketIndex = 0U;
unsigned int CSpriteEngine::ms_nNumVertexToRender = 0U;

unsigned int CSpriteEngine::ms_nVertexBufferID = INVALIDHANDLE;
unsigned int CSpriteEngine::ms_nTexcoordBufferID = INVALIDHANDLE;

unsigned int CSpriteEngine::ms_nMaxNumSprites = 100000;

unsigned int CSpriteEngine::ms_nSpriteID[1000];

ProgramHandle CSpriteEngine::ms_nSprite2DPID = INVALID_PROGRAM_HANDLE;
ProgramHandle CSpriteEngine::ms_nTexturedSprite2DPID = INVALID_PROGRAM_HANDLE;

ProgramHandle CSpriteEngine::ms_nSprite3DPID = INVALID_PROGRAM_HANDLE;
ProgramHandle CSpriteEngine::ms_nTexturedSprite3DPID = INVALID_PROGRAM_HANDLE;

int				CSpriteEngine::ms_nCurrentSequenceSize = 0;
float4			CSpriteEngine::ms_CurrentSequenceColor(1.f, 1.f, 1.f, 1.f);
unsigned int	CSpriteEngine::ms_nCurrentSequenceTextureID = INVALIDHANDLE;

float* CSpriteEngine::ms_pVertexData = NULL;
float* CSpriteEngine::ms_pTexcoordData = NULL;
Packet* CSpriteEngine::ms_pPackets = NULL;
PacketList* CSpriteEngine::ms_pPacketLists = NULL;

std::vector<CSpriteEngine::SSpriteDesc> CSpriteEngine::ms_StickingSprites;

unsigned int gs_SpriteVertexConstantBuffer = INVALIDHANDLE;
unsigned int gs_SpritePixelConstantBuffer = INVALIDHANDLE;


void CSpriteEngine::Init()
{
	/*ms_pVertexData = new float[12 * ms_nMaxNumSprites * sizeof(float)];
	ms_pTexcoordData = new float[8 * ms_nMaxNumSprites * sizeof(float)];

	ms_nVertexBufferID = CDeviceManager::CreateVertexBuffer(NULL, 12 * ms_nMaxNumSprites * sizeof(float), true);
	ms_nTexcoordBufferID = CDeviceManager::CreateVertexBuffer(NULL, 8 * ms_nMaxNumSprites * sizeof(float), true);

	ms_pPackets = new Packet[ms_nMaxNumSprites];
	ms_pPacketLists = new PacketList[ms_nMaxNumSprites];

	ms_nTexturedSprite2DPID = CShader::LoadProgram(SHADER_PATH("Sprites"), "Sprite2D", "TexturedSprite");
	ms_nSprite2DPID = CShader::LoadProgram(SHADER_PATH("Sprites"), "Sprite2D", "Sprite");

	ms_nTexturedSprite3DPID = CShader::LoadProgram(SHADER_PATH("Sprites"), "Sprite3D", "TexturedSprite");
	ms_nSprite3DPID = CShader::LoadProgram(SHADER_PATH("Sprites"), "Sprite3D", "Sprite");

	gs_SpriteVertexConstantBuffer = CDeviceManager::CreateConstantBuffer(NULL, 16 * sizeof(float));
	gs_SpritePixelConstantBuffer = CDeviceManager::CreateConstantBuffer(NULL, 4 * sizeof(float));

	LoadSprites();*/
}



void CSpriteEngine::Terminate()
{
	/*CShader::DeleteProgram(ms_nTexturedSprite2DPID);
	CShader::DeleteProgram(ms_nSprite2DPID);

	CShader::DeleteProgram(ms_nTexturedSprite3DPID);
	CShader::DeleteProgram(ms_nSprite3DPID);

	for (unsigned int i = 0; i < ms_nMaxNumSprites; i++)
		ms_pPacketLists[i].m_pPackets.clear();

	delete[] ms_pVertexData;
	delete[] ms_pTexcoordData;
	delete[] ms_pPacketLists;
	delete[] ms_pPackets;

	CDeviceManager::DeleteVertexBuffer(ms_nVertexBufferID);
	CDeviceManager::DeleteVertexBuffer(ms_nTexcoordBufferID);

	CDeviceManager::DeleteConstantBuffer(gs_SpriteVertexConstantBuffer);
	CDeviceManager::DeleteConstantBuffer(gs_SpritePixelConstantBuffer);*/
}



void CSpriteEngine::LoadSprites()
{
	/*CTexture* pTexture = new CTexture(SPRITES_PATH("bam"));
	SPRITE_BAM_ID = pTexture->m_nID;

	pTexture = new CTexture(SPRITES_PATH("bang"));
	SPRITE_BANG_ID = pTexture->m_nID;

	pTexture = new CTexture(SPRITES_PATH("pow"));
	SPRITE_POW_ID = pTexture->m_nID;

	pTexture = new CTexture(SPRITES_PATH("zap"));
	SPRITE_ZAP_ID = pTexture->m_nID;

	pTexture = new CTexture(SPRITES_PATH("boum"));
	SPRITE_BOUM_ID = pTexture->m_nID;*/
}


void CSpriteEngine::RenderSprites()
{
	/*CFramebuffer::SetDrawBuffers(1);
	CFramebuffer::BindRenderTarget(0, pTarget);
	CFramebuffer::BindDepthStencil(CRenderer::GetDepthStencil());

	CRenderer::DrawPackets(e_RenderType_2D, CMaterial::e_Mixed);*/
}



void CSpriteEngine::PrepareForFlush()
{
	/*void* pVertexData = CDeviceManager::MapBuffer(ms_nVertexBufferID, CDeviceManager::e_Write);
	void* pTexcoordData = CDeviceManager::MapBuffer(ms_nTexcoordBufferID, CDeviceManager::e_Write);

	memcpy(pVertexData, ms_pVertexData, ms_nNextVertexIndex * 3 * sizeof(float));
	memcpy(pTexcoordData, ms_pTexcoordData, ms_nNextVertexIndex * 2 * sizeof(float));

	CDeviceManager::UnmapBuffer(ms_nVertexBufferID);
	CDeviceManager::UnmapBuffer(ms_nTexcoordBufferID);

	float vregisters[16];
	float4x4 WorldViewProj = CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	WorldViewProj.transpose();

	memcpy(vregisters, &(WorldViewProj.m00), 16 * sizeof(float));

	CDeviceManager::FillConstantBuffer(gs_SpriteVertexConstantBuffer, vregisters, 16 * sizeof(float));

	ms_nNumVertexToRender = ms_nNextVertexIndex;

	ms_nNextVertexIndex = 0U;
	ms_nNextPacketIndex = 0U;*/
}



void CSpriteEngine::ProcessStickingSprites()
{
	int nSize = (int)ms_StickingSprites.size();

	for (int i = 0; i < nSize; i++)
	{
		if (ms_StickingSprites[i].m_fTimeLeft < 0.f)
		{
			ms_StickingSprites.erase(ms_StickingSprites.begin() + i);
			i--;
			nSize--;
		}

		else
		{
			AddSprite(ms_StickingSprites[i].m_Position, ms_StickingSprites[i].m_fTexcoords, ms_StickingSprites[i].m_nTextureID, ms_StickingSprites[i].m_Color, CSpriteEngine::Sprite2DUpdateShader);
			ms_StickingSprites[i].m_fTimeLeft -= CEngine::GetFrameDuration() * 1e-3f;
		}
	}
}



void CSpriteEngine::StartCenteredSprite2DSequence(float4& Color, unsigned int nTextureID)
{
	ms_CurrentSequenceColor = Color;
	ms_nCurrentSequenceTextureID = nTextureID;

	ms_nCurrentSequenceSize = 0;
}



void CSpriteEngine::AddCenteredSprite2DToSequence(float fCenter[2], float fSize[2])
{
	float3 vertices[6] = {
		float3(-1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(1.f, 1.f, 0.f)
	};

	float texcoords[12] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};

	for (int i = 0; i < 4; i++)
	{
		vertices[i].x = 0.5f * fSize[0] * vertices[i].x + fCenter[0];
		vertices[i].y = 0.5f * fSize[1] * vertices[i].y + fCenter[1];
	}

	for (int i = 0; i < 4; i++)
	{
		memcpy(&ms_pVertexData[ms_nNextVertexIndex * 3 + 3 * i], &vertices[i].x, 3 * sizeof(float));
	}

	memcpy(&ms_pTexcoordData[ms_nNextVertexIndex * 2], texcoords, 12 * sizeof(float));

	ms_nCurrentSequenceSize += 6;
	ms_nNextVertexIndex += 6;
}



void CSpriteEngine::AddCenteredSprite2DToSequence(float fCenter[2], float fSize[2], float fTexcoords[8])
{
	float3 vertices[6] = {
		float3(-1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(1.f, 1.f, 0.f)
	};

	float texcoords[12];

	for (int i = 0; i < 6; i++)
		texcoords[i] = fTexcoords[i];

	for (int i = 6; i < 12; i++)
		texcoords[i] = fTexcoords[i - 4];

	for (int i = 0; i < 6; i++)
	{
		vertices[i].x = 0.5f * fSize[0] * vertices[i].x + fCenter[0];
		vertices[i].y = 0.5f * fSize[1] * vertices[i].y + fCenter[1];
	}

	for (int i = 0; i < 6; i++)
	{
		memcpy(&ms_pVertexData[ms_nNextVertexIndex * 3 + 3 * i], &vertices[i].x, 3 * sizeof(float));
	}

	memcpy(&ms_pTexcoordData[ms_nNextVertexIndex * 2], texcoords, 12 * sizeof(float));

	ms_nCurrentSequenceSize += 6;
	ms_nNextVertexIndex += 6;
}



void CSpriteEngine::EndCenteredSprite2DSequence()
{
	/*Packet* pPacket = &ms_pPackets[ms_nNextPacketIndex];

	pPacket->m_eType = Packet::e_StandardPacket;

	pPacket->m_nTextures[0] = ms_nCurrentSequenceTextureID;
	pPacket->m_pShaderHook = CSpriteEngine::Sprite2DUpdateShader;
	pPacket->m_bIndexed = false;
	pPacket->m_nFirstIndex = ms_nNextVertexIndex - ms_nCurrentSequenceSize;
	pPacket->m_eTopology = e_TriangleList;
	pPacket->m_nTriangleCount = ms_nCurrentSequenceSize / 3;

	CDeviceManager::BindCommandList(pPacket->VAO);

	CDeviceManager::BindVertexBuffer(ms_nVertexBufferID, 0, 3, CStream::e_Float);
	CDeviceManager::BindVertexBuffer(ms_nTexcoordBufferID, 1, 2, CStream::e_Float);

	CDeviceManager::BindCommandList(0);

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	CPacketManager::AddPacketList(&ms_pPacketLists[ms_nNextPacketIndex], ms_CurrentSequenceColor, false, e_RenderType_2D);

	ms_nNextPacketIndex++;*/
}



void CSpriteEngine::AddSprite(float3 Position[4], float Texcoords[8], unsigned int nTextureID, float4& Color, int(*pShaderHook)(Packet* packet, void* p_pShaderData))
{
	/*for (int i = 0; i < 4; i++)
	{
		memcpy(&ms_pVertexData[ms_nNextVertexIndex * 3 + 3 * i], &Position[i].x, 3 * sizeof(float));
	}

	memcpy(&ms_pTexcoordData[ms_nNextVertexIndex * 2], Texcoords, 8 * sizeof(float));

	Packet* pPacket = &ms_pPackets[ms_nNextPacketIndex];

	pPacket->m_eType = Packet::e_StandardPacket;

	pPacket->m_nTextures[0] = nTextureID;
	pPacket->m_pShaderHook = pShaderHook;
	pPacket->m_bIndexed = false;
	pPacket->m_nFirstIndex = ms_nNextVertexIndex;
	pPacket->m_eTopology = e_TriangleStrip;
	pPacket->m_nTriangleCount = 2;

	CDeviceManager::BindCommandList(pPacket->VAO);

	CDeviceManager::BindVertexBuffer(ms_nVertexBufferID, 0, 3, CStream::e_Float);
	CDeviceManager::BindVertexBuffer(ms_nTexcoordBufferID, 1, 2, CStream::e_Float);

	CDeviceManager::BindCommandList(0);

	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.clear();
	ms_pPacketLists[ms_nNextPacketIndex].m_pPackets.push_back(pPacket);

	CPacketManager::AddPacketList(&ms_pPacketLists[ms_nNextPacketIndex], Color, false, e_RenderType_2D);

	ms_nNextPacketIndex++;
	ms_nNextVertexIndex += 4;*/
}



void CSpriteEngine::AddCenteredSprite2D(float fCenter[2], float fSize[2], float4& Color, unsigned int nTextureID)
{
	float3 vertices[4] = {
		float3(-1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(1.f, 1.f, 0.f)
	};

	float texcoords[8] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};

	for (int i = 0; i < 4; i++)
	{
		vertices[i].x = fSize[0] * vertices[i].x + (fCenter[0] * 2.f - 1.f);
		vertices[i].y = fSize[1] * vertices[i].y + (1.f - 2.f * fCenter[1]);
	}

	AddSprite(vertices, texcoords, nTextureID, Color, CSpriteEngine::Sprite2DUpdateShader);
}



void CSpriteEngine::AddCenteredSprite2D(float fCenter[2], float fSize[2], float fTexcoords[8], float4& Color, unsigned int nTextureID)
{
	float3 vertices[4] = {
		float3(-1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(1.f, 1.f, 0.f)
	};

	for (int i = 0; i < 4; i++)
	{
		vertices[i].x = fSize[0] * vertices[i].x + (fCenter[0] * 2.f + 1.f);
		vertices[i].y = fSize[1] * vertices[i].y + (1.f - 2.f * fCenter[1]);
	}

	AddSprite(vertices, fTexcoords, nTextureID, Color, CSpriteEngine::Sprite2DUpdateShader);
}



void CSpriteEngine::AddCenteredSprite3D(float3& Center, float3& XAxis, float3& YAxis, float fSize[2], float4& Color, unsigned int nTextureID)
{
	float3 vertices[4] = {
		Center - fSize[0] * XAxis - fSize[1] * YAxis,
		Center - fSize[0] * XAxis + fSize[1] * YAxis,
		Center + fSize[0] * XAxis - fSize[1] * YAxis,
		Center + fSize[0] * XAxis + fSize[1] * YAxis
	};

	float texcoords[8] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};

	AddSprite(vertices, texcoords, nTextureID, Color, CSpriteEngine::Sprite3DUpdateShader);
}



void CSpriteEngine::AddStickingCenteredSprite2D(float fCenter[2], float fSize[2], float4& Color, unsigned int nTextureID, float fStickingTime)
{
	float3 vertices[4] = {
		float3(-1.f, -1.f, 0.f),
		float3(-1.f, 1.f, 0.f),
		float3(1.f, -1.f, 0.f),
		float3(1.f, 1.f, 0.f)
	};

	float texcoords[8] = {
		0.f, 0.f,
		0.f, 1.f,
		1.f, 0.f,
		1.f, 1.f
	};

	for (int i = 0; i < 4; i++)
	{
		vertices[i].x = fSize[0] * vertices[i].x + (fCenter[0] * 2.f - 1.f);
		vertices[i].y = fSize[1] * vertices[i].y + (1.f - 2.f * fCenter[1]);
	}

	SSpriteDesc sprite;
	for (int i = 0; i < 4; i++)
		sprite.m_Position[i] = vertices[i];

	for (int i = 0; i < 8; i++)
		sprite.m_fTexcoords[i] = texcoords[i];

	sprite.m_Color = Color;
	sprite.m_nTextureID = nTextureID;
	sprite.m_fTimeLeft = fStickingTime;

	ms_StickingSprites.push_back(sprite);
}



int	CSpriteEngine::Sprite2DUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	/*CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	ProgramHandle nPID;

	if (packet->m_nTextures[0] == INVALIDHANDLE)
		nPID = ms_nSprite2DPID;

	else
		nPID = ms_nTexturedSprite2DPID;

	CShader::BindProgram(nPID);

	CDeviceManager::FillConstantBuffer(gs_SpritePixelConstantBuffer, pShaderData->ModulateColor.v(), 4 * sizeof(float));
	CDeviceManager::BindConstantBuffer(gs_SpritePixelConstantBuffer, nPID, 0);

	if (packet->m_nTextures[0] != INVALIDHANDLE)
	{
		CTexture::SetTexture(nPID, packet->m_nTextures[0], 0);
		CSampler::BindSamplerState(0, CSampler::e_MinMagMip_Linear_UVW_Clamp);
	}

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_Zero, false);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetBlendingState(e_AlphaBlend);
	CRenderStates::SetRasterizerState(e_Solid);*/

	return 1;
}



int	CSpriteEngine::Sprite3DUpdateShader(Packet* packet, void* p_pShaderData)
{
	CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	if (pShaderData->m_nCurrentPass > 0)
		return -1;

	/*CRenderer::SShaderData* pShaderData = (CRenderer::SShaderData*)p_pShaderData;

	ProgramHandle nPID;

	if (packet->m_nTextures[0] == INVALIDHANDLE)
		nPID = ms_nSprite3DPID;

	else
		nPID = ms_nTexturedSprite3DPID;

	CShader::BindProgram(nPID);

	CDeviceManager::FillConstantBuffer(gs_SpritePixelConstantBuffer, pShaderData->ModulateColor.v(), 4 * sizeof(float));
	CDeviceManager::BindConstantBuffer(gs_SpritePixelConstantBuffer, nPID, 0);

	CDeviceManager::BindConstantBuffer(gs_SpriteVertexConstantBuffer, nPID, 1);

	if (packet->m_nTextures[0] != INVALIDHANDLE)
	{
		CTexture::SetTexture(nPID, packet->m_nTextures[0], 0);
		CSampler::BindSamplerState(0, CSampler::e_Anisotropic_Linear_UVW_Clamp);
	}

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetDepthStencil(e_LessEqual, false);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetBlendingState(e_AlphaBlend);
	CRenderStates::SetRasterizerState(e_Solid);*/

	return 1;
}
