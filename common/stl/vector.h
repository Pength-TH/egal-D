#ifndef _vector_h_
#define _vector_h_

#include "common/type.h"
#include "common/allocator/allocator.h"
namespace egal
{
	template <typename T> 
	class TVector
	{
	public:
		explicit TVector(IAllocator& allocator)
			: m_allocator(allocator)
		{
			m_data = nullptr;
			m_capacity = 0;
			m_size = 0;
		}

		explicit TVector(const TVector& rhs)
		{
			m_data = nullptr;
			m_capacity = 0;
			m_size = 0;
			m_allocator = &g_allocator;
			*this = rhs;
		}

		T* begin() const { return m_data; }


		T* end() const { return m_data + m_size; }


		void swap(TVector<T>& rhs)
		{
			ASSERT(&rhs.m_allocator == &m_allocator);

			e_int32 i = rhs.m_capacity;
			rhs.m_capacity = m_capacity;
			m_capacity = i;

			i = m_size;
			m_size = rhs.m_size;
			rhs.m_size = i;

			T* p = rhs.m_data;
			rhs.m_data = m_data;
			m_data = p;
		}


		template <typename Comparator>
		void removeDuplicates(Comparator equals)
		{
			for (e_int32 i = 0; i < m_size - 1; ++i)
			{
				for (e_int32 j = i + 1; j < m_size; ++j)
				{
					if (equals(m_data[i], m_data[j]))
					{
						eraseFast(j);
						--j;
					}
				}
			}
		}

		void removeDuplicates()
		{
			for (e_int32 i = 0; i < m_size - 1; ++i)
			{
				for (e_int32 j = i + 1; j < m_size; ++j)
				{
					if (m_data[i] == m_data[j])
					{
						eraseFast(j);
						--j;
					}
				}
			}
		}

		void operator=(const TVector& rhs)
		{
			if (this != &rhs)
			{
				callDestructors(m_data, m_data + m_size);
				m_allocator.deallocate_aligned(m_data);
				m_data = (T*)m_allocator.allocate_aligned(rhs.m_capacity * sizeof(T), ALIGN_OF(T));
				m_capacity = rhs.m_capacity;
				m_size = rhs.m_size;
				for (e_int32 i = 0; i < m_size; ++i)
				{
					_new ((char*)(m_data + i)) T(rhs.m_data[i]);
				}
			}
		}

		~TVector()
		{
			callDestructors(m_data, m_data + m_size);
			m_allocator.deallocate_aligned(m_data);
		}

		template <typename F>
		e_int32 find(F predicate) const
		{
			for (e_int32 i = 0; i < m_size; ++i)
			{
				if (predicate(m_data[i]))
				{
					return i;
				}
			}
			return -1;
		}

		template <typename R>
		e_int32 indexOf(R item) const
		{
			for (e_int32 i = 0; i < m_size; ++i)
			{
				if (m_data[i] == item)
				{
					return i;
				}
			}
			return -1;
		}

		e_int32 indexOf(const T& item) const
		{
			for (e_int32 i = 0; i < m_size; ++i)
			{
				if (m_data[i] == item)
				{
					return i;
				}
			}
			return -1;
		}

		template <typename F>
		void eraseItems(F predicate)
		{
			for (e_int32 i = m_size - 1; i >= 0; --i)
			{
				if (predicate(m_data[i]))
				{
					erase(i);
				}
			}
		}

		void eraseItemFast(const T& item)
		{
			for (e_int32 i = 0; i < m_size; ++i)
			{
				if (m_data[i] == item)
				{
					eraseFast(i);
					return;
				}
			}
		}

		void eraseFast(e_int32 index)
		{
			if (index >= 0 && index < m_size)
			{
				m_data[index].~T();
				if (index != m_size - 1)
				{
					memmove(m_data + index, m_data + m_size - 1, sizeof(T));
				}
				--m_size;
			}
		}

		void eraseItem(const T& item)
		{
			for (e_int32 i = 0; i < m_size; ++i)
			{
				if (m_data[i] == item)
				{
					erase(i);
					return;
				}
			}
		}


