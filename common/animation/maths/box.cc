#include "common/animation/maths/box.h"
#include "common/animation/maths/math_ex.h"
#include <limits>
namespace egal
{
	namespace math
	{
		Box::Box()
			: min(std::numeric_limits<float>::max()),
			max(-std::numeric_limits<float>::max())
		{}

		Box::Box(const Float3* _points, size_t _stride, size_t _count)
		{
			Float3 local_min(std::numeric_limits<float>::max());
			Float3 local_max(-std::numeric_limits<float>::max());

			const Float3* end = Stride(_points, _stride * _count);
			for (; _points < end; _points = Stride(_points, _stride))
			{
				local_min = Min(local_min, *_points);
				local_max = Max(local_max, *_points);
			}

			min = local_min;
			max = local_max;
		}
	}  // namespace math
}  // namespace egal
