
#define INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_base.h"

#include "common/animation/offline/fbx/fbx_animation.h"
#include "common/animation/offline/fbx/fbx_skeleton.h"

#include "common/animation/offline/raw_animation.h"
#include "common/animation/offline/raw_skeleton.h"

#include "common/egal-d.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{
			namespace fbx
			{

				bool ImportFromFile(const char* _filename, RawSkeleton* _skeleton)
				{
					if (!_skeleton)
					{
						return false;
					}
					// Reset skeleton.
					*_skeleton = RawSkeleton();

					// Import Fbx content.
					FbxManagerInstance fbx_manager;
					FbxSkeletonIOSettings settings(fbx_manager);
					FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
					if (!scene_loader.scene())
					{
						log_error("Failed to import file %s.", _filename);
						return false;
					}

					if (!ExtractSkeleton(scene_loader, _skeleton))
					{
						log_error("Fbx skeleton extraction failed.");
						return false;
					}

					return true;
				}

				bool ImportFromFile(const char* _filename, const Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations)
				{
					if (!_animations)
					{
						return false;
					}
					// Reset animation.
					_animations->clear();

					// Import Fbx content.
					FbxManagerInstance fbx_manager;
					FbxAnimationIOSettings settings(fbx_manager);
					FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
					if (!scene_loader.scene())
					{
						log_error("Failed to import file %s.", _filename);
						return false;
					}

					if (!ExtractAnimations(&scene_loader, _skeleton, _sampling_rate,
						_animations))
					{
						log_error("Fbx animation extraction failed.");
						return false;
					}

					return true;
				}
			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
