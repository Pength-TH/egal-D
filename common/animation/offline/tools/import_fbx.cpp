#include "common/animation/offline/tools/import_fbx.h"

#include "common/animation/offline/skeleton_builder.h"
#include "common/animation/offline/raw_skeleton.h"

#include "common/animation/offline/fbx/fbx_base.h"
#include "common/animation/offline/fbx/fbx.h"
#include "common/animation/offline/fbx/fbx_animation.h"
#include "common/animation/offline/fbx/fbx_skeleton.h"

#include "common/animation/offline/tools/convert2mesh.h"


#include "common/animation/io/archive.h"
#include "common/animation/offline/mesh.h"

#include "editor/tools/import_assert/import_asset_dialog.h"

#include "common/filesystem/ifile_device.h"

namespace egal
{
	static void getRelativePath(char* relative_path, int max_length, const char* source)
	{
		char tmp[MAX_PATH_LENGTH];
		StringUnitl::normalize(source, tmp, sizeof(tmp));

		const char* base_path = getDiskBasePath();
		if (StringUnitl::compareStringN(base_path, tmp, StringUnitl::stringLength(base_path)) == 0)
		{
			int base_path_length = StringUnitl::stringLength(base_path);
			const char* rel_path_start = tmp + base_path_length;
			if (rel_path_start[0] == '/')
			{
				++rel_path_start;
			}
			StringUnitl::copyString(relative_path, max_length, rel_path_start);
		}
		else
		{
			if (base_path && StringUnitl::compareStringN(base_path, tmp, StringUnitl::stringLength(base_path)) == 0)
			{
				int base_path_length = StringUnitl::stringLength(base_path);
				const char* rel_path_start = tmp + base_path_length;
				if (rel_path_start[0] == '/')
				{
					++rel_path_start;
				}
				StringUnitl::copyString(relative_path, max_length, rel_path_start);
			}
			else
			{
				StringUnitl::copyString(relative_path, max_length, tmp);
			}
		}
	}

	ImportFbx::ImportFbx(ImportAssetDialog& dialog)
		: importAssetDialog(dialog)
	{
	}

	ImportFbx::~ImportFbx()
	{

	}

	e_void ImportFbx::import_fbx(const char* file_path, ImportOption& option)
	{
		//export skeleton
		animation::offline::RawSkeleton raw_skeleton;
		animation::Skeleton*			skeleton = NULL;

		// Import Fbx content.
		animation::offline::fbx::FbxManagerInstance fbx_manager;
		animation::offline::fbx::FbxDefaultIOSettings settings(fbx_manager);
		animation::offline::fbx::FbxSceneLoader scene_loader(option.file_path_name, "", fbx_manager, settings);
		FbxScene* scene = scene_loader.scene();
		if (!scene)
		{
			log_error("Failed to import file %s. the fbx file no scene data.", option.file_path_name);
			return;
		}

		log_info("Triangulating scene.");
		FbxGeometryConverter converter(fbx_manager);
		if (!converter.Triangulate(scene, true))
		{
			log_error("Failed to triangulating meshes.");
			return;
		}

		//export skeleton
		{
			if (ExtractSkeleton(scene_loader, &raw_skeleton))
			{
				if (!option.m_raw)
				{
					// Builds runtime skeleton.
					log_info("Builds runtime skeleton.");
					animation::offline::SkeletonBuilder skeleton_builder;
					skeleton = skeleton_builder(raw_skeleton);
					if (!skeleton)
					{
						log_error("Failed to build runtime skeleton.");
						return;
					}
				}
			}
			else
				log_info("the fbx file has no skeleton.");
		}

		FbxNode* lNode = scene->GetRootNode();
		if (lNode)
		{
			for (int i = 0; i < lNode->GetChildCount(); i++)
			{
				FbxNode* pNode = lNode->GetChild(i);

				FbxNodeAttribute::EType lAttributeType;

				if (pNode->GetNodeAttribute() == NULL)
				{
					log_error("Failed to import file %s. the fbx file no scene data.", option.file_path_name);
					return;
				}
				else
				{
					lAttributeType = (pNode->GetNodeAttribute()->GetAttributeType());

					switch (lAttributeType)
					{
					default:
						break;
					case FbxNodeAttribute::eMarker:
						//DisplayMarker(pNode);
						break;

					case FbxNodeAttribute::eSkeleton:
						//DisplaySkeleton(pNode);
						log_info("skeleton");
						break;

					case FbxNodeAttribute::eMesh:
					{
						import_mesh output_mesh;
						output_mesh.parts.resize(1);
						log_info("mesh");
						convert2mesh(output_mesh, pNode, scene_loader, skeleton, option);
					}break;
					case FbxNodeAttribute::eNurbs:
						//DisplayNurb(pNode);
						break;

					case FbxNodeAttribute::ePatch:
						//DisplayPatch(pNode);
						break;

					case FbxNodeAttribute::eCamera:
						//DisplayCamera(pNode);
						break;

					case FbxNodeAttribute::eLight:
						//DisplayLight(pNode);
						break;

					case FbxNodeAttribute::eLODGroup:
						//DisplayLodGroup(pNode);
						break;
					}
				}
			}
		}
	}

