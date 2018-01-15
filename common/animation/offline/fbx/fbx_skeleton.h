
#ifndef ANIMATION_OFFLINE_FBX_FBX_SKELETON_H_
#define ANIMATION_OFFLINE_FBX_FBX_SKELETON_H_
#include "common/animation/platform.h"
#include "common/animation/offline/fbx/fbx_base.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			struct RawSkeleton;

			namespace fbx
			{

				bool ExtractSkeleton(FbxSceneLoader& _loader, RawSkeleton* _skeleton);

			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_FBX_FBX_SKELETON_H_
