#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

class CSemaphore
{
public:

	virtual ~CSemaphore() {}

	enum {
		e_MaxNbTokens = 10,
	};

	static CSemaphore* Create(int max_nb_tokens = e_MaxNbTokens, const char *p_pcName = "unnamed semaphore");
	virtual bool TakeToken() = 0;
	virtual bool ReleaseToken() = 0;

	virtual bool TakeTokenNoWait() { return true; };

};

#endif