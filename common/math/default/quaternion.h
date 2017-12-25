#ifndef _quation_h_
#define _quation_h_
#pragma once

#include "common/type.h"
#include "common/define.h"
#include "common/math/default/float_x.h"

namespace egal
{
	struct Matrix;
	struct Quaternion
	{
		struct AxisAngle
		{
			float3 axis;
			float angle;
		};

		Quaternion() {}
		Quaternion(const float3& axis, float angle);
		Quaternion(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }

		void fromEuler(const float3& euler);
		float3 toEuler() const;
		AxisAngle getAxisAngle() const;
		void set(float _x, float _y, float _z, float _w) { x = _x; y = _y; z = _z; w = _w; }
		void conjugate();
		Quaternion conjugated() const;
		void normalize();
		Quaternion normalized() const;
		Matrix toMatrix() const;

		float3 rotate(const float3& v) const
		{
			// nVidia SDK implementation

			float3 qvec(x, y, z);
			float3 uv = crossProduct(qvec, v);
			float3 uuv = crossProduct(qvec, uv);
			uv *= (2.0f * w);
			uuv *= 2.0f;

			return v + uv + uuv;
		}

		Quaternion operator*(const Quaternion& q) const;
		Quaternion operator-() const;
		Quaternion operator+(const Quaternion& q) const;
		Quaternion operator*(float m) const;
		float3 operator*(const float3& q) const;

		static Quaternion vec3ToVec3(const float3& a, const float3& b);

		float x, y, z, w;

		static const Quaternion IDENTITY;
	};

	 void nlerp(const Quaternion& q1, const Quaternion& q2, Quaternion* out, float t);
}

#endif
