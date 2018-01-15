
#ifndef ANIMATION_RUNTIME_SKELETON_UTILS_H_
#define ANIMATION_RUNTIME_SKELETON_UTILS_H_
#include "common/animation/platform.h"
#include "skeleton.h"

#include "common/animation/maths/transform.h"

namespace egal
{
	namespace animation
	{

		// Get bind-pose of a skeleton joint.
		egal::math::Transform GetJointLocalBindPose(const Skeleton& _skeleton,
			int _joint);

		// Defines the iterator structure used by IterateJointsDF to traverse joint
		// hierarchy.
		struct JointsIterator
		{
			uint16_t joints[Skeleton::kMaxJoints];
			int num_joints;
		};

		// Fills _iterator with the index of the joints of _skeleton traversed in depth-
		// first order.
		// _from indicates the join from which the joint hierarchy traversal begins. Use
		// Skeleton::kNoParentIndex to traverse the whole hierarchy, even if there are
		// multiple roots.
		// This function does not use a recursive implementation, to enforce a
		// predictable stack usage, independent off the data (joint hierarchy) being
		// processed.
		void IterateJointsDF(const Skeleton& _skeleton, int _from,
			JointsIterator* _iterator);

		// Applies a specified functor to each joint in a depth-first order.
		// _Fct is of type void(int _current, int _parent) where the first argument is
		// the child of the second argument. _parent is kNoParentIndex if the _current
		// joint is the root.
		// _from indicates the join from which the joint hierarchy traversal begins. Use
		// Skeleton::kNoParentIndex to traverse the whole hierarchy, even if there are
		// multiple joints.
		// This implementation is based on IterateJointsDF(*, *, JointsIterator$)
		// variant.
		template <typename _Fct>
		inline _Fct IterateJointsDF(const Skeleton& _skeleton, int _from, _Fct _fct)
		{
			// Iterates and fills iterator.
			JointsIterator iterator;
			IterateJointsDF(_skeleton, _from, &iterator);

			// Consumes iterator and call _fct.
			Range<const Skeleton::JointProperties> properties =
				_skeleton.joint_properties();
			for (int i = 0; i < iterator.num_joints; ++i)
			{
				const int joint = iterator.joints[i];
				_fct(joint, properties.begin[joint].parent);
			}
			return _fct;
		}
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_RUNTIME_SKELETON_UTILS_H_
