#include "common/animation/offline/raw_skeleton.h"

#include "common/animation/io/archive.h"
#include "common/animation/maths/math_archive.h"

namespace egal
{
	namespace io
	{

		template <>
		void Save(OArchive& _archive, const animation::offline::RawSkeleton* _skeletons,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawSkeleton& skeleton = _skeletons[i];
				_archive << skeleton.roots;
			}
		}

		template <>
		void Load(IArchive& _archive, animation::offline::RawSkeleton* _skeletons,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawSkeleton& skeleton = _skeletons[i];
				_archive >> skeleton.roots;
			}
		}

		// RawSkeleton::Joint' version can be declared locally as it will be saved from
		// this cpp file only.
		IO_TYPE_VERSION(1, animation::offline::RawSkeleton::Joint)

			template <>
		void Save(OArchive& _archive,
			const animation::offline::RawSkeleton::Joint* _joints,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawSkeleton::Joint& joint = _joints[i];
				_archive << joint.name;
				_archive << joint.transform;
				_archive << joint.children;
			}
		}

		template <>
		void Load(IArchive& _archive, animation::offline::RawSkeleton::Joint* _joints,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawSkeleton::Joint& joint = _joints[i];
				_archive >> joint.name;
				_archive >> joint.transform;
				_archive >> joint.children;
			}
		}
	}  // namespace io
}  // namespace egal
