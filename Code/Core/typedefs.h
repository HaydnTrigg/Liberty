#pragma once

#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__

#include <stdlib.h>

typedef signed long BOOL32;
typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long UL32;
typedef char C8;
typedef signed char S8;
typedef signed short S16;
typedef signed int S32;
typedef signed long SL32;
typedef unsigned __int64 U64;
typedef signed __int64 S64;
typedef float SINGLE;
typedef double DOUBLE;

typedef union
{
	S32 i;
	U32 u;
	SINGLE f;
	void* p;
}
HYBRID;

#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef CLAMP
#define CLAMP(VALUE, MIN, MAX) __max(__min(VALUE, MIN), MAX)
#endif

#endif // __TYPEDEFS_H__
