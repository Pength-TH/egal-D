#ifndef _fibers_h_
#define _fibers_h_
#pragma once

#include "common/type.h"

#if EGAL_PLATFORM_LINUX
	#include <ucontext.h>
#endif

namespace egal
{
	class EngineRoot;
	namespace Fiber
	{
#ifdef _WIN32
		typedef void* Handle;
		typedef void(__stdcall *FiberProc)(void*);
#else 
		typedef ucontext_t Handle;
		typedef void(*FiberProc)(void*);
#endif
		constexpr void* INVALID_FIBER = nullptr;

		void initThread(FiberProc proc, Handle* handle);
		Handle create(int stack_size, FiberProc proc, void* parameter);
		void destroy(Handle fiber);
		void switchTo(Handle* from, Handle fiber);
	}
}
#endif
