#include "common/resource/texture_manager.h"

namespace egal
{
	Texture::Texture(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& _allocator)
		: Resource(path, resource_manager, _allocator)
		, data_reference(0)
		, allocator(_allocator)
		, data(_allocator)
		, bytes_per_pixel(-1)
		, depth(-1)
		, layers(1)
	{
		bgfx_flags = 0;
		is_cubemap = false;
		handle = BGFX_INVALID_HANDLE;
	}


	Texture::~Texture()
	{
		ASSERT(isEmpty());
	}


	e_void Texture::setFlag(e_uint32 flag, e_bool value)
	{
		e_uint32 new_flags = bgfx_flags & ~flag;
		new_flags |= value ? flag : 0;
		bgfx_flags = new_flags;

		m_resource_manager.reload(*this);
	}


	e_void Texture::setFlags(e_uint32 flags)
	{
		if (isReady() && bgfx_flags != flags)
		{
			log_waring("Renderer Trying to set different flags for texture %s. They are ignored.", getPath().c_str());
			return;
		}
		bgfx_flags = flags;
	}


	e_void Texture::destroy()
	{
		doUnload();
	}


	e_bool Texture::create(e_int32 w, e_int32 h, const e_void* data)
	{
		if (data)
		{
			handle = bgfx::createTexture2D(
				(uint16_t)w, (uint16_t)h, false, 1, bgfx::TextureFormat::RGBA8, bgfx_flags, bgfx::copy(data, w * h * 4));
		}
		else
		{
			handle = bgfx::createTexture2D((uint16_t)w, (uint16_t)h, false, 1, bgfx::TextureFormat::RGBA8, bgfx_flags);
		}
		mips = 1;
		width = w;
		height = h;

		e_bool isReady = bgfx::isValid(handle);
		onCreated(isReady ? State::READY : State::FAILURE);

		return isReady;
	}


	e_uint32 Texture::getPixelNearest(e_int32 x, e_int32 y) const
	{
		if (data.empty() || x >= width || y >= height || x < 0 || y < 0 || bytes_per_pixel != 4) return 0;

		return *(e_uint32*)&data[(x + y * width) * 4];
	}


	e_uint32 Texture::getPixel(e_float x, e_float y) const
	{
		ASSERT(bytes_per_pixel == 4);
		if (data.empty() || x >= width || y >= height || x < 0 || y < 0)
		{
			return 0;
		}

		// http://fastcpp.blogspot.sk/2011/06/bilinear-pixel-interpolation-using-sse.html
		e_int32 px = (e_int32)x;
		e_int32 py = (e_int32)y;
		const e_uint32* p0 = (e_uint32*)&data[(px + py * width) * 4];

		const e_uint8* p1 = (e_uint8*)p0;
		const e_uint8* p2 = (e_uint8*)(p0 + 1);
		const e_uint8* p3 = (e_uint8*)(p0 + width);
		const e_uint8* p4 = (e_uint8*)(p0 + 1 + width);

		e_float fx = x - px;
		e_float fy = y - py;
		e_float fx1 = 1.0f - fx;
		e_float fy1 = 1.0f - fy;

		e_int32 w1 = (e_int32)(fx1 * fy1 * 256.0f);
		e_int32 w2 = (e_int32)(fx * fy1 * 256.0f);
		e_int32 w3 = (e_int32)(fx1 * fy * 256.0f);
		e_int32 w4 = (e_int32)(fx * fy * 256.0f);

		e_uint8 res[4];
		res[0] = (e_uint8)((p1[0] * w1 + p2[0] * w2 + p3[0] * w3 + p4[0] * w4) >> 8);
		res[1] = (e_uint8)((p1[1] * w1 + p2[1] * w2 + p3[1] * w3 + p4[1] * w4) >> 8);
		res[2] = (e_uint8)((p1[2] * w1 + p2[2] * w2 + p3[2] * w3 + p4[2] * w4) >> 8);
		res[3] = (e_uint8)((p1[3] * w1 + p2[3] * w2 + p3[3] * w3 + p4[3] * w4) >> 8);

		return *(e_uint32*)res;
	}


