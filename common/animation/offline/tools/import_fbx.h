#ifndef _import_fbx_h_
#define _import_fbx_h_
#include "common/animation/offline/tools/import_option.h"
#include "common/type.h"
#include "common/allocator/egal_allocator.h"
#include "common/egal_string.h"
#include "common/utils/logger.h"
#include "common/filesystem/os_file.h"


namespace egal
{
	class ImportAssetDialog;
	class ImportFbx
	{
	public:
		ImportFbx(ImportAssetDialog& dialog);
		~ImportFbx();

		e_void import_fbx(const char* file_path, ImportOption& option);


		void convert(ImportOption& option);
	protected:
		template <typename T> 
		void write(const T& obj) { out_file.write(&obj, sizeof(obj)); }
		void write(const void* ptr, size_t size) { out_file.write(ptr, size); }
		void writeString(const char* str) { out_file.write(str, strlen(str)); }

	private:
		ImportAssetDialog& importAssetDialog;
		FS::OsFile out_file;
	};
}
#endif
