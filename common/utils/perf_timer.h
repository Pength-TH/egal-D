#ifndef _egal_perf_timer_h_
#define _egal_perf_timer_h_
#pragma once

#include <assert.h>
#include <stdio.h>
#include <string>

#include "common/type.h"
#include "common/config.h"
#include "common/allocator/egal_allocator.h"

#if PLATFORM_WINDOWS
 #include <windows.h>
#endif

namespace egal
{
	class Timer
	{
	public:
		virtual ~Timer() {}

		virtual float tick() = 0;
		virtual float getTimeSinceStart() = 0;
		virtual float getTimeSinceTick() = 0;
		virtual e_uint64 getRawTimeSinceStart() = 0;
		virtual e_uint64 getFrequency() = 0;

		static Timer* create(IAllocator& allocator);
		static void destroy(Timer* timer);
	};

	class ScopedTimer
	{
	public:
		ScopedTimer(const char* name, IAllocator& allocator)
			: m_name(name)
			, m_timer(Timer::create(allocator))
		{

		}

		~ScopedTimer()
		{
			Timer::destroy(m_timer);
		}

		float getTimeSinceStart()
		{
			return m_timer->getTimeSinceStart();
		}

		const char* getName() const { return m_name; }

	private:
		const char* m_name;
		Timer* m_timer;
	};

}

#endif