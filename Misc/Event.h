#ifndef __EVENT_H__
#define __EVENT_H__

class CEvent
{
public:
	virtual ~CEvent() {}
	virtual void		Wait() = 0;		// Bloque si l'event n'est pas d�clanch�
	virtual void		Throw() = 0;	// Declanche l'event
	virtual void		Reset() = 0;	// Doit �tre appell�, une fois l'event d�clanch�, pour que le Wait soit a nouveau bloquant 

	static unsigned int MultipleWait(unsigned int nNumEvents, CEvent** pEvents);

	static CEvent	*Create();
	static void CreateMultipleEvents(unsigned int nNumEvents, CEvent** pEvents);
};


#endif