#include "editor.h"
#include "runtime/IEngine.h"
#include "runtime/Engine.h"
#include "editor/common/entry/cmd.h"
#include "editor/editor_key.h"

#include "imgui/bgfx_imgui.h"
#include "ocornut-imgui/imgui.h"

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
		}

		if (m_reset)
		{
			m_p_engine->resize(m_width, m_height);
			m_reset = false;
		}

		if (g_keys & CAMERA_KEY_FORWARD)
		{
			m_p_engine->on_key_pressed(0, KC_W);
			setKeyState(CAMERA_KEY_FORWARD, false);
			m_p_engine->on_key_released(0, KC_W);
		}
		if (g_keys & CAMERA_KEY_BACKWARD)
		{
			m_p_engine->on_key_pressed(0, KC_S);
			setKeyState(CAMERA_KEY_BACKWARD, false);
			m_p_engine->on_key_released(0, KC_S);
		}

		if (g_keys & CAMERA_KEY_LEFT)
		{
			m_p_engine->on_key_pressed(0, KC_A);
			setKeyState(CAMERA_KEY_LEFT, false);
			m_p_engine->on_key_released(0, KC_A);
		}

		if (g_keys & CAMERA_KEY_RIGHT)
		{
			m_p_engine->on_key_pressed(0, KC_D);
			setKeyState(CAMERA_KEY_RIGHT, false);
			m_p_engine->on_key_released(0, KC_D);
		}

		if (g_keys & CAMERA_KEY_UP)
		{
			m_p_engine->on_key_pressed(0, KC_Q);
			setKeyState(CAMERA_KEY_UP, false);
			m_p_engine->on_key_released(0, KC_Q);
		}

		if (g_keys & CAMERA_KEY_DOWN)
		{
			m_p_engine->on_key_pressed(0, KC_E);
			setKeyState(CAMERA_KEY_DOWN, false);
			m_p_engine->on_key_released(0, KC_E);
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

		imguiEndFrame();

		m_p_engine->run();

		return onUpdate;
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

