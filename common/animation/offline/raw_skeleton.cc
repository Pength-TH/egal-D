#include "common/animation/offline/raw_skeleton.h"

#include "common/animation/skeleton.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			RawSkeleton::RawSkeleton()
				:roots(*g_allocator)
			{
			}

			RawSkeleton::~RawSkeleton()
			{}

			bool RawSkeleton::Validate() const
			{
				if (num_joints() > Skeleton::kMaxJoints)
				{
					return false;
				}
				return true;
			}

			namespace
			{
				struct JointCounter
				{
					JointCounter() : num_joints(0)
					{}
					void operator()(const RawSkeleton::Joint&, const RawSkeleton::Joint*)
					{
						++num_joints;
					}
					int num_joints;
				};
			}  // namespace

			// Iterates through all the root children and count them.
			int RawSkeleton::num_joints() const
			{
				return IterateJointsDF(JointCounter()).num_joints;
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
