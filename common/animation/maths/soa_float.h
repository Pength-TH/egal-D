
#ifndef MATHS_SOA_FLOAT_H_
#define MATHS_SOA_FLOAT_H_

#include <cassert>
#include "common/animation/platform.h"
#include "common/animation/maths/math_constant.h"
#include "common/animation/maths/simd_math.h"

namespace egal
{
	namespace math
	{

		struct SoaFloat2
		{
			SimdFloat4 x, y;

			static INLINE SoaFloat2 Load(_SimdFloat4 _x, _SimdFloat4 _y)
			{
				const SoaFloat2 r =
				{ _x, _y };
				return r;
			}

			static INLINE SoaFloat2 zero()
			{
				const SoaFloat2 r =
				{ simd_float4::zero(), simd_float4::zero() };
				return r;
			}

			static INLINE SoaFloat2 one()
			{
				const SoaFloat2 r =
				{ simd_float4::one(), simd_float4::one() };
				return r;
			}

			static INLINE SoaFloat2 x_axis()
			{
				const SoaFloat2 r =
				{ simd_float4::one(), simd_float4::zero() };
				return r;
			}

			static INLINE SoaFloat2 y_axis()
			{
				const SoaFloat2 r =
				{ simd_float4::zero(), simd_float4::one() };
				return r;
			}
		};

		struct SoaFloat3
		{
			SimdFloat4 x, y, z;

			static INLINE SoaFloat3 Load(_SimdFloat4 _x, _SimdFloat4 _y,
				_SimdFloat4 _z)
			{
				const SoaFloat3 r =
				{ _x, _y, _z };
				return r;
			}

			static INLINE SoaFloat3 Load(const SoaFloat2& _v, _SimdFloat4 _z)
			{
				const SoaFloat3 r =
				{ _v.x, _v.y, _z };
				return r;
			}

			static INLINE SoaFloat3 zero()
			{
				const SoaFloat3 r =
				{ simd_float4::zero(), simd_float4::zero(),
													 simd_float4::zero() };
				return r;
			}

			static INLINE SoaFloat3 one()
			{
				const SoaFloat3 r =
				{ simd_float4::one(), simd_float4::one(),
													 simd_float4::one() };
				return r;
			}

			static INLINE SoaFloat3 x_axis()
			{
				const SoaFloat3 r =
				{ simd_float4::one(), simd_float4::zero(),
													 simd_float4::zero() };
				return r;
			}

			static INLINE SoaFloat3 y_axis()
			{
				const SoaFloat3 r =
				{ simd_float4::zero(), simd_float4::one(),
													 simd_float4::zero() };
				return r;
			}

			static INLINE SoaFloat3 z_axis()
			{
				const SoaFloat3 r =
				{ simd_float4::zero(), simd_float4::zero(),
													 simd_float4::one() };
				return r;
			}
		};

		struct SoaFloat4
		{
			SimdFloat4 x, y, z, w;

			static INLINE SoaFloat4 Load(_SimdFloat4 _x, _SimdFloat4 _y,
				_SimdFloat4 _z, const SimdFloat4& _w)
			{
				const SoaFloat4 r =
				{ _x, _y, _z, _w };
				return r;
			}

			static INLINE SoaFloat4 Load(const SoaFloat3& _v, _SimdFloat4 _w)
			{
				const SoaFloat4 r =
				{ _v.x, _v.y, _v.z, _w };
				return r;
			}

			static INLINE SoaFloat4 Load(const SoaFloat2& _v, _SimdFloat4 _z,
				_SimdFloat4 _w)
			{
				const SoaFloat4 r =
				{ _v.x, _v.y, _z, _w };
				return r;
			}

			static INLINE SoaFloat4 zero()
			{
				const SimdFloat4 zero = simd_float4::zero();
				const SoaFloat4 r =
				{ zero, zero, zero, zero };
				return r;
			}

			static INLINE SoaFloat4 one()
			{
				const SimdFloat4 one = simd_float4::one();
				const SoaFloat4 r =
				{ one, one, one, one };
				return r;
			}

