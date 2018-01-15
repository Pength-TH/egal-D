#include "common/animation/maths/math_archive.h"
#include "common/animation/io/archive.h"
#include "common/animation/maths/box.h"
#include "common/animation/maths/quaternion.h"
#include "common/animation/maths/rect.h"
#include "common/animation/maths/transform.h"
#include "common/animation/maths/vec_float.h"
#include <cassert>

namespace egal
{
	namespace io
	{
		template <>
		void Save(OArchive& _archive, const math::Float2* _values, size_t _count)
		{
			_archive << MakeArray(&_values->x, 2 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Float2* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->x, 2 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Float3* _values, size_t _count)
		{
			_archive << MakeArray(&_values->x, 3 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Float3* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->x, 3 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Float4* _values, size_t _count)
		{
			_archive << MakeArray(&_values->x, 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Float4* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->x, 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Quaternion* _values, size_t _count)
		{
			_archive << MakeArray(&_values->x, 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Quaternion* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->x, 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Transform* _values, size_t _count)
		{
			_archive << MakeArray(&_values->translation.x, 10 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Transform* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->translation.x, 10 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::Box* _values, size_t _count)
		{
			_archive << MakeArray(&_values->min.x, 6 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::Box* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->min.x, 6 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::RectFloat* _values, size_t _count)
		{
			_archive << MakeArray(&_values->left, 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::RectFloat* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->left, 4 * _count);
		}

		template <>
		void Save(OArchive& _archive, const math::RectInt* _values, size_t _count)
		{
			_archive << MakeArray(&_values->left, 4 * _count);
		}
		template <>
		void Load(IArchive& _archive, math::RectInt* _values, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			_archive >> MakeArray(&_values->left, 4 * _count);
		}
	}  // namespace io
}  // namespace egal
