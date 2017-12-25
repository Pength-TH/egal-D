#include "runtime/Resource/audio_manager.h"

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.cpp"
#include <cstdlib>

namespace egal
{
	e_void Audio::unload()
	{
		m_data.clear();
	}

	e_bool Audio::load(FS::IFile& file)
	{
		//PROFILE_FUNCTION();
		short* output = nullptr;
		auto res = stb_vorbis_decode_memory(
			(unsigned e_char*)file.getBuffer(), (e_int32)file.size(), &m_channels, &m_sample_rate, &output);
		if (res <= 0) return false;

		m_data.resize(res * m_channels);
		copyMemory(&m_data[0], output, res * m_channels * sizeof(m_data[0]));
		free(output);

		return true;
	}


	Resource* AudioManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Audio)(path, *this, m_allocator);
	}


	e_void AudioManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Audio*>(&resource));
	}
}