			static INLINE SoaFloat4 x_axis()
			{
				const SimdFloat4 zero = simd_float4::zero();
				const SoaFloat4 r =
				{ simd_float4::one(), zero, zero, zero };
				return r;
			}

			static INLINE SoaFloat4 y_axis()
			{
				const SimdFloat4 zero = simd_float4::zero();
				const SoaFloat4 r =
				{ zero, simd_float4::one(), zero, zero };
				return r;
			}

			static INLINE SoaFloat4 z_axis()
			{
				const SimdFloat4 zero = simd_float4::zero();
				const SoaFloat4 r =
				{ zero, zero, simd_float4::one(), zero };
				return r;
			}

			static INLINE SoaFloat4 w_axis()
			{
				const SimdFloat4 zero = simd_float4::zero();
				const SoaFloat4 r =
				{ zero, zero, zero, simd_float4::one() };
				return r;
			}
		};
	}  // namespace math
}  // namespace egal

// Returns per element addition of _a and _b using operator +.
INLINE egal::math::SoaFloat4 operator+(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SoaFloat4 r =
	{ _a.x + _b.x, _a.y + _b.y, _a.z + _b.z,
										_a.w + _b.w };
	return r;
}
INLINE egal::math::SoaFloat3 operator+(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SoaFloat3 r =
	{ _a.x + _b.x, _a.y + _b.y, _a.z + _b.z };
	return r;
}
INLINE egal::math::SoaFloat2 operator+(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SoaFloat2 r =
	{ _a.x + _b.x, _a.y + _b.y };
	return r;
}

// Returns per element subtraction of _a and _b using operator -.
INLINE egal::math::SoaFloat4 operator-(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SoaFloat4 r =
	{ _a.x - _b.x, _a.y - _b.y, _a.z - _b.z,
										_a.w - _b.w };
	return r;
}
INLINE egal::math::SoaFloat3 operator-(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SoaFloat3 r =
	{ _a.x - _b.x, _a.y - _b.y, _a.z - _b.z };
	return r;
}
INLINE egal::math::SoaFloat2 operator-(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SoaFloat2 r =
	{ _a.x - _b.x, _a.y - _b.y };
	return r;
}

// Returns per element negative value of _v.
INLINE egal::math::SoaFloat4 operator-(const egal::math::SoaFloat4& _v)
{
	const egal::math::SoaFloat4 r =
	{ -_v.x, -_v.y, -_v.z, -_v.w };
	return r;
}
INLINE egal::math::SoaFloat3 operator-(const egal::math::SoaFloat3& _v)
{
	const egal::math::SoaFloat3 r =
	{ -_v.x, -_v.y, -_v.z };
	return r;
}
INLINE egal::math::SoaFloat2 operator-(const egal::math::SoaFloat2& _v)
{
	const egal::math::SoaFloat2 r =
	{ -_v.x, -_v.y };
	return r;
}

// Returns per element multiplication of _a and _b using operator *.
INLINE egal::math::SoaFloat4 operator*(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SoaFloat4 r =
	{ _a.x * _b.x, _a.y * _b.y, _a.z * _b.z,
										_a.w * _b.w };
	return r;
}
INLINE egal::math::SoaFloat3 operator*(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SoaFloat3 r =
	{ _a.x * _b.x, _a.y * _b.y, _a.z * _b.z };
	return r;
}
INLINE egal::math::SoaFloat2 operator*(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SoaFloat2 r =
	{ _a.x * _b.x, _a.y * _b.y };
	return r;
}

// Returns per element multiplication of _a and scalar value _f using
// operator *.
INLINE egal::math::SoaFloat4 operator*(const egal::math::SoaFloat4& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat4 r =
	{ _a.x * _f, _a.y * _f, _a.z * _f, _a.w * _f };
	return r;
}
INLINE egal::math::SoaFloat3 operator*(const egal::math::SoaFloat3& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat3 r =
	{ _a.x * _f, _a.y * _f, _a.z * _f };
	return r;
}
INLINE egal::math::SoaFloat2 operator*(const egal::math::SoaFloat2& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat2 r =
	{ _a.x * _f, _a.y * _f };
	return r;
}

