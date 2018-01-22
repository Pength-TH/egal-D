
#include "common/animation/offline/animation_optimizer.h"
#include "common/animation/maths/math_constant.h"
#include "common/animation/maths/math_ex.h"
#include "common/animation/offline/raw_animation.h"
#include "common/animation/offline/raw_animation_utils.h"
#include "common/animation/skeleton.h"
#include "common/animation/skeleton_utils.h"

#include <cassert>
#include <cstddef>

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			// Setup default values (favoring quality).
			AnimationOptimizer::AnimationOptimizer()
				: translation_tolerance(1e-3f),                 // 1 mm.
				rotation_tolerance(.1f * math::kPi / 180.f),  // 0.1 degree.
				scale_tolerance(1e-3f),                       // 0.1%.
				hierarchical_tolerance(1e-3f)
			{               // 1 mm.
			}

			namespace
			{

				struct JointSpec
				{
					float length;
					float scale;
				};

				typedef std::vector<JointSpec> JointSpecs;

				JointSpec Iter(const Skeleton& _skeleton, uint16_t _joint,
					const JointSpecs& _local_joint_specs,
					float _parent_accumulated_scale,
					JointSpecs* _hierarchical_joint_specs)
				{
					JointSpec local_joint_spec = _local_joint_specs[_joint];
					JointSpec& hierarchical_joint_spec = _hierarchical_joint_specs->at(_joint);
					const Skeleton::JointProperties* properties =
						_skeleton.joint_properties().begin;

					// Applies parent's scale to this joint.
					uint16_t parent = properties[_joint].parent;
					if (parent != Skeleton::kNoParentIndex)
					{
						local_joint_spec.length *= _parent_accumulated_scale;
						local_joint_spec.scale *= _parent_accumulated_scale;
					}

					if (properties[_joint].is_leaf)
					{
						// Set leaf length to 0, as track's own tolerance checks are enough for a
						// leaf.
						hierarchical_joint_spec.length = 0.f;
						hierarchical_joint_spec.scale = 1.f;
					}
					else
					{
						// Find first child.
						uint16_t child = _joint + 1;
						for (; child < _skeleton.num_joints() && properties[child].parent != _joint;
							++child)
						{
						}
						assert(properties[child].parent == _joint);

						// Now iterate childs.
						for (; child < _skeleton.num_joints() && properties[child].parent == _joint;
							++child)
						{
							// Entering each child.
							const JointSpec child_spec =
								Iter(_skeleton, child, _local_joint_specs, local_joint_spec.scale,
									_hierarchical_joint_specs);

							// Accumulated each child specs to this joint.
							hierarchical_joint_spec.length =
								math::_Max(hierarchical_joint_spec.length, child_spec.length);
							hierarchical_joint_spec.scale =
								math::_Max(hierarchical_joint_spec.scale, child_spec.scale);
						}
					}

					// Returns accumulated specs for this joint.
					const JointSpec spec =
					{
											hierarchical_joint_spec.length + local_joint_spec.length,
											hierarchical_joint_spec.scale * _local_joint_specs[_joint].scale };
					return spec;
				}

				void BuildHierarchicalSpecs(const RawAnimation& _animation,
					const Skeleton& _skeleton,
					JointSpecs* _hierarchical_joint_specs)
				{
					assert(_animation.num_tracks() == _skeleton.num_joints());

					// Early out if no joint.
					if (_animation.num_tracks() == 0)
					{
						return;
					}

					// Extracts maximum translations and scales for each track.
					JointSpecs local_joint_specs;
					local_joint_specs.resize(_animation.tracks.size());
					_hierarchical_joint_specs->resize(_animation.tracks.size());

					for (size_t i = 0; i < _animation.tracks.size(); ++i)
					{
						const RawAnimation::JointTrack& track = _animation.tracks[i];

						float max_length = 0.f;
						for (size_t j = 0; j < track.translations.size(); ++j)
						{
							max_length =
								math::_Max(max_length, LengthSqr(track.translations[j].value));
						}
						local_joint_specs[i].length = std::sqrt(max_length);

						float max_scale = 0.f;
						if (track.scales.size() != 0)
						{
							for (size_t j = 0; j < track.scales.size(); ++j)
							{
								max_scale = math::_Max(max_scale, track.scales[j].value.x);
								max_scale = math::_Max(max_scale, track.scales[j].value.y);
								max_scale = math::_Max(max_scale, track.scales[j].value.z);
							}
						}
						else
						{
							max_scale = 1.f;
						}
						local_joint_specs[i].scale = max_scale;
					}

					// Iterates all skeleton roots.
					for (uint16_t root = 0;
						root < _skeleton.num_joints() &&
						_skeleton.joint_properties()[root].parent == Skeleton::kNoParentIndex;
						++root)
					{
						// Entering each root.
						Iter(_skeleton, root, local_joint_specs, 1.f, _hierarchical_joint_specs);
					}
				}

				// Copy _src keys to _dest but except the ones that can be interpolated.
				template <typename _RawTrack, typename _Comparator, typename _Lerp>
				void Filter(const _RawTrack& _src, const _Comparator& _comparator,
					const _Lerp& _lerp, float _tolerance, float _hierarchical_tolerance,
					float _hierarchy_length, _RawTrack* _dest)
				{
					_dest->reserve(_src.size());

					// Only copies the key that cannot be interpolated from the others.
					size_t last_src_pushed = 0;  // Index (in src) of the last pushed key.
					for (size_t i = 0; i < _src.size(); ++i)
					{
						// First and last keys are always pushed.
						if (i == 0)
						{
							_dest->push_back(_src[i]);
							last_src_pushed = i;
						}
						else if (i == _src.size() - 1)
						{
							// Don't push the last value if it's the same as last_src_pushed.
							typename _RawTrack::const_reference left = _src[last_src_pushed];
							typename _RawTrack::const_reference right = _src[i];
							if (!_comparator(left.value, right.value, _tolerance,
								_hierarchical_tolerance, _hierarchy_length))
							{
								_dest->push_back(right);
								last_src_pushed = i;
							}
						}
						else
						{
							// Only inserts i key if keys in range ]last_src_pushed,i] cannot be
							// interpolated from keys last_src_pushed and i + 1.
							typename _RawTrack::const_reference left = _src[last_src_pushed];
							typename _RawTrack::const_reference right = _src[i + 1];
							for (size_t j = last_src_pushed + 1; j <= i; ++j)
							{
								typename _RawTrack::const_reference test = _src[j];
								const float alpha = (test.time - left.time) / (right.time - left.time);
								assert(alpha >= 0.f && alpha <= 1.f);
								if (!_comparator(_lerp(left.value, right.value, alpha), test.value,
									_tolerance, _hierarchical_tolerance,
									_hierarchy_length))
								{
									_dest->push_back(_src[i]);
									last_src_pushed = i;
									break;
								}
							}
						}
					}
					assert(_dest->size() <= _src.size());
				}

				// Translation filtering comparator.
				bool CompareTranslation(const math::Float3& _a, const math::Float3& _b,
					float _tolerance, float _hierarchical_tolerance,
					float _hierarchy_scale)
				{
					if (!Compare(_a, _b, _tolerance))
					{
						return false;
					}

					// Compute the position of the end of the hierarchy.
					const math::Float3 s(_hierarchy_scale);
					return Compare(_a * s, _b * s, _hierarchical_tolerance);
				}

				// Rotation filtering comparator.
				bool CompareRotation(const math::Quaternion& _a, const math::Quaternion& _b,
					float _tolerance, float _hierarchical_tolerance,
					float _hierarchy_length)
				{
					// Compute the shortest unsigned angle between the 2 quaternions.
					// diff_w is w component of a-1 * b.
					const float diff_w = _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
					const float angle = 2.f * std::acos(math::_Min(std::abs(diff_w), 1.f));
					if (std::abs(angle) > _tolerance)
					{
						return false;
					}

					// Deduces the length of the opposite segment at a distance _hierarchy_length.
					const float arc_length = std::sin(angle) * _hierarchy_length;
					return std::abs(arc_length) < _hierarchical_tolerance;
				}

				// Scale filtering comparator.
				bool CompareScale(const math::Float3& _a, const math::Float3& _b,
					float _tolerance, float _hierarchical_tolerance,
					float _hierarchy_length)
				{
					if (!Compare(_a, _b, _tolerance))
					{
						return false;
					}
					// Compute the position of the end of the hierarchy, in both cases _a and _b.
					const math::Float3 l(_hierarchy_length);
					return Compare(_a * l, _b * l, _hierarchical_tolerance);
				}
			}  // namespace

			bool AnimationOptimizer::operator()(const RawAnimation& _input,
				const Skeleton& _skeleton,
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

				// Validates the skeleton matches the animation.
				if (_input.num_tracks() != _skeleton.num_joints())
				{
					return false;
				}

				// First computes bone lengths, that will be used when filtering.
				JointSpecs hierarchical_joint_specs;
				BuildHierarchicalSpecs(_input, _skeleton, &hierarchical_joint_specs);

				// Rebuilds output animation.
				_output->name = _input.name;
				_output->duration = _input.duration;
				_output->tracks.resize(_input.tracks.size());

				for (size_t i = 0; i < _input.tracks.size(); ++i)
				{
					Filter(_input.tracks[i].translations, CompareTranslation, LerpTranslation,
						translation_tolerance, hierarchical_tolerance,
						hierarchical_joint_specs[i].scale, &_output->tracks[i].translations);
					Filter(_input.tracks[i].rotations, CompareRotation, LerpRotation,
						rotation_tolerance, hierarchical_tolerance,
						hierarchical_joint_specs[i].length, &_output->tracks[i].rotations);
					Filter(_input.tracks[i].scales, CompareScale, LerpScale, scale_tolerance,
						hierarchical_tolerance, hierarchical_joint_specs[i].length,
						&_output->tracks[i].scales);
				}

				// Output animation is always valid though.
				return _output->Validate();
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
