#include "import_asset_dialog.h"

#include "common/thread/task.h"
#include "common/filesystem/os_file.h"

#include "tools/base/platform_interface.h"

#include "editor/tools/import_assert/ofbx/fbx2resource.h"

#include "imgui/bgfx_imgui.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#if defined _MSC_VER && _MSC_VER == 1900 
#pragma warning(disable : 4312)
#endif

#include "tools/base/stb/stb_image.h"
#include "tools/base/stb/stb_image_resize.h"
#include <cstddef>

#include "editor/tools/import_assert/ofbx/texture_tools.h"
namespace egal
{
	typedef StaticString<MAX_PATH_LENGTH> PathBuilder;
	struct ImportTextureTask : public MT::Task
	{
		explicit ImportTextureTask(ImportAssetDialog& dialog)
			: Task(dialog.m_allocator)
			, m_dialog(dialog)
		{
		}

		static void getDestinationPath(const char* output_dir,
			const char* source,
			bool to_dds,
			bool to_raw,
			char* out,
			int max_size)
		{
			char basename[MAX_PATH_LENGTH];
			StringUnitl::getBasename(basename, sizeof(basename), source);

			if (to_dds)
			{
				PathBuilder dest_path(output_dir);
				dest_path << "/" << basename << ".dds";
				StringUnitl::copyString(out, max_size, dest_path);
				return;
			}

			if (to_raw)
			{
				PathBuilder dest_path(output_dir);
				dest_path << "/" << basename << ".raw";
				StringUnitl::copyString(out, max_size, dest_path);
				return;
			}

			char ext[MAX_PATH_LENGTH];
			StringUnitl::getExtension(ext, sizeof(ext), source);
			PathBuilder dest_path(output_dir);
			dest_path << "/" << basename << "." << ext;
			StringUnitl::copyString(out, max_size, dest_path);
		}


		int task() override
		{
			m_dialog.setImportMessage("Importing texture...", 0);

			if (!m_dialog.m_image.data)
			{
				m_dialog.setMessage(StaticString<MAX_PATH_LENGTH + 200>("Could not load ") << m_dialog.m_source << " : "
					<< stbi_failure_reason());
				return -1;
			}

			char dest_path[MAX_PATH_LENGTH];
			getDestinationPath(m_dialog.m_output_dir,
				m_dialog.m_source,
				m_dialog.m_convert_to_dds,
				m_dialog.m_convert_to_raw,
				dest_path,
				TlengthOf(dest_path));

			if (m_dialog.m_convert_to_dds)
			{
				m_dialog.setImportMessage("Converting to DDS...", 0);

				//saveAsDDS(m_dialog,
				//	m_dialog.m_source,
				//	m_dialog.m_image.data,
				//	m_dialog.m_image.width,
				//	m_dialog.m_image.height,
				//	m_dialog.m_image.comps == 4,
				//	m_dialog.m_is_normal_map,
				//	dest_path);
			}
			else if (m_dialog.m_convert_to_raw)
			{
				m_dialog.setImportMessage("Converting to RAW...", -1);

				saveAsRaw(m_dialog,
					*g_file_system,
					m_dialog.m_image.data,
					m_dialog.m_image.width,
					m_dialog.m_image.height,
					dest_path,
					m_dialog.m_raw_texture_scale,
					m_dialog.m_allocator);
			}
			else
			{
				m_dialog.setImportMessage("Copying...", -1);

				if (!copyFile(m_dialog.m_source, dest_path))
				{
					m_dialog.setMessage(StaticString<MAX_PATH_LENGTH * 2 + 30>("Could not copy ")
						<< m_dialog.m_source
						<< " to "
						<< dest_path);
				}
			}
			stbi_image_free(m_dialog.m_image.data);
			m_dialog.m_image.data = nullptr;

			return 0;
		}

		ImportAssetDialog& m_dialog;

	}; // struct ImportTextureTask

	template <typename T, typename T2>
	struct GenericTask : public MT::Task
	{
		GenericTask(T _function, T2 _on_destroy, IAllocator& allocator)
			: Task(allocator)
			, function(_function)
			, on_destroy(_on_destroy)
		{
		}

		~GenericTask()
		{
			on_destroy();
		}


