//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef DBG_H
#define DBG_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "dbgflag.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef POSIX
#define __cdecl
#endif

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define DBG_INTERFACE	DLL_EXPORT
#define DBG_OVERLOAD	DLL_GLOBAL_EXPORT
#define DBG_CLASS		DLL_CLASS_EXPORT
#else
#define DBG_INTERFACE	DLL_IMPORT
#define DBG_OVERLOAD	DLL_GLOBAL_IMPORT
#define DBG_CLASS		DLL_CLASS_IMPORT
#endif

#else // BUILD_AS_DLL

#define DBG_INTERFACE	extern
#define DBG_OVERLOAD	
#define DBG_CLASS		
#endif // BUILD_AS_DLL


class Color;


//-----------------------------------------------------------------------------
// Usage model for the Dbg library
//
// 1. Spew.
// 
//   Spew can be used in a static and a dynamic mode. The static
//   mode allows us to display assertions and other messages either only
//   in debug builds, or in non-release builds. The dynamic mode allows us to
//   turn on and off certain spew messages while the application is running.
// 
//   Static Spew messages:
//
//     Assertions are used to detect and warn about invalid states
//     Spews are used to display a particular status/warning message.
//
//     To use an assertion, use
//
//     Assert( (f == 5) );
//     AssertMsg( (f == 5), ("F needs to be %d here!\n", 5) );
//     AssertFunc( (f == 5), BadFunc() );
//     AssertEquals( f, 5 );
//     AssertFloatEquals( f, 5.0f, 1e-3 );
//
//     The first will simply report that an assertion failed on a particular
//     code file and line. The second version will display a print-f formatted message 
//	   along with the file and line, the third will display a generic message and
//     will also cause the function BadFunc to be executed, and the last two
//	   will report an error if f is not equal to 5 (the last one asserts within
//	   a particular tolerance).
//
//     To use a warning, use
//      
//     Warning("Oh I feel so %s all over\n", "yummy");
//
//     Warning will do its magic in only Debug builds. To perform spew in *all*
//     builds, use RelWarning.
//
//	   Three other spew types, Msg, Log, and Error, are compiled into all builds.
//	   These error types do *not* need two sets of parenthesis.
//
//	   Msg( "Isn't this exciting %d?", 5 );
//	   Error( "I'm just thrilled" );
//
//   Dynamic Spew messages
//
//     It is possible to dynamically turn spew on and off. Dynamic spew is 
//     identified by a spew group and priority level. To turn spew on for a 
//     particular spew group, use SpewActivate( "group", level ). This will 
//     cause all spew in that particular group with priority levels <= the 
//     level specified in the SpewActivate function to be printed. Use DSpew 
//     to perform the spew:
//
//     DWarning( "group", level, "Oh I feel even yummier!\n" );
//
//     Priority level 0 means that the spew will *always* be printed, and group
//     '*' is the default spew group. If a DWarning is encountered using a group 
//     whose priority has not been set, it will use the priority of the default 
//     group. The priority of the default group is initially set to 0.      
//
//   Spew output
//   
//     The output of the spew system can be redirected to an externally-supplied
//     function which is responsible for outputting the spew. By default, the 
//     spew is simply printed using printf.
//
//     To redirect spew output, call SpewOutput.
//
//     SpewOutputFunc( OutputFunc );
//
//     This will cause OutputFunc to be called every time a spew message is
//     generated. OutputFunc will be passed a spew type and a message to print.
//     It must return a value indicating whether the debugger should be invoked,
//     whether the program should continue running, or whether the program 
//     should abort. 
//
// 2. Code activation
//
//   To cause code to be run only in debug builds, use DBG_CODE:
//   An example is below.
//
//   DBG_CODE(
//				{
//					int x = 5;
//					++x;
//				}
//           ); 
//
//   Code can be activated based on the dynamic spew groups also. Use
//  
//   DBG_DCODE( "group", level,
//              { int x = 5; ++x; }
//            );
//
// 3. Breaking into the debugger.
//
//   To cause an unconditional break into the debugger in debug builds only, use DBG_BREAK
//
//   DBG_BREAK();
//
//	 You can force a break in any build (release or debug) using
//
//	 DebuggerBreak();
//-----------------------------------------------------------------------------

/* Various types of spew messages */
// I'm sure you're asking yourself why SPEW_ instead of DBG_ ?
// It's because DBG_ is used all over the place in windows.h
// For example, DBG_CONTINUE is defined. Feh.
enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,

	SPEW_TYPE_COUNT
};

