#ifndef _string_h_
#define _string_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/define.h"

namespace egal
{
	namespace StringUnitl
	{
		const e_char* stristr(const e_char* haystack, const e_char* needle);

		e_bool toCStringHex(e_uint8 value, e_char* output, e_int32 length);
		e_bool toCStringPretty(e_int32 value, e_char* output, e_int32 length);
		e_bool toCStringPretty(e_uint32 value, e_char* output, e_int32 length);
		e_bool toCStringPretty(e_uint64 value, e_char* output, e_int32 length);
		e_bool toCString(e_int32 value, e_char* output, e_int32 length);
		e_bool toCString(e_int64 value, e_char* output, e_int32 length);
		e_bool toCString(e_uint64 value, e_char* output, e_int32 length);
		e_bool toCString(e_uint32 value, e_char* output, e_int32 length);
		e_bool toCString(e_float value, e_char* output, e_int32 length, e_int32 after_point);
		const e_char* reverseFind(const e_char* begin_haystack, const e_char* end_haystack, e_char c);
		const e_char* fromCString(const e_char* input, e_int32 length, e_int32* value);
		const e_char* fromCString(const e_char* input, e_int32 length, e_uint64* value);
		const e_char* fromCString(const e_char* input, e_int32 length, e_int64* value);
		const e_char* fromCString(const e_char* input, e_int32 length, e_uint32* value);
		const e_char* fromCString(const e_char* input, e_int32 length, e_uint16* value);
		e_bool copyString(e_char* destination, e_int32 length, const e_char* source);
		e_bool copyNString(e_char* destination,
			e_int32 length,
			const e_char* source,
			e_int32 source_len);
		e_bool catString(e_char* destination, e_int32 length, const e_char* source);
		e_bool catNString(e_char* destination, e_int32 length, const e_char* source, e_int32 source_len);
		e_bool makeLowercase(e_char* destination, e_int32 length, const e_char* source);
		e_char makeUppercase(e_char c);
		e_bool makeUppercase(e_char* destination, e_int32 length, const e_char* source);
		e_char* trimmed(e_char* str);
		e_bool startsWith(const e_char* str, const e_char* prefix);
		e_int32 stringLength(const e_char* str);
		e_bool equalStrings(const e_char* lhs, const e_char* rhs);
		e_bool equalIStrings(const e_char* lhs, const e_char* rhs);
		e_int32 compareMemory(const e_void* lhs, const e_void* rhs, size_t size);
		e_int32 compareString(const e_char* lhs, const e_char* rhs);
		e_int32 compareStringN(const e_char* lhs, const e_char* rhs, e_int32 length);
		e_int32 compareIStringN(const e_char* lhs, const e_char* rhs, e_int32 length);
		e_void copyMemory(e_void* dest, const e_void* src, size_t count);
		e_void moveMemory(e_void* dest, const e_void* src, size_t count);
		e_void setMemory(e_void* ptr, e_uint8 value, size_t num);
		const e_char* findSubstring(const e_char* str, const e_char* substr);
		e_bool endsWith(const e_char* str, const e_char* substr);

		e_void normalize(const e_char* path, e_char* out, e_uint32 max_size);
		e_void getDir(e_char* dir, e_int32 max_length, const e_char* src);
		e_void getBasename(e_char* basename, e_int32 max_length, const e_char* src);
		e_void getFilename(e_char* filename, e_int32 max_length, const e_char* src);
		e_void getExtension(e_char* extension, e_int32 max_length, const e_char* src);
		e_bool hasExtension(const e_char* filename, const e_char* ext);
		e_bool isAbsolute(const e_char* path);

		inline e_bool isLetter(e_char c)
		{
			return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
		}

		inline e_bool isNumeric(e_char c)
		{
			return c >= '0' && c <= '9';
		}

		inline e_bool isUpperCase(e_char c)
		{
			return c >= 'A' && c <= 'Z';
		}

		template <e_int32 SIZE> e_bool copyString(e_char(&destination)[SIZE], const e_char* source)
		{
			return copyString(destination, SIZE, source);
		}

