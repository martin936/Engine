#ifndef __EVENT_WIN_H__
#define __EVENT_WIN_H__

#include "../Event.h"

#include <windows.h>

class CEventWin : public CEvent
{
	friend CEvent;
public:
	CEventWin();
	~CEventWin();
	void	Wait();
	void	Throw();
	void	Reset();

private:
	HANDLE	m_event;
};


#endif
