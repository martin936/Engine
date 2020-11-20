#include "Engine/Engine.h"
#include "Engine/Maths/Maths.h"
#include "Engine/Renderer/Text/Text.h"
#include "Engine/Imgui/imgui.h"
#include "Timer.h"

std::vector<CTimer*> CTimerManager::m_pTimers;


CTimer::CTimer(const char* pName)
{
	sprintf_s(m_cName, "%s", pName);

	m_bUpToDate = true;
	m_bEnabled = false;
	m_fValue = 0.f;
}


void CGPUTimer::Draw()
{
	ImGui::TextColored(ImVec4(0.5f, 1.f, 0.5f, 1.f), "%s : %.5f ms (%.1f fps)", m_cName, GetTime(), 1000.f / GetTime());
}


CCPUTimer::CCPUTimer(const char* pName) : CTimer(pName)
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_nClockFrequency);
#endif
}



void CCPUTimer::Start()
{
#ifdef _WIN32
	QueryPerformanceCounter(&m_nStartTime);
#endif

	m_bUpToDate = false;
	m_bEnabled = true;
}



void CCPUTimer::Stop()
{
#ifdef _WIN32
	QueryPerformanceCounter(&m_nStopTime);

	m_fValue = 1000.f * (m_nStopTime.QuadPart - m_nStartTime.QuadPart) / m_nClockFrequency.QuadPart;
#endif

	m_bUpToDate = true;
}


void CCPUTimer::Draw()
{
	ImGui::TextColored(ImVec4(1.f, 0.5f, 0.f, 1.f), "%s : %.5f ms (%.1f fps)", m_cName, GetTime(), 1000.f / GetTime());
}



void CTimerManager::AddGPUTimer(const char* pName)
{
	CGPUTimer* pTimer = new CGPUTimer(pName);

	m_pTimers.push_back(pTimer);
}


void CTimerManager::AddCPUTimer(const char* pName)
{
	CCPUTimer* pTimer = new CCPUTimer(pName);

	m_pTimers.push_back(pTimer);
}


void CTimerManager::RefreshTimers()
{
	PrepareNextGPUFrame();

	std::vector<CTimer*>::iterator it;

	for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
		(*it)->Refresh();
}


CTimer* CTimerManager::GetGPUTimer(const char* pName)
{
	ms_Lock->Take();

	CTimer* pTimer;

	std::vector<CTimer*>::iterator it;

	for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
		if (strcmp((*it)->m_cName, pName) == 0)
		{
			pTimer = *it;
			ms_Lock->Release();

			return pTimer;
		}

	AddGPUTimer(pName);

	pTimer = m_pTimers.back();

	ms_Lock->Release();

	return pTimer;
}


CTimer* CTimerManager::GetCPUTimer(const char* pName)
{
	ms_Lock->Take();

	CTimer* pTimer;

	std::vector<CTimer*>::iterator it;

	for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
		if (strcmp((*it)->m_cName, pName) == 0)
		{
			pTimer = *it;
			ms_Lock->Release();

			return pTimer;
		}

	AddCPUTimer(pName);

	pTimer = m_pTimers.back();

	ms_Lock->Release();

	return pTimer;
}


void CTimerManager::PrintTimers()
{
	std::vector<CTimer*>::iterator it;

	for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
	{
		if (!(*it)->m_bEnabled)
			continue;

		(*it)->Draw();
	}
}