#ifndef _camera_h_
#define _camera_h_

#include "common/type.h"
#include "common/math/egal_math.h"

namespace egal
{

#define CAMERA_KEY_FORWARD   UINT8_C(0x01)
#define CAMERA_KEY_BACKWARD  UINT8_C(0x02)
#define CAMERA_KEY_LEFT      UINT8_C(0x04)
#define CAMERA_KEY_RIGHT     UINT8_C(0x08)
#define CAMERA_KEY_UP        UINT8_C(0x10)
#define CAMERA_KEY_DOWN      UINT8_C(0x20)

	struct Camera
	{
		Camera()
		{
			reset();
		}

		~Camera()
		{
		}

		void reset()
		{
			m_mouseNow.x  = 0;
			m_mouseNow.y  = 0;
			m_mouseLast.x = 0;
			m_mouseLast.y = 0;
			m_eye.x = 0.0f;
			m_eye.y = 0.0f;
			m_eye.z = -35.0f;
			m_at.x = 0.0f;
			m_at.y = 0.0f;
			m_at.z = -1.0f;
			m_up.x = 0.0f;
			m_up.y = 1.0f;
			m_up.z = 0.0f;
			m_horizontalAngle = 0.01f;
			m_verticalAngle = 0.0f;
			m_mouseSpeed = 0.0020f;
			m_gamepadSpeed = 0.04f;
			m_moveSpeed = 30.0f;
			m_keys = 0;
			m_mouseDown = false;
		}

		void setKeyState(uint8_t _key, bool _down)
		{
			m_keys &= ~_key;
			m_keys |= _down ? _key : 0;
		}

		void update(e_float _deltaTime, const float2& _mouseState);

		void getViewMtx(float* _viewMtx)
		{
			bx::mtxLookAt(_viewMtx, &m_eye[0], &m_at[0], &m_up[0]);
		}

		void setPosition(const float3& _pos)
		{
			m_eye = _pos;
		}

		void setVerticalAngle(float _verticalAngle)
		{
			m_verticalAngle = _verticalAngle;
		}

		void setHorizontalAngle(float _horizontalAngle)
		{
			m_horizontalAngle = _horizontalAngle;
		}

	public:
		float2 m_mouseNow;
		float2 m_mouseLast;

		float3  m_eye;
		float3  m_at;
		float3  m_up;
		e_float m_horizontalAngle;
		e_float m_verticalAngle;

		e_float m_mouseSpeed;
		e_float m_gamepadSpeed;
		e_float m_moveSpeed;

		uint8_t m_keys;
		e_bool m_mouseDown;
	};
}
#endif // CAMERA_H_HEADER_GUARD

