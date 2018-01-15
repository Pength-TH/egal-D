#ifndef ANIMATION_OFFLINE_FBX_FBX_H_
#define ANIMATION_OFFLINE_FBX_FBX_H_

#include "common/animation/offline/raw_animation.h"
#include "common/animation/platform.h"

namespace egal
{
	namespace animation
	{
		//  Forward declares egal runtime skeleton type.
		class Skeleton;

		namespace offline
		{
			//  Forward declares egal offline animation and skeleton types.
			struct RawSkeleton;

			namespace fbx
			{
				// Imports an offline skeleton from _filename fbx document.
				// _skeleton must point to a valid RawSkeleton instance, that will be cleared
				// and filled with skeleton data extracted from the fbx document.
				bool ImportFromFile(const char* _filename, RawSkeleton* _skeleton);

				// Vector of imported animations.
				typedef TVector<RawAnimation> Animations;

				// Imports an offline animation from _filename fbx document.
				// _animation must point to a valid RawSkeleton instance, that will be cleared
				// and filled with animation data extracted from the fbx document.
				// _skeleton is a run-time Skeleton object used to select and sort animation
				// tracks.
				bool ImportFromFile(const char* _filename, const Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations);
			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_FBX_FBX_H_
