#pragma once

#ifndef ASSERT
#include <assert.h>

#undef assert // don't allow usage of assert
#define assert assert_banned

#ifdef _DEBUG
#define FATAL(...) do { __debugbreak(); throw 0; } while(false)
#define ASSERT(expression) do { if(!(expression)) { __debugbreak(); _wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)); } } while(false)
#else
#define FATAL(...) do { __debugbreak(); throw 0; } while(false)
#define ASSERT(expression) do { if(!(expression)) { __debugbreak(); throw 0; } } while(false)
#endif
#endif
