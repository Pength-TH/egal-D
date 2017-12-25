#ifndef _transaction_h_
#define _transaction_h_

#include "common/thread/sync.h"

namespace egal
{
	namespace MT
	{
		template <class T> struct Transaction
		{
			e_void setCompleted() { m_event.trigger(); }
			e_bool isCompleted() { return m_event.poll(); }
			e_void waitForCompletion() { return m_event.wait(); }
			e_void reset() { m_event.reset(); }

			Transaction() : m_event(false) { }

			MT::Event	m_event;
			T			data;
		};
	}
}

#endif
