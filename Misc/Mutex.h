#ifndef __MUTEX_H__
#define __MUTEX_H__

class CMutex
{

public:
	static CMutex* Create();

	virtual ~CMutex() {}
	virtual bool Take() = 0;
	virtual bool Release() = 0;
	virtual bool TryTake() { return Take(); }
};

class CScopedLock
{
public:
	CScopedLock(CMutex* p_pMutex) : m_pMutex(p_pMutex) { m_pMutex->Take(); }
	~CScopedLock() { m_pMutex->Release(); }

private:
	CMutex*	m_pMutex;
};


#endif