enum SpewRetval_t
{
	SPEW_DEBUGGER = 0,
	SPEW_CONTINUE,
	SPEW_ABORT
};

/* type of externally defined function used to display debug spew */
typedef SpewRetval_t (*SpewOutputFunc_t)( SpewType_t spewType, const tchar *pMsg );

/* Used to redirect spew output */
typedef void (*_SpewOutputFunc)(SpewOutputFunc_t func);
extern _SpewOutputFunc SpewOutputFunc;

/* Used to get the current spew output function */
typedef SpewOutputFunc_t (*_GetSpewOutputFunc)(void);
extern _GetSpewOutputFunc GetSpewOutputFunc;

/* This is the default spew fun, which is used if you don't specify one */
typedef SpewRetval_t (*_DefaultSpewFunc)(SpewType_t type, const tchar *pMsg);
extern _DefaultSpewFunc DefaultSpewFunc;

/* Same as the default spew func, but returns SPEW_ABORT for asserts */
typedef SpewRetval_t (*_DefaultSpewFuncAbortOnAsserts)(SpewType_t type, const tchar *pMsg);
extern _DefaultSpewFuncAbortOnAsserts DefaultSpewFuncAbortOnAsserts;

/* Should be called only inside a SpewOutputFunc_t, returns groupname, level, color */
typedef const tchar* (*_GetSpewOutputGroup)(void);
extern _GetSpewOutputGroup GetSpewOutputGroup;
typedef int (*_GetSpewOutputLevel)(void);
extern _GetSpewOutputLevel GetSpewOutputLevel;
typedef const Color* (*_GetSpewOutputColor)(void);
extern _GetSpewOutputColor GetSpewOutputColor;

/* Used to manage spew groups and subgroups */
typedef void (*_SpewActivate)(const tchar* pGroupName, int level);
extern _SpewActivate SpewActivate;
typedef bool (*_IsSpewActive)(const tchar* pGroupName, int level);
extern _IsSpewActive IsSpewActive;

/* Used to display messages, should never be called directly. */
typedef void (*__SpewInfo)(SpewType_t type, const tchar* pFile, int line);
extern __SpewInfo _SpewInfo;
typedef SpewRetval_t (*__SpewMessage)(PRINTF_FORMAT_STRING const tchar* pMsg, ...);
extern __SpewMessage _SpewMessage;
typedef SpewRetval_t (*__DSpewMessage)(const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar* pMsg, ...);
extern __DSpewMessage _DSpewMessage;
typedef SpewRetval_t (*_ColorSpewMessage)(SpewType_t type, const Color *pColor, PRINTF_FORMAT_STRING const tchar* pMsg, ...);
extern _ColorSpewMessage ColorSpewMessage;
typedef void (*__ExitOnFatalAssert)(const tchar* pFile, int line);
extern __ExitOnFatalAssert _ExitOnFatalAssert;
typedef bool (*_ShouldUseNewAssertDialog)();
extern _ShouldUseNewAssertDialog ShouldUseNewAssertDialog;

typedef bool (*_SetupWin32ConsoleIO)();
extern _SetupWin32ConsoleIO SetupWin32ConsoleIO;

// Returns true if they want to break in the debugger.
typedef bool (*_DoNewAssertDialog)(const tchar *pFile, int line, const tchar *pExpression);
extern _DoNewAssertDialog DoNewAssertDialog;

// Allows the assert dialogs to be turned off from code
typedef bool (*_AreAllAssertsDisabled)();
extern _AreAllAssertsDisabled AreAllAssertsDisabled;
typedef void (*_SetAllAssertsDisabled)(bool bAssertsEnabled);
extern _SetAllAssertsDisabled SetAllAssertsDisabled;

// Provides a callback that is called on asserts regardless of spew levels
typedef void (*AssertFailedNotifyFunc_t)( const char *pchFile, int nLine, const char *pchMessage );
typedef void (*_SetAssertFailedNotifyFunc)(AssertFailedNotifyFunc_t func);
extern _SetAssertFailedNotifyFunc SetAssertFailedNotifyFunc;
typedef void (*_CallAssertFailedNotifyFunc)(const char *pchFile, int nLine, const char *pchMessage);
extern _CallAssertFailedNotifyFunc CallAssertFailedNotifyFunc;


/* True if -hushasserts was passed on command line. */
typedef bool (*_HushAsserts)();
extern _HushAsserts HushAsserts;

