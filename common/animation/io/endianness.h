#ifndef _ENDIANNESS_H_
#define _ENDIANNESS_H_

// Declares endianness modes and functions to swap data from a mode to another.

#include <cstddef>
#include "common/animation/platform.h"

namespace egal
{
	enum Endianness
	{
		native,
		little,
		big,
	};

	// Get the native endianness of the targeted processor.
	// This function does not rely on a pre-processor definition as no standard
	// definition exists. It is rather implemented as a portable runtime function.
	inline Endianness GetNativeEndianness()
	{
		const union
		{
			uint16_t s;
			uint8_t c[2];
		} u = { 1 };  // Initializes u.s -> then read u.c.
		return Endianness(u.c[0]);
	}

	// Declare the endian swapper struct that is aimed to be specialized (template
	// meaning) for every type sizes.
	// The swapper provides two functions:
	// - void Swap(_Ty* _ty, size_t _count) swaps the array _ty of _count
	// elements in-place.
	// - _Ty Swap(_Ty _ty) returns a swapped copy of _ty.
	// It can be used directly if _Ty is known or through EndianSwap function.
	// The default value of template attribute _size enables automatic
	// specialization selection.
	template <typename _Ty, size_t _size = sizeof(_Ty)>
	struct EndianSwapper;

	// Internal macro used to swap two bytes.
#define _BYTE_SWAP(_a, _b)	   \
  {                            \
    const uint8_t temp = _a;   \
    _a = _b;                   \
    _b = temp;                 \
  }

// EndianSwapper specialization for 1 byte types.
	template <typename _Ty>
	struct EndianSwapper<_Ty, 1>
	{
		INLINE static void Swap(_Ty* _ty, size_t _count) {
			(void)_ty;
			(void)_count;
		}
		INLINE static _Ty Swap(_Ty _ty) { return _ty; }
	};

	// EndianSwapper specialization for 2 bytes types.
	template <typename _Ty>
	struct EndianSwapper<_Ty, 2>
	{
		INLINE static void Swap(_Ty* _ty, size_t _count)
		{
			char* alias = reinterpret_cast<char*>(_ty);
			for (size_t i = 0; i < _count * 2; i += 2) {
				_BYTE_SWAP(alias[i + 0], alias[i + 1]);
			}
		}
		INLINE static _Ty Swap(_Ty _ty)
		{  // Pass by copy to swap _ty in-place.
			char* alias = reinterpret_cast<char*>(&_ty);
			_BYTE_SWAP(alias[0], alias[1]);
			return _ty;
		}
	};

	// EndianSwapper specialization for 4 bytes types.
	template <typename _Ty>
	struct EndianSwapper<_Ty, 4>
	{
		INLINE static void Swap(_Ty* _ty, size_t _count)
		{
			char* alias = reinterpret_cast<char*>(_ty);
			for (size_t i = 0; i < _count * 4; i += 4)
			{
				_BYTE_SWAP(alias[i + 0], alias[i + 3]);
				_BYTE_SWAP(alias[i + 1], alias[i + 2]);
			}
		}
		INLINE static _Ty Swap(_Ty _ty)
		{  // Pass by copy to swap _ty in-place.
			char* alias = reinterpret_cast<char*>(&_ty);
			_BYTE_SWAP(alias[0], alias[3]);
			_BYTE_SWAP(alias[1], alias[2]);
			return _ty;
		}
	};

	// EndianSwapper specialization for 8 bytes types.
	template <typename _Ty>
	struct EndianSwapper<_Ty, 8>
	{
		INLINE static void Swap(_Ty* _ty, size_t _count)
		{
			char* alias = reinterpret_cast<char*>(_ty);
			for (size_t i = 0; i < _count * 8; i += 8)
			{
				_BYTE_SWAP(alias[i + 0], alias[i + 7]);
				_BYTE_SWAP(alias[i + 1], alias[i + 6]);
				_BYTE_SWAP(alias[i + 2], alias[i + 5]);
				_BYTE_SWAP(alias[i + 3], alias[i + 4]);
			}
		}
		INLINE static _Ty Swap(_Ty _ty)
		{  // Pass by copy to swap _ty in-place.
			char* alias = reinterpret_cast<char*>(&_ty);
			_BYTE_SWAP(alias[0], alias[7]);
			_BYTE_SWAP(alias[1], alias[6]);
			_BYTE_SWAP(alias[2], alias[5]);
			_BYTE_SWAP(alias[3], alias[4]);
			return _ty;
		}
	};

	// _BYTE_SWAP is not useful anymore.
#undef _BYTE_SWAP

	// Helper function that swaps _count elements of the array _ty in place.
	template <typename _Ty>
	INLINE void EndianSwap(_Ty* _ty, size_t _count)
	{
		EndianSwapper<_Ty>::Swap(_ty, _count);
	}

	// Helper function that swaps _ty in place.
	template <typename _Ty>
	INLINE _Ty EndianSwap(_Ty _ty)
	{
		return EndianSwapper<_Ty>::Swap(_ty);
	}
}
#endif  // BASE_ENDIANNESS_H_
