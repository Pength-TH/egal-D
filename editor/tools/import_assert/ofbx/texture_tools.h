#include "editor/tools/import_assert/ofbx/fbx2resource.h"
#include "common/egal-d.h"
#include "common/filesystem/binary.h"

#include "common/thread/task.h"

#include "editor/tools/base/platform_interface.h"
#include "ofbx.h"

#include <3rdparty/crnlib/include/crnlib.h>

namespace egal
{
	class ImportAssetDialog;
	typedef StaticString<MAX_PATH_LENGTH> PathBuilder;

	static bool isImage(const char* path)
	{
		char ext[10];
		StringUnitl::getExtension(ext, sizeof(ext), path);

		static const char* image_extensions[] = {
			"dds", "jpg", "jpeg", "png", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm" };
		StringUnitl::makeLowercase(ext, TlengthOf(ext), ext);
		for (auto image_ext : image_extensions)
		{
			if (StringUnitl::equalStrings(ext, image_ext))
			{
				return true;
			}
		}
		return false;
	}
	

	static void resizeImage(ImportAssetDialog* dlg, int new_w, int new_h)
	{
		ImportAssetDialog::ImageData& img = dlg->m_image;
		e_uint8* mem = (e_uint8*)stbi__malloc(new_w * new_h * 4);
		stbir_resize_uint8(img.data,
			img.width,
			img.height,
			0,
			mem,
			new_w,
			new_h,
			0,
			4);

		stbi_image_free(img.data);
		img.data = mem;
		img.width = new_w;
		img.height = new_h;
	}

	static crn_comp_params s_default_comp_params;
	static bool saveAsDDS(ImportAssetDialog& dialog,
		const char* source_path,
		const e_uint8* image_data,
		int image_width,
		int image_height,
		bool alpha,
		bool normal,
		const char* dest_path)
	{
		ASSERT(image_data);

		dialog.setImportMessage(StaticString<MAX_PATH_LENGTH + 30>("Saving ") << dest_path, 0);

		dialog.getDDSConvertCallbackData().dialog = &dialog;
		dialog.getDDSConvertCallbackData().dest_path = dest_path;
		dialog.getDDSConvertCallbackData().cancel_requested = false;

		crn_uint32 size;
		crn_comp_params comp_params = s_default_comp_params;
		comp_params.m_width = image_width;
		comp_params.m_height = image_height;
		comp_params.m_format = normal ? cCRNFmtDXN_YX : (alpha ? cCRNFmtDXT5 : cCRNFmtDXT1);
		comp_params.m_pImages[0][0] = (e_uint32*)image_data;
		crn_mipmap_params mipmap_params;
		mipmap_params.m_mode = cCRNMipModeGenerateMips;

		void* data = crn_compress(comp_params, mipmap_params, size);
		if (!data)
		{
			dialog.setMessage(StaticString<MAX_PATH_LENGTH + 30>("Could not convert ") << source_path);
			return false;
		}

		FS::OsFile file;
		if (!file.open(dest_path, FS::Mode::CREATE_AND_WRITE))
		{
			dialog.setMessage(StaticString<MAX_PATH_LENGTH + 30>("Could not save ") << dest_path);
			crn_free_block(data);
			return false;
		}

		file.write((const char*)data, size);
		file.close();
		crn_free_block(data);
		return true;
	}

	static bool saveAsRaw(ImportAssetDialog& dialog,
		FS::FileSystem& fs,
		const e_uint8* image_data,
		int image_width,
		int image_height,
		const char* dest_path,
		float scale,
		IAllocator& allocator)
	{
		ASSERT(image_data);

		dialog.setImportMessage(StaticString<MAX_PATH_LENGTH + 30>("Saving ") << dest_path, -1);

		FS::OsFile file;
		if (!file.open(dest_path, FS::Mode::CREATE_AND_WRITE))
		{
			dialog.setMessage(StaticString<MAX_PATH_LENGTH + 30>("Could not save ") << dest_path);
			return false;
		}

		TArrary<e_uint16> data(allocator);
		data.resize(image_width * image_height);
		for (int j = 0; j < image_height; ++j)
		{
			for (int i = 0; i < image_width; ++i)
			{
				data[i + j * image_width] = e_uint16(scale * image_data[(i + j * image_width) * 4]);
			}
		}

		file.write((const char*)&data[0], data.size() * sizeof(data[0]));
		file.close();
		return true;
	}


}