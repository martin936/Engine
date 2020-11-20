#include "DeviceManager.h"
#include "RenderPass.h"
#include "Engine/Renderer/Renderer.h"
#include <stdarg.h>


CRenderPass*												CRenderPass::ms_pCurrent = nullptr;
CRenderPass*												CRenderPass::ms_pCurrentSubPass = nullptr;
CPipelineManager::SPipeline*								CRenderPass::ms_pCurrentPipeline = nullptr;
std::vector<CRenderPass*>									CRenderPass::ms_pRenderPasses;
std::vector<CRenderPass*>									CRenderPass::ms_pSortedRenderPasses;
std::vector<CRenderPass*>									CRenderPass::ms_pLoadingRenderPasses;

thread_local CRenderPass*									CFrameBlueprint::ms_pCurrentRenderPass = nullptr;

std::vector<std::vector<CFrameBlueprint::SResourceBarrier>>	CFrameBlueprint::ms_EventList;
std::vector<std::vector<CFrameBlueprint::SResourceBarrier>>	CFrameBlueprint::ms_BarrierCache;

std::vector<CFrameBlueprint::SResourceUsage>				CFrameBlueprint::ms_ResourceUsage;
std::vector<CFrameBlueprint::SResourceBarrier>				CFrameBlueprint::ms_TransitionsToFirstState;

unsigned int												CFrameBlueprint::ms_nNextPassSortedID = 0;
bool														CFrameBlueprint::ms_bIsSorting = false;

extern bool g_bIsFirstFrame;


CRenderPass::CRenderPass(const char* pcName, CPipelineManager::EPipelineType eType, bool bLoading)
{
	if (pcName)
		strcpy_s<512>(m_cName, pcName);
	else
		m_cName[0] = '\0';

	m_nReadResourceID.clear();
	m_nWritenResourceID.clear();
	m_nPipelineStateID = CPipelineManager::NewPipeline(eType);
	m_pParentPass = nullptr;
	m_SubPasses.clear();

	m_nDepthStencilID = INVALIDHANDLE;
	m_nDepthStencilSlice = -1;
	m_nDepthStencilLevel = -1;

	m_bLoadingPass = bLoading;
	m_nSortedID = INVALIDHANDLE;
	m_bEnableMemoryBarriers = false;
	m_bIsGraphicsRenderPassRunning = false;

	if (bLoading)
	{
		m_nID = static_cast<unsigned int>(ms_pLoadingRenderPasses.size());
		ms_pLoadingRenderPasses.push_back(this);
	}
	else
	{
		m_nID = static_cast<unsigned int>(ms_pRenderPasses.size());
		ms_pRenderPasses.push_back(this);
	}
}


void CRenderPass::Reset()
{
	unsigned numRenderPasses = static_cast<unsigned>(ms_pRenderPasses.size());

	for (unsigned i = 0; i < numRenderPasses; i++)
	{
		//if (!ms_pRenderPasses[i]->m_pParentPass)
		delete ms_pRenderPasses[i];

		ms_pRenderPasses[i] = nullptr;
	}

	ms_pRenderPasses.clear();
	ms_pSortedRenderPasses.clear();
}


void CRenderPass::CopyFrom(const char* pcName)
{
	ASSERT(ms_pCurrent != nullptr);

	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	CRenderPass* pSrc = GetRenderPass(pcName);

	ASSERT(pSrc != nullptr);

	pCurrent->m_nReadResourceID = pSrc->m_nReadResourceID;
	pCurrent->m_nWritenResourceID = pSrc->m_nWritenResourceID;
	pCurrent->m_nDepthStencilID = pSrc->m_nDepthStencilID;
	pCurrent->m_nDepthStencilSlice = pSrc->m_nDepthStencilSlice;
	pCurrent->m_nDepthStencilLevel = pSrc->m_nDepthStencilLevel;

	CPipelineManager::SPipeline* pPipeline = CPipelineManager::GetPipelineState(pCurrent->m_nPipelineStateID);

	pPipeline->CopyFrom(CPipelineManager::GetPipelineState(pSrc->GetPipeline()));
}



