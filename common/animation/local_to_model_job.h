#ifndef ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_
#define ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_
#include "common/animation/platform.h"

namespace egal
{

	// Forward declaration math structures.
	namespace math
	{
		struct SoaTransform;
	}
	namespace math
	{
		struct Float4x4;
	}

	namespace animation
	{
		// Forward declares the Skeleton object used to describe joint hierarchy.
		class Skeleton;

		// Computes model-space joint matrices from local-space SoaTransform.
		// This job uses the skeleton to define joints parent-child hierarchy. The job
		// iterates through all joints to compute their transform relatively to the
		// skeleton root.
		// Job inputs is an array of SoaTransform objects (in local-space), ordered like
		// skeleton's joints. Job output is an array of matrices (in model-space),
		// ordered like skeleton's joints. Output are matrices, because the combination
		// of affine transformations can contain shearing or complex transformation
		// that cannot be represented as Transform object.
		struct LocalToModelJob
		{
			// Default constructor, initializes default values.
			LocalToModelJob() : skeleton(NULL)
			{}

			// Validates job parameters. Returns true for a valid job, or false otherwise:
			// -if any input pointer, including ranges, is NULL.
			// -if the size of the input is smaller than the skeleton's number of joints.
			// Note that this input has a SoA format.
			// -if the size of of the output is smaller than the skeleton's number of
			// joints.
			bool Validate() const;

			// Runs job's local-to-model task.
			// The job is validated before any operation is performed, see Validate() for
			// more details.
			// Returns false if job is not valid. See Validate() function.
			bool Run() const;

			// The Skeleton object describing the joint hierarchy used for local to
			// model space conversion.
			const Skeleton* skeleton;

			// Job input.
			// The input range that store local transforms.
			Range<const egal::math::SoaTransform> input;

			// Job output.
			// The output range to be filled with model matrices.
			Range<egal::math::Float4x4> output;
		};
	}  // namespace animation
}  // namespace egal
#endif  // ANIMATION_RUNTIME_LOCAL_TO_MODEL_JOB_H_
