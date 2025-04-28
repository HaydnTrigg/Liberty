#pragma once

#include "typedefs.h"

DACOM_DEC extern "C" __declspec(noreturn) void explode();

// These macros are here so that macro definitions are refer to the severity levels by name, while
// the C++ code properly uses an enum.
#define __SEV_FATAL 0
#define __SEV_ERROR 1
#define __SEV_WARNING 2
#define __SEV_NOTICE 3
#define __SEV_TRACE_1 4
#define __SEV_TRACE_2 5
#define __SEV_TRACE_3 6
#define __SEV_TRACE_4 7
#define __SEV_TRACE_5 8

#ifndef DA_ERROR_LEVEL
#define DA_ERROR_LEVEL __SEV_TRACE_5
#endif

enum SeverityLevel
{
	SEV_FATAL = __SEV_FATAL, // Unrecoverable errors
	SEV_ERROR = __SEV_ERROR, // Recoverable errors
	SEV_WARNING = __SEV_WARNING, // Conditions which may indicate a problem, but are handled
	SEV_NOTICE = __SEV_NOTICE, // Useful messages that are in release builds.
	SEV_TRACE_1 = __SEV_TRACE_1, // Information useful for debugging or determining program flow
	SEV_TRACE_2 = __SEV_TRACE_2, // Higher traces levels indicate more verbose information
	SEV_TRACE_3 = __SEV_TRACE_3,
	SEV_TRACE_4 = __SEV_TRACE_4,
	SEV_TRACE_5 = __SEV_TRACE_5, // Maximum verbosity
};

// Standard error kinds, used by the libraries
#define ERR_LIB_BASE 65536
enum ErrorKind
{
	ERR_MISSION,
	ERR_NETWORK,
	ERR_RULES,
	ERR_GENERAL = ERR_LIB_BASE, // Catch-all error kind, unclassified
	ERR_FILE, // File related errors
	ERR_MEMORY, // Memory related errors
	ERR_CORRUPT, // Corrupted data structure, bad data
	ERR_ASSERT, // failed assertion
	ERR_BADPARAM, // bad parameters to function call (programmer error)
	ERR_PERFORMANCE, // something could be done better
	ERR_PARSER, // Used by parser in Docuview.dll 
	ERR_UNREACHABLE,
	ERR_NOT_IMPLEMENTED
};

#define ERR_SEV_BITS 4
#define ERR_KIND_BITS (32 - ERR_SEV_BITS)

struct ErrorCode
{
#if 0
	SeverityLevel severity : ERR_SEV_BITS;
	ErrorKind kind : ERR_KIND_BITS;
#else
	// This is for reverse engineering allowing a cast to ErrorCode from an integer
	union
	{
		struct
		{
			SeverityLevel severity : ERR_SEV_BITS;
			ErrorKind kind : ERR_KIND_BITS;
		};
		DWORD internal;
	};
	ErrorCode(DWORD internal_) : internal(internal_) {}
#endif

	ErrorCode() {}
	ErrorCode(ErrorKind _kind, SeverityLevel _severity)
	{
		severity = _severity;
		kind = _kind;
	}
	ErrorCode(unsigned int _kind, SeverityLevel _severity)
	{
		severity = _severity;
		kind = static_cast<ErrorKind>(_kind);
	}
};

extern "C"
{
	typedef int(__cdecl* DA_ERROR_HANDLER) (ErrorCode code, const C8* fmt, ...);
	extern DACOM_DEC DA_ERROR_HANDLER FDUMP;
	extern DACOM_DEC DA_ERROR_HANDLER _FDUMP;
}

// *** WARNING!!! Ugly macro and conditional compilation code beyond this point.
// *** READ AT YOUR OWN RISK!

// Common macro definitions

#define IDE_ERR_FMT "%s(%d) : "

// General errors. All you get is a string.
#define GENERAL_FMT "General:%s"

