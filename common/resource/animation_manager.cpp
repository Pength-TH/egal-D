#include "runtime/Resource/animation_manager.h"

namespace egal
{
	enum class Version
	{
		FIRST = 0,
		COMPRESSION = 1,
		ROOT_MOTION,

		LAST
	};


	Resource* AnimationManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Animation)(path, *this, m_allocator);
	}


	e_void AnimationManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Animation*>(&resource));
	}


	Animation::Animation(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_frame_count(0)
		, m_fps(30)
		, m_mem(allocator)
		, m_bones(allocator)
		, m_root_motion_bone_idx(-1)
	{
	}


	e_void Animation::getRelativePose(e_float time, Pose& pose, Model& model, e_float weight, BoneMask* mask) const
	{
		//PROFILE_FUNCTION();
		ASSERT(!pose.is_absolute);

		if (!model.isReady()) return;

		e_int32 frame = (e_int32)(time * m_fps);
		e_float rcp_fps = 1.0f / m_fps;
		frame = Math::clamp(frame, 0, m_frame_count);
		Vec3* pos = pose.positions;
		Quat* rot = pose.rotations;

		if (frame < m_frame_count)
		{
			for (Bone& bone : m_bones)
			{
				Model::BoneMap::iterator iter = model.getBoneIndex(bone.name);
				if (!iter.isValid()) continue;
				if (mask && mask->bones.find(bone.name) == mask->bones.end()) continue;

				e_int32 idx = 1;
				for (e_int32 c = bone.pos_count; idx < c; ++idx)
				{
					if (bone.pos_times[idx] > frame) break;
				}

				e_float t = e_float(time - bone.pos_times[idx - 1] * rcp_fps) /
					((bone.pos_times[idx] - bone.pos_times[idx - 1]) * rcp_fps);

				e_int32 model_bone_index = iter.value();
				Vec3 anim_pos;
				lerp(bone.pos[idx - 1], bone.pos[idx], &anim_pos, t);
				lerp(pos[model_bone_index], anim_pos, &pos[model_bone_index], weight);

				idx = 1;
				for (e_int32 c = bone.rot_count; idx < c; ++idx)
				{
					if (bone.rot_times[idx] > frame) break;
				}

				t = e_float(time - bone.rot_times[idx - 1] * rcp_fps) /
					((bone.rot_times[idx] - bone.rot_times[idx - 1]) * rcp_fps);

				Quat anim_rot;
				nlerp(bone.rot[idx - 1], bone.rot[idx], &anim_rot, t);
				nlerp(rot[model_bone_index], anim_rot, &rot[model_bone_index], weight);
			}
		}
		else
		{
			for (Bone& bone : m_bones)
			{
				Model::BoneMap::iterator iter = model.getBoneIndex(bone.name);
				if (!iter.isValid()) continue;
				if (mask && mask->bones.find(bone.name) == mask->bones.end()) continue;

				e_int32 model_bone_index = iter.value();
				lerp(pos[model_bone_index], bone.pos[bone.pos_count - 1], &pos[model_bone_index], weight);
				nlerp(rot[model_bone_index], bone.rot[bone.rot_count - 1], &rot[model_bone_index], weight);
			}
		}
	}


	RigidTransform Animation::getBoneTransform(e_float time, e_int32 bone_idx) const
	{
		RigidTransform ret;
		e_int32 frame = (e_int32)(time * m_fps);
		e_float rcp_fps = 1.0f / m_fps;
		frame = Math::clamp(frame, 0, m_frame_count);

		const Bone& bone = m_bones[bone_idx];
		if (frame < m_frame_count)
		{
			e_int32 idx = 1;
			for (e_int32 c = bone.pos_count; idx < c; ++idx)
			{
				if (bone.pos_times[idx] > frame) break;
			}

			e_float t = e_float(time - bone.pos_times[idx - 1] * rcp_fps) /
				((bone.pos_times[idx] - bone.pos_times[idx - 1]) * rcp_fps);
			lerp(bone.pos[idx - 1], bone.pos[idx], &ret.pos, t);

			idx = 1;
			for (e_int32 c = bone.rot_count; idx < c; ++idx)
			{
				if (bone.rot_times[idx] > frame) break;
			}

			t = e_float(time - bone.rot_times[idx - 1] * rcp_fps) /
				((bone.rot_times[idx] - bone.rot_times[idx - 1]) * rcp_fps);
			nlerp(bone.rot[idx - 1], bone.rot[idx], &ret.rot, t);
		}
		else
		{
			ret.pos = bone.pos[bone.pos_count - 1];
			ret.rot = bone.rot[bone.rot_count - 1];
		}
		return ret;
	}


	e_int32 Animation::getBoneIndex(e_uint32 name) const
	{
		for (e_int32 i = 0, c = m_bones.size(); i < c; ++i)
		{
			if (m_bones[i].name == name) return i;
		}
		return -1;
	}


	e_void Animation::getRelativePose(e_float time, Pose& pose, Model& model, BoneMask* mask) const
	{
		//PROFILE_FUNCTION();
		ASSERT(!pose.is_absolute);

		if (!model.isReady()) return;

		e_int32 frame = (e_int32)(time * m_fps);
		e_float rcp_fps = 1.0f / m_fps;
		frame = Math::clamp(frame, 0, m_frame_count);
		Vec3* pos = pose.positions;
		Quat* rot = pose.rotations;

		if (frame < m_frame_count)
		{
			for (Bone& bone : m_bones)
			{
				Model::BoneMap::iterator iter = model.getBoneIndex(bone.name);
				if (!iter.isValid()) continue;
				if (mask && mask->bones.find(bone.name) == mask->bones.end()) continue;

				e_int32 idx = 1;
				for (e_int32 c = bone.pos_count; idx < c; ++idx)
				{
					if (bone.pos_times[idx] > frame) break;
				}

				e_float t = e_float(time - bone.pos_times[idx - 1] * rcp_fps) /
					((bone.pos_times[idx] - bone.pos_times[idx - 1]) * rcp_fps);
				e_int32 model_bone_index = iter.value();
				lerp(bone.pos[idx - 1], bone.pos[idx], &pos[model_bone_index], t);

				idx = 1;
				for (e_int32 c = bone.rot_count; idx < c; ++idx)
				{
					if (bone.rot_times[idx] > frame) break;
				}

				t = e_float(time - bone.rot_times[idx - 1] * rcp_fps) /
					((bone.rot_times[idx] - bone.rot_times[idx - 1]) * rcp_fps);
				nlerp(bone.rot[idx - 1], bone.rot[idx], &rot[model_bone_index], t);
			}
		}
		else
		{
			for (Bone& bone : m_bones)
			{
				Model::BoneMap::iterator iter = model.getBoneIndex(bone.name);
				if (!iter.isValid()) continue;
				if (mask && mask->bones.find(bone.name) == mask->bones.end()) continue;

				e_int32 model_bone_index = iter.value();
				pos[model_bone_index] = bone.pos[bone.pos_count - 1];
				rot[model_bone_index] = bone.rot[bone.rot_count - 1];
			}
		}
	}


	e_bool Animation::load(FS::IFile& file)
	{
		m_bones.clear();
		m_mem.clear();
		Header header;
		file.read(&header, sizeof(header));
		if (header.magic != HEADER_MAGIC)
		{
			g_log_error.log("Animation") << getPath() << " is not an animation file";
			return false;
		}
		if (header.version <= (e_int32)Version::COMPRESSION)
		{
			g_log_error.log("Animation") << "Unsupported animation version " << (e_int32)header.version << " ("
				<< getPath() << ")";
			return false;
		}
		if (header.version > (e_int32)Version::ROOT_MOTION)
		{
			file.read(&m_root_motion_bone_idx, sizeof(m_root_motion_bone_idx));
		}
		else
		{
			m_root_motion_bone_idx = -1;
		}
		m_fps = header.fps;
		file.read(&m_frame_count, sizeof(m_frame_count));
		e_int32 bone_count;
		file.read(&bone_count, sizeof(bone_count));
		m_bones.resize(bone_count);
		if (bone_count == 0) return true;

		e_int32 size = e_int32(file.size() - file.pos());
		m_mem.resize(size);
		file.read(&m_mem[0], size);
		InputBlob blob(&m_mem[0], size);
		for (e_int32 i = 0; i < m_bones.size(); ++i)
		{
			m_bones[i].name = blob.read<e_uint32>();

			m_bones[i].pos_count = blob.read<e_int32>();
			m_bones[i].pos_times = (const e_uint16*)blob.skip(m_bones[i].pos_count * sizeof(e_uint16));
			m_bones[i].pos = (const Vec3*)blob.skip(m_bones[i].pos_count * sizeof(Vec3));

			m_bones[i].rot_count = blob.read<e_int32>();
			m_bones[i].rot_times = (const e_uint16*)blob.skip(m_bones[i].rot_count * sizeof(e_uint16));
			m_bones[i].rot = (const Quat*)blob.skip(m_bones[i].rot_count * sizeof(Quat));
		}

		m_size = file.size();
		return true;
	}


	IAllocator& Animation::getAllocator() const
	{
		return static_cast<AnimationManager&>(m_resource_manager).getAllocator();
	}


	e_void Animation::unload()
	{
		m_bones.clear();
		m_mem.clear();
		m_frame_count = 0;
	}
}
