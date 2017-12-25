#include "runtime/Resource/controller_manager.h"

namespace egal
{
	static const ResourceType ANIMATION_TYPE("animation");
	namespace Anim
	{
		struct Header
		{
			static const e_uint32 FILE_MAGIC = 0x5f4c4143; // == '_LAC'
			e_uint32 magic = FILE_MAGIC;
			e_int32 version = (e_int32)ControllerResource::Version::LAST;
			e_uint32 reserved[4] = { 0 };
		};


		ControllerResource::ControllerResource(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
			: Resource(path, resource_manager, allocator)
			, m_root(nullptr)
			, m_allocator(allocator)
			, m_animation_set(allocator)
			, m_sets_names(allocator)
			, m_masks(allocator)
		{
		}


		ControllerResource::~ControllerResource()
		{
			ASSERT(isEmpty());
		}


		e_void ControllerResource::clearAnimationSets()
		{
			for (AnimSetEntry& entry : m_animation_set)
			{
				if (!entry.animation) continue;

				removeDependency(*entry.animation);
				entry.animation->getResourceManager().unload(*entry.animation);
			}
			m_animation_set.clear();
		}


		e_void ControllerResource::unload()
		{
			_delete(m_allocator, m_root);
			m_root = nullptr;
			clearAnimationSets();
		}


		e_bool ControllerResource::load(FS::IFile& file)
		{
			InputBlob blob(file.getBuffer(), (e_int32)file.size());
			e_int32 version;
			return deserialize(blob, version);
		}


		e_bool ControllerResource::deserialize(InputBlob& blob, e_int32& version)
		{
			version = -1;
			Header header;
			blob.read(header);
			if (header.magic != Header::FILE_MAGIC)
			{
				g_log_error.log("Animation") << getPath().c_str() << " is not an animation controller file.";
				return false;
			}
			if (header.version > (e_int32)Version::LAST)
			{
				g_log_error.log("Animation") << getPath().c_str() << " has unsupported version.";
				return false;
			}

			version = header.version;
			Component::Type type;
			blob.read(type);
			m_root = createComponent(*this, type, m_allocator);
			m_root->deserialize(blob, nullptr, header.version);
			blob.read(m_input_decl.inputs_count);
			for (e_int32 j = 0; j < m_input_decl.inputs_count; ++j)
			{
				e_int32 i = j;
				if (header.version > (e_int32)Version::INPUT_REFACTOR) blob.read(i);
				auto& input = m_input_decl.inputs[i];
				blob.readString(input.name.data, lengthOf(input.name.data));
				blob.read(input.type);
				blob.read(input.offset);
			}

			blob.read(m_input_decl.constants_count);
			for (e_int32 j = 0; j < m_input_decl.constants_count; ++j)
			{
				e_int32 i = j;
				if (header.version > (e_int32)Version::INPUT_REFACTOR) blob.read(i);
				auto& constant = m_input_decl.constants[i];
				blob.readString(constant.name.data, lengthOf(constant.name.data));
				blob.read(constant.type);
				switch (constant.type)
				{
				case InputDecl::e_bool: blob.read(constant.b_value); break;
				case InputDecl::e_int32: blob.read(constant.i_value); break;
				case InputDecl::e_float: blob.read(constant.f_value); break;
				default: ASSERT(false); return false;
				}
			}

			clearAnimationSets();
			auto* manager = m_resource_manager.getOwner().get(ANIMATION_TYPE);

			e_int32 count = blob.read<e_int32>();
			m_animation_set.reserve(count);
			for (e_int32 i = 0; i < count; ++i)
			{
				e_int32 set = 0;
				if (header.version > (e_int32)Version::ANIMATION_SETS) set = blob.read<e_int32>();
				e_uint32 key = blob.read<e_uint32>();
				e_char path[MAX_PATH_LENGTH];
				blob.readString(path, lengthOf(path));
				Animation* anim = path[0] ? (Animation*)manager->load(Path(path)) : nullptr;
				addAnimation(set, key, anim);
			}
			if (header.version > (e_int32)Version::ANIMATION_SETS)
			{
				count = blob.read<e_int32>();
				m_sets_names.resize(count);
				for (e_int32 i = 0; i < count; ++i)
				{
					blob.readString(m_sets_names[i].data, lengthOf(m_sets_names[i].data));
				}
			}
			else
			{
				m_sets_names.emplace("default");
			}
			if (header.version > (e_int32)Version::MASKS)
			{
				m_masks.clear();
				e_int32 masks_count = blob.read<e_int32>();
				for (e_int32 i = 0; i < masks_count; ++i)
				{
					BoneMask& mask = m_masks.emplace(m_allocator);
					blob.read(mask.name);
					e_int32 bone_count = blob.read<e_int32>();
					for (e_int32 j = 0; j < bone_count; ++j)
					{
						e_uint32 key = blob.read<e_uint32>();
						mask.bones.insert(key, 1);
					}
				}
			}
			if (header.version > (e_int32)Version::END_GUARD)
			{
				e_uint32 end_guard = blob.read<e_uint32>();
				if (end_guard != header.magic) return false;
			}
			return true;
		}

		e_void ControllerResource::serialize(OutputBlob& blob)
		{
			Header header;
			blob.write(header);
			blob.write(m_root->type);
			m_root->serialize(blob);
			blob.write(m_input_decl.inputs_count);
			for (e_int32 i = 0; i < lengthOf(m_input_decl.inputs); ++i)
			{
				auto& input = m_input_decl.inputs[i];
				if (input.type == InputDecl::EMPTY) continue;
				blob.write(i);
				blob.writeString(input.name);
				blob.write(input.type);
				blob.write(input.offset);
			}
			blob.write(m_input_decl.constants_count);
			for (e_int32 i = 0; i < lengthOf(m_input_decl.constants); ++i)
			{
				auto& constant = m_input_decl.constants[i];
				if (constant.type == InputDecl::EMPTY) continue;
				blob.write(i);
				blob.writeString(constant.name);
				blob.write(constant.type);
				switch (constant.type)
				{
				case InputDecl::e_bool: blob.write(constant.b_value); break;
				case InputDecl::e_int32: blob.write(constant.i_value); break;
				case InputDecl::e_float: blob.write(constant.f_value); break;
				default: ASSERT(false); return;
				}

			}
			blob.write(m_animation_set.size());
			for (AnimSetEntry& entry : m_animation_set)
			{
				blob.write(entry.set);
				blob.write(entry.hash);
				blob.writeString(entry.animation ? entry.animation->getPath().c_str() : "");
			}
			blob.write(m_sets_names.size());
			for (const StaticString<32>& name : m_sets_names)
			{
				blob.writeString(name);
			}
			blob.write(m_masks.size());
			for (BoneMask& mask : m_masks)
			{
				blob.write(mask.name);
				blob.write(mask.bones.size());
				for (auto iter = mask.bones.begin(), end = mask.bones.end(); iter != end; ++iter)
				{
					blob.write(iter.key());
				}
			}
			blob.write(header.magic);
		}

		e_void ControllerResource::addAnimation(e_int32 set, e_uint32 hash, Animation* animation)
		{
			m_animation_set.push({ set, hash, animation });
			if (animation) addDependency(*animation);
		}

		ComponentInstance* ControllerResource::createInstance(IAllocator& allocator) const
		{
			return m_root ? m_root->createInstance(allocator) : nullptr;
		}

		Resource* ControllerManager::createResource(const ArchivePath& path)
		{
			return _aligned_new(m_allocator, ControllerResource)(path, *this, m_allocator);
		}


		e_void ControllerManager::destroyResource(Resource& resource)
		{
			_delete(m_allocator, (ControllerResource*)&resource);
		}
	}
}
