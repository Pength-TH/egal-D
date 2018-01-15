
#include "common/animation/io/archive.h"
#include <cassert>

namespace egal
{
	namespace io
	{
		// OArchive implementation.
		OArchive::OArchive(Stream* _stream, Endianness _endianness)
			: stream_(_stream), endian_swap_(_endianness != GetNativeEndianness())
		{
			assert(stream_ && stream_->opened() &&
				L"_stream argument must point a valid opened stream.");
			// Save as a single byte as it does not need to be swapped.
			uint8_t endianness = static_cast<uint8_t>(_endianness);
			*this << endianness;
		}

		// IArchive implementation.
		IArchive::IArchive(Stream* _stream) : stream_(_stream), endian_swap_(false)
		{
			assert(stream_ && stream_->opened() &&
				L"_stream argument must point a valid opened stream.");
			// Endianness was saved as a single byte, as it does not need to be swapped.
			uint8_t endianness;
			*this >> endianness;
			endian_swap_ = endianness != GetNativeEndianness();
		}
	}  // namespace io
}  // namespace egal
