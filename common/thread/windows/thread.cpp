#include "common/type.h"
#include "common/thread/thread.h"

namespace egal
{
	namespace MT
	{
		static_assert(sizeof(ThreadID) == sizeof(::GetCurrentThreadId()), "Not matching");

		e_void sleep(e_uint32 milliseconds) { ::Sleep(milliseconds); }
		e_void yield() { sleep(0); }

		e_uint32 getCPUsCount()
		{
			SYSTEM_INFO sys_info;
			GetSystemInfo(&sys_info);

			e_uint32 num = sys_info.dwNumberOfProcessors;
			num = num > 0 ? num : 1;

			return num;
		}

		ThreadID getCurrentThreadID() { return ::GetCurrentThreadId(); }

		e_uint64 getThreadAffinityMask()
		{
			DWORD_PTR affinity_mask = ::SetThreadAffinityMask(::GetCurrentThread(), ~(DWORD_PTR)0);
			::SetThreadAffinityMask(::GetCurrentThread(), affinity_mask);
			return affinity_mask;
		}

		static const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
		typedef struct tagTHREADNAME_INFO
		{
			DWORD type;
			LPCSTR name;
			DWORD thread_id;
			DWORD flags;
		} THREADNAME_INFO;
#pragma pack(pop)

		e_void setThreadName(ThreadID thread_id, const e_char* thread_name)
		{
			THREADNAME_INFO info;
			info.type = 0x1000;
			info.name = thread_name;
			info.thread_id = thread_id;
			info.flags = 0;

			__try
			{
				RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
			}
		}
	}
}