// Multiplies _a and _b, then adds _addend.
// v = (_a * _b) + _addend
INLINE egal::math::SoaFloat2 MAdd(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b,
	const egal::math::SoaFloat2& _addend)
{
	const egal::math::SoaFloat2 r =
	{ egal::math::MAdd(_a.x, _b.x, _addend.x),
										egal::math::MAdd(_a.y, _b.y, _addend.y) };
	return r;
}
INLINE egal::math::SoaFloat3 MAdd(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b,
	const egal::math::SoaFloat3& _addend)
{
	const egal::math::SoaFloat3 r =
	{ egal::math::MAdd(_a.x, _b.x, _addend.x),
										egal::math::MAdd(_a.y, _b.y, _addend.y),
										egal::math::MAdd(_a.z, _b.z, _addend.z) };
	return r;
}
INLINE egal::math::SoaFloat4 MAdd(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b,
	const egal::math::SoaFloat4& _addend)
{
	const egal::math::SoaFloat4 r =
	{ egal::math::MAdd(_a.x, _b.x, _addend.x),
										egal::math::MAdd(_a.y, _b.y, _addend.y),
										egal::math::MAdd(_a.z, _b.z, _addend.z),
										egal::math::MAdd(_a.w, _b.w, _addend.w) };
	return r;
}

// Returns per element division of _a and _b using operator /.
INLINE egal::math::SoaFloat4 operator/(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SoaFloat4 r =
	{ _a.x / _b.x, _a.y / _b.y, _a.z / _b.z,
										_a.w / _b.w };
	return r;
}
INLINE egal::math::SoaFloat3 operator/(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SoaFloat3 r =
	{ _a.x / _b.x, _a.y / _b.y, _a.z / _b.z };
	return r;
}
INLINE egal::math::SoaFloat2 operator/(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SoaFloat2 r =
	{ _a.x / _b.x, _a.y / _b.y };
	return r;
}

// Returns per element division of _a and scalar value _f using operator/.
INLINE egal::math::SoaFloat4 operator/(const egal::math::SoaFloat4& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat4 r =
	{ _a.x / _f, _a.y / _f, _a.z / _f, _a.w / _f };
	return r;
}
INLINE egal::math::SoaFloat3 operator/(const egal::math::SoaFloat3& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat3 r =
	{ _a.x / _f, _a.y / _f, _a.z / _f };
	return r;
}
INLINE egal::math::SoaFloat2 operator/(const egal::math::SoaFloat2& _a,
	egal::math::_SimdFloat4 _f)
{
	const egal::math::SoaFloat2 r =
	{ _a.x / _f, _a.y / _f };
	return r;
}