// Usage: GENERAL_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *str)
// Example: GENERAL_ERROR ("A bad thing happened!");

#define GENERAL_FATAL(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_FATAL), IDE_ERR_FMT "FATAL:" GENERAL_FMT, __FILE__, __LINE__, reason)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define GENERAL_ERROR(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_ERROR), IDE_ERR_FMT "ERROR:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_ERROR(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define GENERAL_WARNING(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_WARNING), IDE_ERR_FMT "WARNING:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_WARNING(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_NOTICE
#define GENERAL_NOTICE(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_NOTICE), IDE_ERR_FMT "NOTICE:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_NOTICE(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define GENERAL_TRACE_1(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_TRACE_1), IDE_ERR_FMT "TRACE_1:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_TRACE_1(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define GENERAL_TRACE_2(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_TRACE_2), IDE_ERR_FMT "TRACE_2:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_TRACE_2(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define GENERAL_TRACE_3(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_TRACE_3), IDE_ERR_FMT "TRACE_3:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_TRACE_3(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define GENERAL_TRACE_4(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_TRACE_4), IDE_ERR_FMT "TRACE_4:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_TRACE_4(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define GENERAL_TRACE_5(reason) \
	(*_FDUMP) (ErrorCode(ERR_GENERAL, SEV_TRACE_5), IDE_ERR_FMT "TRACE_5:" GENERAL_FMT, __FILE__, __LINE__, reason)
#else
#define GENERAL_TRACE_5(reason) 
#endif

// File errors. You get the filename and the operation (open, close, read, write, or seek)
#define FILE_FMT "File: on file \"%s\": failed on %s"

// Usage: FILE_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *filename, const C8 *operation)
// Example: FILE_ERROR ("datafile.dat", "Seek");

#define FILE_FATAL(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_FATAL), IDE_ERR_FMT "FATAL:" FILE_FMT, __FILE__, __LINE__, file, operation)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define FILE_ERROR(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_ERROR), IDE_ERR_FMT "ERROR:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_ERROR(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define FILE_WARNING(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_WARNING), IDE_ERR_FMT "WARNING:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_WARNING(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define FILE_TRACE_1(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_TRACE_1), IDE_ERR_FMT "TRACE_1:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_TRACE_1(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define FILE_TRACE_2(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_TRACE_2), IDE_ERR_FMT "TRACE_2:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_TRACE_2(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define FILE_TRACE_3(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_TRACE_3), IDE_ERR_FMT "TRACE_3:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_TRACE_3(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define FILE_TRACE_4(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_TRACE_4), IDE_ERR_FMT "TRACE_4:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_TRACE_4(file, operation) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define FILE_TRACE_5(file, operation) \
	(*_FDUMP) (ErrorCode(ERR_FILE, SEV_TRACE_5), IDE_ERR_FMT "TRACE_5:" FILE_FMT, __FILE__, __LINE__, file, operation)
#else
#define FILE_TRACE_5(file, operation) 
#endif

// Memory errors. You get a cause string, a source file name, and a line number
#define MEMORY_FMT "Memory: %s"

// Usage: MEMORY_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *reason)
// Example: MEMORY_ERROR ("Out of memory loading bitmap");

#define MEMORY_FATAL(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_FATAL), IDE_ERR_FMT "FATAL:" MEMORY_FMT, __FILE__, __LINE__, reason)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define MEMORY_ERROR(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_ERROR), IDE_ERR_FMT "ERROR:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_ERROR(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define MEMORY_WARNING(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_WARNING), IDE_ERR_FMT "WARNING:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_WARNING(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define MEMORY_TRACE_1(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_TRACE_1), IDE_ERR_FMT "TRACE_1:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_TRACE_1(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define MEMORY_TRACE_2(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_TRACE_2), IDE_ERR_FMT "TRACE_2:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_TRACE_2(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define MEMORY_TRACE_3(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_TRACE_3), IDE_ERR_FMT "TRACE_3:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_TRACE_3(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define MEMORY_TRACE_4(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_TRACE_4), IDE_ERR_FMT "TRACE_4:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_TRACE_4(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define MEMORY_TRACE_5(reason) \
	(*_FDUMP) (ErrorCode(ERR_MEMORY, SEV_TRACE_5), IDE_ERR_FMT "TRACE_5:" MEMORY_FMT, __FILE__, __LINE__, reason)
