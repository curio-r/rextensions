// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"
#include "win32.h"

#include "Detours Express 3.0\include\detours.h"
#pragma comment(lib, "Detours Express 3.0\\lib.X86\\detours.lib")

template<typename T>
union _void_cast_u
{
	T Fn;
	char *Ptr;

	inline _void_cast_u(T fn) : Fn(fn) {}
};

// Required for casting class method pointers
#define void_cast(T) ((_void_cast_u<decltype(T)>{T}).Ptr)
