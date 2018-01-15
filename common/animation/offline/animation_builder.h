
#ifndef ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
#define ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
#include "common/animation/platform.h"
#include "common/egal-d.h"

namespace egal
{
	namespace animation
	{

		// Forward declares the runtime animation type.
		class Animation;

		namespace offline
		{

			// Forward declares the offline animation type.
			struct RawAnimation;

			// Defines the class responsible of building runtime animation instances from
			// offline raw animations.
			// No optimization at all is performed on the raw animation.
			class AnimationBuilder
			{
			public:
				// Creates an Animation based on _raw_animation and *this builder parameters.
				// Returns a valid Animation on success
				// The returned animation will then need to be deleted using the default
				// allocator Delete() function.
				// See RawAnimation::Validate() for more details about failure reasons.
				Animation* operator()(const RawAnimation& _raw_animation) const;
			};
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_ANIMATION_BUILDER_H_
