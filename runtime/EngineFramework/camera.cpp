#include <bx/timer.h>
#include <bx/math.h>
#include "camera.h"
#include <bx/allocator.h>

namespace egal
{
	void Camera::update(e_float _deltaTime, const float2& _mouseState)
	{
		if (!m_mouseDown)
			m_mouseLast = _mouseState;

		if (m_mouseDown)
			m_mouseNow = _mouseState;

		if (m_mouseDown)
		{
			e_int32 deltaX = m_mouseNow.x - m_mouseLast.x;
			e_int32 deltaY = m_mouseNow.y - m_mouseLast.y;

			m_horizontalAngle += m_mouseSpeed * e_float(deltaX);
			m_verticalAngle   -= m_mouseSpeed * e_float(deltaY);

			m_mouseLast = m_mouseNow;
		}

		m_horizontalAngle += m_gamepadSpeed;
		m_verticalAngle   -= m_gamepadSpeed;

		float direction[3] =
		{
			bx::fcos(m_verticalAngle) * bx::fsin(m_horizontalAngle),
			bx::fsin(m_verticalAngle),
			bx::fcos(m_verticalAngle) * bx::fcos(m_horizontalAngle),
		};

		float right[3] =
		{
			bx::fsin(m_horizontalAngle - bx::kPiHalf),
			0,
			bx::fcos(m_horizontalAngle - bx::kPiHalf),
		};

		float up[3];
		bx::vec3Cross(up, right, direction);

		if (m_keys & CAMERA_KEY_FORWARD)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, direction, _deltaTime * m_moveSpeed);

			bx::vec3Add(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_FORWARD, false);
		}

		if (m_keys & CAMERA_KEY_BACKWARD)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, direction, _deltaTime * m_moveSpeed);

			bx::vec3Sub(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_BACKWARD, false);
		}

		if (m_keys & CAMERA_KEY_LEFT)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, right, _deltaTime * m_moveSpeed);

			bx::vec3Add(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_LEFT, false);
		}

		if (m_keys & CAMERA_KEY_RIGHT)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, right, _deltaTime * m_moveSpeed);

			bx::vec3Sub(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_RIGHT, false);
		}

		if (m_keys & CAMERA_KEY_UP)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, up, _deltaTime * m_moveSpeed);

			bx::vec3Add(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_UP, false);
		}

		if (m_keys & CAMERA_KEY_DOWN)
		{
			float pos[3];
			bx::vec3Move(pos, &m_eye[0]);

			float tmp[3];
			bx::vec3Mul(tmp, up, _deltaTime * m_moveSpeed);

			bx::vec3Sub(&m_eye[0], pos, tmp);
			setKeyState(CAMERA_KEY_DOWN, false);
		}

		bx::vec3Add(&m_at[0], &m_eye[0], direction);
		bx::vec3Cross(&m_up[0], right, direction);
	}


}
