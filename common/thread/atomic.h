#ifndef _atomic_h_
#define _atomic_h_

#include "common/type.h"

namespace egal
{
	namespace MT
	{
		e_int32 atomicIncrement(e_int32 volatile* value);
		e_int32 atomicDecrement(e_int32 volatile* value);
		e_int32 atomicAdd(e_int32 volatile* addend, e_int32 value);
		e_int32 atomicSubtract(e_int32 volatile* addend, e_int32 value);
		e_bool compareAndExchange(e_int32 volatile* dest, e_int32 exchange, e_int32 comperand);
		e_bool compareAndExchange64(e_int64 volatile* dest, e_int64 exchange, e_int64 comperand);
		e_void memoryBarrier();
	}
}
#endif