bool CRenderPass::BeginGraphics(const char* pcName, bool bLoading)
{
	ASSERT(ms_pCurrent == nullptr && "End hasn't been called");

	CRenderer::SetVertexLayout(e_Vertex_Layout_Engine);

	if ((bLoading && GetLoadingRenderPassID(pcName) != INVALIDHANDLE) || (!bLoading && GetRenderPassID(pcName) != INVALIDHANDLE))
		return false;

	ms_pCurrent = new CRenderPass(pcName, CPipelineManager::e_GraphicsPipeline, bLoading);
	ms_pCurrentSubPass = nullptr;

	ms_pCurrentPipeline = CPipelineManager::GetPipelineState(ms_pCurrent->m_nPipelineStateID);

	return true;
}


bool CRenderPass::BeginCompute(const char* pcName)
{
	ASSERT(ms_pCurrent == nullptr && "End hasn't been called");

	if (GetRenderPassID(pcName) != INVALIDHANDLE)
		return false;

	ms_pCurrent = new CRenderPass(pcName, CPipelineManager::e_ComputePipeline);
	ms_pCurrentSubPass = nullptr;

	ms_pCurrentPipeline = CPipelineManager::GetPipelineState(ms_pCurrent->m_nPipelineStateID);

	return true;
}


void CRenderPass::End()
{
	ASSERT(ms_pCurrent != nullptr && "Begin hasn't been called");

	if (ms_pCurrent->m_SubPasses.size() > 0 && ms_pCurrentSubPass == nullptr)
	{
		ms_pCurrent = nullptr;
		return;
	}

	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	CPipelineManager::SPipeline* pPipeline = CPipelineManager::GetPipelineState(pCurrent->m_nPipelineStateID);

	if (pPipeline != nullptr)
	{
		if (pPipeline->m_eType == CPipelineManager::e_GraphicsPipeline)
		{
			pCurrent->CreateFramebuffer();

			size_t nNumResources = pCurrent->m_nReadResourceID.size();

			for (size_t i = 0; i < nNumResources; i++)
			{
				if (pCurrent->m_nReadResourceID[i].m_eType == e_Texture)
					pPipeline->SetNumTextures(pCurrent->m_nReadResourceID[i].m_nSlot, 1);

				else if (pCurrent->m_nReadResourceID[i].m_eType == e_Buffer)
					pPipeline->SetNumBuffers(pCurrent->m_nReadResourceID[i].m_nSlot, 1);
			}

			nNumResources = pCurrent->m_nWritenResourceID.size();

			for (size_t i = 0; i < nNumResources; i++)
			{
				if (pCurrent->m_nWritenResourceID[i].m_eType == e_RenderTarget)
				{
					ETextureFormat format = ETextureFormat::e_R8G8B8A8;
					unsigned int nSampleCount = 1;
					unsigned int nSampleQuality = 0;

					if (pCurrent->m_nWritenResourceID[i].m_nResourceID != INVALIDHANDLE)
					{
						format			= CTextureInterface::GetTextureFormat(pCurrent->m_nWritenResourceID[i].m_nResourceID);
						nSampleCount	= CTextureInterface::GetTextureSampleCount(pCurrent->m_nWritenResourceID[i].m_nResourceID);
						nSampleQuality	= CTextureInterface::GetTextureSampleQuality(pCurrent->m_nWritenResourceID[i].m_nResourceID);
					}

					pPipeline->SetRenderTargetFormat(pCurrent->m_nWritenResourceID[i].m_nSlot, format);
					pPipeline->SetSampleDesc(nSampleCount, nSampleQuality);
				}

				else if (pCurrent->m_nWritenResourceID[i].m_eType == e_UnorderedAccess)
				{
					if (pCurrent->m_nWritenResourceID[i].m_eResourceType == e_Texture)
						pPipeline->SetNumRWTextures(pCurrent->m_nWritenResourceID[i].m_nSlot, 1);

					else if(pCurrent->m_nWritenResourceID[i].m_eResourceType == e_Buffer)
						pPipeline->SetNumRWBuffers(pCurrent->m_nWritenResourceID[i].m_nSlot, 1);
				}
			}

			if (pCurrent->m_nDepthStencilID != INVALIDHANDLE)
			{
				unsigned int nSampleCount	= CTextureInterface::GetTextureSampleCount(pCurrent->m_nDepthStencilID);
				unsigned int nSampleQuality = CTextureInterface::GetTextureSampleQuality(pCurrent->m_nDepthStencilID);

				ETextureFormat format = CTextureInterface::GetTextureFormat(pCurrent->m_nDepthStencilID);
				pPipeline->SetDepthStencilFormat(format);
				pPipeline->SetSampleDesc(nSampleCount, nSampleQuality);
			}
		}

		else
		{
			size_t nNumResources = pCurrent->m_nReadResourceID.size();

			for (size_t i = 0; i < nNumResources; i++)
			{
				if (pCurrent->m_nReadResourceID[i].m_eType == e_Texture)
					pPipeline->SetNumTextures(pCurrent->m_nReadResourceID[i].m_nSlot, 1);

				else if (pCurrent->m_nReadResourceID[i].m_eType == e_Buffer)
					pPipeline->SetNumBuffers(pCurrent->m_nReadResourceID[i].m_nSlot, 1);
			}

			nNumResources = pCurrent->m_nWritenResourceID.size();

			for (size_t i = 0; i < nNumResources; i++)
			{
				if (pCurrent->m_nWritenResourceID[i].m_eType == e_UnorderedAccess)
				{
					if (pCurrent->m_nWritenResourceID[i].m_eResourceType == e_Texture)
						pPipeline->SetNumRWTextures(pCurrent->m_nWritenResourceID[i].m_nSlot, 1);

					else if (pCurrent->m_nWritenResourceID[i].m_eResourceType == e_Buffer)
						pPipeline->SetNumRWBuffers(pCurrent->m_nWritenResourceID[i].m_nSlot, 1);
				}
			}
		}

		pPipeline->Create();
	}

	if (ms_pCurrentSubPass)
		ms_pCurrentSubPass = nullptr;

	else
		ms_pCurrent = nullptr;

	ms_pCurrentPipeline = nullptr;
}

