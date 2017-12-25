
#include "common/resource/resource_public.h"
#include "common/egal-d.h"

namespace egal
{
	template<> ArchivePathManager*	 Singleton<ArchivePathManager>::msSingleton = 0;

	ResourceType::ResourceType(const e_char* type_name)
	{
		ASSERT(type_name[0] == 0 || (type_name[0] >= 'a' && type_name[0] <= 'z'));
		type = crc32(type_name);
	}

	ArchivePath::ArchivePath()
	{
		m_data = ArchivePathManager::getSingletonPtr()->getPath(0, "");
	}

	ArchivePath::ArchivePath(e_uint32 hash)
	{
		m_data = ArchivePathManager::getSingletonPtr()->getPath(hash);
		ASSERT(m_data);
	}
	ArchivePath::ArchivePath(const ArchivePath& rhs)
		: m_data(rhs.m_data)
	{
		ArchivePathManager::getSingletonPtr()->incrementRefCount(m_data);
	}

	ArchivePath::ArchivePath(const e_char* s1, const e_char* s2)
	{
		StaticString<MAX_PATH_LENGTH> tmp(s1, s2);
		e_char out[MAX_PATH_LENGTH];
		StringUnitl::normalize(tmp, out, TlengthOf(out));
		e_uint32 hash = crc32(out);
		m_data = ArchivePathManager::getSingletonPtr()->getPath(hash, out);
	}

	ArchivePath::ArchivePath(const e_char* s1, const e_char* s2, const e_char* s3)
	{
		StaticString<MAX_PATH_LENGTH> tmp(s1, s2, s3);
		e_char out[MAX_PATH_LENGTH];
		StringUnitl::normalize(tmp, out, TlengthOf(out));
		e_uint32 hash = crc32(out);
		m_data = ArchivePathManager::getSingletonPtr()->getPath(hash, out);
	}

	ArchivePath::ArchivePath(const e_char* path)
	{
		e_char tmp[MAX_PATH_LENGTH];
		size_t len = StringUnitl::stringLength(path);
		ASSERT(len < MAX_PATH_LENGTH);
		StringUnitl::normalize(path, tmp, (e_uint32)len + 1);
		e_uint32 hash = crc32(tmp);
		m_data = ArchivePathManager::getSingletonPtr()->getPath(hash, tmp);
	}

	ArchivePath::~ArchivePath()
	{
		ArchivePathManager::getSingletonPtr()->decrementRefCount(m_data);
	}

	e_int32 ArchivePath::length() const
	{
		return StringUnitl::stringLength(m_data->m_path);
	}

	e_void ArchivePath::operator =(const ArchivePath& rhs)
	{
		ArchivePathManager::getSingletonPtr()->decrementRefCount(m_data);
		m_data = m_data;
		ArchivePathManager::getSingletonPtr()->incrementRefCount(m_data);
	}

	e_void ArchivePath::operator =(const e_char* rhs)
	{
		ArchivePathManager::getSingletonPtr()->decrementRefCount(m_data);
		e_char tmp[MAX_PATH_LENGTH];
		size_t len = StringUnitl::stringLength(rhs);
		ASSERT(len < MAX_PATH_LENGTH);
		StringUnitl::normalize(rhs, tmp, (e_uint32)len + 1);
		e_uint32 hash = crc32(tmp);
		m_data = ArchivePathManager::getSingletonPtr()->getPath(hash, tmp);
	}

//*******************************************************************************
//*******************************************************************************
	ArchivePathManager::ArchivePathManager(IAllocator& allocator)
		: m_paths(allocator)
		, m_mutex(false)
		, m_allocator(allocator)
	{
		m_empty_path = _aligned_new(m_allocator, ArchivePath)();
	}

	ArchivePathManager::~ArchivePathManager()
	{
		_delete(m_allocator, m_empty_path);
		ASSERT(m_paths.size() == 0);
	}

	const ArchivePath& ArchivePathManager::getEmptyPath()
	{
		return *ArchivePathManager::getSingletonPtr()->m_empty_path;
	}

	//e_void ResourcePathManager::serialize(OutputBlob& serializer)
	//{
	//	MT::SpinLock lock(m_mutex);
	//	clear();
	//	serializer.write((e_int32)m_paths.size());
	//	for (e_int32 i = 0; i < m_paths.size(); ++i)
	//	{
	//		serializer.writeString(m_paths.at(i)->m_path);
	//	}
	//}

	//e_void ResourcePathManager::deserialize(InputBlob& serializer)
	//{
	//	MT::SpinLock lock(m_mutex);
	//	e_int32 size;
	//	serializer.read(size);
	//	for (e_int32 i = 0; i < size; ++i)
	//	{
	//		e_char path[MAX_PATH_LENGTH];
	//		serializer.readString(path, sizeof(path));
	//		e_uint32 hash = crc32(path);
	//		PathInternal* internal = getPathMultithreadUnsafe(hash, path);
	//		--internal->m_ref_count;
	//	}
	//}

	Archive* ArchivePathManager::getPath(e_uint32 hash)
	{
		MT::SpinLock lock(m_mutex);
		e_int32 index = m_paths.find(hash);
		if (index < 0)
		{
			return nullptr;
		}
		++m_paths.at(index)->m_ref_count;
		return m_paths.at(index);
	}

	Archive* ArchivePathManager::getPath(e_uint32 hash, const e_char* path)
	{
		MT::SpinLock lock(m_mutex);
		return getPathMultithreadUnsafe(hash, path);
	}

	e_void ArchivePathManager::clear()
	{
		for (e_int32 i = m_paths.size() - 1; i >= 0; --i)
		{
			if (m_paths.at(i)->m_ref_count == 0)
			{
				_delete(m_allocator, m_paths.at(i));
				m_paths.eraseAt(i);
			}
		}
	}

	Archive* ArchivePathManager::getPathMultithreadUnsafe(e_uint32 hash, const e_char* path)
	{
		e_int32 index = m_paths.find(hash);
		if (index < 0)
		{
			Archive* internal = _aligned_new(m_allocator, Archive);
			internal->m_ref_count = 1;
			internal->m_id = hash;
			StringUnitl::copyString(internal->m_path, path);
			m_paths.insert(hash, internal);
			return internal;
		}
		else
		{
			++m_paths.at(index)->m_ref_count;
			return m_paths.at(index);
		}
	}

	e_void ArchivePathManager::incrementRefCount(Archive* path)
	{
		MT::SpinLock lock(m_mutex);
		++path->m_ref_count;
	}

	e_void ArchivePathManager::decrementRefCount(Archive* path)
	{
		MT::SpinLock lock(m_mutex);
		--path->m_ref_count;
		if (path->m_ref_count == 0)
		{
			m_paths.erase(path->m_id);
			_delete(m_allocator, path);
		}
	}
}
