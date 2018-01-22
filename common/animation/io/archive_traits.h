
#ifndef _IO_ARCHIVE_TRAITS_H_
#define _IO_ARCHIVE_TRAITS_H_

#include <stdint.h>
#include <cstddef>

namespace egal
{
	namespace io
	{

		// Forward declaration of archive types.
		class OArchive;
		class IArchive;

		// Default loading and saving external declaration.
		// Those template implementations aim to be specialized at compilation time by
		// non-member Load and save functions. For example the specialization of the
		// Save() function for a type Type is:
		// void Save(OArchive& _archive, const Extrusive* _test, size_t _count) {
		// }
		// The Load() function receives the version _version of type _Ty at the time the
		// archive was saved.
		// This uses a polymorphism rather than template specialization to avoid
		// including the file that contains the template definition.
		//
		// This default function call member _Ty::Load/Save function.
		template <typename _Ty>
		inline void Save(OArchive& _archive, const _Ty* _ty, size_t _count);
		template <typename _Ty>
		inline void Load(IArchive& _archive, _Ty* _ty, size_t _count,
			uint32_t _version);

		// clang-format off

		// Declares the current (compile time) version of _type.
		// This macro must be used inside namespace egal::io.
		// Syntax is: IO_TYPE_VERSION(46, Foo).
#define IO_TYPE_VERSION(_version, _type)	 \
		STATIC_ASSERT(_version > 0);				 \
		namespace internal {                         \
		template<> struct Version<const _type> {     \
			  enum { kValue = _version };            \
		};                                           \
		}  // internal

		// Declares the current (compile time) version of a template _type.
		// This macro must be used inside namespace egal::io.
		// IO_TYPE_VERSION_T1(46, typename, Foo<char>).
#define IO_TYPE_VERSION_T1(_version, _arg0, ...)	 \
		STATIC_ASSERT(_version > 0);						 \
		namespace internal {                                 \
		template<_arg0>                                      \
		struct Version<const __VA_ARGS__> {                  \
			  enum { kValue = _version };                    \
		};                                                   \
		}  // internal

		// Declares the current (compile time) version of a template _type.
		// This macro must be used inside namespace egal::io.
		// IO_TYPE_VERSION_T1(46, typename, typename, Foo<char, bool>).
#define IO_TYPE_VERSION_T2(_version, _arg0, _arg1, ...)		\
		STATIC_ASSERT(_version > 0);								\
		namespace internal {                                        \
		template<_arg0, _arg1>                                      \
		struct Version<const __VA_ARGS__> {                         \
			  enum { kValue = _version };                           \
		};                                                          \
																	\
		}  // internal

		// Declares the current (compile time) version of a template _type.
		// This macro must be used inside namespace egal::io.
		// IO_TYPE_VERSION_T3(
		//   46, typename, typename, typename, Foo<char, bool, bool>).
#define IO_TYPE_VERSION_T3(_version, _arg0, _arg1, _arg2, ...)	   \
		STATIC_ASSERT(_version > 0);									   \
		namespace internal {                                               \
		template<_arg0, _arg1, _arg2>                                      \
		struct Version<const __VA_ARGS__> {                                \
			  enum { kValue = _version };                                  \
		};                                                                 \
																		   \
		}  // internal

		// Declares the current (compile time) version of a template _type.
		// This macro must be used inside namespace egal::io.
		// IO_TYPE_VERSION_T4(
		//   46, typename, typename, typename, bool, Foo<char, bool, bool, false>).
#define IO_TYPE_VERSION_T4(_version, _arg0, _arg1, _arg2, _arg3, ...)	  \
		STATIC_ASSERT(_version > 0);											  \
		namespace internal {                                                      \
		template<_arg0, _arg1, _arg2, _arg3>                                      \
		struct Version<const __VA_ARGS__> {                                       \
			  enum { kValue = _version };                                         \
		};                                                                        \
		}  // internal

