#include "common/animation/offline/tools/convert2skel.h"

#include <cstdlib>
#include <cstring>

#include "common/animation/offline/raw_skeleton.h"
#include "common/animation/offline/skeleton_builder.h"
#include "common/animation/skeleton.h"
#include "common/egal-d.h"

static bool ValidateEndianness(const egal::options::Option& _option,
	int /*_argc*/)
{
	const egal::options::StringOption& option =
		static_cast<const egal::options::StringOption&>(_option);
	bool valid = std::strcmp(option.value(), "native") == 0 ||
		std::strcmp(option.value(), "little") == 0 ||
		std::strcmp(option.value(), "big") == 0;
	if (!valid)
	{
		egal::log::Err() << "Invalid endianess option." << std::endl;
	}
	return valid;
}

static bool ValidateLogLevel(const egal::options::Option& _option,
	int /*_argc*/)
{
	const egal::options::StringOption& option =
		static_cast<const egal::options::StringOption&>(_option);
	bool valid = std::strcmp(option.value(), "verbose") == 0 ||
		std::strcmp(option.value(), "standard") == 0 ||
		std::strcmp(option.value(), "silent") == 0;
	if (!valid)
	{
		egal::log::Err() << "Invalid log level option." << std::endl;
	}
	return valid;
}

namespace egal
{
	namespace animation
	{
		namespace offline
		{
			int SkeletonConverter::operator()(int _argc, const char** _argv)
			{
				// Parses arguments.
				egal::options::ParseResult parse_result = egal::options::ParseCommandLine(
					_argc, _argv, "1.1",
					"Imports a skeleton from a file and converts it to egal binary raw or "
					"runtime skeleton format");
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

				// Imports skeleton from the file.
				log_info("Importing file %s.", OPTIONS_file);

				if (!egal::io::File::Exist(OPTIONS_file))
				{
					egal::log::Err() << "File \"" << OPTIONS_file << "\" doesn't exist."
						<< std::endl;
					return EXIT_FAILURE;
				}

				egal::animation::offline::RawSkeleton raw_skeleton;
				if (!Import(OPTIONS_file, &raw_skeleton))
				{
					egal::log::Err() << "Failed to import file \"" << OPTIONS_file << "\""
						<< std::endl;
					return EXIT_FAILURE;
				}

				// Needs to be done before opening the output file, so that if it fails then
				// there's no invalid file outputted.
				egal::animation::Skeleton* skeleton = NULL;
				if (!OPTIONS_raw)
				{
					// Builds runtime skeleton.
					egal::log::Log() << "Builds runtime skeleton." << std::endl;
					egal::animation::offline::SkeletonBuilder builder;
					skeleton = builder(raw_skeleton);
					if (!skeleton)
					{
						egal::log::Err() << "Failed to build runtime skeleton." << std::endl;
						return EXIT_FAILURE;
					}
				}

				// Prepares output stream. File is a RAII so it will close automatically at
				// the end of this scope.
				// Once the file is opened, nothing should fail as it would leave an invalid
				// file on the disk.
				{
					egal::log::Log() << "Opens output file: " << OPTIONS_skeleton << std::endl;
					egal::io::File file(OPTIONS_skeleton, "wb");
					if (!file.opened())
					{
						egal::log::Err() << "Failed to open output file: " << OPTIONS_skeleton
							<< std::endl;
						egal::memory::default_allocator()->Delete(skeleton);
						return EXIT_FAILURE;
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

					// Fills output archive with the skeleton.
					if (OPTIONS_raw)
					{
						egal::log::Log() << "Outputs RawSkeleton to binary archive." << std::endl;
						archive << raw_skeleton;
					}
					else
					{
						egal::log::Log() << "Outputs Skeleton to binary archive." << std::endl;
						archive << *skeleton;
					}
					egal::log::Log() << "Skeleton binary archive successfully outputed."
						<< std::endl;
				}

				// Delete local objects.
				egal::memory::default_allocator()->Delete(skeleton);

				return EXIT_SUCCESS;
			}
		}  // namespace offline
	}  // namespace animation
}  // namespace egal
