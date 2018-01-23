
#include "runtime/EngineFramework/engine_root.h"
#include "common/egal-d.h"
#include "common/lua/lua_function.h"
#include "common/lua/lua_manager.h"
#include "runtime/EngineFramework/buffer.h"
#include "runtime/EngineFramework/component_manager.h"
#include "runtime/EngineFramework/engine_root.h"
#include "runtime/EngineFramework/scene_manager.h"
#include "runtime/EngineFramework/renderer.h"
#include "runtime/EngineFramework/culling_system.h"
#include "runtime/EngineFramework/pipeline.h"
#include "common/lua/lua_function.h"

#include "common/input/framework_listener.h"

#include "common/resource/entity_manager.h"
#include "runtime/EngineFramework/scene_manager.h"

namespace egal
{
	IAllocator* g_allocator;

#pragma pack(1)
	class SerializedEngineHeader
	{
	public:
		e_uint32 m_magic;
		e_uint32 m_reserved; // for crc
	};
#pragma pack()

	template<> EngineRoot * Singleton<EngineRoot>::msSingleton = 0;

	EngineRoot::EngineRoot(const char* base_path0, const char* base_path1, FS::FileSystem* fs, IAllocator& allocator)
		: m_allocator(allocator)
		, m_resource_manager(allocator)
		, m_lua_resources(allocator)
		, m_last_lua_resource_idx(-1)
		, m_fps(0)
		, m_is_game_running(false)
		, m_last_time_delta(0)
		, m_time(0)
		, m_path_manager(allocator)
		, m_time_multiplier(1.0f)
		, m_paused(false)
		, m_next_frame(false)
		, m_lifo_allocator(allocator, 10 * 1024 * 1024)
		, m_p_render(nullptr)
		, m_p_pipeline(nullptr)
	{
		log_info("Core Creating engine...");
		
		Profiler::setThreadName("Main");

		/** init dump */
		enableCrashReporting(false);
		installUnhandledExceptionHandler();
		
		m_platform_data = {};

		m_p_lua_manager = new LuaManager();
		m_p_lua_manager->init(m_allocator);

		JobSystem::init(m_allocator);
	
		if (!fs)
			init_file_system(allocator);

		m_resource_manager.create(*g_file_system);

		m_timer = Timer::create(m_allocator);
		m_fps_timer = Timer::create(m_allocator);
		m_fps_frame = 0;

		Reflection::init(m_allocator);
		
		m_plugin_manager = PluginManager::create(*this);
		
		m_input_system = InputSystem::create(*this);
		
		log_info("Core Engine create successed.");
	}

	EngineRoot::~EngineRoot()
	{
		InputSystem::destroy(*m_input_system);

		PluginManager::destroy(m_plugin_manager);
		
		Reflection::shutdown();

		Timer::destroy(m_timer);
		Timer::destroy(m_fps_timer);

		m_resource_manager.destroy();

		JobSystem::shutdown();

		for (Resource* res : m_lua_resources)
		{
			res->getResourceManager().unload(*res);
		}

		SAFE_DELETE(m_p_lua_manager);
		SAFE_DELETE(m_p_component_manager);
	}

	Renderer& EngineRoot::getRender()
	{
		return *m_p_render;
	}

	void EngineRoot::setPatchArchivePath(const char* path)
	{

	}

	void EngineRoot::setPlatformData(const PlatformData& data)
	{
		m_platform_data = data;

	}

	const EngineRoot::PlatformData& EngineRoot::getPlatformData()
	{
		return m_platform_data;
	}

	IAllocator& EngineRoot::getAllocator() { return m_allocator; }

	ComponentManager& EngineRoot::createComponentManager()
	{
		m_p_component_manager = new ComponentManager(m_allocator);
		m_p_component_manager->setName("com_manager");

		if (!m_p_render)
			m_p_render = Renderer::create(*this);
			
		m_p_scene_manager = new SceneManager(*m_p_render, *this, *m_p_component_manager, m_allocator);//  SceneManager::createInstance(*m_p_render, *this, *m_p_component_manager, m_allocator);
		m_p_component_manager->addScene(m_p_scene_manager);

		m_p_pipeline = Pipeline::create(*m_p_render, ArchivePath("./preview.lua"), "egal", m_allocator);
		m_p_pipeline->load();

		m_p_render->setMainPipeline(m_p_pipeline);

		while (g_file_system->hasWork())
		{
			MT::sleep(100);
			g_file_system->updateAsyncTransactions();
		}

		m_p_pipeline->setScene(m_p_scene_manager);
		m_p_pipeline->resize(1024, 768);
		m_p_render->resize(1024, 768);

		const TArrary<IPlugin*>& plugins = m_plugin_manager->getPlugins();
		for (auto* plugin : plugins)
		{
			plugin->createScenes(*m_p_component_manager);
		}

		return *m_p_component_manager;
	}

