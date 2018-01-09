#ifndef _queue_h_
#define _queue_h_
#pragma once

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
namespace egal
{
	template <typename T, e_uint32 count>
	class TQueue
	{
	public:
		struct Iterator
		{
			TQueue* owner;
			e_uint32 cursor;

			bool operator!=(const Iterator& rhs) const { return cursor != rhs.cursor || owner != rhs.owner; }
			void operator ++() { ++cursor; }
			T& value() { e_uint32 idx = cursor & (count - 1); return owner->m_buffer[idx]; }
		};

		explicit TQueue(IAllocator& allocator)
			: m_allocator(allocator)
		{
			ASSERT(Math::isPowOfTwo(count));
			m_buffer = (T*)(m_allocator.allocate(sizeof(T) * count));
			m_wr = m_rd = 0;
		}

		~TQueue()
		{
			m_allocator.deallocate(m_buffer);
		}

		bool full() const { return size() == count; }
		bool empty() const { return m_rd == m_wr; } 
		e_uint32 size() const { return m_wr - m_rd; }
		Iterator begin() { return {this, m_rd}; }
		Iterator end() { return {this, m_wr}; }

		void push_back(const T& item)
		{
			ASSERT(m_wr - m_rd < count);

			e_uint32 idx = m_wr & (count - 1);
			_new( &m_buffer[idx]) T(item);
			++m_wr;
		}

		void pop()
		{
			ASSERT(m_wr != m_rd);

			e_uint32 idx = m_rd & (count - 1);
			(&m_buffer[idx])->~T();
			m_rd++;
		}

		T& front()
		{
			e_uint32 idx = m_rd & (count - 1);
			return m_buffer[idx];
		}

		const T& front() const
		{
			e_uint32 idx = m_rd & (count - 1);
			return m_buffer[idx];
		}

		T& back()
		{
			ASSERT(!empty());

			e_uint32 idx = m_wr & (count - 1);
			return m_buffer[idx - 1];
		}

		const T& back() const
		{
			ASSERT(!empty());

			e_uint32 idx = m_wr & (count - 1);
			return m_buffer[idx - 1];
		}

	private:
		IAllocator& m_allocator;
		e_uint32 m_rd;
		e_uint32 m_wr;
		T* m_buffer;
	};
}
#endif
