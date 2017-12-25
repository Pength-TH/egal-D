#include "common/type.h"
#include "common/thread/task.h"
#include "common/thread/thread.h"

namespace egal 
{
	namespace MT
	{
		const e_uint32 STACK_SIZE = 0x8000;

		struct TaskImpl
		{
			explicit TaskImpl(IAllocator& allocator)
				: m_allocator(allocator)
			{
			}

			IAllocator& m_allocator;
			HANDLE m_handle;
			DWORD m_thread_id;
			e_uint64 m_affinity_mask;
			e_uint32 m_priority;
			volatile e_bool m_is_running;
			volatile e_bool m_force_exit;
			volatile e_bool m_exited;
			const e_char* m_thread_name;
			Task* m_owner;
		};

		static DWORD WINAPI threadFunction(LPVOID ptr)
		{
			e_uint32 ret = 0xffffFFFF;
			struct TaskImpl* impl = reinterpret_cast<TaskImpl*>(ptr);
			setThreadName(impl->m_thread_id, impl->m_thread_name);
			//Profiler::setThreadName(impl->m_thread_name); todo
			if (!impl->m_force_exit)
			{
				ret = impl->m_owner->task();
			}
			impl->m_exited = true;
			impl->m_is_running = false;

			return ret;
		}

		Task::Task(IAllocator& allocator)
		{
			TaskImpl* impl = _aligned_new(allocator, TaskImpl)(allocator);
			impl->m_handle = nullptr;
			impl->m_affinity_mask = getThreadAffinityMask();
			impl->m_priority = ::GetThreadPriority(GetCurrentThread());
			impl->m_is_running = false;
			impl->m_force_exit = false;
			impl->m_exited = false;
			impl->m_thread_name = "";
			impl->m_owner = this;

			m_implementation = impl;
		}

		Task::~Task()
		{
			ASSERT(!m_implementation->m_handle);
			_delete(m_implementation->m_allocator, m_implementation);
		}

		e_bool Task::create(const e_char* name)
		{
			HANDLE handle = CreateThread(
				nullptr, STACK_SIZE, threadFunction, m_implementation, CREATE_SUSPENDED, &m_implementation->m_thread_id);
			if (handle)
			{
				m_implementation->m_exited = false;
				m_implementation->m_thread_name = name;
				m_implementation->m_handle = handle;
				m_implementation->m_is_running = true;

				e_bool success = ::ResumeThread(m_implementation->m_handle) != -1;
				if (success)
				{
					return true;
				}
				::CloseHandle(m_implementation->m_handle);
				m_implementation->m_handle = nullptr;
				return false;
			}
			return false;
		}

		e_bool Task::destroy()
		{
			while (m_implementation->m_is_running)
			{
				yield();
			}

			::CloseHandle(m_implementation->m_handle);
			m_implementation->m_handle = nullptr;
			return true;
		}

		e_void Task::setAffinityMask(e_uint64 affinity_mask)
		{
			m_implementation->m_affinity_mask = affinity_mask;
			if (m_implementation->m_handle)
			{
				::SetThreadAffinityMask(m_implementation->m_handle, affinity_mask);
			}
		}

		e_uint64 Task::getAffinityMask() const
		{
			return m_implementation->m_affinity_mask;
		}

		e_bool Task::isRunning() const
		{
			return m_implementation->m_is_running;
		}

		e_bool Task::isFinished() const
		{
			return m_implementation->m_exited;
		}

		e_bool Task::isForceExit() const
		{
			return m_implementation->m_force_exit;
		}

		IAllocator& Task::getAllocator()
		{
			return m_implementation->m_allocator;
		}

		e_void Task::forceExit(e_bool wait)
		{
			m_implementation->m_force_exit = true;

			while (!isFinished() && wait)
			{
				yield();
			}
		}

	}
}