	e_uint32 Texture::compareTGA(FS::IFile* file1, FS::IFile* file2, e_int32 difference, IAllocator& allocator)
	{
		TGAHeader header1, header2;
		file1->read(&header1, sizeof(header1));
		file2->read(&header2, sizeof(header2));

		if (header1.bitsPerPixel != header2.bitsPerPixel ||
			header1.width != header2.width || header1.height != header2.height ||
			header1.dataType != header2.dataType ||
			header1.imageDescriptor != header2.imageDescriptor)
		{
			log_error("Renderer Trying to compare textures with different formats");
			return 0xffffFFFF;
		}

		e_int32 color_mode = header1.bitsPerPixel / 8;
		if (header1.dataType != 2)
		{
			log_error("Renderer Unsupported texture format");
			return 0xffffFFFF;
		}

		e_int32 different_pixel_count = 0;
		size_t pixel_count = header1.width * header1.height;
		e_uint8* img1 = (e_uint8*)allocator.allocate(pixel_count * color_mode);
		e_uint8* img2 = (e_uint8*)allocator.allocate(pixel_count * color_mode);

		file1->read(img1, pixel_count * color_mode);
		file2->read(img2, pixel_count * color_mode);

		for (size_t i = 0; i < pixel_count * color_mode; i += color_mode)
		{
			for (e_int32 j = 0; j < color_mode; ++j)
			{
				if (Math::abs(img1[i + j] - img2[i + j]) > difference)
				{
					++different_pixel_count;
					break;
				}
			}
		}

		allocator.deallocate(img1);
		allocator.deallocate(img2);

		return different_pixel_count;
	}


	e_bool Texture::saveTGA(FS::IFile* file,
		e_int32 width,
		e_int32 height,
		e_int32 bytes_per_pixel,
		const e_uint8* image_dest,
		const ArchivePath& path,
		IAllocator& allocator)
	{
		if (bytes_per_pixel != 4)
		{
			log_error("Renderer Texture %s could not be saved, unsupported TGA format", path.c_str());
			return false;
		}

		e_uint8* data = (e_uint8*)allocator.allocate(width * height * 4);

		TGAHeader header;
		StringUnitl::setMemory(&header, 0, sizeof(header));
		header.bitsPerPixel = (e_char)(bytes_per_pixel * 8);
		header.height = (short)height;
		header.width = (short)width;
		header.dataType = 2;

		file->write(&header, sizeof(header));

		for (long y = 0; y < header.height; y++)
		{
			long read_index = y * header.width * 4;
			long write_index = ((header.imageDescriptor & 32) != 0) ? read_index : y * header.width * 4;
			for (long x = 0; x < header.width; x++)
			{
				data[write_index + 0] = image_dest[write_index + 2];
				data[write_index + 1] = image_dest[write_index + 1];
				data[write_index + 2] = image_dest[write_index + 0];
				data[write_index + 3] = image_dest[write_index + 3];
				write_index += 4;
			}
		}

		file->write(data, width * height * 4);

		allocator.deallocate(data);

		return true;
	}


	static e_void saveTGA(Texture& texture)
	{
		if (texture.data.empty())
		{
			log_error("Renderer Texture %s could not be saved, no data was loaded", texture.getPath().c_str());
			return;
		}

		FS::FileSystem& fs = texture.getResourceManager().getOwner().getFileSystem();
		FS::IFile* file = fs.open(fs.getDefaultDevice(), texture.getPath().c_str(), FS::Mode::CREATE_AND_WRITE);

		Texture::saveTGA(file,
			texture.width,
			texture.height,
			texture.bytes_per_pixel,
			&texture.data[0],
			texture.getPath(),
			texture.allocator);

		fs.close(*file);
	}


