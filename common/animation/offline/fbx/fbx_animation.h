
#ifndef ANIMATION_OFFLINE_FBX_FBX_ANIMATION_H_
#define ANIMATION_OFFLINE_FBX_FBX_ANIMATION_H_
#include "common/animation/platform.h"
#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_base.h"

namespace egal
{
	namespace animation
	{

		class Skeleton;

		namespace offline
		{

			struct RawAnimation;

			namespace fbx
			{

				bool ExtractAnimations(FbxSceneLoader* _scene_loader, const Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations);

			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_FBX_FBX_ANIMATION_H_
