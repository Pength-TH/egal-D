
#include "common/animation/maths/math_archive.h"
#include <cassert>
#include "common/animation/io/archive.h"
#include "common/animation/maths/soa_float.h"
#include "common/animation/maths/soa_float4x4.h"
#include "common/animation/maths/soa_quaternion.h"
#include "common/animation/maths/soa_transform.h"

namespace egal
{
	namespace io
	{
		template <>
		void Save(OArchive& _archive, const math::SoaFloat2* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
				2 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaFloat2* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 2 * 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SoaFloat3* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
				3 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaFloat3* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 3 * 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SoaFloat4* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
				4 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaFloat4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 4 * 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SoaQuaternion* _values,
			size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->x),
				4 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaQuaternion* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->x), 4 * 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SoaFloat4x4* _values, size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->cols[0].x),
				4 * 4 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaFloat4x4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->cols[0].x),
				4 * 4 * 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::SoaTransform* _values,
			size_t _count)
		{
			_archive << MakeArray(reinterpret_cast<const float*>(&_values->translation.x),
				10 * 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::SoaTransform* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(reinterpret_cast<float*>(&_values->translation.x),
				10 * 4 * _count);
		}
	}  // namespace io
}  // namespace egal
