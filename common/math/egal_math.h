#ifndef __egal_math_h__
#define __egal_math_h__
#pragma once

#include "common/type.h"
#if EGAL_MATH == 0
#define MATH_DEFAULT
#elif EGAL_MATH == 1
#define MATH_MATHFU
#endif

#ifdef MATH_DEFAULT
	#include "common/math/default/simd.h"
	#include "common/math/default/float_x.h"
	#include "common/math/default/matrix.h"
	#include "common/math/default/quaternion.h"
	#include "common/math/default/math_utils.h"
#endif

#ifdef MATH_MATHFU
	#include "common/math/mathfu/utilities.h"
	#include "common/math/mathfu/constants.h"
	#include "common/math/mathfu/rect.h"
#endif // MATH_MATHFU

namespace egal
{
#ifdef MATH_DEFAULT
	typedef Matrix			float4x4;
#endif

#ifdef MATH_MATHFU
	typedef mathfu::Vector<e_int32, 2>		Int2;
	typedef mathfu::Vector<e_int32, 3>		Int3;
	typedef mathfu::Vector<e_int32, 4>		Int4;

	typedef mathfu::Vector<e_float, 2>		float2;
	typedef mathfu::Vector<e_float, 3>		float3;
	typedef mathfu::Vector<e_float, 4>		float4;

	typedef mathfu::Matrix<e_float, 2, 2>	float2x2;
	typedef mathfu::Matrix<e_float, 3, 3>	float3x3;
	typedef mathfu::Matrix<e_float, 4, 4>	float4x4;

	typedef mathfu::Quaternion<e_float>		Quaternion;

	typedef mathfu::Rect<e_float>			Rect;


	#define dotProduct(a,b)   mathfu::DotProductHelper(a, b)
	#define crossProduct(a,b) mathfu::CrossProductHelper(a, b)
	#define lerp(a,b,t) mathfu::LerpHelper(a, b, t);


	/** function */
	namespace Math
	{
		template <typename T> e_bool isPowOfTwo(T n)
		{
			return (n) && !(n & (n - 1));
		}

		template <typename T> T minimum(T a)
		{
			return a;
		}

		template <typename T1, typename... T2> T1 minimum(T1 a, T2... b)
		{
			T1 min_b = minimum(b...);
			return a < min_b ? a : min_b;
		}

		template <typename T> T maximum(T a)
		{
			return a;
		}

		template <typename T1, typename... T2> T1 maximum(T1 a, T2... b)
		{
			T1 min_b = maximum(b...);
			return a > min_b ? a : min_b;
		}

		// converts e_float to uint so it can be used in radix sort
		// e_float float_value = 0;
		// e_uint32 sort_key = floatFlip(*(e_uint32*)&float_value);
		// http://stereopsis.com/radix.html

		INLINE e_uint32 floatFlip(e_uint32 float_bits_value)
		{
			e_uint32 mask = -e_int32(float_bits_value >> 31) | 0x80000000;
			return float_bits_value ^ mask;
		}

		INLINE e_float floor(e_float f)
		{
			return e_float(e_int32(f));
		}

		template <typename T> T abs(T a)
		{
			return a > 0 ? a : -a;
		}

		template <typename T> T signum(T a)
		{
			return a > 0 ? (T)1 : (a < 0 ? (T)-1 : 0);
		}

		template <typename T>
		T clamp(T value, T min_value, T max_value)
		{
			return minimum(maximum(value, min_value), max_value);
		}

		INLINE e_uint32 nextPow2(e_uint32 v)
		{
			v--;
			v |= v >> 1;
			v |= v >> 2;
			v |= v >> 4;
			v |= v >> 8;
			v |= v >> 16;
			v++;
			return v;
		}

		INLINE e_uint32 log2(e_uint32 v)
		{
			e_uint32 r = (v > 0xffff) << 4; v >>= r;
			e_uint32 shift = (v > 0xff) << 3; v >>= shift; r |= shift;
			shift = (v > 0xf) << 2; v >>= shift; r |= shift;
			shift = (v > 0x3) << 1; v >>= shift; r |= shift;
			r |= (v >> 1);
			return r;
		}

		INLINE e_float degreesToRadians(e_float angle)
		{
			return angle * Math::C_DegreeToRadian;
		}

		INLINE e_double degreesToRadians(e_double angle)
		{
			return angle * Math::C_DegreeToRadian;
		}

		INLINE e_float degreesToRadians(e_int32 angle)
		{
			return angle * Math::C_DegreeToRadian;
		}

		float3 degreesToRadians(const float3& v);

		INLINE e_float radiansToDegrees(e_float angle)
		{
			return angle / Math::C_RadianToDegree;
		}

		float3 radiansToDegrees(const float3& v);

		e_float angleDiff(e_float a, e_float b);

		INLINE e_float easeInOut(e_float t)
		{
			e_float scaled_t = t * 2;
			if (scaled_t < 1)
			{
				return 0.5f * scaled_t * scaled_t;
			}
			--scaled_t;
			return -0.5f * (scaled_t * (scaled_t - 2) - 1);
		}


