#include "common/filesystem/file_device.h"
#include "common/filesystem/stb/mf_resource.h"

#include "common/thread/atomic.h"
#include "common/thread/lock_free_fixed_queue.h"
#include "common/thread/sync.h"
#include "common/thread/task.h"
#include "common/thread/thread.h"
#include "common/thread/transaction.h"

#include "common/egal_string.h"

#include "common/template.h"

#include "common/egal-d.h"

const mf_resource* mf_resources = nullptr;
int mf_resources_count = 0;

namespace egal
{
	namespace FS
	{

#define C_MAX_TRANS 16

#pragma region IFile
		enum TransFlags
		{
			TE_CLOSE = 0,
			TE_SUCCESS = 0x1,
			TE_IS_OPEN = TE_SUCCESS << 1,
			TE_FAIL = TE_IS_OPEN << 1,
			TE_CANCELED = TE_FAIL << 1
		};

		struct AsyncItem
		{
			IFile* m_file;
			ReadCallback m_cb;
			Mode m_mode;
			e_uint32 m_id;
			e_char m_path[MAX_PATH_LENGTH];
			e_uint8 m_flags;
		};

		typedef MT::Transaction<AsyncItem> AsynTrans;
		typedef MT::LockFreeFixedQueue<AsynTrans, C_MAX_TRANS> TransQueue;
		typedef TQueue<AsynTrans*, C_MAX_TRANS> InProgressQueue;
		typedef TArrary<AsyncItem> ItemsTable;
		typedef TArrary<IFileDevice*> DevicesTable;

		e_void IFile::release()
		{
			getDevice().destroyFile(this);
		}

		IFile& IFile::operator<<(const e_char* text)
		{
			write(text, StringUnitl::stringLength(text));
			return *this;
		}

		//e_void IFile::getContents(OutputBlob& blob)
		//{
		//	size_t tmp = size();
		//	blob.resize((int)tmp);
		//	read(blob.getMutableData(), tmp);
		//}

		class FSTask : public MT::Task
		{
		public:
			FSTask(TransQueue* queue, IAllocator& allocator)
				: MT::Task(allocator)
				, m_trans_queue(queue)
			{
			}

			~FSTask() = default;

			int task() override
			{
				while (!m_trans_queue->isAborted())
				{
					//PROFILE_BLOCK("transaction"); todo
					AsynTrans* tr = m_trans_queue->pop(true);
					if (!tr) break;

					if ((tr->data.m_flags & TE_IS_OPEN) == TE_IS_OPEN)
					{
						tr->data.m_flags |=
							tr->data.m_file->open(tr->data.m_path, tr->data.m_mode) ? TE_SUCCESS : TE_FAIL;
					}
					else if ((tr->data.m_flags & TE_CLOSE) == TE_CLOSE)
					{
						tr->data.m_file->close();
						tr->data.m_file->release();
						tr->data.m_file = nullptr;
					}
					tr->setCompleted();
				}
				return 0;
			}

			e_void stop() { m_trans_queue->abort(); }

		private:
			TransQueue* m_trans_queue;
		};


		class FileSystemImpl : public FileSystem
		{
		public:
			explicit FileSystemImpl(IAllocator& allocator)
				: m_allocator(allocator)
				, m_pending(m_allocator)
				, m_devices(m_allocator)
				, m_in_progress(m_allocator)
				, m_last_id(0)
			{
				m_disk_device.m_devices[0] = nullptr;
				m_memory_device.m_devices[0] = nullptr;
				m_default_device.m_devices[0] = nullptr;
				m_save_game_device.m_devices[0] = nullptr;
				m_task = _aligned_new(m_allocator, FSTask)(&m_transaction_queue, m_allocator);
				m_task->create("FSTask");
			}

			~FileSystemImpl()
			{
				m_task->stop();
				m_task->destroy();
				_delete(m_allocator, m_task);
				while (!m_in_progress.empty())
				{
					auto* trans = m_in_progress.front();
					m_in_progress.pop();
					if (trans->data.m_file) close(*trans->data.m_file);
				}
				for (auto& i : m_pending)
				{
					close(*i.m_file);
				}
			}

			BaseProxyAllocator& getAllocator() { return m_allocator; }


			e_bool hasWork() const override { return !m_in_progress.empty() || !m_pending.empty(); }


