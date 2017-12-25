#ifndef _simd_h_
#define _simd_h_
#pragma once
#include "common/type.h"
#include "common/define.h"
#include "common/math/default/float_x.h"

#ifdef _WIN32
	#include <xmmintrin.h>
#else
	#include <cmath>
#endif

namespace egal
{
#ifdef _WIN32
	typedef __m128 simd4;

	INLINE simd4 f4LoadUnaligned(const void* src)
	{
		return _mm_loadu_ps((const float*)(src));
	}

	INLINE simd4 f4Load(const void* src)
	{
		return _mm_load_ps((const float*)(src));
	}


	INLINE simd4 f4Splat(float value)
	{
		return _mm_set_ps1(value);
	}


	INLINE void f4Store(void* dest, simd4 src)
	{
		_mm_store_ps((float*)dest, src);
	}


	INLINE int f4MoveMask(simd4 a)
	{
		return _mm_movemask_ps(a);
	}


	INLINE simd4 f4Add(simd4 a, simd4 b)
	{
		return _mm_add_ps(a, b);
	}


	INLINE simd4 f4Sub(simd4 a, simd4 b)
	{
		return _mm_sub_ps(a, b);
	}


	INLINE simd4 f4Mul(simd4 a, simd4 b)
	{
		return _mm_mul_ps(a, b);
	}


	INLINE simd4 f4Div(simd4 a, simd4 b)
	{
		return _mm_div_ps(a, b);
	}


	INLINE simd4 f4Rcp(simd4 a)
	{
		return _mm_rcp_ps(a);
	}


	INLINE simd4 f4Sqrt(simd4 a)
	{
		return _mm_sqrt_ps(a);
	}


	INLINE simd4 f4Rsqrt(simd4 a)
	{
		return _mm_rsqrt_ps(a);
	}


	INLINE simd4 f4Min(simd4 a, simd4 b)
	{
		return _mm_min_ps(a, b);
	}


	INLINE simd4 f4Max(simd4 a, simd4 b)
	{
		return _mm_max_ps(a, b);
	}

#else 
	struct simd4
	{
		float x, y, z, w;
	};


	INLINE simd4 f4LoadUnaligned(const void* src)
	{
		return *(const simd4*)src;
	}


	INLINE simd4 f4Load(const void* src)
	{
		return *(const simd4*)src;
	}


	INLINE simd4 f4Splat(float value)
	{
		return { value, value, value, value };
	}


	INLINE void f4Store(void* dest, simd4 src)
	{
		(*(simd4*)dest) = src;
	}


	INLINE int f4MoveMask(simd4 a)
	{
		return (a.w < 0 ? (1 << 3) : 0) |
			(a.z < 0 ? (1 << 2) : 0) |
			(a.y < 0 ? (1 << 1) : 0) |
			(a.x < 0 ? 1 : 0);
	}


	INLINE simd4 f4Add(simd4 a, simd4 b)
	{
		return{
			a.x + b.x,
			a.y + b.y,
			a.z + b.z,
			a.w + b.w
		};
	}


	INLINE simd4 f4Sub(simd4 a, simd4 b)
	{
		return{
			a.x - b.x,
			a.y - b.y,
			a.z - b.z,
			a.w - b.w
		};
	}


	INLINE simd4 f4Mul(simd4 a, simd4 b)
	{
		return{
			a.x * b.x,
			a.y * b.y,
			a.z * b.z,
			a.w * b.w
		};
	}


	INLINE simd4 f4Div(simd4 a, simd4 b)
	{
		return{
			a.x / b.x,
			a.y / b.y,
			a.z / b.z,
			a.w / b.w
		};
	}


	INLINE simd4 f4Rcp(simd4 a)
	{
		return{
			1 / a.x,
			1 / a.y,
			1 / a.z,
			1 / a.w
		};
	}


	INLINE simd4 f4Sqrt(simd4 a)
	{
		return{
			(float)sqrt(a.x),
			(float)sqrt(a.y),
			(float)sqrt(a.z),
			(float)sqrt(a.w)
		};
	}


	INLINE simd4 f4Rsqrt(simd4 a)
	{
		return{
			1 / (float)sqrt(a.x),
			1 / (float)sqrt(a.y),
			1 / (float)sqrt(a.z),
			1 / (float)sqrt(a.w)
		};
	}


	INLINE simd4 f4Min(simd4 a, simd4 b)
	{
		return{
			a.x < b.x ? a.x : b.x,
			a.y < b.y ? a.y : b.y,
			a.z < b.z ? a.z : b.z,
			a.w < b.w ? a.w : b.w
		};
	}


	INLINE simd4 f4Max(simd4 a, simd4 b)
	{
		return{
			a.x > b.x ? a.x : b.x,
			a.y > b.y ? a.y : b.y,
			a.z > b.z ? a.z : b.z,
			a.w > b.w ? a.w : b.w
		};
	}

#endif
}

#endif