#if defined( USE_SDL )
typedef void (*_SetAssertDialogParent)(struct SDL_Window *window);
extern _SetAssertDialogParent SetAssertDialogParent;
typedef struct SDL_Window * (*_GetAssertDialogParent)();
extern _GetAssertDialogParent GetAssertDialogParent;
#endif

/* Used to define macros, never use these directly. */

#ifdef _PREFAST_
	// When doing /analyze builds define _AssertMsg to be __analysis_assume. This tells
	// the compiler to assume that the condition is true, which helps to suppress many
	// warnings. This define is done in debug and release builds.
	// The unfortunate !! is necessary because otherwise /analyze is incapable of evaluating
	// all of the logical expressions that the regular compiler can handle.
	// Include _msg in the macro so that format errors in it are detected.
	#define _AssertMsg( _exp, _msg, _executeExp, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	// Force asserts on for /analyze so that we get a __analysis_assume of all of the constraints.
	#define DBGFLAG_ASSERT
	#define DBGFLAG_ASSERTFATAL
	#define DBGFLAG_ASSERTDEBUG
#else
	#define  _AssertMsg( _exp, _msg, _executeExp, _bFatal )	\
		do {																\
			if (!(_exp)) 													\
			{ 																\
				_SpewInfo( SPEW_ASSERT, __TFILE__, __LINE__ );				\
				SpewRetval_t retAssert = _SpewMessage("%s", static_cast<const char*>( _msg ));	\
				CallAssertFailedNotifyFunc( __TFILE__, __LINE__, _msg );					\
				_executeExp; 												\
				if ( retAssert == SPEW_DEBUGGER)									\
				{															\
					if ( !ShouldUseNewAssertDialog() || DoNewAssertDialog( __TFILE__, __LINE__, _msg ) ) \
					{														\
						DebuggerBreak();									\
					}														\
					if ( _bFatal )											\
					{														\
						_ExitOnFatalAssert( __TFILE__, __LINE__ );			\
					}														\
				}															\
			}																\
		} while (0)

	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) \
		do {																\
			static bool fAsserted;											\
			if (!fAsserted )												\
			{ 																\
				_AssertMsg( _exp, _msg, (fAsserted = true), _bFatal );		\
			}																\
		} while (0)
#endif

/* Spew macros... */

// AssertFatal macros
// AssertFatal is used to detect an unrecoverable error condition.
// If enabled, it may display an assert dialog (if DBGFLAG_ASSERTDLG is turned on or running under the debugger),
// and always terminates the application

#ifdef DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), true )
#define  AssertFatalOnce( _exp )								_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), true )
#define  AssertFatalMsg( _exp, _msg, ... )						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), true )
#define  AssertFatalMsgOnce( _exp, _msg )						_AssertMsgOnce( _exp, _msg, true )
#define  AssertFatalFunc( _exp, _f )							_AssertMsg( _exp, _T("Assertion Failed: " _T(#_exp), _f, true )
#define  AssertFatalEquals( _exp, _expectedValue )				AssertFatalMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )   AssertFatalMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  VerifyFatal( _exp )									AssertFatal( _exp )
#define  VerifyEqualsFatal( _exp, _expectedValue )				AssertFatalEquals( _exp, _expectedValue )

#define  AssertFatalMsg1( _exp, _msg, a1 )									AssertFatalMsg( _exp, _msg, a1 )
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								AssertFatalMsg( _exp, _msg, a1, a2 )
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							AssertFatalMsg( _exp, _msg, a1, a2, a3 )
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertFatalMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									((void)0)
#define  AssertFatalOnce( _exp )								((void)0)
#define  AssertFatalMsg( _exp, _msg )							((void)0)
#define  AssertFatalMsgOnce( _exp, _msg )						((void)0)
#define  AssertFatalFunc( _exp, _f )							((void)0)
#define  AssertFatalEquals( _exp, _expectedValue )				((void)0)
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  VerifyFatal( _exp )									(_exp)
#define  VerifyEqualsFatal( _exp, _expectedValue )				(_exp)

#define  AssertFatalMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERTFATAL

// Assert macros
// Assert is used to detect an important but survivable error.
// It's only turned on when DBGFLAG_ASSERT is true.

#ifdef DBGFLAG_ASSERT