// Returns true if each element of a is less than each element of _b.
INLINE egal::math::SimdInt4 operator<(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLt(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpLt(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpLt(_a.w, _b.w);
	return egal::math::And(egal::math::And(egal::math::And(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator<(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLt(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpLt(_a.z, _b.z);
	return egal::math::And(egal::math::And(x, y), z);
}
INLINE egal::math::SimdInt4 operator<(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLt(_a.y, _b.y);
	return egal::math::And(x, y);
}

// Returns true if each element of a is less or equal to each element of _b.
INLINE egal::math::SimdInt4 operator<=(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpLe(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpLe(_a.w, _b.w);
	return egal::math::And(egal::math::And(egal::math::And(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator<=(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpLe(_a.z, _b.z);
	return egal::math::And(egal::math::And(x, y), z);
}
INLINE egal::math::SimdInt4 operator<=(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpLe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpLe(_a.y, _b.y);
	return egal::math::And(x, y);
}

// Returns true if each element of a is greater than each element of _b.
INLINE egal::math::SimdInt4 operator>(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGt(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpGt(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpGt(_a.w, _b.w);
	return egal::math::And(egal::math::And(egal::math::And(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator>(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGt(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpGt(_a.z, _b.z);
	return egal::math::And(egal::math::And(x, y), z);
}
INLINE egal::math::SimdInt4 operator>(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGt(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGt(_a.y, _b.y);
	return egal::math::And(x, y);
}

// Returns true if each element of a is greater or equal to each element of _b.
INLINE egal::math::SimdInt4 operator>=(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpGe(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpGe(_a.w, _b.w);
	return egal::math::And(egal::math::And(egal::math::And(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator>=(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpGe(_a.z, _b.z);
	return egal::math::And(egal::math::And(x, y), z);
}
INLINE egal::math::SimdInt4 operator>=(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpGe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpGe(_a.y, _b.y);
	return egal::math::And(x, y);
}

// Returns true if each element of _a is equal to each element of _b.
// Uses a bitwise comparison of _a and _b, no tolerance is applied.
INLINE egal::math::SimdInt4 operator==(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpEq(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpEq(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpEq(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpEq(_a.w, _b.w);
	return egal::math::And(egal::math::And(egal::math::And(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator==(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpEq(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpEq(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpEq(_a.z, _b.z);
	return egal::math::And(egal::math::And(x, y), z);
}
INLINE egal::math::SimdInt4 operator==(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpEq(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpEq(_a.y, _b.y);
	return egal::math::And(x, y);
}

// Returns true if each element of a is different from each element of _b.
// Uses a bitwise comparison of _a and _b, no tolerance is applied.
INLINE egal::math::SimdInt4 operator!=(const egal::math::SoaFloat4& _a,
	const egal::math::SoaFloat4& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpNe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpNe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpNe(_a.z, _b.z);
	const egal::math::SimdInt4 w = egal::math::CmpNe(_a.w, _b.w);
	return egal::math::Or(egal::math::Or(egal::math::Or(x, y), z), w);
}
INLINE egal::math::SimdInt4 operator!=(const egal::math::SoaFloat3& _a,
	const egal::math::SoaFloat3& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpNe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpNe(_a.y, _b.y);
	const egal::math::SimdInt4 z = egal::math::CmpNe(_a.z, _b.z);
	return egal::math::Or(egal::math::Or(x, y), z);
}
INLINE egal::math::SimdInt4 operator!=(const egal::math::SoaFloat2& _a,
	const egal::math::SoaFloat2& _b)
{
	const egal::math::SimdInt4 x = egal::math::CmpNe(_a.x, _b.x);
	const egal::math::SimdInt4 y = egal::math::CmpNe(_a.y, _b.y);
	return egal::math::Or(x, y);
}

namespace egal
{
	namespace math
	{

		// Returns the (horizontal) addition of each element of _v.
		INLINE SimdFloat4 HAdd(const SoaFloat4& _v)
		{
			return _v.x + _v.y + _v.z + _v.w;
		}
		INLINE SimdFloat4 HAdd(const SoaFloat3& _v)
		{
			return _v.x + _v.y + _v.z;
		}
		INLINE SimdFloat4 HAdd(const SoaFloat2& _v)
		{
			return _v.x + _v.y;
		}

		// Returns the dot product of _a and _b.
		INLINE SimdFloat4 Dot(const SoaFloat4& _a, const SoaFloat4& _b)
		{
			return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
		}
		INLINE SimdFloat4 Dot(const SoaFloat3& _a, const SoaFloat3& _b)
		{
			return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z;
		}
		INLINE SimdFloat4 Dot(const SoaFloat2& _a, const SoaFloat2& _b)
		{
			return _a.x * _b.x + _a.y * _b.y;
		}

		// Returns the cross product of _a and _b.
		INLINE SoaFloat3 CrossProduct(const SoaFloat3& _a, const SoaFloat3& _b)
		{
			const SoaFloat3 r =
			{ _a.y * _b.z - _b.y * _a.z, _a.z * _b.x - _b.z * _a.x,
											 _a.x * _b.y - _b.x * _a.y };
			return r;
		}

		// Returns the length |_v| of _v.
		INLINE SimdFloat4 Length(const SoaFloat4& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
			return Sqrt(len2);
		}
		INLINE SimdFloat4 Length(const SoaFloat3& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
			return Sqrt(len2);
		}
		INLINE SimdFloat4 Length(const SoaFloat2& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y;
			return Sqrt(len2);
		}

		// Returns the square length |_v|^2 of _v.
		INLINE SimdFloat4 LengthSqr(const SoaFloat4& _v)
		{
			return _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
		}
		INLINE SimdFloat4 LengthSqr(const SoaFloat3& _v)
		{
			return _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
		}
		INLINE SimdFloat4 LengthSqr(const SoaFloat2& _v)
		{
			return _v.x * _v.x + _v.y * _v.y;
		}

		// Returns the normalized vector _v.
		INLINE SoaFloat4 Normalize(const SoaFloat4& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
			assert(AreAllTrue(CmpNe(len2, simd_float4::zero())) &&
				"_v is not normalizable");
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat4 r =
			{ _v.x * inv_len, _v.y * inv_len, _v.z * inv_len,
											 _v.w * inv_len };
			return r;
		}
		INLINE SoaFloat3 Normalize(const SoaFloat3& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
			assert(AreAllTrue(CmpNe(len2, simd_float4::zero())) &&
				"_v is not normalizable");
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat3 r =
			{ _v.x * inv_len, _v.y * inv_len, _v.z * inv_len };
			return r;
		}
		INLINE SoaFloat2 Normalize(const SoaFloat2& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y;
			assert(AreAllTrue(CmpNe(len2, simd_float4::zero())) &&
				"_v is not normalizable");
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat2 r =
			{ _v.x * inv_len, _v.y * inv_len };
			return r;
		}

		// Test if each vector _v is normalized.
		INLINE math::SimdInt4 IsNormalized(const SoaFloat4& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceSq));
		}
		INLINE math::SimdInt4 IsNormalized(const SoaFloat3& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceSq));
		}
		INLINE math::SimdInt4 IsNormalized(const SoaFloat2& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceSq));
		}

		// Test if each vector _v is normalized using estimated tolerance.
		INLINE math::SimdInt4 IsNormalizedEst(const SoaFloat4& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceEstSq));
		}
		INLINE math::SimdInt4 IsNormalizedEst(const SoaFloat3& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceEstSq));
		}
		INLINE math::SimdInt4 IsNormalizedEst(const SoaFloat2& _v)
		{
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y;
			return CmpLt(Abs(len2 - math::simd_float4::one()),
				simd_float4::Load1(kNormalizationToleranceEstSq));
		}

		// Returns the normalized vector _v if the norm of _v is not 0.
		// Otherwise returns _safer.
		INLINE SoaFloat4 NormalizeSafe(const SoaFloat4& _v,
			const SoaFloat4& _safer)
		{
			assert(AreAllTrue(IsNormalizedEst(_safer)) && "_safer is not normalized");
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z + _v.w * _v.w;
			const math::SimdInt4 b = CmpNe(len2, math::simd_float4::zero());
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat4 r =
			{
							Select(b, _v.x * inv_len, _safer.x), Select(b, _v.y * inv_len, _safer.y),
							Select(b, _v.z * inv_len, _safer.z), Select(b, _v.w * inv_len, _safer.w) };
			return r;
		}
		INLINE SoaFloat3 NormalizeSafe(const SoaFloat3& _v,
			const SoaFloat3& _safer)
		{
			assert(AreAllTrue(IsNormalizedEst(_safer)) && "_safer is not normalized");
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y + _v.z * _v.z;
			const math::SimdInt4 b = CmpNe(len2, math::simd_float4::zero());
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat3 r =
			{ Select(b, _v.x * inv_len, _safer.x),
											 Select(b, _v.y * inv_len, _safer.y),
											 Select(b, _v.z * inv_len, _safer.z) };
			return r;
		}
		INLINE SoaFloat2 NormalizeSafe(const SoaFloat2& _v,
			const SoaFloat2& _safer)
		{
			assert(AreAllTrue(IsNormalizedEst(_safer)) && "_safer is not normalized");
			const SimdFloat4 len2 = _v.x * _v.x + _v.y * _v.y;
			const math::SimdInt4 b = CmpNe(len2, math::simd_float4::zero());
			const SimdFloat4 inv_len = math::simd_float4::one() / Sqrt(len2);
			const SoaFloat2 r =
			{ Select(b, _v.x * inv_len, _safer.x),
											 Select(b, _v.y * inv_len, _safer.y) };
			return r;
		}

		// Returns the linear interpolation of _a and _b with coefficient _f.
		// _f is not limited to range [0,1].
		INLINE SoaFloat4 Lerp(const SoaFloat4& _a, const SoaFloat4& _b,
			_SimdFloat4 _f)
		{
			const SoaFloat4 r =
			{ (_b.x - _a.x) * _f + _a.x, (_b.y - _a.y) * _f + _a.y,
											 (_b.z - _a.z) * _f + _a.z, (_b.w - _a.w) * _f + _a.w };
			return r;
		}
		INLINE SoaFloat3 Lerp(const SoaFloat3& _a, const SoaFloat3& _b,
			_SimdFloat4 _f)
		{
			const SoaFloat3 r =
			{ (_b.x - _a.x) * _f + _a.x, (_b.y - _a.y) * _f + _a.y,
											 (_b.z - _a.z) * _f + _a.z };
			return r;
		}
		INLINE SoaFloat2 Lerp(const SoaFloat2& _a, const SoaFloat2& _b,
			_SimdFloat4 _f)
		{
			const SoaFloat2 r =
			{ (_b.x - _a.x) * _f + _a.x, (_b.y - _a.y) * _f + _a.y };
			return r;
		}

		// Returns the minimum of each element of _a and _b.
		INLINE SoaFloat4 Min(const SoaFloat4& _a, const SoaFloat4& _b)
		{
			const SoaFloat4 r =
			{ Min(_a.x, _b.x), Min(_a.y, _b.y), Min(_a.z, _b.z),
											 Min(_a.w, _b.w) };
			return r;
		}
		INLINE SoaFloat3 Min(const SoaFloat3& _a, const SoaFloat3& _b)
		{
			const SoaFloat3 r =
			{ Min(_a.x, _b.x), Min(_a.y, _b.y), Min(_a.z, _b.z) };
			return r;
		}
		INLINE SoaFloat2 Min(const SoaFloat2& _a, const SoaFloat2& _b)
		{
			const SoaFloat2 r =
			{ Min(_a.x, _b.x), Min(_a.y, _b.y) };
			return r;
		}

		// Returns the maximum of each element of _a and _b.
		INLINE SoaFloat4 Max(const SoaFloat4& _a, const SoaFloat4& _b)
		{
			const SoaFloat4 r =
			{ Max(_a.x, _b.x), Max(_a.y, _b.y), Max(_a.z, _b.z),
											 Max(_a.w, _b.w) };
			return r;
		}
		INLINE SoaFloat3 Max(const SoaFloat3& _a, const SoaFloat3& _b)
		{
			const SoaFloat3 r =
			{ Max(_a.x, _b.x), Max(_a.y, _b.y), Max(_a.z, _b.z) };
			return r;
		}
		INLINE SoaFloat2 Max(const SoaFloat2& _a, const SoaFloat2& _b)
		{
			const SoaFloat2 r =
			{ Max(_a.x, _b.x), Max(_a.y, _b.y) };
			return r;
		}

		// Clamps each element of _x between _a and _b.
		// _a must be less or equal to b;
		INLINE SoaFloat4 Clamp(const SoaFloat4& _a, const SoaFloat4& _v,
			const SoaFloat4& _b)
		{
			return Max(_a, Min(_v, _b));
		}
		INLINE SoaFloat3 Clamp(const SoaFloat3& _a, const SoaFloat3& _v,
			const SoaFloat3& _b)
		{
			return Max(_a, Min(_v, _b));
		}
		INLINE SoaFloat2 Clamp(const SoaFloat2& _a, const SoaFloat2& _v,
			const SoaFloat2& _b)
		{
			return Max(_a, Min(_v, _b));
		}
	}  // namespace math
}  // namespace egal
#endif  // MATHS_SOA_FLOAT_H_
