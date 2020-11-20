#ifndef GAMEPLAY_INPUTS_INC
#define GAMEPLAY_INPUTS_INC

#include "Engine/Engine.h"

#ifdef _WIN32
#define DIRECTINPUT_VERSION 0x0800
#include <winapifamily.h>
#include <DInput.h>
#endif

#define TOTAL_BUTTON_COUNT 6

#define UNBINDED_KEY      -1

#define GAMEPAD_A				0
#define GAMEPAD_B				1
#define GAMEPAD_X				2
#define GAMEPAD_Y				3
#define GAMEPAD_LB				4
#define GAMEPAD_RB				5
#define GAMEPAD_SELECT			6
#define GAMEPAD_START			7
#define GAMEPAD_CENTER			8
#define GAMEPAD_L				9
#define GAMEPAD_R				10

#define GAMEPAD_STICK_L_SIDE	0
#define GAMEPAD_STICK_L_UP		1

#ifdef _WIN32
#define GAMEPAD_STICK_R_SIDE	2
#define GAMEPAD_STICK_R_UP		3
#else
#define GAMEPAD_STICK_R_SIDE	3
#define GAMEPAD_STICK_R_UP		2
#endif
#define GAMEPAD_ARROW_UP		10
#define GAMEPAD_ARROW_RIGHT	11
#define GAMEPAD_ARROW_DOWN		12
#define GAMEPAD_ARROW_LEFT		13
#define GAMEPAD_LT				4
#define GAMEPAD_RT				5



#define INPUT_L_HORIZONTAL_AXIS 0
#define INPUT_L_VERTICAL_AXIS	1
#define INPUT_R_HORIZONTAL_AXIS	2
#define INPUT_R_VERTICAL_AXIS	3



class CInputDevice
{
public:
	
	enum InputDeviceType
	{
		INPUT_DEVICE_KEYBOARD,
		INPUT_DEVICE_JOYSTICK,
		INPUT_DEVICE_MOUSE
	};

	CInputDevice(InputDeviceType eType);
	~CInputDevice();

	inline static void SetNumActions(int nActions)
	{
		ms_nNumActions = nActions;
	}

	inline InputDeviceType GetType(void) const { return m_eType; }
	inline bool IsReady(void) const { return m_bActive; }

	virtual void Update(void) {};
	virtual void FillActionList(bool* pbActions, int eNbActions = -1) const {};
	virtual void GetSticks(float* pLeftStick, float* pRightStick, bool* pActions = NULL) const {};

	virtual void UseMap(int* pButtonMap, int nButtonCount, int* pAxesMap = NULL, int nAxesCount = 0) {};

protected:

#ifdef _WIN32
	static LPDIRECTINPUT8	ms_pDirectInputDevice;
	static HRESULT			ms_nInitDirectInputResult;
#endif

	bool	m_bActive;
	bool	m_bInitialized;

	static int	ms_nNumActions;

private:

	static int ms_nNumDevices;

	InputDeviceType m_eType;
};


class CJoystick : public CInputDevice
{
public:

	CJoystick(void);
	~CJoystick(void);

	void Update(void);

	static bool IsPlugedIn(int nID);

	virtual void FillActionList(bool* pbActions, int eNbActions = -1) const override;
	void GetSticks(float* pLeftStick, float* pRightStick, bool* pActions = NULL) const;

	void UseMap(int* pButtonMap, int nButtonCount, int* pAxesMap = NULL, int nAxesCount = 0);

	static int		ms_nActiveJoysticks;

protected:

	unsigned char*	m_pButtons;
	float*			m_pAxes;

	int				m_nButtonCount;
	int				m_nAxesCount;

	int*			m_pButtonMap;
	int*			m_pAxesMap;

	int*			m_pMappedButtons;
	int*			m_pMappedAxes;

	int				m_nMappedButtonCount;
	int				m_nMappedAxesCount;

	int				m_nID;
};


class CKeyboard : public CInputDevice
{
public:
	enum EKeyboard
	{
		e_MaxNbKeyboard = 4,
	};

