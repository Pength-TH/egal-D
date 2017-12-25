#ifndef _ifile_device_h_
#define _ifile_device_h_

#include "common/type.h"
#include "common/stl/delegate.h"

namespace egal
{
	namespace FS
	{
		struct Mode
		{
			enum Value
			{
				NONE = 0,
				READ = 0x1,
				WRITE = READ << 1,
				OPEN = WRITE << 1,
				CREATE = OPEN << 1,

				CREATE_AND_WRITE = CREATE | WRITE,
				OPEN_AND_READ = OPEN | READ
			};

			Mode() : value(0) {}
			Mode(Value _value) : value(_value) { }
			Mode(e_int32 _value) : value(_value) { }
			operator Value() const { return (Value)value; }
			e_int32 value;
		};

		struct SeekMode
		{
			enum Value
			{
				BEGIN = 0,
				END,
				CURRENT,
			};
			SeekMode(Value _value) : value(_value) {}
			SeekMode(e_uint32 _value) : value(_value) {}
			operator Value() { return (Value)value; }
			e_uint32 value;
		};
		struct IFileDevice;
		struct IFile
		{
			IFile() {}
			virtual ~IFile() {}

			virtual e_bool open(const e_char* path, Mode mode) = 0;
			virtual e_void close() = 0;

			virtual e_bool read(e_void* buffer, size_t size) = 0;
			virtual e_bool write(const e_void* buffer, size_t size) = 0;

			virtual const e_void* getBuffer() const = 0;
			virtual size_t size() = 0;

			virtual e_bool seek(SeekMode base, size_t pos) = 0;
			virtual size_t pos() = 0;

			IFile& operator << (const e_char* text);
			//e_void getContents(OutputBlob& blob);
			e_void release();
		protected:
			virtual IFileDevice& getDevice() = 0;

		};

		struct IFileDevice
		{
			IFileDevice() {}
			virtual ~IFileDevice() {}

			virtual IFile* createFile(IFile* child) = 0;
			virtual e_void destroyFile(IFile* file) = 0;

			virtual const e_char* name() const = 0;
		};
	
		typedef TDelegate<e_void(IFile&, e_bool)> ReadCallback;

		struct DeviceList
		{
			IFileDevice* m_devices[8];
		};

		class FileSystem
		{
		public:
			static const e_uint32 INVALID_ASYNC = 0xffffFFFF;
			static FileSystem* create(IAllocator& allocator);
			static e_void destroy(FileSystem* fs);

			FileSystem() {}
			virtual ~FileSystem() {}

			virtual e_bool mount(IFileDevice* device) = 0;
			virtual e_bool unMount(IFileDevice* device) = 0;

			virtual IFile* open(const DeviceList& device_list, const e_char* file, Mode mode) = 0;
			virtual e_uint32 openAsync(const DeviceList& device_list,
				const e_char* file,	int mode, const ReadCallback& call_back) = 0;

			virtual e_void cancelAsync(e_uint32 id) = 0;

			virtual e_void close(IFile& file) = 0;
			virtual e_void closeAsync(IFile& file) = 0;

			virtual e_void updateAsyncTransactions() = 0;

			virtual e_void fillDeviceList(const e_char* dev, DeviceList& device_list) = 0;
			virtual const DeviceList& getDefaultDevice() const = 0;
			virtual const DeviceList& getSaveGameDevice() const = 0;
			virtual const DeviceList& getMemoryDevice() const = 0;
			virtual const DeviceList& getDiskDevice() const = 0;


			virtual e_void setDefaultDevice(const e_char* dev) = 0;
			virtual e_void setSaveGameDevice(const e_char* dev) = 0;
			virtual e_bool hasWork() const = 0;
		};
	}

	//--------------------------------------------------------------
	//--------------------------------------------------------------
	//--------------------------------------------------------------
	extern FS::FileSystem* g_file_system;
	extern void init_file_system(IAllocator &allocator);
	extern void destory_file_system(IAllocator &m_allocator);
}
#endif
