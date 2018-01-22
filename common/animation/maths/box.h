#ifndef MATHS_BOX_H_
#define MATHS_BOX_H_

#include <cstddef>
#include "common/animation/platform.h"
#include "common/animation/maths/vec_float.h"

namespace egal
{
	namespace math
	{

		// Defines an axis aligned box.
		struct Box
		{
			// Constructs an invalid box.
			Box();

			// Constructs a box with the specified _min and _max bounds.
			Box(const Float3& _min, const Float3& _max) : min_size(_min), max_size(_max)
			{}

			// Constructs the smallest box that contains the _count points _points.
			// _stride is the number of bytes points.
			Box(const Float3* _points, size_t _stride, size_t _count);

			// Tests whether *this is a valid box.
			bool is_valid() const
			{
				return min_size <= max_size;
			}

			// Tests whether _p is within box bounds.
			bool is_inside(const Float3& _p) const
			{
				return _p >= min_size && _p <= max_size;
			}

			// Box's min and max bounds.
			Float3 min_size;
			Float3 max_size;
		};

		// Merges two boxes _a and _b.
		// Both _a and _b can be invalid.
		INLINE Box Merge(const Box& _a, const Box& _b)
		{
			if (!_a.is_valid())
			{
				return _b;
			}
			else if (!_b.is_valid())
			{
				return _a;
			}
			return Box(Min(_a.min_size, _b.min_size), Max(_a.max_size, _b.max_size));
		}
	}  // namespace math
}  // namespace egal
#endif  // MATHS_BOX_H_
