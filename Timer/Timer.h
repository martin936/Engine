#ifndef __TIMER_H__
#define __TIMER_H__

#include <vector>
#include "Engine/Misc/Mutex.h"

class CTimer
{
	friend class CTimerManager;
public:

	CTimer(const char* pName);
	~CTimer() {};

	virtual void Start() {}
	virtual void Stop() {}

	virtual void Refresh() {}

	virtual void Draw() {}

	inline float GetTime() const 
	{
		return m_fValue;
	}

protected:

	float	m_fValue;
	char	m_cName[256];

	bool	m_bUpToDate;
	bool	m_bEnabled;
};


class CGPUTimer : public CTimer
{
public:

	CGPUTimer(const char* pName);
	~CGPUTimer();

	void Start() override;
	void Stop() override;

	void Refresh() override;

	void Draw() override;

private:

	unsigned int m_nQueryID[2];

	bool	m_bEnabled4EngineFlush;

#ifdef __OPENGL__
	GLuint64	m_nStartTime;
	GLuint64	m_nStopTime;
#endif
};


class CCPUTimer : public CTimer
{
public:

	CCPUTimer(const char* pName);
	~CCPUTimer() {};

	void Start() override;
	void Stop() override;

	void Draw() override;

private:

	LARGE_INTEGER m_nClockFrequency;
	LARGE_INTEGER m_nStartTime;
	LARGE_INTEGER m_nStopTime;
};


class CTimerManager
{
public:

	static void Init();
	static void Terminate();

	static void RefreshTimers();
	static void PrintTimers();

	static void ResetGPUTimers();

	static CTimer* GetGPUTimer(const char* pName);
	static CTimer* GetCPUTimer(const char* pName);

	inline static void InitFrame()
	{
		std::vector<CTimer*>::iterator it;

		for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
			(*it)->m_bEnabled = false;
	}

	inline static void Clear()
	{
		std::vector<CTimer*>::iterator it;

		for (it = m_pTimers.begin(); it < m_pTimers.end(); it++)
			delete *it;

		m_pTimers.clear();
	}

private:

	static void PrepareNextGPUFrame();

	static unsigned int ms_nMaxNumGPUTimers;

	static void AddGPUTimer(const char* pName);
	static void AddCPUTimer(const char* pName);

	static CMutex* ms_Lock;

	static std::vector<CTimer*> m_pTimers;
};


#endif
