#include "Inputs.h"
#include "Engine/Renderer/Window/Window.h"
#include "Engine/Device/DeviceManager.h"
#include "Engine/Editor/Adjustables/Adjustables.h"


ADJUSTABLE("Dead Zone", float, gs_fDeadZone, 0.3f, 0.f, 1.f, "Gameplay/Controller")
ADJUSTABLE("Trigger Zone", float, gs_fTriggerZone, 0.6f, 0.f, 1.f, "Gameplay/Controller")


int CJoystick::ms_nActiveJoysticks = 0;
CMouse* CMouse::ms_ppCurrent[e_MaxNbMouse] = { NULL };

//CKeyboard* CKeyboard::ms_pCurrent = NULL;

int CInputDevice::ms_nNumActions = 0;
int CInputDevice::ms_nNumDevices = 0;

LPDIRECTINPUT8	CInputDevice::ms_pDirectInputDevice;
HRESULT			CInputDevice::ms_nInitDirectInputResult;

float	CMouse::ms_fPressureMaxReturned = 1000.0f;
float	CMouse::ms_fGeneralClampInputValue = 0.15f;

CKeyboard *CKeyboard::ms_ppCurrent[CKeyboard::e_MaxNbKeyboard] = { NULL, NULL, NULL, NULL };
int CKeyboard::ms_nNbKeyboardCreated = 0;

#ifdef _WIN32
extern HINSTANCE g_hInstance;
#endif

bool g_bShouldMaskWinKey = true;

