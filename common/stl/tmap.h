#ifndef _map_h_
#define _map_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
namespace egal
{
	template <typename Key, typename Value>
	class TMap
	{
	public:
		explicit TMap(IAllocator& allocator)
			: m_allocator(allocator)
			, m_size(0)
			, m_capacity(0)
			, m_keys(nullptr)
			, m_values(nullptr)
		{}

		~TMap()
		{
			callDestructors(m_keys, m_size);
			callDestructors(m_values, m_size);
			m_allocator.deallocate(m_keys);
		}

		Value& insert(const Key& key)
		{
			if (m_capacity == m_size) reserve(m_capacity < 4 ? 4 : m_capacity * 2);

			e_int32 i = index(key);
			ASSERT(i >= 0 && ((i < m_size && m_keys[i] != key) || i == m_size));
			StringUnitl::moveMemory(m_keys + i + 1, m_keys + i, sizeof(Key) * (m_size - i));
			StringUnitl::moveMemory(m_values + i + 1, m_values + i, sizeof(Value) * (m_size - i));
			_new(&m_values[i]) Value();
			_new(&m_keys[i]) Key(key);
			++m_size;
			return m_values[i];
		}

		template <class _Ty> struct remove_reference
		{
			typedef _Ty type;
		};

		template <class _Ty> struct remove_reference<_Ty&>
		{
			typedef _Ty type;
		};

		template <class _Ty> struct remove_reference<_Ty&&>
		{
			typedef _Ty type;
		};

		template <class _Ty> _Ty&& myforward(typename remove_reference<_Ty>::type& _Arg)
		{
			return (static_cast<_Ty&&>(_Arg));
		}

		template <typename... Params> Value& emplace(const Key& key, Params&&... params)
		{
			if (m_capacity == m_size) reserve(m_capacity < 4 ? 4 : m_capacity * 2);

			e_int32 i = index(key);
			ASSERT(i >= 0 && ((i < m_size && m_keys[i] != key) || i == m_size));

			StringUnitl::moveMemory(m_keys + i + 1, m_keys + i, sizeof(Key) * (m_size - i));
			StringUnitl::moveMemory(m_values + i + 1, m_values + i, sizeof(Value) * (m_size - i));
			_new (&m_values[i]) Value(myforward<Params>(params)...);
			_new (&m_keys[i]) Key(key);
			++m_size;

			return m_values[i];
		}

		e_int32 insert(const Key& key, const Value& value)
		{
			if (m_capacity == m_size) reserve(m_capacity < 4 ? 4 : m_capacity * 2);

			e_int32 i = index(key);
			if (i >= 0 && ((i < m_size && m_keys[i] != key) || i == m_size))
			{
				StringUnitl::moveMemory(m_keys + i + 1, m_keys + i, sizeof(Key) * (m_size - i));
				StringUnitl::moveMemory(m_values + i + 1, m_values + i, sizeof(Value) * (m_size - i));
				_new(&m_values[i]) Value(value);
				_new(&m_keys[i]) Key(key);
				++m_size;

				return i;
			}
			return -1;
		}

		bool find(const Key& key, Value& value) const
		{
			e_int32 i = find(key);
			if (i < 0)
			{
				return false;
			}
			value = m_values[i];
			return true;
		}

		e_int32 find(const Key& key) const
		{
			e_int32 l = 0;
			e_int32 h = m_size - 1;
			while (l < h)
			{
				e_int32 mid = (l + h) >> 1;
				if (m_keys[mid] < key)
				{
					l = mid + 1;
				}
				else
				{
					h = mid;
				}
			}
			if (l == h && m_keys[l] == key)
			{
				return l;
			}
			return -1;
		}

		const Value& operator [](const Key& key) const
		{
			e_int32 index = find(key);
			if (index >= 0)
			{
				return m_values[index];
			}
			else
			{
				ASSERT(false);
				return m_values[0];
			}
		}

		Value& operator [](const Key& key)
		{
			e_int32 index = find(key);
			if (index >= 0)
			{
				return m_values[index];
			}
			else
			{
				return m_values[insert(key, Value())];
			}
		}

		e_int32 size() const
		{
			return m_size;
		}

		Value& get(const Key& key)
		{
			e_int32 index = find(key);
			ASSERT(index >= 0);
			return m_values[index];
		}

		Value* begin() { return m_values; }
		Value* end() { return m_values + m_size; }
		const Value* begin() const { return m_values; }
		const Value* end() const { return m_values + m_size; }

		Value& at(e_int32 index)
		{
			return m_values[index];
		}

		const Value& at(e_int32 index) const
		{
			return m_values[index];
		}

		void clear()
		{
			callDestructors(m_keys, m_size);
			callDestructors(m_values, m_size);
			m_size = 0;
		}

		void reserve(e_int32 new_capacity)
		{
			if (m_capacity >= new_capacity) return;

			e_uint8* new_data = (e_uint8*)m_allocator.allocate(new_capacity * (sizeof(Key) + sizeof(Value)));

			StringUnitl::copyMemory(new_data, m_keys, sizeof(Key) * m_size);
			StringUnitl::copyMemory(new_data + sizeof(Key) * new_capacity, m_values, sizeof(Value) * m_size);

			m_allocator.deallocate(m_keys);
			m_keys = (Key*)new_data;
			m_values = (Value*)(new_data + sizeof(Key) * new_capacity);

			m_capacity = new_capacity;
		}

		const Key& getKey(e_int32 index) const
		{
			return m_keys[index];
		}

		void eraseAt(e_int32 index)
		{
			if (index >= 0 && index < m_size)
			{
				m_values[index].~Value();
				m_keys[index].~Key();
				if (index < m_size - 1)
				{
					StringUnitl::moveMemory(m_keys + index, m_keys + index + 1, sizeof(Key) * (m_size - index - 1));
					StringUnitl::moveMemory(m_values + index, m_values + index + 1, sizeof(Value) * (m_size - index - 1));
				}
				--m_size;
			}
		}

		void erase(const Key& key)
		{
			e_int32 i = find(key);
			if (i >= 0)
			{
				eraseAt(i);
			}
		}

	private:
		template <typename T> void callDestructors(T* ptr, e_int32 count)
		{
			for (e_int32 i = 0; i < count; ++i)
			{
				ptr[i].~T();
			}
		}

		e_int32 index(const Key& key) const
		{
			e_int32 l = 0;
			e_int32 h = m_size - 1;
			while (l < h)
			{
				e_int32 mid = (l + h) >> 1;
				if (m_keys[mid] < key)
				{
					l = mid + 1;
				}
				else
				{
					h = mid;
				}
			}
			if (l + 1 == m_size && m_keys[l] < key)
			{
				return l + 1;
			}
			return l;
		}

	private:
		IAllocator&		m_allocator;
		Key*			m_keys;
		Value*			m_values;
		e_int32			m_size;
		e_int32			m_capacity;
	};
}
#endif
