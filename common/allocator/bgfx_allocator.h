#ifndef _bgfx_allocator_h_
#define _bgfx_allocator_h_

#include "bx/allocator.h"

namespace egal
{
	struct bgfx_allocator : public bx::AllocatorI
	{
		explicit bgfx_allocator(egal::IAllocator& source)
			: m_source(source)
		{
		}

		static const size_t NATURAL_ALIGNEMENT = 8;

		void* realloc(void* _ptr, size_t _size, size_t _alignment, const char*, uint32_t)
		{
			if (0 == _size)
			{
				if (_ptr)
				{
					if (NATURAL_ALIGNEMENT >= _alignment)
					{
						m_source.deallocate(_ptr);
						return nullptr;
					}

					m_source.deallocate_aligned(_ptr);
				}

				return nullptr;
			}
			else if (!_ptr)
			{
				if (NATURAL_ALIGNEMENT >= _alignment)
					return m_source.allocate(_size);

				return m_source.allocate_aligned(_size, _alignment);
			}

			if (NATURAL_ALIGNEMENT >= _alignment)
				return m_source.reallocate(_ptr, _size);

			return m_source.reallocate_aligned(_ptr, _size, _alignment);
		}
		egal::IAllocator& m_source;
	};
}

#endif
