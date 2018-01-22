#include "common/filesystem/os_file.h"
#include "common/egal-d.h"

#if PLATFORM_WINDOWS
namespace egal
{
	namespace FS
	{

		OsFile::OsFile()
		{
			m_handle = (e_void*)INVALID_HANDLE_VALUE;
			static_assert(sizeof(m_handle) >= sizeof(HANDLE), "");
		}

		OsFile::~OsFile()
		{
			ASSERT((HANDLE)m_handle == INVALID_HANDLE_VALUE);
		}

		e_bool OsFile::open(const e_char* Archive, Mode mode)
		{
			m_handle = (HANDLE)::CreateFile(Archive,
				(Mode::WRITE & mode) ? GENERIC_WRITE : 0 | ((Mode::READ & mode) ? GENERIC_READ : 0),
				(Mode::WRITE & mode) ? 0 : FILE_SHARE_READ,
				nullptr,
				(Mode::CREATE & mode) ? CREATE_ALWAYS : OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr);


			return INVALID_HANDLE_VALUE != m_handle;
		}

		e_void OsFile::flush()
		{
			ASSERT(nullptr != m_handle);
			FlushFileBuffers((HANDLE)m_handle);
		}

		e_void OsFile::close()
		{
			if (INVALID_HANDLE_VALUE != (HANDLE)m_handle)
			{
				::CloseHandle((HANDLE)m_handle);
				m_handle = (e_void*)INVALID_HANDLE_VALUE;
			}
		}

		e_bool OsFile::writeText(const e_char* text)
		{
			int len = StringUnitl::stringLength(text);
			return write(text, len);
		}

		e_bool OsFile::write(const e_void* data, size_t size)
		{
			ASSERT(INVALID_HANDLE_VALUE != (HANDLE)m_handle);
			size_t written = 0;
			::WriteFile((HANDLE)m_handle, data, (DWORD)size, (LPDWORD)&written, nullptr);
			return size == written;
		}

		e_bool OsFile::read(e_void* data, size_t size)
		{
			ASSERT(INVALID_HANDLE_VALUE != m_handle);
			DWORD readed = 0;
			e_bool success = ::ReadFile((HANDLE)m_handle, data, (DWORD)size, (LPDWORD)&readed, nullptr);
			return success && size == readed;
		}

		size_t OsFile::size()
		{
			ASSERT(INVALID_HANDLE_VALUE != m_handle);
			return ::GetFileSize((HANDLE)m_handle, 0);
		}

		e_bool OsFile::fileExists(const e_char* Archive)
		{
			DWORD dwAttrib = GetFileAttributes(Archive);
			return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
				!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
		}

		size_t OsFile::pos()
		{
			ASSERT(INVALID_HANDLE_VALUE != m_handle);
			return ::SetFilePointer((HANDLE)m_handle, 0, nullptr, FILE_CURRENT);
		}

		e_bool OsFile::seek(SeekMode base, size_t pos)
		{
			ASSERT(INVALID_HANDLE_VALUE != m_handle);
			int dir = 0;
			switch (base)
			{
			case SeekMode::BEGIN:
				dir = FILE_BEGIN;
				break;
			case SeekMode::END:
				dir = FILE_END;
				break;
			case SeekMode::CURRENT:
				dir = FILE_CURRENT;
				break;
			default:
				ASSERT(false);
				break;
			}

			LARGE_INTEGER dist;
			dist.QuadPart = pos;
			return ::SetFilePointer((HANDLE)m_handle, dist.u.LowPart, &dist.u.HighPart, dir) != INVALID_SET_FILE_POINTER;
		}

		template <class T> 
		void OsFile::write(const T& value)
		{
			write(&value, sizeof(T));
		}

		OsFile& OsFile::operator <<(const e_char* text)
		{
			write(text, StringUnitl::stringLength(text));
			return *this;
		}

		OsFile& OsFile::operator <<(e_uint8 value)
		{
			e_char buf[20];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}

		OsFile& OsFile::operator <<(e_int16 value)
		{
			e_char buf[20];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}


		OsFile& OsFile::operator <<(e_uint16 value)
		{
			e_char buf[20];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}

		OsFile& OsFile::operator <<(e_int32 value)
		{
			e_char buf[20];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}


		OsFile& OsFile::operator <<(e_uint32 value)
		{
			e_char buf[20];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}


		OsFile& OsFile::operator <<(e_uint64 value)
		{
			e_char buf[30];
			StringUnitl::toCString(value, buf, TlengthOf(buf));
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}
		OsFile& OsFile::operator <<(String value)
		{
			write(value.c_str(), StringUnitl::stringLength(value.c_str()));
			return *this;
		}

		OsFile& OsFile::operator <<(float value)
		{
			e_char buf[128];
			StringUnitl::toCString(value, buf, TlengthOf(buf), 7);
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}

		OsFile& OsFile::operator <<(bool value)
		{
			e_char buf[128];
			StringUnitl::toCString(value, buf, TlengthOf(buf), 7);
			write(buf, StringUnitl::stringLength(buf));
			return *this;
		}
	}
}
#endif