			e_bool mount(IFileDevice* device) override
			{
				for (int i = 0; i < m_devices.size(); i++)
				{
					if (m_devices[i] == device)
					{
						return false;
					}
				}

				if (StringUnitl::equalStrings(device->name(), "memory"))
				{
					m_memory_device.m_devices[0] = device;
					m_memory_device.m_devices[1] = nullptr;
				}
				else if (StringUnitl::equalStrings(device->name(), "disk"))
				{
					m_disk_device.m_devices[0] = device;
					m_disk_device.m_devices[1] = nullptr;
				}

				m_devices.push_back(device);
				return true;
			}

			e_bool unMount(IFileDevice* device) override
			{
				for (int i = 0; i < m_devices.size(); i++)
				{
					if (m_devices[i] == device)
					{
						m_devices.eraseFast(i);
						return true;
					}
				}

				return false;
			}


			IFile* createFile(const DeviceList& device_list)
			{
				IFile* prev = nullptr;
				for (int i = 0; i < TlengthOf(device_list.m_devices); ++i)
				{
					if (!device_list.m_devices[i])
					{
						break;
					}
					prev = device_list.m_devices[i]->createFile(prev);
				}
				return prev;
			}


			IFile* open(const DeviceList& device_list, const e_char* file, Mode mode) override
			{
				IFile* prev = createFile(device_list);

				if (prev)
				{
					if (prev->open(file, mode))
					{
						return prev;
					}
					else
					{
						prev->release();
						return nullptr;
					}
				}
				return nullptr;
			}


			e_uint32 openAsync(const DeviceList& device_list,
				const e_char* file,
				int mode,
				const ReadCallback& call_back) override
			{
				IFile* prev = createFile(device_list);

				if (prev)
				{
					AsyncItem& item = m_pending.emplace();

					item.m_file = prev;
					item.m_cb = call_back;
					item.m_mode = mode;
					StringUnitl::copyString(item.m_path, file);
					item.m_flags = TE_IS_OPEN;
					item.m_id = m_last_id;
					++m_last_id;
					if (m_last_id == INVALID_ASYNC) m_last_id = 0;
					return item.m_id;
				}

				return INVALID_ASYNC;
			}


			e_void cancelAsync(e_uint32 id) override
			{
				if (id == INVALID_ASYNC) return;

				for (int i = 0, c = m_pending.size(); i < c; ++i)
				{
					if (m_pending[i].m_id == id)
					{
						m_pending[i].m_flags |= TE_CANCELED;
						return;
					}
				}

				for (auto iter = m_in_progress.begin(), end = m_in_progress.end(); iter != end; ++iter)
				{
					if (iter.value()->data.m_id == id)
					{
						iter.value()->data.m_flags |= TE_CANCELED;
						return;
					}
				}

			}


			e_void setDefaultDevice(const e_char* dev) override { fillDeviceList(dev, m_default_device); }


			e_void fillDeviceList(const e_char* dev, DeviceList& device_list) override
			{
				const e_char* token = nullptr;

				int device_index = 0;
				const e_char* end = dev + StringUnitl::stringLength(dev);

				while (end > dev)
				{
					token = StringUnitl::reverseFind(dev, token, ':');
					e_char device[32];
					if (token)
					{
						StringUnitl::copyNString(device, (int)sizeof(device), token + 1, int(end - token - 1));
					}
					else
					{
						StringUnitl::copyNString(device, (int)sizeof(device), dev, int(end - dev));
					}
					end = token;
					device_list.m_devices[device_index] = getDevice(device);
					ASSERT(device_list.m_devices[device_index]);
					++device_index;
				}
				device_list.m_devices[device_index] = nullptr;
			}


			const DeviceList& getMemoryDevice() const override { return m_memory_device; }
			const DeviceList& getDiskDevice() const override { return m_disk_device; }
			e_void setSaveGameDevice(const e_char* dev) override { fillDeviceList(dev, m_save_game_device); }


			e_void close(IFile& file) override
			{
				file.close();
				file.release();
			}


			e_void closeAsync(IFile& file) override
			{
				AsyncItem& item = m_pending.emplace();

				item.m_file = &file;
				item.m_cb.bind<closeAsync>();
				item.m_mode = 0;
				item.m_flags = TE_CLOSE;
			}