		int task() override
		{
			function();
			return 0;
		}

		T2 on_destroy;
		T function;
	};

	template <typename T, typename T2> 
	GenericTask<T, T2>* makeTask(T function, T2 on_destroy, IAllocator& allocator)
	{
		return _aligned_new(allocator, GenericTask<T, T2>)(function, on_destroy, allocator);
	}

/**************************************************************************************************************/
/**************************************************************************************************************/
/**************************************************************************************************************/
	ImportAssetDialog::ImportAssetDialog(IAllocator &allocator)
		: m_task(nullptr)
		, m_allocator(allocator)
		, m_mutex(false)
		, m_is_open(true)
		, m_saved_textures(allocator)
		, m_sources(allocator)
		, m_importFbx(*this)
		, m_is_fbx_file(false)
		, m_is_importing_fbx(false)
	{
		m_image.data = nullptr;

		m_is_open = false;
		m_message[0] = '\0';
		m_import_message[0] = '\0';
		m_task = nullptr;
		m_source[0] = '\0';
		m_output_dir[0] = '\0';
		m_mesh_output_filename[0] = '\0';
		m_texture_output_dir[0] = '\0';

		StringUnitl::copyString(m_last_dir, TlengthOf(m_last_dir), getDiskBasePath());

		m_is_importing_texture = false;

		resourceSer = new ResourceSerializer(*this);
	}

	ImportAssetDialog::~ImportAssetDialog()
	{
		SAFE_DELETE(resourceSer);
	}

	void ImportAssetDialog::setMessage(const char* message)
	{
		MT::SpinLock lock(m_mutex);
		StringUnitl::copyString(m_message, message);
	}

	void ImportAssetDialog::setImportMessage(const char* message, float progress_fraction)
	{
		MT::SpinLock lock(m_mutex);
		StringUnitl::copyString(m_import_message, message);
		m_progress_fraction = progress_fraction;
	}

	void ImportAssetDialog::saveModelMetadata()
	{
		if (m_sources.empty()) return;

		PathBuilder model_path(m_output_dir, "/", m_mesh_output_filename, ".msh");
		char tmp[MAX_PATH_LENGTH];
		StringUnitl::normalize(model_path, tmp, TlengthOf(tmp));
		e_uint32 model_path_hash = crc32(tmp);

		resourceSer->writeModel(m_output_dir, m_mesh_output_filename);
		resourceSer->writeMaterials(m_output_dir, m_output_dir, m_mesh_output_filename);

		/*WriteBinary blob(*g_allocator);
		blob.reserve(1024);
		blob.write(resourceSer->meshes.size());

		blob.write(resourceSer->materials.size());
		for (auto& i : resourceSer->materials)
		{
			blob.write(i.import);
			blob.write(i.alpha_cutout);
			blob.write(i.shader);
			blob.write(TlengthOf(i.textures));
			for (int j = 0; j < TlengthOf(i.textures); ++j)
			{
				auto& texture = i.textures[j];
				blob.write(texture.import);
				blob.write(texture.path);
				blob.write(texture.src);
				blob.write(texture.to_dds);
			}
		}
		int sources_count = m_sources.size();
		blob.write(sources_count);
		blob.write(&m_sources[0], sizeof(m_sources) * m_sources.size());

		FS::OsFile out_file;
		if (!out_file.open(tmp, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("FBX Failed to create %s.", tmp);
			return;
		}
		out_file.write(blob.getData(), blob.getPos());
		out_file.close();*/
	}

