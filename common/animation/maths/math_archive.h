
#ifndef MATHS_MATH_ARCHIVE_H_
#define MATHS_MATH_ARCHIVE_H_
#include "common/animation/platform.h"
#include "common/animation/io/archive_traits.h"

namespace egal
{
	namespace math
	{
		struct Float2;
		struct Float3;
		struct Float4;
		struct Quaternion;
		struct Transform;
		struct Box;
		struct RectFloat;
		struct RectInt;
	}  // namespace math
	namespace io
	{
		IO_TYPE_NOT_VERSIONABLE(math::Float2)
			template <>
		void Save(OArchive& _archive, const math::Float2* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Float2* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Float3)
			template <>
		void Save(OArchive& _archive, const math::Float3* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Float3* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Float4)
			template <>
		void Save(OArchive& _archive, const math::Float4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Float4* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Quaternion)
			template <>
		void Save(OArchive& _archive, const math::Quaternion* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Quaternion* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Transform)
			template <>
		void Save(OArchive& _archive, const math::Transform* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Transform* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::Box)
			template <>
		void Save(OArchive& _archive, const math::Box* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::Box* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::RectFloat)
			template <>
		void Save(OArchive& _archive, const math::RectFloat* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::RectFloat* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::RectInt)
			template <>
		void Save(OArchive& _archive, const math::RectInt* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::RectInt* _values, size_t _count,
			uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // MATHS_MATH_ARCHIVE_H_