bool CRenderPass::BeginGraphicsSubPass()
{
	ASSERT(ms_pCurrent != nullptr && "Begin hasn't been called");
	ASSERT(ms_pCurrentSubPass == nullptr && "EndSubPass hasn't been called");

	ms_pCurrentSubPass = new CRenderPass(nullptr, CPipelineManager::e_GraphicsPipeline);
	ms_pCurrentSubPass->m_pParentPass = ms_pCurrent;

	ms_pCurrent->m_SubPasses.push_back(ms_pCurrentSubPass);

	ms_pCurrentPipeline = CPipelineManager::GetPipelineState(ms_pCurrentSubPass->m_nPipelineStateID);

	return true;
}

bool CRenderPass::BeginComputeSubPass()
{
	ASSERT(ms_pCurrent != nullptr && "Begin hasn't been called");
	ASSERT(ms_pCurrentSubPass == nullptr && "EndSubPass hasn't been called");

	ms_pCurrentSubPass = new CRenderPass(nullptr, CPipelineManager::e_ComputePipeline);
	ms_pCurrentSubPass->m_pParentPass = ms_pCurrent;

	ms_pCurrent->m_SubPasses.push_back(ms_pCurrentSubPass);

	ms_pCurrentPipeline = CPipelineManager::GetPipelineState(ms_pCurrentSubPass->m_nPipelineStateID);

	return true;
}


void CRenderPass::EndSubPass()
{
	ASSERT(ms_pCurrent != nullptr && "Begin hasn't been called");
	ASSERT(ms_pCurrentSubPass != nullptr && "BeginSubPass hasn't been called");

	End();
}


void CRenderPass::SetEmptyPipeline()
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	pCurrent->m_nPipelineStateID = INVALIDHANDLE;
}


void CRenderPass::BindResourceToRead(unsigned int nSlot, unsigned int nResourceID, CShader::EShaderType nShaderStages, EResourceType eType)
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	pCurrent->m_nReadResourceID.push_back({ nSlot, nResourceID, eType, -1, -1, nShaderStages });
}


void CRenderPass::BindResourceToRead(unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, CShader::EShaderType nShaderStages, EResourceType eType)
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	pCurrent->m_nReadResourceID.push_back({ nSlot, nResourceID, eType, nSlice, nLevel, nShaderStages });
}


void CRenderPass::BindResourceToWrite(unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType, EResourceType eResourceType)
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	if (nResourceID != INVALIDHANDLE)
		pCurrent->m_nWritenResourceID.push_back({ nSlot, nResourceID, eResourceType, -1, -1, eType });

	else
	{
		CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pCurrent->m_nPipelineStateID);
		pipeline->SetRenderTargetFormat(nSlot, ETextureFormat::e_R8G8B8A8_SRGB);
	}
}