CKeyboard::EKey gs_pnPc2Eko[CKeyboard::e_NbKey] =
{
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Escape,					//#define DIK_ESCAPE          0x01
	CKeyboard::e_Key_1,							//#define DIK_1               0x02
	CKeyboard::e_Key_2,							//#define DIK_2               0x03
	CKeyboard::e_Key_3,							//#define DIK_3               0x04
	CKeyboard::e_Key_4,							//#define DIK_4               0x05
	CKeyboard::e_Key_5,							//#define DIK_5               0x06
	CKeyboard::e_Key_6,							//#define DIK_6               0x07
	CKeyboard::e_Key_7,							//#define DIK_7               0x08
	CKeyboard::e_Key_8,							//#define DIK_8               0x09
	CKeyboard::e_Key_9,							//#define DIK_9               0x0A
	CKeyboard::e_Key_0,							//#define DIK_0               0x0B
	CKeyboard::e_Key_Minus,						//#define DIK_MINUS           0x0C
	CKeyboard::e_Key_Equals,					//#define DIK_EQUALS          0x0D
	CKeyboard::e_Key_Back,						//#define DIK_BACK            0x0E
	CKeyboard::e_Key_Tab,						//#define DIK_TAB             0x0F
	CKeyboard::e_Key_Q,							//#define DIK_Q               0x10
	CKeyboard::e_Key_W,							//#define DIK_W               0x11
	CKeyboard::e_Key_E,							//#define DIK_E               0x12
	CKeyboard::e_Key_R,							//#define DIK_R               0x13
	CKeyboard::e_Key_T,							//#define DIK_T               0x14
	CKeyboard::e_Key_Y,							//#define DIK_Y               0x15
	CKeyboard::e_Key_U,							//#define DIK_U               0x16
	CKeyboard::e_Key_I,							//#define DIK_I               0x17
	CKeyboard::e_Key_O,							//#define DIK_O               0x18
	CKeyboard::e_Key_P,							//#define DIK_P               0x19
	CKeyboard::e_Key_LBracket,					//#define DIK_LBRACKET        0x1A
	CKeyboard::e_Key_RBracket,					//#define DIK_RBRACKET        0x1B
	CKeyboard::e_Key_Return,					//#define DIK_RETURN          0x1C
	CKeyboard::e_Key_LControl,					//#define DIK_LCONTROL        0x1D
	CKeyboard::e_Key_A,							//#define DIK_A               0x1E
	CKeyboard::e_Key_S,							//#define DIK_S               0x1F
	CKeyboard::e_Key_D,							//#define DIK_D               0x20
	CKeyboard::e_Key_F,							//#define DIK_F               0x21
	CKeyboard::e_Key_G,							//#define DIK_G               0x22
	CKeyboard::e_Key_H,							//#define DIK_H               0x23
	CKeyboard::e_Key_J,							//#define DIK_J               0x24
	CKeyboard::e_Key_K,							//#define DIK_K               0x25
	CKeyboard::e_Key_L,							//#define DIK_L               0x26
	CKeyboard::e_Key_Semicolon,					//#define DIK_SEMICOLON       0x27
	CKeyboard::e_Key_Apostrophe,				//#define DIK_APOSTROPHE      0x28
	CKeyboard::e_Key_Grave,						//#define DIK_GRAVE           0x29
	CKeyboard::e_Key_LShift,					//#define DIK_LSHIFT          0x2A
	CKeyboard::e_Key_BackSlash,					//#define DIK_BACKSLASH       0x2B
	CKeyboard::e_Key_Z,							//#define DIK_Z               0x2C
	CKeyboard::e_Key_X,							//#define DIK_X               0x2D
	CKeyboard::e_Key_C,							//#define DIK_C               0x2E
	CKeyboard::e_Key_V,							//#define DIK_V               0x2F
	CKeyboard::e_Key_B,							//#define DIK_B               0x30
	CKeyboard::e_Key_N,							//#define DIK_N               0x31
	CKeyboard::e_Key_M	,						//#define DIK_M               0x32
	CKeyboard::e_Key_Comma,						//#define DIK_COMMA           0x33
	CKeyboard::e_Key_Period,					//#define DIK_PERIOD          0x34
	CKeyboard::e_Key_Slash,						//#define DIK_SLASH           0x35
	CKeyboard::e_Key_RShift,					//#define DIK_RSHIFT          0x36
	CKeyboard::e_Key_Multiply,					//#define DIK_MULTIPLY        0x37
	CKeyboard::e_Key_LMenu,						//#define DIK_LMENU           0x38
	CKeyboard::e_Key_Space,						//#define DIK_SPACE           0x39
	CKeyboard::e_Key_Capital,					//#define DIK_CAPITAL         0x3A
	CKeyboard::e_Key_F1,						//#define DIK_F1              0x3B
	CKeyboard::e_Key_F2,						//#define DIK_F2              0x3C
	CKeyboard::e_Key_F3,						//#define DIK_F3              0x3D
	CKeyboard::e_Key_F4,						//#define DIK_F4              0x3E
	CKeyboard::e_Key_F5,						//#define DIK_F5              0x3F
	CKeyboard::e_Key_F6,						//#define DIK_F6              0x40
	CKeyboard::e_Key_F7,						//#define DIK_F7              0x41
	CKeyboard::e_Key_F8,						//#define DIK_F8              0x42
	CKeyboard::e_Key_F9,						//#define DIK_F9              0x43
	CKeyboard::e_Key_F10,						//#define DIK_F10             0x44
	CKeyboard::e_Key_Numlock,					//#define DIK_NUMLOCK         0x45
	CKeyboard::e_Key_Scroll,					//#define DIK_SCROLL          0x46
	CKeyboard::e_Key_Numpad7,					//#define DIK_NUMPAD7         0x47
	CKeyboard::e_Key_Numpad8,					//#define DIK_NUMPAD8         0x48
	CKeyboard::e_Key_Numpad9,					//#define DIK_NUMPAD9         0x49
	CKeyboard::e_Key_Subtract,					//#define DIK_SUBTRACT        0x4A
	CKeyboard::e_Key_Numpad4,					//#define DIK_NUMPAD4         0x4B
	CKeyboard::e_Key_Numpad5,					//#define DIK_NUMPAD5         0x4C
	CKeyboard::e_Key_Numpad6,					//#define DIK_NUMPAD6         0x4D
	CKeyboard::e_Key_Add,						//#define DIK_ADD             0x4E
	CKeyboard::e_Key_Numpad1,					//#define DIK_NUMPAD1         0x4F
	CKeyboard::e_Key_Numpad2,					//#define DIK_NUMPAD2         0x50
	CKeyboard::e_Key_Numpad3,					//#define DIK_NUMPAD3         0x51
	CKeyboard::e_Key_Numpad0,					//#define DIK_NUMPAD0         0x52
	CKeyboard::e_Key_Decimal,					//#define DIK_DECIMAL         0x53
	CKeyboard::e_Key_Undefined,					//							0x54
	CKeyboard::e_Key_Undefined,					//							0x55
	CKeyboard::e_Key_Oem_102,					//#define DIK_OEM_102         0x56
	CKeyboard::e_Key_F11,						//#define DIK_F11             0x57
	CKeyboard::e_Key_F12,						//#define DIK_F12             0x58
	CKeyboard::e_Key_Undefined,					//							0x59
	CKeyboard::e_Key_Undefined,					//							0x5A
	CKeyboard::e_Key_Undefined,					//							0x5B
	CKeyboard::e_Key_Undefined,					//							0x5C
	CKeyboard::e_Key_Undefined,					//							0x5D
	CKeyboard::e_Key_Undefined,					//							0x5E
	CKeyboard::e_Key_Undefined,					//							0x5F
	CKeyboard::e_Key_Undefined,					//							0x60
	CKeyboard::e_Key_Undefined,					//							0x61
	CKeyboard::e_Key_Undefined,					//							0x62
	CKeyboard::e_Key_Undefined,					//							0x63
	CKeyboard::e_Key_F13,						//#define DIK_F13             0x64                                
	CKeyboard::e_Key_F14,						//#define DIK_F14             0x65
	CKeyboard::e_Key_F15,						//#define DIK_F15             0x66
	CKeyboard::e_Key_Undefined,					//							0x67
	CKeyboard::e_Key_Undefined,					//							0x68
	CKeyboard::e_Key_Undefined,					//							0x69
	CKeyboard::e_Key_Undefined,					//							0x6A
	CKeyboard::e_Key_Undefined,					//							0x6B
	CKeyboard::e_Key_Undefined,					//							0x6C
	CKeyboard::e_Key_Undefined,					//							0x6D
	CKeyboard::e_Key_Undefined,					//							0x6E
	CKeyboard::e_Key_Undefined,					//							0x6F
	CKeyboard::e_Key_Kana,						//#define DIK_KANA            0x70
	CKeyboard::e_Key_Undefined,					//							0x71
	CKeyboard::e_Key_Undefined,					//							0x72
	CKeyboard::e_Key_Abnt_c1,					//#define DIK_ABNT_C1         0x73                                
	CKeyboard::e_Key_Undefined,					//							0x74
	CKeyboard::e_Key_Undefined,					//							0x75
	CKeyboard::e_Key_Undefined,					//							0x76
	CKeyboard::e_Key_Undefined,					//							0x77
	CKeyboard::e_Key_Undefined,					//							0x78
	CKeyboard::e_Key_Convert,					//#define DIK_CONVERT         0x79
	CKeyboard::e_Key_Undefined,					//							0x7A
	CKeyboard::e_Key_NoConvert,					//#define DIK_NOCONVERT       0x7B
	CKeyboard::e_Key_Undefined,					//							0x7C
	CKeyboard::e_Key_Yen,						//#define DIK_YEN             0x7D
	CKeyboard::e_Key_Abnt_c2,					//#define DIK_ABNT_C2         0x7E
	CKeyboard::e_Key_Undefined,					//							0x7F
	CKeyboard::e_Key_Undefined,					//							0x80
	CKeyboard::e_Key_Undefined,					//							0x81
	CKeyboard::e_Key_Undefined,					//							0x82
	CKeyboard::e_Key_Undefined,					//							0x83
	CKeyboard::e_Key_Undefined,					//							0x84
	CKeyboard::e_Key_Undefined,					//							0x85
	CKeyboard::e_Key_Undefined,					//							0x86							0x59
	CKeyboard::e_Key_Undefined,					//							0x87
	CKeyboard::e_Key_Undefined,					//							0x88						0x59
	CKeyboard::e_Key_Undefined,					//							0x89						0x59
	CKeyboard::e_Key_Undefined,					//							0x8A
	CKeyboard::e_Key_Undefined,					//							0x8B
	CKeyboard::e_Key_Undefined,					//							0x8C
	CKeyboard::e_Key_NumPadEquals,				//#define DIK_NUMPADEQUALS    0x8D
	CKeyboard::e_Key_Undefined,					//							0x8E
	CKeyboard::e_Key_Undefined,					//							0x8F
	CKeyboard::e_Key_PrevTrack,					//#define DIK_PREVTRACK       0x90
	CKeyboard::e_Key_At,						//#define DIK_AT              0x91
	CKeyboard::e_Key_Colon,						//#define DIK_COLON           0x92
	CKeyboard::e_Key_Underline,					//#define DIK_UNDERLINE       0x93
	CKeyboard::e_Key_Kanji,						//#define DIK_KANJI           0x94
	CKeyboard::e_Key_Stop,						//#define DIK_STOP            0x95
	CKeyboard::e_Key_Ax,						//#define DIK_AX              0x96
	CKeyboard::e_Key_Unlabeled,					//#define DIK_UNLABELED       0x97
	CKeyboard::e_Key_Undefined,					//							0x98
	CKeyboard::e_Key_Nexttrack	,				//#define DIK_NEXTTRACK       0x99
	CKeyboard::e_Key_Undefined,					//							0x9A
	CKeyboard::e_Key_Undefined,					//							0x9B
	CKeyboard::e_Key_NumPadCenter,				//#define DIK_NUMPADENTER     0x9C
	CKeyboard::e_Key_RControl,					//#define DIK_RCONTROL        0x9D
	CKeyboard::e_Key_Undefined,					//							0x9E
	CKeyboard::e_Key_Undefined,					//							0x9F
	CKeyboard::e_Key_Mute,						//#define DIK_MUTE            0xA0
	CKeyboard::e_Key_Calculator,				//#define DIK_CALCULATOR      0xA1
	CKeyboard::e_Key_PlayPause,					//#define DIK_PLAYPAUSE       0xA2
	CKeyboard::e_Key_Undefined,					//							0xA3
	CKeyboard::e_Key_MediaStop,					//#define DIK_MEDIASTOP       0xA4
	CKeyboard::e_Key_Undefined,					//							0xA5
	CKeyboard::e_Key_Undefined,					//							0xA6
	CKeyboard::e_Key_Undefined,					//							0xA7
	CKeyboard::e_Key_Undefined,					//							0xA8
	CKeyboard::e_Key_Undefined,					//							0xA9
	CKeyboard::e_Key_Undefined,					//							0xAA
	CKeyboard::e_Key_Undefined,					//							0xAB
	CKeyboard::e_Key_Undefined,					//							0xAC
	CKeyboard::e_Key_Undefined,					//							0xAD
	CKeyboard::e_Key_VolumeDown,				//#define DIK_VOLUMEDOWN      0xAE
	CKeyboard::e_Key_Undefined,					//							0xAF
	CKeyboard::e_Key_VolumeUp,					//#define DIK_VOLUMEUP        0xB0
	CKeyboard::e_Key_Undefined,					//							0xB1
	CKeyboard::e_Key_Webhome,					//#define DIK_WEBHOME         0xB2
	CKeyboard::e_Key_NumPadComma,				//#define DIK_NUMPADCOMMA     0xB3
	CKeyboard::e_Key_Undefined,					//							0xB4
	CKeyboard::e_Key_Divide,					//#define DIK_DIVIDE          0xB5
	CKeyboard::e_Key_Undefined,					//							0xB6
	CKeyboard::e_Key_Sysrq,						//#define DIK_SYSRQ           0xB7
	CKeyboard::e_Key_Rmenu,						//#define DIK_RMENU           0xB8
	CKeyboard::e_Key_Undefined,					//							0xB9
	CKeyboard::e_Key_Undefined,					//							0xBA
	CKeyboard::e_Key_Undefined,					//							0xBB
	CKeyboard::e_Key_Undefined,					//							0xBC
	CKeyboard::e_Key_Undefined,					//							0xBD
	CKeyboard::e_Key_Undefined,					//							0xBE
	CKeyboard::e_Key_Undefined,					//							0xBF
	CKeyboard::e_Key_Undefined,					//							0xC0
	CKeyboard::e_Key_Undefined,					//							0xC1
	CKeyboard::e_Key_Undefined,					//							0xC2
	CKeyboard::e_Key_Undefined,					//							0xC3
	CKeyboard::e_Key_Undefined,					//							0xC4
	CKeyboard::e_Key_Pause,						//#define DIK_PAUSE           0xC5
	CKeyboard::e_Key_Undefined,					//							0xC6
	CKeyboard::e_Key_Home,						//#define DIK_HOME            0xC7
	CKeyboard::e_Key_Up,						//#define DIK_UP              0xC8
	CKeyboard::e_Key_Prior,						//#define DIK_PRIOR           0xC9
	CKeyboard::e_Key_Undefined,					//							0xCA
	CKeyboard::e_Key_Left,						//#define DIK_LEFT            0xCB
	CKeyboard::e_Key_Undefined,					//							0xCC
	CKeyboard::e_Key_Right,						//#define DIK_RIGHT           0xCD
	CKeyboard::e_Key_Undefined,					//							0xCE
	CKeyboard::e_Key_End,						//#define DIK_END             0xCF
	CKeyboard::e_Key_Down,						//#define DIK_DOWN            0xD0
	CKeyboard::e_Key_Next,						//#define DIK_NEXT            0xD1
	CKeyboard::e_Key_Insert,					//#define DIK_INSERT          0xD2
	CKeyboard::e_Key_Delete,					//#define DIK_DELETE          0xD3
	CKeyboard::e_Key_Undefined,					//							0xD4
	CKeyboard::e_Key_Undefined,					//							0xD5
	CKeyboard::e_Key_Undefined,					//							0xD6
	CKeyboard::e_Key_Undefined,					//							0xD7
	CKeyboard::e_Key_Undefined,					//							0xD8
	CKeyboard::e_Key_Undefined,					//							0xD9
	CKeyboard::e_Key_Undefined,					//							0xDA
	CKeyboard::e_Key_LWin,						//#define DIK_LWIN            0xDB
	CKeyboard::e_Key_RWin,						//#define DIK_RWIN            0xDC
	CKeyboard::e_Key_Apps,						//#define DIK_APPS            0xDD
	CKeyboard::e_Key_Power,						//#define DIK_POWER           0xDE
	CKeyboard::e_Key_Sleep,						//#define DIK_SLEEP           0xDF
	CKeyboard::e_Key_Undefined,					//							0xE0
	CKeyboard::e_Key_Undefined,					//							0xE1
	CKeyboard::e_Key_Undefined,					//							0xE2
	CKeyboard::e_Key_Wake,						//#define DIK_WAKE            0xE3
	CKeyboard::e_Key_Undefined,					//							0xE4
	CKeyboard::e_Key_WebSearch,					//#define DIK_WEBSEARCH       0xE5
	CKeyboard::e_Key_WebFavorites,				//#define DIK_WEBFAVORITES    0xE6
	CKeyboard::e_Key_WebRefresh,				//#define DIK_WEBREFRESH      0xE7
	CKeyboard::e_Key_WebStop,					//#define DIK_WEBSTOP         0xE8
	CKeyboard::e_Key_WebForward,				//#define DIK_WEBFORWARD      0xE9
	CKeyboard::e_Key_WebBack,					//#define DIK_WEBBACK         0xEA
	CKeyboard::e_Key_MyComputer,				//#define DIK_MYCOMPUTER      0xEB
	CKeyboard::e_Key_Mail,						//#define DIK_MAIL            0xEC
	CKeyboard::e_Key_MediaSelect,				//#define DIK_MEDIASELECT     0xED
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
	CKeyboard::e_Key_Undefined,					//
};

