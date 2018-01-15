
#ifndef MATHS_SOA_MATH_ARCHIVE_H_
#define MATHS_SOA_MATH_ARCHIVE_H_

#include "common/animation/io/archive_traits.h"
#include "common/animation/platform.h"

namespace egal
{
	namespace math
	{
		struct SoaFloat2;
		struct SoaFloat3;
		struct SoaFloat4;
		struct SoaQuaternion;
		struct SoaFloat4x4;
		struct SoaTransform;
	}  // namespace math
	namespace io
	{
		IO_TYPE_NOT_VERSIONABLE(math::SoaFloat2)
			template <>
		void Save(OArchive& _archive, const math::SoaFloat2* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaFloat2* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SoaFloat3)
			template <>
		void Save(OArchive& _archive, const math::SoaFloat3* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaFloat3* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SoaFloat4)
			template <>
		void Save(OArchive& _archive, const math::SoaFloat4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaFloat4* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SoaQuaternion)
			template <>
		void Save(OArchive& _archive, const math::SoaQuaternion* _values,
			size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaQuaternion* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SoaFloat4x4)
			template <>
		void Save(OArchive& _archive, const math::SoaFloat4x4* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaFloat4x4* _values, size_t _count,
			uint32_t _version);

		IO_TYPE_NOT_VERSIONABLE(math::SoaTransform)
			template <>
		void Save(OArchive& _archive, const math::SoaTransform* _values, size_t _count);
		template <>
		void Load(IArchive& _archive, math::SoaTransform* _values, size_t _count,
			uint32_t _version);
	}  // namespace io
}  // namespace egal
#endif  // MATHS_SOA_MATH_ARCHIVE_H_
