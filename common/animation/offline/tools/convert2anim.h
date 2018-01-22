
#ifndef ANIMATION_OFFLINE_TOOLS_CONVERT2ANIM_H_
#define ANIMATION_OFFLINE_TOOLS_CONVERT2ANIM_H_

#include "common/animation/platform.h"
#include "common/animation/offline/raw_animation.h"


namespace egal
{
	namespace animation
	{

		class Skeleton;

		namespace offline
		{

			class AnimationConverter
			{
			public:
				int operator()(int _argc, const char** _argv);

			protected:
				typedef std::vector<RawAnimation> Animations;

			private:
				virtual bool Import(const char* _filename,
					const egal::animation::Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations) = 0;
			};
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_TOOLS_CONVERT2ANIM_H_
