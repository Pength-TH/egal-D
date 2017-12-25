#ifndef _lock_free_fixed_queue_h_
#define _lock_free_fixed_queue_h_

#include "common/type.h"
#include "common/thread/sync.h"
namespace egal
{
	namespace MT
	{
		template <class T, e_int32 size>
		class LockFreeFixedQueue
		{
		public:
			LockFreeFixedQueue()
				: m_al(0)
				, m_fr(0)
				, m_rd(0)
				, m_wr(0)
				, m_aborted(false)
				, m_data_signal(0, size)
			{
				for (e_int32 i = 0; i < size; i++)
				{
					m_alloc[i].data.pair.key = i;
					m_alloc[i].data.pair.el = i;
					m_queue[i].data.pair.key = i;
					m_queue[i].data.pair.el = -1;
				}
			}

			~LockFreeFixedQueue()
			{
			}

			T* alloc(e_bool wait)
			{
				do
				{
					if ((m_al - m_fr) < size)
					{
						e_int32 alloc_ptr = m_al;
						e_int32 alloc_idx = alloc_ptr & (size - 1);

						Node cur_val(alloc_ptr, m_alloc[alloc_idx].data.pair.el);

						if (cur_val.data.pair.el > -1)
						{
							Node new_val(alloc_ptr, -1);

							if (compareAndExchange64(&m_alloc[alloc_idx].data.val, new_val.data.val, cur_val.data.val))
							{
								atomicIncrement(&m_al);
								auto* val = (T*)&m_pool[cur_val.data.pair.el * sizeof(T)];
								_new(val) T();
								return val;
							}
						}
					}
				} while (wait);

				return nullptr;
			}

			e_void dealoc(T* tr)
			{
				tr->~T();
				e_int32 idx = e_int32(tr - (T*)m_pool);
				ASSERT(idx >= 0 && idx < size);

				Node cur_val(0, -1);
				Node new_val(0, idx);

				for (;;)
				{
					e_int32 free_ptr = m_fr;
					e_int32 free_idx = free_ptr & (size - 1);

					cur_val.data.pair.key = free_ptr;
					new_val.data.pair.key = free_ptr + size;
					if (compareAndExchange64(&m_alloc[free_idx].data.val, new_val.data.val, cur_val.data.val))
					{
						atomicIncrement(&m_fr);
						break;
					}
				}
			}

			e_bool push(const T* tr, e_bool wait)
			{
				e_int32 idx = e_int32(tr - (T*)m_pool);
				ASSERT(idx >= 0 && idx < size);

				do
				{
					ASSERT((m_wr - m_rd) < size);

					Node cur_node(0, -1);
					Node new_node(0, idx);

					e_int32 cur_write_idx = m_wr;
					e_int32 idx = cur_write_idx & (size - 1);

					cur_node.data.pair.key = cur_write_idx;
					new_node.data.pair.key = cur_write_idx;

					if (compareAndExchange64(&m_queue[idx].data.val, new_node.data.val, cur_node.data.val))
					{
						atomicIncrement(&m_wr);
						m_data_signal.signal();
						return true;
					}
				} while (wait);

				return false;
			}

			T* pop(e_bool wait)
			{
				e_bool can_read = wait ? m_data_signal.wait(), wait : m_data_signal.poll();

				if (isAborted())
				{
					return nullptr;
				}

				while (can_read)
				{
					if (m_rd != m_wr)
					{
						e_int32 cur_read_idx = m_rd;
						e_int32 idx = cur_read_idx & (size - 1);

						Node cur_node(cur_read_idx, m_queue[idx].data.pair.el);

						if (cur_node.data.pair.el > -1)
						{
							Node new_node(cur_read_idx + size, -1);

							if (compareAndExchange64(&m_queue[idx].data.val, new_node.data.val, cur_node.data.val))
							{
								atomicIncrement(&m_rd);
								return (T*)&m_pool[cur_node.data.pair.el * sizeof(T)];
							}
						}
					}
				}

				return nullptr;
			}


			e_bool isAborted() const
			{
				return m_aborted;
			}

			e_bool isEmpty() const
			{
				return m_rd == m_wr;
			}

			e_void abort()
			{
				m_aborted = true;
				m_data_signal.signal();
			}

		private:

			struct Node
			{
				union
				{
					struct
					{
						e_int32 key;
						e_int32	el;
					} pair;
					e_int64		val;
				} data;

				Node()
				{
				}

				Node(e_int32 k, e_int32 i)
				{
					data.pair.key = k;
					data.pair.el = i;
				}
			};

			volatile e_int32	m_al;
			volatile e_int32	m_fr;
			volatile e_int32	m_rd;
			volatile e_int32	m_wr;
			Node				m_alloc[size];
			Node				m_queue[size];
			e_uint8				m_pool[sizeof(T) * size];
			volatile e_bool		m_aborted;
			MT::Semaphore		m_data_signal;
		};
	}
}
#endif