void CRenderPass::BindResourceToWrite(unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, EResourceAccessType eType, EResourceType eResourceType)
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	if (nResourceID != INVALIDHANDLE)
		pCurrent->m_nWritenResourceID.push_back({ nSlot, nResourceID, eResourceType, nSlice, nLevel, eType });

	else
	{
		CPipelineManager::SPipeline* pipeline = CPipelineManager::GetPipelineState(pCurrent->m_nPipelineStateID);
		pipeline->SetRenderTargetFormat(nSlot, ETextureFormat::e_R8G8B8A8);
	}
}


void CRenderPass::BindDepthStencil(unsigned int nResourceID, int nSlice, int nLevel)
{
	CRenderPass* pCurrent = ms_pCurrentSubPass == nullptr ? ms_pCurrent : ms_pCurrentSubPass;

	if (nResourceID != INVALIDHANDLE)
		pCurrent->m_nWritenResourceID.push_back({ 0, nResourceID, CRenderPass::e_Texture, nSlice, nLevel, e_DepthStencil_Write });

	pCurrent->m_nDepthStencilID = nResourceID;
	pCurrent->m_nDepthStencilSlice = nSlice;
	pCurrent->m_nDepthStencilLevel = nLevel;
}


void CRenderPass::ChangeResourceToRead(const char* pcName, unsigned int nSlot, unsigned int nResourceID, unsigned int nShaderStages)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStages))
		{
			pass->m_nReadResourceID[i].m_nResourceID = nResourceID;
			pass->m_nReadResourceID[i].m_nSlice = -1;
			pass->m_nReadResourceID[i].m_nLevel = -1;
			break;
		}
	}
}


void CRenderPass::ChangeResourceToRead(const char* pcName, unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, unsigned int nShaderStages)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStages))
		{
			pass->m_nReadResourceID[i].m_nResourceID = nResourceID;
			pass->m_nReadResourceID[i].m_nSlice = nSlice;
			pass->m_nReadResourceID[i].m_nLevel = nLevel;
			break;
		}
	}
}


void CRenderPass::ChangeSubPassResourceToRead(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, unsigned int nShaderStages)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStages))
		{
			pass->m_nReadResourceID[i].m_nResourceID = nResourceID;
			pass->m_nReadResourceID[i].m_nSlice = -1;
			pass->m_nReadResourceID[i].m_nLevel = -1;
			break;
		}
	}
}


void CRenderPass::ChangeSubPassResourceToRead(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, unsigned int nShaderStages)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStages))
		{
			pass->m_nReadResourceID[i].m_nResourceID = nResourceID;
			pass->m_nReadResourceID[i].m_nSlice = nSlice;
			pass->m_nReadResourceID[i].m_nLevel = nLevel;
			break;
		}
	}
}


void CRenderPass::ChangeSubPassResourceToWrite(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			pass->m_nWritenResourceID[i].m_nResourceID = nResourceID;
			pass->m_nWritenResourceID[i].m_nSlice = -1;
			pass->m_nWritenResourceID[i].m_nLevel = -1;
			break;
		}
	}
}


void CRenderPass::ChangeSubPassResourceToWrite(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, EResourceAccessType eType)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			pass->m_nWritenResourceID[i].m_nResourceID = nResourceID;
			pass->m_nWritenResourceID[i].m_nSlice = nSlice;
			pass->m_nWritenResourceID[i].m_nLevel = nLevel;
			break;
		}
	}
}


void CRenderPass::ChangeResourceToWrite(const char* pcName, unsigned int nSlot, unsigned int nResourceID, EResourceAccessType eType)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			pass->m_nWritenResourceID[i].m_nResourceID = nResourceID;
			pass->m_nWritenResourceID[i].m_nSlice = -1;
			pass->m_nWritenResourceID[i].m_nLevel = -1;
			break;
		}
	}
}


void CRenderPass::ChangeResourceToWrite(const char* pcName, unsigned int nSlot, unsigned int nResourceID, int nSlice, int nLevel, EResourceAccessType eType)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			pass->m_nWritenResourceID[i].m_nResourceID = nResourceID;
			pass->m_nWritenResourceID[i].m_nSlice = nSlice;
			pass->m_nWritenResourceID[i].m_nLevel = nLevel;
			break;
		}
	}
}


