
#ifndef MATHS_SIMD_MATH_ARCHIVE_H_
#define MATHS_SIMD_MATH_ARCHIVE_H_
#include "common/animation/platform.h"
#include "common/animation/io/archive_traits.h"
#include "common/animation/maths/simd_math.h"

namespace egal
{
	namespace io
	{
		IO_TYPE_NOT_VERSIONABLE(math::SimdFloat4)
			template <>
		void Save(OArchive& _archive, const math::SimdFloat4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SimdFloat4* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SimdInt4)
			template <>
		void Save(OArchive& _archive, const math::SimdInt4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SimdInt4* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Float4x4)
			template <>
		void Save(OArchive& _archive, const math::Float4x4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Float4x4* _values, size_t _count,
			uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // MATHS_SIMD_MATH_ARCHIVE_H_
