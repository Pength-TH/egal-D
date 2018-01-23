#include <bx/timer.h>
#include <bx/math.h>
#include "camera.h"
#include <bx/allocator.h>

namespace egal
{
	void Camera::update(e_float _deltaTime, const float2& _mouseState)
	{
		m_deltaTime = _deltaTime;

		m_camera_direction[0] = bx::fcos(m_verticalAngle) * bx::fsin(m_horizontalAngle);
		m_camera_direction[1] = bx::fsin(m_verticalAngle);
		m_camera_direction[2] = bx::fcos(m_verticalAngle) * bx::fcos(m_horizontalAngle);

		m_camera_right[0] = bx::fsin(m_horizontalAngle - bx::kPiHalf);
		m_camera_right[1] = 0;
		m_camera_right[2] = bx::fcos(m_horizontalAngle - bx::kPiHalf);

		bx::vec3Cross(m_camera_up, m_camera_right, m_camera_direction);

		bx::vec3Add(m_at, m_eye, m_camera_direction);
		bx::vec3Cross(m_up, m_camera_right, m_camera_direction);
	}

	void Camera::forward()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_direction, m_deltaTime * m_moveSpeed);

		bx::vec3Add(&m_eye[0], pos, tmp);
	}
	void Camera::backward()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_direction, m_deltaTime * m_moveSpeed);

		bx::vec3Sub(&m_eye[0], pos, tmp);
	}
	void Camera::left()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_right, m_deltaTime * m_moveSpeed);

		bx::vec3Add(&m_eye[0], pos, tmp);
	}
	void Camera::right()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_right, m_deltaTime * m_moveSpeed);

		bx::vec3Sub(&m_eye[0], pos, tmp);
	}
	void Camera::up()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_up, m_deltaTime * m_moveSpeed);

		bx::vec3Add(&m_eye[0], pos, tmp);
	}
	void Camera::down()
	{
		float pos[3];
		bx::vec3Move(pos, &m_eye[0]);

		float tmp[3];
		bx::vec3Mul(tmp, m_camera_up, m_deltaTime * m_moveSpeed);

		bx::vec3Sub(&m_eye[0], pos, tmp);
	}
}