			e_void updateAsyncTransactions() override
			{
				//PROFILE_FUNCTION(); todo
				while (!m_in_progress.empty())
				{
					AsynTrans* tr = m_in_progress.front();
					if (!tr->isCompleted()) break;

					//PROFILE_BLOCK("processAsyncTransaction"); todo
					m_in_progress.pop();

					if ((tr->data.m_flags & TE_CANCELED) == 0)
					{
						tr->data.m_cb.invoke(*tr->data.m_file, !!(tr->data.m_flags & TE_SUCCESS));
					}
					if ((tr->data.m_flags & (TE_SUCCESS | TE_FAIL)) != 0)
					{
						closeAsync(*tr->data.m_file);
					}
					m_transaction_queue.dealoc(tr);
				}

				e_int32 can_add = C_MAX_TRANS - m_in_progress.size();
				while (can_add && !m_pending.empty())
				{
					AsynTrans* tr = m_transaction_queue.alloc(false);
					if (tr)
					{
						AsyncItem& item = m_pending[0];
						tr->data.m_file = item.m_file;
						tr->data.m_cb = item.m_cb;
						tr->data.m_id = item.m_id;
						tr->data.m_mode = item.m_mode;
						StringUnitl::copyString(tr->data.m_path, sizeof(tr->data.m_path), item.m_path);
						tr->data.m_flags = item.m_flags;
						tr->reset();

						m_transaction_queue.push_back(tr, true);
						m_in_progress.push_back(tr);
						m_pending.erase(0);
					}
					can_add--;
				}
			}

			const DeviceList& getDefaultDevice() const override { return m_default_device; }

			const DeviceList& getSaveGameDevice() const override { return m_save_game_device; }

			IFileDevice* getDevice(const e_char* device)
			{
				for (int i = 0; i < m_devices.size(); ++i)
				{
					if (StringUnitl::equalStrings(m_devices[i]->name(), device)) return m_devices[i];
				}

				return nullptr;
			}

			static e_void closeAsync(IFile&, e_bool) {}

		private:
			BaseProxyAllocator m_allocator;
			FSTask* m_task;
			DevicesTable m_devices;

			ItemsTable m_pending;
			TransQueue m_transaction_queue;
			InProgressQueue m_in_progress;

			DeviceList m_disk_device;
			DeviceList m_memory_device;
			DeviceList m_default_device;
			DeviceList m_save_game_device;
			e_uint32 m_last_id;
		};

		FileSystem* FileSystem::create(IAllocator& allocator)
		{
			return _aligned_new(allocator, FileSystemImpl)(allocator);
		}

		e_void FileSystem::destroy(FileSystem* fs)
		{
			_delete(static_cast<FileSystemImpl*>(fs)->getAllocator().getSourceAllocator(), fs);
		}

#pragma endregion

#pragma region DiskFile&Device
		struct DiskFile : public IFile
		{
			DiskFile(IFile* fallthrough, DiskFileDevice& device, IAllocator& allocator)
				: m_device(device)
				, m_allocator(allocator)
				, m_fallthrough(fallthrough)
			{
				m_use_fallthrough = false;
			}

			~DiskFile()
			{
				if (m_fallthrough) m_fallthrough->release();
			}

			IFileDevice& getDevice() override
			{
				return m_device;
			}

			e_bool open(const e_char* path, Mode mode) override
			{
				e_char tmp[MAX_PATH_LENGTH];
				if (StringUnitl::stringLength(path) > 1 && path[1] == ':')
				{
					StringUnitl::copyString(tmp, path);
				}
				else
				{
					StringUnitl::copyString(tmp, m_device.getBasePath());
					StringUnitl::catString(tmp, path);
				}
				e_bool want_read = (mode & Mode::READ) != 0;
				if (want_read && !OsFile::fileExists(tmp) && m_fallthrough)
				{
					m_use_fallthrough = true;
					return m_fallthrough->open(path, mode);
				}
				return m_file.open(tmp, mode);
			}

			e_void close() override
			{
				if (m_fallthrough) m_fallthrough->close();
				m_file.close();
				m_use_fallthrough = false;
			}

			e_bool read(e_void* buffer, size_t size) override
			{
				if (m_use_fallthrough) return m_fallthrough->read(buffer, size);
				return m_file.read(buffer, size);
			}

			e_bool write(const e_void* buffer, size_t size) override
			{
				if (m_use_fallthrough) return m_fallthrough->write(buffer, size);
				return m_file.write(buffer, size);
			}

			e_void flush() override
			{
				if (m_use_fallthrough) return m_fallthrough->flush();
				return m_file.flush();
			}

