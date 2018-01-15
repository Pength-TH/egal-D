
#include "common/animation/animation.h"
#include "common/animation/maths/math_archive.h"
#include "common/animation/maths/math_ex.h"
#include "common/animation/io/archive.h"

#include <cassert>
#include <cstring>

// Internal include file
#define INCLUDE_PRIVATE_HEADER  // Allows to include private headers.
#include "common/animation/animation_keyframe.h"

#include "common/allocator/egal_allocator.h"
#include "common/utils/logger.h"

namespace egal
{
	namespace animation
	{

		Animation::Animation() : duration_(0.f), num_tracks_(0), name_(NULL)
		{}

		Animation::~Animation()
		{
			Deallocate();
		}

		void Animation::Allocate(size_t name_len, size_t _translation_count,
			size_t _rotation_count, size_t _scale_count)
		{
			// Distributes buffer memory while ensuring proper alignment (serves larger
			// alignment values first).
			STATIC_ASSERT(ALIGN_OF(TranslationKey) >= ALIGN_OF(RotationKey) &&
				ALIGN_OF(RotationKey) >= ALIGN_OF(ScaleKey) &&
				ALIGN_OF(ScaleKey) >= ALIGN_OF(char));

			assert(name_ == NULL && translations_.Size() == 0 && rotations_.Size() == 0 &&
				scales_.Size() == 0);

			// Compute overall size and allocate a single buffer for all the data.
			const size_t buffer_size = (name_len > 0 ? name_len + 1 : 0) +
				_translation_count * sizeof(TranslationKey) +
				_rotation_count * sizeof(RotationKey) +
				_scale_count * sizeof(ScaleKey);
			char* buffer = (char*)g_allocator->allocate(buffer_size);

			// Fix up pointers
			translations_.begin = reinterpret_cast<TranslationKey*>(buffer);
			assert(math::IsAligned(translations_.begin, ALIGN_OF(TranslationKey)));
			buffer += _translation_count * sizeof(TranslationKey);
			translations_.end = reinterpret_cast<TranslationKey*>(buffer);

			rotations_.begin = reinterpret_cast<RotationKey*>(buffer);
			assert(math::IsAligned(rotations_.begin, ALIGN_OF(RotationKey)));
			buffer += _rotation_count * sizeof(RotationKey);
			rotations_.end = reinterpret_cast<RotationKey*>(buffer);

			scales_.begin = reinterpret_cast<ScaleKey*>(buffer);
			assert(math::IsAligned(scales_.begin, ALIGN_OF(ScaleKey)));
			buffer += _scale_count * sizeof(ScaleKey);
			scales_.end = reinterpret_cast<ScaleKey*>(buffer);

			// Let name be NULL if animation has no name. Allows to avoid allocating this
			// buffer in the constructor of empty animations.
			name_ = reinterpret_cast<char*>(name_len > 0 ? buffer : NULL);
			assert(math::IsAligned(name_, ALIGN_OF(char)));
		}

		void Animation::Deallocate()
		{
			g_allocator->deallocate(translations_.begin);

			name_ = NULL;
			translations_ = egal::Range<TranslationKey>();
			rotations_ = egal::Range<RotationKey>();
			scales_ = egal::Range<ScaleKey>();
		}

		size_t Animation::size() const
		{
			const size_t size =
				sizeof(*this) + translations_.Size() + rotations_.Size() + scales_.Size();
			return size;
		}

		void Animation::Save(egal::io::OArchive& _archive) const
		{
			_archive << duration_;
			_archive << static_cast<int32_t>(num_tracks_);

			const size_t name_len = name_ ? std::strlen(name_) : 0;
			_archive << static_cast<int32_t>(name_len);

			const ptrdiff_t translation_count = translations_.Count();
			_archive << static_cast<int32_t>(translation_count);
			const ptrdiff_t rotation_count = rotations_.Count();
			_archive << static_cast<int32_t>(rotation_count);
			const ptrdiff_t scale_count = scales_.Count();
			_archive << static_cast<int32_t>(scale_count);

			_archive << egal::io::MakeArray(name_, name_len);

			for (ptrdiff_t i = 0; i < translation_count; ++i)
			{
				const TranslationKey& key = translations_.begin[i];
				_archive << key.time;
				_archive << key.track;
				_archive << egal::io::MakeArray(key.value);
			}

			for (ptrdiff_t i = 0; i < rotation_count; ++i)
			{
				const RotationKey& key = rotations_.begin[i];
				_archive << key.time;
				uint16_t track = key.track;
				_archive << track;
				uint8_t largest = key.largest;
				_archive << largest;
				bool sign = key.sign;
				_archive << sign;
				_archive << egal::io::MakeArray(key.value);
			}

			for (ptrdiff_t i = 0; i < scale_count; ++i)
			{
				const ScaleKey& key = scales_.begin[i];
				_archive << key.time;
				_archive << key.track;
				_archive << egal::io::MakeArray(key.value);
			}
		}

		void Animation::Load(egal::io::IArchive& _archive, uint32_t _version)
		{
			// Destroy animation in case it was already used before.
			Deallocate();
			duration_ = 0.f;
			num_tracks_ = 0;

			// No retro-compatibility with anterior versions.
			if (_version != 4)
			{
				log_error("Unsupported Animation version %s.", _version);
				return;
			}

			_archive >> duration_;

			int32_t num_tracks;
			_archive >> num_tracks;
			num_tracks_ = num_tracks;

			int32_t name_len;
			_archive >> name_len;
			int32_t translation_count;
			_archive >> translation_count;
			int32_t rotation_count;
			_archive >> rotation_count;
			int32_t scale_count;
			_archive >> scale_count;

			Allocate(name_len, translation_count, rotation_count, scale_count);

			if (name_)
			{  // NULL name_ is supported.
				_archive >> egal::io::MakeArray(name_, name_len);
				name_[name_len] = 0;
			}

			for (int i = 0; i < translation_count; ++i)
			{
				TranslationKey& key = translations_.begin[i];
				_archive >> key.time;
				_archive >> key.track;
				_archive >> egal::io::MakeArray(key.value);
			}

			for (int i = 0; i < rotation_count; ++i)
			{
				RotationKey& key = rotations_.begin[i];
				_archive >> key.time;
				uint16_t track;
				_archive >> track;
				key.track = track;
				uint8_t largest;
				_archive >> largest;
				key.largest = largest & 3;
				bool sign;
				_archive >> sign;
				key.sign = sign & 1;
				_archive >> egal::io::MakeArray(key.value);
			}

			for (int i = 0; i < scale_count; ++i)
			{
				ScaleKey& key = scales_.begin[i];
				_archive >> key.time;
				_archive >> key.track;
				_archive >> egal::io::MakeArray(key.value);
			}
		}
	}  // namespace animation
}  // namespace egal
