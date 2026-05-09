#include "Engine/Device/DeviceManager.h"
#include "Engine/Device/CommandListManager.h"
#include "Engine/Device/PipelineManager.h"
#include "Engine/Misc/Mutex.h"
#include "../Timer.h"


unsigned int	CTimerManager::ms_nMaxNumGPUTimers = 512;
CMutex*			CTimerManager::ms_Lock = nullptr;

unsigned int	gs_NumPools		= 5;
unsigned int	gs_CurrentPool	= 0;

unsigned int	gs_NextQueryID	= 0;
float			gs_TimestampPeriod;
VkQueryPool*	gs_QueryPool = nullptr;


void CTimerManager::Init()
{
	VkQueryPoolCreateInfo createInfo{};
	createInfo.sType		= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.queryType	= VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount	= ms_nMaxNumGPUTimers;

	gs_QueryPool = new VkQueryPool[gs_NumPools];

	for (unsigned int i = 0; i < gs_NumPools; i++)
	{
		VkResult res = vkCreateQueryPool(CDeviceManager::GetDevice(), &createInfo, nullptr, &gs_QueryPool[i]);
		ASSERT(res == VK_SUCCESS);
	}

	ms_Lock = CMutex::Create();

	VkPhysicalDeviceProperties prop;
	vkGetPhysicalDeviceProperties(CDeviceManager::GetPhysicalDevice(), &prop);
	gs_TimestampPeriod = prop.limits.timestampPeriod;

	VkCommandBuffer cmd = reinterpret_cast<VkCommandBuffer>(CCommandListManager::BeginOneTimeCommandList());

	for (unsigned int i = 0; i < gs_NumPools; i++)
		vkCmdResetQueryPool(cmd, gs_QueryPool[i], 0, ms_nMaxNumGPUTimers);

	CCommandListManager::EndOneTimeCommandList(cmd);
}


void CTimerManager::Terminate()
{
	std::vector<CTimer*>::iterator it;

	for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
		delete *it;

	m_pTimers.clear();

	for (unsigned int i = 0; i < gs_NumPools; i++)
		vkDestroyQueryPool(CDeviceManager::GetDevice(), gs_QueryPool[i], nullptr);

	delete ms_Lock;

	delete gs_QueryPool;
}


void CTimerManager::ResetGPUTimers()
{
	VkCommandBuffer cmd = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdResetQueryPool(cmd, gs_QueryPool[gs_CurrentPool], 0, ms_nMaxNumGPUTimers);
}


void CTimerManager::PrepareNextGPUFrame()
{
	gs_CurrentPool = (gs_CurrentPool + 1) % gs_NumPools;
}


CGPUTimer::CGPUTimer(const char* pName) : CTimer(pName)
{
	m_bEnabled4EngineFlush = false;

	m_nQueryID[0] = gs_NextQueryID;
	m_nQueryID[1] = gs_NextQueryID + 1;

	gs_NextQueryID += 2;
}


CGPUTimer::~CGPUTimer()
{

}


void CGPUTimer::Start()
{
	m_bEnabled4EngineFlush = true;

	VkCommandBuffer cmd = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	// TOP_OF_PIPE here, BOTTOM_OF_PIPE on Stop: captures wall-clock pass duration on the GPU.
	// Using BOTTOM_OF_PIPE on both can collapse the delta toward 0 for small passes running
	// while the pipeline is otherwise idle, since both timestamps drain at the same point.
	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, gs_QueryPool[gs_CurrentPool], m_nQueryID[0]);
}


void CGPUTimer::Stop()
{
	VkCommandBuffer cmd = reinterpret_cast<VkCommandBuffer>(CCommandListManager::GetCurrentThreadCommandListPtr());

	vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, gs_QueryPool[gs_CurrentPool], m_nQueryID[1]);

	m_bUpToDate = false;
}


void CGPUTimer::Refresh()
{
	if (m_bUpToDate)
		return;

	// Each result block is { timestamp, availability } = 2 * sizeof(size_t) bytes.
	// The stride between blocks must be at least one full block; passing sizeof(size_t)
	// is invalid per spec and produces driver-dependent garbage.
	size_t results[4] = { 0, 0, 0, 0 };	// [start_ts, start_avail, stop_ts, stop_avail]
	const size_t stride = 2 * sizeof(size_t);

	VkResult res = vkGetQueryPoolResults(CDeviceManager::GetDevice(), gs_QueryPool[gs_CurrentPool], m_nQueryID[0], 2, sizeof(results), results, stride, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

	// Commit a new value only when the stop endpoint is available (matching the original
	// semantics) and the delta is sane (guards against unsigned underflow on stale slots).
	if (res == VK_SUCCESS && results[3] != 0 && results[2] >= results[0])
		m_fValue = (results[2] - results[0]) * gs_TimestampPeriod * 1e-6f;

	m_bEnabled = m_bEnabled4EngineFlush;
	m_bEnabled4EngineFlush = false;
}
