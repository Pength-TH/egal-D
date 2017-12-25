
#ifndef _texture_manager_h_
#define _texture_manager_h_

#include "common/resource/resource_manager.h"
#include "common/resource/resource_define.h"

namespace egal
{
	namespace FS
	{
		class FileSystem;
	}

#pragma pack(1)
	struct TGAHeader
	{
		e_uint8 idLength;
		e_uint8 colourMapType;
		e_uint8 dataType;
		e_uint16 colourMapOrigin;
		e_uint16 colourMapLength;
		e_uint8 colourMapDepth;
		e_uint16 xOrigin;
		e_uint16 yOrigin;
		e_uint16 width;
		e_uint16 height;
		e_uint8 bitsPerPixel;
		e_uint8 imageDescriptor;
	};
#pragma pack()


	class Texture : public Resource
	{
	public:
		Texture(const ArchivePath& path, ResourceManagerBase& resource_manager, IAllocator& allocator);
		~Texture();

		e_bool create(e_int32 w, e_int32 h, const e_void* data);
		e_void destroy();

		const e_uint8* getData() const { return data.empty() ? nullptr : &data[0]; }
		e_uint8* getData() { return data.empty() ? nullptr : &data[0]; }
		e_void addDataReference();
		e_void removeDataReference();
		e_void onDataUpdated(e_int32 x, e_int32 y, e_int32 w, e_int32 h);
		e_void save();
		e_void setFlags(e_uint32 flags);
		e_void setFlag(e_uint32 flag, e_bool value);
		e_uint32 getPixelNearest(e_int32 x, e_int32 y) const;
		e_uint32 getPixel(e_float x, e_float y) const;

		static e_uint32 compareTGA(FS::IFile* file1, FS::IFile* file2, e_int32 difference, IAllocator& allocator);
		static e_bool saveTGA(FS::IFile* file,
			e_int32 width,
			e_int32 height,
			e_int32 bytes_per_pixel,
			const e_uint8* image_dest,
			const ArchivePath& path,
			IAllocator& allocator);
		static e_bool loadTGA(FS::IFile& file, TGAHeader& header, TVector<e_uint8>& data, const e_char* path);

	public:
		e_int32 width;
		e_int32 height;
		e_int32 bytes_per_pixel;
		e_int32 depth;
		e_int32 layers;
		e_int32 mips;
		e_bool is_cubemap;
		e_uint32 bgfx_flags;
		egal::TextureHandle handle;
		IAllocator& allocator;
		e_int32 data_reference;
		TVector<e_uint8> data;

	private:
		e_void unload() override;
		e_bool load(FS::IFile& file) override;
		e_bool loadTGA(FS::IFile& file);
	};
}

namespace egal
{
	class TextureManager : public ResourceManagerBase
	{
	public:
		explicit TextureManager(IAllocator& allocator);
		~TextureManager();

		e_uint8* getBuffer(e_int32 size);

	protected:
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
		e_uint8* m_buffer;
		e_int32 m_buffer_size;
	};
}
#endif