		e_float pow(e_float base, e_float exponent);

		e_uint64 randGUID();

		e_uint32 rand();

		e_uint32 rand(e_uint32 from, e_uint32 to);

		void seedRandom(e_uint32 seed);

		e_float randFloat();

		e_float randFloat(e_float from, e_float to);

		INLINE float3 Quaternion_rotate(const Quaternion& qv, const float3& v)
		{
			float3 qvec = qv.vector();
			float3 uv = crossProduct(qvec, v);
			float3 uuv = crossProduct(qvec, uv);
			uv *= (2.0f * qv.scalar());
			uuv *= 2.0f;

			return v + uv + uuv;
		}

		INLINE Quaternion nlerp(const Quaternion& _q1, const Quaternion& _q2, e_float t)
		{
			float4 q1 = float4(_q1.vector(), _q1.scalar());
			float4 q2 = float4(_q2.vector(), _q2.scalar());

			e_float inv = 1.0f - t;
			if (q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w < 0) t = -t;
			e_float ox = q1.x * inv + q2.x * t;
			e_float oy = q1.y * inv + q2.y * t;
			e_float oz = q1.z * inv + q2.z * t;
			e_float ow = q1.w * inv + q2.w * t;
			e_float l = 1 / sqrt(ox * ox + oy * oy + oz * oz + ow * ow);
			ox *= l;
			oy *= l;
			oz *= l;
			ow *= l;

			return Quaternion(ow, ox, oy, oz);
		}
	}

	struct RigidTransform;
	/*transform */
	struct Transform
	{
		Transform() {}
		Transform(const float3& _pos, const Quaternion& _rot, e_float _scale)
			: pos(_pos)
			, rot(_rot)
			, scale(_scale)
		{
		}

		Transform inverted() const
		{
			Transform result;
			result.rot.set_scalar(-result.rot.scalar());
			result.pos = Math::Quaternion_rotate(result.rot, (-pos / scale));
			result.scale = 1.0f / scale;
			return result;
		}

		Transform operator*(const Transform& rhs) const
		{
			return { Math::Quaternion_rotate(rot, rhs.pos * scale) + pos, rot * rhs.rot, scale * rhs.scale };
		}

		float3 transform(const float3& value) const
		{
			return pos + Math::Quaternion_rotate(rot, value) * scale;
		}

		RigidTransform getRigidPart() const;

		float4x4 tofloat4x4() const
		{
			float3x3 mtx = rot.ToMatrix();
			float3 tranpos = mtx.TranslationVector3D() + pos;
			mtx = mtx * scale;

			//need to check
			return float4x4(mtx(0, 0), mtx(1, 0), mtx(2, 0), 0,
				mtx(0, 1), mtx(1, 1), mtx(2, 1), 0,
				mtx(0, 2), mtx(1, 2), mtx(2, 2), 0,
				tranpos.x, tranpos.y, tranpos.z, 0
			);
		}

		Quaternion	rot;
		float3		pos;
		e_float		scale;
	};

	/** physic transform */
	struct RigidTransform
	{
		RigidTransform() {}
		RigidTransform(const float3& _pos, const Quaternion& _rot)
			: pos(_pos)
			, rot(_rot)
		{
		}

		RigidTransform inverted() const
		{
			RigidTransform result;
			result.rot.set_scalar(-result.rot.scalar());
			result.pos = Math::Quaternion_rotate(result.rot, -pos);
			return result;
		}

		RigidTransform operator*(const RigidTransform& rhs) const
		{
			return{ Math::Quaternion_rotate(rot, rhs.pos) + pos, rot * rhs.rot };
		}

		float3 transform(const float3& value) const
		{
			return pos + Math::Quaternion_rotate(rot, value);
		}

		RigidTransform interpolate(const RigidTransform& rhs, e_float t)
		{
			RigidTransform ret;

			ret.pos = lerp(pos, rhs.pos, t);
			// check it
			ret.rot = Math::nlerp(rot, rhs.rot, t);
			return ret;
		}

		inline Transform toScaled(e_float scale) const
		{
			return Transform(pos, rot, scale);
		}

		float4x4 toFloat4x4() const
		{
			float3x3 mtx = rot.ToMatrix();
			float3 tranpos = mtx.TranslationVector3D() + pos;
			//need to check
			return float4x4(mtx(0, 0), mtx(1, 0), mtx(2, 0), 0,
				mtx(0, 1), mtx(1, 1), mtx(2, 1), 0,
				mtx(0, 2), mtx(1, 2), mtx(2, 2), 0,
				tranpos.x, tranpos.y, tranpos.z, 0);
		}

		Quaternion rot;
		float3 pos;
	};
#endif

	static bool RealEqual(float a, float b,
		float tolerance = std::numeric_limits<float>::epsilon())
	{
		if (fabs(b - a) <= tolerance)
			return true;
		else
			return false;
	}
}

#endif
