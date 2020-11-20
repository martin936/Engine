#include "Engine/Engine.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Renderer/Renderer.h"
#include "VolumetricLightmaps.h"


ProgramHandle	gs_InsertGeometryPID		= INVALID_PROGRAM_HANDLE;
ProgramHandle	gs_BuildSHListPID			= INVALID_PROGRAM_HANDLE;
ProgramHandle	gs_DisplayProbesPID			= INVALID_PROGRAM_HANDLE;

unsigned int	gs_InsertGeometryConstants	= INVALID_HANDLE;
unsigned int	gs_DisplayProbesConstants	= INVALID_HANDLE;

CVolumetricLightMap* CVolumetricLightMapsManager::ms_pCurrentMap = NULL;

unsigned int	CVolumetricLightMapsManager::ms_nMaxLevel		= 5;
float			CVolumetricLightMapsManager::ms_fMinCellSize	= 0.01f;
CFramebuffer*	CVolumetricLightMapsManager::ms_pFBO			= NULL;
SRenderTarget*	CVolumetricLightMapsManager::ms_pDebugTarget	= NULL;

unsigned int	gs_GlobalBuffer = INVALID_HANDLE;


struct SProbeIndex
{
	float3			m_Pos;
	unsigned int	m_nIndex;
};



void CVolumetricLightMapsManager::Init()
{
	gs_InsertGeometryPID	= CShader::LoadProgram(SHADER_PATH("VolumetricLightmaps"), "VolumetricLightmaps", "InsertGeometry");
	gs_BuildSHListPID		= CShader::LoadProgram(SHADER_PATH("VolumetricLightmaps"), "BuildList");
	gs_DisplayProbesPID		= CShader::LoadProgram(SHADER_PATH("VolumetricLightmaps"), "DisplayProbes", "DisplayProbes");

	gs_InsertGeometryConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(float4x4));
	gs_DisplayProbesConstants	= CDeviceManager::CreateConstantBuffer(NULL, sizeof(float4x4));

	ms_pCurrentMap = new CVolumetricLightMap;

	ms_pFBO = new CFramebuffer;
	ms_pFBO->SetDrawBuffers(1);

	unsigned int nLevel = 3;
	int nSize = (int)pow<int>(4, nLevel);

	ms_pDebugTarget = CFramebuffer::CreateRenderTarget(nSize, nSize, ETextureFormat::e_R8);

	unsigned int nOne = 1U;
	gs_GlobalBuffer = CDeviceManager::CreateStorageBuffer(&nOne, sizeof(unsigned int));
}



void CVolumetricLightMapsManager::Terminate()
{
	CShader::DeleteProgram(gs_InsertGeometryPID);
	CShader::DeleteProgram(gs_BuildSHListPID);
	CShader::DeleteProgram(gs_DisplayProbesPID);

	CDeviceManager::DeleteConstantBuffer(gs_InsertGeometryConstants);
	CDeviceManager::DeleteConstantBuffer(gs_DisplayProbesConstants);

	CDeviceManager::DeleteStorageBuffer(gs_GlobalBuffer);

	delete ms_pFBO;

	delete ms_pDebugTarget;

	if (ms_pCurrentMap != NULL)
		delete ms_pCurrentMap;
}



CVolumetricLightMap::CVolumetricLightMap()
{
	m_nMaxLevel = CVolumetricLightMapsManager::ms_nMaxLevel;
	m_Size = 100.f;
	m_Center = 0.f;
	m_bIsReady = false;
	m_nByteWidth = 0;

	char* pData = new char[128 * 1024 * 1024];
	ZeroMemory(pData, 128 * 1024 * 1024);

	m_nVolumetricLightMapID = CDeviceManager::CreateStorageBuffer(pData, 128 * 1024 * 1024);

	m_nSHBufferID = CDeviceManager::CreateStorageBuffer(NULL, 2 * 1024 * 1024 * sizeof(SProbeIndex));

	delete[] pData;
}



CVolumetricLightMap::~CVolumetricLightMap()
{
	if (m_nVolumetricLightMapID != INVALID_HANDLE)
		CDeviceManager::DeleteStorageBuffer(m_nVolumetricLightMapID);
}



void CVolumetricLightMap::Build()
{
	SubdivideGrid();

	BuildSHList();
}



