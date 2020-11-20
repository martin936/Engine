#ifndef __EVENT_H__
#define __EVENT_H__

class CEvent
{
public:
	virtual ~CEvent() {}
	virtual void		Wait() = 0;		// Bloque si l'event n'est pas déclanché
	virtual void		Throw() = 0;	// Declanche l'event
	virtual void		Reset() = 0;	// Doit être appellé, une fois l'event déclanché, pour que le Wait soit a nouveau bloquant 

	static unsigned int MultipleWait(unsigned int nNumEvents, CEvent** pEvents);

	static CEvent	*Create();
	static void CreateMultipleEvents(unsigned int nNumEvents, CEvent** pEvents);
};


#endif