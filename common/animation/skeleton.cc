
#include "common/animation/skeleton.h"

#include "common/animation/io/archive.h"
#include "common/animation/maths/math_ex.h"
#include "common/animation/maths/soa_math_archive.h"
#include "common/animation/maths/soa_transform.h"

#include "common/allocator/egal_allocator.h"
#include "common/utils/logger.h"

#include <cstring>

namespace egal
{
	namespace io
	{
		// JointProperties' version can be declared locally as it will be saved from
		// this
		// cpp file only.
		IO_TYPE_VERSION(1, animation::Skeleton::JointProperties)

			// Specializes Skeleton::JointProperties. This structure's bitset isn't written
			// as-is because of endianness issues.
			template <>
		void Save(OArchive& _archive,
			const animation::Skeleton::JointProperties* _properties,
			size_t _count)
		{
			for (size_t i = 0; i < _count; ++i)
			{
				uint16_t parent = _properties[i].parent;
				_archive << parent;
				bool is_leaf = _properties[i].is_leaf != 0;
				_archive << is_leaf;
			}
		}

		template <>
		void Load(IArchive& _archive, animation::Skeleton::JointProperties* _properties,
			size_t _count, uint32_t _version)
		{
			(void)_version;
			for (size_t i = 0; i < _count; ++i)
			{
				uint16_t parent;
				_archive >> parent;
				_properties[i].parent = parent;
				bool is_leaf;
				_archive >> is_leaf;
				_properties[i].is_leaf = is_leaf;
			}
		}
	}  // namespace io

	namespace animation
	{

		Skeleton::Skeleton()
		{}

		Skeleton::~Skeleton()
		{
			Deallocate();
		}

		char* Skeleton::Allocate(size_t _chars_size, size_t _num_joints)
		{
			// Distributes buffer memory while ensuring proper alignment (serves larger
			// alignment values first).
			STATIC_ASSERT(
				ALIGN_OF(math::SoaTransform) >= ALIGN_OF(char*) &&
				ALIGN_OF(char*) >= ALIGN_OF(Skeleton::JointProperties) &&
				ALIGN_OF(Skeleton::JointProperties) >= ALIGN_OF(char));

			assert(bind_pose_.Size() == 0 && joint_names_.Size() == 0 &&
				joint_properties_.Size() == 0);

			// Early out if no joint.
			if (_num_joints == 0)
			{
				return NULL;
			}

			// Bind poses have SoA format
			const size_t bind_poses_size =
				(_num_joints + 3) / 4 * sizeof(math::SoaTransform);
			const size_t names_size = _num_joints * sizeof(char*);
			const size_t properties_size =
				_num_joints * sizeof(Skeleton::JointProperties);
			const size_t buffer_size =
				names_size + _chars_size + properties_size + bind_poses_size;

			// Allocates whole buffer.
			char* buffer = reinterpret_cast<char*>(g_allocator->allocate_aligned(buffer_size, ALIGN_OF(math::SoaTransform)));

			// Bind pose first, biggest alignment.
			bind_pose_.begin = reinterpret_cast<math::SoaTransform*>(buffer);
			assert(math::IsAligned(bind_pose_.begin, ALIGN_OF(math::SoaTransform)));
			buffer += bind_poses_size;
			bind_pose_.end = reinterpret_cast<math::SoaTransform*>(buffer);

			// Then names array, second biggest alignment.
			joint_names_.begin = reinterpret_cast<char**>(buffer);
			assert(math::IsAligned(joint_names_.begin, ALIGN_OF(char**)));
			buffer += names_size;
			joint_names_.end = reinterpret_cast<char**>(buffer);

			// Properties, third biggest alignment.
			joint_properties_.begin =
				reinterpret_cast<Skeleton::JointProperties*>(buffer);
			assert(math::IsAligned(joint_properties_.begin,
				ALIGN_OF(Skeleton::JointProperties)));
			buffer += properties_size;
			joint_properties_.end = reinterpret_cast<Skeleton::JointProperties*>(buffer);

			// Remaning buffer will be used to store joint names.
			return buffer;
		}

		void Skeleton::Deallocate()
		{
			g_allocator->deallocate_aligned(bind_pose_.begin);
			bind_pose_.Clear();
			joint_names_.Clear();
			joint_properties_.Clear();
		}

		void Skeleton::Save(egal::io::OArchive& _archive) const
		{
			const int32_t num_joints = this->num_joints();

			// Early out if skeleton's empty.
			_archive << num_joints;
			if (!num_joints)
			{
				return;
			}

			// Stores names. They are all concatenated in the same buffer, starting at
			// joint_names_[0].
			size_t chars_count = 0;
			for (int i = 0; i < num_joints; ++i)
			{
				chars_count += (std::strlen(joint_names_[i]) + 1) * sizeof(char);
			}
			_archive << static_cast<int32_t>(chars_count);
			_archive << egal::io::MakeArray(joint_names_[0], chars_count);

			// Stores joint's properties.
			_archive << egal::io::MakeArray(joint_properties_);

			// Stores bind poses.
			_archive << egal::io::MakeArray(bind_pose_);
		}

		void Skeleton::Load(egal::io::IArchive& _archive, uint32_t _version)
		{
			// Deallocate skeleton in case it was already used before.
			Deallocate();

			if (_version != 1)
			{
				log_error("Unsupported Skeleton version %s.", _version);
				return;
			}

			int32_t num_joints;
			_archive >> num_joints;

			// Early out if skeleton's empty.
			if (!num_joints)
			{
				return;
			}

			// Read names.
			int32_t chars_count;
			_archive >> chars_count;

			// Allocates all skeleton data members.
			char* cursor = Allocate(chars_count, num_joints);

			// Reads name's buffer, they are all contiguous in the same buffer.
			_archive >> egal::io::MakeArray(cursor, chars_count);

			// Fixes up array of pointers. Stops at num_joints - 1, so that it doesn't
			// read memory past the end of the buffer.
			for (int i = 0; i < num_joints - 1; ++i)
			{
				joint_names_[i] = cursor;
				cursor += std::strlen(joint_names_[i]) + 1;
			}
			// num_joints is > 0, as this was tested at the beginning of the function.
			joint_names_[num_joints - 1] = cursor;

			_archive >> egal::io::MakeArray(joint_properties_);
			_archive >> egal::io::MakeArray(bind_pose_);
		}
	}  // namespace animation
}  // namespace egal
