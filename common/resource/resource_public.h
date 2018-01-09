#ifndef _resource_public_h_
#define _resource_public_h_

#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/math/egal_math.h"
#include "common/stl/delegate.h"
#include "common/stl/thash_map.h"
#include "common/template.h"
#include "common/thread/sync.h"

#include "common/utils/singleton.h"
#include "common/utils/logger.h"
#include "common/egal_string.h"

namespace egal
{
	struct Archive
	{
	public:
		e_uint32 getHash() const { return m_id; }
		const e_char* c_str() const { return m_path; }
		e_bool isValid() const { return m_path[0] != '\0'; }

		e_char m_path[MAX_PATH_LENGTH];
		e_char m_dir[MAX_PATH_LENGTH];
		e_char m_basename[MAX_PATH_LENGTH];
		e_char m_extension[10];

		e_uint32 m_id;

		volatile e_uint32 m_ref_count;
	};
	const Archive INVALID_ARCHIVE = { -1 };

	class ArchivePath
	{
	public:
		ArchivePath();
		ArchivePath(const ArchivePath& rhs);
		ArchivePath(const e_char* s1, const e_char* s2);
		ArchivePath(const e_char* s1, const e_char* s2, const e_char* s3);

		explicit ArchivePath(e_uint32 hash);
		explicit ArchivePath(const e_char* path);

		e_void operator=(const ArchivePath& rhs);
		e_void operator=(const e_char* rhs);
		e_bool operator==(const ArchivePath& rhs) const
		{
			return m_data->m_id == rhs.m_data->m_id;
		}
		e_bool operator!=(const ArchivePath& rhs) const
		{
			return m_data->m_id != rhs.m_data->m_id;
		}

		~ArchivePath();

		e_uint32 getHash() const { return m_data->m_id; }
		const e_char* c_str() const { return m_data->m_path; }

		e_int32 length() const;
		e_bool isValid() const { return m_data->isValid(); }
	public:
		Archive* m_data;
	};

	class ArchivePathManager : public Singleton<ArchivePathManager>
	{
		friend class ArchivePath;

	public:
		explicit ArchivePathManager(IAllocator& allocator);
		~ArchivePathManager();

		e_void clear();
		static const ArchivePath& getEmptyPath();

	private:
		Archive* getPath(e_uint32 hash, const e_char* path);
		Archive* getPath(e_uint32 hash);
		Archive* getPathMultithreadUnsafe(e_uint32 hash, const e_char* path);

		e_void incrementRefCount(Archive* path);
		e_void decrementRefCount(Archive* path);

	private:
		IAllocator& m_allocator;
		TMap<e_uint32, Archive*> m_paths;
		MT::SpinMutex m_mutex;
		ArchivePath* m_empty_path;
	};
}

#endif
