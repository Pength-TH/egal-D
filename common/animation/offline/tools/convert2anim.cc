#include "common/animation/offline/tools/convert2anim.h"

#include <cstdlib>
#include <cstring>

#include "common/animation/offline/additive_animation_builder.h"
#include "common/animation/offline/animation_builder.h"
#include "common/animation/offline/animation_optimizer.h"
#include "common/animation/offline/raw_animation.h"
#include "common/animation/offline/raw_skeleton.h"
#include "common/animation/offline/skeleton_builder.h"
#include "common/animation/io/archive.h"

#include "common/animation/animation.h"
#include "common/animation/skeleton.h"


#include "common/animation/offline/tools/import_fbx.h"

namespace egal
{
	namespace animation
	{
		namespace offline
		{

			namespace
			{
				void DisplaysOptimizationstatistics(const RawAnimation& _non_optimized,
					const RawAnimation& _optimized)
				{
					size_t opt_translations = 0, opt_rotations = 0, opt_scales = 0;
					for (size_t i = 0; i < _optimized.tracks.size(); ++i)
					{
						const RawAnimation::JointTrack& track = _optimized.tracks[i];
						opt_translations += track.translations.size();
						opt_rotations += track.rotations.size();
						opt_scales += track.scales.size();
					}
					size_t non_opt_translations = 0, non_opt_rotations = 0, non_opt_scales = 0;
					for (size_t i = 0; i < _non_optimized.tracks.size(); ++i)
					{
						const RawAnimation::JointTrack& track = _non_optimized.tracks[i];
						non_opt_translations += track.translations.size();
						non_opt_rotations += track.rotations.size();
						non_opt_scales += track.scales.size();
					}

					// Computes optimization ratios.
					float translation_ratio =
						non_opt_translations != 0
						? 100.f * (non_opt_translations - opt_translations) /
						non_opt_translations
						: 0;
					float rotation_ratio =
						non_opt_rotations != 0
						? 100.f * (non_opt_rotations - opt_rotations) / non_opt_rotations
						: 0;
					float scale_ratio =
						non_opt_scales != 0
						? 100.f * (non_opt_scales - opt_scales) / non_opt_scales
						: 0;

					log_info("Optimization stage results:");
					log_info(" - Rotations key frames optimization: %d.", rotation_ratio);
					log_info(" - Scaling key frames optimization: %d.", scale_ratio);
				}

				egal::animation::Skeleton* ImportSkeleton(ImportOption &option)
				{
					// Reads the skeleton from the binary egal stream.
					egal::animation::Skeleton* skeleton = NULL;
					{
						log_info("Opens input skeleton egal binary file: %s", option.out_skeleton_path_name);
						egal::io::File file(option.out_skeleton_path_name.c_str(), "rb");
						if (!file.opened())
						{
							log_error("Failed to open input skeleton egal binary file: %s", option.out_skeleton_path_name);
							return NULL;
						}
						egal::io::IArchive archive(&file);

						// File could contain a RawSkeleton or a Skeleton.
						if (archive.TestTag<egal::animation::offline::RawSkeleton>())
						{
							log_info("Reading RawSkeleton from file.");

							// Reading the skeleton cannot file.
							egal::animation::offline::RawSkeleton raw_skeleton;
							archive >> raw_skeleton;

							// Builds runtime skeleton.
							log_info("Builds runtime skeleton.");
							egal::animation::offline::SkeletonBuilder builder;
							skeleton = builder(raw_skeleton);
							if (!skeleton)
							{
								log_error("Failed to build runtime skeleton.");
								return NULL;
							}
						}
						else if (archive.TestTag<egal::animation::Skeleton>())
						{
							// Reads input archive to the runtime skeleton.
							// This operation cannot fail.
							skeleton = (egal::animation::Skeleton*)g_allocator->allocate(sizeof(egal::animation::Skeleton));
							archive >> *skeleton;
						}
						else
						{
							log_error("Failed to read input skeleton from binary file: %s", option.out_skeleton_path_name);
							return NULL;
						}
					}
					return skeleton;
				}

