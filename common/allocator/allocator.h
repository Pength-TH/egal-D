#ifndef _allocator_h_
#define _allocator_h_

struct PlacementNewDummy {};
inline void* operator new(size_t, PlacementNewDummy, void* ptr) { return ptr; }
inline void operator delete(void*, PlacementNewDummy, void*) {}

namespace egal
{
#define ALIGN_OF(...) __alignof(__VA_ARGS__) //sizeof(__VA_ARGS__) ^ (sizeof(__VA_ARGS__) & (sizeof(__VA_ARGS__) - 1))
#define _new(var)  new(PlacementNewDummy(), var)
#define _aligned_new(allocator, ...) new (PlacementNewDummy(), (allocator).allocate_aligned(sizeof(__VA_ARGS__), ALIGN_OF(__VA_ARGS__))) __VA_ARGS__
#define _delete(allocator, var) (allocator).deleteObject(var);

	struct IAllocator
	{
		virtual ~IAllocator() {}

		virtual void* allocate(size_t size) = 0;
		virtual void deallocate(void* ptr) = 0;
		virtual void* reallocate(void* ptr, size_t size) = 0;

		virtual void* allocate_aligned(size_t size, size_t align) = 0;
		virtual void deallocate_aligned(void* ptr) = 0;
		virtual void* reallocate_aligned(void* ptr, size_t size, size_t align) = 0;

		template <class T> void deleteObject(T* ptr)
		{
			if (ptr)
			{
				ptr->~T();
				deallocate_aligned(ptr);
			}
		}
	};

	template <class Alloc>
	class AllocatedObject
	{
	public:
		explicit AllocatedObject()
		{ }

		~AllocatedObject()
		{ }

#if _DEBUG
		void* operator new(size_t sz, size_t block, const char* file, int line)
		{
			return Alloc::allocate(sz, file, line, 0);
		}

		/// operator new, with debug line info
		void* operator new(size_t sz, const char* file, int line, const char* func)
		{
			return Alloc::allocate(sz, file, line, func);
		}

		void* operator new(size_t sz)
		{
			return Alloc::allocate(sz);
		}

		/// placement operator new
		void* operator new(size_t sz, void* ptr)
		{
			(void)sz;
			return ptr;
		}

		/// array operator new, with debug line info
		void* operator new[](size_t sz, const char* file, int line, const char* func)
		{
			return Alloc::allocate(sz, file, line, func);
		}

			void* operator new[](size_t sz)
		{
			return Alloc::allocate(sz);
		}

			void operator delete(void* ptr)
		{
			Alloc::deallocate(ptr);
		}

		// Corresponding operator for placement delete (second param same as the first)
		void operator delete(void* ptr, void*)
		{
			Alloc::deallocate(ptr);
		}

		void operator delete(void* ptr, size_t block, const char* file, int line)
		{
			return Alloc::deallocate(ptr);
		}

		// only called if there is an exception in corresponding 'new'
		void operator delete(void* ptr, const char*, int, const char*)
		{
			Alloc::deallocate(ptr);
		}

		void operator delete[](void* ptr)
		{
			Alloc::deallocate(ptr);
		}

			void operator delete[](void* ptr, const char*, int, const char*)
		{
			Alloc::deallocate(ptr);
		}
#endif

	};

//-------------------------------------------------------------------------------
/**
* default
*/
#if USE_DEFAULT_ALLOCATOR
	/**
	* default_allocator
	*/
	class DefaultAllocator : public IAllocator
	{
	public:
		void* allocate(size_t n) override
		{
			return malloc(n);;
		}

		void deallocate(void* p) override
		{
			free(p);
		}

		void* reallocate(void* ptr, size_t size) override
		{
			return realloc(ptr, size);
		}

		void* allocate_aligned(size_t size, size_t align) override
		{
#if PLATFORM_WINDOWS
			return _aligned_malloc(size, align);
#else
			return aligned_alloc(align, size);
#endif
		}
		void deallocate_aligned(void* ptr) override
		{
#if PLATFORM_WINDOWS
			_aligned_free(ptr);
#else
			free(ptr);
#endif
		}
		void* reallocate_aligned(void* ptr, size_t size, size_t align) override
		{
#if PLATFORM_WINDOWS
			return _aligned_realloc(ptr, size, align);
#else
			if (size == 0) 
			{
				free(ptr);
				return nullptr;
			}
			void* newptr = aligned_alloc(align, size);
			if (newptr == nullptr) 
			{
				return nullptr;
			}
			memcpy(newptr, ptr, malloc_usable_size(ptr));
			free(ptr);
			return newptr;
#endif
		}
	};

	typedef AllocatedObject<DefaultAllocator> GeneralAllocatedObject;
#endif


	//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
	extern IAllocator* g_allocator;
}

#endif
