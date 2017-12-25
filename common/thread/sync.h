#ifndef _sync_h_
#define _sync_h_

#include "common/type.h"

#ifdef PLATFORM_LINUX
	#include <pthread.h>
#endif

namespace egal
{
	namespace MT
	{
#if defined PLATFORM_WINDOWS
		typedef e_void* SemaphoreHandle;
		typedef e_void* MutexHandle;
		typedef e_void* EventHandle;
		typedef volatile e_int32 SpinMutexHandle;
#elif defined PLATFORM_LINUX
		struct SemaphoreHandle
		{
			pthread_mutex_t mutex;
			pthread_cond_t cond;
			e_int32 count;
		};
		typedef pthread_mutex_t MutexHandle;
		struct EventHandle
		{
			pthread_mutex_t mutex;
			pthread_cond_t cond;
			e_bool signaled;
			e_bool manual_reset;
		};
		typedef volatile e_int32 SpinMutexHandle;
#endif

		class Semaphore
		{
		public:
			Semaphore(e_int32 init_count, e_int32 max_count);
			~Semaphore();

			e_void signal();

			e_void wait();
			e_bool poll();

		private:
			SemaphoreHandle m_id;
		};

		class Event
		{
		public:
			explicit Event(e_bool manual_reset);
			~Event();

			e_void reset();

			e_void trigger();

			e_void wait();
			e_void waitTimeout(e_uint32 timeout_ms);
			e_bool poll();

		private:
			EventHandle m_id;
		};

		class SpinMutex
		{
		public:
			explicit SpinMutex(e_bool locked);
			~SpinMutex();

			e_void lock();
			e_bool poll();

			e_void unlock();

		private:
			SpinMutexHandle m_id;
		};

		class SpinLock
		{
		public:
			explicit SpinLock(SpinMutex& mutex)
				: m_mutex(mutex)
			{
				mutex.lock();
			}
			~SpinLock() { m_mutex.unlock(); }

		private:
			SpinLock(const SpinLock&);
			e_void operator=(const SpinLock&);
			SpinMutex& m_mutex;
		};
	}
}

#endif

