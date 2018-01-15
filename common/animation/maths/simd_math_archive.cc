
#include "common/animation/maths/simd_math_archive.h"
#include <cassert>
#include "common/animation/io/archive.h"

namespace egal
{
	namespace io
	{
		template <>
		void Save(OArchive& _archive, const math::SimdFloat4* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(_values), 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SimdFloat4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(_values), 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SimdInt4* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const int*>(_values), 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SimdInt4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<int*>(_values), 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Float4x4* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(_values), 16 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Float4x4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(_values), 16 * _count);
		}
	}  // namespace io
}  // namespace egal