	void writeSkeleton()
	{
		//create skeleton file
		//io::File file(option.out_skeleton_path_name.c_str(), "wb");
		//if (!file.opened())
		//{
		//	log_error("Failed to open output file: %s.", option.out_skeleton_path_name.c_str());

		//	if (skeleton)
		//		g_allocator->deallocate(skeleton);
		//	return;
		//}

		//// Initializes output endianness from options.
		//Endianness endianness = option.m_endian;
		//log_info(endianness == Endianness::little ? "Little Endian output binary format selected." : "Big Endian output binary format selected.");

		//// Initializes output archive.
		//io::OArchive archive(&file, endianness);

		//// Fills output archive with the skeleton.
		//if (option.m_raw && raw_skeleton.Validate())
		//{
		//	log_info("Outputs RawSkeleton to binary archive.");
		//	archive << raw_skeleton;
		//	log_info("Outputs RawSkeleton binary end. at %s.", option.out_skeleton_path_name.c_str());
		//}
		//else if (skeleton)
		//{
		//	log_info("Outputs Skeleton to binary archive.");
		//	archive << *skeleton;
		//	log_info("Outputs Skeleton binary end. at %s.", option.out_skeleton_path_name.c_str());
		//}
	}

	void writemesh(import_mesh& mesh, ImportOption& option)
	{
		FS::OsFile out_file;
		StaticString<MAX_PATH_LENGTH> out_path(option.out_dir, mesh.m_name, ".msh");
		if (!out_file.open(out_path, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("FBX Failed to create %s.", out_path);
			return;
		}
		out_file.close();

		EngineRoot::getSingletonPtr()->getResourceManager().get(RESOURCE_ENTITY_TYPE)->load(ArchivePath(out_path));
		return;
		
		egal::Resource* res_mat = EngineRoot::getSingletonPtr()->getResourceManager().get(RESOURCE_MATERIAL_TYPE)->create();
		egal::Material* material = static_cast<egal::Material*>(res_mat);

		egal::Resource* res = EngineRoot::getSingletonPtr()->getResourceManager().get(RESOURCE_ENTITY_TYPE)->create();
		egal::Entity* entit = static_cast<egal::Entity*>(res);
		
		bgfx::VertexDecl global_vertex_decl;
		global_vertex_decl.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
		global_vertex_decl.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
		global_vertex_decl.add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true);

		if (mesh.colored())
		{
			global_vertex_decl.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false);
		}
		else if (mesh.tangentsed())
		{
			global_vertex_decl.add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, true);
		}
		else if (mesh.skinned())
		{
			global_vertex_decl.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
		}
		else if (mesh.skinned())
		{
			global_vertex_decl.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Int16, false, true);
		}

		egal::Mesh egal_mesh(material, global_vertex_decl, mesh.m_name, *g_allocator);
		
		egal_mesh.indices_count = mesh.triangle_index_count();

		for (int i = 0; i < mesh.parts.size(); i++)
		{
			import_mesh::Part& part = mesh.parts.at(i);
			for (int ii = 0; ii < mesh.triangle_indices.size(); ii++)
			{
				egal_mesh.indices[ii] = mesh.triangle_indices[ii];
			}

			int32_t data_size = part.positions.size() * sizeof(float) + part.uvs.size() * sizeof(float);
			const bgfx::Memory* vertices_mem = bgfx::alloc(data_size);
			for (int i = 0; i < part.positions.size(); i = i +3)
			{
				egal_mesh.vertices[i/3] = float3(part.positions.at(i), part.positions.at(i+1), part.positions.at(i+2));
			}

			for (int i = 0; i < part.uvs.size(); i++)
			{
				egal_mesh.uvs[i/2] = float2(part.uvs.at(i), part.uvs.at(i + 1));
			}
		}

		entit->m_meshes.push_back(egal_mesh);

		//mesh->m_meshes.resize(option.m_meshs.size());

		entit->save(out_path);

		
	}

	void writeMaterial(import_material& material, ImportOption& option)
	{
		FS::OsFile out_file;
		//if (option.out_mesh_path_name[0] != '\0')
		//{
		//	if (!out_file.open(option.out_mesh_path_name.c_str(), FS::Mode::CREATE_AND_WRITE))
		//	{
		//		log_error("FBX Failed to create %s.", mesh_output_filename);
		//		continue;
		//	}
		//}
		//else
		//{
		StaticString<MAX_PATH_LENGTH> path(option.out_dir, material.name, ".mat");
		if (!out_file.open(path, FS::Mode::CREATE_AND_WRITE))
		{
			log_error("FBX Failed to create %s.", path);
			return;
		}
		//}

		out_file << "{\n\t\"shader\" : \"pipelines/rigid/rigid.shd\"";
		out_file << ",\n\t\"defines\" : [\"ALPHA_CUTOUT\"]";

		for (int i = 0; i < material.m_textures.size(); i++)
		{
			if (material.m_textures[i].path[0] != '\0')
			{
				out_file << ",\n\t\"texture\" : { \"source\" : \"";

				char fileName[1024];
				StringUnitl::copyString(fileName, option.out_dir);
				StringUnitl::catString(fileName, material.m_textures[i].path);
				getRelativePath(fileName, 1024, fileName);

				char dirName[1024];
				StringUnitl::getDir(dirName, 1024, fileName);

				char baseName[256];
				StringUnitl::getBasename(baseName, 256, fileName);
				char ext[128];
				StringUnitl::getExtension(ext, 128, fileName);

				e_char skeleton_path[MAX_PATH_LENGTH];
				StringUnitl::copyString(skeleton_path, dirName);
				StringUnitl::catString(skeleton_path, baseName);

				out_file << dirName;
				out_file << baseName;
				out_file << ".";
				out_file << (option.to_dds ? "dds" : ext);
				out_file << "\"";
				out_file << ", \"srgb\" : true ";
				out_file << "}";
			}
			else
			{
				out_file << ",\n\t\"texture\" : {";
				out_file << " \"srgb\" : true ";
				out_file << "}";
			}
		}
		out_file << ",\n\t\"color\" : [" << material.Diffuse.x << "," << material.Diffuse.y << "," << material.Diffuse.z << ",1]";
		out_file << "\n}";

		out_file.close();
	}

	void ImportFbx::convert(ImportOption& option)
	{
		importAssetDialog.setImportMessage("Writing materials...", 0.9f);
		for (int i = 0; i < option.m_materials.size(); i++)
		{
			writeMaterial(option.m_materials[i], option);
		}

		//importAssetDialog.setImportMessage("Writing meshs...", 0.9f);
		//for (int i = 0; i < option.m_meshs.size(); i++)
		//{
		//	writemesh(option.m_meshs[i], option);
		//}

		importAssetDialog.setImportMessage("Writing skeleton...", 0.9f);

		importAssetDialog.setImportMessage("Writing animation...", 0.9f);

	}

}
