#include "common/egal_string.h"
#include "common/egal-d.h"

namespace egal
{
	egal_string::egal_string(IAllocator& allocator)
		: m_allocator(allocator)
	{
		m_cstr = nullptr;
		m_size = 0;
	}

	egal_string::egal_string(const egal_string& rhs, e_int32 start, e_int32 length)
		: m_allocator(rhs.m_allocator)
	{
		m_size = length - start <= rhs.m_size ? length : rhs.m_size - start;
		m_cstr = (e_char*)m_allocator.allocate((m_size + 1) * sizeof(e_char));
		memcpy(m_cstr, rhs.m_cstr + start, m_size * sizeof(e_char));
		m_cstr[m_size] = 0;
	}

	egal_string::egal_string(const e_char* rhs, e_int32 length, IAllocator& allocator)
		: m_allocator(allocator)
	{
		m_size = length;
		m_cstr = (e_char*)m_allocator.allocate((m_size + 1) * sizeof(e_char));
		memcpy(m_cstr, rhs, m_size * sizeof(e_char));
		m_cstr[m_size] = 0;
	}

	egal_string::egal_string(const egal_string& rhs)
		: m_allocator(rhs.m_allocator)
	{
		m_cstr = (e_char*)m_allocator.allocate((rhs.m_size + 1) * sizeof(e_char));
		m_size = rhs.m_size;
		memcpy(m_cstr, rhs.m_cstr, m_size * sizeof(e_char));
		m_cstr[m_size] = 0;
	}

	egal_string::egal_string(const e_char* rhs, IAllocator& allocator)
		: m_allocator(allocator)
	{
		m_size = StringUnitl::stringLength(rhs);
		m_cstr = (e_char*)m_allocator.allocate((m_size + 1) * sizeof(e_char));
		memcpy(m_cstr, rhs, sizeof(e_char) * (m_size + 1));
	}

	egal_string::~egal_string() { m_allocator.deallocate(m_cstr); }

	e_char egal_string::operator[](e_int32 index) const
	{
		ASSERT(index >= 0 && index < m_size);
		return m_cstr[index];
	}

	e_void egal_string::set(const e_char* rhs, e_int32 size)
	{
		if (rhs < m_cstr || rhs >= m_cstr + m_size)
		{
			m_allocator.deallocate(m_cstr);
			m_size = size;
			m_cstr = (e_char*)m_allocator.allocate(m_size + 1);
			memcpy(m_cstr, rhs, size);
			m_cstr[size] = '\0';
		}
	}

	e_void egal_string::operator=(const egal_string& rhs)
	{
		if (&rhs != this)
		{
			m_allocator.deallocate(m_cstr);
			m_cstr = (e_char*)m_allocator.allocate((rhs.m_size + 1) * sizeof(e_char));
			m_size = rhs.m_size;
			memcpy(m_cstr, rhs.m_cstr, sizeof(e_char) * (m_size + 1));
		}
	}

	e_void egal_string::operator=(const e_char* rhs)
	{
		if (rhs < m_cstr || rhs >= m_cstr + m_size)
		{
			m_allocator.deallocate(m_cstr);
			if (rhs)
			{
				m_size = StringUnitl::stringLength(rhs);
				m_cstr = (e_char*)m_allocator.allocate((m_size + 1) * sizeof(e_char));
				memcpy(m_cstr, rhs, sizeof(e_char) * (m_size + 1));
			}
			else
			{
				m_size = 0;
				m_cstr = nullptr;
			}
		}
	}

