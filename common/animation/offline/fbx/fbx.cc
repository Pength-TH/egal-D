
#define INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_base.h"

#include "common/animation/offline/fbx/fbx_animation.h"
#include "common/animation/offline/fbx/fbx_skeleton.h"

#include "common/animation/offline/raw_animation.h"
#include "common/animation/offline/raw_skeleton.h"



namespace egal
{
	namespace animation
	{
		namespace offline
		{
			namespace fbx
			{

				ReturnType ImportFromFile(const char* _filename, RawSkeleton* _skeleton)
				{
					if (!_skeleton)
					{
						return ERROR_LOAD_ERROR;
					}
					// Reset skeleton.
					*_skeleton = RawSkeleton();

					// Import Fbx content.
					FbxManagerInstance fbx_manager;
					FbxSkeletonIOSettings settings(fbx_manager);
					FbxSceneLoader scene_loader(_filename, "", fbx_manager, settings);
					if (!scene_loader.scene())
					{
						log_error("Failed to import file %s. the fbx file no scene data.", _filename);
						return ERROR_LOAD_SCENE;
					}

					if (!ExtractSkeleton(scene_loader, _skeleton))
					{
						log_info("Fbx skeleton extraction failed.");
						return ERROR_LOAD_SKELETON;
					}

					return ERROR_LOAD_END;
				}

				ReturnType ImportFromFile(const char* _filename, const Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations)
				{
					if (!_animations)
					{
						return ERROR_LOAD_ERROR;
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
						return ERROR_LOAD_SCENE;
					}

					if (!ExtractAnimations(&scene_loader, _skeleton, _sampling_rate,
						_animations))
					{
						log_error("Fbx animation extraction failed.");
						return ERROR_LOAD_ANIMATION;
					}

					return ERROR_LOAD_END;
				}
			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
