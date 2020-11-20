#include "EventWin.h"

CEvent *CEvent::Create()
{
	return new CEventWin;
}

void CEvent::CreateMultipleEvents(unsigned int nNumEvents, CEvent** pEvents)
{
	for (unsigned int i = 0; i < nNumEvents; i++)
		pEvents[i] = Create();
}

CEventWin::CEventWin()
{
	m_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	Reset();
}

CEventWin :: ~CEventWin()
{
	CloseHandle(m_event);
}


void	CEventWin::Wait()
{
	WaitForSingleObject(m_event, INFINITE);
}


unsigned int CEvent::MultipleWait(unsigned int nNumEvents, CEvent** pEvents)
{
	HANDLE* events = new HANDLE[nNumEvents];

	for (unsigned int i = 0; i < nNumEvents; i++)
		events[i] = static_cast<CEventWin*>(pEvents[i])->m_event;

	DWORD ret = WaitForMultipleObjects(nNumEvents, events, FALSE, INFINITE);

	delete[] events;

	return static_cast<unsigned int>(ret - WAIT_OBJECT_0);
}

void	CEventWin::Throw()
{
	SetEvent(m_event);
}


void	CEventWin::Reset()
{
	ResetEvent(m_event);
}
