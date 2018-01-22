#ifndef _import_asset_dialog_h_
#define _import_asset_dialog_h_
#pragma once

#include "common/egal-d.h"
#include "editor/tools/base/editor_base.h"

#include "common/animation/offline/tools/import_fbx.h"


namespace egal
{
	namespace MT { class Task; }

	class ResourceSerializer;
	class ImportAssetDialog : public IWinodw
	{
		friend struct ImportTask;
		friend struct ConvertTask;
		friend struct ImportTextureTask;
	public:
		struct DDSConvertCallbackData
		{
			ImportAssetDialog* dialog;
			const char* dest_path;
			bool cancel_requested;
		};

	public:
		explicit ImportAssetDialog(IAllocator &allocator);
		~ImportAssetDialog();
		void setMessage(const char* message);
		void setImportMessage(const char* message, float progress_fraction);
		
		void saveModelMetadata();
		void onWindowGUI();

		DDSConvertCallbackData& getDDSConvertCallbackData() { return m_dds_convert_callback; }
		const char* getName() const { return "import_asset"; }
		bool onDropFile(const char* file);

	public:
		bool m_is_open;

	private:
		bool checkSource();
		void checkTask(bool wait);
		void convert(bool use_ui);
		void _import();
		void start_import_file();
		void getMessage(char* msg, int max_size);
		bool hasMessage();
		void importTexture();
		bool isTextureDirValid() const;
		void onMaterialsGUI();
		void onMeshesGUI();
		void onAnimationsGUI();
		
		void onImageGUI();
		
		void onLODsGUI();

		void clearSources();
		void addSource(const char* src);

	public:
		TArrary<e_uint32>						m_saved_textures;
		TArrary<StaticString<MAX_PATH_LENGTH> > m_sources;
		
		char m_import_message[1024];

		struct ImageData
		{
			e_byte* data;
			int width;
			int height;
			int comps;
			int resize_size[2];
		} m_image;

		float m_progress_fraction;
		char m_message[1024];
		char m_last_dir[MAX_PATH_LENGTH];
		char m_source[MAX_PATH_LENGTH];
		char m_mesh_output_filename[MAX_PATH_LENGTH];
		char m_output_dir[MAX_PATH_LENGTH];
		char m_texture_output_dir[MAX_PATH_LENGTH];
		bool m_convert_to_dds;
		bool m_convert_to_raw;
		bool m_is_normal_map;
		bool m_is_importing_texture;
		float m_raw_texture_scale;

		bool m_is_fbx_file;
		bool m_is_importing_fbx;

		MT::Task*		m_task;
		MT::SpinMutex	m_mutex;

		IAllocator&		m_allocator;
		DDSConvertCallbackData m_dds_convert_callback;

		ImportOption	m_fbx_importer_option;
		ImportFbx       m_importFbx;

		ResourceSerializer* resourceSer;
	};
}
#endif
