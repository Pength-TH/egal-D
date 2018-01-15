
#include "common/animation/local_to_model_job.h"

#include <cassert>

#include "common/animation/maths/math_ex.h"
#include "common/animation/maths/simd_math.h"
#include "common/animation/maths/soa_float4x4.h"
#include "common/animation/maths/soa_transform.h"

#include "common/animation/skeleton.h"

namespace egal
{
	namespace animation
	{

		bool LocalToModelJob::Validate() const
		{
			// Don't need any early out, as jobs are valid in most of the performance
			// critical cases.
			// Tests are written in multiple lines in order to avoid branches.
			bool valid = true;

			// Test for NULL begin pointers.
			if (!skeleton)
			{
				return false;
			}
			valid &= input.begin != NULL;
			valid &= output.begin != NULL;

			const int num_joints = skeleton->num_joints();
			const int num_soa_joints = (num_joints + 3) / 4;

			// Test input and output ranges, implicitly tests for NULL end pointers.
			valid &= input.end - input.begin >= num_soa_joints;
			valid &= output.end - output.begin >= num_joints;

			return valid;
		}

		bool LocalToModelJob::Run() const
		{
			using math::SoaTransform;
			using math::SoaFloat4x4;
			using math::Float4x4;

			if (!Validate())
			{
				return false;
			}

			// Early out if no joint.
			const int num_joints = skeleton->num_joints();
			if (num_joints == 0)
			{
				return true;
			}

			// Fetch joint's properties.
			Range<const Skeleton::JointProperties> properties =
				skeleton->joint_properties();

			// Output.
			Float4x4* const model_matrices = output.begin;

			// Initializes an identity matrix that will be used to compute roots model
			// matrices without requiring a branch.
			const Float4x4 identity = Float4x4::identity();

			// Converts to matrices and applies hierarchical transformation.
			for (int joint = 0; joint < num_joints;)
			{
				// Builds soa matrices from soa transforms.
				const SoaTransform& transform = input.begin[joint / 4];
				const SoaFloat4x4 local_soa_matrices = SoaFloat4x4::FromAffine(
					transform.translation, transform.rotation, transform.scale);
				// Converts to aos matrices.
				math::SimdFloat4 local_aos_matrices[16];
				math::Transpose16x16(&local_soa_matrices.cols[0].x, local_aos_matrices);

				// Applies hierarchical transformation.
				const int proceed_up_to = joint + math::Min(4, num_joints - joint);
				const math::SimdFloat4* local_aos_matrix = local_aos_matrices;
				for (; joint < proceed_up_to; ++joint, local_aos_matrix += 4)
				{
					const int parent = properties.begin[joint].parent;
					const Float4x4* parent_matrix =
						math::Select(parent == Skeleton::kNoParentIndex, &identity,
							&model_matrices[parent]);
					const Float4x4 local_matrix =
					{
					{local_aos_matrix[0], local_aos_matrix[1],
																		local_aos_matrix[2],
																		local_aos_matrix[3]} };
					model_matrices[joint] = (*parent_matrix) * local_matrix;
				}
			}
			return true;
		}
	}  // namespace animation
}  // namespace egal