			const e_void* getBuffer() const override
			{
				if (m_use_fallthrough) return m_fallthrough->getBuffer();
				return nullptr;
			}

			size_t size() override
			{
				if (m_use_fallthrough) return m_fallthrough->size();
				return m_file.size();
			}

			e_bool seek(SeekMode base, size_t pos) override
			{
				if (m_use_fallthrough) return m_fallthrough->seek(base, pos);
				return m_file.seek(base, pos);
			}

			size_t pos() override
			{
				if (m_use_fallthrough) return m_fallthrough->pos();
				return m_file.pos();
			}

			DiskFileDevice& m_device;
			IAllocator& m_allocator;
			OsFile m_file;
			IFile* m_fallthrough;
			e_bool m_use_fallthrough;
		};

		DiskFileDevice::DiskFileDevice(const e_char* name, const e_char* base_path, IAllocator& allocator)
			: m_allocator(allocator)
		{
			StringUnitl::copyString(m_name, name);
			StringUnitl::normalize(base_path, m_base_path, TlengthOf(m_base_path));
			if (m_base_path[0] != '\0') StringUnitl::catString(m_base_path, "/");
		}

		e_void DiskFileDevice::setBasePath(const e_char* e_char)
		{
			StringUnitl::normalize(e_char, m_base_path, TlengthOf(m_base_path));
			if (m_base_path[0] != '\0') StringUnitl::catString(m_base_path, "/");
		}

		e_void DiskFileDevice::destroyFile(IFile* file)
		{
			_delete(m_allocator, file);
		}

		IFile* DiskFileDevice::createFile(IFile* fallthrough)
		{
			return _aligned_new(m_allocator, DiskFile)(fallthrough, *this, m_allocator);
		}
#pragma endregion

#pragma region PackFile&Device
		class PackFile : public IFile
		{
		public:
			PackFile(PackFileDevice& device, IAllocator& allocator)
				: m_device(device)
				, m_local_offset(0)
			{
			}

			e_bool open(const e_char* path, Mode mode) override
			{
				char out[MAX_PATH_LENGTH];
				StringUnitl::normalize(path, out, TlengthOf(out));

				auto iter = m_device.m_files.find(crc32(out));
				if (iter == m_device.m_files.end()) return false;
				m_file = iter.value();
				m_local_offset = 0;
				return m_device.m_file.seek(SeekMode::BEGIN, (size_t)m_file.offset);
			}

			e_bool read(e_void* buffer, size_t size) override
			{
				if (m_device.m_offset != m_file.offset + m_local_offset)
				{
					if (!m_device.m_file.seek(FS::SeekMode::BEGIN, size_t(m_file.offset + m_local_offset)))
					{
						return false;
					}
				}
				m_local_offset += size;
				return m_device.m_file.read(buffer, size);
			}

			e_bool seek(SeekMode base, size_t pos) override
			{
				m_local_offset = pos;
				return m_device.m_file.seek(SeekMode::BEGIN, size_t(m_file.offset + pos));
			}

			IFileDevice& getDevice() override { return m_device; }
			e_void close() override { m_local_offset = 0; }
			e_bool write(const e_void* buffer, size_t size) override { ASSERT(false); return false; }
			const e_void* getBuffer() const override { return nullptr; }
			size_t size() override { return (size_t)m_file.size; }
			size_t pos() override { return m_local_offset; }
			e_void flush() override
			{

			}
		private:
			virtual ~PackFile() = default;

			PackFileDevice::PackFileInfo m_file;
			PackFileDevice& m_device;
			size_t m_local_offset;
		}; // class PackFile

		PackFileDevice::PackFileDevice(IAllocator& allocator)
			: m_allocator(allocator)
			, m_files(allocator)
		{
		}
		PackFileDevice::~PackFileDevice()
		{
			m_file.close();
		}

		e_bool PackFileDevice::mount(const e_char* e_char)
		{
			m_file.close();
			if (!m_file.open(e_char, Mode::OPEN_AND_READ)) return false;

			e_int32 count;
			m_file.read(&count, sizeof(count));
			for (int i = 0; i < count; ++i)
			{
				e_uint32 hash;
				m_file.read(&hash, sizeof(hash));
				PackFileInfo info;
				m_file.read(&info, sizeof(info));
				m_files.insert(hash, info);
			}
			m_offset = m_file.pos();
			return true;
		}

		e_void PackFileDevice::destroyFile(IFile* file)
		{
			_delete(m_allocator, file);
		}


