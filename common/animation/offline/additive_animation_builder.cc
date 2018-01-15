
#include "common/animation/offline/additive_animation_builder.h"

#include <cassert>
#include <cstddef>

#include "common/animation/offline/raw_animation.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			namespace
			{
				template <typename _RawTrack, typename _MakeDelta>
				void MakeDelta(const _RawTrack& _src, const _MakeDelta& _make_delta,
					_RawTrack* _dest)
				{
					_dest->reserve(_src.size());

					// Early out if no key.
					if (_src.empty())
					{
						return;
					}

					// Stores reference value.
					typename _RawTrack::const_reference reference = _src[0];

					// Copy animation keys.
					for (size_t i = 0; i < _src.size(); ++i)
					{
						const typename _RawTrack::value_type delta =
						{
													_src[i].time, _make_delta(reference.value, _src[i].value) };
						_dest->push_back(delta);
					}
				}

				math::Float3 MakeDeltaTranslation(const math::Float3& _reference,
					const math::Float3& _value)
				{
					return _value - _reference;
				}

				math::Quaternion MakeDeltaRotation(const math::Quaternion& _reference,
					const math::Quaternion& _value)
				{
					return _value * Conjugate(_reference);
				}

				math::Float3 MakeDeltaScale(const math::Float3& _reference,
					const math::Float3& _value)
				{
					return _value / _reference;
				}
			}  // namespace

			// Setup default values (favoring quality).
			AdditiveAnimationBuilder::AdditiveAnimationBuilder()
			{}

			bool AdditiveAnimationBuilder::operator()(const RawAnimation& _input,
				RawAnimation* _output) const
			{
				if (!_output)
				{
					return false;
				}
				// Reset output animation to default.
				*_output = RawAnimation();

				// Validate animation.
				if (!_input.Validate())
				{
					return false;
				}

				// Rebuilds output animation.
				_output->duration = _input.duration;
				_output->tracks.resize(_input.tracks.size());

				for (size_t i = 0; i < _input.tracks.size(); ++i)
				{
					MakeDelta(_input.tracks[i].translations, MakeDeltaTranslation,
						&_output->tracks[i].translations);
					MakeDelta(_input.tracks[i].rotations, MakeDeltaRotation,
						&_output->tracks[i].rotations);
					MakeDelta(_input.tracks[i].scales, MakeDeltaScale,
						&_output->tracks[i].scales);
				}

				// Output animation is always valid though.
				return _output->Validate();
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
