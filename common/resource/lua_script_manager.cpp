#include "runtime/Resource/lua_script_manager.h"

namespace egal
{
	LuaScript::LuaScript(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_source_code(allocator)
	{
	}


	LuaScript::~LuaScript() = default;


	e_void LuaScript::unload()
	{
		m_source_code = "";
	}


	e_bool LuaScript::load(FS::IFile& file)
	{
		m_source_code.set((const e_char*)file.getBuffer(), (e_int32)file.size());
		m_size = file.size();
		return true;
	}


	LuaScriptManager::LuaScriptManager(IAllocator& allocator)
		: ResourceManagerBase(allocator)
		, m_allocator(allocator)
	{
	}


	LuaScriptManager::~LuaScriptManager() = default;


	Resource* LuaScriptManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, LuaScript)(path, *this, m_allocator);
	}


	e_void LuaScriptManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<LuaScript*>(&resource));
	}
}
