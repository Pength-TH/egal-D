#ifndef _audio_manager_h_
#define _audio_manager_h_
#pragma once

#include "runtime/Resource/resource_define.h"
#include "runtime/Resource/resource_public.h"
#include "runtime/Resource/resource_manager.h"

namespace egal
{
	class Audio : public Resource
	{
	public:
		Clip(const ArchivePath& path, ResourceManagerBase& manager, IAllocator& allocator)
			: Resource(path, manager, allocator)
			, m_data(allocator)
		{
		}

		e_void unload() override;
		e_bool load(FS::IFile& file) override;
		e_int32 getChannels() const { return m_channels; }
		e_int32 getSampleRate() const { return m_sample_rate; }
		e_int32 getSize() const { return m_data.size() * sizeof(m_data[0]); }
		e_uint16* getData() { return &m_data[0]; }
		e_float getLengthSeconds() const { return m_data.size() / e_float(m_channels * m_sample_rate); }

	private:
		e_int32 m_channels;
		e_int32 m_sample_rate;
		Array<e_uint16> m_data;
	};

	class AudioManager : public ResourceManagerBase
	{
	public:
		explicit ClipManager(IAllocator& allocator)
			: ResourceManagerBase(allocator)
			, m_allocator(allocator)
		{
		}

		~ClipManager() {}

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
	};

}

#endif
