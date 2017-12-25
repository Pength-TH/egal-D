#ifndef _controller_manager_h_
#define _controller_manager_h_
#pragma once

#include "runtime/Resource/resource_define.h"
#include "runtime/Resource/resource_public.h"
#include "runtime/Resource/resource_manager.h"

namespace egal
{
	namespace Anim
	{
		class ControllerManager : public ResourceManagerBase
		{
		public:
			explicit ControllerManager(IAllocator& allocator)
				: ResourceManagerBase(allocator)
				, m_allocator(allocator)
			{}
			~ControllerManager() {}
			IAllocator& getAllocator() const { return m_allocator; }

		protected:
			Resource* createResource(const ArchivePath& path) override;
			e_void destroyResource(Resource& resource) override;

		private:
			IAllocator& m_allocator;
		};


		class ControllerResource : public Resource
		{
		public:
			enum class Version : e_int32
			{
				ANIMATION_SETS,
				MAX_ROOT_ROTATION_SPEED,
				INPUT_REFACTOR,
				ENTER_EXIT_EVENTS,
				ANIMATION_SPEED_MULTIPLIER,
				MASKS,
				END_GUARD,

				LAST
			};

		public:
			ControllerResource(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
			~ControllerResource();

			e_void create() { onCreated(State::READY); }
			e_void unload() override;
			e_bool load(FS::IFile& file) override;
			ComponentInstance* createInstance(IAllocator& allocator) const;
			e_void serialize(OutputBlob& blob);
			e_bool deserialize(InputBlob& blob, e_int32& version);
			IAllocator& getAllocator() const { return m_allocator; }
			e_void addAnimation(e_int32 set, e_uint32 hash, Animation* animation);

			struct AnimSetEntry
			{
				e_int32 set;
				e_uint32 hash;
				Animation* animation;
			};

			Array<AnimSetEntry> m_animation_set;
			Array<StaticString<32>> m_sets_names;
			Array<BoneMask> m_masks;
			InputDecl m_input_decl;
			Component* m_root;

		private:
			e_void clearAnimationSets();

			IAllocator& m_allocator;
		};
	}
}

#endif