	enum EKey
	{
		e_Key_Escape,
		e_Key_1,
		e_Key_2,
		e_Key_3,
		e_Key_4,
		e_Key_5,
		e_Key_6,
		e_Key_7,
		e_Key_8,
		e_Key_9,
		e_Key_0,
		e_Key_Minus,
		e_Key_Equals,
		e_Key_Back,
		e_Key_Tab,
		e_Key_Q,
		e_Key_W,
		e_Key_E,
		e_Key_R,
		e_Key_T,
		e_Key_Y,
		e_Key_U,
		e_Key_I,
		e_Key_O,
		e_Key_P,
		e_Key_LBracket,
		e_Key_RBracket,
		e_Key_Return,
		e_Key_LControl,
		e_Key_A,
		e_Key_S,
		e_Key_D,
		e_Key_F,
		e_Key_G,
		e_Key_H,
		e_Key_J,
		e_Key_K,
		e_Key_L,
		e_Key_Semicolon,
		e_Key_Apostrophe,
		e_Key_Grave,
		e_Key_LShift,
		e_Key_BackSlash,
		e_Key_Z,
		e_Key_X,
		e_Key_C,
		e_Key_V,
		e_Key_B,
		e_Key_N,
		e_Key_M,
		e_Key_Comma,
		e_Key_Period,
		e_Key_Slash,
		e_Key_RShift,
		e_Key_Multiply,
		e_Key_LMenu,
		e_Key_Space,
		e_Key_Capital,
		e_Key_F1,
		e_Key_F2,
		e_Key_F3,
		e_Key_F4,
		e_Key_F5,
		e_Key_F6,
		e_Key_F7,
		e_Key_F8,
		e_Key_F9,
		e_Key_F10,
		e_Key_Numlock,
		e_Key_Scroll,
		e_Key_Numpad7,
		e_Key_Numpad8,
		e_Key_Numpad9,
		e_Key_Subtract,
		e_Key_Numpad4,
		e_Key_Numpad5,
		e_Key_Numpad6,
		e_Key_Add,
		e_Key_Numpad1,
		e_Key_Numpad2,
		e_Key_Numpad3,
		e_Key_Numpad0,
		e_Key_Decimal,
		e_Key_Oem_102,
		e_Key_F11,
		e_Key_F12,
		e_Key_F13,
		e_Key_F14,
		e_Key_F15,
		e_Key_Kana,
		e_Key_Abnt_c1,
		e_Key_Convert,
		e_Key_NoConvert,
		e_Key_Yen,
		e_Key_Abnt_c2,
		e_Key_NumPadEquals,
		e_Key_PrevTrack,
		e_Key_At,
		e_Key_Colon,
		e_Key_Underline,
		e_Key_Kanji,
		e_Key_Stop,
		e_Key_Ax,
		e_Key_Unlabeled,
		e_Key_Nexttrack,
		e_Key_NumPadCenter,
		e_Key_RControl,
		e_Key_Mute,
		e_Key_Calculator,
		e_Key_PlayPause,
		e_Key_MediaStop,
		e_Key_VolumeDown,
		e_Key_VolumeUp,
		e_Key_Webhome,
		e_Key_NumPadComma,
		e_Key_Divide,
		e_Key_Sysrq,
		e_Key_Rmenu,
		e_Key_Pause,
		e_Key_Home,
		e_Key_Up,
		e_Key_Prior,
		e_Key_Left,
		e_Key_Right,
		e_Key_End,
		e_Key_Down,
		e_Key_Next,
		e_Key_Insert,
		e_Key_Delete,
		e_Key_LWin,
		e_Key_RWin,
		e_Key_Apps,
		e_Key_Power,
		e_Key_Sleep,
		e_Key_Wake,
		e_Key_WebSearch,
		e_Key_WebFavorites,
		e_Key_WebRefresh,
		e_Key_WebStop,
		e_Key_WebForward,
		e_Key_WebBack,
		e_Key_MyComputer,
		e_Key_Mail,
		e_Key_MediaSelect,