//============================================================================
//====== CKeyboard :: CKeyboard
CKeyboard::CKeyboard() : CInputDevice(INPUT_DEVICE_KEYBOARD)
{
	for (int i = 0; i < e_NbEkoKey; ++i)
	{
		m_pcKeyPressure[i] = 0;
		m_pcLastKeyPressure[i] = m_pcKeyPressure[i];
		m_pbIsCLicked[i] = false;
	}

	// Set current.
	ASSERT(ms_ppCurrent[ms_nNbKeyboardCreated] == NULL);
	ms_ppCurrent[ms_nNbKeyboardCreated] = this;
	ms_nNbKeyboardCreated++;

	HRESULT hr;

	// Test si direct input existe.
	if (ms_nInitDirectInputResult != DI_OK)
		return;

	assert(ms_pDirectInputDevice != NULL);

	/// Cree le device.
	m_pDev = NULL;
	if (ms_pDirectInputDevice->CreateDevice(GUID_SysKeyboard, &m_pDev, NULL) != DI_OK)
		return;
	assert(m_pDev != NULL);

	// Set keyboard format.
	if (m_pDev->SetDataFormat(&c_dfDIKeyboard) != DI_OK)
		return;

	// Set Cooperative Level.
#ifdef _DEBUG
	hr = m_pDev->SetCooperativeLevel((HWND)CWindow::GetMainWindow()->GetHandle(), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
#else
	if (g_bShouldMaskWinKey)
		hr = m_pDev->SetCooperativeLevel((HWND)CWindow::GetMainWindow()->GetHandle(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND | DISCL_NOWINKEY);
	else
		hr = m_pDev->SetCooperativeLevel((HWND)CWindow::GetMainWindow()->GetHandle(), DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
#endif

	if (FAILED(hr))
		return;

	// Premiere aqcuisition.
	hr = m_pDev->Acquire();
	m_bConnected = SUCCEEDED(hr);
}

//============================================================================
//====== CKeyboard :: ~CKeyboard
CKeyboard :: ~CKeyboard()
{
	if (ms_ppCurrent[0] != NULL)
	{
		m_pDev->Unacquire();
		m_pDev->Release();
	}
}

float GetNewInterpolation(float fMin1, float fMax1, float fMin2, float fMax2, float fCurrentValue)
{
	return (fMax2 * (fCurrentValue - fMin1) - fMin2 * (fCurrentValue - fMax1)) / (fMax1 - fMin1);
}

//============================================================================
//====== CKeyboard :: FillPushKey
void CKeyboard::FillPushKey(void)
{
	ASSERT(m_pDev != NULL);

	// reaquire if necessary
	if (!m_bConnected)
	{
		HRESULT hr = m_pDev->Acquire();

		if (FAILED(hr))
			return;

		m_bConnected = true;
	}

	//record all key pushed
	char pKey[e_NbKey];
	memset(pKey, 0, sizeof(pKey));

	HRESULT hr = m_pDev->GetDeviceState(sizeof(pKey), (LPVOID)&pKey);
	if (FAILED(hr))
	{
		m_pDev->Unacquire();
		m_bConnected = false;
		for (int i = 0; i < e_NbKey; ++i)
			m_pcKeyPressure[gs_pnPc2Eko[i]] = 0;
		return;
	}


	//for each key
	for (int i = 0; i < e_NbKey; ++i)
	{
		m_pcKeyPressure[gs_pnPc2Eko[i]] = (pKey[i] & 0x80) ? 1 : 0;
	}
}

//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
float CKeyboard::GetPressure(int p_nIdKey)
{
	ASSERT(p_nIdKey < e_NbKey);
	ASSERT(p_nIdKey < e_NbEkoKey);

	return GetNewInterpolation(0.0f, 1.0f, 0.0f, CMouse::ms_fPressureMaxReturned, m_pcKeyPressure[p_nIdKey]);
}



void CKeyboard::ResetKeys()
{
	for (int i = 0; i < e_NbEkoKey; ++i)
	{
		m_pcLastKeyPressure[i] = m_pcKeyPressure[i];
		m_pcKeyPressure[i] = 0;
		m_pbIsCLicked[i] = false;
	}
}

//============================================================================
//====== CKeyboard :: Process
void CKeyboard::Process()
{
	//Reset each Key
	ResetKeys();

	// Fill m_pKeyPressure.
	FillPushKey();

	for (int i = 0; i < e_NbEkoKey; ++i)
	{
		m_pbIsCLicked[i] = false;
		if (m_pcLastKeyPressure[i] == 0 && m_pcKeyPressure[i] != 0)
		{
			m_pbIsCLicked[i] = true;
		}
	}
}

#if defined _DEBUG
#define OnlyInDebugBuildsForKeyboard(x) x
#else
#define OnlyInDebugBuildsForKeyboard(x)
#endif

///============================================================================
///====== CKeyboard :: Create
CKeyboard *CKeyboard::Create(void)
{
	new CKeyboard();

	return GetCurrent(CKeyboard::ms_nNbKeyboardCreated - 1);
}

///============================================================================
///====== CKeyboard :: Destroy
void CKeyboard::DestroyAll(void)
{
	int i;

	for (i = 0; i < e_MaxNbKeyboard; i++)
	{
		if (ms_ppCurrent[i] != NULL)
		{
			delete ms_ppCurrent[i];
			ms_ppCurrent[i] = NULL;
		}
	}
	ms_nNbKeyboardCreated = 0;
}

///============================================================================
///====== CKeyboard :: IsAnyKeyPressed
bool CKeyboard::IsAnyKeyPressed(int* p_nPressedKeyIndex /* = nullptr */)
{
	for (int i = 0; i < e_NbEkoKey; ++i)
	{
		if (m_pcKeyPressure[i] > 0.0f)
		{
			if (p_nPressedKeyIndex)
				*p_nPressedKeyIndex = i;
			return true;
		}
	}

	return false;
}



CInputDevice::CInputDevice(InputDeviceType eType)
{
	m_bActive = false;
	m_eType = eType;
	m_bInitialized = false;
	ms_nNumDevices++;

#ifdef _WIN32
	if (ms_pDirectInputDevice == NULL)
	{
		ms_nInitDirectInputResult = DirectInput8Create(g_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&ms_pDirectInputDevice, NULL);
	}
#endif
}


CInputDevice::~CInputDevice()
{
	ms_nNumDevices--;

#ifdef _WIN32
	if (ms_nNumDevices == 0)
	{
		ms_pDirectInputDevice->Release();
		ms_pDirectInputDevice = NULL;
		ms_nInitDirectInputResult = S_FALSE;
	}
#endif
}


CJoystick::CJoystick() : CInputDevice(INPUT_DEVICE_JOYSTICK)
{
	m_bActive = false;
	m_pButtonMap = NULL;
	m_pMappedButtons = NULL;
	m_pMappedAxes = NULL;
	m_pAxesMap = NULL;
	m_pButtons = NULL;
	m_pAxes = NULL;

	m_bInitialized = false;

	for (int i = 0; i < 8; i++)
	{
#ifdef __OPENGL__
		m_nID = GLFW_JOYSTICK_1 + ms_nActiveJoysticks + i;

		if (glfwJoystickPresent(m_nID) == GL_TRUE)
		{
			ms_nActiveJoysticks++;
			m_bActive = true;
			break;
		}
#endif
	}

	if (!m_bActive)
	{
#ifdef __OPENGL__
		m_nID = GLFW_JOYSTICK_1 + ms_nActiveJoysticks;
		ms_nActiveJoysticks++;
#endif
	}

	if (m_bActive)
	{
#ifdef __OPENGL__
		glfwGetJoystickButtons(m_nID, &m_nButtonCount);
		glfwGetJoystickAxes(m_nID, &m_nAxesCount);
#endif

		m_pButtonMap	= new int[m_nButtonCount]();
		m_pAxesMap		= new int[m_nAxesCount]();

		m_bInitialized = true;
	}

	else
	{
		m_pButtonMap = new int[25]();
		m_pAxesMap = new int[25]();
	}
}

CJoystick::~CJoystick(void)
{
	if (m_pButtonMap != NULL)
		delete[] m_pButtonMap;

	if (m_pAxesMap != NULL)
		delete[] m_pAxesMap;

	m_pButtonMap = NULL;
	m_pAxesMap = NULL;
}


void CJoystick::UseMap(int* pButtonMap, int nButtonCount, int* pAxesMap, int nAxesCount)
{
	m_nMappedButtonCount = nButtonCount;
	m_nMappedAxesCount = nAxesCount;

	SAFE_DELETE(m_pMappedButtons)
	SAFE_DELETE(m_pMappedAxes)
	SAFE_DELETE(m_pButtons)
	SAFE_DELETE(m_pAxes)

	m_pMappedButtons = new int[m_nMappedButtonCount]();
	m_pMappedAxes = new int[m_nMappedAxesCount]();
	m_pButtons = new unsigned char[m_nMappedButtonCount]();
	m_pAxes = new float[m_nMappedAxesCount]();

	for (int i = 0; i < m_nMappedButtonCount; i++)
	{
		m_pMappedButtons[i] = *(pButtonMap + i * 2);
		m_pButtonMap[m_pMappedButtons[i]] = *(pButtonMap + i * 2 + 1);
	}

	for (int i = 0; i < m_nMappedAxesCount; i++)
	{
		m_pMappedAxes[i] = *(pAxesMap + i * 2);
		m_pAxesMap[m_pMappedAxes[i]] = *(pAxesMap + i * 2 + 1);
	}
}



bool CJoystick::IsPlugedIn(int nID)
{
#ifdef __OPENGL__
	return glfwJoystickPresent(GLFW_JOYSTICK_1 + nID);
#else
	return false;
#endif
}


void CJoystick::Update(void)
{
#ifdef __OPENGL__
	if (!glfwJoystickPresent(m_nID))
	{
		m_bActive = false;
		m_bInitialized = false;
		return;
	}

	if (!m_bInitialized)
	{
		glfwGetJoystickButtons(m_nID, &m_nButtonCount);
		glfwGetJoystickAxes(m_nID, &m_nAxesCount);

		m_bActive = true;
		m_bInitialized = true;
	}

	int nButtonCount, nAxeCount;

	const unsigned char* pButtons = glfwGetJoystickButtons(m_nID, &nButtonCount);
	const float* pAxes = glfwGetJoystickAxes(m_nID, &nAxeCount);

	for (int i = 0; i < m_nMappedButtonCount; i++)
	{
		m_pButtons[i] = pButtons[m_pMappedButtons[i]];
	}

	for (int i = 0; i < m_nMappedAxesCount; i++)
	{
		if (m_pMappedAxes[i] < 0)
			m_pAxes[i] = -pAxes[-m_pMappedAxes[i]];

		else
			m_pAxes[i] = pAxes[m_pMappedAxes[i]];
	}
#endif
}


void CJoystick::FillActionList(bool* pbActions, int p_nNbActions) const
{
	int i;
	int eCurrentAction = 0;
	int nNbActions = p_nNbActions;
	bool bFillAxis = false;

	if (p_nNbActions < 0)
	{
		nNbActions = ms_nNumActions;
		bFillAxis = true;
	}

	while (eCurrentAction <= nNbActions)
	{
		if (bFillAxis)
			pbActions[eCurrentAction] = false;

		for (i = 0; i < m_nMappedButtonCount; i++)
		{
			if (m_pButtonMap[m_pMappedButtons[i]] == eCurrentAction && m_pButtons[i])
				pbActions[eCurrentAction] = true;
		}

		if (bFillAxis)
		{
			for (i = 0; i < m_nMappedAxesCount; i++)
			{
				if (m_pAxesMap[m_pMappedAxes[i]] == eCurrentAction && m_pAxes[i] > gs_fDeadZone)
					pbActions[eCurrentAction] = true;
			}
		}

		eCurrentAction++;
	}
}


void CJoystick::GetSticks(float* pLeftStick, float* pRightStick, bool* pActions) const
{
	if (pLeftStick != NULL)
	{
		pLeftStick[0] = m_pAxes[INPUT_L_HORIZONTAL_AXIS];
		pLeftStick[1] = m_pAxes[INPUT_L_VERTICAL_AXIS];
	}

	if (pRightStick != NULL)
	{
		pRightStick[0] = m_pAxes[INPUT_R_HORIZONTAL_AXIS];
		pRightStick[1] = m_pAxes[INPUT_R_VERTICAL_AXIS];
	}
}

/*
CKeyboard::CKeyboard(void) : CInputDevice(INPUT_DEVICE_KEYBOARD)
{
	m_bActive = true;

	m_pMappedButtons = NULL;
#ifdef __OPENGL__
	m_pButtonMap = new int[GLFW_KEY_LAST]();
#endif
	m_pButtons = NULL;

	m_bInitialized = true;
}


void CKeyboard::UseMap(int* pButtonMap, int nButtonCount, int* pAxesMap, int nAxesCount)
{
	m_nMappedButtonCount = nButtonCount;

	SAFE_DELETE(m_pMappedButtons)
	SAFE_DELETE(m_pButtons)

	m_pMappedButtons = new int[m_nMappedButtonCount]();
	m_pButtons = new unsigned char[m_nMappedButtonCount]();

	for (int i = 0; i < m_nMappedButtonCount; i++)
	{
		m_pMappedButtons[i] = *(pButtonMap + i * 2);
		m_pButtonMap[m_pMappedButtons[i]] = *(pButtonMap + i * 2 + 1);
	}
}



void CKeyboard::Update(void)
{
#ifdef __OPENGL__
	GLFWwindow* pMainWindow = (GLFWwindow*)(CWindow::GetMainWindow()->m_pHandle);

	for (int i = 0; i < m_nMappedButtonCount; i++)
	{
		if (glfwGetKey(pMainWindow, m_pMappedButtons[i]) == GLFW_PRESS)
			m_pButtons[i] = 1;

		else
			m_pButtons[i] = 0;
	}
#endif
}

void CKeyboard::FillActionList(bool* pbActions, int eNbActions) const
{
	int i;
	int eCurrentAction = 0;

	while (eCurrentAction <= ms_nNumActions)
	{
		pbActions[eCurrentAction] = false;

		for (i = 0; i < m_nMappedButtonCount; i++)
		{
			if (m_pButtonMap[m_pMappedButtons[i]] == eCurrentAction && m_pButtons[i])
				pbActions[eCurrentAction] = true;
		}

		eCurrentAction++;
	}
}

void CKeyboard::GetSticks(float* pLeftStick, float* pRightStick, bool* pActions) const
{
	if (pActions == NULL)
		return;

	float right		= pActions[CPlayer::ActionType::e_MoveRight] ? 1.f : 0.f;
	float left		= pActions[CPlayer::ActionType::e_MoveLeft] ? 1.f : 0.f;
	float up		= pActions[CPlayer::ActionType::e_MoveUp] ? 1.f : 0.f;
	float down		= pActions[CPlayer::ActionType::e_MoveDown] ? 1.f : 0.f;
	float aim_right = pActions[CPlayer::ActionType::e_AimRight] ? 1.f : 0.f;
	float aim_left	= pActions[CPlayer::ActionType::e_AimLeft] ? 1.f : 0.f;
	float aim_up	= pActions[CPlayer::ActionType::e_AimUp] ? 1.f : 0.f;
	float aim_down	= pActions[CPlayer::ActionType::e_AimDown] ? 1.f : 0.f;

	pLeftStick[0] = right - left;
	pLeftStick[1] = up - down;

	pRightStick[0] = aim_right - aim_left;
	pRightStick[1] = aim_up - aim_down;
}


bool CKeyboard::IsPressed(int key) const
{
#ifdef __OPENGL__
	GLFWwindow* pMainWindow = (GLFWwindow*)(CWindow::GetMainWindow()->m_pHandle);

	if (glfwGetKey(pMainWindow, key) == GLFW_PRESS)
		return true;
#endif

	return false;
}


bool CKeyboard::IsReleased(int key) const
{
#ifdef __OPENGL__
	GLFWwindow* pMainWindow = (GLFWwindow*)(CWindow::GetMainWindow()->m_pHandle);

	if (glfwGetKey(pMainWindow, key) == GLFW_RELEASE)
		return true;
#endif

	return false;
}*/


CCursor::CCursor()
{
	m_fAxeX_Max = 640.0f;
	m_fAxeY_Max = 480.0f;
	m_fAxeZ_Max = 1000.0f;
	m_fSensibility_Max = 10.0f;

	//First init: the cursor is on the middle of the screen
	m_pfAxes[e_Key_AxeX] = m_fAxeX_Max / 2.0f;
	m_pfAxes[e_Key_AxeY] = m_fAxeY_Max / 2.0f;
	m_pfAxes[e_Key_AxeZ] = m_fAxeZ_Max / 2.0f;

	m_pfLastAxes[e_Key_AxeX] = m_pfAxes[e_Key_AxeX];
	m_pfLastAxes[e_Key_AxeY] = m_pfAxes[e_Key_AxeY];
	m_pfLastAxes[e_Key_AxeZ] = m_pfAxes[e_Key_AxeZ];

	m_pfDirection[e_Key_AxeX] = 0.0f;
	m_pfDirection[e_Key_AxeY] = 0.0f;
	m_pfDirection[e_Key_AxeZ] = 0.0f;

	m_fVelocity = 0.0f;
	m_fSensibility = 1.0f;
}
//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
void CCursor::SetX(float p_fX)
{
	m_pfLastAxes[e_Key_AxeX] = m_pfAxes[e_Key_AxeX];
	if (p_fX < 0.0f)
		m_pfAxes[e_Key_AxeX] = 0.0f;
	else
		if (p_fX > m_fAxeX_Max)
		{
			m_pfAxes[e_Key_AxeX] = m_fAxeX_Max;
		}
		else
		{
			m_pfAxes[e_Key_AxeX] = p_fX;
		}
}
//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
void CCursor::SetY(float p_fY)
{
	m_pfLastAxes[e_Key_AxeY] = m_pfAxes[e_Key_AxeY];
	if (p_fY < 0.0f)
		m_pfAxes[e_Key_AxeY] = 0.0f;
	else
		if (p_fY > m_fAxeY_Max)
		{
			m_pfAxes[e_Key_AxeY] = m_fAxeY_Max;
		}
		else
		{
			m_pfAxes[e_Key_AxeY] = p_fY;
		}
}
//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
void CCursor::SetZ(float p_fZ)
{
	m_pfLastAxes[e_Key_AxeZ] = m_pfAxes[e_Key_AxeZ];
	if (p_fZ < 0.0f)
		m_pfAxes[e_Key_AxeZ] = 0.0f;
	else
		if (p_fZ > m_fAxeZ_Max)
		{
			m_pfAxes[e_Key_AxeZ] = m_fAxeZ_Max;
		}
		else
		{
			m_pfAxes[e_Key_AxeZ] = p_fZ;
		}
}
//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
void CCursor::SetSensibility(float p_fSensibility)
{
#define MIN_SENS_MOUSE		0.00001f
	float fMinSens = MIN_SENS_MOUSE;
	m_fSensibility = p_fSensibility;
}


void CCursor::SetDirection(float p_fX, float p_fY, float p_fZ)
{
	m_pfDirection[0] = p_fX;
	m_pfDirection[1] = p_fY;
	m_pfDirection[2] = p_fZ;
}
//-------------------------------------------------------------------------------
///
//-------------------------------------------------------------------------------
CCursor& CCursor:: operator=(const CCursor& p_Module)
{

	m_pfAxes[e_Key_AxeX] = p_Module.m_pfAxes[e_Key_AxeX];
	m_pfAxes[e_Key_AxeY] = p_Module.m_pfAxes[e_Key_AxeY];
	m_pfAxes[e_Key_AxeZ] = p_Module.m_pfAxes[e_Key_AxeZ];

	m_pfLastAxes[e_Key_AxeX] = p_Module.m_pfLastAxes[e_Key_AxeX];
	m_pfLastAxes[e_Key_AxeY] = p_Module.m_pfLastAxes[e_Key_AxeY];
	m_pfLastAxes[e_Key_AxeZ] = p_Module.m_pfLastAxes[e_Key_AxeZ];

	m_pfDirection[e_Key_AxeX] = p_Module.m_pfDirection[e_Key_AxeX];
	m_pfDirection[e_Key_AxeY] = p_Module.m_pfDirection[e_Key_AxeY];
	m_pfDirection[e_Key_AxeZ] = p_Module.m_pfDirection[e_Key_AxeZ];

	return *this;
}


CMouse::CMouse() : CInputDevice(INPUT_DEVICE_MOUSE)
{
#ifdef _WIN32
	m_pDev = NULL;
	if (ms_pDirectInputDevice->CreateDevice(GUID_SysMouse, &m_pDev, NULL) != DI_OK)
		return;

	if (m_pDev == NULL)
		return;

	// Set mouse format.
	if (m_pDev->SetDataFormat(&c_dfDIMouse2) != DI_OK)
		return;

	SetLockToScreen(false);

	m_pDev->Acquire();
#else
	m_fPosX = m_fPosY = 0.f;
#endif
}


void	CMouse::SetLockToScreen(bool p_bLock)
{
	DWORD dwMode = DISCL_NONEXCLUSIVE | DISCL_FOREGROUND;

	m_bLocked = p_bLock;

	if (m_pDev == NULL)
		return;

	if (m_bLocked)
		dwMode = DISCL_EXCLUSIVE | DISCL_FOREGROUND;

	m_pDev->Unacquire();
	HRESULT res = m_pDev->SetCooperativeLevel((HWND)CWindow::GetMainWindow()->GetHandle(), dwMode);
	ASSERT(res == DI_OK);
	m_pDev->Acquire();
}


CMouse::~CMouse()
{
	if (m_pDev != NULL)
	{
		m_pDev->Unacquire();
		m_pDev->Release();
	}
}


float CMouse::GetPressure(int p_nIdKey)
{
	assert(p_nIdKey < e_NbKey);

	float fPression = 0.0f;
	if (p_nIdKey >= e_NbAxis)
	{
		fPression = GetNewInterpolation(0.0f, 1.0f, 0.0f, ms_fPressureMaxReturned, (float)m_pcKeyPressure[p_nIdKey - e_NbAxis]);
	}
	else
	{
		fPression = GetNewInterpolation(0.0f, 65535.0f, 0.0f, ms_fPressureMaxReturned, (float)m_pfKeyPressure[p_nIdKey]) * m_pMouseCursor.GetSensibility();
	}

	return fPression;
}


void CMouse::Process()
{
	if (m_pDev == NULL)
		return;

	memset(&m_MouseState, 0, sizeof(DIMOUSESTATE2));
	for (int i = 0; i < e_NbButton; ++i)
		m_pcKeyPressure[i] = 0;
	for (int i = 0; i < e_NbAxis; ++i)
		m_pfKeyPressure[i] = 0.0f;

	HRESULT hRes = m_pDev->GetDeviceState(sizeof(DIMOUSESTATE2), &m_MouseState);

	//when mouse go out of the screen
	if (hRes == DIERR_INPUTLOST || hRes == 0x8007000c)
	{
		m_pDev->Unacquire();
		m_pDev->Acquire();
		hRes = m_pDev->GetDeviceState(sizeof(DIMOUSESTATE2), &m_MouseState);
	}

	if (m_MouseState.lX > 0.0f)
	{
		int nTdop = 0;
	}

	float fNewValue = m_pMouseCursor.GetX() + m_MouseState.lX * m_pMouseCursor.GetSensibility();
	m_pMouseCursor.SetX(fNewValue);

	fNewValue = m_pMouseCursor.GetY() + m_MouseState.lY * m_pMouseCursor.GetSensibility();
	m_pMouseCursor.SetY(fNewValue);


	m_pfKeyPressure[(int)CMouse::e_Button_MouseGoRight] = GetXStep() > 0.0f ? GetXStep() : 0.0f;
	m_pfKeyPressure[(int)CMouse::e_Button_MouseGoLeft] = GetXStep() < 0.0f ? -GetXStep() : 0.0f;
	m_pfKeyPressure[(int)CMouse::e_Button_MouseGoDown] = GetYStep() > 0.0f ? GetYStep() : 0.0f;
	m_pfKeyPressure[(int)CMouse::e_Button_MouseGoUp] = GetYStep() < 0.0f ? -GetYStep() : 0.0f;

	m_pcKeyPressure[(int)CMouse::e_Button_Next - e_NbAxis] = m_MouseState.lZ != 0 ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_LeftClick - e_NbAxis] = m_MouseState.rgbButtons[0] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_RightClick - e_NbAxis] = m_MouseState.rgbButtons[1] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_WheelClick - e_NbAxis] = m_MouseState.rgbButtons[2] ? 1 : 0;

	m_pcKeyPressure[(int)CMouse::e_Button_0 - e_NbAxis] = m_MouseState.rgbButtons[0] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_1 - e_NbAxis] = m_MouseState.rgbButtons[1] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_2 - e_NbAxis] = m_MouseState.rgbButtons[2] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_3 - e_NbAxis] = m_MouseState.rgbButtons[3] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_4 - e_NbAxis] = m_MouseState.rgbButtons[4] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_5 - e_NbAxis] = m_MouseState.rgbButtons[5] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_6 - e_NbAxis] = m_MouseState.rgbButtons[6] ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_7 - e_NbAxis] = m_MouseState.rgbButtons[7] ? 1 : 0;

	m_pcKeyPressure[(int)CMouse::e_Button_WheelUp - e_NbAxis] = (m_MouseState.lZ > 0) ? 1 : 0;
	m_pcKeyPressure[(int)CMouse::e_Button_WheelDown - e_NbAxis] = (m_MouseState.lZ < 0) ? 1 : 0;


	// nouvelle facon de gerer la position de la souris, on utilise la position de WINDOWS
	POINT point;
	GetCursorPos(&point);
	ScreenToClient((HWND)CWindow::GetMainWindow()->GetHandle(), &point);

	if (m_bLocked)
		LockPointToScreen(&point);

	float fPosX = (float)point.x / CDeviceManager::GetDeviceWidth() * 640.0f;
	float fPosY = (float)point.y / CDeviceManager::GetDeviceHeight() * 480.0f;

	m_pMouseCursor.SetX(fPosX);
	m_pMouseCursor.SetY(fPosY);
}


