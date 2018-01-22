#ifndef _plugin_manager_h_
#define _plugin_manager_h_
#pragma once
#include "common/type.h"
#include "common/struct.h"
#include "common/template.h"
#include "common/stl/delegate.h"
#include "common/stl/delegate_list.h"
namespace egal
{
	bool copyFile(const char* from, const char* to);
	void messageBox(const char* text);
	void setCommandLine(int argc, char* argv[]);
	bool getCommandLine(char* output, int max_size);
	void* loadLibrary(const char* path);
	void unloadLibrary(void* handle);
	void* getLibrarySymbol(void* handle, const char* name);
}

namespace egal
{
	class ComponentManager;
	class SceneManager;
	class WriteBinary;
	class ReadBinary;
	struct ISerializer;
	struct IDeserializer;
	class EngineRoot;
	class Renderer;

	struct IPlugin
	{
		~IPlugin() {}

		void serialize(WriteBinary&) {}
		void deserialize(ReadBinary&) {}
		void update(float) {}
		const char* getName() const { return ""; };
		void pluginAdded(IPlugin& plugin) {}

		void createScenes(ComponentManager&) {}
		void destroyScene(SceneManager*) { ASSERT(false); }
		void startGame() {}
		void stopGame() {}
	};

	class PluginManager
	{
	private:
		typedef TArrary<IPlugin*> PluginList;
		typedef TArrary<void*> LibraryList;
	public:
		PluginManager(EngineRoot& engine, IAllocator& allocator);
		~PluginManager();

		static PluginManager* create(EngineRoot& engine);
		static void destroy(PluginManager* manager);

		IPlugin* load(const char* path);
		IAllocator& getAllocator();
		void addPlugin(IPlugin* plugin);
		void frame(float dt, bool paused);
		void serialize(WriteBinary& serializer);
		void deserialize(ReadBinary& serializer);
		IPlugin* getPlugin(const char* name);
		const TArrary<IPlugin*>& getPlugins() const;
		const TArrary<void*>& getLibraries() const;
		TDelegateList<void(void*)>& libraryLoaded();
	private:
		EngineRoot& m_engine;
		TDelegateList<void(void*)> m_library_loaded;
		LibraryList m_libraries;
		PluginList m_plugins;
		IAllocator& m_allocator;
	};

	struct StaticPluginRegister
	{
		typedef IPlugin* (*Creator)(EngineRoot& engine);
		StaticPluginRegister(const char* name, Creator creator);

		static IPlugin* create(const char* name, EngineRoot& engine);

		StaticPluginRegister* next;
		Creator creator;
		const char* name;
	};
}

#endif
