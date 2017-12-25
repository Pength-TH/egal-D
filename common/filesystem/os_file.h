#ifndef _os_file_h_
#define _os_file_h_

#include "common/filesystem/ifile_device.h"

namespace egal
{
	struct IAllocator;
	namespace FS
	{
		//不同的平台实现各自的方法
		class OsFile
		{
		public:
			OsFile();
			~OsFile();

			e_bool open(const e_char* Archive, Mode mode);
			e_void close();
			e_void flush();

			e_bool write(const e_void* data, size_t size);
			e_bool writeText(const e_char* text);
			e_bool read(e_void* data, size_t size);

			size_t size();
			size_t pos();

			e_bool seek(SeekMode base, size_t pos);

			OsFile& operator <<(const e_char* text);
			OsFile& operator <<(e_char c) { write(&c, sizeof(c)); return *this; }
			OsFile& operator <<(e_int32 value);
			OsFile& operator <<(e_uint32 value);
			OsFile& operator <<(e_uint64 value);
			OsFile& operator <<(float value);

			static e_bool fileExists(const e_char* Archive);

		private:
			e_void* m_handle;
		};
	}
}
#endif