				bool OutputSingleAnimation()
				{
					return strchr(OPTIONS_animation.value(), '*') == NULL;
				}

				String BuildFilename(const char* _filename, const char* _animation)
				{
					String output(_filename);
					const size_t asterisk = output.find('*');
					if (asterisk != std::string::npos)
					{
						output.replace(asterisk, 1, _animation);
					}
					return output;
				}

				bool Export(const egal::animation::offline::RawAnimation _raw_animation,
					const egal::animation::Skeleton& _skeleton)
				{
					// Raw animation to build and output.
					egal::animation::offline::RawAnimation raw_animation;

					// Make delta animation if requested.
					if (OPTIONS_additive)
					{
						log_info("Makes additive animation.");
						egal::animation::offline::AdditiveAnimationBuilder additive_builder;
						RawAnimation raw_additive;
						if (!additive_builder(_raw_animation, &raw_additive))
						{
							log_error("Failed to make additive animation.");
							return false;
						}
						// Copy animation.
						raw_animation = raw_additive;
					}
					else
					{
						raw_animation = _raw_animation;
					}

					// Optimizes animation if option is enabled.
					if (OPTIONS_optimize)
					{
						log_info("Optimizing animation.");
						egal::animation::offline::AnimationOptimizer optimizer;
						optimizer.rotation_tolerance = OPTIONS_rotation;
						optimizer.translation_tolerance = OPTIONS_translation;
						optimizer.scale_tolerance = OPTIONS_scale;
						optimizer.hierarchical_tolerance = OPTIONS_hierarchical;
						egal::animation::offline::RawAnimation raw_optimized_animation;
						if (!optimizer(raw_animation, _skeleton, &raw_optimized_animation))
						{
							log_error("Failed to optimize animation.");
							return false;
						}

						// Displays optimization statistics.
						DisplaysOptimizationstatistics(raw_animation, raw_optimized_animation);

						// Brings data back to the raw animation.
						raw_animation = raw_optimized_animation;
					}

					// Builds runtime animation.
					egal::animation::Animation* animation = NULL;
					if (!OPTIONS_raw)
					{
						log_info("Builds runtime animation.");
						egal::animation::offline::AnimationBuilder builder;
						animation = builder(raw_animation);
						if (!animation)
						{
							log_error("Failed to build runtime animation.");
							return false;
						}
					}

					{
						// Prepares output stream. File is a RAII so it will close automatically at
						// the end of this scope.
						// Once the file is opened, nothing should fail as it would leave an invalid
						// file on the disk.

						// Builds output filename.
						String filename =
							BuildFilename(OPTIONS_animation, _raw_animation.name.c_str());

						log_info("Opens output file: %s", filename);
						egal::io::File file(filename.c_str(), "wb");
						if (!file.opened())
						{
							log_error("Failed to open output file: %s", filename);
							egal::memory::default_allocator()->Delete(animation);
							return false;
						}

						// Initializes output endianness from options.
						egal::Endianness endianness = egal::GetNativeEndianness();
						if (std::strcmp(OPTIONS_endian, "little"))
						{
							endianness = egal::kLittleEndian;
						}
						else if (std::strcmp(OPTIONS_endian, "big"))
						{
							endianness = egal::kBigEndian;
						}
						log_info(endianness == egal::kLittleEndian ? "Little Endian output binary format selected." : "Big Endian output binary format selected.");

						// Initializes output archive.
						egal::io::OArchive archive(&file, endianness);

						// Fills output archive with the animation.
						if (OPTIONS_raw)
						{
							log_info("Outputs RawAnimation to binary archive."); archive << raw_animation;
						}
						else
						{
							log_info("Outputs Animation to binary archive."); archive << *animation;
						}
					}

					log_info("Animation binary archive successfully outputted." << std::endl;

					// Delete local objects.
					egal::memory::default_allocator()->Delete(animation);

					return true;
				}
			}  // namespace

		}  // namespace offline
	}  // namespace animation
}  // namespace egal
