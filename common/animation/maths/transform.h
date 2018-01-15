
#ifndef MATHS_TRANSFORM_H_
#define MATHS_TRANSFORM_H_

#include "common/animation/maths/quaternion.h"
#include "common/animation/maths/vec_float.h"
#include "common/animation/platform.h"

namespace egal
{
	namespace math
	{

		// Stores an affine transformation with separate translation, rotation and scale
		// attributes.
		struct Transform
		{
			// Translation affine transformation component.
			Float3 translation;

			// Rotation affine transformation component.
			Quaternion rotation;

			// Scale affine transformation component.
			Float3 scale;

			// Builds an identity transform.
			static INLINE Transform identity()
			{
				const Transform ret =
				{ Float3::zero(), Quaternion::identity(),
													   Float3::one() };
				return ret;
			}
		};
	}  // namespace math
}  // namespace egal
#endif  // MATHS_TRANSFORM_H_
