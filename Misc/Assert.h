#ifndef ASSERT_H
#define ASSERT_H

#ifdef __cplusplus

#include "Engine/Engine.h"
#include <assert.h>

#define CALLSTACK_MAX_SIZE	4096

extern bool g_bIgnoreAllAssert;
extern bool g_bAssertInTest;
extern bool g_bAssertBreakOnRetry;

void PrintStackASSERTTool(char *p_pcOut, unsigned int p_nMaxSize);

typedef void (SendCallstackDefinition)(const char *p_pErrorAndCallstack);
typedef SendCallstackDefinition* SendCallstack;
extern SendCallstack g_pSendCallstack;


//-------------------------------------------------------------------------------
//
//-------------------------------------------------------------------------------
#ifdef ENABLE_CHECKS
#	define ASSERT_TYPES 0
#else
#	define ASSERT_TYPES 2
#endif

#if defined(_PREFAST_)
#	define AnalysisAssume(x) __assume(x)
#else
#	define AnalysisAssume(x) (void)(0)
#endif

///============================================================================
///====== Assert
#if ASSERT_TYPES != 2
bool AssertFailed(const char *p_pSourceFile, int p_nLineNumber, const char* p_pcExpression, const char* p_pcMsg = nullptr);
void FlushOtherThreadAsserts();
bool CheckDebuggerAttached();
#else
inline bool AssertFailed(const char *p_pSourceFile, int p_nLineNumber, const char* p_pcExpression, const char* p_pcMsg = nullptr) { return false; }
#endif

#if defined(_MSC_VER)
#define ASSERT_BREAK_CODE	do { \
	if (CheckDebuggerAttached()) \
		__debugbreak(); \
} while(false)
#else
#define ASSERT_BREAK_CODE	do { \
	(void)0; \
} while(false)
#endif

#if ASSERT_TYPES == 0
#define assert_bis(_Expression, _ExpressionTxt) (void)( (!!(_Expression)) || (ASSERT(_ExpressionTxt, _CRT_WIDE(__FILE__), __LINE__), 0) )
#define assert_ter(_Expression, _ExpressionTxt) (void)( (!!(_Expression)) || (_wassert(_CRT_WIDE(#_ExpressionTxt), _CRT_WIDE(__FILE__), __LINE__), 0) )

///============================================================================
///====== AssertForTest
void AssertFailed_ForTest(const char *p_pSourceFile, int p_nLineNumber, const char* p_pcExpression);
#define ASSERT_FAILED(x) do { \
									if(g_bIgnoreAllAssert){	\
										(void)0;	\
									}	\
									else if (!g_bAssertInTest || g_pSendCallstack == nullptr) \
									{ \
										if (AssertFailed(__FILE__, __LINE__, x)) \
											ASSERT_BREAK_CODE; \
									} \
									else \
										AssertFailed_ForTest(__FILE__, __LINE__, x); \
									} while(false)
#define ASSERT_FAILED_MSG(x, msg) do { \
										if (AssertFailed(__FILE__, __LINE__, x, msg)) \
											ASSERT_BREAK_CODE; \
									} while(false)
#elif ASSERT_TYPES == 1
#define ASSERT_FAILED(x) do { \
										if (AssertFailed(__FILE__, __LINE__, x)) \
											ASSERT_BREAK_CODE; \
									} while(false)
#define ASSERT_FAILED_MSG(x, msg) do { \
										if (AssertFailed(__FILE__, __LINE__, x, msg)) \
											ASSERT_BREAK_CODE; \
									} while(false)
#endif

// ASSERT_TYPES:
//		0: Home brewed asserts for PC
//		1: Home brewed asserts
//		2: No asserts
#if ASSERT_TYPES == 0 || ASSERT_TYPES == 1
#define ASSERT(x) do { \
						if (g_bIgnoreAllAssert || !!(x)) \
							(void)0; \
						else \
							ASSERT_FAILED(#x); \
						AnalysisAssume(!!(x)); \
					} while(false)

#define ASSERT_NOTNULL(x) do { \
								if (g_bIgnoreAllAssert || (x != nullptr)) \
									(void)0; \
								else \
									ASSERT_FAILED(#x); \
								AnalysisAssume(x != nullptr); \
							} while(false)

#define STATICASSERT(x) staticASSERT(x, #x)

#define	ASSERT_MSG(x, ...)  do { \
									if (g_bIgnoreAllAssert || !!(x)) \
										(void)0; \
									else { \
										char pcAssertMsg[1024]; \
										sprintf(pcAssertMsg, __VA_ARGS__); \
										ASSERT_FAILED_MSG(#x, pcAssertMsg); \
									} \
								AnalysisAssume(!!(x)); \
								} while(false)
#elif ASSERT_TYPES == 2
#define	ASSERT(x) (void)(0)
#define ASSERT_NOTNULL(x) (void)(0)
#define ASSERT_FAILED(x) (void)(0)
#define STATICASSERT(x) staticASSERT(x, #x)
#define	ASSERT_MSG(x, ...) (void)(0)
#else
#error "Unsupported ASSERT_TYPES value"
#endif

#if ASSERT_TYPES != 2
#define ASSERT_FAILED_MESSAGE(...)				\
	{														\
		if(g_bIgnoreAllAssert)								\
			(void)0;										\
		else {												\
			char pcAssertMsg[1024];							\
			sprintf(pcAssertMsg, __VA_ARGS__);	\
			ASSERT_FAILED(pcAssertMsg);				\
		}													\
	}
#else
#define ASSERT_FAILED_MESSAGE(...)	(void)(0)
#endif

#else // !__cplusplus
#include <assert.h>
#define ASSERT(x) assert(x)
#define ASSERT_NOTNULL(x) assert(x != nullptr)
#define STATICASSERT(x) staticASSERT(x, #x)
#endif


#if !defined (EKOPLF_PS4_DEFINE) && !defined (CE_PS3_PLATFORM) && !defined (EKOPLF_PSP2_DEFINE) && !defined EKOPLF_NX_DEFINE && !defined(EKOPLF_PS5_DEFINE)
#define EKOPLF_HAS_RTTI
#endif

#ifdef __cplusplus
template<class T, class U>
T checked_static_cast(U* p_pObject)
{
#ifdef EKOPLF_HAS_RTTI
	ASSERT(p_pObject == nullptr || dynamic_cast<T> (p_pObject) != nullptr);
#endif
	return static_cast<T>(p_pObject);
}

template<class T, class U>
T& checked_static_cast(U& p_object)
{
#ifdef EKOPLF_HAS_RTTI
	ASSERT(dynamic_cast<T*> (&p_object) != nullptr);
#endif
	return *(static_cast<T*>(&p_object));
}
#endif

#ifndef AssertNotReached
#define AssertNotReached() ASSERT(false)
#endif

#ifndef AssertNotImplemented
#define AssertNotImplemented() ASSERT(false)
#endif


#endif	// ASSERT_H