#define  Assert( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsg( _exp, _msg, ... )  						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), false )
#define  AssertOnce( _exp )       							_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), false )
#define  AssertMsgOnce( _exp, _msg )  						_AssertMsgOnce( _exp, _msg, false )
#define  AssertFunc( _exp, _f )   							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), _f, false )
#define  AssertEquals( _exp, _expectedValue )              	AssertMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFloatEquals( _exp, _expectedValue, _tol )  	AssertMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  Verify( _exp )           							Assert( _exp )
#define  VerifyMsg1( _exp, _msg, a1 )						AssertMsg1( _exp, _msg, a1 )
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					AssertMsg2( _exp, _msg, a1, a2 )
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				AssertMsg3( _exp, _msg, a1, a2, a3 )
#define  VerifyEquals( _exp, _expectedValue )           	AssertEquals( _exp, _expectedValue )
#define  DbgVerify( _exp )           						Assert( _exp )

#define  AssertMsg1( _exp, _msg, a1 )									AssertMsg( _exp, _msg, a1 )
#define  AssertMsg2( _exp, _msg, a1, a2 )								AssertMsg( _exp, _msg, a1, a2 )
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							AssertMsg( _exp, _msg, a1, a2, a3 )
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERT

#define  Assert( _exp )										((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg, ... )						((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define	 VerifyMsg1( _exp, _msg, a1 )						(_exp)
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					(_exp)
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)
#define  DbgVerify( _exp )									(_exp)

#define  AssertMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERT

typedef void (*PrintFunc)(const char* msg, ...);
typedef void (*ColorFunc)(int level, const Color& c, const char* msg, ...);

extern ColorFunc ConColorMsg;
extern PrintFunc ConMsg;
extern PrintFunc Msg;
extern PrintFunc Warning;
extern PrintFunc DevMsg;
extern PrintFunc DevWarning;
inline void Error(...) { std::terminate(); }

// The Always version of the assert macros are defined even when DBGFLAG_ASSERT is not, 
// so they will be available even in release.
#define  AssertAlways( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsgAlways( _exp, _msg )  							_AssertMsg( _exp, _msg, ((void)0), false )

// Stringify a number
#define V_STRINGIFY_INTERNAL(x) #x
// Extra level of indirection needed when passing in a macro to avoid getting the macro name instead of value
#define V_STRINGIFY(x) V_STRINGIFY_INTERNAL(x)

// Macros to help decorate warnings or errors with the location in code
#define FILE_LINE_FUNCTION_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):" __FUNCTION__ ":"
#define FILE_LINE_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):"
#define FUNCTION_LINE_STRING __FUNCTION__ "(" V_STRINGIFY(__LINE__) "): "

// Handy define for inserting clickable messages into the build output.
// Use like this:
// #pragma MESSAGE("Some message")
#define MESSAGE(msg) message(__FILE__ "(" V_STRINGIFY(__LINE__) "): " msg)


// You can use this macro like a runtime assert macro.
// If the condition fails, then Error is called with the message. This macro is called
// like AssertMsg, where msg must be enclosed in parenthesis:
//
// ErrorIfNot( bCondition, ("a b c %d %d %d", 1, 2, 3) );
#define ErrorIfNot( condition, msg ) \
	if ( condition )		\
		;					\
	else 					\
	{						\
		Error msg;			\
	}


typedef void (*_COM_TimestampedLog)(PRINTF_FORMAT_STRING char const *fmt, ...);
extern _COM_TimestampedLog COM_TimestampedLog;

/* Code macros, debugger interface */

#ifdef DBGFLAG_ASSERT

#define DBG_CODE( _code )            if (0) ; else { _code }
#define DBG_CODE_NOSCOPE( _code )	 _code
#define DBG_DCODE( _g, _l, _code )   if (IsSpewActive( _g, _l )) { _code } else {}
#define DBG_BREAK()                  DebuggerBreak()	/* defined in platform.h */ 

#else /* not _DEBUG */

#define DBG_CODE( _code )            ((void)0)
#define DBG_CODE_NOSCOPE( _code )	 
#define DBG_DCODE( _g, _l, _code )   ((void)0)
#define DBG_BREAK()                  ((void)0)

#endif /* _DEBUG */

//-----------------------------------------------------------------------------

#ifndef _RETAIL
class CScopeMsg
{
public:
	CScopeMsg( const char *pszScope )
	{
		m_pszScope = pszScope;
		Msg( "%s { ", pszScope );
	}
	~CScopeMsg()
	{
		Msg( "} %s", m_pszScope );
	}
	const char *m_pszScope;
};
#define SCOPE_MSG( msg ) CScopeMsg scopeMsg( msg )
#else
#define SCOPE_MSG( msg )
#endif


//-----------------------------------------------------------------------------
// Macro to assist in asserting constant invariants during compilation