void CRenderPass::ChangeDepthStencil(const char* pcName, unsigned int nResourceID)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass->m_nDepthStencilID = nResourceID;
	pass->m_nDepthStencilSlice = -1;
	pass->m_nDepthStencilLevel = -1;
}


void CRenderPass::ChangeSubPassDepthStencil(const char* pcName, unsigned int nSubPassID, unsigned int nResourceID)
{
#ifndef EKOPLF_PS5_DEFINE
	if (!CFrameBlueprint::IsSorting())
		return;
#endif

	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	pass->m_nDepthStencilID = nResourceID;
	pass->m_nDepthStencilSlice = -1;
	pass->m_nDepthStencilLevel = -1;
}


unsigned int CRenderPass::GetReadResourceID(const char* pcName, unsigned int nSlot, unsigned int nShaderStage)
{
	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStage))
		{
			return pass->m_nReadResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetSubPassReadResourceID(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, unsigned int nShaderStage)
{
	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStage))
		{
			return pass->m_nReadResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetReadResourceID(unsigned int nSlot, unsigned int nShaderStage)
{
	CRenderPass* pass = CFrameBlueprint::ms_pCurrentRenderPass;

	unsigned numResourcesToRead = static_cast<unsigned>(pass->m_nReadResourceID.size());

	for (unsigned i = 0; i < numResourcesToRead; i++)
	{
		if ((pass->m_nReadResourceID[i].m_nSlot == nSlot) && (pass->m_nReadResourceID[i].m_nShaderStages & nShaderStage))
		{
			return pass->m_nReadResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetWrittenResourceID(const char* pcName, unsigned int nSlot, EResourceAccessType eType)
{
	CRenderPass* pass = GetRenderPass(pcName);

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			return pass->m_nWritenResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetSubPassWrittenResourceID(const char* pcName, unsigned int nSubPassID, unsigned int nSlot, EResourceAccessType eType)
{
	CRenderPass* pass = GetRenderPass(pcName);

	pass = pass->m_SubPasses[nSubPassID];

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			return pass->m_nWritenResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetWrittenResourceID(unsigned int nSlot, EResourceAccessType eType)
{
	CRenderPass* pass = CFrameBlueprint::ms_pCurrentRenderPass;

	unsigned numResourcesToWrite = static_cast<unsigned>(pass->m_nWritenResourceID.size());

	for (unsigned i = 0; i < numResourcesToWrite; i++)
	{
		if ((pass->m_nWritenResourceID[i].m_nSlot == nSlot) && (pass->m_nWritenResourceID[i].m_eType == eType))
		{
			return pass->m_nWritenResourceID[i].m_nResourceID;
		}
	}

	return INVALIDHANDLE;
}


unsigned int CRenderPass::GetSubPassMask(int numSubPasses, ...)
{
	va_list vl;
	va_start(vl, numSubPasses);

	unsigned int mask = 0;
	int lastIndex = 0;

	for (int i = 0; i < numSubPasses && lastIndex >= 0; i++)
	{
		int index = va_arg(vl, int);

		if (index < 0)
			mask |= 0xffffffff - ((1 << lastIndex) - 1);

		else
			mask |= (1 << index);

		lastIndex = index;
	}

	va_end(vl);

	return mask;
}


void CRenderPass::Run(unsigned int nCommandListID, void* pData, unsigned int subPassMask)
{
	unsigned numSubPasses = static_cast<unsigned>(m_SubPasses.size());

	if (numSubPasses > 0)
	{
		for (unsigned i = 0; i < numSubPasses; i++)
			if (subPassMask & (1 << i))
				m_SubPasses[i]->Run(nCommandListID, pData);
	}

	else
	{
		CPipelineManager::BindPipeline(nCommandListID, m_nPipelineStateID);

		if (!m_bLoadingPass)
		{
			if (ms_pSortedRenderPasses.size() > 0 && this == ms_pSortedRenderPasses[0])
			{
				CFrameBlueprint::TransitionResourcesToFirstState(m_nSortedID);
				CDeviceManager::PrepareToDraw();
			}

			CFrameBlueprint::PrepareForRenderPass(this);
		}

		m_pEntryPoint();

		if (!m_bLoadingPass)
		{
			CFrameBlueprint::FinishRenderPass(this);

			if (ms_pSortedRenderPasses.size() > 0 && this == ms_pSortedRenderPasses.back())
				CDeviceManager::PrepareToFlip();
		}
	}
}


unsigned int CRenderPass::GetRenderPassID(const char* pcName)
{
	unsigned int numPasses = static_cast<unsigned int>(ms_pRenderPasses.size());

	for (unsigned int i = 0; i < numPasses; i++)
		if (!strcmp(ms_pRenderPasses[i]->m_cName, pcName))
			return ms_pRenderPasses[i]->m_nID;

	return 0xffffffff;
}


unsigned int CRenderPass::GetLoadingRenderPassID(const char* pcName)
{
	std::vector<CRenderPass*>::iterator it;

	for (it = ms_pLoadingRenderPasses.begin(); it < ms_pLoadingRenderPasses.end(); it++)
		if (!strcmp((*it)->m_cName, pcName))
			return (*it)->m_nID;

	return 0xffffffff;
}



void CFrameBlueprint::Init()
{
	ms_EventList.clear();
	ms_ResourceUsage.clear();
}



void CFrameBlueprint::Terminate()
{
	int numRenderPass = static_cast<int>(CRenderPass::ms_pRenderPasses.size());

	for (int i = 0; i < numRenderPass; i++)
		delete CRenderPass::ms_pRenderPasses[i];

	CRenderPass::ms_pRenderPasses.clear();
}


unsigned int CFrameBlueprint::GetCurrentSubPass()
{
	if (ms_pCurrentRenderPass->m_pParentPass != nullptr)
	{
		CRenderPass* parent = ms_pCurrentRenderPass->m_pParentPass;

		unsigned int numSubPasses = static_cast<unsigned int>(parent->m_SubPasses.size());

		for (unsigned int i = 0; i < numSubPasses; i++)
			if (parent->m_SubPasses[i] == ms_pCurrentRenderPass)
				return i;
	}

	return INVALIDHANDLE;
}


bool CFrameBlueprint::IsCurrentRenderPass(const char* pcName)
{
	ASSERT(ms_pCurrentRenderPass != nullptr);

	CRenderPass* pass = ms_pCurrentRenderPass;

	while (pass->m_pParentPass != nullptr)
		pass = pass->m_pParentPass;

	return !strcmp(pcName, pass->m_cName);
}


void CFrameBlueprint::PrepareForRenderPass(CRenderPass* renderPass)
{
	ms_pCurrentRenderPass = renderPass;

	if (ms_EventList.size() > 0)
	{
		unsigned numBarriers = static_cast<unsigned>(ms_EventList[renderPass->m_nSortedID].size());

		for (unsigned i = 0; i < numBarriers; i++)
		{
			if (ms_EventList[renderPass->m_nSortedID][i].m_bExecuteBeforeDrawCall)
				ExecuteBarrier(renderPass->m_nSortedID, i);
		}

		FlushBarriers(renderPass->m_nSortedID);
	}

	if (ms_pCurrentRenderPass->m_nPipelineStateID == INVALIDHANDLE || CPipelineManager::GetPipelineType(ms_pCurrentRenderPass->m_nPipelineStateID) == CPipelineManager::e_GraphicsPipeline)
	{
		unsigned numWrittenResources = static_cast<unsigned>(ms_pCurrentRenderPass->m_nWritenResourceID.size());

		if (numWrittenResources == 0)
			CDeviceManager::SetViewport(0, 0, CDeviceManager::GetDeviceWidth(), CDeviceManager::GetDeviceHeight());

		else
			CDeviceManager::SetViewport(0, 0, CTextureInterface::GetTextureWidth(ms_pCurrentRenderPass->m_nWritenResourceID[0].m_nResourceID, ms_pCurrentRenderPass->m_nWritenResourceID[0].m_nLevel), CTextureInterface::GetTextureHeight(ms_pCurrentRenderPass->m_nWritenResourceID[0].m_nResourceID, ms_pCurrentRenderPass->m_nWritenResourceID[0].m_nLevel));

		for (unsigned i = 0; i < numWrittenResources; i++)
		{
			CRenderPass::SWriteResource res = ms_pCurrentRenderPass->m_nWritenResourceID[i];

			if (res.m_eType == CRenderPass::e_UnorderedAccess)
			{
				if (res.m_eResourceType == CRenderPass::e_Texture)
					CTextureInterface::SetRWTexture(res.m_nResourceID, res.m_nSlot);

				else if (res.m_eResourceType == CRenderPass::e_Buffer)
					CResourceManager::SetRwBuffer(res.m_nSlot, res.m_nResourceID);
			}
				
		}

		unsigned numReadResources = static_cast<unsigned>(ms_pCurrentRenderPass->m_nReadResourceID.size());

		for (unsigned i = 0; i < numReadResources; i++)
		{
			CRenderPass::SReadOnlyResource res = ms_pCurrentRenderPass->m_nReadResourceID[i];

			if (res.m_eType == CRenderPass::e_Texture)
				CTextureInterface::SetTexture(res.m_nResourceID, res.m_nSlot, (CShader::EShaderType)res.m_nShaderStages, res.m_nSlice, res.m_nLevel);

			else if (res.m_eType == CRenderPass::e_Buffer)
				CResourceManager::SetBuffer(res.m_nSlot, res.m_nResourceID);
		}
	}

	else
	{
		unsigned numWrittenResources = static_cast<unsigned>(ms_pCurrentRenderPass->m_nWritenResourceID.size());

		for (unsigned i = 0; i < numWrittenResources; i++)
		{
			CRenderPass::SWriteResource res = ms_pCurrentRenderPass->m_nWritenResourceID[i];

			if (res.m_eType == CRenderPass::e_UnorderedAccess)
			{
				if (res.m_eResourceType == CRenderPass::e_Texture)
					CTextureInterface::SetRWTexture(res.m_nResourceID, res.m_nSlot);

				else if (res.m_eResourceType == CRenderPass::e_Buffer)
					CResourceManager::SetRwBuffer(res.m_nSlot, res.m_nResourceID);
			}
		}

		unsigned numReadResources = static_cast<unsigned>(ms_pCurrentRenderPass->m_nReadResourceID.size());

		for (unsigned i = 0; i < numReadResources; i++)
		{
			CRenderPass::SReadOnlyResource res = ms_pCurrentRenderPass->m_nReadResourceID[i];

			if (res.m_eType == CRenderPass::e_Texture)
				CTextureInterface::SetTexture(res.m_nResourceID, res.m_nSlot, (CShader::EShaderType)res.m_nShaderStages, res.m_nSlice, res.m_nLevel);

			else if (res.m_eType == CRenderPass::e_Buffer)
				CResourceManager::SetBuffer(res.m_nSlot, res.m_nResourceID);
		}
	}

	if (CPipelineManager::GetPipelineType(ms_pCurrentRenderPass->m_nPipelineStateID) == CPipelineManager::e_GraphicsPipeline)
		renderPass->BeginRenderPass();
}


void CFrameBlueprint::FinishRenderPass(CRenderPass* renderPass)
{
	if (CPipelineManager::GetPipelineType(ms_pCurrentRenderPass->m_nPipelineStateID) == CPipelineManager::e_GraphicsPipeline)
		renderPass->EndRenderPass();

	if (ms_EventList.size() > 0)
	{
		unsigned numBarriers = static_cast<unsigned>(ms_EventList[renderPass->m_nSortedID].size());

		for (unsigned i = 0; i < numBarriers; i++)
		{
			if (!ms_EventList[renderPass->m_nSortedID][i].m_bExecuteBeforeDrawCall)
				ExecuteBarrier(renderPass->m_nSortedID, i);
		}

		FlushBarriers(renderPass->m_nSortedID);
	}
}


void CFrameBlueprint::EndFrame()
{
	if (ms_EventList.size() > 0)
	{
		unsigned numEvents = static_cast<unsigned>(ms_EventList.back().size());

		for (unsigned i = 0; i < numEvents; i++)
		{
			ExecuteBarrier(INVALIDHANDLE, i);
		}

		FlushBarriers(INVALIDHANDLE);
	}
}


void CFrameBlueprint::TransitionResourcesToFirstState(unsigned int renderpass)
{
	unsigned numBarriers = static_cast<unsigned>(ms_TransitionsToFirstState.size());

	if (numBarriers > 0)
	{
		for (unsigned i = 0; i < numBarriers; i++)
			ms_BarrierCache[renderpass].push_back(ms_TransitionsToFirstState[i]);

		FlushBarriers(renderpass);

		ms_TransitionsToFirstState.clear();
	}
}


void CFrameBlueprint::ExecuteBarrier(unsigned int nRenderPassID, unsigned int nEventID)
{
	unsigned nID = nRenderPassID == INVALIDHANDLE ? static_cast<unsigned>(ms_BarrierCache.size() - 1) : nRenderPassID;

	ms_BarrierCache[nID].push_back(ms_EventList[nID][nEventID]);
}

void CFrameBlueprint::UnorderedAccessBarrier(unsigned int nResourceID, EBarrierFlags eFlags, CRenderPass::EResourceType eType)
{
	if (ms_BarrierCache.size() > 0)
	{
		unsigned int nID = ms_pCurrentRenderPass->m_nSortedID;

		SResourceBarrier barrier;
		barrier.m_eType = e_Barrier_UAV;
		barrier.m_eResourceType = eType;
		barrier.m_eFlags = eFlags;
		barrier.m_UAV.m_nResourceID = nResourceID;

		ms_BarrierCache[nID].push_back(barrier);
	}
}


void CFrameBlueprint::InsertMemoryBarrier()
{
	if (ms_BarrierCache.size() > 0)
	{
		unsigned int nID = ms_pCurrentRenderPass->m_nSortedID;

		SResourceBarrier barrier;
		barrier.m_eType = e_Barrier_Memory;

		ms_BarrierCache[nID].push_back(barrier);
	}
}


void CFrameBlueprint::FlushBarriers()
{
	FlushBarriers(ms_pCurrentRenderPass->m_nSortedID);
}

unsigned int CFrameBlueprint::GetResourceIndex(unsigned int nResourceID, CRenderPass::EResourceType eType)
{
	unsigned numResources = static_cast<unsigned>(ms_ResourceUsage.size());

	for (unsigned i = 0; i < numResources; i++)
	{
		if (ms_ResourceUsage[i].m_nResourceID == nResourceID && ms_ResourceUsage[i].m_eType == eType)
			return i;
	}

	SResourceUsage usage;
	usage.m_nResourceID = nResourceID;
	usage.m_eType = eType;

	ms_ResourceUsage.push_back(usage);

	return static_cast<unsigned int>(ms_ResourceUsage.size() - 1);
}


void CFrameBlueprint::SetNextRenderPass(SRenderPassTask renderPass)
{
	if (renderPass.m_pRenderPass->m_SubPasses.size() > 0)
	{
		int numSubPasses = static_cast<int>(renderPass.m_pRenderPass->m_SubPasses.size());

		for (int i = 0; i < numSubPasses; i++)
			if (renderPass.m_nSubPassMask & (1 << i))
				SetNextRenderPass({ renderPass.m_pRenderPass->m_SubPasses[i], 0xffffffff });
	}

	else
	{
		renderPass.m_pRenderPass->m_nSortedID = ms_nNextPassSortedID;
		ms_nNextPassSortedID++;
	}
}

void CFrameBlueprint::PrepareForSort()
{
	ms_bIsSorting = true;
	ms_nNextPassSortedID = 0;

	unsigned numPasses = static_cast<unsigned>(CRenderPass::ms_pSortedRenderPasses.size());

	for (unsigned i = 0; i < numPasses; i++)
		CRenderPass::ms_pSortedRenderPasses[i]->m_nSortedID = INVALIDHANDLE;

	CRenderPass::ms_pSortedRenderPasses.clear();
}


void CFrameBlueprint::SortRenderPasses()
{
	CRenderPass::ms_pSortedRenderPasses.clear();
	CRenderPass::ms_pSortedRenderPasses.resize(CRenderPass::ms_pRenderPasses.size());

	ms_bIsSorting = false;

	unsigned numPasses = static_cast<unsigned>(CRenderPass::ms_pRenderPasses.size());
	unsigned numUsedPasses = 0;

	for (unsigned i = 0; i < numPasses; i++)
	{
		if (CRenderPass::ms_pRenderPasses[i]->m_nSortedID != INVALIDHANDLE)
		{
			CRenderPass::ms_pSortedRenderPasses[CRenderPass::ms_pRenderPasses[i]->m_nSortedID] = CRenderPass::ms_pRenderPasses[i];
			numUsedPasses++;
		}
	}

	CRenderPass::ms_pSortedRenderPasses.resize(numUsedPasses);
}


