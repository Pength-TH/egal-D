#ifndef _binary_h_
#define _binary_h_

#include "common/type.h"
#include "common/egal_string.h"
namespace egal
{
	class ReadBinary;
	class WriteBinary
	{
	public:
		explicit WriteBinary(IAllocator& allocator);
		WriteBinary(void* data, int size);
		WriteBinary(const WriteBinary& rhs);
		WriteBinary(const WriteBinary& blob, IAllocator& allocator);
		WriteBinary(const ReadBinary& blob, IAllocator& allocator);
		void operator =(const WriteBinary& rhs);
		~WriteBinary();

		void resize(int size);
		void reserve(int size);
		const void* getData() const { return m_data; }
		void* getMutableData() { return m_data; }
		int getPos() const { return m_pos; }
		void write(const String& string);
		void write(const void* data, int size);
		void writeString(const char* string);

		template <class T> void write(const T& value);
		void clear();

		// Class type saving.
		template <typename _Ty>
		void operator<<(const _Ty& _ty) 
		{
			write(_ty);
		}

		WriteBinary& operator << (const char* str);
		WriteBinary& operator << (e_uint64 value);
		WriteBinary& operator << (e_int64 value);
		WriteBinary& operator << (e_int32 value);
		WriteBinary& operator << (e_uint32 value);
		WriteBinary& operator << (e_int16 value);
		WriteBinary& operator << (e_uint16 value);
		WriteBinary& operator << (e_int8 value);
		WriteBinary& operator << (e_uint8 value);
		WriteBinary& operator << (float value);

	private:
		void* m_data;
		int m_size;
		int m_pos;
		IAllocator* m_allocator;
	};

	template <class T> void WriteBinary::write(const T& value)
	{
		write(&value, sizeof(T));
	}

	template <> inline void WriteBinary::write<bool>(const bool& value)
	{
		e_uint8 v = value;
		write(&v, sizeof(v));
	}

	class ReadBinary
	{
	public:
		ReadBinary(const void* data, int size);
		explicit ReadBinary(const WriteBinary& blob);

		bool read(String& string);
		bool read(void* data, int size);
		bool readString(char* data, int max_size);
		template <class T> void read(T& value) { read(&value, sizeof(T)); }
		template <class T> T read();
		const void* skip(int size);
		const void* getData() const { return (const void*)m_data; }
		int getSize() const { return m_size; }
		int getPosition() { return m_pos; }
		void setPosition(int pos) { m_pos = pos; }
		void rewind() { m_pos = 0; }
		e_uint8 readChar() { ++m_pos; return m_data[m_pos - 1]; }

	private:
		const e_uint8* m_data;
		int m_size;
		int m_pos;
	};

	template <class T> T ReadBinary::read()
	{
		T v;
		read(&v, sizeof(v));
		return v;
	}

	template <> inline bool ReadBinary::read<bool>()
	{
		e_uint8 v;
		read(&v, sizeof(v));
		return v != 0;
	}
}
#endif