void CVolumetricLightMap::SubdivideGrid()
{
	CPacketManager::ForceShaderHook(CVolumetricLightMap::UpdateShader);

	float4x4 ViewProj = 0.f;
	ViewProj.m00 = 1.f / m_Size.x;
	ViewProj.m11 = 1.f / m_Size.y;
	ViewProj.m22 = 1.f / m_Size.z;
	ViewProj.m33 = 1.f;
	ViewProj.m03 = -m_Center.x;
	ViewProj.m13 = -m_Center.y;
	ViewProj.m23 = -m_Center.z;

	ViewProj.transpose();

	CVolumetricLightMapsManager::ms_pFBO->SetActive();
	CVolumetricLightMapsManager::ms_pFBO->BindRenderTarget(0, CVolumetricLightMapsManager::ms_pDebugTarget);

	CDeviceManager::FillConstantBuffer(gs_InsertGeometryConstants, ViewProj.m(), sizeof(ViewProj));

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred | CMaterial::e_Mixed | CMaterial::e_Forward);

	ViewProj = 0.f;

	ViewProj.m20 = 1.f / m_Size.x;
	ViewProj.m01 = 1.f / m_Size.y;
	ViewProj.m12 = 1.f / m_Size.z;
	ViewProj.m33 = 1.f;
	ViewProj.m03 = -m_Center.x;
	ViewProj.m13 = -m_Center.y;
	ViewProj.m23 = -m_Center.z;

	ViewProj.transpose();

	CDeviceManager::FillConstantBuffer(gs_InsertGeometryConstants, ViewProj.m(), sizeof(ViewProj));

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred | CMaterial::e_Mixed | CMaterial::e_Forward);

	ViewProj = 0.f;

	ViewProj.m10 = 1.f / m_Size.x;
	ViewProj.m21 = 1.f / m_Size.y;
	ViewProj.m02 = 1.f / m_Size.z;
	ViewProj.m33 = 1.f;
	ViewProj.m03 = -m_Center.x;
	ViewProj.m13 = -m_Center.y;
	ViewProj.m23 = -m_Center.z;

	ViewProj.transpose();

	CDeviceManager::FillConstantBuffer(gs_InsertGeometryConstants, ViewProj.m(), sizeof(ViewProj));

	CRenderer::DrawPackets(e_RenderType_Standard, CMaterial::e_Deferred | CMaterial::e_Mixed | CMaterial::e_Forward);

	CPacketManager::ForceShaderHook(NULL);
}



void CVolumetricLightMap::BuildSHList()
{
	CDeviceManager::SetMemoryBarrierOnBufferAccess();

	unsigned int nNbNodes;

	unsigned int* pMapped = (unsigned int*)CDeviceManager::MapBuffer(gs_GlobalBuffer, CDeviceManager::e_Read | CDeviceManager::e_Write);

	nNbNodes = (*pMapped) * 64U;
	m_nByteWidth = nNbNodes * sizeof(CVolumetricLightMapsManager::SLightMapNode);
	*pMapped = 0U;

	CDeviceManager::UnmapBuffer(gs_GlobalBuffer);

	CDeviceManager::SetMemoryBarrierOnBufferMapping();

	CShader::BindProgram(gs_BuildSHListPID);

	CDeviceManager::BindStorageBuffer(m_nVolumetricLightMapID, 0, sizeof(CVolumetricLightMapsManager::SLightMapNode), 0, 128 * ((nNbNodes + 127) / 128));
	CDeviceManager::BindStorageBuffer(gs_GlobalBuffer, 1, sizeof(unsigned int), 0, 1);
	CDeviceManager::BindStorageBuffer(m_nSHBufferID, 2, sizeof(SProbeIndex), 0, 128 * ((nNbNodes + 127) / 128));

	CDeviceManager::SetUniform(gs_BuildSHListPID, "NbProbes", nNbNodes);

	CDeviceManager::DispatchDraw((nNbNodes + 127) / 128, 1, 1);

	CDeviceManager::SetMemoryBarrierOnBufferAccess();

	pMapped = (unsigned int*)CDeviceManager::MapBuffer(gs_GlobalBuffer, CDeviceManager::e_Read);

	m_nNbSHProbes = *pMapped;

	CDeviceManager::UnmapBuffer(gs_GlobalBuffer);
}



int CVolumetricLightMap::UpdateShader(Packet* packet, void* p_pShaderData)
{
	if (packet->m_nCurrentPass > 0)
		return -1;

	CShader::BindProgram(gs_InsertGeometryPID);

	CDeviceManager::BindStorageBuffer(CVolumetricLightMapsManager::ms_pCurrentMap->m_nVolumetricLightMapID, 0, sizeof(CVolumetricLightMapsManager::SLightMapNode), 0, 2 * 1024 * 1024);
	CDeviceManager::BindStorageBuffer(gs_GlobalBuffer, 1, sizeof(unsigned int), 0, 1);

	CDeviceManager::BindConstantBuffer(gs_InsertGeometryConstants, gs_InsertGeometryPID, 0);

	CRenderStates::SetWriteMask(e_Red);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetRasterizerState(e_Solid);

	return 1;
}



void CVolumetricLightMap::DisplayProbes(SRenderTarget* pTarget)
{
	CVolumetricLightMapsManager::ms_pFBO->SetActive();
	CVolumetricLightMapsManager::ms_pFBO->BindRenderTarget(0, pTarget);

	CShader::BindProgram(gs_DisplayProbesPID);

	float4x4 pregisters = *CRenderer::GetCurrentCamera()->GetViewProjMatrix();
	pregisters.transpose();

	CDeviceManager::FillConstantBuffer(gs_DisplayProbesConstants, &pregisters, sizeof(float4x4));
	CDeviceManager::BindConstantBuffer(gs_DisplayProbesConstants, gs_DisplayProbesPID, 0);

	CStream::SetMultiStreams(m_nSHBufferID, 4U, 1U);

	CRenderStates::SetWriteMask(e_Red | e_Green | e_Blue | e_Alpha);
	CRenderStates::SetCullMode(e_CullNone);
	CRenderStates::SetRasterizerState(e_Solid);
	CRenderStates::SetBlendingState(e_Opaque);

	CDeviceManager::Draw(e_PointList, 0, 1);
}