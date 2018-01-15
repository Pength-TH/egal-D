#include "common/animation/offline/raw_animation.h"

#include "common/animation/io/archive.h"
#include "common/animation/maths/math_archive.h"
#include "common/egal-d.h"

namespace egal
{
	namespace io
	{

		template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation* _animations, size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawAnimation& animation = _animations[i];
				_archive << animation.duration;
				_archive << animation.tracks;
				_archive << animation.name;
			}
		}

		template <>
		void Load(IArchive& _archive, animation::offline::RawAnimation* _animations,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawAnimation& animation = _animations[i];
				_archive >> animation.duration;
				_archive >> animation.tracks;
				if (_version > 1)
				{
					_archive >> animation.name;
				}
			}
		}

		// RawAnimation::*Keys' version can be declared locally as it will be saved from
		// this cpp file only.

		IO_TYPE_VERSION(1, animation::offline::RawAnimation::JointTrack)

			template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation::JointTrack* _tracks,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawAnimation::JointTrack& track = _tracks[i];
				_archive << track.translations;
				_archive << track.rotations;
				_archive << track.scales;
			}
		}

		template <>
		void Load(IArchive& _archive,
			animation::offline::RawAnimation::JointTrack* _tracks, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawAnimation::JointTrack& track = _tracks[i];
				_archive >> track.translations;
				_archive >> track.rotations;
				_archive >> track.scales;
			}
		}

		IO_TYPE_VERSION(1, animation::offline::RawAnimation::TranslationKey)

			template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation::TranslationKey* _keys,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawAnimation::TranslationKey& key = _keys[i];
				_archive << key.time;
				_archive << key.value;
			}
		}

		template <>
		void Load(IArchive& _archive,
			animation::offline::RawAnimation::TranslationKey* _keys,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawAnimation::TranslationKey& key = _keys[i];
				_archive >> key.time;
				_archive >> key.value;
			}
		}

		IO_TYPE_VERSION(1, animation::offline::RawAnimation::RotationKey)

			template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation::RotationKey* _keys,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawAnimation::RotationKey& key = _keys[i];
				_archive << key.time;
				_archive << key.value;
			}
		}

		template <>
		void Load(IArchive& _archive,
			animation::offline::RawAnimation::RotationKey* _keys, size_t _count,
			uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawAnimation::RotationKey& key = _keys[i];
				_archive >> key.time;
				_archive >> key.value;
			}
		}

		IO_TYPE_VERSION(1, animation::offline::RawAnimation::ScaleKey)

			template <>
		void Save(OArchive& _archive,
			const animation::offline::RawAnimation::ScaleKey* _keys,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				const animation::offline::RawAnimation::ScaleKey& key = _keys[i];
				_archive << key.time;
				_archive << key.value;
			}
		}

		template <>
		void Load(IArchive& _archive, animation::offline::RawAnimation::ScaleKey* _keys,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				animation::offline::RawAnimation::ScaleKey& key = _keys[i];
				_archive >> key.time;
				_archive >> key.value;
			}
		}
	}  // namespace io
}  // namespace egal