		IFile* PackFileDevice::createFile(IFile*)
		{
			return _aligned_new(m_allocator, PackFile)(*this, m_allocator);
		}
#pragma endregion

#pragma region MemoryFile&Device
		class MemoryFile : public IFile
		{
		public:
			MemoryFile(IFile* file, MemoryFileDevice& device, IAllocator& allocator)
				: m_device(device)
				, m_buffer(nullptr)
				, m_size(0)
				, m_capacity(0)
				, m_pos(0)
				, m_file(file)
				, m_write(false)
				, m_allocator(allocator)
			{
			}

			~MemoryFile()
			{
				if (m_file)
				{
					m_file->release();
				}
				m_allocator.deallocate(m_buffer);
			}


			IFileDevice& getDevice() override
			{
				return m_device;
			}

			e_bool open(const e_char* path, Mode mode) override
			{
				ASSERT(!m_buffer); // reopen is not supported currently

				m_write = !!(mode & Mode::WRITE);
				if (m_file)
				{
					if (m_file->open(path, mode))
					{
						if (mode & Mode::READ)
						{
							m_capacity = m_size = m_file->size();
							m_buffer = (e_uint8*)m_allocator.allocate(sizeof(e_uint8) * m_size);
							m_file->read(m_buffer, m_size);
							m_pos = 0;
						}

						return true;
					}
				}
				else
				{
					if (mode & Mode::WRITE)
					{
						return true;
					}
				}

				return false;
			}

			e_void close() override
			{
				if (m_file)
				{
					if (m_write)
					{
						m_file->seek(SeekMode::BEGIN, 0);
						m_file->write(m_buffer, m_size);
					}
					m_file->close();
				}

				m_allocator.deallocate(m_buffer);
				m_buffer = nullptr;
			}

			e_bool read(e_void* buffer, size_t size) override
			{
				size_t amount = m_pos + size < m_size ? size : m_size - m_pos;
				StringUnitl::copyMemory(buffer, m_buffer + m_pos, (int)amount);
				m_pos += amount;
				return amount == size;
			}

			e_bool write(const e_void* buffer, size_t size) override
			{
				size_t pos = m_pos;
				size_t cap = m_capacity;
				size_t sz = m_size;
				if (pos + size > cap)
				{
					size_t new_cap = Math::maximum(cap * 2, pos + size);
					e_uint8* new_data = (e_uint8*)m_allocator.allocate(sizeof(e_uint8) * new_cap);
					StringUnitl::copyMemory(new_data, m_buffer, (int)sz);
					m_allocator.deallocate(m_buffer);
					m_buffer = new_data;
					m_capacity = new_cap;
				}

				StringUnitl::copyMemory(m_buffer + pos, buffer, (int)size);
				m_pos += size;
				m_size = pos + size > sz ? pos + size : sz;

				return true;
			}

			e_void flush() override
			{
			}

			const e_void* getBuffer() const override
			{
				return m_buffer;
			}

			size_t size() override
			{
				return m_size;
			}

			e_bool seek(SeekMode base, size_t pos) override
			{
				switch (base)
				{
				case SeekMode::BEGIN:
					ASSERT(pos <= (e_int32)m_size);
					m_pos = pos;
					break;
				case SeekMode::CURRENT:
					ASSERT(0 <= e_int32(m_pos + pos) && e_int32(m_pos + pos) <= e_int32(m_size));
					m_pos += pos;
					break;
				case SeekMode::END:
					ASSERT(pos <= (e_int32)m_size);
					m_pos = m_size - pos;
					break;
				default:
					ASSERT(0);
					break;
				}

				e_bool ret = m_pos <= m_size;
				m_pos = Math::minimum(m_pos, m_size);
				return ret;
			}

			size_t pos() override
			{
				return m_pos;
			}

		private:
			IAllocator& m_allocator;
			MemoryFileDevice& m_device;
			e_uint8* m_buffer;
			size_t m_size;
			size_t m_capacity;
			size_t m_pos;
			IFile* m_file;
			e_bool m_write;
		};

		e_void MemoryFileDevice::destroyFile(IFile* file)
		{
			_delete(m_allocator, file);
		}

		IFile* MemoryFileDevice::createFile(IFile* child)
		{
			return _aligned_new(m_allocator, MemoryFile)(child, *this, m_allocator);
		}
#pragma endregion

#pragma region ResourceFile&Device