		void insert(e_int32 index, const T& value)
		{
			if (m_size == m_capacity)
			{
				grow();
			}
			memmove(m_data + index + 1, m_data + index, sizeof(T) * (m_size - index));
			_new (&m_data[index]) T(value);
			++m_size;
		}


		void erase(e_int32 index)
		{
			if (index >= 0 && index < m_size)
			{
				m_data[index].~T();
				if (index < m_size - 1)
				{
					memmove(m_data + index, m_data + index + 1, sizeof(T) * (m_size - index - 1));
				}
				--m_size;
			}
		}

		void push(const T& value)
		{
			e_int32 size = m_size;
			if (size == m_capacity)
			{
				grow();
			}
			_new ((char*)(m_data + size)) T(value);
			++size;
			m_size = size;
		}


		template <class _Ty> struct remove_reference { typedef _Ty type; };
		template <class _Ty> struct remove_reference<_Ty&> { typedef _Ty type; };
		template <class _Ty> struct remove_reference<_Ty&&> { typedef _Ty type; };

		template <class _Ty> _Ty&& myforward(typename remove_reference<_Ty>::type& _Arg)
		{
			return (static_cast<_Ty&&>(_Arg));
		}

		template <typename... Params> T& emplace(Params&&... params)
		{
			if (m_size == m_capacity)
			{
				grow();
			}
			_new ((char*)(m_data + m_size)) T(myforward<Params>(params)...);
			++m_size;
			return m_data[m_size - 1];
		}

		template <typename... Params> T& emplaceAt(e_int32 idx, Params&&... params)
		{
			if (m_size == m_capacity)
			{
				grow();
			}
			for (e_int32 i = m_size - 1; i >= idx; --i)
			{
				memcpy(&m_data[i + 1], &m_data[i], sizeof(m_data[i]));
			}
			_new ((char*)(m_data + idx)) T(myforward<Params>(params)...);
			++m_size;
			return m_data[idx];
		}

		bool empty() const { return m_size == 0; }

		void clear()
		{
			callDestructors(m_data, m_data + m_size);
			m_size = 0;
		}

		const T& back() const { return m_data[m_size - 1]; }


		T& back() { return m_data[m_size - 1]; }


		void pop()
		{
			if (m_size > 0)
			{
				m_data[m_size - 1].~T();
				--m_size;
			}
		}

		void resize(e_int32 size)
		{
			if (size > m_capacity)
			{
				reserve(size);
			}
			for (e_int32 i = m_size; i < size; ++i)
			{
				_new ((char*)(m_data + i)) T();
			}
			callDestructors(m_data + size, m_data + m_size);
			m_size = size;
		}


		void reserve(e_int32 capacity)
		{
			if (capacity > m_capacity)
			{
				T* newData = (T*)m_allocator.allocate_aligned(capacity * sizeof(T), ALIGN_OF(T));
				memcpy(newData, m_data, sizeof(T) * m_size);
				m_allocator.deallocate_aligned(m_data);
				m_data = newData;
				m_capacity = capacity;
			}
		}

		const T& operator[](e_int32 index) const
		{
			ASSERT(index >= 0 && index < m_size);
			return m_data[index];
		}

		T& operator[](e_int32 index)
		{
			ASSERT(index >= 0 && index < m_size);
			return m_data[index];
		}

		e_int32 size() const { return m_size; }
		e_int32 capacity() const { return m_capacity; }

	private:
		void grow()
		{
			e_int32 new_capacity = m_capacity == 0 ? 4 : m_capacity * 2;
			m_data = (T*)m_allocator.reallocate_aligned(m_data, new_capacity * sizeof(T), ALIGN_OF(T));
			m_capacity = new_capacity;
		}

		void callDestructors(T* begin, T* end)
		{
			for (; begin < end; ++begin)
			{
				begin->~T();
			}
		}

	private:
		IAllocator& m_allocator;
		e_int32 m_capacity;
		e_int32 m_size;
		T* m_data;
	};
}
#endif
