#ifndef _file_device_h_
#define _file_device_h_

#include "common/filesystem/ifile_device.h"
#include "common/filesystem/os_file.h"
#include "common/allocator/proxy_allocator.h"
#include "common/filesystem/stb/mf_resource.h"

#include "common/stl/delegate.h"
#include "common/stl/hash_map.h"

namespace egal
{
	namespace FS
	{
		/**DiskFileDevice*/
		class DiskFileDevice : public IFileDevice
		{
		public:
			DiskFileDevice(const e_char* name, const e_char* base_path, IAllocator& allocator);

			IFile* createFile(IFile* child) override;
			e_void destroyFile(IFile* file) override;
			const e_char* getBasePath() const { return m_base_path; }
			e_void setBasePath(const e_char* Archive);
			const e_char* name() const override { return m_name; }

		private:
			IAllocator& m_allocator;
			e_char m_base_path[MAX_PATH_LENGTH];
			e_char m_name[20];
		};

		/**PakFileDevice*/
		class PackFileDevice : public IFileDevice
		{
			friend class PackFile;
		public:
			explicit PackFileDevice(IAllocator& allocator);
			~PackFileDevice();

			IFile* createFile(IFile* child) override;
			e_void destroyFile(IFile* file) override;
			const e_char* name() const override { return "pack"; }
			e_bool mount(const e_char* Archive);

		private:
			struct PackFileInfo
			{
				e_uint64 offset;
				e_uint64 size;
			};

			THashMap<e_uint32, PackFileInfo> m_files;
			size_t m_offset;
			OsFile m_file;
			IAllocator& m_allocator;
		};

		/**MemoryFileDevice*/
		class MemoryFileDevice : public IFileDevice
		{
		public:
			explicit MemoryFileDevice(IAllocator& allocator) : m_allocator(allocator) {}

			e_void destroyFile(IFile* file) override;
			IFile* createFile(IFile* child) override;

			const e_char* name() const override { return "memory"; }

		private:
			IAllocator& m_allocator;
		};

		/**ResourceFileDevice*/
		class ResourceFileDevice : public IFileDevice
		{
		public:
			explicit ResourceFileDevice(IAllocator& allocator)
				: m_allocator(allocator)
			{
			}

			e_void destroyFile(IFile* file) override;
			IFile* createFile(IFile* child) override;
			int getResourceFilesCount() const;
			const mf_resource* getResource(int index) const;

			const e_char* name() const override { return "resource"; }

		private:
			IAllocator& m_allocator;
		};

		enum class EventType
		{
			OPEN_BEGIN = 0,
			OPEN_FINISHED,
			CLOSE_BEGIN,
			CLOSE_FINISHED,
			READ_BEGIN,
			READ_FINISHED,
			WRITE_BEGIN,
			WRITE_FINISHED,
			SIZE_BEGIN,
			SIZE_FINISHED,
			SEEK_BEGIN,
			SEEK_FINISHED,
			POS_BEGIN,
			POS_FINISHED
		};

		struct Event
		{
			EventType type;
			uintptr handle;
			const e_char* path;
			e_int32 ret;
			e_int32 param;
		};

		/**FileEventsDevice*/
		class FileEventsDevice : public IFileDevice
		{
		public:
			explicit FileEventsDevice(IAllocator& allocator) : m_allocator(allocator) {}

			typedef TDelegate<e_void(const Event&)>  EventCallback;

			EventCallback OnEvent;

			e_void destroyFile(IFile* file) override;
			IFile* createFile(IFile* child) override;

			const e_char* name() const override { return "events"; }

		private:
			IAllocator& m_allocator;
		};
	}
}
#endif
