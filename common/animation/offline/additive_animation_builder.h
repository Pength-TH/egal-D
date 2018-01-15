
#ifndef ANIMATION_OFFLINE_ADDITIVE_ANIMATION_BUILDER_H_
#define ANIMATION_OFFLINE_ADDITIVE_ANIMATION_BUILDER_H_
#include "common/animation/platform.h"
namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Forward declare offline animation type.
			struct RawAnimation;

			// Defines the class responsible for building a delta animation from an offline
			// raw animation. This is used to create animations compatible with additive
			// blending.
			class AdditiveAnimationBuilder
			{
			public:
				// Initializes the builder.
				AdditiveAnimationBuilder();

				// Builds delta animation from _input..
				// Returns true on success and fills _output_animation with the delta
				// version of _input animation.
				// *_output must be a valid RawAnimation instance.
				// Returns false on failure and resets _output to an empty animation.
				// See RawAnimation::Validate() for more details about failure reasons.
				bool operator()(const RawAnimation& _input, RawAnimation* _output) const;
			};
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_ADDITIVE_ANIMATION_BUILDER_H_
