#include "common/animation/offline/tools/convert2anim.h"

#include <cstdlib>
#include <cstring>

#include "common/animation/offline/additive_animation_builder.h"
#include "common/animation/offline/animation_builder.h"
#include "common/animation/offline/animation_optimizer.h"
#include "common/animation/offline/raw_animation.h"
#include "common/animation/offline/raw_skeleton.h"
#include "common/animation/offline/skeleton_builder.h"

#include "common/animation/animation.h"
#include "common/animation/skeleton.h"
#include "common/egal-d.h"

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

					log_info("Optimization stage results:" << std::endl;
					log_info(" - Translations key frames optimization: "
						<< translation_ratio << "%" << std::endl;
					log_info(" - Rotations key frames optimization: " << rotation_ratio
						<< "%" << std::endl;
					log_info(" - Scaling key frames optimization: " << scale_ratio
						<< "%" << std::endl;
				}

				egal::animation::Skeleton* ImportSkeleton()
				{
					// Reads the skeleton from the binary egal stream.
					egal::animation::Skeleton* skeleton = NULL;
					{
						log_info("Opens input skeleton egal binary file: "
							<< OPTIONS_skeleton << std::endl;
						egal::io::File file(OPTIONS_skeleton, "rb");
						if (!file.opened())
						{
							log_error("Failed to open input skeleton egal binary file: "
								<< OPTIONS_skeleton << std::endl;
							return NULL;
						}
						egal::io::IArchive archive(&file);

						// File could contain a RawSkeleton or a Skeleton.
						if (archive.TestTag<egal::animation::offline::RawSkeleton>())
						{
							log_info("Reading RawSkeleton from file." << std::endl;

							// Reading the skeleton cannot file.
							egal::animation::offline::RawSkeleton raw_skeleton;
							archive >> raw_skeleton;

							// Builds runtime skeleton.
							log_info("Builds runtime skeleton." << std::endl;
							egal::animation::offline::SkeletonBuilder builder;
							skeleton = builder(raw_skeleton);
							if (!skeleton)
							{
								log_error("Failed to build runtime skeleton." << std::endl;
								return NULL;
							}
						}
						else if (archive.TestTag<egal::animation::Skeleton>())
						{
							// Reads input archive to the runtime skeleton.
							// This operation cannot fail.
							skeleton =
								egal::memory::default_allocator()->New<egal::animation::Skeleton>();
							archive >> *skeleton;
						}
						else
						{
							log_error("Failed to read input skeleton from binary file: "
								<< OPTIONS_skeleton << std::endl;
							return NULL;
						}
					}
					return skeleton;
				}

				bool OutputSingleAnimation()
				{
					return strchr(OPTIONS_animation.value(), '*') == NULL;
				}

				egal::String::Std BuildFilename(const char* _filename, const char* _animation)
				{
					egal::String::Std output(_filename);
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
						log_info("Makes additive animation." << std::endl;
						egal::animation::offline::AdditiveAnimationBuilder additive_builder;
						RawAnimation raw_additive;
						if (!additive_builder(_raw_animation, &raw_additive))
						{
							log_error("Failed to make additive animation." << std::endl;
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
						log_info("Optimizing animation." << std::endl;
						egal::animation::offline::AnimationOptimizer optimizer;
						optimizer.rotation_tolerance = OPTIONS_rotation;
						optimizer.translation_tolerance = OPTIONS_translation;
						optimizer.scale_tolerance = OPTIONS_scale;
						optimizer.hierarchical_tolerance = OPTIONS_hierarchical;
						egal::animation::offline::RawAnimation raw_optimized_animation;
						if (!optimizer(raw_animation, _skeleton, &raw_optimized_animation))
						{
							log_error("Failed to optimize animation." << std::endl;
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
						log_info("Builds runtime animation." << std::endl;
						egal::animation::offline::AnimationBuilder builder;
						animation = builder(raw_animation);
						if (!animation)
						{
							log_error("Failed to build runtime animation." << std::endl;
							return false;
						}
					}

					{
						// Prepares output stream. File is a RAII so it will close automatically at
						// the end of this scope.
						// Once the file is opened, nothing should fail as it would leave an invalid
						// file on the disk.

						// Builds output filename.
						egal::String::Std filename =
							BuildFilename(OPTIONS_animation, _raw_animation.name.c_str());

						log_info("Opens output file: " << filename << std::endl;
						egal::io::File file(filename.c_str(), "wb");
						if (!file.opened())
						{
							log_error("Failed to open output file: " << filename
								<< std::endl;
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
						egal::log::Log() << (endianness == egal::kLittleEndian ? "Little" : "Big")
							<< " Endian output binary format selected." << std::endl;

						// Initializes output archive.
						egal::io::OArchive archive(&file, endianness);

						// Fills output archive with the animation.
						if (OPTIONS_raw)
						{
							log_info("Outputs RawAnimation to binary archive." << std::endl;
							archive << raw_animation;
						}
						else
						{
							log_info("Outputs Animation to binary archive." << std::endl;
							archive << *animation;
						}
					}

					log_info("Animation binary archive successfully outputted."
						<< std::endl;

					// Delete local objects.
					egal::memory::default_allocator()->Delete(animation);

					return true;
				}
			}  // namespace

			int AnimationConverter::operator()(int _argc, const char** _argv)
			{
				// Parses arguments.
				egal::options::ParseResult parse_result = egal::options::ParseCommandLine(
					_argc, _argv, "1.1",
					"Imports a animation from a file and converts it to egal binary raw or "
					"runtime animation format");
				if (parse_result != egal::options::kSuccess)
				{
					return parse_result == egal::options::kExitSuccess ? EXIT_SUCCESS
						: EXIT_FAILURE;
				}

				// Initializes log level from options.
				egal::log::Level log_level = egal::log::GetLevel();
				if (std::strcmp(OPTIONS_log_level, "silent") == 0)
				{
					log_level = egal::log::Silent;
				}
				else if (std::strcmp(OPTIONS_log_level, "standard") == 0)
				{
					log_level = egal::log::Standard;
				}
				else if (std::strcmp(OPTIONS_log_level, "verbose") == 0)
				{
					log_level = egal::log::Verbose;
				}
				egal::log::SetLevel(log_level);

				// Ensures file to import actually exist.
				if (!egal::io::File::Exist(OPTIONS_file))
				{
					egal::log::Err() << "File \"" << OPTIONS_file << "\" doesn't exist."
						<< std::endl;
					return EXIT_FAILURE;
				}

				// Import skeleton instance.
				egal::animation::Skeleton* skeleton = ImportSkeleton();
				if (!skeleton)
				{
					return EXIT_FAILURE;
				}

				// Imports animation from the document.
				egal::log::Log() << "Importing file \"" << OPTIONS_file << "\"" << std::endl;

				bool success = false;
				Animations animations;
				if (Import(OPTIONS_file, *skeleton, OPTIONS_sampling_rate, &animations))
				{
					success = true;

					if (OutputSingleAnimation() && animations.size() > 1)
					{
						egal::log::Log() << animations.size()
							<< " animations found. Only the first one ("
							<< animations[0].name << ") will be exported."
							<< std::endl;

						// Remove all unhandled animations.
						animations.resize(1);
					}

					// Iterate all imported animation, build and output them.
					for (size_t i = 0; i < animations.size(); ++i)
					{
						success &= Export(animations[i], *skeleton);
					}
				}
				else
				{
					log_error("Failed to import file \"" << OPTIONS_file << "\""
						<< std::endl;
				}

				egal::memory::default_allocator()->Delete(skeleton);

				return success ? EXIT_SUCCESS : EXIT_FAILURE;
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