		class ResourceFile : public IFile
		{
		public:
			ResourceFile(IFile* file, ResourceFileDevice& device, IAllocator& allocator)
				: m_device(device)
				, m_resource(nullptr)
			{
				ASSERT(file == nullptr);
			}

			~ResourceFile() = default;


			IFileDevice& getDevice() override { return m_device; }


			e_bool open(const e_char* path, Mode mode) override
			{
				ASSERT(!m_resource); // reopen is not supported currently
				ASSERT((mode & Mode::WRITE) == 0);
				if ((mode & Mode::WRITE) != 0) return false;
				int count = mf_get_all_resources_count();
				const mf_resource* resources = mf_get_all_resources();
				m_resource = nullptr;
				for (int i = 0; i < count; ++i)
				{
					const mf_resource* res = &resources[i];
					if (StringUnitl::equalIStrings(path, res->path))
					{
						m_resource = res;
						break;
					}
				}
				if (!m_resource) return false;
				m_pos = 0;
				return true;
			}

			e_void close() override
			{
				m_resource = nullptr;
			}

			e_bool read(e_void* buffer, size_t size) override
			{
				size_t amount = m_pos + size < m_resource->size ? size : m_resource->size - m_pos;
				StringUnitl::copyMemory(buffer, m_resource->value + m_pos, (int)amount);
				m_pos += amount;
				return amount == size;
			}

			e_bool write(const e_void* buffer, size_t size) override
			{
				ASSERT(false);
				return false;
			}

			e_void flush() override
			{

			}

			const e_void* getBuffer() const override { return m_resource->value; }

			size_t size() override { return m_resource->size; }

			e_bool seek(SeekMode base, size_t pos) override
			{
				switch (base)
				{
				case SeekMode::BEGIN:
					ASSERT(pos <= (e_int32)m_resource->size);
					m_pos = pos;
					break;
				case SeekMode::CURRENT:
					ASSERT(0 <= e_int32(m_pos + pos) && e_int32(m_pos + pos) <= e_int32(m_resource->size));
					m_pos += pos;
					break;
				case SeekMode::END:
					ASSERT(pos <= (e_int32)m_resource->size);
					m_pos = m_resource->size - pos;
					break;
				default: ASSERT(0); break;
				}

				e_bool ret = m_pos <= m_resource->size;
				m_pos = Math::minimum(m_pos, m_resource->size);
				return ret;
			}

			size_t pos() override { return m_pos; }

		private:
			ResourceFileDevice& m_device;
			const mf_resource* m_resource;
			size_t m_pos;
		};


		e_void ResourceFileDevice::destroyFile(IFile* file)
		{
			_delete(m_allocator, file);
		}


		const mf_resource* ResourceFileDevice::getResource(int index) const
		{
			return &mf_get_all_resources()[index];
		}


		int ResourceFileDevice::getResourceFilesCount() const
		{
			return mf_get_all_resources_count();
		}


		IFile* ResourceFileDevice::createFile(IFile* child)
		{
			return _aligned_new(m_allocator, ResourceFile)(child, *this, m_allocator);
		}
#pragma endregion 

#pragma region EventsFile&Device
		class EventsFile : public IFile
		{
		public:
			EventsFile& operator= (const EventsFile& rhs) = delete;
			EventsFile(const EventsFile& rhs) = delete;

			EventsFile(IFile& file, FileEventsDevice& device, FileEventsDevice::EventCallback& cb)
				: m_file(file)
				, m_cb(cb)
				, m_device(device)
			{
			}

			virtual ~EventsFile()
			{
				m_file.release();
			}


			IFileDevice& getDevice() override
			{
				return m_device;
			}


			e_bool open(const e_char* path, Mode mode) override
			{
				invokeEvent(EventType::OPEN_BEGIN, path, -1, mode);
				e_bool ret = m_file.open(path, mode);
				invokeEvent(EventType::OPEN_FINISHED, path, ret ? 1 : 0, mode);

				return ret;
			}


			e_void close() override
			{
				invokeEvent(EventType::CLOSE_BEGIN, "", -1, -1);
				m_file.close();

				invokeEvent(EventType::CLOSE_FINISHED, "", -1, -1);
			}


			e_bool read(e_void* buffer, size_t size) override
			{
				invokeEvent(EventType::READ_BEGIN, "", -1, (e_int32)size);
				e_bool ret = m_file.read(buffer, size);

				invokeEvent(EventType::READ_FINISHED, "", ret ? 1 : 0, (e_int32)size);
				return ret;
			}


