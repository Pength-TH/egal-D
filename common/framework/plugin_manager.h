#ifndef _plugin_manager_h_
#define _plugin_manager_h_
#pragma once

#include "common/egal-d.h"
#include "common/serializer/serializer.h"

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
	struct IPlugin;
	struct IScene
	{
		virtual ~IScene() {}

		virtual ComponentHandle createComponent(ComponentType, Entity) = 0;
		virtual void destroyComponent(ComponentHandle component, ComponentType type) = 0;
		virtual void serialize(WriteBinary& serializer) = 0;
		virtual void serialize(ISerializer& serializer) {}
		virtual void deserialize(IDeserializer& serializer) {}
		virtual void deserialize(ReadBinary& serializer) = 0;
		virtual IPlugin& getPlugin() const = 0;
		virtual void update(float time_delta, bool paused) = 0;
		virtual void lateUpdate(float time_delta, bool paused) {}
		virtual ComponentHandle getComponent(Entity entity, ComponentType type) = 0;
		virtual Universe& getUniverse() = 0;
		virtual void startGame() {}
		virtual void stopGame() {}
		virtual int getVersion() const { return -1; }
		virtual void clear() = 0;
	};

	struct IPlugin
	{
		virtual ~IPlugin();

		virtual void serialize(WriteBinary&) {}
		virtual void deserialize(ReadBinary&) {}
		virtual void update(float) {}
		virtual const char* getName() const = 0;
		virtual void pluginAdded(IPlugin& plugin) {}

		virtual void createScenes(Universe&) {}
		virtual void destroyScene(IScene*) { ASSERT(false); }
		virtual void startGame() {}
		virtual void stopGame() {}
	};

	class PluginManager
	{
	public:
		virtual ~PluginManager() {}

		static PluginManager* create(Engine& engine);
		static void destroy(PluginManager* manager);

		virtual IPlugin* load(const char* path) = 0;
		virtual void addPlugin(IPlugin* plugin) = 0;
		virtual void update(float dt, bool paused) = 0;
		virtual void serialize(WriteBinary& serializer) = 0;
		virtual void deserialize(ReadBinary& serializer) = 0;
		virtual IPlugin* getPlugin(const char* name) = 0;
		virtual const TVector<IPlugin*>& getPlugins() const = 0;
		virtual const TVector<void*>& getLibraries() const = 0;
		virtual DelegateList<void(void*)>& libraryLoaded() = 0;
	};

	struct StaticPluginRegister
	{
		typedef IPlugin* (*Creator)(Engine& engine);
		StaticPluginRegister(const char* name, Creator creator);

		static IPlugin* create(const char* name, Engine& engine);

		StaticPluginRegister* next;
		Creator creator;
		const char* name;
	};

}
#endif