// This implementation of compile time assert has zero cost (so it can safely be
// included in release builds) and can be used at file scope or function scope.
// We're using an ancient version of GCC that can't quite handle some
// of our complicated templates properly.  Use some preprocessor trickery
// to workaround this
#ifndef __has_feature
  #define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif
#if __has_feature(cxx_static_assert)
	// If available use static_assert instead of weird language tricks. This
	// leads to much more readable messages when compile time assert constraints
	// are violated.
	#define COMPILE_TIME_ASSERT( pred ) static_assert( pred, "Compile time assert constraint is not true: " #pred )
#elif defined __GNUC__
	#define COMPILE_TIME_ASSERT( pred ) typedef int UNIQUE_ID[ (pred) ? 1 : -1 ] __attribute__((unused))
#else
	#if _MSC_VER >= 1600
	// If available use static_assert instead of weird language tricks. This
	// leads to much more readable messages when compile time assert constraints
	// are violated.
	#define COMPILE_TIME_ASSERT( pred ) static_assert( pred, "Compile time assert constraint is not true: " #pred )
	#else
	// Due to gcc bugs this can in rare cases (some template functions) cause redeclaration
	// errors when used multiple times in one scope. Fix by adding extra scoping.
	#define COMPILE_TIME_ASSERT( pred ) typedef char compile_time_assert_type[(pred) ? 1 : -1];
	#endif
#endif
// ASSERT_INVARIANT used to be needed in order to allow COMPILE_TIME_ASSERTs at global
// scope. However the new COMPILE_TIME_ASSERT macro supports that by default.
#define ASSERT_INVARIANT( pred )	COMPILE_TIME_ASSERT( pred )


#ifdef _DEBUG
template<typename DEST_POINTER_TYPE, typename SOURCE_POINTER_TYPE>
inline DEST_POINTER_TYPE assert_cast(SOURCE_POINTER_TYPE* pSource)
{
    Assert( static_cast<DEST_POINTER_TYPE>(pSource) == dynamic_cast<DEST_POINTER_TYPE>(pSource) );
    return static_cast<DEST_POINTER_TYPE>(pSource);
}
#else
#define assert_cast static_cast
#endif

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:

// uncrafted - these don't do anything in the release build of tier0.dll anyway
FORCEINLINE void _AssertValidReadPtr(void* ptr, int count = 1) {}
FORCEINLINE void _AssertValidWritePtr(void* ptr, int count = 1) {}
FORCEINLINE void _AssertValidReadWritePtr(void* ptr, int count = 1) {}
FORCEINLINE void AssertValidStringPtr(void* ptr, int maxchar) {}

template<class T> inline void AssertValidReadPtr( T* ptr, int count = 1 )		     { _AssertValidReadPtr( (void*)ptr, count ); }
template<class T> inline void AssertValidWritePtr( T* ptr, int count = 1 )		     { _AssertValidWritePtr( (void*)ptr, count ); }
template<class T> inline void AssertValidReadWritePtr( T* ptr, int count = 1 )	     { _AssertValidReadWritePtr( (void*)ptr, count ); }
#define AssertValidStringPtr AssertValidReadPtr

#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

//-----------------------------------------------------------------------------
// Macro to protect functions that are not reentrant

#define ASSERT_NO_REENTRY()

//-----------------------------------------------------------------------------
//
// Purpose: Inline string formatter
//

#include "tier0/valve_off.h"
class CDbgFmtMsg
{
public:
	CDbgFmtMsg(PRINTF_FORMAT_STRING const tchar *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{ 
		va_list arg_ptr;

		va_start(arg_ptr, pszFormat);
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, pszFormat, arg_ptr);
		va_end(arg_ptr);

		m_szBuf[sizeof(m_szBuf)-1] = 0;
	}

	operator const tchar *() const				
	{ 
		return m_szBuf; 
	}

private:
	tchar m_szBuf[256];
};
#include "tier0/valve_on.h"

//-----------------------------------------------------------------------------
//
// Purpose: Embed debug info in each file.
//
#if defined( _WIN32 ) && !defined( _X360 )

	#ifdef _DEBUG
		#pragma comment(compiler)
	#endif

#endif

//-----------------------------------------------------------------------------
//
// Purpose: Wrap around a variable to create a simple place to put a breakpoint
//


template< class Type >
class CDataWatcher
{
private:
	CDataWatcher(); // refuse to compile in non-debug builds
};

//-----------------------------------------------------------------------------

#endif /* DBG_H */
