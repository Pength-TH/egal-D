#ifndef _camera_h_
#define _camera_h_

#include "common/egal-d.h"

#include <bx/bx.h>
#include <bx/math.h>

namespace egal
{
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
			m_eye[0] = 0.0f;
			m_eye[1] = 0.0f;
			m_eye[2] = -35.0f;
			m_at[0] = 0.0f;
			m_at[1] = 0.0f;
			m_at[2] = -1.0f;
			m_up[0] = 0.0f;
			m_up[1] = 1.0f;
			m_up[2] = 0.0f;
			m_horizontalAngle = 0.01f;
			m_verticalAngle = 0.0f;
			m_mouseSpeed = 0.0020f;
			m_gamepadSpeed = 0.04f;
			m_moveSpeed = 1.0f;
		}

		void update(e_float _deltaTime, const float2& _mouseState);

		/** camera move */
		void forward();
		void backward();
		void left();
		void right();
		void up();
		void down();

		void getViewMtx(float* _viewMtx)
		{
			bx::mtxLookAt(_viewMtx, &m_eye[0], &m_at[0], &m_up[0]);
		}

		void setPosition(const float3& _pos)
		{
			m_eye[0] = _pos.x;
			m_eye[1] = _pos.y;
			m_eye[2] = _pos.z;
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
		GameObject	game_object;
		e_float		fov;
		e_float		aspect;
		e_float		near_flip;
		e_float		far_flip;
		e_float		ortho_size;
		e_float		screen_width;
		e_float		screen_height;
		e_bool		is_ortho;
		e_char		slot[MAX_SLOT_LENGTH + 1];

		e_float		m_eye[3];
		e_float		m_at[3];
		e_float		m_up[3];

		e_float		m_camera_direction[3];
		e_float		m_camera_right[3];
		e_float		m_camera_up[3];

		e_float     m_deltaTime;

		e_float		m_horizontalAngle;
		e_float		m_verticalAngle;

		e_float		m_mouseSpeed;
		e_float		m_gamepadSpeed;
		e_float		m_moveSpeed;
	};
}
#endif 