			e_bool write(const e_void* buffer, size_t size) override
			{
				invokeEvent(EventType::WRITE_BEGIN, "", -1, (e_int32)size);
				e_bool ret = m_file.write(buffer, size);

				invokeEvent(EventType::WRITE_FINISHED, "", ret ? 1 : 0, (e_int32)size);
				return ret;
			}

			e_void flush() override
			{
				return m_file.flush();
			}

			const e_void* getBuffer() const override
			{
				return m_file.getBuffer();
			}


			size_t size() override
			{
				invokeEvent(EventType::SIZE_BEGIN, "", -1, -1);
				size_t ret = m_file.size();

				invokeEvent(EventType::SIZE_FINISHED, "", (e_int32)ret, -1);
				return ret;
			}


			e_bool seek(SeekMode base, size_t pos) override
			{
				invokeEvent(EventType::SEEK_BEGIN, "", (e_int32)pos, base);
				e_bool ret = m_file.seek(base, pos);

				invokeEvent(EventType::SEEK_FINISHED, "", (e_int32)ret, base);
				return ret;
			}


			size_t pos() override
			{
				invokeEvent(EventType::POS_BEGIN, "", -1, -1);
				size_t ret = m_file.pos();

				invokeEvent(EventType::POS_FINISHED, "", (e_int32)ret, -1);
				return ret;
			}


		private:

			e_void invokeEvent(EventType type, const e_char* path, e_int32 ret, e_int32 param)
			{
				Event event;
				event.type	 = type;
				event.handle = uintptr(this);
				event.path	 = path;
				event.ret	 = ret;
				event.param	 = param;

				m_cb.invoke(event);
			}

			FileEventsDevice&					m_device;
			IFile&								m_file;
			FileEventsDevice::EventCallback&	m_cb;
		};


		e_void FileEventsDevice::destroyFile(IFile* file)
		{
			_delete(m_allocator, file);
		}


		IFile* FileEventsDevice::createFile(IFile* child)
		{
			return _aligned_new(m_allocator, EventsFile)(*child, *this, OnEvent);
		}
#pragma endregion
	}

	/**init filesystem*/
#pragma region init
	FS::FileSystem*			g_file_system			= nullptr;
	FS::ResourceFileDevice* g_resource_file_device	= nullptr;
	FS::MemoryFileDevice*	g_mem_file_device		= nullptr;
	FS::DiskFileDevice*		g_disk_file_device		= nullptr;
	FS::PackFileDevice*		g_pack_file_device		= nullptr;

	void init_file_system(IAllocator &m_main_allocator)
	{
		if (g_file_system)
			return;

		e_char current_dir[MAX_PATH_LENGTH];
#ifdef PLATFORM_WINDOWS
		GetCurrentDirectory(sizeof(current_dir), current_dir);
#else
		current_dir[0] = '\0';
#endif

		g_file_system			= FS::FileSystem::create(m_main_allocator);

		g_mem_file_device		= _aligned_new(m_main_allocator, FS::MemoryFileDevice)(m_main_allocator);
		g_disk_file_device		= _aligned_new(m_main_allocator, FS::DiskFileDevice)("disk", current_dir, m_main_allocator);
		g_pack_file_device		= _aligned_new(m_main_allocator, FS::PackFileDevice)(m_main_allocator);
		g_resource_file_device  = _aligned_new(m_main_allocator, FS::ResourceFileDevice)(m_main_allocator);

		g_file_system->mount(g_mem_file_device);
		g_file_system->mount(g_disk_file_device);
		g_file_system->mount(g_resource_file_device);
		g_file_system->mount(g_pack_file_device);
		
		g_pack_file_device->mount("data.pak");

		g_file_system->setDefaultDevice("memory:disk:resource");
		g_file_system->setSaveGameDevice("memory:disk:resource");
	}

	const e_char* getDiskBasePath()
	{
		return g_disk_file_device->getBasePath();
	}

	void destory_file_system(IAllocator &m_allocator)
	{
		FS::FileSystem::destroy(g_file_system);
		_delete(m_allocator, g_mem_file_device);
		_delete(m_allocator, g_resource_file_device);
		_delete(m_allocator, g_disk_file_device);
		_delete(m_allocator, g_pack_file_device);
	}
#pragma endregion

}
