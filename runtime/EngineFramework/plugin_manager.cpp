#include "runtime/EngineFramework/plugin_manager.h"
#include "common/egal-d.h"
#include "common/debug/debug.h"

#include "runtime/EngineFramework/engine_root.h"

#ifdef PLATFORM_WINDOWS
namespace egal
{
	bool copyFile(const char* from, const char* to)
	{
		if (CopyFile(from, to, FALSE) == FALSE) return false;

		FILETIME ft;
		SYSTEMTIME st;

		GetSystemTime(&st);
		SystemTimeToFileTime(&st, &ft);
		HANDLE handle = CreateFile(to, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		bool f = SetFileTime(handle, (LPFILETIME)NULL, (LPFILETIME)NULL, &ft) != FALSE;
		ASSERT(f);
		CloseHandle(handle);

		return true;
	}

	void messageBox(const char* text)
	{
		MessageBox(NULL, text, "Message", MB_OK);
	}

	void setCommandLine(int, char**)
	{
		ASSERT(false);
	}

	bool getCommandLine(char* output, int max_size)
	{
		return StringUnitl::copyString(output, max_size, GetCommandLine());
	}

	void* loadLibrary(const char* path)
	{
		return LoadLibrary(path);
	}

	void unloadLibrary(void* handle)
	{
		FreeLibrary((HMODULE)handle);
	}

	void* getLibrarySymbol(void* handle, const char* name)
	{
		return (void*)GetProcAddress((HMODULE)handle, name);
	}
}

#elif EGAL_PLATFORM_LINUX

namespace egal
{
	bool copyFile(const char* from, const char* to)
	{
		int source = open(from, O_RDONLY, 0);
		int dest = open(to, O_WRONLY | O_CREAT, 0644);

		struct stat stat_source;
		fstat(source, &stat_source);

		sendfile(dest, source, 0, stat_source.st_size);

		close(source);
		close(dest);

		return false;
	}


	void messageBox(const char* text)
	{
		printf("%s", text);
	}


	static int s_argc = 0;
	static char** s_argv = nullptr;


	void setCommandLine(int argc, char* argv[])
	{
		s_argc = argc;
		s_argv = argv;
	}


	bool getCommandLine(char* output, int max_size)
	{
		if (max_size <= 0) return false;
		if (s_argc < 2)
		{
			*output = '\0';
		}
		else
		{
			copyString(output, max_size, s_argv[1]);
			for (int i = 2; i < s_argc; ++i)
			{
				catString(output, max_size, " ");
				catString(output, max_size, s_argv[i]);
			}
		}
		return true;
	}


	void* loadLibrary(const char* path)
	{
		return dlopen(path, RTLD_LOCAL | RTLD_LAZY);
	}


	void unloadLibrary(void* handle)
	{
		dlclose(handle);
	}


	void* getLibrarySymbol(void* handle, const char* name)
	{
		return dlsym(handle, name);
	}
}

#endif


namespace egal
{
	PluginManager::PluginManager(EngineRoot& engine, IAllocator& allocator)
			: m_plugins(allocator)
			, m_libraries(allocator)
			, m_allocator(allocator)
			, m_engine(engine)
			, m_library_loaded(allocator)
		{ }


	PluginManager::~PluginManager()
		{
			for (int i = m_plugins.size() - 1; i >= 0; --i)
			{
				_delete(m_engine.getAllocator(), m_plugins[i]);
			}

			for (int i = 0; i < m_libraries.size(); ++i)
			{
				unloadLibrary(m_libraries[i]);
			}
		}


		void PluginManager::frame(float dt, bool paused)
		{
			//PROFILE_FUNCTION();
			for (int i = 0, c = m_plugins.size(); i < c; ++i)
			{
				m_plugins[i]->update(dt);
			}
		}


		void PluginManager::serialize(WriteBinary& serializer)
		{
			for (int i = 0, c = m_plugins.size(); i < c; ++i)
			{
				m_plugins[i]->serialize(serializer);
			}
		}


		void PluginManager::deserialize(ReadBinary& serializer)
		{
			for (int i = 0, c = m_plugins.size(); i < c; ++i)
			{
				m_plugins[i]->deserialize(serializer);
			}
		}


		const TVector<void*>& PluginManager::getLibraries() const
		{
			return m_libraries;
		}


		const TVector<IPlugin*>& PluginManager::getPlugins() const
		{
			return m_plugins;
		}


		IPlugin* PluginManager::getPlugin(const char* name)
		{
			for (int i = 0; i < m_plugins.size(); ++i)
			{
				if (StringUnitl::equalStrings(m_plugins[i]->getName(), name))
				{
					return m_plugins[i];
				}
			}
			return 0;
		}


		TDelegateList<void(void*)>& PluginManager::libraryLoaded()
		{
			return m_library_loaded;
		}


		IPlugin* PluginManager::load(const char* path)
		{
			char path_with_ext[MAX_PATH_LENGTH];
			StringUnitl::copyString(path_with_ext, path);
#ifdef _WIN32
			StringUnitl::catString(path_with_ext, ".dll");
#elif defined __linux__
			StringUnitl::catString(path_with_ext, ".so");
#else 
#error Unknown platform
#endif
			log_info("Core loading plugin %s.", path_with_ext);
			typedef IPlugin* (*PluginCreator)(EngineRoot&);
			auto* lib = loadLibrary(path_with_ext);
			if (lib)
			{
				PluginCreator creator = (PluginCreator)getLibrarySymbol(lib, "createPlugin");
				if (creator)
				{
					IPlugin* plugin = creator(m_engine);
					if (!plugin)
					{
						log_error("Core createPlugin failed.");
						_delete(m_engine.getAllocator(), plugin);
						ASSERT(false);
					}
					else
					{
						addPlugin(plugin);
						m_libraries.push_back(lib);
						m_library_loaded.invoke(lib);
						log_info("Core Plugin loaded.");
						Debug::StackTree::refreshModuleList();
						return plugin;
					}
				}
				else
				{
					log_error("Core No createPlugin function in plugin.");
				}
				unloadLibrary(lib);
			}
			else
			{
				auto* plugin = StaticPluginRegister::create(path, m_engine);
				if (plugin)
				{
					log_info("Core Plugin loaded.");
					addPlugin(plugin);
					return plugin;
				}
				log_waring("Core Failed to load plugin.");
			}
			return nullptr;
		}


		IAllocator& PluginManager::getAllocator() { return m_allocator; }


		void PluginManager::addPlugin(IPlugin* plugin)
		{
			m_plugins.push_back(plugin);
			for (auto* i : m_plugins)
			{
				i->pluginAdded(*plugin);
				plugin->pluginAdded(*i);
			}
		}


	PluginManager* PluginManager::create(EngineRoot& engine)
	{
		return _aligned_new(engine.getAllocator(), PluginManager)(engine, engine.getAllocator());
	}

	void PluginManager::destroy(PluginManager* manager)
	{
		_delete(static_cast<PluginManager*>(manager)->getAllocator(), manager);
	}

	static StaticPluginRegister* s_first_plugin = nullptr;
	StaticPluginRegister::StaticPluginRegister(const char* name, Creator creator)
	{
		this->creator = creator;
		this->name = name;
		next = s_first_plugin;
		s_first_plugin = this;
	}

	IPlugin* StaticPluginRegister::create(const char* name, EngineRoot& engine)
	{
		auto* i = s_first_plugin;
		while (i)
		{
			if (StringUnitl::equalStrings(name, i->name))
			{
				return i->creator(engine);
			}
			i = i->next;
		}
		return nullptr;
	}

}
