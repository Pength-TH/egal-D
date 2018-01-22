
#ifndef MATHS_MATH_EX_H_
#define MATHS_MATH_EX_H_

#include <cassert>
#include <cmath>
#include "common/animation/platform.h"

namespace egal
{
	namespace math
	{
		// Returns the linear interpolation of _a and _b with coefficient _f.
		// _f is not limited to range [0,1].
		INLINE float Lerp(float _a, float _b, float _f)
		{
			return (_b - _a) * _f + _a;
		}

		// Returns the minimum of _a and _b. Comparison's based on operator <.
		template <typename _Ty>
		INLINE _Ty _Min(_Ty _a, _Ty _b)
		{
			return (_a < _b) ? _a : _b;
		}

		// Returns the maximum of _a and _b. Comparison's based on operator <.
		template <typename _Ty>
		INLINE _Ty _Max(_Ty _a, _Ty _b)
		{
			return (_b < _a) ? _a : _b;
		}

		// Clamps _x between _a and _b. Comparison's based on operator <.
		// Result is unknown if _a is not less or equal to _b.
		template <typename _Ty>
		INLINE _Ty Clamp(_Ty _a, _Ty _x, _Ty _b)
		{
			const _Ty min = _x < _b ? _x : _b;
			return min < _a ? _a : min;
		}

		// Implements int selection, avoiding branching.
		INLINE int Select(bool _b, int _true, int _false)
		{
			return _false ^ (-static_cast<int>(_b) & (_true ^ _false));
		}

		// Implements float selection, avoiding branching.
		INLINE float Select(bool _b, float _true, float _false)
		{
			union
			{
				float f;
				int32_t i;
			} t =
			{ _true };
			union
			{
				float f;
				int32_t i;
			} f =
			{ _false };
			union
			{
				int32_t i;
				float f;
			} r =
			{ f.i ^ (-static_cast<int32_t>(_b) & (t.i ^ f.i)) };
			return r.f;
		}

		// Implements pointer selection, avoiding branching.
		template <typename _Ty>
		INLINE _Ty* Select(bool _b, _Ty* _true, _Ty* _false)
		{
			union
			{
				_Ty* p;
				intptr_t i;
			} t =
			{ _true };
			union
			{
				_Ty* p;
				intptr_t i;
			} f =
			{ _false };
			union
			{
				intptr_t i;
				_Ty* p;
			} r =
			{ f.i ^ (-static_cast<intptr_t>(_b) & (t.i ^ f.i)) };
			return r.p;
		}

		// Implements const pointer selection, avoiding branching.
		template <typename _Ty>
		INLINE const _Ty* Select(bool _b, const _Ty* _true, const _Ty* _false)
		{
			union
			{
				const _Ty* p;
				intptr_t i;
			} t =
			{ _true };
			union
			{
				const _Ty* p;
				intptr_t i;
			} f =
			{ _false };
			union
			{
				intptr_t i;
				const _Ty* p;
			} r =
			{ f.i ^ (-static_cast<intptr_t>(_b) & (t.i ^ f.i)) };
			return r.p;
		}

		// Tests whether _block is aligned to _alignment boundary.
		template <typename _Ty>
		INLINE bool IsAligned(_Ty _value, size_t _alignment)
		{
			return (_value & (_alignment - 1)) == 0;
		}
		template <typename _Ty>
		INLINE bool IsAligned(_Ty* _address, size_t _alignment)
		{
			return (reinterpret_cast<uintptr_t>(_address) & (_alignment - 1)) == 0;
		}

		// Aligns _block address to the first greater address that is aligned to
		// _alignment boundaries.
		template <typename _Ty>
		INLINE _Ty Align(_Ty _value, size_t _alignment)
		{
			return static_cast<_Ty>(_value + (_alignment - 1)) & (0 - _alignment);
		}
		template <typename _Ty>
		INLINE _Ty* Align(_Ty* _address, size_t _alignment)
		{
			return reinterpret_cast<_Ty*>(
				(reinterpret_cast<uintptr_t>(_address) + (_alignment - 1)) &
				(0 - _alignment));
		}

		// Strides a pointer of _stride bytes.
		template <typename _Ty>
		INLINE _Ty* Stride(_Ty* _value, intptr_t _stride)
		{
			return reinterpret_cast<const _Ty*>(reinterpret_cast<uintptr_t>(_value) +
				_stride);
		}
	}  // namespace math
}  // namespace egal
#endif  // MATHS_MATH_EX_H_
