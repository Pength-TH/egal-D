#ifndef _thread_h_
#define _thread_h_

#include "common/type.h"

#ifdef PLATFORM_LINUX
	#include <pthread.h>
#endif

namespace egal 
{
	namespace MT 
	{
#ifdef PLATFORM_WINDOWS
		typedef e_uint32 ThreadID;
#else
		typedef pthread_t ThreadID;
#endif

		e_void setThreadName(ThreadID thread_id, const e_char* thread_name);
		e_void sleep(e_uint32 milliseconds);
		e_void yield();
		e_uint32 getCPUsCount();
		ThreadID getCurrentThreadID();
		e_uint64 getThreadAffinityMask();

	}
}
#endif
