#ifndef _egal_perf_timer_h_
#define _egal_perf_timer_h_
#pragma once

#include <assert.h>
#include <stdio.h>
#include <string>

#include "common/type.h"
#include "common/config.h"

#if PLATFORM_WINDOWS

#include <windows.h>

#endif

namespace egal
{
	class Timer
	{
	public:
		virtual ~Timer() {}

		virtual e_float Tick()
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			e_float delta = static_cast<e_float>((double)(tick.QuadPart - m_last_tick.QuadPart) / (double)m_frequency.QuadPart);
			m_last_tick = tick;
			return delta;
		}
		virtual e_float Start()
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			e_float delta =
				static_cast<e_float>((double)(tick.QuadPart - m_first_tick.QuadPart) / (double)m_frequency.QuadPart);
			return delta;
		}
		e_float Stop()
		{
			LARGE_INTEGER stopCycles;
			QueryPerformanceCounter(&stopCycles);
			e_float delta =
				static_cast<e_float>(((double)(stopCycles.QuadPart - m_first_tick.QuadPart) / m_frequency.QuadPart));
			return delta;
		}

		virtual e_float SinceTick()
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			e_float delta = static_cast<e_float>((double)(tick.QuadPart - m_last_tick.QuadPart) / (double)m_frequency.QuadPart);
			return delta;
		}
		virtual e_uint64 SinceStart()
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			return tick.QuadPart - m_first_tick.QuadPart;
		}
		virtual e_uint64 getFrequency()
		{
			return m_frequency.QuadPart;
		}
	private:
		LARGE_INTEGER m_frequency;
		LARGE_INTEGER m_last_tick;
		LARGE_INTEGER m_first_tick;
	};

	// -------------------------------------------------------------------------------------------------------------
	class ScopedTimer
	{
	public:
		ScopedTimer(const e_char* message)
		{
			m_message = message;
			m_timer.Start();
		}
		~ScopedTimer()
		{
			m_timer.Stop();
		}

	private:
		ScopedTimer() {};
		Timer m_timer;
		const e_char * m_message;
	};

}

#endif