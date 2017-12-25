#ifndef _resource_public_h_
#define _resource_public_h_

#include "common/egal-d.h"
#include "common/thread/sync.h"

namespace egal
{
	enum ResourceTypeDefine
	{
		RTD_DEFAULT = 0,
		RTD_TEXTURE,
		RTD_MATERIAL,
		RTD_MESH,
		RTD_SKELETON,
		RTD_ANIMATION,
		RTD_TERRAIN,
		RTD_PARTICLE,
		RTD_TAG,
		RTD_LEVEL,
		RTD_COLLISION,
		RTD_SHADER,
		RTD_MODEL,

		RTD_EDITOR,
		RTD_END
	};

	struct ResourceType
	{
		ResourceType() : type(0) {}
		explicit ResourceType(const e_char* type_name);
		e_uint32 type;

		e_bool operator !=(const ResourceType& rhs) const { return rhs.type != type; }
		e_bool operator ==(const ResourceType& rhs) const { return rhs.type == type; }

		inline e_bool isValid(ResourceType type) { return type.type != 0; }
	};
	const ResourceType INVALID_RESOURCE_TYPE("");

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