	void EngineRoot::destroyComponentManager(ComponentManager& com_man)
	{
		auto& scenes = com_man.getScenes();
		for (int i = scenes.size() - 1; i >= 0; --i)
		{
			auto* scene = scenes[i];
			scenes.pop_back();
			scene->clear();
			scene->getPlugin().destroyScene(scene);
		}

		if (m_p_pipeline)
			Pipeline::destroy(m_p_pipeline);

		SAFE_DELETE(m_p_scene_manager);

		m_resource_manager.removeUnreferenced();

		if (m_p_render)
			Renderer::destroy(m_p_render, m_allocator);

		SAFE_DELETE(m_p_component_manager);
	}

	PluginManager& EngineRoot::getPluginManager()
	{
		return *m_plugin_manager;
	}

	void EngineRoot::startGame()
	{
		ASSERT(!m_is_game_running);
		m_is_game_running = true;
		for (auto* scene : m_p_component_manager->getScenes())
		{
			scene->startGame();
		}
		for (auto* plugin : m_plugin_manager->getPlugins())
		{
			plugin->startGame();
		}
	}

	void EngineRoot::stopGame()
	{
		ASSERT(m_is_game_running);
		m_is_game_running = false;
		for (auto* scene : m_p_component_manager->getScenes())
		{
			scene->stopGame();
		}
		for (auto* plugin : m_plugin_manager->getPlugins())
		{
			plugin->stopGame();
		}
	}

	void EngineRoot::pause(bool pause)
	{
		m_paused = pause;
	}

	void EngineRoot::nextFrame()
	{
		m_next_frame = true;
	}

	void EngineRoot::setTimeMultiplier(float multiplier)
	{
		m_time_multiplier = Math::maximum(multiplier, 0.001f);
	}

	void EngineRoot::frame()
	{
		//PROFILE_FUNCTION();
		++m_fps_frame;
		if (m_fps_timer->getTimeSinceTick() > 0.5f)
		{
			m_fps = m_fps_frame / m_fps_timer->tick();
			m_fps_frame = 0;
		}
		float dt = m_timer->tick() * m_time_multiplier;
		if (m_next_frame)
		{
			m_paused = false;
			dt = 1 / 30.0f;
		}
		m_time += dt;
		m_last_time_delta = dt;
		{
			//PROFILE_BLOCK("update scenes");
			for (auto* scene : m_p_component_manager->getScenes())
			{
				scene->frame(dt, m_paused);
			}
		}
		{
			//PROFILE_BLOCK("late update scenes");
			for (auto* scene : m_p_component_manager->getScenes())
			{
				scene->lateUpdate(dt, m_paused);
			}
		}

		m_p_scene_manager->frame(dt, false);
		m_p_pipeline->frame();

		m_plugin_manager->frame(dt, m_paused);
		m_input_system->update(dt);
		g_file_system->updateAsyncTransactions();

		m_p_render->frame(false);
		if (m_next_frame)
		{
			m_paused = true;
			m_next_frame = false;
		}
	}

	egal::e_void EngineRoot::on_mouse_moved(e_float x, e_float y)
	{
// 		if (m_p_scene_manager)
// 			m_p_scene_manager->camera_rotate(m_camera, x, y);
	}

	InputSystem& EngineRoot::getInputSystem() { return *m_input_system; }

	ResourceManager& EngineRoot::getResourceManager()
	{
		return m_resource_manager;
	}

	float EngineRoot::getFPS() const { return m_fps; }

	void EngineRoot::serializerSceneVersions(WriteBinary& serializer)
	{
		serializer.write(m_p_component_manager->getScenes().size());
		for (auto* scene : m_p_component_manager->getScenes())
		{
			serializer.write(crc32(scene->getPlugin().getName()));
			serializer.write(scene->getVersion());
		}
	}

	void EngineRoot::serializePluginList(WriteBinary& serializer)
	{
		serializer.write((e_int32)m_plugin_manager->getPlugins().size());
		for (auto* plugin : m_plugin_manager->getPlugins())
		{
			serializer.writeString(plugin->getName());
		}
	}

	bool EngineRoot::hasSupportedSceneVersions(ReadBinary& serializer)
	{
		e_int32 count;
		serializer.read(count);
		for (int i = 0; i < count; ++i)
		{
			e_uint32 hash;
			serializer.read(hash);
			auto* scene = m_p_component_manager->getScene(hash);
			int version;
			serializer.read(version);
			if (version != scene->getVersion())
			{
				log_error("Core Plugin %s has incompatible version.", scene->getPlugin().getName());
				return false;
			}
		}
		return true;
	}

