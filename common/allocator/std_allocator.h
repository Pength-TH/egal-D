#ifndef _STD_ALLOCATOR_H_
#define _STD_ALLOCATOR_H_

#include <new>

#include "common/allocator/egal_allocator.h"

namespace egal
{
	// Define a STL allocator compliant allocator->
	template <typename _Ty>
	class StdAllocator 
	{
	public:
		typedef _Ty value_type;                     // Element type.
		typedef value_type* pointer;                // Pointer to element.
		typedef value_type& reference;              // Reference to element.
		typedef const value_type* const_pointer;    // Constant pointer to element.
		typedef const value_type& const_reference;  // Constant reference to element.
		typedef size_t size_type;                   // Quantities of elements.
		typedef ptrdiff_t difference_type;		    // Difference between two pointers.

		// Converts an StdAllocator<_Ty> to an StdAllocator<_Other>.
		template <class _Other>
		struct rebind 
		{
			typedef StdAllocator<_Other> other;
		};

		// Returns address of mutable _val.
		pointer address(reference _val) const { return (&_val); }

		// Returns address of non-mutable _val.
		const_pointer address(const_reference _val) const { return (&_val); }

		// Constructs default allocator (does nothing).
		StdAllocator() {}

		// Constructs by copying (does nothing).
		StdAllocator(const StdAllocator<_Ty>&) {}

		// Constructs from a related allocator (does nothing).
		template <class _Other>
		StdAllocator(const StdAllocator<_Other>&) {}

		// Assigns from a related allocator (does nothing).
		template <class _Other>
		StdAllocator<_Ty>& operator=(const StdAllocator<_Other>&) { return (*this); }

		// Deallocates object at _Ptr, ignores size.
		void deallocate(pointer _ptr, size_type) 
		{
			g_allocator->deallocate(_ptr);
		}

		// Allocates array of _Count elements.
		pointer allocate(size_type _count) 
		{
			return (_Ty*)g_allocator->allocate(_count);
		}

		// Allocates array of _Count elements, ignores hint.
		pointer allocate(size_type _count, const void*) { return (allocate(_count)); }

		// Constructs object at _Ptr with value _val.
		void construct(pointer _ptr, const _Ty& _val) 
		{
			void* vptr = _ptr;
			::new (vptr) _Ty(_val);
		}

		// Destroys object at _Ptr.
		void destroy(pointer _ptr) 
		{
			if (_ptr) 
			{
				_ptr->~_Ty();
			}
		}

		// Estimates maximum array size.
		size_t max_size() const 
		{
			size_t count = static_cast<size_t>(-1) / sizeof(_Ty);
			return (count > 0 ? count : 1);
		}
	};

	// Tests for allocator equality (always true).
	template <class _Ty, class _Other>
	inline bool operator==(const StdAllocator<_Ty>&, const StdAllocator<_Other>&) 
	{
		return true;
	}

	// Tests for allocator inequality (always false).
	template <class _Ty, class _Other>
	inline bool operator!=(const StdAllocator<_Ty>&, const StdAllocator<_Other>&) 
	{
		return false;
	}
}
#endif  