	e_bool egal_string::operator!=(const egal_string& rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs.m_cstr) != 0;
	}

	e_bool egal_string::operator!=(const e_char* rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs) != 0;
	}

	e_bool egal_string::operator==(const egal_string& rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs.m_cstr) == 0;
	}

	e_bool egal_string::operator==(const e_char* rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs) == 0;
	}

	e_bool egal_string::operator<(const egal_string& rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs.m_cstr) < 0;
	}


	e_bool egal_string::operator>(const egal_string& rhs) const
	{
		return StringUnitl::compareString(m_cstr, rhs.m_cstr) > 0;
	}


	egal_string egal_string::substr(e_int32 start, e_int32 length) const
	{
		return egal_string(*this, start, length);
	}


	e_void egal_string::resize(e_int32 size)
	{
		ASSERT(size > 0);
		if (size <= 0) return;

		m_cstr = (e_char*)m_allocator.reallocate(m_cstr, size);
		m_size = size;
		m_cstr[size - 1] = '\0';
	}


	egal_string& egal_string::cat(const e_char* value, e_int32 length)
	{
		if (value < m_cstr || value >= m_cstr + m_size)
		{
			if (m_cstr)
			{
				e_int32 new_size = m_size + length;
				e_char* new_cstr = (e_char*)m_allocator.allocate(new_size + 1);
				memcpy(new_cstr, m_cstr, sizeof(e_char) * m_size + 1);
				m_allocator.deallocate(m_cstr);
				m_cstr = new_cstr;
				m_size = new_size;
				StringUnitl::catNString(m_cstr, m_size + 1, value, length);
			}
			else
			{
				m_size = length;
				m_cstr = (e_char*)m_allocator.allocate(m_size + 1);
				StringUnitl::copyNString(m_cstr, m_size + 1, value, length);
			}
		}
		return *this;
	}


	egal_string& egal_string::cat(e_float value)
	{
		e_char tmp[40];
		StringUnitl::toCString(value, tmp, 30, 10);
		cat(tmp);
		return *this;
	}


	egal_string& egal_string::cat(e_char* value)
	{
		cat((const e_char*)value);
		return *this;
	}


	egal_string& egal_string::cat(const e_char* rhs)
	{
		if (rhs < m_cstr || rhs >= m_cstr + m_size)
		{
			if (m_cstr)
			{
				e_int32 new_size = m_size + StringUnitl::stringLength(rhs);
				e_char* new_cstr = (e_char*)m_allocator.allocate(new_size + 1);
				memcpy(new_cstr, m_cstr, sizeof(e_char) * m_size + 1);
				m_allocator.deallocate(m_cstr);
				m_cstr = new_cstr;
				m_size = new_size;
				StringUnitl::catString(m_cstr, m_size + 1, rhs);
			}
			else
			{
				m_size = StringUnitl::stringLength(rhs);
				m_cstr = (e_char*)m_allocator.allocate(m_size + 1);
				StringUnitl::copyString(m_cstr, m_size + 1, rhs);
			}
		}
		return *this;
	}

	namespace StringUnitl
	{

		static e_char makeLowercase(e_char c)
		{
			return c >= 'A' && c <= 'Z' ? c - ('A' - 'a') : c;
		}


		e_char makeUppercase(e_char c)
		{
			return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c;
		}


		e_int32 compareMemory(const e_void* lhs, const e_void* rhs, size_t size)
		{
			return memcmp(lhs, rhs, size);
		}


		e_int32 compareStringN(const e_char* lhs, const e_char* rhs, e_int32 length)
		{
			return strncmp(lhs, rhs, length);
		}


		e_int32 compareIStringN(const e_char* lhs, const e_char* rhs, e_int32 length)
		{
#ifdef _WIN32
			return _strnicmp(lhs, rhs, length);
#else
			return strncasecmp(lhs, rhs, length);
#endif
		}


		e_int32 compareString(const e_char* lhs, const e_char* rhs)
		{
			return strcmp(lhs, rhs);
		}


		e_bool equalStrings(const e_char* lhs, const e_char* rhs)
		{
			return strcmp(lhs, rhs) == 0;
		}


		e_bool equalIStrings(const e_char* lhs, const e_char* rhs)
		{
#ifdef _WIN32
			return _stricmp(lhs, rhs) == 0;
#else
			return strcasecmp(lhs, rhs) == 0;
#endif
		}


		e_int32 stringLength(const e_char* str)
		{
			return (e_int32)strlen(str);
		}


		e_void moveMemory(e_void* dest, const e_void* src, size_t count)
		{
			memmove(dest, src, count);
		}


		e_void setMemory(e_void* ptr, e_uint8 value, size_t num)
		{
			memset(ptr, value, num);
		}


		e_void copyMemory(e_void* dest, const e_void* src, size_t count)
		{
			memcpy(dest, src, count);
		}


		e_bool endsWith(const e_char* str, const e_char* substr)
		{
			e_int32 len = stringLength(str);
			e_int32 len2 = stringLength(substr);
			if (len2 > len) return false;
			return equalStrings(str + len - len2, substr);
		}


		const e_char* stristr(const e_char* haystack, const e_char* needle)
		{
			const e_char* c = haystack;
			while (*c)
			{
				if (makeLowercase(*c) == makeLowercase(needle[0]))
				{
					const e_char* n = needle + 1;
					const e_char* c2 = c + 1;
					while (*n && *c2)
					{
						if (makeLowercase(*n) != makeLowercase(*c2)) break;
						++n;
						++c2;
					}
					if (*n == 0) return c;
				}
				++c;
			}
			return nullptr;
		}


		e_bool makeLowercase(e_char* destination, e_int32 length, const e_char* source)
		{
			if (!source)
			{
				return false;
			}

			while (*source && length)
			{
				*destination = makeLowercase(*source);
				--length;
				++destination;
				++source;
			}
			if (length > 0)
			{
				*destination = 0;
				return true;
			}
			return false;
		}


		e_bool makeUppercase(e_char* destination, e_int32 length, const e_char* source)
		{
			if (!source)
			{
				return false;
			}

			while (*source && length)
			{
				*destination = makeUppercase(*source);
				--length;
				++destination;
				++source;
			}
			if (length > 0)
			{
				*destination = 0;
				return true;
			}
			return false;
		}


		const e_char* findSubstring(const e_char* haystack, const e_char* needle)
		{
			return strstr(haystack, needle);
		}


		e_bool copyNString(e_char* destination, e_int32 length, const e_char* source, e_int32 source_len)
		{
			ASSERT(length >= 0);
			ASSERT(source_len >= 0);
			if (!source)
			{
				return false;
			}

			while (*source && length > 1 && source_len)
			{
				*destination = *source;
				--source_len;
				--length;
				++destination;
				++source;
			}
			if (length > 0)
			{
				*destination = 0;
				return *source == '\0' || source_len == 0;
			}
			return false;
		}


		e_bool copyString(e_char* destination, e_int32 length, const e_char* source)
		{
			if (!source || length < 1)
			{
				return false;
			}

			while (*source && length > 1)
			{
				*destination = *source;
				--length;
				++destination;
				++source;
			}
			*destination = 0;
			return *source == '\0';
		}


		const e_char* reverseFind(const e_char* begin_haystack, const e_char* end_haystack, e_char c)
		{
			const e_char* tmp = end_haystack == nullptr ? nullptr : end_haystack - 1;
			if (tmp == nullptr)
			{
				tmp = begin_haystack;
				while (*tmp)
				{
					++tmp;
				}
			}
			while (tmp >= begin_haystack && *tmp != c)
			{
				--tmp;
			}
			if (tmp >= begin_haystack)
			{
				return tmp;
			}
			return nullptr;
		}


		e_bool catNString(e_char* destination,
			e_int32 length,
			const e_char* source,
			e_int32 source_len)
		{
			while (*destination && length)
			{
				--length;
				++destination;
			}
			return copyNString(destination, length, source, source_len);
		}


		e_bool catString(e_char* destination, e_int32 length, const e_char* source)
		{
			while (*destination && length)
			{
				--length;
				++destination;
			}
			return copyString(destination, length, source);
		}

		static e_void reverse(e_char* str, e_int32 length)
		{
			e_char* beg = str;
			e_char* end = str + length - 1;
			while (beg < end)
			{
				e_char tmp = *beg;
				*beg = *end;
				*end = tmp;
				++beg;
				--end;
			}
		}

		const e_char* fromCString(const e_char* input, e_int32 length, e_int32* value)
		{
			e_int64 val;
			const e_char* ret = fromCString(input, length, &val);
			*value = (e_int32)val;
			return ret;
		}

		const e_char* fromCString(const e_char* input, e_int32 length, e_int64* value)
		{
			if (length > 0)
			{
				const e_char* c = input;
				*value = 0;
				if (*c == '-')
				{
					++c;
					--length;
					if (!length)
					{
						return nullptr;
					}
				}
				while (length && *c >= '0' && *c <= '9')
				{
					*value *= 10;
					*value += *c - '0';
					++c;
					--length;
				}
				if (input[0] == '-')
				{
					*value = -*value;
				}
				return c;
			}
			return nullptr;
		}

		const e_char* fromCString(const e_char* input, e_int32 length, e_uint16* value)
		{
			e_uint32 tmp;
			const e_char* ret = fromCString(input, length, &tmp);
			*value = e_uint16(tmp);
			return ret;
		}

		const e_char* fromCString(const e_char* input, e_int32 length, e_uint32* value)
		{
			if (length > 0)
			{
				const e_char* c = input;
				*value = 0;
				if (*c == '-')
				{
					return nullptr;
				}
				while (length && *c >= '0' && *c <= '9')
				{
					*value *= 10;
					*value += *c - '0';
					++c;
					--length;
				}
				return c;
			}
			return nullptr;
		}


		const e_char* fromCString(const e_char* input, e_int32 length, e_uint64* value)
		{
			if (length > 0)
			{
				const e_char* c = input;
				*value = 0;
				if (*c == '-')
				{
					return nullptr;
				}
				while (length && *c >= '0' && *c <= '9')
				{
					*value *= 10;
					*value += *c - '0';
					++c;
					--length;
				}
				return c;
			}
			return nullptr;
		}


		e_bool toCStringPretty(e_int32 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			if (length > 0)
			{
				if (value < 0)
				{
					value = -value;
					--length;
					*c = '-';
					++c;
				}
				return toCStringPretty((e_uint32)value, c, length);
			}
			return false;
		}


		e_bool toCStringPretty(e_uint32 value, e_char* output, e_int32 length)
		{
			return toCStringPretty(e_uint64(value), output, length);
		}


		e_bool toCStringPretty(e_uint64 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			e_char* num_start = output;
			if (length > 0)
			{
				if (value == 0)
				{
					if (length == 1)
					{
						return false;
					}
					*c = '0';
					*(c + 1) = 0;
					return true;
				}
				e_int32 counter = 0;
				while (value > 0 && length > 1)
				{
					*c = value % 10 + '0';
					value = value / 10;
					--length;
					++c;
					if ((counter + 1) % 3 == 0 && length > 1 && value > 0)
					{
						*c = ' ';
						++c;
						counter = 0;
					}
					else
					{
						++counter;
					}
				}
				if (length > 0)
				{
					reverse(num_start, (e_int32)(c - num_start));
					*c = 0;
					return true;
				}
			}
			return false;
		}


		e_bool toCString(e_int32 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			if (length > 0)
			{
				if (value < 0)
				{
					value = -value;
					--length;
					*c = '-';
					++c;
				}
				return toCString((e_uint32)value, c, length);
			}
			return false;
		}

		e_bool toCString(e_int64 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			if (length > 0)
			{
				if (value < 0)
				{
					value = -value;
					--length;
					*c = '-';
					++c;
				}
				return toCString((e_uint64)value, c, length);
			}
			return false;
		}

		e_bool toCString(e_uint64 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			e_char* num_start = output;
			if (length > 0)
			{
				if (value == 0)
				{
					if (length == 1)
					{
						return false;
					}
					*c = '0';
					*(c + 1) = 0;
					return true;
				}
				while (value > 0 && length > 1)
				{
					*c = value % 10 + '0';
					value = value / 10;
					--length;
					++c;
				}
				if (length > 0)
				{
					reverse(num_start, (e_int32)(c - num_start));
					*c = 0;
					return true;
				}
			}
			return false;
		}

		e_bool toCStringHex(e_uint8 value, e_char* output, e_int32 length)
		{
			if (length < 2)
			{
				return false;
			}
			e_uint8 first = value / 16;
			if (first > 9)
			{
				output[0] = 'A' + first - 10;
			}
			else
			{
				output[0] = '0' + first;
			}
			e_uint8 second = value % 16;
			if (second > 9)
			{
				output[1] = 'A' + second - 10;
			}
			else
			{
				output[1] = '0' + second;
			}
			return true;
		}

		e_bool toCString(e_uint32 value, e_char* output, e_int32 length)
		{
			e_char* c = output;
			e_char* num_start = output;
			if (length > 0)
			{
				if (value == 0)
				{
					if (length == 1)
					{
						return false;
					}
					*c = '0';
					*(c + 1) = 0;
					return true;
				}
				while (value > 0 && length > 1)
				{
					*c = value % 10 + '0';
					value = value / 10;
					--length;
					++c;
				}
				if (length > 0)
				{
					reverse(num_start, (e_int32)(c - num_start));
					*c = 0;
					return true;
				}
			}
			return false;
		}

		static e_bool increment(e_char* output, e_char* end, e_bool is_space_after)
		{
			e_char carry = 1;
			e_char* c = end;
			while (c >= output)
			{
				if (*c == '.')
				{
					--c;
				}
				*c += carry;
				if (*c > '9')
				{
					*c = '0';
					carry = 1;
				}
				else
				{
					carry = 0;
					break;
				}
				--c;
			}
			if (carry && is_space_after)
			{
				e_char* c = end + 1; // including '\0' at the end of the string
				while (c >= output)
				{
					*(c + 1) = *c;
					--c;
				}
				++c;
				*c = '1';
				return true;
			}
			return !carry;
		}


		e_bool toCString(e_float value, e_char* output, e_int32 length, e_int32 after_point)
		{
			if (length < 2)
			{
				return false;
			}
			if (value < 0)
			{
				*output = '-';
				++output;
				value = -value;
				--length;
			}
			// e_int32 part
			e_int32 exponent = value == 0 ? 0 : (e_int32)log10(value);
			e_float num = value;
			e_char* c = output;
			if (num  < 1 && num > -1 && length > 1)
			{
				*c = '0';
				++c;
				--length;
			}
			else
			{
				while ((num >= 1 || exponent >= 0) && length > 1)
				{
					e_float power = (e_float)pow(10, exponent);
					e_char digit = (e_char)floor(num / power);
					num -= digit * power;
					*c = digit + '0';
					--exponent;
					--length;
					++c;
				}
			}
			// decimal part
			e_float dec_part = num;
			if (length > 1 && after_point > 0)
			{
				*c = '.';
				++c;
				--length;
			}
			else if (length > 0 && after_point == 0)
			{
				*c = 0;
				return true;
			}
			else
			{
				return false;
			}
			while (length > 1 && after_point > 0)
			{
				dec_part *= 10;
				e_char tmp = (e_char)dec_part;
				*c = tmp + '0';
				dec_part -= tmp;
				++c;
				--length;
				--after_point;
			}
			*c = 0;
			if ((e_int32)(dec_part + 0.5f))
				increment(output, c - 1, length > 1);
			return true;
		}


		e_char* trimmed(e_char* str)
		{
			while (*str && (*str == '\t' || *str == ' '))
			{
				++str;
			}
			return str;
		}


		e_bool startsWith(const e_char* str, const e_char* prefix)
		{
			const e_char* lhs = str;
			const e_char* rhs = prefix;
			while (*rhs && *lhs && *lhs == *rhs)
			{
				++lhs;
				++rhs;
			}

			return *rhs == 0;
		}

		void normalize(const e_char* path, e_char* out, e_uint32 max_size)
		{
			ASSERT(max_size > 0);
			e_uint32 i = 0;

			bool is_prev_slash = false;

			if (path[0] == '.' && (path[1] == '\\' || path[1] == '/'))
				++path;
#ifdef _WIN32
			if (path[0] == '\\' || path[0] == '/')
				++path;
#endif
			while (*path != '\0' && i < max_size)
			{
				bool is_current_slash = *path == '\\' || *path == '/';

				if (is_current_slash && is_prev_slash)
				{
					++path;
					continue;
				}

				*out = *path == '\\' ? '/' : *path;
#ifdef _WIN32
				*out = *path >= 'A' && *path <= 'Z' ? *path - 'A' + 'a' : *out;
#endif

				path++;
				out++;
				i++;

				is_prev_slash = is_current_slash;
			}
			(i < max_size ? *out : *(out - 1)) = '\0';
		}

		void getDir(e_char* dir, e_int32 max_length, const e_char* src)
		{
			copyString(dir, max_length, src);
			for (e_int32 i = stringLength(dir) - 1; i >= 0; --i)
			{
				if (dir[i] == '\\' || dir[i] == '/')
				{
					++i;
					dir[i] = '\0';
					return;
				}
			}
			dir[0] = '\0';
		}

		void getBasename(e_char* basename, e_int32 max_length, const e_char* src)
		{
			basename[0] = '\0';
			for (e_int32 i = stringLength(src) - 1; i >= 0; --i)
			{
				if (src[i] == '\\' || src[i] == '/' || i == 0)
				{
					if (src[i] == '\\' || src[i] == '/')
						++i;
					e_int32 j = 0;
					basename[j] = src[i];
					while (j < max_length - 1 && src[i + j] && src[i + j] != '.')
					{
						++j;
						basename[j] = src[j + i];
					}
					basename[j] = '\0';
					return;
				}
			}
		}

		void getFilename(e_char* filename, e_int32 max_length, const e_char* src)
		{
			for (e_int32 i = stringLength(src) - 1; i >= 0; --i)
			{
				if (src[i] == '\\' || src[i] == '/')
				{
					++i;
					copyString(filename, max_length, src + i);
					return;
				}
			}
			copyString(filename, max_length, src);
		}

		void getExtension(e_char* extension, e_int32 max_length, const e_char* src)
		{
			ASSERT(max_length > 0);
			for (e_int32 i = stringLength(src) - 1; i >= 0; --i)
			{
				if (src[i] == '.')
				{
					++i;
					copyString(extension, max_length, src + i);
					return;
				}
			}
			extension[0] = '\0';
		}

		bool hasExtension(const e_char* filename, const e_char* ext)
		{
			e_char tmp[20];
			getExtension(tmp, TlengthOf(tmp), filename);
			makeLowercase(tmp, TlengthOf(tmp), tmp);

			return equalStrings(tmp, ext);
		}

		bool isAbsolute(const e_char* path)
		{
			return path[0] != '\0' && path[1] == ':';
		}
	}
}
