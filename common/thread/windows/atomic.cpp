#include "common/thread/atomic.h"

#include <intrin.h>

namespace egal
{
	namespace MT
	{
		e_int32 atomicIncrement(e_int32 volatile* value)
		{
			return _InterlockedIncrement((volatile long*)value);
		}

		e_int32 atomicDecrement(e_int32 volatile* value)
		{
			return _InterlockedDecrement((volatile long*)value);
		}

		e_int32 atomicAdd(e_int32 volatile* addend, e_int32 value)
		{
			return _InterlockedExchangeAdd((volatile long*)addend, value);
		}

		e_int32 atomicSubtract(e_int32 volatile* addend, e_int32 value)
		{
			return _InterlockedExchangeAdd((volatile long*)addend, -value);
		}

		e_bool compareAndExchange(e_int32 volatile* dest, e_int32 exchange, e_int32 comperand)
		{
			return _InterlockedCompareExchange((volatile long*)dest, exchange, comperand) == comperand;
		}

		e_bool compareAndExchange64(e_int64 volatile* dest, e_int64 exchange, e_int64 comperand)
		{
			return _InterlockedCompareExchange64(dest, exchange, comperand) == comperand;
		}


		e_void memoryBarrier()
		{
#ifdef _M_AMD64
			__faststorefence();
#elif defined _IA64_
			__mf();
#else
			e_int32 Barrier;
			__asm {
				xchg Barrier, eax
			}
#endif
		}

	}
}
