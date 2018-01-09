#include "common/utils/perf_timer.h"

namespace egal
{
	struct TimerImpl : public Timer
	{
		explicit TimerImpl(IAllocator& allocator)
			: m_allocator(allocator)
		{
			QueryPerformanceFrequency(&m_frequency);
			QueryPerformanceCounter(&m_last_tick);
			m_first_tick = m_last_tick;
		}


		float getTimeSinceStart() override
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			float delta =
				static_cast<float>((double)(tick.QuadPart - m_first_tick.QuadPart) / (double)m_frequency.QuadPart);
			return delta;
		}


		e_uint64 getRawTimeSinceStart() override
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			return tick.QuadPart - m_first_tick.QuadPart;
		}


		e_uint64 getFrequency() override
		{
			return m_frequency.QuadPart;
		}


		float getTimeSinceTick() override
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			float delta = static_cast<float>((double)(tick.QuadPart - m_last_tick.QuadPart) / (double)m_frequency.QuadPart);
			return delta;
		}

		float tick() override
		{
			LARGE_INTEGER tick;
			QueryPerformanceCounter(&tick);
			float delta = static_cast<float>((double)(tick.QuadPart - m_last_tick.QuadPart) / (double)m_frequency.QuadPart);
			m_last_tick = tick;
			return delta;
		}

		IAllocator& m_allocator;
		LARGE_INTEGER m_frequency;
		LARGE_INTEGER m_last_tick;
		LARGE_INTEGER m_first_tick;
	};


	/**/
	Timer* Timer::create(IAllocator& allocator)
	{
		return _aligned_new(allocator, TimerImpl)(allocator);
	}


	void Timer::destroy(Timer* timer)
	{
		if (!timer) return;

		_delete(static_cast<TimerImpl*>(timer)->m_allocator, timer);
	}

}

