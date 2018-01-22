#ifndef ANIMATION_OFFLINE_RAW_SKELETON_H_
#define ANIMATION_OFFLINE_RAW_SKELETON_H_

#include "common/animation/io/archive_traits.h"
#include "common/animation/maths/transform.h"

#include <vector>

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Off-line skeleton type.
			// This skeleton type is not intended to be used in run time. It is used to
			// define the offline skeleton object that can be converted to the runtime
			// skeleton using the SkeletonBuilder. This skeleton structure exposes joints'
			// hierarchy. A joint is defined with a name, a transformation (its bind pose),
			// and its children. Children are exposed as a public std::vector of joints.
			// This same type is used for skeleton roots, also exposed from the public API.
			// The public API exposed through std:vector's of joints can be used freely with
			// the only restriction that the total number of joints does not exceed
			// Skeleton::kMaxJoints.
			struct RawSkeleton
			{
				// Construct an empty skeleton.
				RawSkeleton();

				// The destructor is responsible for deleting the roots and their hierarchy.
				~RawSkeleton();

				// Offline skeleton joint type.
				struct Joint
				{
					// Type of the list of children joints.
					typedef std::vector<Joint> Children;

					// Children joints.
					Children children;

					// The name of the joint.
					std::string name;

					// Joint bind pose transformation in local space.
					math::Transform transform;
				};

				// Tests for *this validity.
				// Returns true on success or false on failure if the number of joints exceeds
				// egal::Skeleton::kMaxJoints.
				bool Validate() const;

				// Returns the number of joints of *this animation.
				// This function is not constant time as it iterates the hierarchy of joints
				// and counts them.
				int num_joints() const;

				// Applies a specified functor to each joint in a depth-first order.
				// _Fct is of type void(const Joint& _current, const Joint* _parent) where the
				// first argument is the child of the second argument. _parent is null if the
				// _current joint is the root.
				template <typename _Fct>
				_Fct IterateJointsDF(_Fct _fct) const
				{
					IterHierarchyDF(roots, NULL, _fct);
					return _fct;
				}

				// Applies a specified functor to each joint in a breadth-first order.
				// _Fct is of type void(const Joint& _current, const Joint* _parent) where the
				// first argument is the child of the second argument. _parent is null if the
				// _current joint is the root.
				template <typename _Fct>
				_Fct IterateJointsBF(_Fct _fct) const
				{
					IterHierarchyBF(roots, NULL, _fct);
					return _fct;
				}

				// Declares the skeleton's roots. Can be empty if the skeleton has no joint.
				Joint::Children roots;

			private:
				// Internal function used to iterate through joint hierarchy depth-first.
				template <typename _Fct>
				static void IterHierarchyDF(const RawSkeleton::Joint::Children& _children,
					const RawSkeleton::Joint* _parent, _Fct& _fct)
				{
					for (size_t i = 0; i < _children.size(); ++i)
					{
						const RawSkeleton::Joint& current = _children[i];
						_fct(current, _parent);
						IterHierarchyDF(current.children, &current, _fct);
					}
				}

				// Internal function used to iterate through joint hierarchy breadth-first.
				template <typename _Fct>
				static void IterHierarchyBF(const RawSkeleton::Joint::Children& _children,
					const RawSkeleton::Joint* _parent, _Fct& _fct)
				{
					for (size_t i = 0; i < _children.size(); ++i)
					{
						const RawSkeleton::Joint& current = _children[i];
						_fct(current, _parent);
					}
					for (size_t i = 0; i < _children.size(); ++i)
					{
						const RawSkeleton::Joint& current = _children[i];
						IterHierarchyBF(current.children, &current, _fct);
					}
				}
			};
		}  // namespace offline
	}  // namespace animation
	namespace io
	{
		
			
		IO_TYPE_VERSION(1, animation::offline::RawSkeleton);
		IO_TYPE_TAG("raw_skeleton", animation::offline::RawSkeleton);

			// Should not be called directly but through io::Archive << and >> operators.
		template <>
		void Save(OArchive& _archive, const animation::offline::RawSkeleton* _skeletons,
			size_t _count);

		template <>
		void Load(IArchive& _archive, animation::offline::RawSkeleton* _skeletons,
			size_t _count, uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // ANIMATION_OFFLINE_RAW_SKELETON_H_
