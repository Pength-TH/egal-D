#ifndef ANIMATION_OFFLINE_RAW_ANIMATION_H_
#define ANIMATION_OFFLINE_RAW_ANIMATION_H_

#include "common/animation/io/archive_traits.h"
#include "common/animation/maths/quaternion.h"
#include "common/animation/maths/vec_float.h"

#include "common/egal-d.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Offline animation type.
			// This animation type is not intended to be used in run time. It is used to
			// define the offline animation object that can be converted to the runtime
			// animation using the AnimationBuilder.
			// This animation structure exposes tracks of keyframes. Keyframes are defined
			// with a time and a value which can either be a translation (3 float x, y, z),
			// a rotation (a quaternion) or scale coefficient (3 floats x, y, z). Tracks are
			// defined as a set of three different std::vectors (translation, rotation and
			// scales). Animation structure is then a vector of tracks, along with a
			// duration value.
			// Finally the RawAnimation structure exposes Validate() function to check that
			// it is valid, meaning that all the following rules are respected:
			//  1. Animation duration is greater than 0.
			//  2. Keyframes' time are sorted in a strict ascending order.
			//  3. Keyframes' time are all within [0,animation duration] range.
			// Animations that would fail this validation will fail to be converted by the
			// AnimationBuilder.
			struct RawAnimation
			{
				// Constructs an valid RawAnimation with a 1s default duration.
				RawAnimation();

				// Deallocates raw animation.
				~RawAnimation();

				// Tests for *this validity.
				// Returns true if animation data (duration, tracks) is valid:
				//  1. Animation duration is greater than 0.
				//  2. Keyframes' time are sorted in a strict ascending order.
				//  3. Keyframes' time are all within [0,animation duration] range.
				bool Validate() const;

				// Defines a raw translation key frame.
				struct TranslationKey
				{
					// Key frame time.
					float time;

					// Key frame value.
					typedef math::Float3 Value;
					Value value;

					// Provides identity transformation for a translation key.
					static math::Float3 identity()
					{
						return math::Float3::zero();
					}
				};

				// Defines a raw rotation key frame.
				struct RotationKey
				{
					// Key frame time.
					float time;

					// Key frame value.
					typedef math::Quaternion Value;
					math::Quaternion value;

					// Provides identity transformation for a rotation key.
					static math::Quaternion identity()
					{
						return math::Quaternion::identity();
					}
				};

				// Defines a raw scaling key frame.
				struct ScaleKey
				{
					// Key frame time.
					float time;

					// Key frame value.
					typedef math::Float3 Value;
					math::Float3 value;

					// Provides identity transformation for a scale key.
					static math::Float3 identity()
					{
						return math::Float3::one();
					}
				};

				// Defines a track of key frames for a bone, including translation, rotation
				// and scale.
				struct JointTrack
				{
					JointTrack()
						: translations(*g_allocator)
						, rotations(*g_allocator)
						, scales(*g_allocator)
					{

					}
					typedef TVector<TranslationKey> Translations;
					Translations translations;
					typedef TVector<RotationKey> Rotations;
					Rotations rotations;
					typedef TVector<ScaleKey> Scales;
					Scales scales;
				};

				// Returns the number of tracks of this animation.
				int num_tracks() const
				{
					return static_cast<int>(tracks.size());
				}

				// Stores per joint JointTrack, ie: per joint animation key-frames.
				// tracks_.size() gives the number of animated joints.
				TVector<JointTrack> tracks;

				// The duration of the animation. All the keys of a valid RawAnimation are in
				// the range [0,duration].
				float duration;

				// Name of the animation.
				egal::String name;
			};
		}  // namespace offline
	}  // namespace animation
	namespace io
	{
		//IO_TYPE_VERSION(2, animation::offline::RawAnimation)
		//IO_TYPE_TAG("egal-raw_animation", animation::offline::RawAnimation)

		// Should not be called directly but through io::Archive << and >> operators.
		template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation* _animations, size_t _count);

		template <>
		void Load(IArchive& _archive, animation::offline::RawAnimation* _animations,
			size_t _count, uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // ANIMATION_OFFLINE_RAW_ANIMATION_H_