	void ImportAssetDialog::onMaterialsGUI()
	{
		StaticString<30> label("Materials (");
		label << m_fbx_importer_option.m_materials.size() << ")###Materials";
		if (!ImGui::CollapsingHeader(label)) return;

		ImGui::Indent();
		if (ImGui::Button("Import all materials"))
		{
			for (auto& mat : m_fbx_importer_option.m_materials) mat.m_import = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Do not import any materials"))
		{
			for (auto& mat : m_fbx_importer_option.m_materials) mat.m_import = false;
		}
		if (ImGui::Button("Import all textures"))
		{
			for (auto& mat : m_fbx_importer_option.m_materials)
			{
				for (auto& tex : mat.m_textures) tex.m_import = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Do not import any textures"))
		{
			for (auto& mat : m_fbx_importer_option.m_materials)
			{
				for (auto& tex : mat.m_textures) tex.m_import = false;
			}
		}
		for (auto& mat : m_fbx_importer_option.m_materials)
		{
			const char* material_name = mat.name;
			if (ImGui::TreeNode(mat.name, "%s", material_name))
			{
				ImGui::Checkbox("Import material", &mat.m_import);
				ImGui::Checkbox("Alpha cutout material", &mat.alpha_cutout);

				ImGui::Columns(4);
				ImGui::Text("Path");
				ImGui::NextColumn();
				ImGui::Text("Import");
				ImGui::NextColumn();
				ImGui::Text("Convert to DDS");
				ImGui::NextColumn();
				ImGui::Text("Source");
				ImGui::NextColumn();
				ImGui::Separator();
				for (int i = 0; i < mat.m_textures.size(); ++i)
				{
					ImGui::Text("%s", mat.m_textures[i].path);
					ImGui::NextColumn();
					ImGui::Checkbox(StaticString<20>("###imp", i), &mat.m_textures[i].m_import);
					ImGui::NextColumn();
					ImGui::Checkbox(StaticString<20>("###dds", i), &mat.m_textures[i].m_todds);
					ImGui::NextColumn();
					if (ImGui::Button(StaticString<50>("Browse###brw", i)))
					{
						if (PlatformInterface::getOpenFilename(
							mat.m_textures[i].path, TlengthOf(mat.m_textures[i].path), "All\0*.*\0", nullptr))
						{
							mat.m_textures[i].is_valid = true;
						}
					}
					ImGui::SameLine();
					ImGui::Text("%s", mat.m_textures[i].path);
					ImGui::NextColumn();
				}
				ImGui::Columns();

				ImGui::TreePop();
			}
		}
		ImGui::Unindent();
	}

	void ImportAssetDialog::onLODsGUI()
	{
		if(!ImGui::CollapsingHeader("LODs")) 
			return;

		for (int i = 0; i < TlengthOf(m_fbx_importer_option.lods_distances); ++i)
		{
			bool b = m_fbx_importer_option.lods_distances[i] < 0;
			if (ImGui::Checkbox(StaticString<20>("Infinite###lod_inf", i), &b))
			{
				m_fbx_importer_option.lods_distances[i] *= -1;
			}
			if (m_fbx_importer_option.lods_distances[i] >= 0)
			{
				ImGui::SameLine();
				ImGui::DragFloat(StaticString<10>("LOD ", i), &m_fbx_importer_option.lods_distances[i], 1.0f, 1.0f, FLT_MAX);
			}
		}
	}

	void ImportAssetDialog::onAnimationsGUI()
	{
		/*StaticString<30> label("Animations (");
		label << m_fbx_importer->animations.size() << ")###Animations";
		if (!ImGui::CollapsingHeader(label)) return;

		ImGui::DragFloat("Time scale", &m_fbx_importer->time_scale, 1.0f, 0, FLT_MAX, "%.5f");
		ImGui::DragFloat("Max position error", &m_fbx_importer->position_error, 0, FLT_MAX);
		ImGui::DragFloat("Max rotation error", &m_fbx_importer->rotation_error, 0, FLT_MAX);

		ImGui::Indent();
		ImGui::Columns(4);

		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Import");
		ImGui::NextColumn();
		ImGui::Text("Root motion bone");
		ImGui::NextColumn();
		ImGui::Text("Splits");
		ImGui::NextColumn();
		ImGui::Separator();

		ImGui::PushID("anims");
		for (int i = 0; i < m_fbx_importer->animations.size(); ++i)
		{
			FBXImporter::ImportAnimation& animation = m_fbx_importer->animations[i];
			ImGui::PushID(i);
			ImGui::InputText("###name", animation.output_filename.data, lengthOf(animation.output_filename.data));
			ImGui::NextColumn();
			ImGui::Checkbox("", &animation.import);
			ImGui::NextColumn();
			auto getter = [](void* data, int idx, const char** out) -> bool {
				auto* that = (ImportAssetDialog*)data;
				*out = that->m_fbx_importer->bones[idx]->name;
				return true;
			};
			ImGui::Combo("##rb", &animation.root_motion_bone_idx, getter, this, m_fbx_importer->bones.size());
			ImGui::NextColumn();
			if (ImGui::Button("Add split")) animation.splits.emplace();
			for (int i = 0; i < animation.splits.size(); ++i)
			{
				auto& split = animation.splits[i];
				if (ImGui::TreeNodeEx(StaticString<64>("", i)))
				{
					ImGui::InputText("Name", split.name.data, sizeof(split.name.data));
					ImGui::InputInt("From", &split.from_frame);
					ImGui::InputInt("To", &split.to_frame);
					if (ImGui::Button("Remove"))
					{
						animation.splits.erase(i);
						--i;
					}
					ImGui::TreePop();
				}
			}
			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::PopID();
		ImGui::Columns();
		ImGui::Unindent();*/
	}


	void ImportAssetDialog::onMeshesGUI()
	{
		StaticString<30> label("Meshes (");
		label << m_fbx_importer_option.m_meshs.size() << ")###Meshes";
		if (!ImGui::CollapsingHeader(label)) return;

		ImGui::InputText("Output mesh filename", m_mesh_output_filename, sizeof(m_mesh_output_filename));

		ImGui::Indent();
		ImGui::Columns(5);

		ImGui::Text("Mesh");
		ImGui::NextColumn();
		ImGui::Text("Material");
		ImGui::NextColumn();
		ImGui::Text("Import mesh");
		ImGui::NextColumn();
		ImGui::Text("Import physics");
		ImGui::NextColumn();
		ImGui::Text("LOD");
		ImGui::NextColumn();
		ImGui::Separator();

		for (auto& mesh : m_fbx_importer_option.m_meshs)
		{
			ImGui::Text("%s", mesh.m_name);
			ImGui::NextColumn();

			auto material = mesh.m_material;
			ImGui::Text("%s", material.name);
			ImGui::NextColumn();

			ImGui::Checkbox(StaticString<30>("###mesh", (e_uint64)&mesh), &mesh.m_import);
			if (ImGui::GetIO().MouseClicked[1] && ImGui::IsItemHovered()) ImGui::OpenPopup("ContextMesh");
			ImGui::NextColumn();
			ImGui::Checkbox(StaticString<30>("###phy", (e_uint64)&mesh), &mesh.m_import_physics);
			if (ImGui::GetIO().MouseClicked[1] && ImGui::IsItemHovered()) ImGui::OpenPopup("ContextPhy");
			ImGui::NextColumn();
			ImGui::Combo(StaticString<30>("###lod", (e_uint64)&mesh), &mesh.m_general_lod, "LOD 1\0LOD 2\0LOD 3\0LOD 4\0");
			ImGui::NextColumn();
		}
		ImGui::Columns();
		ImGui::Unindent();
		if (ImGui::BeginPopup("ContextMesh"))
		{
			if (ImGui::Selectable("Select all"))
			{
				for (auto& mesh : m_fbx_importer_option.m_meshs) mesh.m_import = true;
			}
			if (ImGui::Selectable("Deselect all"))
			{
				for (auto& mesh : m_fbx_importer_option.m_meshs) mesh.m_import = false;
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopup("ContextPhy"))
		{
			if (ImGui::Selectable("Select all"))
			{
				for (auto& mesh : m_fbx_importer_option.m_meshs) mesh.m_import_physics = true;
			}
			if (ImGui::Selectable("Deselect all"))
			{
				for (auto& mesh : m_fbx_importer_option.m_meshs) mesh.m_import_physics = false;
			}
			ImGui::EndPopup();
		}
	}


	void ImportAssetDialog::onWindowGUI()
	{
		//if (!ImGui::BeginDock("Import Asset", &m_is_open))
		//{
		//	ImGui::EndDock();
		//	return;
		//}

		if (m_task)
		{
			checkTask(false);
			{
				MT::SpinLock lock(m_mutex);
				ImGui::Text("%s", m_import_message);
				if (m_progress_fraction >= 0) ImGui::ProgressBar(m_progress_fraction);
			}
			//ImGui::EndDock();
			return;
		}

		if (hasMessage())
		{
			char msg[1024];
			getMessage(msg, sizeof(msg));
			ImGui::Text("%s", msg);
			if (ImGui::Button("OK"))
			{
				setMessage("");
			}
			//ImGui::EndDock();
			return;
		}

		if (m_is_importing_texture)
		{
			if (ImGui::Button("Cancel"))
			{
				m_dds_convert_callback.cancel_requested = true;
			}

			checkTask(false);

			{
				MT::SpinLock lock(m_mutex);
				ImGui::Text("%s", m_import_message);
				if (m_progress_fraction >= 0) ImGui::ProgressBar(m_progress_fraction);
			}
			//ImGui::EndDock();
			return;
		}

		if (ImGui::Button("Add source"))
		{
			if (PlatformInterface::getOpenFilename(m_source, sizeof(m_source), "All\0*.*\0", m_source))
			{
				checkSource();
			}
		}

		onImageGUI();

		if (m_is_fbx_file && !m_is_importing_fbx)
		{
			ImGui::Separator();
			ImGui::Combo("Endianness", &(int&)m_fbx_importer_option.m_endian, "native\0little\0big\0");

			ImGui::Checkbox("Raw Skeleton", &m_fbx_importer_option.m_raw); // false
			ImGui::Checkbox("Split Mesh", &m_fbx_importer_option.m_split); // ttue
			ImGui::DragInt("Influences", &m_fbx_importer_option.m_influences, 1, 0, 10);
			
			ImGui::Checkbox("Additive", &m_fbx_importer_option.m_additive); //false
			ImGui::DragFloat("Hierarchical", &m_fbx_importer_option.m_hierarchical, 0.01, 0.001, 1);//0.001
			ImGui::Checkbox("Optimize", &m_fbx_importer_option.m_optimize); //true
			ImGui::DragFloat("Rotation", &m_fbx_importer_option.m_rotation, 0.01, 0.001, 1);; //0.00174533
			ImGui::DragFloat("Sampling_rate", &m_fbx_importer_option.m_sampling_rate, 0.01, 0.001, 1);; //0
			ImGui::DragFloat("Scale", &m_fbx_importer_option.m_scale, 0.01, 0.001, 1);;// 0.001
			ImGui::DragFloat("Translation", &m_fbx_importer_option.m_translation, 0.01, 0.001, 1);;// 0.001
			
			if (ImGui::Button("import"))
			{
				start_import_file();
				m_is_importing_fbx = true;
			}
		}

		if (m_is_importing_fbx)
		{
			ImGui::SameLine();
			if (ImGui::Button("Clear all sources"))
				clearSources();

			if (ImGui::CollapsingHeader("Advanced"))
			{
				ImGui::Checkbox("Create billboard LOD", &m_fbx_importer_option.create_billboard_lod);
				ImGui::Checkbox("Center meshes", &m_fbx_importer_option.center_mesh);
				ImGui::Checkbox("Import Vertex Colors", &m_fbx_importer_option.import_vertex_colors);
				ImGui::DragFloat("Scale", &m_fbx_importer_option.mesh_scale, 0.01f, 0.001f, 0);
				ImGui::Combo("Orientation", &(int&)m_fbx_importer_option.orientation, "Y up\0Z up\0-Z up\0-X up\0X up\0");
				ImGui::Combo("Root Orientation", &(int&)m_fbx_importer_option.root_orientation, "Y up\0Z up\0-Z up\0-X up\0X up\0");
				ImGui::Checkbox("Make physics convex", &m_fbx_importer_option.make_convex);
			}

			onMeshesGUI();
			onLODsGUI();
			onMaterialsGUI();
			onAnimationsGUI();

			if (ImGui::CollapsingHeader("Output", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::InputText("Output directory", m_output_dir, sizeof(m_output_dir));
				ImGui::SameLine();
				if (ImGui::Button("...###browseoutput"))
				{
					if (PlatformInterface::getOpenDirectory(m_output_dir, sizeof(m_output_dir), m_last_dir))
					{
						StringUnitl::copyString(m_last_dir, m_output_dir);
						StringUnitl::copyString(m_fbx_importer_option.out_dir, m_output_dir);
					}
				}

				ImGui::InputText("Texture output directory", m_texture_output_dir, sizeof(m_texture_output_dir));
				ImGui::SameLine();
				if (ImGui::Button("...###browsetextureoutput"))
				{
					if (PlatformInterface::getOpenDirectory(m_texture_output_dir, sizeof(m_texture_output_dir), m_last_dir))
					{
						StringUnitl::copyString(m_last_dir, m_texture_output_dir);
					}
				}

				if (m_output_dir[0] != '\0')
				{
					if (!isTextureDirValid())
					{
						ImGui::Text("Texture output directory must be an ancestor of the working "
							"directory or empty.");
					}
					else if (ImGui::Button("Convert"))
					{
						convert(true);
					}
				}
			}


			//if (ImGui::BeginPopupModal("Invalid texture"))
			//{
			//	for (auto& mat : materials)
			//	{
			//		for (auto& tex : mat.textures)
			//		{
			//			if (!tex.fbx || tex.is_valid || !tex.import) continue;
			//			ImGui::Text("Texture %s is not valid", tex.path.data);
			//		}
			//	}
			//	if (ImGui::Button("OK"))
			//	{
			//		ImGui::CloseCurrentPopup();
			//	}
			//	ImGui::EndPopup();
			//}
		}

		//ImGui::EndDock();
	}

	bool ImportAssetDialog::onDropFile(const char* file)
	{
		if (StringUnitl::hasExtension(file, "fbx") || StringUnitl::hasExtension(file, "tga") || StringUnitl::hasExtension(file, "dds"))
		{
			m_is_open = true;
			addSource(file);
			return true;
		}
		return false;
	}

	bool ImportAssetDialog::checkSource()
	{
		if (!PlatformInterface::fileExists(m_source))
			return false;

		m_fbx_importer_option.clear();

		if (m_output_dir[0] == '\0')
			StringUnitl::getDir(m_output_dir, sizeof(m_output_dir), m_source);
		if (m_mesh_output_filename[0] == '\0')
			StringUnitl::getBasename(m_mesh_output_filename, sizeof(m_mesh_output_filename), m_source);

		if (isImage(m_source))
		{
			stbi_image_free(m_image.data);
			m_image.data = stbi_load(m_source, &m_image.width, &m_image.height, &m_image.comps, 4);
			m_image.resize_size[0] = m_image.width;
			m_image.resize_size[1] = m_image.height;
			return m_image.data != nullptr;
		}
		stbi_image_free(m_image.data);
		m_image.data = nullptr;

		/**fbx*/
		char ext[10];
		StringUnitl::getExtension(ext, sizeof(ext), m_source);
		StringUnitl::makeLowercase(ext, TlengthOf(ext), ext);
		if (StringUnitl::equalStrings(ext, "fbx"))
		{
			e_char mesh_path[MAX_PATH_LENGTH];
			StringUnitl::copyString(mesh_path, m_output_dir);
			StringUnitl::catString(mesh_path, m_mesh_output_filename);
			StringUnitl::catString(mesh_path, ".mesh");

			e_char skeleton_path[MAX_PATH_LENGTH];
			StringUnitl::copyString(skeleton_path, m_output_dir);
			StringUnitl::catString(skeleton_path, m_mesh_output_filename);
			StringUnitl::catString(skeleton_path, ".skeleton");

			StringUnitl::copyString(m_fbx_importer_option.file_path_name, TlengthOf(m_fbx_importer_option.file_path_name), m_source);
			StringUnitl::copyString(m_fbx_importer_option.out_mesh_path_name, TlengthOf(m_fbx_importer_option.out_mesh_path_name), mesh_path);
			StringUnitl::copyString(m_fbx_importer_option.out_skeleton_path_name, TlengthOf(m_fbx_importer_option.out_skeleton_path_name), skeleton_path);
			StringUnitl::copyString(m_fbx_importer_option.out_dir, m_output_dir);

			m_is_fbx_file = true;
		}
		return true;
	}

	void ImportAssetDialog::checkTask(bool wait)
	{
		if (!m_task) return;
		if (!wait && !m_task->isFinished()) return;

		if (wait)
		{
			while (!m_task->isFinished()) MT::sleep(200);
		}

		m_task->destroy();
		_delete(m_allocator, m_task);
		m_task = nullptr;
		m_is_importing_texture = false;
	}

	void ImportAssetDialog::convert(bool use_ui)
	{
		ASSERT(!m_task);

		if (m_sources.empty())
		{
			setMessage("Nothing to import.");
			return;
		}

		if (resourceSer->bones.size() > Entity::Bone::MAX_COUNT)
		{
			setMessage("Too many bones.");
			return;
		}

		for (auto& material : resourceSer->materials)
		{
			for (auto& tex : material.textures)
			{
				if (tex.fbx && !tex.is_valid && tex.import)
				{
					if (use_ui) 
						ImGui::OpenPopup("Invalid texture");
					else 
						log_error("Tools Invalid texture %s.", tex.src);
					return;
				}
			}
		}

		//saveModelMetadata();

		IAllocator& allocator = *g_allocator;
		m_task = makeTask(
			[this]() {
			if (resourceSer->save(m_output_dir, m_mesh_output_filename, m_texture_output_dir))
			{
				for (auto& mat : resourceSer->materials)
				{
					for (int i = 0; i < TlengthOf(mat.textures); ++i)
					{
						auto& tex = mat.textures[i];

						if (!tex.fbx) continue;
						if (!tex.import) continue;

						FileInfo texture_info(tex.src);
						PathBuilder dest(m_texture_output_dir[0] ? m_texture_output_dir : m_output_dir);
						dest << "/" << texture_info.m_basename << (tex.to_dds ? ".dds" : texture_info.m_extension);

						bool is_src_dds = StringUnitl::equalStrings(texture_info.m_extension, "dds");
						if (tex.to_dds && !is_src_dds && false)
						{
							int image_width, image_height, image_comp;
							auto data = stbi_load(tex.src, &image_width, &image_height, &image_comp, 4);
							if (!data)
							{
								StaticString<MAX_PATH_LENGTH + 20> error_msg("Could not load image ", tex.src);
								setMessage(error_msg);
								return;
							}

							bool is_normal_map = i == 1;
							if (!saveAsDDS(*this,
								tex.src,
								data,
								image_width,
								image_height,
								image_comp == 4,
								is_normal_map,
								dest))
							{
								stbi_image_free(data);
								setMessage(
									StaticString<MAX_PATH_LENGTH * 2 + 20>("Error converting ", tex.src, " to ", dest));
								return;
							}
							stbi_image_free(data);
						}
						else
						{
							if (StringUnitl::equalStrings(tex.src, dest))
							{
								if (!PlatformInterface::fileExists(tex.src))
								{
									setMessage(StaticString<MAX_PATH_LENGTH + 20>(tex.src, " not found"));
									return;
								}
							}
							else if (!copyFile(tex.src, dest))
							{
								setMessage(
									StaticString<MAX_PATH_LENGTH * 2 + 20>("Error copying ", tex.src, " to ", dest));
								return;
							}
						}
					}
				}

				setMessage("Success.");
			}
		},
			[this]() {
			if (resourceSer->create_billboard_lod)
			{
				PathBuilder mesh_path(m_output_dir, "/");
				mesh_path << m_mesh_output_filename << ".msh";

				if (m_texture_output_dir[0])
				{
					PathBuilder texture_path(m_texture_output_dir, m_mesh_output_filename, "_billboard.dds");
					PathBuilder normal_texture_path(
						m_texture_output_dir, m_mesh_output_filename, "_billboard_normal.dds");
					//createBillboard(*this,
					//	Path(mesh_path),
					//	Path(texture_path),
					//	Path(normal_texture_path),
					//	BillboardSceneData::TEXTURE_SIZE);
				}
				else
				{
					PathBuilder texture_path(m_output_dir, "/", m_mesh_output_filename, "_billboard.dds");
					PathBuilder normal_texture_path(m_output_dir, "/", m_mesh_output_filename, "_billboard_normal.dds");
					//createBillboard(*this,
					//	Path(mesh_path),
					//	Path(texture_path),
					//	Path(normal_texture_path),
					//	BillboardSceneData::TEXTURE_SIZE);
				}
			}
		},
			allocator);
		m_task->create("ConvertTask");
	}

	void ImportAssetDialog::_import()
	{
		convert(false);
	}

	void ImportAssetDialog::start_import_file()
	{
		ASSERT(!m_task);
		m_sources.emplace(m_source);
		setImportMessage("Importing...", -1);

		m_task = makeTask(
			[this]()
		{
			resourceSer->addSource(m_source);
			m_importFbx.import_fbx(m_source, m_fbx_importer_option);
		}
		, []() {}, m_allocator);

		m_task->create("Import mesh");
	}

	void ImportAssetDialog::getMessage(char* msg, int max_size)
	{
		MT::SpinLock lock(m_mutex);
		StringUnitl::copyString(msg, max_size, m_message);
	}

	bool ImportAssetDialog::hasMessage()
	{
		MT::SpinLock lock(m_mutex);
		return m_message[0] != '\0';
	}

	void ImportAssetDialog::importTexture()
	{
		ASSERT(!m_task);
		setImportMessage("Importing texture...", 0);

		char dest_path[MAX_PATH_LENGTH];
		ImportTextureTask::getDestinationPath(
			m_output_dir, m_source, m_convert_to_dds, m_convert_to_raw, dest_path, TlengthOf(dest_path));

		char tmp[MAX_PATH_LENGTH];
		StringUnitl::normalize(dest_path, tmp, TlengthOf(tmp));
		ResourceSerializer::getRelativePath(dest_path, TlengthOf(dest_path), tmp);
		e_uint32 hash = crc32(dest_path);

		m_is_importing_texture = true;
		m_task = _aligned_new(m_allocator, ImportTextureTask)(*this);
		m_task->create("ImportTextureTask");
	}

	bool ImportAssetDialog::isTextureDirValid() const
	{
		if (!m_texture_output_dir[0]) return true;
		char normalized_path[MAX_PATH_LENGTH];
		StringUnitl::normalize(m_texture_output_dir, normalized_path, TlengthOf(normalized_path));

		const char* base_path = getDiskBasePath();
		return StringUnitl::compareStringN(base_path, normalized_path, StringUnitl::stringLength(base_path)) == 0;
	}

	void ImportAssetDialog::onImageGUI()
	{
		if (!isImage(m_source))
			return;

		if (ImGui::Checkbox("Convert to raw", &m_convert_to_raw))
		{
			if (m_convert_to_raw) m_convert_to_dds = false;
		}

		int dxt_quality = 0;
		if (ImGui::Combo("Quality", &dxt_quality, "Super Fast\0Fast\0Normal\0Better\0Uber\0"))
		{
		}

		int quality_lvl = 0;
		if (ImGui::DragInt("Quality level", &quality_lvl, 1, 0, 255))
		{
		}

		int compressor_type = 0;
		if (ImGui::Combo("Compressor type", &compressor_type, "CRN\0CRN fast\0RYG\0"))
		{
		}

		if (ImGui::Checkbox("Normal map", &m_is_normal_map))
		{
			if (m_is_normal_map) m_convert_to_dds = true;
		}
		if (m_convert_to_raw)
		{
			ImGui::SameLine();
			ImGui::DragFloat("Scale", &m_raw_texture_scale, 1.0f, 0.01f, 256.0f);
		}
		if (ImGui::Checkbox("Convert to DDS", &m_convert_to_dds))
		{
			if (m_convert_to_dds) m_convert_to_raw = false;
		}
		ImGui::InputText("Output directory", m_output_dir, sizeof(m_output_dir));
		ImGui::SameLine();
		if (ImGui::Button("...###browseoutput"))
		{
			auto* base_path = getDiskBasePath();
			PlatformInterface::getOpenDirectory(m_output_dir, sizeof(m_output_dir), base_path);
		}

		if (m_image.data)
		{
			ImGui::LabelText("Size", "%d x %d", m_image.width, m_image.height);

			ImGui::InputInt2("Resize", m_image.resize_size);
			if (ImGui::Button("Resize"))
				resizeImage(this, m_image.resize_size[0], m_image.resize_size[1]);
		}

		if (ImGui::Button("Import texture"))
			importTexture();
	}

	void ImportAssetDialog::clearSources()
	{
		checkTask(true);
		stbi_image_free(m_image.data);
		m_image.data = nullptr;
		resourceSer->clearSources();
		m_mesh_output_filename[0] = '\0';
	}

	void ImportAssetDialog::addSource(const char* src)
	{
		StringUnitl::copyString(m_source, src);
		checkSource();
		checkTask(true);
	}

}
