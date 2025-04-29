#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <Windows.h>

#define DA_XSTR(s) DA_STR(s)
#define DA_STR(s) #s

#ifndef __max
#define __max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef __min
#define __min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define debug_point { static int _debug_point; (void)(_debug_point++); } 
#define unused(expression) debug_point; (void)(expression)
#define noop() ((void)0)
#define UNKNOWN unsigned int

#define ROUND_UP_TO_MULTIPLE(value, multiple) (((value) + (multiple) - 1) / (multiple) * (multiple))

#ifdef __INTELLISENSE__
// Shut the fuck up IntelliSense
#define __thiscall __fastcall
#endif

#ifdef __clang__
#define CLANG_DIAGNOSTIC_PUSH() __pragma(clang diagnostic push)
#define CLANG_DIAGNOSTIC_IGNORED(warning) __pragma(clang diagnostic ignored warning)
#define CLANG_DIAGNOSTIC_POP() __pragma(clang diagnostic pop)
#else
#define CLANG_DIAGNOSTIC_PUSH()
#define CLANG_DIAGNOSTIC_IGNORED(warning)
#define CLANG_DIAGNOSTIC_POP()
#endif

template<typename Output, typename Input>
Output coerce_cast(Input const& value)
{
	return *reinterpret_cast<Output const*>(&value);
}

CORE_DEC const char* HRESULT_TO_STRING(HRESULT hr);

#include <Core/fatal.h>
#include <Core/typedefs.h>
#include <Core/genresult.h>
#include <Core/libver.h>
#include <Core/TempStr.h>
#include <Core/stddat.h>
#include <Core/MangledAssert.h>