		e_Key_Undefined,

		e_NbEkoKey,
		e_NbKey = 256
	};

protected:
	static CKeyboard *		ms_ppCurrent[e_MaxNbKeyboard];
	static int				ms_nNbKeyboardCreated;

	char					m_pcKeyPressure[e_NbEkoKey];
	char					m_pcLastKeyPressure[e_NbEkoKey];
	bool					m_pbIsCLicked[e_NbEkoKey];

	CKeyboard();
	~CKeyboard();

	LPDIRECTINPUTDEVICE8	m_pDev;

	bool					m_bConnected;

public:
	static CKeyboard*		GetCurrent(int p_nNum = 0) 
	{ 
		if (ms_ppCurrent[p_nNum] == nullptr)
			ms_ppCurrent[p_nNum] = new CKeyboard();

		return ms_ppCurrent[p_nNum];
	}

	static CKeyboard*		Create(void);
	static int				GetNbKeyboardCreated() { return ms_nNbKeyboardCreated; }
	void					DestroyAll(void);
	void					Process();
	bool					IsClicked(EKey eKey) { return m_pbIsCLicked[(int)eKey]; }
	bool					IsPressed(EKey eKey) { return m_pcKeyPressure[(int)eKey] > 0.0f ? true : false; }
	bool					IsReleased(EKey eKey) { return m_pcLastKeyPressure[(int)eKey] > 0.f && m_pcKeyPressure[(int)eKey] == 0.f; }
	bool					IsAnyKeyPressed(int* p_nPressedKeyIndex = nullptr);

	void					ResetKeys();

	void					FillPushKey(void);
	float					GetPressure(int p_nIdKey);
};


/*class CKeyboard : public CInputDevice
{
public:

	inline static CKeyboard* GetInstance()
	{
		if (ms_pCurrent == NULL)
			ms_pCurrent = new CKeyboard;

		return ms_pCurrent;
	}

	CKeyboard(void);
	~CKeyboard(void);

	void Update();

	virtual void FillActionList(bool* pbActions, int eNbActions = -1) const override;
	void GetSticks(float* pLeftStick, float* pRightStick, bool* pActions = NULL) const;

	bool IsPressed(int key) const;
	bool IsReleased(int key) const;

	void UseMap(int* pButtonMap, int nButtonCount, int* pAxesMap = NULL, int nAxesCount = 0);

protected:

	unsigned char*	m_pButtons;

	int*			m_pButtonMap;
	int*			m_pMappedButtons;

	int				m_nMappedButtonCount;

	static CKeyboard* ms_pCurrent;
};*/



class CCursor
{

public:
	enum EKey
	{
		e_Key_AxeX = 0,
		e_Key_AxeY,
		e_Key_AxeZ,
		e_Key_Number
	};


private:
	float m_fAxeX_Max;
	float m_fAxeY_Max;
	float m_fAxeZ_Max;
	float m_fSensibility_Max;

	float m_pfAxes[3];			//the current position
	float m_pfLastAxes[3];		//The last position

	float m_pfDirection[3];		//The direction
	float m_fVelocity;			//The velocity 
	float m_fSensibility;

public:
	CCursor();

	void	SetXMax(float p_fX) { m_fAxeX_Max = p_fX; }
	void	SetYMax(float p_fY) { m_fAxeY_Max = p_fY; }
	void	SetZMax(float p_fZ) { m_fAxeZ_Max = p_fZ; }
	void	SetSensibilityMax(float p_fSpeedFactor) { m_fSensibility = p_fSpeedFactor; }

	void	SetX(float p_fX);
	void	SetY(float p_fY);
	void	SetZ(float p_fZ);
	void	SetSensibility(float p_fSpeedFactor);

	float	GetX() { return m_pfAxes[e_Key_AxeX]; }
	float	GetY() { return m_pfAxes[e_Key_AxeY]; }
	float	GetZ() { return m_pfAxes[e_Key_AxeZ]; }

