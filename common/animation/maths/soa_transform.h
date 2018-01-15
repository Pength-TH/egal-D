
#ifndef MATHS_SOA_TRANSFORM_H_
#define MATHS_SOA_TRANSFORM_H_

#include "common/animation/maths/soa_float.h"
#include "common/animation/maths/soa_quaternion.h"
#include "common/animation/platform.h"

namespace egal
{
	namespace math
	{

		// Stores an affine transformation with separate translation, rotation and scale
		// attributes.
		struct SoaTransform
		{
			SoaFloat3 translation;
			SoaQuaternion rotation;
			SoaFloat3 scale;

			static INLINE SoaTransform identity()
			{
				const SoaTransform ret =
				{ SoaFloat3::zero(), SoaQuaternion::identity(),
														  SoaFloat3::one() };
				return ret;
			}
		};
	}  // namespace math
}  // namespace egal
#endif  // MATHS_SOA_TRANSFORM_H_
