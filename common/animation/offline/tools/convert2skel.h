
#ifndef ANIMATION_OFFLINE_TOOLS_CONVERT2SKEL_H_
#define ANIMATION_OFFLINE_TOOLS_CONVERT2SKEL_H_
#include "common/animation/platform.h"
namespace egal
{
	namespace animation
	{
		namespace offline
		{

			struct RawSkeleton;

			class SkeletonConverter
			{
			public:
				int operator()(int _argc, const char** _argv);

			private:
				virtual bool Import(const char* _filename,
					egal::animation::offline::RawSkeleton* _skeleton) = 0;
			};
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_OFFLINE_TOOLS_CONVERT2SKEL_H_
