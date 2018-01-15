#include "common/animation/offline/raw_animation_utils.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Translation interpolation method.
			// This must be the same Lerp as the one used by the sampling job.
			math::Float3 LerpTranslation(const math::Float3& _a, const math::Float3& _b,
				float _alpha)
			{
				return math::Lerp(_a, _b, _alpha);
			}

			// Rotation interpolation method.
			// This must be the same Lerp as the one used by the sampling job.
			// The goal is to take the shortest path between _a and _b. This code replicates
			// this behavior that is actually not done at runtime, but when building the
			// animation.
			math::Quaternion LerpRotation(const math::Quaternion& _a,
				const math::Quaternion& _b, float _alpha)
			{
				// Finds the shortest path. This is done by the AnimationBuilder for runtime
				// animations.
				const float dot = _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
				return math::NLerp(_a, dot < 0.f ? -_b : _b, _alpha);  // _b an -_b are the
																	   // same rotation.
			}

			// Scale interpolation method.
			// This must be the same Lerp as the one used by the sampling job.
			math::Float3 LerpScale(const math::Float3& _a, const math::Float3& _b,
				float _alpha)
			{
				return math::Lerp(_a, _b, _alpha);
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
