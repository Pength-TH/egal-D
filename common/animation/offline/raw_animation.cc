#include "common/animation/offline/raw_animation.h"
#include "common/animation/skeleton.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			RawAnimation::RawAnimation()
				: duration(1.f)
				, tracks(*g_allocator)
				, name(*g_allocator)
			{

			}

			RawAnimation::~RawAnimation()
			{}

			namespace
			{

				// Implements key frames' time range and ordering checks.
				// See AnimationBuilder::Create for more details.
				template <typename _Key>
				static bool ValidateTrack(const typename TVector<_Key>& _track,
					float _duration)
				{
					float previous_time = -1.f;
					for (size_t k = 0; k < _track.size(); ++k)
					{
						const float frame_time = _track[k].time;
						// Tests frame's time is in range [0:duration].
						if (frame_time < 0.f || frame_time > _duration)
						{
							return false;
						}
						// Tests that frames are sorted.
						if (frame_time <= previous_time)
						{
							return false;
						}
						previous_time = frame_time;
					}
					return true;  // Validated.
				}
			}  // namespace

			bool RawAnimation::Validate() const
			{
				if (duration <= 0.f)
				{  // Tests duration is valid.
					return false;
				}
				if (tracks.size() > Skeleton::kMaxJoints)
				{  // Tests number of tracks.
					return false;
				}
				// Ensures that all key frames' time are valid, ie: in a strict ascending
				// order and within range [0:duration].
				for (size_t j = 0; j < tracks.size(); ++j)
				{
					const RawAnimation::JointTrack& track = tracks[j];
					if (!ValidateTrack<TranslationKey>(track.translations, duration) ||
						!ValidateTrack<RotationKey>(track.rotations, duration) ||
						!ValidateTrack<ScaleKey>(track.scales, duration))
					{
						return false;
					}
				}

				return true;  // *this is valid.
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