	bool EngineRoot::hasSerializedPlugins(ReadBinary& serializer)
	{
		e_int32 count;
		serializer.read(count);
		for (int i = 0; i < count; ++i)
		{
			char tmp[32];
			serializer.readString(tmp, sizeof(tmp));
			if (!m_plugin_manager->getPlugin(tmp))
			{
				log_error("Core Missing plugin %s.", tmp);
				return false;
			}
		}
		return true;
	}

	e_uint32 EngineRoot::serialize(WriteBinary& serializer)
	{
		SerializedEngineHeader header;
		header.m_magic = SERIALIZED_ENGINE_MAGIC; // == '_LEN'
		header.m_reserved = 0;
		serializer.write(header);
		serializePluginList(serializer);
		serializerSceneVersions(serializer);
		//m_path_manager.serialize(serializer);
		int pos = serializer.getPos();
		m_p_component_manager->serialize(serializer);
		m_plugin_manager->serialize(serializer);
		serializer.write((e_int32)m_p_component_manager->getScenes().size());
		for (auto* scene : m_p_component_manager->getScenes())
		{
			serializer.writeString(scene->getPlugin().getName());
			scene->serialize(serializer);
		}
		e_uint32 crc = crc32((const e_uint8*)serializer.getData() + pos, serializer.getPos() - pos);
		return crc;
	}

	bool EngineRoot::deserialize(ReadBinary& serializer)
	{
		SerializedEngineHeader header;
		serializer.read(header);
		if (header.m_magic != SERIALIZED_ENGINE_MAGIC)
		{
			log_error("Core Wrong or corrupted file");
			return false;
		}
		if (!hasSerializedPlugins(serializer)) return false;
		if (!hasSupportedSceneVersions(serializer)) return false;

		m_p_component_manager->deserialize(serializer);
		m_plugin_manager->deserialize(serializer);
		e_int32 scene_count;
		serializer.read(scene_count);
		for (int i = 0; i < scene_count; ++i)
		{
			char tmp[32];
			serializer.readString(tmp, sizeof(tmp));
			SceneManager* scene = m_p_component_manager->getScene(crc32(tmp));
			scene->deserialize(serializer);
		}
		m_path_manager.clear();
		return true;
	}

	ComponentUID EngineRoot::createComponent(GameObject game_object, ComponentType type)
	{
		SceneManager* scene = m_p_component_manager->getScene(type);
		if (!scene) return ComponentUID::INVALID;

		return ComponentUID(game_object, type, scene, scene->createComponent(type, game_object));
	}

	void EngineRoot::unloadLuaResource(int resource_idx)
	{
		if (resource_idx < 0) return;
		Resource* res = m_lua_resources[resource_idx];
		m_lua_resources.erase(resource_idx);
		res->getResourceManager().unload(*res);
	}

	int EngineRoot::addLuaResource(const ArchivePath& path, ResourceType type)
	{
		ResourceManagerBase* manager = m_resource_manager.get(type);
		if (!manager) return -1;
		Resource* res = manager->load(path);
		++m_last_lua_resource_idx;
		m_lua_resources.insert(m_last_lua_resource_idx, res);
		return m_last_lua_resource_idx;
	}

	Resource* EngineRoot::getLuaResource(int idx) const
	{
		auto iter = m_lua_resources.find(idx);
		if (iter.isValid()) return iter.value();
		return nullptr;
	}

	IAllocator& EngineRoot::getLIFOAllocator()
	{
		return m_lifo_allocator;
	}

	void EngineRoot::runScript(const char* src, int src_length, const char* path)
	{
		if (m_p_lua_manager)
		{
			m_p_lua_manager->runScript(src, src_length, path);
		}
	}

	lua_State* EngineRoot::getState() { return m_p_lua_manager->getState(); }

	ArchivePathManager& EngineRoot::getArchivePathManager() { return m_path_manager; }
	float EngineRoot::getLastTimeDelta() const { return m_last_time_delta / m_time_multiplier; }
	double EngineRoot::getTime() const { return m_time; }


	EngineRoot* EngineRoot::create(const char* base_path0,
		const char* base_path1,
		FS::FileSystem* fs,
		IAllocator& allocator)
	{
		return _aligned_new(allocator, EngineRoot)(base_path0, base_path1, fs, allocator);
	}

	void EngineRoot::destroy(EngineRoot* engine, IAllocator& allocator)
	{
		_delete(allocator, engine);
	}
}