	float	GetXMax() { return m_fAxeX_Max; }
	float	GetYMax() { return m_fAxeY_Max; }
	float	GetZMax() { return m_fAxeZ_Max; }
	float	GetSensibilityMax() { return m_fSensibility_Max; }


	float	GetLastX() { return m_pfLastAxes[e_Key_AxeX]; }
	float	GetLastY() { return m_pfLastAxes[e_Key_AxeY]; }
	float	GetLastZ() { return m_pfLastAxes[e_Key_AxeZ]; }

	void	SetDirection(float p_fX, float p_fY, float p_fZ);

	float	GetVelocity() { return m_fVelocity; }
	void	SetVelocity(float p_fVelocity) { m_fVelocity = p_fVelocity; }

	float	GetDirectionX() { return m_pfDirection[e_Key_AxeX]; }
	float	GetDirectionY() { return m_pfDirection[e_Key_AxeY]; }
	float	GetDirectionZ() { return m_pfDirection[e_Key_AxeZ]; }

	float	GetSensibility() { return m_fSensibility; }

	CCursor& operator=(const CCursor& p_Module);
};



class CMouse : public CInputDevice
{
public:

	enum EKey
	{
		e_Button_MouseGoUp,
		e_Button_MouseGoDown,
		e_Button_MouseGoLeft,
		e_Button_MouseGoRight,

		e_Button_LeftClick,
		e_Button_RightClick,
		e_Button_WheelClick,
		e_Button_Next,

		e_Button_WheelUp,
		e_Button_WheelDown,
		e_Button_0,
		e_Button_1,
		e_Button_2,
		e_Button_3,
		e_Button_4,
		e_Button_5,
		e_Button_6,
		e_Button_7,
		e_NbKey,
	};

	enum EMouse
	{
		e_NbAxis = 4,
		e_NbButton = CMouse::e_NbKey - e_NbAxis,
		e_MaxNbMouse = 8,
	};

	CMouse();
	~CMouse();

	void	Process();

	void	SetPos(float fPosX, float fPosY);
	void	GetPos(float* fPosX, float* fPosY);
	void	GetLastPos(float* fPosX, float* fPosY);

	bool	IsPressed(EKey eKey);

	static float	ms_fPressureMaxReturned;
	static float	ms_fGeneralClampInputValue;

	static CMouse *		GetCurrent(int p_nNum = 0)
	{
		if (ms_ppCurrent[p_nNum] == nullptr)
			ms_ppCurrent[p_nNum] = new CMouse();

		return ms_ppCurrent[p_nNum];
	}

private:

#ifdef _WIN32
	LPDIRECTINPUTDEVICE8	m_pDev;
	DIMOUSESTATE2			m_MouseState;

	float	m_pfKeyPressure[e_NbAxis];
	char	m_pcKeyPressure[e_NbButton];
	bool	m_bLocked;

	void	LockPointToScreen(POINT* p_pPointClient);

	void	SetLockToScreen(bool p_bLock);

	CCursor	m_pMouseCursor;

	static CMouse *		ms_ppCurrent[e_MaxNbMouse];
	static int			ms_nNbMouseCreated;

	void				DestroyAll(void);

	static int			GetNbMouseCreated() { return ms_nNbMouseCreated; }

	void			Process(float p_fDeltaTime);
	float			GetPressure(int p_nIdKey);
	float			GetXStep(void);
	float			GetYStep(void);
	void			SetX(float p_fPosx);
	void			SetY(float p_fPosy);

	//Public
	float			GetXPos(void) { return m_pMouseCursor.GetX(); }
	float			GetYPos(void) { return m_pMouseCursor.GetY(); }
	float			GetLastXPos(void) { return m_pMouseCursor.GetLastX(); }
	float			GetLastYPos(void) { return m_pMouseCursor.GetLastY(); }
	void			SetSpeedFactor(float p_fSpeed) { m_pMouseCursor.SetSensibility(p_fSpeed); }
	float			GetSpeedFactor() { return m_pMouseCursor.GetSensibility(); }
#else
	float	m_fPosX;
	float	m_fPosY;
#endif
};


#endif

