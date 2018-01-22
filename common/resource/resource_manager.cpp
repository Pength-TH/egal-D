#include "common/resource/resource_manager.h"

namespace egal
{
	Resource::Resource(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: m_ref_count()
		, m_empty_dep_count(1)
		, m_failed_dep_count(0)
		, m_current_state(State::EMPTY)
		, m_desired_state(State::EMPTY)
		, m_path(path)
		, m_size()
		, m_cb(allocator)
		, m_resource_manager(resource_manager)
		, m_async_op(FS::FileSystem::INVALID_ASYNC)
	{
	}


	Resource::~Resource() = default;


	e_void Resource::checkState()
	{
		auto old_state = m_current_state;
		if (m_failed_dep_count > 0 && m_current_state != State::FAILURE)
		{
			m_current_state = State::FAILURE;
			m_cb.invoke(old_state, m_current_state, *this);
		}

		if (m_failed_dep_count == 0)
		{
			if (m_empty_dep_count == 0 && m_current_state != State::READY &&
				m_desired_state != State::EMPTY)
			{
				onBeforeReady();
				m_current_state = State::READY;
				m_cb.invoke(old_state, m_current_state, *this);
			}

			if (m_empty_dep_count > 0 && m_current_state != State::EMPTY)
			{
				onBeforeEmpty();
				m_current_state = State::EMPTY;
				m_cb.invoke(old_state, m_current_state, *this);
			}
		}
	}


	e_void Resource::fileLoaded(FS::IFile& file, e_bool success)
	{
		m_async_op = FS::FileSystem::INVALID_ASYNC;
		if (m_desired_state != State::READY) return;

		ASSERT(m_current_state != State::READY);
		ASSERT(m_empty_dep_count == 1);

		if (!success)
		{
			log_error("Core Could not open %s", getPath().c_str());
			ASSERT(m_empty_dep_count > 0);
			--m_empty_dep_count;
			++m_failed_dep_count;
			checkState();
			m_async_op = FS::FileSystem::INVALID_ASYNC;
			return;
		}

		if (!load(file))
		{
			++m_failed_dep_count;
		}

		ASSERT(m_empty_dep_count > 0);
		--m_empty_dep_count;
		checkState();
		m_async_op = FS::FileSystem::INVALID_ASYNC;
	}


	e_void Resource::doUnload()
	{
		if (m_async_op != FS::FileSystem::INVALID_ASYNC)
		{
			FS::FileSystem& fs = m_resource_manager.getOwner().getFileSystem();
			fs.cancelAsync(m_async_op);
			m_async_op = FS::FileSystem::INVALID_ASYNC;
		}

		m_desired_state = State::EMPTY;
		unload();
		ASSERT(m_empty_dep_count <= 1);

		m_size = 0;
		m_empty_dep_count = 1;
		m_failed_dep_count = 0;
		checkState();
	}


	e_void Resource::onCreated(State state)
	{
		ASSERT(m_empty_dep_count == 1);
		ASSERT(m_failed_dep_count == 0);

		m_current_state = state;
		m_desired_state = State::READY;
		m_failed_dep_count = state == State::FAILURE ? 1 : 0;
		m_empty_dep_count = 0;
	}


	e_void Resource::doLoad()
	{
		if (m_desired_state == State::READY) return;
		m_desired_state = State::READY;

		if (m_async_op != FS::FileSystem::INVALID_ASYNC) return;
		FS::FileSystem& fs = m_resource_manager.getOwner().getFileSystem();
		FS::ReadCallback cb;
		cb.bind<Resource, &Resource::fileLoaded>(this);
		m_async_op = fs.openAsync(fs.getDefaultDevice(), m_path.c_str(), FS::Mode::OPEN_AND_READ, cb);
	}


	e_void Resource::addDependency(Resource& dependent_resource)
	{
		ASSERT(m_desired_state != State::EMPTY);

		dependent_resource.m_cb.bind<Resource, &Resource::onStateChanged>(this);
		if (dependent_resource.isEmpty()) 
			++m_empty_dep_count;
		if (dependent_resource.isFailure()) 
			++m_failed_dep_count;

		checkState();
	}


	e_void Resource::removeDependency(Resource& dependent_resource)
	{
		dependent_resource.m_cb.unbind<Resource, &Resource::onStateChanged>(this);
		if (dependent_resource.isEmpty())
		{
			ASSERT(m_empty_dep_count > 0);
			--m_empty_dep_count;
		}
		if (dependent_resource.isFailure())
		{
			ASSERT(m_failed_dep_count > 0);
			--m_failed_dep_count;
		}

		checkState();
	}


	e_void Resource::onStateChanged(State old_state, State new_state, Resource&)
	{
		ASSERT(old_state != new_state);
		ASSERT(m_current_state != State::EMPTY || m_desired_state != State::EMPTY);

		if (old_state == State::EMPTY)
		{
			ASSERT(m_empty_dep_count > 0);
			--m_empty_dep_count;
		}
		if (old_state == State::FAILURE)
		{
			ASSERT(m_failed_dep_count > 0);
			--m_failed_dep_count;
		}

		if (new_state == State::EMPTY) ++m_empty_dep_count;
		if (new_state == State::FAILURE) ++m_failed_dep_count;

		checkState();
	}
}

namespace egal
{
	e_void ResourceManagerBase::LoadHook::continueLoad(Resource& resource)
	{
		ASSERT(resource.isEmpty());
		resource.remRef(); // release from hook
		resource.m_desired_state = Resource::State::EMPTY;
		resource.doLoad();
	}

