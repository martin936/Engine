#ifndef __WINDOW_H__
#define __WINDOW_H__


class CWindow
{
	friend class CKeyboard;
	friend class CMouse;

public:

	CWindow(int width, int height, const char* title, bool bFullscreen);
	CWindow(void* pHandle);
	~CWindow(void);

	static void Initialize(void);

	inline int GetWidth() const { return m_nWidth; }
	inline int GetHeight() const { return m_nHeight; }

	inline void* GetHandle() const { return m_pHandle; }

	void MainLoop(void) const;

	void Exit()
	{
		m_bShouldQuit = true;
	}

	inline static CWindow* GetMainWindow(void) { return ms_pMainWindow; }
	
	void SetMainWindow(void) { ms_pMainWindow = this; }

	void SwapBuffers(void) const;

	static void Terminate(void);

	static float GetTime();

	inline float GetAspectRatio() const { return 1.f * m_nWidth / m_nHeight; }

private:

	int m_nWidth;
	int m_nHeight;
	bool m_bFullscreen;

	bool m_bShouldQuit;

	void* m_pHandle;

	static bool ms_bInit;
	static CWindow* ms_pMainWindow;
};


#endif
