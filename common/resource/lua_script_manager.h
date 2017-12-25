#ifndef _lua_script_manager_h_
#define _lua_script_manager_h_
#pragma once

#include "runtime/Resource/resource_define.h"
#include "runtime/Resource/resource_public.h"
#include "runtime/Resource/resource_manager.h"

namespace egal
{
	class LuaScript : public Resource
	{
	public:
		LuaScript(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		virtual ~LuaScript();

		e_void unload() override;
		e_bool load(FS::IFile& file) override;
		const e_char* getSourceCode() const { return m_source_code.c_str(); }

	private:
		string m_source_code;
	};


	class LuaScriptManager : public ResourceManagerBase
	{
	public:
		explicit LuaScriptManager(IAllocator& allocator);
		~LuaScriptManager();

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
	};
}

#endif