		template <e_int32 SIZE> e_bool catString(e_char(&destination)[SIZE], const e_char* source)
		{
			return catString(destination, SIZE, source);
		}
	}

	template <e_int32 size> 
	struct StaticString
	{
		StaticString() 
		{ 
			data[0] = '\0'; 
		}

		explicit StaticString(const e_char* str) 
		{ 
			StringUnitl::copyString(data, size, str); 
		}

		template <typename... Args> StaticString(const e_char* str, Args... args)
		{
			StringUnitl::copyString(data, size, str);
			e_int32 tmp[] = { (add(args), 0)... };
			(e_void)tmp;
		}

		template <e_int32 value_size> StaticString& operator<<(StaticString<value_size>& value)
		{
			add(value);
			return *this;
		}

		template <typename T> StaticString& operator<<(T value)
		{
			add(value);
			return *this;
		}

		template <e_int32 value_size> e_void add(StaticString<value_size>& value) 
		{ 
			StringUnitl::catString(data, size, value.data); 
		}
		e_void add(const e_char* value) 
		{ 
			StringUnitl::catString(data, size, value); 
		}
		e_void add(e_char* value) 
		{ 
			StringUnitl::catString(data, size, value); 
		}

		e_void operator=(const e_char* str) 
		{ 
			StringUnitl::copyString(data, str); 
		}

		e_void add(e_float value)
		{
			e_int32 len = StringUnitl::stringLength(data);
			StringUnitl::toCString(value, data + len, size - len, 3);
		}

		template <typename T> e_void add(T value)
		{
			e_int32 len = StringUnitl::stringLength(data);
			StringUnitl::toCString(value, data + len, size - len);
		}

		e_bool operator<(const e_char* str) const 
		{
			return StringUnitl::compareString(data, str) < 0;
		}

		e_bool operator==(const e_char* str) const 
		{
			return StringUnitl::equalStrings(data, str);
		}

		e_bool operator!=(const e_char* str) const 
		{
			return !StringUnitl::equalStrings(data, str);
		}

		StaticString<size> operator +(const e_char* rhs)
		{
			return StaticString<size>(*this, rhs);
		}

		e_bool empty() const 
		{
			return data[0] == '\0'; 
		}

		operator const e_char*() const 
		{ 
			return data; 
		}
		e_char data[size];
	};

	class egal_string
	{
	public:
		explicit egal_string(IAllocator& allocator);
		egal_string(const egal_string& rhs, e_int32 start, e_int32 length);
		egal_string(const e_char* rhs, e_int32 length, IAllocator& allocator);
		egal_string(const egal_string& rhs);
		egal_string(const e_char* rhs, IAllocator& allocator);
		~egal_string();

		e_void resize(e_int32 size);
		e_char* getData() { return m_cstr; }
		e_char operator[](e_int32 index) const;
		e_void set(const e_char* rhs, e_int32 size);
		e_void operator=(const egal_string& rhs);
		e_void operator=(const e_char* rhs);
		e_bool operator!=(const egal_string& rhs) const;
		e_bool operator!=(const e_char* rhs) const;
		e_bool operator==(const egal_string& rhs) const;
		e_bool operator==(const e_char* rhs) const;
		e_bool operator<(const egal_string& rhs) const;
		e_bool operator>(const egal_string& rhs) const;
		e_int32 length() const { return m_size; }
		const e_char* c_str() const { return m_cstr; }
		egal_string substr(e_int32 start, e_int32 length) const;
		egal_string& cat(const e_char* value, e_int32 length);
		egal_string& cat(e_float value);
		egal_string& cat(e_char* value);
		egal_string& cat(const e_char* value);

		template <class V> egal_string& cat(V value)
		{
			e_char tmp[30];
			StringUnitl::toCString(value, tmp, 30);
			cat(tmp);
			return *this;
		}

		IAllocator& m_allocator;
	private:
		e_int32 m_size;
		e_char* m_cstr;
	};

}

#endif
