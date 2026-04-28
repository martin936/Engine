#ifndef __DEBUG_BACKGROUND_H__
#define __DEBUG_BACKGROUND_H__


class CTexture;

class CDebugBackground
{
public:

	static void Init();

	static void SetTexture(CTexture& pTexture);

	static unsigned int GetTextureID();

	static bool IsEnabled()
	{
		return ms_nTextureID != INVALIDHANDLE;
	}

private:

	static unsigned int ms_nTextureID;
};


#endif
