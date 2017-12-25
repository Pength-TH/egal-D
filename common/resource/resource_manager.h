#ifndef _resource_manager_h_
#define _resource_manager_h_

#include "common/type.h"
#include "common/filesystem/ifile_device.h"

#include "common/resource/resource_public.h"
namespace egal
{
	class ResourceManager;
	class ResourceManagerBase;
	class Resource
	{
	public:
		friend class ResourceManagerBase;

		enum class State : e_uint32
		{
			EMPTY = 0,
			READY,
			FAILURE,
		};

		typedef TDelegateList<e_void(State, State, Resource&)> ObserverCallback;

	public:
		INLINE State getState() const { return m_current_state; }

		INLINE e_bool isEmpty() const { return State::EMPTY == m_current_state; }
		INLINE e_bool isReady() const { return State::READY == m_current_state; }
		INLINE e_bool isFailure() const { return State::FAILURE == m_current_state; }
		INLINE e_uint32 getRefCount() const { return m_ref_count; }
		INLINE ObserverCallback& getObserverCb() { return m_cb; }
		INLINE size_t size() const { return m_size; }
		INLINE const ArchivePath& getPath() const { return m_path; }
		INLINE ResourceManagerBase& getResourceManager() { return m_resource_manager; }

		template <typename C, e_void (C::*Function)(State, State, Resource&)> e_void onLoaded(C* instance)
		{
			m_cb.bind<C, Function>(instance);
			if (isReady())
			{
				(instance->*Function)(State::READY, State::READY, *this);
			}
		}

	protected:
		Resource(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		virtual ~Resource();

		virtual e_void onBeforeReady() {}
		virtual e_void onBeforeEmpty() {}
		virtual e_void unload() = 0;
		virtual e_bool load(FS::IFile& file) = 0;

		e_void onCreated(State state);
		e_void doUnload();

		e_void addDependency(Resource& dependent_resource);
		e_void removeDependency(Resource& dependent_resource);

	protected:
		State	 m_desired_state;
		e_uint16 m_empty_dep_count;
		size_t	 m_size;
		ResourceManagerBase& m_resource_manager;

	protected:
		e_void checkState();

	private:
		e_void doLoad();
		e_void fileLoaded(FS::IFile& file, e_bool success);
		e_void onStateChanged(State old_state, State new_state, Resource&);

		INLINE e_uint32 addRef() { return ++m_ref_count; }
		INLINE e_uint32 remRef() { return --m_ref_count; }

		Resource(const Resource&);
		e_void operator=(const Resource&);

	private:
		ObserverCallback m_cb;
		ArchivePath m_path;
		e_uint16	m_ref_count;
		e_uint16	m_failed_dep_count;
		State		m_current_state;
		e_uint32	m_async_op;
	}; // class Resource
}


namespace egal
{
	class ResourceManagerBase
	{
		friend class Resource;
	public:
		typedef THashMap<e_uint32, Resource*> ResourceTable;

		struct LoadHook
		{
			explicit LoadHook(ResourceManagerBase& manager) : m_manager(manager) {}
			virtual ~LoadHook() {}

			virtual e_bool onBeforeLoad(Resource& resource) = 0;
			e_void continueLoad(Resource& resource);

			ResourceManagerBase& m_manager;
		};

	public:
		e_void create(ResourceType type, ResourceManager& owner);
		e_void destroy();

		e_void setLoadHook(LoadHook& load_hook);
		e_void enableUnload(e_bool enable);

		Resource* load(const ArchivePath& path);
		e_void load(Resource& resource);
		e_void removeUnreferenced();

		e_void unload(const ArchivePath& path);
		e_void unload(Resource& resource);

		e_void reload(const ArchivePath& path);
		e_void reload(Resource& resource);
		ResourceTable& getResourceTable() { return m_resources; }

		explicit ResourceManagerBase(IAllocator& allocator);
		virtual ~ResourceManagerBase();
		ResourceManager& getOwner() const { return *m_owner; }

	protected:
		virtual Resource* createResource(const ArchivePath& path) = 0;
		virtual e_void destroyResource(Resource& resource) = 0;
		Resource* get(const ArchivePath& path);

	private:
		IAllocator& m_allocator;
		LoadHook* m_load_hook;
		ResourceTable m_resources;
		ResourceManager* m_owner;
		e_bool m_is_unload_enabled;
	};

	class ResourceManager
	{
		typedef THashMap<e_uint32, ResourceManagerBase*> ResourceManagerTable;

	public:
		explicit ResourceManager(IAllocator& allocator);
		~ResourceManager();

		e_void create(FS::FileSystem& fs);
		e_void destroy();

		IAllocator& getAllocator() { return m_allocator; }
		ResourceManagerBase* get(ResourceType type);
		const ResourceManagerTable& getAll() const { return m_resource_managers; }

		e_void add(ResourceType type, ResourceManagerBase* rm);
		e_void remove(ResourceType type);
		e_void reload(const ArchivePath& path);
		e_void removeUnreferenced();
		e_void enableUnload(e_bool enable);

		FS::FileSystem& getFileSystem() { return *m_file_system; }

	private:
		IAllocator& m_allocator;
		ResourceManagerTable m_resource_managers;
		FS::FileSystem* m_file_system;
	};
}
#endif
