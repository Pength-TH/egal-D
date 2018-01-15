
#ifndef ANIMATION_OFFLINE_SKELETON_BUILDER_H_
#define ANIMATION_OFFLINE_SKELETON_BUILDER_H_

#include "common/animation/platform.h"
#include "common/animation/maths/transform.h"

namespace egal
{
	namespace animation
	{
		// Forward declares the runtime skeleton type.
		class Skeleton;

		namespace offline
		{
			// Forward declares the offline skeleton type.
			struct RawSkeleton;

			// Defines the class responsible of building Skeleton instances.
			class SkeletonBuilder
			{
			public:
				// Creates a Skeleton based on _raw_skeleton and *this builder parameters.
				// Returns a Skeleton instance on success which will then be deleted using
				// the default allocator Delete() function.
				// Returns NULL on failure. See RawSkeleton::Validate() for more details about
				// failure reasons.
				Skeleton* operator()(const RawSkeleton& _raw_skeleton) const;
			};
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_SKELETON_BUILDER_H_
