
#include "Assert.h"

#ifdef ENABLE_CHECKS

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __cplusplus
bool g_bIgnoreAllAssert = false;
bool g_bAssertInTest = false;
bool g_bAssertOtherThreadsRedirectToMainThread = false;
bool g_bAssertBreakOnRetry = false;
SendCallstack g_pSendCallstack = nullptr;
#endif


#include "Windows.h"
#include "DbgHelp.h"
#include <WinBase.h>
#include <stdlib.h>
#include "Mutex.h"
#include "Engine/Renderer/Window/Window.h"
#include "../Threads/Thread.h"
#pragma comment(lib, "Dbghelp.lib")

#define ASSERT_THREADSAFE

#ifdef ASSERT_THREADSAFE

class CAssertContext
{
public:
	static CMutex* GetMutex()
	{
		static CMutex* pMutex = CMutex::Create();

		return pMutex;
	}

	~CAssertContext()
	{
		delete GetMutex();
	}
};

static CAssertContext gs_AssertContext;

#define SCOPED_LOCK CScopedLock lock(CAssertContext::GetMutex());

#else

#define SCOPED_LOCK 

#endif


#define MAX_NB_ADDRESS_TO_IGNORE	100

struct SAssertToIgnoreDescription
{
	ULONG64 adressToIgnore;
	DWORD	lineToIgnore;
};

SYMBOL_INFO g_symbol[256];
SAssertToIgnoreDescription g_AssertToIgnore[MAX_NB_ADDRESS_TO_IGNORE];
int g_AssertToIgnoreNb = 0;

#define NB_STACKFRAMES_TO_IGNORE 2

struct CMutexStaticHolder
{
	// Used to avoid a leak.
	// We don't want to force clients to call an Init/Destroy function,
	// so a static mutex pointer created at startup won't do it, since it'll never
	// be destroyed. With this holder, the runtime will call the destructor at shutdown.
	CMutex* m_pMutex;
	CMutexStaticHolder() { m_pMutex = CMutex::Create(); }
	~CMutexStaticHolder() { delete m_pMutex; }
};

CMutexStaticHolder gs_pOtherThreadAssertMutex;
char gs_pcAssertMessages[8][4096] = { 0 };


bool GetToIgnore_AssertTool()
{
	SCOPED_LOCK

		void         * stack[100];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;
	SAssertToIgnoreDescription	currentAddress;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	frames = CaptureStackBackTrace(0, 100, stack, NULL);
	symbol = g_symbol;
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);


	DWORD  dwDisplacement;
	IMAGEHLP_LINE64 line;
	SymFromAddr(process, (DWORD64)(stack[NB_STACKFRAMES_TO_IGNORE]), 0, symbol);
	SymGetLineFromAddr64(process, (DWORD64)(stack[NB_STACKFRAMES_TO_IGNORE]), &dwDisplacement, &line);

	currentAddress.adressToIgnore = symbol->Address;
	currentAddress.lineToIgnore = line.LineNumber;

	bool bIgnore = false;
	for (int i = 0; i < g_AssertToIgnoreNb; i++)
	{
		if (g_AssertToIgnore[i].adressToIgnore == currentAddress.adressToIgnore
			&&	g_AssertToIgnore[i].lineToIgnore == currentAddress.lineToIgnore)
			bIgnore = true;

		if (bIgnore)
			break;
	}

	return bIgnore;
}

void PrintStack_AssertTool(char *p_pcOut, unsigned int p_nMaxSize)
{
	SCOPED_LOCK

	unsigned int   i;
	void         * stack[100];
	char		   pCharTemp[1024];
	unsigned short frames;
	SYMBOL_INFO  * symbol;
	HANDLE         process;

	process = GetCurrentProcess();
	SymInitialize(process, NULL, TRUE);
	frames = CaptureStackBackTrace(0, 100, stack, NULL);
	symbol = g_symbol;
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

	sprintf(pCharTemp, "\n");
	strncat(p_pcOut, pCharTemp, p_nMaxSize);

	for (i = 0; i < frames; i++)
	{
		DWORD  dwDisplacement;
		IMAGEHLP_LINE64 line;
		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
		SymGetLineFromAddr64(process, (DWORD64)(stack[i]), &dwDisplacement, &line);

		if (i >= NB_STACKFRAMES_TO_IGNORE)
		{
			sprintf(pCharTemp, "\t %i: %s - (l.%i)\n", frames - i - 1, symbol->Name, line.LineNumber);
			strncat(p_pcOut, pCharTemp, p_nMaxSize);

			if (i == NB_STACKFRAMES_TO_IGNORE)
			{
				g_AssertToIgnore[g_AssertToIgnoreNb].adressToIgnore = symbol->Address;
				g_AssertToIgnore[g_AssertToIgnoreNb].lineToIgnore = line.LineNumber;
				g_AssertToIgnoreNb++;
			}
		}
	}
}




#include <atomic>
std::atomic<int> g_nAssertPopupCounter{ 0 };