	e_void Texture::save()
	{
		e_char ext[5];
		ext[0] = 0;
		StringUnitl::getExtension(ext, 5, getPath().c_str());
		if (StringUnitl::equalStrings(ext, "raw") && bytes_per_pixel == 2)
		{
			FS::FileSystem& fs = m_resource_manager.getOwner().getFileSystem();
			FS::IFile* file = fs.open(fs.getDefaultDevice(), getPath().c_str(), FS::Mode::CREATE_AND_WRITE);

			file->write(&data[0], data.size() * sizeof(data[0]));
			fs.close(*file);
		}
		else if (StringUnitl::equalStrings(ext, "tga") && bytes_per_pixel == 4)
		{
			egal::saveTGA(*this);
		}
		else
		{
			log_error("Renderer Texture %s can not be saved - unsupported format.", getPath().c_str());
		}
	}

	e_void Texture::onDataUpdated(e_int32 x, e_int32 y, e_int32 w, e_int32 h)
	{
		//PROFILE_FUNCTION();

		const bgfx::Memory* mem;

		if (bytes_per_pixel == 2)
		{
			const e_uint16* src_mem = (const e_uint16*)&data[0];
			mem = bgfx::alloc(w * h * sizeof(e_float));
			e_float* dst_mem = (e_float*)mem->data;

			for (e_int32 j = 0; j < h; ++j)
			{
				for (e_int32 i = 0; i < w; ++i)
				{
					dst_mem[i + j * w] = src_mem[x + i + (y + j) * width] / 65535.0f;
				}
			}
		}
		else
		{
			ASSERT(bytes_per_pixel == 4);
			const e_uint8* src_mem = (const e_uint8*)&data[0];
			mem = bgfx::alloc(w * h * bytes_per_pixel);
			e_uint8* dst_mem = mem->data;

			for (e_int32 j = 0; j < h; ++j)
			{
				StringUnitl::copyMemory(
					&dst_mem[(j * w) * bytes_per_pixel],
					&src_mem[(x + (y + j) * width) * bytes_per_pixel],
					bytes_per_pixel * w);
			}
		}
		bgfx::updateTexture2D(handle, 0, 0, (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h, mem);
	}


	e_bool loadRaw(Texture& texture, FS::IFile& file)
	{
		//PROFILE_FUNCTION();
		size_t size = file.size();
		texture.bytes_per_pixel = 2;
		texture.width = (e_int32)sqrt(size / texture.bytes_per_pixel);
		texture.height = texture.width;

		if (texture.data_reference)
		{
			texture.data.resize((e_int32)size);
			file.read(&texture.data[0], size);
		}

		const e_uint16* src_mem = (const e_uint16*)file.getBuffer();
		const bgfx::Memory* mem = bgfx::alloc(texture.width * texture.height * sizeof(e_float));
		e_float* dst_mem = (e_float*)mem->data;

		for (e_int32 i = 0; i < texture.width * texture.height; ++i)
		{
			dst_mem[i] = src_mem[i] / 65535.0f;
		}

		texture.handle = bgfx::createTexture2D((uint16_t)texture.width,
			(uint16_t)texture.height,
			false,
			1,
			bgfx::TextureFormat::R32F,
			texture.bgfx_flags,
			nullptr);
		bgfx::setName(texture.handle, texture.getPath().c_str());
		// update must be here because texture is immutable otherwise 
		bgfx::updateTexture2D(texture.handle, 0, 0, 0, 0, (uint16_t)texture.width, (uint16_t)texture.height, mem);
		texture.depth = 1;
		texture.layers = 1;
		texture.mips = 1;
		texture.is_cubemap = false;
		return bgfx::isValid(texture.handle);
	}


	e_bool Texture::loadTGA(FS::IFile& file, TGAHeader& header, TVector<e_uint8>& data, const e_char* path)
	{
		//PROFILE_FUNCTION();
		file.read(&header, sizeof(header));

		e_int32 bytes_per_pixel = header.bitsPerPixel / 8;
		e_int32 image_size = header.width * header.height * 4;
		if (header.dataType != 2 && header.dataType != 10)
		{
			log_error("Renderer Unsupported texture format %s", path);
			return false;
		}

		if (bytes_per_pixel < 3)
		{
			log_error("Renderer Unsupported color mode %s.", path);
			return false;
		}

		e_int32 pixel_count = header.width * header.height;
		data.resize(image_size);
		e_uint8* image_dest = &data[0];

		e_bool is_rle = header.dataType == 10;
		if (is_rle)
		{
			e_uint8* out = image_dest;
			e_uint8 byte;
			struct Pixel {
				e_uint8 uint8[4];
			} pixel;
			do
			{
				file.read(&byte, sizeof(byte));
				if (byte < 128)
				{
					e_uint8 count = byte + 1;
					for (e_uint8 i = 0; i < count; ++i)
					{
						file.read(&pixel, bytes_per_pixel);
						out[0] = pixel.uint8[2];
						out[1] = pixel.uint8[1];
						out[2] = pixel.uint8[0];
						if (bytes_per_pixel == 4) out[3] = pixel.uint8[3];
						else out[3] = 255;
						out += 4;
					}
				}
				else
				{
					byte -= 127;
					file.read(&pixel, bytes_per_pixel);
					for (e_int32 i = 0; i < byte; ++i)
					{
						out[0] = pixel.uint8[2];
						out[1] = pixel.uint8[1];
						out[2] = pixel.uint8[0];
						if (bytes_per_pixel == 4) out[3] = pixel.uint8[3];
						else out[3] = 255;
						out += 4;
					}
				}
			} while (out - image_dest < pixel_count * 4);
		}
		else
		{
			for (long y = 0; y < header.height; y++)
			{
				long idx = y * header.width * 4;
				for (long x = 0; x < header.width; x++)
				{
					file.read(&image_dest[idx + 2], sizeof(e_uint8));
					file.read(&image_dest[idx + 1], sizeof(e_uint8));
					file.read(&image_dest[idx + 0], sizeof(e_uint8));
					if (bytes_per_pixel == 4)
						file.read(&image_dest[idx + 3], sizeof(e_uint8));
					else
						image_dest[idx + 3] = 255;
					idx += 4;
				}
			}
		}
		return true;
	}


	e_bool Texture::loadTGA(FS::IFile& file)
	{
		//PROFILE_FUNCTION();
		TGAHeader header;
		file.read(&header, sizeof(header));

		bytes_per_pixel = header.bitsPerPixel / 8;
		e_int32 image_size = header.width * header.height * 4;
		if (header.dataType != 2 && header.dataType != 10)
		{
			log_error("Renderer Unsupported texture format %s.", getPath().c_str());
			return false;
		}

		if (bytes_per_pixel < 3)
		{
			log_error("Renderer Unsupported color mode %s.", getPath().c_str());
			return false;
		}

		width = header.width;
		height = header.height;
		e_int32 pixel_count = width * height;
		is_cubemap = false;
		TextureManager& manager = static_cast<TextureManager&>(getResourceManager());
		if (data_reference)
		{
			data.resize(image_size);
		}
		e_uint8* image_dest = data_reference ? &data[0] : (e_uint8*)manager.getBuffer(image_size);

		e_bool is_rle = header.dataType == 10;
		if (is_rle)
		{
			e_uint8* out = image_dest;
			e_uint8 byte;
			struct Pixel {
				e_uint8 uint8[4];
			} pixel;
			do
			{
				file.read(&byte, sizeof(byte));
				if (byte < 128)
				{
					e_uint8 count = byte + 1;
					for (e_uint8 i = 0; i < count; ++i)
					{
						file.read(&pixel, bytes_per_pixel);
						out[0] = pixel.uint8[2];
						out[1] = pixel.uint8[1];
						out[2] = pixel.uint8[0];
						if (bytes_per_pixel == 4) out[3] = pixel.uint8[3];
						else out[3] = 255;
						out += 4;
					}
				}
				else
				{
					byte -= 127;
					file.read(&pixel, bytes_per_pixel);
					for (e_int32 i = 0; i < byte; ++i)
					{
						out[0] = pixel.uint8[2];
						out[1] = pixel.uint8[1];
						out[2] = pixel.uint8[0];
						if (bytes_per_pixel == 4) out[3] = pixel.uint8[3];
						else out[3] = 255;
						out += 4;
					}
				}
			} while (out - image_dest < pixel_count * 4);
		}
		else
		{
			for (long y = 0; y < header.height; y++)
			{
				long idx = y * header.width * 4;
				for (long x = 0; x < header.width; x++)
				{
					file.read(&image_dest[idx + 2], sizeof(e_uint8));
					file.read(&image_dest[idx + 1], sizeof(e_uint8));
					file.read(&image_dest[idx + 0], sizeof(e_uint8));
					if (bytes_per_pixel == 4)
						file.read(&image_dest[idx + 3], sizeof(e_uint8));
					else
						image_dest[idx + 3] = 255;
					idx += 4;
				}
			}
		}
		bytes_per_pixel = 4;
		mips = 1;
		handle = bgfx::createTexture2D(
			header.width,
			header.height,
			false,
			0,
			bgfx::TextureFormat::RGBA8,
			bgfx_flags,
			nullptr);
		bgfx::setName(handle, getPath().c_str());
		// update must be here because texture is immutable otherwise 
		bgfx::updateTexture2D(handle,
			0,
			0,
			0,
			0,
			header.width,
			header.height,
			bgfx::copy(image_dest, header.width * header.height * 4));
		depth = 1;
		layers = 1;
		return bgfx::isValid(handle);
	}

	e_void Texture::addDataReference()
	{
		++data_reference;
		if (data_reference == 1 && isReady())
		{
			m_resource_manager.reload(*this);
		}
	}

	e_void Texture::removeDataReference()
	{
		--data_reference;
		if (data_reference == 0)
		{
			data.clear();
		}
	}

	static e_bool loadDDSorKTX(Texture& texture, FS::IFile& file)
	{
		bgfx::TextureInfo info;
		const auto* mem = bgfx::copy(file.getBuffer(), (e_uint32)file.size());
		texture.handle = bgfx::createTexture(mem, texture.bgfx_flags, 0, &info);
		bgfx::setName(texture.handle, texture.getPath().c_str());
		texture.width = info.width;
		texture.mips = info.numMips;
		texture.height = info.height;
		texture.depth = info.depth;
		texture.layers = info.numLayers;
		texture.is_cubemap = info.cubeMap;
		return bgfx::isValid(texture.handle);
	}

	e_bool Texture::load(FS::IFile& file)
	{
		//PROFILE_FUNCTION();

		const e_char* path = getPath().c_str();
		size_t len = getPath().length();
		e_bool loaded = false;
		if (len > 3 && (StringUnitl::equalStrings(path + len - 4, ".dds") || StringUnitl::equalStrings(path + len - 4, ".ktx")))
		{
			loaded = loadDDSorKTX(*this, file);
		}
		else if (len > 3 && StringUnitl::equalStrings(path + len - 4, ".raw"))
		{
			loaded = loadRaw(*this, file);
		}
		else
		{
			loaded = loadTGA(file);
		}
		if (!loaded)
		{
			log_waring("Renderer Error loading texture %s.", path);
			return false;
		}

		m_size = file.size();
		return true;
	}

	e_void Texture::unload()
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
			handle = BGFX_INVALID_HANDLE;
		}
		data.clear();
	}
}

namespace egal
{
	TextureManager::TextureManager(IAllocator& allocator)
		: ResourceManagerBase(allocator)
		, m_allocator(allocator)
	{
		m_buffer = nullptr;
		m_buffer_size = -1;
	}

	TextureManager::~TextureManager()
	{
		m_allocator.deallocate(m_buffer);
	}

	Resource* TextureManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Texture)(path, *this, m_allocator);
	}

	e_void TextureManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Texture*>(&resource));
	}

	e_uint8* TextureManager::getBuffer(e_int32 size)
	{
		if (m_buffer_size < size)
		{
			m_allocator.deallocate(m_buffer);
			m_buffer = nullptr;
			m_buffer_size = -1;
		}
		if (m_buffer == nullptr)
		{
			m_buffer = (e_uint8*)m_allocator.allocate(sizeof(e_uint8) * size);
			m_buffer_size = size;
		}
		return m_buffer;
	}
}