#else
#define MEMORY_TRACE_5(reason) 
#endif

// Corruption errors. You get a context string, a source file name, and a line number
#define CORRUPT_FMT "Corrupt Data: %s"

// Usage: CORRUPT_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *reason)
// Example: CORRUPT_ERROR ("Object database");

#define CORRUPT_FATAL(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_FATAL), IDE_ERR_FMT "FATAL:" CORRUPT_FMT, __FILE__, __LINE__, reason)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define CORRUPT_ERROR(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_ERROR), IDE_ERR_FMT "ERROR:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_ERROR(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define CORRUPT_WARNING(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_WARNING), IDE_ERR_FMT "WARNING:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_WARNING(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define CORRUPT_TRACE_1(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_TRACE_1), IDE_ERR_FMT "TRACE_1:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_TRACE_1(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define CORRUPT_TRACE_2(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_TRACE_2), IDE_ERR_FMT "TRACE_2:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_TRACE_2(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define CORRUPT_TRACE_3(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_TRACE_3), IDE_ERR_FMT "TRACE_3:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_TRACE_3(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define CORRUPT_TRACE_4(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_TRACE_4), IDE_ERR_FMT "TRACE_4:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_TRACE_4(reason) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define CORRUPT_TRACE_5(reason) \
	(*_FDUMP) (ErrorCode(ERR_CORRUPT, SEV_TRACE_5), IDE_ERR_FMT "TRACE_5:" CORRUPT_FMT, __FILE__, __LINE__, reason)
#else
#define CORRUPT_TRACE_5(reason) 
#endif

//--------------------------------------------------------------------
//------------------Assertions and Parameter Checking ----------------
//--------------------------------------------------------------------

// These differ from the previous error reporting functions by only reporting if the given
// expression fails.

// Inclusion of the standard C++ library assert function.
// It is done this way to prevent having to update the spec of the system
// assertion function or macro with different compiler versions. -TNB
#include <assert.h>

// Assertion errors. You get a source file name, a line, and an expression string
#define ASSERT_FMT "Assertion failed: %s"

// Usage: ASSERT_({FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}},expression)
// Example: ASSERT_FATAL(ptr != NULL);