		// Declares that _type is not versionable. Its version number is 0.
		// Once a type has been declared not versionable, it cannot be changed without
		// braking versioning.
		// This macro must be used inside namespace egal::io.
		// Syntax is: IO_TYPE_NOT_VERSIONABLE(Foo).
#define IO_TYPE_NOT_VERSIONABLE(_type)			    \
		namespace internal							\
		{											\
			template<> struct Version<const _type>  \
			{										\
			  enum { kValue = 0 };                  \
			};                                      \
		}  // internal

		// Declares that a template _type is not versionable. Its version number is 0.
		// Once a type has been declared not versionable, it cannot be changed without
		// braking versioning.
		// This macro must be used inside namespace egal::io.
		// Syntax is:
		// IO_TYPE_NOT_VERSIONABLE_T1(typename, Foo<char>).
#define IO_TYPE_NOT_VERSIONABLE_T1(_arg0, ...)	   \
		namespace internal {                               \
		template<_arg0>                                    \
		struct Version<const __VA_ARGS__> {                \
			  enum { kValue = 0 };                         \
		};                                                 \
		}  // internal

		// Decline non-versionable template declaration to 2 template arguments.
		// Syntax is:
		// IO_TYPE_NOT_VERSIONABLE_T2(typename, bool, Foo<char, true>).
#define IO_TYPE_NOT_VERSIONABLE_T2(_arg0, _arg1, ...)	  \
		namespace internal {                                      \
		template<_arg0, _arg1>                                    \
		struct Version<const __VA_ARGS__> {                       \
			  enum { kValue = 0 };                                \
		};                                                        \
		}  // internal

		// Decline non-versionable template declaration to 3 template arguments.
		// Syntax is:
		// IO_TYPE_NOT_VERSIONABLE_T3(
		//   typename, typename, bool, Foo<char, int, true>).
#define IO_TYPE_NOT_VERSIONABLE_T3(_arg0, _arg1, _arg2, ...)	 \
		namespace internal {                                             \
		template<_arg0, _arg1, _arg2>                                    \
		struct Version<const __VA_ARGS__> {                              \
			  enum { kValue = 0 };                                       \
		};                                                               \
		}  // internal

		// Decline non-versionable template declaration to 4 template arguments.
		// Syntax is:
		// IO_TYPE_NOT_VERSIONABLE_T4(
		//   typename, typename, bool, typename, Foo<char, int, true, bool>).
#define IO_TYPE_NOT_VERSIONABLE_T4(_arg0, _arg1, _arg2, _arg3, ...)		\
		namespace internal {                                                    \
		template<_arg0, _arg1, _arg2, _arg3>                                    \
		struct Version<const __VA_ARGS__> {                                     \
			  enum { kValue = 0 };                                              \
		};                                                                      \
		}  // internal

		// Declares the tag of a template _type.
		// A tag is a c-string that can be used to check the type (through its tag) of
		// the next object to be read from an archive. If no tag is defined, then no
		// check is performed.
		// This macro must be used inside namespace egal::io.
		// IO_TYPE_TAG("FourtySix", Foo).
#define IO_TYPE_TAG(_tag, _type)										 \
		namespace internal {                                                     \
		template<> struct Tag<const _type> {                                     \
			  /* Length includes null terminated character to detect partial */  \
			  /* tag mapping.*/                                                  \
			  enum { kTagLength = ARRAY_SIZE(_tag) };							 \
			  static const char* Get() { return _tag; }                          \
		};                                                                       \
		}  // internal

		// clang-format on
		namespace internal
		{
			// Definition of version specializable template struct.
			// There's no default implementation in order to force user to define it, which
			// in turn forces those who want to serialize an object to include the file that
			// defines it's version. This helps with detecting issues at compile time.
			template <typename _Ty>
			struct Version;

			// Defines default tag value, which is disabled.
			template <typename _Ty>
			struct Tag
			{
				enum { kTagLength = 0 };
			};
		}  // namespace internal
	}  // namespace io
}  // namespace egal
#endif  // _IO_ARCHIVE_TRAITS_H_
