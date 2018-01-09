#ifndef _base_proxy_allocator_h_
#define _base_proxy_allocator_h_

#include "common/define.h"
#include "common/type.h"

#include "common/allocator/egal_allocator.h"

#include "common/thread/atomic.h"
#include "common/thread/lock_free_fixed_queue.h"
#include "common/thread/sync.h"
#include "common/thread/task.h"
#include "common/thread/thread.h"
#include "common/thread/transaction.h"

namespace egal
{
	class BaseProxyAllocator : public IAllocator
	{
	public:
		explicit BaseProxyAllocator(IAllocator& source)
			: m_source(source)
		{
			m_allocation_count = 0;
		}

		virtual ~BaseProxyAllocator() { ASSERT(m_allocation_count == 0); }


		void* allocate_aligned(size_t size, size_t align) override
		{
			MT::atomicIncrement(&m_allocation_count);
			return m_source.allocate_aligned(size, align);
		}


		void deallocate_aligned(void* ptr) override
		{
			if (ptr)
			{
				MT::atomicDecrement(&m_allocation_count);
				m_source.deallocate_aligned(ptr);
			}
		}


		void* reallocate_aligned(void* ptr, size_t size, size_t align) override
		{
			if (!ptr) MT::atomicIncrement(&m_allocation_count);
			if (size == 0) MT::atomicDecrement(&m_allocation_count);
			return m_source.reallocate_aligned(ptr, size, align);
		}


		void* allocate(size_t size) override
		{
			MT::atomicIncrement(&m_allocation_count);
			return m_source.allocate(size);
		}

		void deallocate(void* ptr) override
		{
			if (ptr)
			{
				MT::atomicDecrement(&m_allocation_count);
				m_source.deallocate(ptr);
			}
		}

		void* reallocate(void* ptr, size_t size) override
		{
			if (!ptr) MT::atomicIncrement(&m_allocation_count);
			if (size == 0) MT::atomicDecrement(&m_allocation_count);
			return m_source.reallocate(ptr, size);
		}


		IAllocator& getSourceAllocator() { return m_source; }

	private:
		IAllocator& m_source;
		volatile e_int32 m_allocation_count;
	};
}

#endif
