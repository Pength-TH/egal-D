#include "common/thread/sync.h"
#include "common/thread/atomic.h"
#include "common/thread/thread.h"

namespace egal
{
	namespace MT
	{
		Semaphore::Semaphore(e_int32 init_count, e_int32 max_count)
		{
			m_id = ::CreateSemaphore(nullptr, init_count, max_count, nullptr);
		}

		Semaphore::~Semaphore()
		{
			::CloseHandle(m_id);
		}

		e_void Semaphore::signal()
		{
			::ReleaseSemaphore(m_id, 1, nullptr);
		}

		e_void Semaphore::wait()
		{
			::WaitForSingleObject(m_id, INFINITE);
		}

		e_bool Semaphore::poll()
		{
			return WAIT_OBJECT_0 == ::WaitForSingleObject(m_id, 0);
		}


		Event::Event(e_bool manual_reset)
		{
			m_id = ::CreateEvent(nullptr, manual_reset, FALSE, nullptr);
		}

		Event::~Event()
		{
			::CloseHandle(m_id);
		}

		e_void Event::reset()
		{
			::ResetEvent(m_id);
		}

		e_void Event::trigger()
		{
			::SetEvent(m_id);
		}

		e_void Event::waitTimeout(e_uint32 timeout_ms)
		{
			::WaitForSingleObject(m_id, timeout_ms);
		}

		e_void Event::wait()
		{
			::WaitForSingleObject(m_id, INFINITE);
		}

		e_bool Event::poll()
		{
			return WAIT_OBJECT_0 == ::WaitForSingleObject(m_id, 0);
		}


		SpinMutex::SpinMutex(e_bool locked)
			: m_id(0)
		{
			if (locked)
			{
				lock();
			}
		}

		SpinMutex::~SpinMutex() = default;


		e_void SpinMutex::lock()
		{
			for (;;)
			{
				if (compareAndExchange(&m_id, 1, 0))
				{
					memoryBarrier();
					return;
				}

				while (m_id)
				{
					yield();
				}
			}
		}

		e_bool SpinMutex::poll()
		{
			if (compareAndExchange(&m_id, 1, 0))
			{
				memoryBarrier();
				return true;
			}
			return false;
		}

		e_void SpinMutex::unlock()
		{
			memoryBarrier();
			m_id = 0;
		}

	}
}