	e_void ResourceManagerBase::create(ResourceType type, ResourceManager& owner)
	{
		owner.add(type, this);
		m_owner = &owner;
	}

	e_void ResourceManagerBase::destroy()
	{
		for (auto iter = m_resources.begin(), end = m_resources.end(); iter != end; ++iter)
		{
			Resource* resource = iter.value();
			if (!resource->isEmpty())
			{
				log_error("Engine Leaking resource %s", resource->getPath().c_str());
			}
			destroyResource(*resource);
		}
		m_resources.clear();
	}

	Resource* ResourceManagerBase::get(const ArchivePath& path)
	{
		ResourceTable::iterator it = m_resources.find(path.m_data->getHash());

		if (m_resources.end() != it)
		{
			return *it;
		}

		return nullptr;
	}

	Resource* ResourceManagerBase::load(const ArchivePath& path)
	{
		if (!path.isValid()) 
			return nullptr;
		
		Resource* resource = get(path);

		if (nullptr == resource)
		{
			resource = createResource(path);
			m_resources.insert(path.m_data->getHash(), resource);
		}

		if (resource->isEmpty() && resource->m_desired_state == Resource::State::EMPTY)
		{
			if (m_load_hook && m_load_hook->onBeforeLoad(*resource))
			{
				resource->m_desired_state = Resource::State::READY;
				resource->addRef(); // for hook
				resource->addRef(); // for return value
				return resource;
			}
			resource->doLoad();
		}

		resource->addRef();
		return resource;
	}

	egal::Resource* ResourceManagerBase::create()
	{
		Resource* resource = createResource(); //空的资源 不需要addref，设置路径后 条用load 在add
		return resource;
	}

	e_void ResourceManagerBase::removeUnreferenced()
	{
		if (!m_is_unload_enabled) return;

		TArrary<Resource*> to_remove(m_allocator);
		for (auto* i : m_resources)
		{
			if (i->getRefCount() == 0) to_remove.push_back(i);
		}

		for (auto* i : to_remove)
		{
			m_resources.erase(i->getPath().m_data->getHash());
			destroyResource(*i);
		}
	}

	e_void ResourceManagerBase::load(Resource& resource)
	{
		if (resource.isEmpty() && resource.m_desired_state == Resource::State::EMPTY)
		{
			if (m_load_hook && m_load_hook->onBeforeLoad(resource))
			{
				resource.addRef(); // for hook
				resource.addRef(); // for return value
				return;
			}
			resource.doLoad();
		}

		resource.addRef();
	}

	e_void ResourceManagerBase::unload(const ArchivePath& path)
	{
		Resource* resource = get(path);
		if (resource) unload(*resource);
	}

	e_void ResourceManagerBase::unload(Resource& resource)
	{
		e_int32 new_ref_count = resource.remRef();
		ASSERT(new_ref_count >= 0);
		if (new_ref_count == 0 && m_is_unload_enabled)
		{
			resource.doUnload();
		}
	}

	e_void ResourceManagerBase::reload(const ArchivePath& path)
	{
		Resource* resource = get(path);
		if (resource) reload(*resource);
	}

	e_void ResourceManagerBase::reload(Resource& resource)
	{
		resource.doUnload();
		resource.doLoad();
	}

	e_void ResourceManagerBase::enableUnload(e_bool enable)
	{
		m_is_unload_enabled = enable;
		if (!enable) return;

		for (auto* resource : m_resources)
		{
			if (resource->getRefCount() == 0)
			{
				resource->doUnload();
			}
		}
	}

	e_void ResourceManagerBase::setLoadHook(LoadHook& load_hook)
	{
		ASSERT(!m_load_hook);
		m_load_hook = &load_hook;
	}

	ResourceManagerBase::ResourceManagerBase(IAllocator& allocator)
		: m_resources(allocator)
		, m_allocator(allocator)
		, m_owner(nullptr)
		, m_is_unload_enabled(true)
		, m_load_hook(nullptr)
	{ }

	ResourceManagerBase::~ResourceManagerBase()
	{
		ASSERT(m_resources.empty());
	}

	/*************************************************************************************/
	/*************************************************************************************/
	/*************************************************************************************/
	ResourceManager::ResourceManager(IAllocator& allocator)
		: m_resource_managers(allocator)
		, m_allocator(allocator)
		, m_file_system(nullptr)
	{
	}

	ResourceManager::~ResourceManager() = default;


	e_void ResourceManager::create(FS::FileSystem& fs)
	{
		m_file_system = &fs;
	}

	e_void ResourceManager::destroy()
	{
	}

	ResourceManagerBase* ResourceManager::get(ResourceType type)
	{
		return m_resource_managers[type.type];
	}

	e_void ResourceManager::add(ResourceType type, ResourceManagerBase* rm)
	{
		m_resource_managers.insert(type.type, rm);
	}

	e_void ResourceManager::remove(ResourceType type)
	{
		m_resource_managers.erase(type.type);
	}

	e_void ResourceManager::removeUnreferenced()
	{
		for (auto* manager : m_resource_managers)
		{
			manager->removeUnreferenced();
		}
	}

	e_void ResourceManager::enableUnload(e_bool enable)
	{
		for (auto* manager : m_resource_managers)
		{
			manager->enableUnload(enable);
		}
	}

	e_void ResourceManager::reload(const ArchivePath& path)
	{
		for (auto* manager : m_resource_managers)
		{
			manager->reload(path);
		}
	}
}