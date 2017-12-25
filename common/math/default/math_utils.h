#ifndef _math_utils_h_
#define _math_utils_h_
#pragma once

#include "common/math/math_const.h"
#include "common/math/default/quaternion.h"
#include "common/math/default/float_x.h"
#include "common/math/default/matrix.h"
#include "common/math/default/simd.h"

namespace egal
{
	struct float3;
	namespace Math
	{
		e_bool getRayPlaneIntersecion(const float3& origin,
			const float3& dir,
			const float3& plane_point,
			const float3& normal,
			float& out);
		e_bool getRaySphereIntersection(const float3& origin,
			const float3& dir,
			const float3& center,
			float radius,
			float3& out);
		e_bool getRayAABBIntersection(const float3& origin,
			const float3& dir,
			const float3& min,
			const float3& size,
			float3& out);
		float getLineSegmentDistance(const float3& origin,
			const float3& dir,
			const float3& a,
			const float3& b);
		e_bool getRayTriangleIntersection(const float3& origin,
			const float3& dir,
			const float3& a,
			const float3& b,
			const float3& c,
			float* out_t);
		e_bool getSphereTriangleIntersection(const float3& center,
			float radius,
			const float3& v0,
			const float3& v1,
			const float3& v2);

		template <typename T> INLINE void swap(T& a, T& b)
		{
			T tmp = a;
			a = b;
			b = tmp;
		}

		template <typename T> INLINE T minimum(T a)
		{
			return a;
		}

		template <typename T1, typename... T2> INLINE T1 minimum(T1 a, T2... b)
		{
			T1 min_b = minimum(b...);
			return a < min_b ? a : min_b;
		}

		template <typename T> INLINE T maximum(T a)
		{
			return a;
		}

		template <typename T1, typename... T2> INLINE T1 maximum(T1 a, T2... b)
		{
			T1 min_b = maximum(b...);
			return a > min_b ? a : min_b;
		}

		// converts float to uint so it can be used in radix sort
		// float float_value = 0;
		// e_uint32 sort_key = floatFlip(*(e_uint32*)&float_value);
		// http://stereopsis.com/radix.html
		INLINE e_uint32 floatFlip(e_uint32 float_bits_value)
		{
			e_uint32 mask = -e_int32(float_bits_value >> 31) | 0x80000000;
			return float_bits_value ^ mask;
		}

		INLINE float floor(float f)
		{
			return float(int(f));
		}

		template <typename T> INLINE T abs(T a)
		{
			return a > 0 ? a : -a;
		}

		template <typename T> INLINE T signum(T a)
		{
			return a > 0 ? (T)1 : (a < 0 ? (T)-1 : 0);
		}

		template <typename T>
		INLINE T clamp(T value, T min_value, T max_value)
		{
			return minimum(maximum(value, min_value), max_value);
		}

		inline e_uint32 nextPow2(e_uint32 v)
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

		inline e_uint32 log2(e_uint32 v)
		{
			e_uint32 r = (v > 0xffff) << 4; v >>= r;
			e_uint32 shift = (v > 0xff) << 3; v >>= shift; r |= shift;
			shift = (v > 0xf) << 2; v >>= shift; r |= shift;
			shift = (v > 0x3) << 1; v >>= shift; r |= shift;
			r |= (v >> 1);
			return r;
		}

		template <typename T> e_bool isPowOfTwo(T n)
		{
			return (n) && !(n & (n - 1));
		}

		INLINE float degreesToRadians(float angle)
		{
			return angle * C_Pi / 180.0f;
		}

		INLINE double degreesToRadians(double angle)
		{
			return angle * C_Pi / 180.0;
		}

		INLINE float degreesToRadians(int angle)
		{
			return angle * C_Pi / 180.0f;
		}

		float3 degreesToRadians(const float3& v);

		INLINE float radiansToDegrees(float angle)
		{
			return angle / C_Pi * 180.0f;
		}

		float3 radiansToDegrees(const float3& v);

		float angleDiff(float a, float b);

		inline float easeInOut(float t)
		{
			float scaled_t = t * 2;
			if (scaled_t < 1)
			{
				return 0.5f * scaled_t * scaled_t;
			}
			--scaled_t;
			return -0.5f * (scaled_t * (scaled_t - 2) - 1);
		}


		float pow(float base, float exponent);
		e_uint64 randGUID();
		e_uint32 rand();
		e_uint32 rand(e_uint32 from, e_uint32 to);
		void seedRandom(e_uint32 seed);
		float randFloat();
		float randFloat(float from, float to);

	}
}
#endif
