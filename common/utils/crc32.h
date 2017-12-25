#ifndef _crc32_h_
#define _crc32_h_

#include "common/type.h"
namespace egal
{
	e_uint32 crc32(const e_void* data, e_int32 length);
	e_uint32 crc32(const e_char* str);
	e_uint32 continueCrc32(e_uint32 original_crc, const e_char* str);
	e_uint32 continueCrc32(e_uint32 original_crc, const e_void* data, e_int32 length);
}
#endif