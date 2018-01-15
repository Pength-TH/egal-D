
#include "common/animation/maths/simd_math.h"

namespace egal
{
	namespace math
	{
		const char* SimdImplementationName()
		{
#if defined(SIMD_AVX)
			return "AVX";
#elif defined(SIMD_SSE4_2)
			return "SSE4.2";
#elif defined(SIMD_SSE4_1)
			return "SSE4.1";
#elif defined(SIMD_SSSE3)
			return "SSSE3";
#elif defined(SIMD_SSE3)
			return "SSE3";
#elif defined(SIMD_SSEx)
			return "SSE";
#elif defined(SIMD_REF)
			return "Reference";
#else
#error No simd_math implementation detected.
#endif
		}

	}  // namespace math
}  // namespace egal
