#ifndef _engine_h_
#define _engine_h_
#pragma once

#include "common/egal-d.h"
#include "common/allocator/lifo_allocator.h"
#include "common/resource/resource_manager.h"

struct lua_State;

namespace egal
{
	class ComponentManager;
	class PluginManager;
	class InputSystem;
	class ArchivePathManager;
	class LuaManager;

	/** 引擎的管理类 */
	class EngineRoot
	{
	public:
		static EngineRoot* create(const char* base_path0,
			const char* base_path1,
			FS::FileSystem* fs,
			IAllocator& allocator);

		/** 卸载引擎,关闭所有系统 */
		static void destroy(EngineRoot* engine, IAllocator& allocator);

	public:
		struct PlatformData
		{
			void* window_handle;
			void* display;
		};

	public:
		EngineRoot(const char* base_path0, const char* base_path1, FS::FileSystem* fs, IAllocator& allocator);
		~EngineRoot();

		void operator=(const EngineRoot&) = delete;
		EngineRoot(const EngineRoot&) = delete;

		ComponentManager& createComponentManager();
		void destroyComponentManager(ComponentManager& context);

		void setPlatformData(const PlatformData& data);
		const PlatformData& getPlatformData();

		void setPatchArchivePath(const char* path);

		InputSystem& getInputSystem();
		PluginManager& getPluginManager();
		ResourceManager& getResourceManager();
		IAllocator& getAllocator();

		void startGame(ComponentManager& context);
		void stopGame(ComponentManager& context);

		Renderer& getRender();

		void frame(ComponentManager& context);
		float getFPS() const;
		
		e_uint32 serialize(ComponentManager& ctx, WriteBinary& serializer);
		bool deserialize(ComponentManager& ctx, ReadBinary& serializer);
		void serializerSceneVersions(WriteBinary& serializer, ComponentManager& ctx);
		void serializePluginList(WriteBinary& serializer);
		bool hasSupportedSceneVersions(ReadBinary& serializer, ComponentManager& ctx);
		bool hasSerializedPlugins(ReadBinary& serializer);

		double getTime() const;
		float getLastTimeDelta() const;
		void setTimeMultiplier(float multiplier);
		void pause(bool pause);
		void nextFrame();
		ArchivePathManager& getArchivePathManager();
		void runScript(const char* src, int src_length, const char* path);

		ComponentUID createComponent(ComponentManager& com_man, GameObject game_object, ComponentType type);
		
		IAllocator& getLIFOAllocator();

		e_void on_mouse_moved(e_float x, e_float y);

		lua_State* getState();
		class Resource* getLuaResource(int idx) const;
		int addLuaResource(const ArchivePath& path, struct ResourceType type);
		void unloadLuaResource(int resource_idx);
	private:
		IAllocator&		m_allocator;
		LIFOAllocator	m_lifo_allocator;

		ComponentManager*	m_p_component_manager;
		SceneManager*		m_p_scene_manager;
		Pipeline*			m_p_pipeline;
		Renderer*			m_p_render;
		ResourceManager		m_resource_manager;
		PluginManager*		m_plugin_manager;
		InputSystem*		m_input_system;

		Timer* m_timer;
		Timer* m_fps_timer;

		int		m_fps_frame;
		float	m_time_multiplier;
		float	m_fps;
		float	m_last_time_delta;
		double	m_time;
		bool	m_is_game_running;
		bool	m_paused;
		bool	m_next_frame;

		PlatformData		m_platform_data;
		ArchivePathManager	m_path_manager;

		LuaManager*			m_p_lua_manager;
		THashMap<int, Resource*>	m_lua_resources;
		int							m_last_lua_resource_idx;

		TVector<FrameBuffer::RenderBuffer> buffers;
		GameObject m_camera;


		e_float  m_mouse_speed;




		GameObject	m_entity;
		Resource*   m_entity_resource;

	};
}
#endif
