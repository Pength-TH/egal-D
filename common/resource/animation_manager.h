#ifndef _animation_manager_h_
#define _animation_manager_h_
#pragma once

#include "runtime/Resource/resource_define.h"
#include "runtime/Resource/resource_public.h"
#include "runtime/Resource/resource_manager.h"

namespace egal
{
	namespace FS
	{
		class FileSystem;
		struct IFile;
	}

	class Model;
	struct Pose;
	struct Quat;
	struct Vec3;

	class AnimationManager : public ResourceManagerBase
	{
	public:
		explicit AnimationManager(IAllocator& allocator)
			: ResourceManagerBase(allocator)
			, m_allocator(allocator)
		{}
		~AnimationManager() {}
		IAllocator& getAllocator() { return m_allocator; }

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
	};


	struct BoneMask
	{
		BoneMask(IAllocator& allocator) : bones(allocator) {}
		e_uint32 name;
		HashMap<e_uint32, e_uint8> bones;
	};


	class Animation : public Resource
	{
	public:
		static const e_uint32 HEADER_MAGIC = 0x5f4c4146; // '_LAF'

	public:
		struct Header
		{
			e_uint32 magic;
			e_uint32 version;
			e_uint32 fps;
		};

	public:
		Animation(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);

		e_int32 getRootMotionBoneIdx() const { return m_root_motion_bone_idx; }
		RigidTransform getBoneTransform(e_float time, e_int32 bone_idx) const;
		e_void getRelativePose(e_float time, Pose& pose, Model& model, BoneMask* mask) const;
		e_void getRelativePose(e_float time, Pose& pose, Model& model, e_float weight, BoneMask* mask) const;
		e_int32 getFrameCount() const { return m_frame_count; }
		e_float getLength() const { return m_frame_count / (e_float)m_fps; }
		e_int32 getFPS() const { return m_fps; }
		e_int32 getBoneCount() const { return m_bones.size(); }
		e_int32 getBoneIndex(e_uint32 name) const;

	private:
		IAllocator& getAllocator() const;

		e_void unload() override;
		e_bool load(FS::IFile& file) override;

	private:
		e_int32	m_frame_count;
		struct Bone
		{
			e_uint32 name;
			e_int32 pos_count;
			const e_uint16* pos_times;
			const Vec3* pos;
			e_int32 rot_count;
			const e_uint16* rot_times;
			const Quat* rot;
		};
		Array<Bone> m_bones;
		Array<e_uint8> m_mem;
		e_int32 m_fps;
		e_int32 m_root_motion_bone_idx;
	};
}

#endif