#include "Engine.h"

#include "common/egal-d.h"
#include "common/filesystem/binary.h"

#include "runtime/EngineFramework/scene_manager.h"

#include <bgfx/platform.h>

namespace egal  /** 初始化引擎 */
{
	IEngine* IEngine::create(const egal_params& param)
	{
		IEngine* app = new Engine(param);
		return app;
	}

	/** 卸载引擎,关闭所有系统 */
	void IEngine::destroy(IEngine* engine)
	{
		SAFE_DELETE(engine);
	}
}


namespace egal
{
	Engine::Engine(const egal_params& param)
		: m_allocator(m_main_allocator)
		, m_width(1024)
		, m_height(768)
		, m_async_op(FALSE)
		, m_frame_scale_time(1.0f)
		, m_p_engine_root(nullptr)
	{
		g_allocator = &m_main_allocator;

		/** 先初始化文件系统 */
		init_file_system(m_allocator);
		
		/** logger */
		init_log();
		log_info("egal engine init start...");

		e_char current_dir[MAX_PATH_LENGTH];
#ifdef PLATFORM_WINDOWS
		GetCurrentDirectory(sizeof(current_dir), current_dir);
#else
		current_dir[0] = '\0';
#endif

		m_p_frame_timer = Timer::create(m_allocator);

		log_info("current dir: %s.\n if the dir error, may be too long.", current_dir);

		/** 初始化引擎 相关系统 */
		m_p_engine_root = EngineRoot::create(current_dir, "", g_file_system, m_allocator);
		if (!m_p_engine_root)
		{
			log_error("engine init failed.");
			return;
		}

		/**设置窗口*/
		EngineRoot::PlatformData platform_data = {};
		platform_data.window_handle = param.hWnd;
		m_p_engine_root->setPlatformData(platform_data);

		/**初始化组件管理器*/
		m_p_component_manager = &m_p_engine_root->createComponentManager();

		
		for (int i = 0; i < KC_BOARD_END; i++)
			g_key_board[i] = false;

		for (int i = 0; i < MB_END; i++)
			g_mouse_board[i] = false;
	}

	Engine::~Engine()
	{
		for (int i = 0; i < KC_BOARD_END; i++)
			g_key_board[i] = false;

		for (int i = 0; i < MB_END; i++)
			g_mouse_board[i] = false;

		EngineRoot::destroy(m_p_engine_root, m_allocator);
		Timer::destroy(m_p_frame_timer);
		
		//close_log();
		//destory_file_system(m_allocator);
	}

	e_bool Engine::resize(uint32_t width, uint32_t height)
	{
		if (m_p_engine_root)
			m_p_engine_root->getRender().resize(width, height);
		return TRUE;
	}

	e_bool Engine::run()
	{
		float frame_time = m_p_frame_timer->tick();
		m_p_engine_root->frame(*m_p_component_manager);

		if (frame_time < 1 / 100.0f)
		{
			//PROFILE_BLOCK("sleep");
			MT::sleep(e_uint32(1000 / 100.0f - frame_time * 1000));
		}


		return TRUE;
	}

	e_bool Engine::pause()
	{

		return TRUE;
	}

	e_bool Engine::resume()
	{

		return TRUE;
	}

	bool Engine::on_mouse_moved(double x, double y, double distance)
	{
		m_p_engine_root->on_mouse_moved(x, y);

		return TRUE;
	}

	bool Engine::on_mouse_pressed(const MouseButtonID &mouseid, double distance)
	{
		return TRUE;
	}

	bool Engine::on_mouse_released(const MouseButtonID &mouseid, double distance)
	{

		return TRUE;
	}

	bool Engine::on_key_pressed(unsigned int textid, const KeyCode &keyCode)
	{
		egal::SceneManager* m_p_scene_manager = m_p_engine_root->getSceneManager();
		ComponentHandle cmp_hand = m_p_scene_manager->getCameraInSlot("main");
		GameObject m_camera = m_p_scene_manager->getCameraGameObject(cmp_hand);
		float m_mouse_speed = 1.0;
		//input
		{
			if (keyCode == KC_W)
				m_p_scene_manager->camera_navigate(m_camera, 1.0f, 0, 0, m_mouse_speed);
			if (keyCode == KC_S)
				m_p_scene_manager->camera_navigate(m_camera, -1.0f, 0, 0, m_mouse_speed);
			if (keyCode == KC_A)
				m_p_scene_manager->camera_navigate(m_camera, 0.0f, -1.0f, 0, m_mouse_speed);
			if (keyCode == KC_D)
				m_p_scene_manager->camera_navigate(m_camera, 0.0f, 1.0f, 0, m_mouse_speed);
			if (keyCode == KC_Q)
				m_p_scene_manager->camera_navigate(m_camera, 0, 0, -1.0f, m_mouse_speed);
			if (keyCode == KC_E)
				m_p_scene_manager->camera_navigate(m_camera, 0, 0, 1.0f, m_mouse_speed);
		}
		return TRUE;
	}

	bool Engine::on_key_released(unsigned int textid, const KeyCode &keyCode)
	{

		return TRUE;
	}

	void Engine::update(double timeSinceLastFrame)
	{

	}
}