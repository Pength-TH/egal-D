
#include "common/animation/offline/skeleton_builder.h"

#include <cstring>

#include "common/animation/offline/raw_skeleton.h"
#include "common/animation/skeleton.h"
#include "common/animation/maths/soa_transform.h"


#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/egal_string.h"
#include "common/stl/tarrary.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			namespace
			{
				// Stores each traversed joint to a vector.
				struct JointLister
				{
					explicit JointLister(int _num_joints)
					{
						linear_joints.reserve(_num_joints);
					}
					void operator()(const RawSkeleton::Joint& _current,
						const RawSkeleton::Joint* _parent)
					{
						// Looks for the "lister" parent.
						int parent = Skeleton::kNoParentIndex;
						if (_parent)
						{
							// Start searching from the last joint.
							int j = static_cast<int>(linear_joints.size()) - 1;
							for (; j >= 0; --j)
							{
								if (linear_joints[j].joint == _parent)
								{
									parent = j;
									break;
								}
							}
							assert(parent >= 0);
						}
						const Joint listed =
						{ &_current, parent };
						linear_joints.push_back(listed);
					}
					struct Joint
					{
						const RawSkeleton::Joint* joint;
						int parent;
					};
					// Array of joints in the traversed DAG order.
					std::vector<Joint> linear_joints;
				};
			}  // namespace

			// Validates the RawSkeleton and fills a Skeleton.
			// Uses RawSkeleton::IterateJointsBF to traverse in DAG breadth-first order.
			// This favors cache coherency (when traversing joints) and reduces
			// Load-Hit-Stores (reusing the parent that has just been computed).
			Skeleton* SkeletonBuilder::operator()(const RawSkeleton& _raw_skeleton) const
			{
				// Tests _raw_skeleton validity.
				if (!_raw_skeleton.Validate())
				{
					return NULL;
				}

				// Everything is fine, allocates and fills the skeleton.
				// Will not fail.
				Skeleton* skeleton = (Skeleton*) g_allocator->allocate( sizeof(Skeleton) );
				const int num_joints = _raw_skeleton.num_joints();

				// Iterates through all the joint of the raw skeleton and fills a sorted joint
				// list.
				JointLister lister(num_joints);
				_raw_skeleton.IterateJointsBF<JointLister&>(lister);
				assert(static_cast<int>(lister.linear_joints.size()) == num_joints);

				// Computes name's buffer size.
				size_t chars_size = 0;
				for (int i = 0; i < num_joints; ++i)
				{
					const RawSkeleton::Joint& current = *lister.linear_joints[i].joint;
					chars_size += (StringUnitl::stringLength(current.name.c_str()) + 1) * sizeof(char);
				}

				// Allocates all skeleton members.
				char* cursor = skeleton->Allocate(chars_size, num_joints);

				// Copy names. All names are allocated in a single buffer. Only the first name
				// is set, all other names array entries must be initialized.
				for (int i = 0; i < num_joints; ++i)
				{
					const RawSkeleton::Joint& current = *lister.linear_joints[i].joint;
					skeleton->joint_names_[i] = cursor;
					strcpy(cursor, current.name.c_str());
					cursor += (StringUnitl::stringLength(current.name.c_str()) + 1) * sizeof(char);
				}

				// Transfers sorted joints hierarchy to the new skeleton.
				for (int i = 0; i < num_joints; ++i)
				{
					skeleton->joint_properties_[i].parent = lister.linear_joints[i].parent;
					skeleton->joint_properties_[i].is_leaf =
						lister.linear_joints[i].joint->children.empty();
				}

				// Transfers t-poses.
				const math::SimdFloat4 w_axis = math::simd_float4::w_axis();
				const math::SimdFloat4 zero = math::simd_float4::zero();
				const math::SimdFloat4 one = math::simd_float4::one();

				for (int i = 0; i < skeleton->num_soa_joints(); ++i)
				{
					math::SimdFloat4 translations[4];
					math::SimdFloat4 scales[4];
					math::SimdFloat4 rotations[4];
					for (int j = 0; j < 4; ++j)
					{
						if (i * 4 + j < num_joints)
						{
							const RawSkeleton::Joint& src_joint =
								*lister.linear_joints[i * 4 + j].joint;
							translations[j] =
								math::simd_float4::Load3PtrU(&src_joint.transform.translation.x);
							rotations[j] = math::NormalizeSafe4(
								math::simd_float4::LoadPtrU(&src_joint.transform.rotation.x),
								w_axis);
							scales[j] = math::simd_float4::Load3PtrU(&src_joint.transform.scale.x);
						}
						else
						{
							translations[j] = zero;
							rotations[j] = w_axis;
							scales[j] = one;
						}
					}
					// Fills the SoaTransform structure.
					math::Transpose4x3(translations, &skeleton->bind_pose_[i].translation.x);
					math::Transpose4x4(rotations, &skeleton->bind_pose_[i].rotation.x);
					math::Transpose4x3(scales, &skeleton->bind_pose_[i].scale.x);
				}

				return skeleton;  // Success.
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