int AddInBoxWithType(unsigned int nBoxType, const char *s)
{
#ifdef ENABLE_CHECKS
	if (g_bAssertInTest)
	{
		//if in test just assert no pop up
		ASSERT_FAILED(s);
		return -1;
	}
#endif

#ifdef _WIN32
	return MessageBox((HWND)CWindow::GetMainWindow()->GetHandle(), s, "Message", nBoxType);
#endif

	return /*IDOK*/ 1;
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
bool AssertFailed(const char *p_pSourceFile, int p_nLineNumber, const char* p_pcExpression, const char* p_pcMsg /*= nullptr*/)
{
	bool bHaveToBreak = false;

	bool bIgnore = GetToIgnore_AssertTool();

	if (!bIgnore)
	{
		char pError[4096];
		if (p_pcMsg)
			sprintf(pError, "Assert from: %s (%d) \n\n%s\n\n Expression : %s\n", p_pSourceFile, p_nLineNumber, p_pcMsg, p_pcExpression);
		else
			sprintf(pError, "Assert from: %s (%d) \n \n Expression : %s\n", p_pSourceFile, p_nLineNumber, p_pcExpression);

		PrintStack_AssertTool(pError, sizeof(pError));

		if (g_bAssertOtherThreadsRedirectToMainThread == false || CThread::IsThisThreadMainThread())
		{
			g_nAssertPopupCounter += 1;
			int nMessageReturn = AddInBoxWithType(0x00000002L /*MB_ABORTRETRYIGNORE*/ | 0x00000010L /*MB_ICONHAND*/ | 0x00001000L /*MB_SYSTEMMODAL*/, pError);

			if (nMessageReturn != 5 /*IDIGNORE*/)
				g_AssertToIgnoreNb--;

			if (nMessageReturn == 4 /*IDRETRY*/ && g_bAssertBreakOnRetry)
			{
				bHaveToBreak = true;
			}

			if (nMessageReturn == 3 /*IDABORT*/)
			{
				bHaveToBreak = true;
			}
		}
		else
		{
			if (IsDebuggerPresent())
			{
				// An assert has been triggered on a thread that is not the main thread.
				// An issue with message box prevents us from displaying them in that case.
				// So we break, and if you want to get the same "ignore feature" as the box
				// you just have to change the value of bIgnoreAllAssertsOfThisType with the debugger.
				bool bIgnoreAllAssertsOfThisType = false;
				bHaveToBreak = true;
				if (!bIgnoreAllAssertsOfThisType)
					g_AssertToIgnoreNb--;
			}
			else
			{
				// If not in a debugger, we store the message to be displayed in main thread later with FlushOtherThreadAsserts
				g_AssertToIgnoreNb--; // "Ignore" feature will not work there.
				CScopedLock lock(gs_pOtherThreadAssertMutex.m_pMutex);
				for (int i = 0; i < lengthof(gs_pcAssertMessages); ++i)
				{
					if (gs_pcAssertMessages[i][0] == '\0')
					{
						strcpy(gs_pcAssertMessages[i], pError);
						break;
					}
				}
			}
		}
	}

	return bHaveToBreak;
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void FlushOtherThreadAsserts()
{
#ifdef EKOPLF_PC_DEFINE
	ASSERT(g_bAssertOtherThreadsRedirectToMainThread);
	CScopedLock lock(gs_pOtherThreadAssertMutex.m_pMutex);
	for (int i = 0; i < lengthof(gs_pcAssertMessages); ++i)
	{
		if (gs_pcAssertMessages[i][0] == '\0')
			break;
		int nDialogRet = CintiaEngine::CDebugLog::AddInBoxWithType(0x00000001L /*MB_OKCANCEL*/ | 0x00000010L /*MB_ICONHAND*/ | 0x00001000L /*MB_SYSTEMMODAL*/, gs_pcAssertMessages[i]);
		if (nDialogRet == IDCANCEL)
			exit(-1);
		gs_pcAssertMessages[i][0] = '\0';
	}
#endif
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
bool CheckDebuggerAttached()
{
#if ASSERT_TYPES != 2
# if !defined NDEBUG && (defined(EKOPLF_PS4_DEFINE) || defined(EKOPLF_PS5_DEFINE))
	return sceDbgIsDebuggerAttached();
# elif defined _MSC_VER
	return IsDebuggerPresent() != 0;
# else
	return true;
# endif
#else // #if ASSERT_TYPES == 2
	return false;
#endif
}

//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
void AssertFailed_ForTest(const char *p_pSourceFile, int p_nLineNumber, const char* p_pcExpression)
{
#if defined EKOPLF_PC_DEFINE
	char pError[CALLSTACK_MAX_SIZE];
	Str::sprintf(pError, "Assert from: %s (%d) \n \n Expression : %s\n", p_pSourceFile, p_nLineNumber, p_pcExpression);
	PrintStack_AssertTool(pError, sizeof(pError));
	g_pSendCallstack(pError);
#else
	assert(false);
#endif
}

#endif