#define ASSERT_DEF(sev,expr) \
if (!(expr)) \
{ \
	(*_FDUMP) (ErrorCode(ERR_ASSERT, SEV_##sev), IDE_ERR_FMT #sev ":" ASSERT_FMT, __FILE__, __LINE__, #expr); \
	__debugbreak(); \
} ((void)0)

#define ASSERT_FATAL(expr) ASSERT_DEF(FATAL,expr)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define ASSERT_ERROR(expr) ASSERT_DEF(ERROR,expr)
#else
#define ASSERT_ERROR(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define ASSERT_WARNING(expr) ASSERT_DEF(WARNING,expr)
#else
#define ASSERT_WARNING(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define ASSERT_TRACE_1(expr) ASSERT_DEF(TRACE_1,expr)
#else
#define ASSERT_TRACE_1(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define ASSERT_TRACE_2(expr) ASSERT_DEF(TRACE_2,expr)
#else
#define ASSERT_TRACE_2(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define ASSERT_TRACE_3(expr) ASSERT_DEF(TRACE_3,expr)
#else
#define ASSERT_TRACE_3(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define ASSERT_TRACE_4(expr) ASSERT_DEF(TRACE_4,expr)
#else
#define ASSERT_TRACE_4(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define ASSERT_TRACE_5(expr) ASSERT_DEF(TRACE_5,expr)
#else
#define ASSERT_TRACE_5(expr) 
#endif

// Bad parameter errors. You get a source file name, a line, and a parameter name
#define BADPARAM_FMT "Bad Parameter: %s"

// Usage: BADPARAM_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *filename, int line, const C8 *pName)
// Example: BADPARAM_ERROR ("instancePtr");

#define BADPARAM_DEF(sev,expr) {\
	if (!(expr)) { \
	(*_FDUMP) (ErrorCode(ERR_BADPARAM, SEV_##sev), IDE_ERR_FMT #sev ":" BADPARAM_FMT, __FILE__, __LINE__, #expr); \
	assert(expr); \
	} \
}

#define BADPARAM_FATAL(expr) BADPARAM_DEF(FATAL,expr)

#if DA_ERROR_LEVEL >= __SEV_ERROR
#define BADPARAM_ERROR(expr) BADPARAM_DEF(ERROR,expr)
#else
#define BADPARAM_ERROR(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_WARNING
#define BADPARAM_WARNING(expr) BADPARAM_DEF(WARNING,expr)
#else
#define BADPARAM_WARNING(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define BADPARAM_TRACE_1(expr) BADPARAM_DEF(TRACE_1,expr)
#else
#define BADPARAM_TRACE_1(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define BADPARAM_TRACE_2(expr) BADPARAM_DEF(TRACE_2,expr)
#else
#define BADPARAM_TRACE_2(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define BADPARAM_TRACE_3(expr) BADPARAM_DEF(TRACE_3,expr)
#else
#define BADPARAM_TRACE_3(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define BADPARAM_TRACE_4(expr) BADPARAM_DEF(TRACE_4,expr)
#else
#define BADPARAM_TRACE_4(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define BADPARAM_TRACE_5(expr) BADPARAM_DEF(TRACE_5,expr)
#else
#define BADPARAM_TRACE_5(expr) 
#endif

// Performance warning. You get a source file name, a line, and a descriptive message
#define PERFORMANCE_FMT "Performance warning: %s"

// Usage: PERFORMANCE_{FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}} (const C8 *filename, int line, const C8 *pName)
// Example: PERFORMANCE_TRACE1 ("Nothing to do!");

#define PERFORMANCE_DEF(sev,expr) \
	(*_FDUMP) (ErrorCode(ERR_PERFORMANCE, SEV_##sev), IDE_ERR_FMT #sev ":" PERFORMANCE_FMT, __FILE__, __LINE__, #expr)

#if DA_ERROR_LEVEL >= __SEV_TRACE_1
#define PERFORMANCE_TRACE_1(expr) PERFORMANCE_DEF(TRACE_1,expr)
#else
#define PERFORMANCE_TRACE_1(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_2
#define PERFORMANCE_TRACE_2(expr) PERFORMANCE_DEF(TRACE_2,expr)
#else
#define PERFORMANCE_TRACE_2(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_3
#define PERFORMANCE_TRACE_3(expr) PERFORMANCE_DEF(TRACE_3,expr)
#else
#define PERFORMANCE_TRACE_3(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_4
#define PERFORMANCE_TRACE_4(expr) PERFORMANCE_DEF(TRACE_4,expr)
#else
#define PERFORMANCE_TRACE_4(expr) 
#endif

#if DA_ERROR_LEVEL >= __SEV_TRACE_5
#define PERFORMANCE_TRACE_5(expr) PERFORMANCE_DEF(TRACE_5,expr)
#else
#define PERFORMANCE_TRACE_5(expr) 
#endif

//--------------------------------------------------------------------
//---------------------DA replacement for ASSERT----------------------
//--------------------------------------------------------------------

// NOTE: ASSERT and ASSERT2 are compiled out for a FINAL_RELEASE build.

#undef ASSERT
#undef ASSERT2

#ifdef FINAL_RELEASE

#define ASSERT(exp) ((void)0)
#define ASSERT2(exp) ((void)0)

#else

#define ASSERT(exp) ASSERT_FATAL(exp)
#define ASSERT2(sev,exp) ASSERT_##sev(exp)

#endif /* FINAL_RELEASE */



//--------------------------------------------------------------------
//------------------Unreachable Errors & Optimization-----------------
//--------------------------------------------------------------------

// Unreachable errors. You get a source file name, a line, and an expression string
#define UNREACHABLE_FMT "Unreachable"

// Usage: UNREACHABLE_({FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}},expression)
// Example: UNREACHABLE_FATAL(ptr != NULL);

#ifdef _DEBUG

#define UNREACHABLE_DEF(sev) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wexceptions\"") \
	do { (*_FDUMP) (ErrorCode(ERR_UNREACHABLE, SEV_##sev), IDE_ERR_FMT #sev ":" UNREACHABLE_FMT, __FILE__, __LINE__); \
	__debugbreak(); explode(); __assume(0); } while(false) \
	_Pragma("clang diagnostic pop")

#else

#define UNREACHABLE_DEF(sev) \
	((*_FDUMP) (ErrorCode(ERR_UNREACHABLE, SEV_##sev), IDE_ERR_FMT #sev ":" UNREACHABLE_FMT, __FILE__, __LINE__), __assume(0))

#endif

#define UNREACHABLE_FATAL() UNREACHABLE_DEF(FATAL)

#ifdef FINAL_RELEASE

#define UNREACHABLE __assume(0)

#else

#define UNREACHABLE UNREACHABLE_FATAL()

#endif /* FINAL_RELEASE */



//--------------------------------------------------------------------
//----------------Not Implemented Errors & Optimization---------------
//--------------------------------------------------------------------

// Not_Implemented errors. You get a source file name, a line, and an expression string
#define NOT_IMPLEMENTED_FMT "Not Implemented"

// Usage: NOT_IMPLEMENTED_({FATAL,ERROR,WARNING,TRACE_{1,2,3,4,5}},expression)
// Example: NOT_IMPLEMENTED_FATAL(ptr != NULL);

#ifdef _DEBUG

#define NOT_IMPLEMENTED_DEF(sev) \
	_Pragma("clang diagnostic push") \
	_Pragma("clang diagnostic ignored \"-Wexceptions\"") \
	do { (*_FDUMP) (ErrorCode(ERR_NOT_IMPLEMENTED, SEV_##sev), IDE_ERR_FMT #sev ":" UNREACHABLE_FMT, __FILE__, __LINE__); \
	__debugbreak(); explode(); __assume(0); } while(false) \
	_Pragma("clang diagnostic pop")

#else

#define NOT_IMPLEMENTED_DEF(sev) \
	((*_FDUMP) (ErrorCode(ERR_NOT_IMPLEMENTED, SEV_##sev), IDE_ERR_FMT #sev ":" NOT_IMPLEMENTED_FMT, __FILE__, __LINE__), __assume(0))

#endif

#define NOT_IMPLEMENTED_FATAL() NOT_IMPLEMENTED_DEF(FATAL)

#ifdef FINAL_RELEASE

#define NOT_IMPLEMENTED __assume(0)

#else

#define NOT_IMPLEMENTED NOT_IMPLEMENTED_FATAL()

#endif /* FINAL_RELEASE */


//--------------------------------------------------------------------
//------------------------DA parameter checking-----------------------
//--------------------------------------------------------------------

// NOTE: CHECKPARAM is compiled out for a FINAL_RELEASE build.

#undef CHECKPARAM

#ifdef USE_CHECKPARAM

#define CHECKPARAM(sev,exp) ((void)0)

#else

#define CHECKPARAM(sev,exp) BADPARAM_##sev(exp)

#endif /* USE_CHECKPARAM */
