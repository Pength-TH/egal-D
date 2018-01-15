
#ifndef ANIMATION_RUNTIME_ANIMATION_H_
#define ANIMATION_RUNTIME_ANIMATION_H_

#include "common/animation/platform.h"
namespace egal
{
	namespace io
	{
		class IArchive;
		class OArchive;
	}  // namespace io
	namespace animation
	{
		// Forward declares the AnimationBuilder, used to instantiate an Animation.
		namespace offline
		{
			class AnimationBuilder;
		}

		// Forward declaration of key frame's type.
		struct TranslationKey;
		struct RotationKey;
		struct ScaleKey;

		// Defines a runtime skeletal animation clip.
		// The runtime animation data structure stores animation keyframes, for all the
		// joints of a skeleton. This structure is usually filled by the
		// AnimationBuilder and deserialized/loaded at runtime.
		// For each transformation type (translation, rotation and scale), Animation
		// structure stores a single array of keyframes that contains all the tracks
		// required to animate all the joints of a skeleton, matching breadth-first
		// joints order of the runtime skeleton structure. In order to optimize cache
		// coherency when sampling the animation, Keyframes in this array are sorted by
		// time, then by track number.
		class Animation
		{
		public:
			// Builds a default animation.
			Animation();

			// Declares the public non-virtual destructor.
			~Animation();

			// Gets the animation clip duration.
			float duration() const
			{
				return duration_;
			}

			// Gets the number of animated tracks.
			int num_tracks() const
			{
				return num_tracks_;
			}

			// Returns the number of SoA elements matching the number of tracks of *this
			// animation. This value is useful to allocate SoA runtime data structures.
			int num_soa_tracks() const
			{
				return (num_tracks_ + 3) / 4;
			}

			// Gets animation name.
			const char* name() const
			{
				return name_ ? name_ : "";
			}

			// Gets the buffer of translations keys.
			egal::Range<const TranslationKey> translations() const
			{
				return translations_;
			}

			// Gets the buffer of rotation keys.
			egal::Range<const RotationKey> rotations() const
			{
				return rotations_;
			}

			// Gets the buffer of scale keys.
			egal::Range<const ScaleKey> scales() const
			{
				return scales_;
			}

			// Get the estimated animation's size in bytes.
			size_t size() const;

			// Serialization functions.
			// Should not be called directly but through io::Archive << and >> operators.
			void Save(egal::io::OArchive& _archive) const;
			void Load(egal::io::IArchive& _archive, uint32_t _version);

		protected:
		private:
			// Disables copy and assignation.
			Animation(Animation const&);
			void operator=(Animation const&);

			// AnimationBuilder class is allowed to instantiate an Animation.
			friend class offline::AnimationBuilder;

			// Internal destruction function.
			void Allocate(size_t _name_len, size_t _translation_count,
				size_t _rotation_count, size_t _scale_count);
			void Deallocate();

			// Duration of the animation clip.
			float duration_;

			// The number of joint tracks. Can differ from the data stored in translation/
			// rotation/scale buffers because of SoA requirements.
			int num_tracks_;

			// Animation name.
			char* name_;

			// Stores all translation/rotation/scale keys begin and end of buffers.
			egal::Range<TranslationKey> translations_;
			egal::Range<RotationKey> rotations_;
			egal::Range<ScaleKey> scales_;
		};
	}  // namespace animation

	namespace io
	{
		//IO_TYPE_VERSION(4, animation::Animation);
		//IO_TYPE_TAG("animation", animation::Animation);
	}  // namespace io
}  // namespace egal
#endif  // ANIMATION_RUNTIME_ANIMATION_H_
