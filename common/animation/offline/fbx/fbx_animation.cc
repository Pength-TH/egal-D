
#define INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "common/animation/offline/fbx/fbx_animation.h"
#include "common/animation/offline/raw_animation.h"
#include "common/animation/skeleton.h"
#include "common/animation/skeleton_utils.h"
#include "common/animation/maths/transform.h"
#include "common/egal-d.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{
			namespace fbx
			{

				namespace
				{
					bool ExtractAnimation(FbxSceneLoader* _scene_loader, FbxAnimStack* anim_stack,
						const Skeleton& _skeleton, float _sampling_rate,
						RawAnimation* _animation)
					{
						FbxScene* scene = _scene_loader->scene();
						assert(scene);

						log_info("Extracting animation [%s].", anim_stack->GetName());
						// Setup Fbx animation evaluator.
						scene->SetCurrentAnimationStack(anim_stack);

						// Set animation name.
						_animation->name = anim_stack->GetName();

						// Extract animation duration.
						FbxTimeSpan time_spawn;
						const FbxTakeInfo* take_info = scene->GetTakeInfo(anim_stack->GetName());
						if (take_info)
						{
							time_spawn = take_info->mLocalTimeSpan;
						}
						else
						{
							scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(time_spawn);
						}

						// Get frame rate from the scene.
						FbxTime::EMode mode = scene->GetGlobalSettings().GetTimeMode();
						const float scene_frame_rate =
							static_cast<float>((mode == FbxTime::eCustom)
								? scene->GetGlobalSettings().GetCustomFrameRate()
								: FbxTime::GetFrameRate(mode));

						// Deduce sampling period.
						// Scene frame rate is used when provided argument is <= 0.
						float sampling_rate;
						if (_sampling_rate > 0.f)
						{
							sampling_rate = _sampling_rate;
							log_info("Using sampling rate of %s hz.", sampling_rate);
						}
						else
						{
							sampling_rate = scene_frame_rate;
							log_info("Using scene sampling rate of %s hz.", sampling_rate);
						}
						const float sampling_period = 1.f / sampling_rate;

						// Get scene start and end.
						const float start =
							static_cast<float>(time_spawn.GetStart().GetSecondDouble());
						const float end = static_cast<float>(time_spawn.GetStop().GetSecondDouble());

						// Animation duration could be 0 if it's just a pose. In this case we'll set a
						// default 1s duration.
						if (end > start)
						{
							_animation->duration = end - start;
						}
						else
						{
							_animation->duration = 1.f;
						}

						// Allocates all tracks with the same number of joints as the skeleton.
						// Tracks that would not be found will be set to skeleton bind-pose
						// transformation.
						_animation->tracks.resize(_skeleton.num_joints());

						// Iterate all skeleton joints and fills there track with key frames.
						FbxAnimEvaluator* evaluator = scene->GetAnimationEvaluator();
						for (int i = 0; i < _skeleton.num_joints(); i++)
						{
							RawAnimation::JointTrack& track = _animation->tracks[i];

							// Find a node that matches skeleton joint.
							const char* joint_name = _skeleton.joint_names()[i];
							FbxNode* node = scene->FindNodeByName(joint_name);

							if (!node)
							{
								// Empty joint track.
								log_info("No animation track found for joint %s . Using skeleton bind pose instead.", joint_name);

								// Get joint's bind pose.
								const egal::math::Transform& bind_pose =
									egal::animation::GetJointLocalBindPose(_skeleton, i);

								const RawAnimation::TranslationKey tkey =
								{ 0.f, bind_pose.translation };
								track.translations.push_back(tkey);

								const RawAnimation::RotationKey rkey =
								{ 0.f, bind_pose.rotation };
								track.rotations.push_back(rkey);

								const RawAnimation::ScaleKey skey =
								{ 0.f, bind_pose.scale };
								track.scales.push_back(skey);

								continue;
							}

							// Reserve keys in animation tracks (allocation strategy optimization
							// purpose).
							const int max_keys =
								static_cast<int>(3.f + (end - start) / sampling_period);
							track.translations.reserve(max_keys);
							track.rotations.reserve(max_keys);
							track.scales.reserve(max_keys);

							// Evaluate joint transformation at the specified time.
							// Make sure to include "end" time, and enter the loop once at least.
							bool loop_again = true;
							for (float t = start; loop_again; t += sampling_period)
							{
								if (t >= end)
								{
									t = end;
									loop_again = false;
								}

								// Evaluate transform matric at t.
								const FbxAMatrix matrix =
									_skeleton.joint_properties()[i].parent == Skeleton::kNoParentIndex
									? evaluator->GetNodeGlobalTransform(node, FbxTimeSeconds(t))
									: evaluator->GetNodeLocalTransform(node, FbxTimeSeconds(t));

								// Convert to a transform obejct in egal unit/axis system.
								egal::math::Transform transform;
								if (!_scene_loader->converter()->ConvertTransform(matrix, &transform))
								{
									log_error("Failed to extract animation transform for joint %s at t = %d", joint_name, t);
									return false;
								}

								// Fills corresponding track.
								const float local_time = t - start;
								const RawAnimation::TranslationKey translation =
								{ local_time,
																												  transform.translation };
								track.translations.push_back(translation);
								const RawAnimation::RotationKey rotation =
								{ local_time,
																											transform.rotation };
								track.rotations.push_back(rotation);
								const RawAnimation::ScaleKey scale =
								{ local_time, transform.scale };
								track.scales.push_back(scale);
							}
						}

						// Output animation must be valid at that point.
						assert(_animation->Validate());

						return true;
					}
				}  // namespace

				bool ExtractAnimations(FbxSceneLoader* _scene_loader, const Skeleton& _skeleton,
					float _sampling_rate, Animations* _animations)
				{
					// Clears output
					_animations->clear();

					FbxScene* scene = _scene_loader->scene();
					assert(scene);

					int anim_stacks_count = scene->GetSrcObjectCount<FbxAnimStack>();

					// Early out if no animation's found.
					if (anim_stacks_count == 0)
					{
						log_error("No animation found.");
						return false;
					}

					// Prepares ouputs.
					_animations->resize(anim_stacks_count);

					// Sequentially import all available animations.
					bool success = true;
					for (int i = 0; i < anim_stacks_count && success; ++i)
					{
						FbxAnimStack* anim_stack = scene->GetSrcObject<FbxAnimStack>(i);
						success &= ExtractAnimation(_scene_loader, anim_stack, _skeleton,
							_sampling_rate, &_animations->at(i));
					}

					// Clears output if somthing failed during import, avoids partial data.
					if (!success)
					{
						_animations->clear();
					}

					return success;
				}
			}  // namespace fbx
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
