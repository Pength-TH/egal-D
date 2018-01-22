#ifndef MATHS_INTERNAL_SIMD_MATH_CONFIG_H_
#define MATHS_INTERNAL_SIMD_MATH_CONFIG_H_

#include "common/animation/platform.h"
#define BUILD_SIMD_REF
// Avoid SIMD instruction detection if reference (aka scalar) implementation is
// forced.
#if !defined(BUILD_SIMD_REF)

// Try to match a SSE2+ version.
#if defined(__AVX__) || defined(SIMD_AVX)
#include <immintrin.h>
#define SIMD_AVX
#define SIMD_SSE4_2  // SSE4.2 is available if avx is.
#endif

#if defined(__SSE4_2__) || defined(SIMD_SSE4_2)
#include <nmmintrin.h>
#define SIMD_SSE4_2
#define SIMD_SSE4_1  // SSE4.1 is available if SSE4.2 is.
#endif

#if defined(__SSE4_1__) || defined(SIMD_SSE4_1)
#include <smmintrin.h>
#define SIMD_SSE4_1
#define SIMD_SSSE3  // SSSE3 is available if SSE4.1 is.
#endif

#if defined(__SSSE3__) || defined(SIMD_SSSE3)
#include <tmmintrin.h>
#define SIMD_SSSE3
#define SIMD_SSE3  // SSE3 is available if SSSE3 is.
#endif

#if defined(__SSE3__) || defined(SIMD_SSE3)
#include <pmmintrin.h>
#define SIMD_SSE3
#define SIMD_SSE2  // SSE2 is available if SSE3 is.
#endif

// x64/amd64 have SSE2 instructions
// _M_IX86_FP is 2 if /arch:SSE2, /arch:AVX or /arch:AVX2 was used.
#if defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || \
    (_M_IX86_FP >= 2) || defined(SIMD_SSE2)
#include <emmintrin.h>
#define SIMD_SSE2
#define SIMD_SSEx  // SIMD_SSEx is the generic flag for SSE support
#endif

// End of SIMD instruction detection
#endif  // !BUILD_SIMD_REF

// SEE* intrinsics available
#if defined(SIMD_SSEx)

namespace egal
{
	namespace math
	{

		// Vector of four floating point values.
		typedef __m128 SimdFloat4;

		// Argument type for Float4.
		typedef const __m128 _SimdFloat4;

		// Vector of four integer values.
		typedef __m128i SimdInt4;

		// Argument type for Int4.
		typedef const __m128i _SimdInt4;
	}  // namespace math
}  // namespace egal

#else  // No builtin simd available

// No simd instruction set detected, switch back to reference implementation.
// SIMD_REF is the generic flag for SIMD reference implementation.
#define SIMD_REF

// Declares reference simd float and integer vectors outside of egal::math, in
// order to match non-reference implementation details.

// Vector of four floating point values.
struct SimdFloat4Def
{
	ALIGN(16) float x;
	float y;
	float z;
	float w;
};

// Vector of four integer values.
struct SimdInt4Def
{
	ALIGN(16) int x;
	int y;
	int z;
	int w;
};

namespace egal
{
	namespace math
	{

		// Vector of four floating point values.
		typedef SimdFloat4Def SimdFloat4;

		// Argument type for SimdFloat4
		typedef const SimdFloat4& _SimdFloat4;

		// Vector of four integer values.
		typedef SimdInt4Def SimdInt4;

		// Argument type for SimdInt4.
		typedef const SimdInt4& _SimdInt4;

	}  // namespace math
}  // namespace egal

#endif  // SIMD_x
#endif  // MATHS_INTERNAL_SIMD_MATH_CONFIG_H_
