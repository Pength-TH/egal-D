#ifndef _editor_key_h_
#define _editor_key_h_

#include <windows.h>

#include "common.h"
#include "entry/cmd.h"
#include "entry/input.h"

#include "common/type.h"
#include "common/input/framework_listener.h"
namespace egal
{

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)

	e_uint8  g_keys;
	void setKeyState(uint8_t _key, bool _down)
	{
		g_keys &= ~_key;
		g_keys |= _down ? _key : 0;
	}

	HWND g_first_hand;

	int cmdMove(CmdContext* /*_context*/, void* /*_userData*/, int _argc, char const* const* _argv)
	{
		if (_argc > 1)
		{
			if (0 == bx::strCmp(_argv[1], "forward"))
			{
				setKeyState(CAMERA_KEY_FORWARD, true);
				return 0;
			}
			else if (0 == bx::strCmp(_argv[1], "left"))
			{
				setKeyState(CAMERA_KEY_LEFT, true);
				return 0;
			}
			else if (0 == bx::strCmp(_argv[1], "right"))
			{
				setKeyState(CAMERA_KEY_RIGHT, true);
				return 0;
			}
			else if (0 == bx::strCmp(_argv[1], "backward"))
			{
				setKeyState(CAMERA_KEY_BACKWARD, true);
				return 0;
			}
			else if (0 == bx::strCmp(_argv[1], "up"))
			{
				setKeyState(CAMERA_KEY_UP, true);
				return 0;
			}
			else if (0 == bx::strCmp(_argv[1], "down"))
			{
				setKeyState(CAMERA_KEY_DOWN, true);
				return 0;
			}
		}
		return 1;
	}

	static void cmd(const void* _userData)
	{
		cmdExec((const char*)_userData);
	}

	static const InputBinding s_camBindings[] =
	{
		{ entry::Key::KeyW,             entry::Modifier::None, 0, cmd, "move forward" },
		{ entry::Key::GamepadUp,        entry::Modifier::None, 0, cmd, "move forward" },
		{ entry::Key::KeyA,             entry::Modifier::None, 0, cmd, "move left" },
		{ entry::Key::GamepadLeft,      entry::Modifier::None, 0, cmd, "move left" },
		{ entry::Key::KeyS,             entry::Modifier::None, 0, cmd, "move backward" },
		{ entry::Key::GamepadDown,      entry::Modifier::None, 0, cmd, "move backward" },
		{ entry::Key::KeyD,             entry::Modifier::None, 0, cmd, "move right" },
		{ entry::Key::GamepadRight,     entry::Modifier::None, 0, cmd, "move right" },
		{ entry::Key::KeyQ,             entry::Modifier::None, 0, cmd, "move down" },
		{ entry::Key::GamepadShoulderL, entry::Modifier::None, 0, cmd, "move down" },
		{ entry::Key::KeyE,             entry::Modifier::None, 0, cmd, "move up" },
		{ entry::Key::GamepadShoulderR, entry::Modifier::None, 0, cmd, "move up" },

		INPUT_BINDING_END
	};

	uint32_t			m_width;
	uint32_t			m_height;
	uint32_t			m_debug;
	uint32_t			m_reset;
	e_float             m_move_speed = 0.05f;
	e_float				m_gamepadSpeed;
	e_float				m_mouseSpeed = 0.5f;
	e_float				m_verticalAngle;
	e_float				m_horizontalAngle;
	MouseCoords			m_mouseNow;
	MouseCoords			m_mouseLast;
	bool				m_mouseDown;
	entry::MouseState	_mouseState;

	bool key_update()
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &_mouseState))
		{
			if (!m_mouseDown)
			{
				m_mouseLast.m_mx = _mouseState.m_mx;
				m_mouseLast.m_my = _mouseState.m_my;
			}

			m_mouseDown = !!_mouseState.m_buttons[entry::MouseButton::Right];

			if (m_mouseDown)
			{
				m_mouseNow.m_mx = _mouseState.m_mx;
				m_mouseNow.m_my = _mouseState.m_my;
			}

			if (m_mouseDown)
			{
				int32_t deltaX = m_mouseNow.m_mx - m_mouseLast.m_mx;
				int32_t deltaY = m_mouseNow.m_my - m_mouseLast.m_my;

				m_horizontalAngle += m_mouseSpeed * float(deltaX);
				m_verticalAngle -= m_mouseSpeed * float(deltaY);

				m_mouseLast.m_mx = m_mouseNow.m_mx;
				m_mouseLast.m_my = m_mouseNow.m_my;
			}

			entry::GamepadHandle handle = { 0 };
			m_horizontalAngle += m_gamepadSpeed * inputGetGamepadAxis(handle, entry::GamepadAxis::RightX) / 32768.0f;
			m_verticalAngle -= m_gamepadSpeed * inputGetGamepadAxis(handle, entry::GamepadAxis::RightY) / 32768.0f;

			const int32_t gpx = inputGetGamepadAxis(handle, entry::GamepadAxis::LeftX);
			const int32_t gpy = inputGetGamepadAxis(handle, entry::GamepadAxis::LeftY);

			g_keys |= gpx < -16834 ? CAMERA_KEY_LEFT : 0;
			g_keys |= gpx > 16834 ? CAMERA_KEY_RIGHT : 0;
			g_keys |= gpy < -16834 ? CAMERA_KEY_FORWARD : 0;
			g_keys |= gpy > 16834 ? CAMERA_KEY_BACKWARD : 0;
			return true;
		}
		return false;
	}

	void register_key()
	{
		cmdAdd("move", cmdMove);
		inputAddBindings("camBindings", s_camBindings);
	}
}

#endif
