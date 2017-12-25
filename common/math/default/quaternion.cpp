#include "common/math/default/quaternion.h"
#include "common/math/default/float_x.h"
#include "common/math/default/matrix.h"

#include "common/math/math_const.h"

#include <cmath>

namespace egal
{
	const Quaternion Quaternion::IDENTITY = { 0, 0, 0, 1 };

	Quaternion::AxisAngle Quaternion::getAxisAngle() const
	{
		AxisAngle ret;
		if (fabs(1 - w*w) < 0.00001f)
		{
			ret.angle = 0;
			ret.axis.set(0, 1, 0);
		}
		else
		{
			ret.angle = 2 * acosf(w);
			float tmp = 1 / sqrt(1 - w*w);
			ret.axis.set(x * tmp, y * tmp, z * tmp);
		}
		return ret;
	}


	Quaternion::Quaternion(const float3& axis, float angle)
	{
		float half_angle = angle * 0.5f;
		float s = sinf(half_angle);
		w = cosf(half_angle);
		x = axis.x * s;
		y = axis.y * s;
		z = axis.z * s;
	}


	Quaternion Quaternion::vec3ToVec3(const float3& a, const float3& b)
	{
		float angle = acosf(dotProduct(a, b));
		float3 normal = crossProduct(a, b);
		float normal_len = normal.length();
		return Quaternion(normal_len < 0.001f ? float3(0, 1, 0) : normal * (1 / normal_len), angle);
	}


	void Quaternion::fromEuler(const float3& euler)
	{
		ASSERT(euler.x >= -Math::C_Pi_2 && euler.x <= Math::C_Pi_2);
		float ex = euler.x * 0.5f;
		float ey = euler.y * 0.5f;
		float ez = euler.z * 0.5f;
		float sinX = sinf(ex);
		float cosX = cosf(ex);
		float sinY = sinf(ey);
		float cosY = cosf(ey);
		float sinZ = sinf(ez);
		float cosZ = cosf(ez);

		w = cosY * cosX * cosZ + sinY * sinX * sinZ;
		x = cosY * sinX * cosZ + sinY * cosX * sinZ;
		y = sinY * cosX * cosZ - cosY * sinX * sinZ;
		z = cosY * cosX * sinZ - sinY * sinX * cosZ;
	}


	float3 Quaternion::toEuler() const
	{
		// from urho3d
		float check = 2.0f * (-y * z + w * x);

		if (check < -0.995f)
		{
			return float3(
				-Math::C_Pi * 0.5f, 0.0f, -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)));
		}
		else if (check > 0.995f)
		{
			return float3(
				Math::C_Pi * 0.5f, 0.0f, atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)));
		}
		else
		{
			return float3(asinf(check),
				atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)),
				atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)));
		}
	}


	void Quaternion::conjugate()
	{
		w = -w;
	}


	Quaternion Quaternion::conjugated() const
	{
		return Quaternion(x, y, z, -w);
	}


	void Quaternion::normalize()
	{
		float l = 1 / sqrt(x*x + y*y + z*z + w*w);
		x *= l;
		y *= l;
		z *= l;
		w *= l;
	}

	Quaternion Quaternion::normalized() const
	{
		float l = 1 / sqrt(x * x + y * y + z * z + w * w);
		return Quaternion(x * l, y * l, z * l, w * l);
	}

	void nlerp(const Quaternion& q1, const Quaternion& q2, Quaternion* out, float t)
	{
		float inv = 1.0f - t;
		if (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w < 0) t = -t;
		float ox = q1.x * inv + q2.x * t;
		float oy = q1.y * inv + q2.y * t;
		float oz = q1.z * inv + q2.z * t;
		float ow = q1.w * inv + q2.w * t;
		float l = 1 / sqrt(ox * ox + oy * oy + oz * oz + ow * ow);
		ox *= l;
		oy *= l;
		oz *= l;
		ow *= l;
		out->x = ox;
		out->y = oy;
		out->z = oz;
		out->w = ow;
	}


	Quaternion Quaternion::operator*(const Quaternion& rhs) const
	{
		return Quaternion(w * rhs.x + rhs.w * x + y * rhs.z - rhs.y * z,
			w * rhs.y + rhs.w * y + z * rhs.x - rhs.z * x,
			w * rhs.z + rhs.w * z + x * rhs.y - rhs.x * y,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
	}


	Quaternion Quaternion::operator-() const
	{
		return Quaternion(x, y, z, -w);
	}

	Quaternion Quaternion::operator+(const Quaternion& q) const
	{
		return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w);
	}

	Quaternion Quaternion::operator*(float m) const
	{
		return Quaternion(x * m, y * m, z * m, w * m);
	}

	float3 Quaternion::operator*(const float3& q) const
	{
		return rotate(q);
	}

	Matrix Quaternion::toMatrix() const
	{
		float fx = x + x;
		float fy = y + y;
		float fz = z + z;
		float fwx = fx*w;
		float fwy = fy*w;
		float fwz = fz*w;
		float fxx = fx*x;
		float fxy = fy*x;
		float fxz = fz*x;
		float fyy = fy*y;
		float fyz = fz*y;
		float fzz = fz*z;

		Matrix mtx;
		mtx.m11 = 1.0f - (fyy + fzz);
		mtx.m21 = fxy - fwz;
		mtx.m31 = fxz + fwy;
		mtx.m12 = fxy + fwz;
		mtx.m22 = 1.0f - (fxx + fzz);
		mtx.m32 = fyz - fwx;
		mtx.m13 = fxz - fwy;
		mtx.m23 = fyz + fwx;
		mtx.m33 = 1.0f - (fxx + fyy);

		mtx.m41 = mtx.m42 = mtx.m43 = mtx.m14 = mtx.m24 = mtx.m34 = 0;
		mtx.m44 = 1;
		return mtx;
	}
}
