#include "editor.h"
#include "runtime/IEngine.h"
#include "runtime/app.h"
#include "editor/common/entry/cmd.h"
#include "editor/editor_key.h"

#include "imgui/bgfx_imgui.h"
#include "ocornut-imgui/imgui.h"

#include "runtime/EngineFramework/scene_manager.h"

void winSetHwnd(HWND _window)
{
	egal::g_first_hand = _window;
}

namespace egal
{
	Editor::Editor(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
		register_key();

		egal_params param;
		param.hWnd = g_first_hand;
		m_p_engine = IEngine::create(param);
		m_p_log_ui = new LogUI(*g_allocator); //delete
		imguiCreate();
	}

	void Editor::init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height)
	{
		//init camera
		ComponentManager* m_component = ComponentManager::getSingletonPtr();
		m_camera_object = m_component->createGameObject(float3(0, 1.48, 2.88), Quaternion(0, 0, 0, 1));
		m_component->setGameObjectName(m_camera_object, "main");
		auto camera_handle = SceneManager::getSingletonPtr()->createCamera(m_camera_object);
		SceneManager::getSingletonPtr()->setCameraSlot(camera_handle, "main");

		//create template resource
		{
			for (int i = 0; i < 50; i++)
			{
				for (int j = 0; j < 50; j++)
				{
					auto m_entity = m_component->createGameObject(float3(i * 3, 0, j * 2), Quaternion(0, 0, 0, 1));
					auto mesh_back_cmp = SceneManager::getSingletonPtr()->createComponent(COMPONENT_ENTITY_INSTANCE_TYPE, m_entity);
					SceneManager::getSingletonPtr()->setEntityInstancePath(mesh_back_cmp, ArchivePath("test/wsg_bs_taidaonv_001.msh"));
				}
			}
			//for (int i = 0; i < 110; i++)
			//{
			//	for (int j = 0; j < 110; j++)
			//	{
			//		auto m_entity = m_component->createGameObject(float3(i, 5, j), Quaternion(0, 0, 0, 1));
			//		auto mesh_back_cmp = SceneManager::getSingletonPtr()->createComponent(COMPONENT_ENTITY_INSTANCE_TYPE, m_entity);
			//		SceneManager::getSingletonPtr()->setEntityInstancePath(mesh_back_cmp, ArchivePath("test/wsg_bs_taidaonv_001.msh"));
			//	}
			//}
			//for (int i = 0; i < 110; i++)
			//{
			//	for (int j = 0; j < 110; j++)
			//	{
			//		auto m_entity = m_component->createGameObject(float3(i, 10, j), Quaternion(0, 0, 0, 1));
			//		auto mesh_back_cmp = SceneManager::getSingletonPtr()->createComponent(COMPONENT_ENTITY_INSTANCE_TYPE, m_entity);
			//		SceneManager::getSingletonPtr()->setEntityInstancePath(mesh_back_cmp, ArchivePath("test/wsg_bs_taidaonv_001.msh"));
			//	}
			//}

			while (g_file_system->hasWork())
			{
				MT::sleep(100);
				g_file_system->updateAsyncTransactions();
			}
		}

		m_p_import_assert = new ImportAssetDialog(*g_allocator); //delete
	}

	int Editor::shutdown()
	{
		imguiDestroy();

		IEngine::destroy(m_p_engine);
		return 0;
	}

	bool Editor::update()
	{
		bool onUpdate = key_update();

		if (onUpdate)
		{
			m_p_engine->on_mouse_moved(_mouseState.m_mx, _mouseState.m_my, _mouseState.m_mz);
			camera_unity_update();
		}

		if (m_reset)
		{
			m_p_engine->resize(m_width, m_height);
			m_reset = false;
		}

		// Draw UI
		imguiBeginFrame(_mouseState.m_mx
			, _mouseState.m_my
			, (_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0)
			| (_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0)
			| (_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
			, _mouseState.m_mz
			, uint16_t(m_width)
			, uint16_t(m_height)
		);

		//for (auto window : m_windows)
		//{
		//	window.onWindowGUI();
		//	window.update();
		//}

		m_p_import_assert->onWindowGUI();
		m_p_import_assert->update(0.1);
		m_p_log_ui->onGUI();


		bool p_open = true;
		ImGui::Begin("overlay", &p_open, ImVec2(300, 200), 0.3, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetWindowPos("overlay", ImVec2(io.DisplaySize.x - ImGui::GetWindowWidth() - 2, 20));

		if (ImGui::TreeNode("Camera"))
		{
			float3 pos = ComponentManager::getSingletonPtr()->getPosition(m_camera_object);
			Quaternion qu = ComponentManager::getSingletonPtr()->getRotation(m_camera_object);
			ImGui::Text("fps:%.2f", EngineRoot::getSingletonPtr()->getFPS());
			ImGui::Text("cam  pos:%.2f %.2f %.2f", pos.x, pos.y, pos.z);
			ImGui::Text("cam roat:%.2f %.2f %.2f %.2f", qu.x, qu.y, qu.z, qu.w);
			ImGui::DragFloat("Camera Speed:", &m_move_speed, 0.01, 0.0, 3.0);
			ImGui::Separator();
			ImGui::TreePop();
		}
		ImGui::End();

		imguiEndFrame();

		m_p_engine->run();

		return onUpdate;
	}


	void Editor::camera_unity_update()
	{
		if (g_keys & CAMERA_KEY_FORWARD)
		{
			m_p_engine->on_key_pressed(0, KC_W);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, 1.0f, 0, 0, m_move_speed);
			setKeyState(CAMERA_KEY_FORWARD, false);
			m_p_engine->on_key_released(0, KC_W);
		}
		if (g_keys & CAMERA_KEY_BACKWARD)
		{
			m_p_engine->on_key_pressed(0, KC_S);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, -1.0f, 0, 0, m_move_speed);
			setKeyState(CAMERA_KEY_BACKWARD, false);
			m_p_engine->on_key_released(0, KC_S);
		}

		if (g_keys & CAMERA_KEY_LEFT)
		{
			m_p_engine->on_key_pressed(0, KC_A);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, 0.0f, -1.0f, 0, m_move_speed);
			setKeyState(CAMERA_KEY_LEFT, false);
			m_p_engine->on_key_released(0, KC_A);
		}

		if (g_keys & CAMERA_KEY_RIGHT)
		{
			m_p_engine->on_key_pressed(0, KC_D);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, 0.0f, 1.0f, 0, m_move_speed);
			setKeyState(CAMERA_KEY_RIGHT, false);
			m_p_engine->on_key_released(0, KC_D);
		}

		if (g_keys & CAMERA_KEY_UP)
		{
			m_p_engine->on_key_pressed(0, KC_Q);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, 0, 0, -1.0f, m_move_speed);
			setKeyState(CAMERA_KEY_UP, false);
			m_p_engine->on_key_released(0, KC_Q);
		}

		if (g_keys & CAMERA_KEY_DOWN)
		{
			m_p_engine->on_key_pressed(0, KC_E);
			SceneManager::getSingletonPtr()->camera_navigate(m_camera_object, 0, 0, 1.0f, m_move_speed);
			setKeyState(CAMERA_KEY_DOWN, false);
			m_p_engine->on_key_released(0, KC_E);
		}

		if (m_mouseDown)
			SceneManager::getSingletonPtr()->camera_rotate(m_camera_object, m_horizontalAngle, m_verticalAngle);

	}

	void Editor::createWindow()
	{
		entry::WindowHandle handle = entry::createWindow(rand() % 1280, rand() % 720, 640, 480);
		if (entry::isValid(handle))
		{
			char str[256];
			bx::snprintf(str, BX_COUNTOF(str), "Window - handle %d", handle.idx);
			entry::setWindowTitle(handle, str);
			m_windows[handle.idx].m_handle = handle;
		}
	}

	void Editor::destroyWindow()
	{
		for (uint32_t ii = 0; ii < 50; ++ii)
		{
			if (bgfx::isValid(m_fbh[ii]))
			{
				bgfx::destroy(m_fbh[ii]);
				m_fbh[ii].idx = bgfx::kInvalidHandle;

				// Flush destruction of swap chain before destroying window!
				bgfx::frame();
				bgfx::frame();
			}

			if (entry::isValid(m_windows[ii].m_handle))
			{
				entry::destroyWindow(m_windows[ii].m_handle);
				m_windows[ii].m_handle.idx = UINT16_MAX;
				return;
			}
		}
	}
}

