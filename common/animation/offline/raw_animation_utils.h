#ifndef ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_
#define ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_

#include "common/animation/offline/raw_animation.h"

#include "common/animation/maths/transform.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Translation interpolation method.
			math::Float3 LerpTranslation(const math::Float3& _a, const math::Float3& _b,
				float _alpha);

			// Rotation interpolation method.
			math::Quaternion LerpRotation(const math::Quaternion& _a,
				const math::Quaternion& _b, float _alpha);

			// Scale interpolation method.
			math::Float3 LerpScale(const math::Float3& _a, const math::Float3& _b,
				float _alpha);
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_RAW_ANIMATION_UTILS_H_
