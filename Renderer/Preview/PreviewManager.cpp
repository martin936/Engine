#include "Engine/Renderer/Renderer.h"
#include "PreviewManager.h"


CPreviewManager* CPreviewManager::ms_pCurrent = NULL;



CPreviewManager::CPreviewManager()
{
	
}



void CPreviewManager::Load()
{
	
}


void CPreviewManager::Load(const char* cScenePath)
{

}


void CPreviewManager::SetIBL(const char* cEnvironmentMapPath, const char* cIrradianceMapPath)
{

}


void CPreviewManager::Unload()
{
	if (m_bIsMeshLoaded)
	{
		std::vector<PacketList*>::iterator it;

		for (it = m_pPacketList.begin(); it < m_pPacketList.end(); it++)
			delete *it;

		m_bIsMeshLoaded = false;
	}

	if (m_bIsIBLLoaded)
	{
		delete m_pEnvironmentMap;
		delete m_pIrradianceMap;
		m_bIsIBLLoaded = false;
	}
}


CPreviewManager::~CPreviewManager()
{
	Unload();

	ms_pCurrent = NULL;
}


void CPreviewManager::Run()
{
	
}