float	CMouse::GetXStep(void) 
{ 
	return (float)m_MouseState.lX; 
}

float	CMouse::GetYStep(void) 
{ 
	return (float)m_MouseState.lY; 
}

void	CMouse::SetX(float p_fPosx) 
{ 
	m_pMouseCursor.SetX(p_fPosx); 
}

void	CMouse::SetY(float p_fPosy) 
{ 
	m_pMouseCursor.SetY(p_fPosy); 
}


void CMouse::LockPointToScreen(POINT* p_pPointClient)
{
	int nDisplayWidth = CDeviceManager::GetDeviceWidth();
	int nDisplayHeight = CDeviceManager::GetDeviceHeight();

	if (p_pPointClient->x < 0)
	{
		p_pPointClient->x = 0;
	}
	if (p_pPointClient->y < 0)
	{
		p_pPointClient->y = 0;
	}
	if (p_pPointClient->x > nDisplayWidth)
	{
		p_pPointClient->x = nDisplayWidth;
	}
	if (p_pPointClient->y > nDisplayHeight)
	{
		p_pPointClient->y = nDisplayHeight;
	}

	POINT point = *p_pPointClient;

	ClientToScreen((HWND)CWindow::GetMainWindow()->GetHandle(), &point);
	SetCursorPos(point.x, point.y);
}


void CMouse::DestroyAll()
{
	for (int i = 0; i < e_MaxNbMouse; i++)
		if (ms_ppCurrent[i] != nullptr)
		{
			delete ms_ppCurrent[i];
			ms_ppCurrent[i] = nullptr;
		}
}


void CMouse::GetPos(float* fPosX, float* fPosY)
{
	*fPosX = GetXPos() / 640.f;
	*fPosY = GetYPos() / 480.f;
}


void CMouse::GetLastPos(float* fPosX, float* fPosY)
{
	*fPosX = GetLastXPos() / 640.f;
	*fPosY = GetLastYPos() / 480.f;
}


void CMouse::SetPos(float fPosX, float fPosY)
{
	SetX(fPosX * 640.f);
	SetY(fPosY * 480.f);
}


bool CMouse::IsPressed(EKey eButton)
{
	return GetPressure(eButton) > 0.1f;
}
