
#include "common/filesystem/binary.h"
#include "common/egal-d.h"

namespace egal
{
	WriteBinary::WriteBinary(IAllocator& allocator)
		: m_allocator(&allocator)
		, m_data(nullptr)
		, m_size(0)
		, m_pos(0)
	{}

	WriteBinary::WriteBinary(void* data, int size)
		: m_data(data)
		, m_size(size)
		, m_allocator(nullptr)
		, m_pos(0)
	{
	}

	WriteBinary::WriteBinary(const WriteBinary& blob, IAllocator& allocator)
		: m_allocator(&allocator)
		, m_pos(blob.m_pos)
	{
		if (blob.m_size > 0)
		{
			m_data = allocator.allocate(blob.m_size);
			StringUnitl::copyMemory(m_data, blob.m_data, blob.m_size);
			m_size = blob.m_size;
		}
		else
		{
			m_data = nullptr;
			m_size = 0;
		}
	}

	WriteBinary::WriteBinary(const ReadBinary& blob, IAllocator& allocator)
		: m_allocator(&allocator)
		, m_pos(blob.getSize())
	{
		if (blob.getSize() > 0)
		{
			m_data = allocator.allocate(blob.getSize());
			StringUnitl::copyMemory(m_data, blob.getData(), blob.getSize());
			m_size = blob.getSize();
		}
		else
		{
			m_data = nullptr;
			m_size = 0;
		}
	}

	WriteBinary::~WriteBinary()
	{
		if (m_allocator) m_allocator->deallocate(m_data);
	}

	WriteBinary& WriteBinary::operator << (const char* str)
	{
		write(str, StringUnitl::stringLength(str));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (e_uint64 value)
	{
		char tmp[40];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (e_int64 value)
	{
		char tmp[40];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (e_int32 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}
	WriteBinary& WriteBinary::operator << (e_uint32 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (e_int16 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}
	WriteBinary& WriteBinary::operator << (e_uint16 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (e_int8 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}
	WriteBinary& WriteBinary::operator << (e_uint8 value)
	{
		char tmp[20];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp));
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary& WriteBinary::operator << (float value)
	{
		char tmp[30];
		StringUnitl::toCString(value, tmp, TlengthOf(tmp), 6);
		write(tmp, StringUnitl::stringLength(tmp));
		return *this;
	}

	WriteBinary::WriteBinary(const WriteBinary& rhs)
	{
		m_allocator = rhs.m_allocator;
		m_pos = rhs.m_pos;
		if (rhs.m_size > 0)
		{
			m_data = m_allocator->allocate(rhs.m_size);
			StringUnitl::copyMemory(m_data, rhs.m_data, rhs.m_size);
			m_size = rhs.m_size;
		}
		else
		{
			m_data = nullptr;
			m_size = 0;
		}
	}

	void WriteBinary::operator =(const WriteBinary& rhs)
	{
		ASSERT(rhs.m_allocator);
		if (m_allocator) m_allocator->deallocate(m_data);

		m_allocator = rhs.m_allocator;
		m_pos = rhs.m_pos;
		if (rhs.m_size > 0)
		{
			m_data = m_allocator->allocate(rhs.m_size);
			StringUnitl::copyMemory(m_data, rhs.m_data, rhs.m_size);
			m_size = rhs.m_size;
		}
		else
		{
			m_data = nullptr;
			m_size = 0;
		}
	}

	void WriteBinary::write(const String& string)
	{
		writeString(string.c_str());
	}

	void WriteBinary::write(const void* data, int size)
	{
		if (!size) return;

		if (m_pos + size > m_size)
		{
			reserve((m_pos + size) << 1);
		}
		StringUnitl::copyMemory((e_uint8*)m_data + m_pos, data, size);
		m_pos += size;
	}

	void WriteBinary::writeString(const char* string)
	{
		if (string)
		{
			e_int32 size = StringUnitl::stringLength(string) + 1;
			write(size);
			write(string, size);
		}
		else
		{
			write((e_int32)0);
		}
	}


	void WriteBinary::clear()
	{
		m_pos = 0;
	}

	void WriteBinary::reserve(int size)
	{
		if (size <= m_size) return;

		ASSERT(m_allocator);
		e_uint8* tmp = (e_uint8*)m_allocator->allocate(size);
		StringUnitl::copyMemory(tmp, m_data, m_size);
		m_allocator->deallocate(m_data);
		m_data = tmp;
		m_size = size;
	}

	void WriteBinary::resize(int size)
	{
		m_pos = size;
		if (size <= m_size) return;

		ASSERT(m_allocator);
		e_uint8* tmp = (e_uint8*)m_allocator->allocate(size);
		StringUnitl::copyMemory(tmp, m_data, m_size);
		m_allocator->deallocate(m_data);
		m_data = tmp;
		m_size = size;
	}

	ReadBinary::ReadBinary(const void* data, int size)
		: m_data((const e_uint8*)data)
		, m_size(size)
		, m_pos(0)
	{}

	ReadBinary::ReadBinary(const WriteBinary& blob)
		: m_data((const e_uint8*)blob.getData())
		, m_size(blob.getPos())
		, m_pos(0)
	{}

	const void* ReadBinary::skip(int size)
	{
		auto* pos = m_data + m_pos;
		m_pos += size;
		if (m_pos > m_size)
		{
			m_pos = m_size;
		}

		return (const void*)pos;
	}

	bool ReadBinary::read(void* data, int size)
	{
		if (m_pos + (int)size > m_size)
		{
			for (e_int32 i = 0; i < size; ++i)
				((unsigned char*)data)[i] = 0;
			return false;
		}
		if (size)
		{
			StringUnitl::copyMemory(data, ((char*)m_data) + m_pos, size);
		}
		m_pos += size;
		return true;
	}

	bool ReadBinary::read(String& str)
	{
		e_int32 size;
		read(size);
		str.resize(size);
		bool res = read(str.getData(), size);
		return res;
	}

	bool ReadBinary::readString(char* data, int max_size)
	{
		e_int32 size;
		read(size);
		ASSERT(size <= max_size);
		bool res = read(data, size < max_size ? size : max_size);
		for (int i = max_size; i < size; ++i)
		{
			readChar();
		}
		return res && size <= max_size;
	}
}
