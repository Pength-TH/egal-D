#include "common/resource/entity_manager.h"
#include "common/egal-d.h"

#include "common/resource/resource_manager.h"
#include "common/resource/material_manager.h"

#include "common/lua/lua_manager.h"

namespace egal
{
	e_bool Entity::parseVertexDecl(FS::IFile& file, bgfx::VertexDecl* vertex_decl)
	{
		vertex_decl->begin();

		e_uint32 attribute_count;
		file.read(&attribute_count, sizeof(attribute_count));

		for (e_uint32 i = 0; i < attribute_count; ++i)
		{
			e_char tmp[50];
			e_uint32 len;
			file.read(&len, sizeof(len));
			if (len > sizeof(tmp) - 1)
			{
				return false;
			}
			file.read(tmp, len);
			tmp[len] = '\0';

			if (StringUnitl::equalStrings(tmp, "in_position"))
			{
				vertex_decl->add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
			}
			else if (StringUnitl::equalStrings(tmp, "in_colors"))
			{
				vertex_decl->add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false);
			}
			else if (StringUnitl::equalStrings(tmp, "in_tex_coords"))
			{
				vertex_decl->add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
			}
			else if (StringUnitl::equalStrings(tmp, "in_normal"))
			{
				vertex_decl->add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true);
			}
			else if (StringUnitl::equalStrings(tmp, "in_tangents"))
			{
				vertex_decl->add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, true);
			}
			else if (StringUnitl::equalStrings(tmp, "in_weights"))
			{
				vertex_decl->add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
			}
			else if (StringUnitl::equalStrings(tmp, "in_indices"))
			{
				vertex_decl->add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Int16, false, true);
			}
			else
			{
				ASSERT(false);
				return false;
			}

			e_uint32 type;
			file.read(&type, sizeof(type));
		}

		vertex_decl->end();
		return true;
	}
	
	e_bool Entity::parseVertexDeclEx(FS::IFile& file, bgfx::VertexDecl* vertex_decl)
	{
		vertex_decl->begin();

		e_uint32 attribute_count;
		file.read(&attribute_count, sizeof(attribute_count));

		for (e_uint32 i = 0; i < attribute_count; ++i)
		{
			e_int32 attr;
			file.read(&attr, sizeof(attr));

			if (attr == (e_int32)Attrs::Position)
			{
				vertex_decl->add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::Color0)
			{
				vertex_decl->add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true, false);
			}
			else if (attr == (e_int32)Attrs::TexCoord0)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::Normal)
			{
				vertex_decl->add(bgfx::Attrib::Normal, 4, bgfx::AttribType::Uint8, true, true);
			}
			else if (attr == (e_int32)Attrs::Tangent)
			{
				vertex_decl->add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Uint8, true, true);
			}
			else if (attr == (e_int32)Attrs::Weight)
			{
				vertex_decl->add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::Indices)
			{
				vertex_decl->add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Int16, false, true);
			}
			else if (attr == (e_int32)Attrs::Color1)
			{
				vertex_decl->add(bgfx::Attrib::Color1, 4, bgfx::AttribType::Uint8, true, false);
			}
			else if (attr == (e_int32)Attrs::TexCoord1)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord2)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord2, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord3)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord3, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord4)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord4, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord5)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord5, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord6)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord6, 2, bgfx::AttribType::Float);
			}
			else if (attr == (e_int32)Attrs::TexCoord7)
			{
				vertex_decl->add(bgfx::Attrib::TexCoord7, 2, bgfx::AttribType::Float);
			}
			else
			{
				ASSERT(false);
				return false;
			}
		}

		vertex_decl->end();
		return true;
	}

	egal::e_void Entity::save(const char * filepath)
	{
		FS::IFile* _file = g_file_system->open(g_file_system->getDiskDevice(), filepath, FS::Mode::CREATE_AND_WRITE);
		if (!_file)
		{
			log_error("FBX Failed to create %s.", filepath);
			return;
		}
		FS::IFile& out_file = *_file;

		FileHeader header;
		header.magic = 0x5f4c4d4f; // == '_LMO';
		header.version = (e_uint32)FileVersion::LATEST;
		out_file << header;

		e_uint32 flags = 0;
		out_file << flags;

		saveMeshes(out_file, (FileVersion)header.version);

		saveBones(out_file);

		saveBones(out_file);

		_file->close();
	}

	e_bool Entity::saveBones(FS::IFile& file)
	{
		e_int32 bone_count = m_bones.size();
		file << bone_count;

		if (bone_count < 0) 
			return false;
		
		if (bone_count > Bone::MAX_COUNT)
		{
			log_waring("Entity %s has too many bones con't save.", getPath().c_str());
			return false;
		}

		for (e_int32 i = 0; i < bone_count; ++i)
		{
			Entity::Bone& b = m_bones.at(i);
			
			e_int32 len = sizeof(b.name);
			if (len >= MAX_PATH_LENGTH)
			{
				log_error("bone name is too long.");
				return false;
			}
			file.writeString(&b.name);

			len = sizeof(b.parent);
			file.writeString(&b.parent);

			file << b.transform.pos;
			file << b.transform.rot;
		}
		return true;
	}
	e_bool Entity::saveMeshes(FS::IFile& file, FileVersion version)
	{
		if (version <= FileVersion::MULTIPLE_VERTEX_DECLS)
			log_error("not support version file.");

		e_int32 meshCount = getMeshCount();
		file << meshCount;

		for (e_int32 i = 0; i < meshCount; ++i)
		{
			Mesh& mesh = m_meshes.at(i);

			e_uint32 attribute_count = 3;
			bool hasColor0 = mesh.vertex_decl.has(bgfx::Attrib::Enum::Color0) ? attribute_count++ : attribute_count;
			bool hasColor1 = mesh.vertex_decl.has(bgfx::Attrib::Enum::Color1) ? attribute_count++ : attribute_count;
			bool hasTangent = mesh.vertex_decl.has(bgfx::Attrib::Enum::Tangent) ? attribute_count++ : attribute_count;
			bool hasIndices = mesh.vertex_decl.has(bgfx::Attrib::Enum::Indices) ? attribute_count++ : attribute_count;
			bool hasWeight = mesh.vertex_decl.has(bgfx::Attrib::Enum::Weight) ? attribute_count++ : attribute_count;
			bool hasuv1 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord1) ? attribute_count++ : attribute_count;
			bool hasuv2 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord2) ? attribute_count++ : attribute_count;
			bool hasuv3 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord3) ? attribute_count++ : attribute_count;
			bool hasuv4 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord4) ? attribute_count++ : attribute_count;
			bool hasuv5 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord5) ? attribute_count++ : attribute_count;
			bool hasuv6 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord6) ? attribute_count++ : attribute_count;
			bool hasuv7 = mesh.vertex_decl.has(bgfx::Attrib::Enum::TexCoord7) ? attribute_count++ : attribute_count;
			
			//VertexDeclEx
			file << attribute_count;
			file << (e_int32)Attrs::Position;
			file << (e_int32)Attrs::Normal;
			file << (e_int32)Attrs::TexCoord0;

			if (hasColor0)
				file << (e_int32)Attrs::Color0;
			if (hasColor1)
				file << (e_int32)Attrs::Color1;
			if (hasuv1)
				file << (e_int32)Attrs::TexCoord1;
			if (hasuv2)
				file << (e_int32)Attrs::TexCoord2;
			if (hasuv3)
				file << (e_int32)Attrs::TexCoord3;
			if (hasuv4)
				file << (e_int32)Attrs::TexCoord4;
			if (hasuv5)
				file << (e_int32)Attrs::TexCoord5;
			if (hasuv6)
				file << (e_int32)Attrs::TexCoord6;
			if (hasuv7)
				file << (e_int32)Attrs::TexCoord7;
			if (hasTangent)
				file << (e_int32)Attrs::Tangent;
			if (hasIndices && hasWeight)
			{
				file << (e_int32)Attrs::Indices;
				file << (e_int32)Attrs::Weight;
			}

			e_int32 str_size = sizeof(mesh.material->getPath().c_str());
			if (str_size >= MAX_PATH_LENGTH)
			{
				log_error("material path name is too long");
				return false;
			}

			file << str_size;
			file << mesh.material->getPath().c_str();
			file.writeString(&mesh.name);
		}

		for (e_int32 i = 0; i < meshCount; ++i)
		{
			Mesh& mesh = m_meshes[i];
			
			e_int32 index_size = mesh.indices.size();
			e_int32 indices_count = mesh.indices_count;
			
			file << index_size;
			file << indices_count;

			file << mesh.indices;
		}

		for (e_int32 i = 0; i < meshCount; ++i)
		{
			Mesh& mesh = m_meshes[i];
			e_int32 data_size = mesh.vertices.size();
			
			file << data_size;
			file << mesh.vertices;
		}
		file << m_bounding_radius;
		file << m_aabb._min;
	}
	e_bool Entity::saveLODs(FS::IFile& file)
	{
		e_int32 lod_count = m_lod_count;
		file << lod_count;

		if (lod_count <= 0 || lod_count > TlengthOf(m_lods))
		{
			return false;
		}
		for (e_int32 i = 0; i < lod_count; ++i)
		{
			file << m_lods[i].to_mesh;
			file << m_lods[i].distance;
		}
		return true;
	}

	//--------------------------------------------------

	e_bool Entity::parseBones(FS::IFile& file)
	{
		e_int32 bone_count;
		file.read(&bone_count, sizeof(bone_count));
		if (bone_count < 0) return false;
		if (bone_count > Bone::MAX_COUNT)
		{
			log_waring("Renderer Entity %s has too many bones.", getPath().c_str());
			return false;
		}

		m_bones.reserve(bone_count);
		for (e_int32 i = 0; i < bone_count; ++i)
		{
			Entity::Bone& b = m_bones.emplace(m_allocator);
			e_int32 len;
			file.read(&len, sizeof(len));
			e_char tmp[MAX_PATH_LENGTH];
			if (len >= MAX_PATH_LENGTH)
			{
				return false;
			}
			file.read(tmp, len);
			tmp[len] = 0;
			b.name = tmp;
			m_bone_map.insert(crc32(b.name.c_str()), m_bones.size() - 1);
			file.read(&len, sizeof(len));
			if (len >= MAX_PATH_LENGTH)
			{
				return false;
			}
			if (len > 0)
			{
				file.read(tmp, len);
				tmp[len] = 0;
				b.parent = tmp;
			}
			else
			{
				b.parent = "";
			}
			file.read(&b.transform.pos.x, sizeof(e_float) * 3);
			file.read(&b.transform.rot.x, sizeof(e_float) * 4);
		}
		m_first_nonroot_bone_index = -1;
		for (e_int32 i = 0; i < bone_count; ++i)
		{
			Entity::Bone& b = m_bones[i];
			if (b.parent.length() == 0)
			{
				if (m_first_nonroot_bone_index != -1)
				{
					log_error("Renderer Invalid skeleton in %s.", getPath().c_str());
					return false;
				}
				b.parent_idx = -1;
			}
			else
			{
				b.parent_idx = getBoneIdx(b.parent.c_str());
				if (b.parent_idx > i || b.parent_idx < 0)
				{
					log_error("Renderer Invalid skeleton in %s.", getPath().c_str());
					return false;
				}
				if (m_first_nonroot_bone_index == -1)
				{
					m_first_nonroot_bone_index = i;
				}
			}
		}

		for (e_int32 i = 0; i < m_bones.size(); ++i)
		{
			m_bones[i].inv_bind_transform = m_bones[i].transform.inverted();
		}

		for (e_int32 i = 0; i < m_bones.size(); ++i)
		{
			e_int32 p = m_bones[i].parent_idx;
			if (p >= 0)
			{
				m_bones[i].relative_transform = m_bones[p].inv_bind_transform * m_bones[i].transform;
			}
			else
			{
				m_bones[i].relative_transform = m_bones[i].transform;
			}
		}
		return true;
	}

	e_bool Entity::parseMeshes(const bgfx::VertexDecl& global_vertex_decl, FS::IFile& file, FileVersion version)
	{
		if (version <= FileVersion::MULTIPLE_VERTEX_DECLS) return parseMeshesOld(global_vertex_decl, file, version);

		e_int32 object_count = 0;
		file.read(&object_count, sizeof(object_count));
		if (object_count <= 0) return false;

		e_char model_dir[MAX_PATH_LENGTH];
		StringUnitl::getDir(model_dir, MAX_PATH_LENGTH, getPath().c_str());

		m_meshes.reserve(object_count);
		for (e_int32 i = 0; i < object_count; ++i)
		{
			bgfx::VertexDecl vertex_decl;
			if (!parseVertexDeclEx(file, &vertex_decl)) return false;

			e_int32 str_size;
			file.read(&str_size, sizeof(str_size));
			e_char material_name[MAX_PATH_LENGTH];
			material_name[str_size] = 0;
			file.read(material_name, str_size);
			if (str_size >= MAX_PATH_LENGTH) return false;

			material_name[str_size] = 0;

			e_char material_path[MAX_PATH_LENGTH];
			StringUnitl::copyString(material_path, model_dir);
			StringUnitl::catString(material_path, material_name);
			StringUnitl::catString(material_path, ".mat");

			auto* material_manager = m_resource_manager.getOwner().get(RESOURCE_MATERIAL_TYPE);
			Material* material = static_cast<Material*>(material_manager->load(ArchivePath(material_path)));

			file.read(&str_size, sizeof(str_size));
			e_char mesh_name[MAX_PATH_LENGTH];
			mesh_name[str_size] = 0;
			file.read(mesh_name, str_size);

			m_meshes.emplace(material, vertex_decl, mesh_name, m_allocator);
			addDependency(*material);
		}

		for (e_int32 i = 0; i < object_count; ++i)
		{
			Mesh& mesh = m_meshes[i];
			e_int32 index_size;
			e_int32 indices_count;
			file.read(&index_size, sizeof(index_size));
			if (index_size != 2 && index_size != 0) return false;
			file.read(&indices_count, sizeof(indices_count));
			if (indices_count <= 0) return false;
			mesh.indices.resize(index_size * indices_count);
			file.read(&mesh.indices[0], mesh.indices.size());

			mesh.flags = index_size == 2 ? Mesh::Flags::INDICES_16_BIT : 0;
			mesh.indices_count = indices_count;
			const bgfx::Memory* indices_mem = bgfx::copy(&mesh.indices[0], mesh.indices.size());
			mesh.index_buffer_handle = bgfx::createIndexBuffer(indices_mem);
		}

		for (e_int32 i = 0; i < object_count; ++i)
		{
			Mesh& mesh = m_meshes[i];
			e_int32 data_size;
			file.read(&data_size, sizeof(data_size));
			const bgfx::Memory* vertices_mem = bgfx::alloc(data_size);
			file.read(vertices_mem->data, vertices_mem->size);
			mesh.vertex_buffer_handle = bgfx::createVertexBuffer(vertices_mem, mesh.vertex_decl);

			const bgfx::VertexDecl& vertex_decl = mesh.vertex_decl;
			e_int32 position_attribute_offset = vertex_decl.getOffset(bgfx::Attrib::Position);
			e_int32 uv_attribute_offset = vertex_decl.getOffset(bgfx::Attrib::TexCoord0);
			e_int32 weights_attribute_offset = vertex_decl.getOffset(bgfx::Attrib::Weight);
			e_int32 bone_indices_attribute_offset = vertex_decl.getOffset(bgfx::Attrib::Indices);
			e_bool keep_skin = m_loading_flags & (e_uint32)LoadingFlags::KEEP_SKIN;
			keep_skin = keep_skin && vertex_decl.has(bgfx::Attrib::Weight) && vertex_decl.has(bgfx::Attrib::Indices);

			e_int32 vertex_size = mesh.vertex_decl.getStride();
			e_int32 mesh_vertex_count = vertices_mem->size / mesh.vertex_decl.getStride();
			mesh.vertices.resize(mesh_vertex_count);
			mesh.uvs.resize(mesh_vertex_count);
			if (keep_skin) mesh.skin.resize(mesh_vertex_count);
			const e_uint8* vertices = vertices_mem->data;
			for (e_int32 j = 0; j < mesh_vertex_count; ++j)
			{
				e_int32 offset = j * vertex_size;
				if (keep_skin)
				{
					mesh.skin[j].weights = *(const float4*)&vertices[offset + weights_attribute_offset];
					StringUnitl::copyMemory(mesh.skin[j].indices,
						&vertices[offset + bone_indices_attribute_offset],
						sizeof(mesh.skin[j].indices));
				}
				mesh.vertices[j] = *(const float3*)&vertices[offset + position_attribute_offset];
				mesh.uvs[j] = *(const float2*)&vertices[offset + uv_attribute_offset];
			}
		}
		file.read(&m_bounding_radius, sizeof(m_bounding_radius));
		file.read(&m_aabb, sizeof(m_aabb));

		return true;
	}


	e_bool Entity::parseMeshesOld(bgfx::VertexDecl global_vertex_decl, FS::IFile& file, FileVersion version)
	{
		e_int32 object_count = 0;
		file.read(&object_count, sizeof(object_count));
		if (object_count <= 0) return false;

		m_meshes.reserve(object_count);
		e_char model_dir[MAX_PATH_LENGTH];
		StringUnitl::getDir(model_dir, MAX_PATH_LENGTH, getPath().c_str());
		struct Offsets
		{
			e_int32 attribute_array_offset;
			e_int32 attribute_array_size;
			e_int32 indices_offset;
			e_int32 mesh_tri_count;
		};
		TArrary<Offsets> mesh_offsets(m_allocator);
		for (e_int32 i = 0; i < object_count; ++i)
		{
			e_int32 str_size;
			file.read(&str_size, sizeof(str_size));
			e_char material_name[MAX_PATH_LENGTH];
			file.read(material_name, str_size);
			if (str_size >= MAX_PATH_LENGTH) return false;

			material_name[str_size] = 0;

			e_char material_path[MAX_PATH_LENGTH];
			StringUnitl::copyString(material_path, model_dir);
			StringUnitl::catString(material_path, material_name);
			StringUnitl::catString(material_path, ".mat");

			auto* material_manager = m_resource_manager.getOwner().get(RESOURCE_MATERIAL_TYPE);
			Material* material = static_cast<Material*>(material_manager->load(ArchivePath(material_path)));

			Offsets& offsets = mesh_offsets.emplace();
			file.read(&offsets.attribute_array_offset, sizeof(offsets.attribute_array_offset));
			file.read(&offsets.attribute_array_size, sizeof(offsets.attribute_array_size));
			file.read(&offsets.indices_offset, sizeof(offsets.indices_offset));
			file.read(&offsets.mesh_tri_count, sizeof(offsets.mesh_tri_count));

			file.read(&str_size, sizeof(str_size));
			if (str_size >= MAX_PATH_LENGTH)
			{
				material_manager->unload(*material);
				return false;
			}

			e_char mesh_name[MAX_PATH_LENGTH];
			mesh_name[str_size] = 0;
			file.read(mesh_name, str_size);

			bgfx::VertexDecl vertex_decl = global_vertex_decl;
			if (version <= FileVersion::SINGLE_VERTEX_DECL)
			{
				parseVertexDecl(file, &vertex_decl);
				if (i != 0 && global_vertex_decl.m_hash != vertex_decl.m_hash)
				{
					log_error("Renderer Entity %s contains meshes with different vertex declarations.", getPath().c_str());
				}
				if (i == 0) global_vertex_decl = vertex_decl;
			}


			m_meshes.emplace(material,
				vertex_decl,
				mesh_name,
				m_allocator);
			addDependency(*material);
		}

		e_int32 indices_count = 0;
		file.read(&indices_count, sizeof(indices_count));
		if (indices_count <= 0) return false;

		e_int32 index_size = (m_flags & (e_uint32)Entity::Flags::INDICES_16BIT) ? 2 : 4;
		TArrary<e_uint8> indices(m_allocator);
		indices.resize(indices_count * index_size);
		file.read(&indices[0], indices.size());

		e_int32 vertices_size = 0;
		file.read(&vertices_size, sizeof(vertices_size));
		if (vertices_size <= 0) return false;

		TArrary<e_uint8> vertices(m_allocator);
		vertices.resize(vertices_size);
		file.read(&vertices[0], vertices.size());

		e_int32 vertex_count = 0;
		for (const Offsets& offsets : mesh_offsets)
		{
			vertex_count += offsets.attribute_array_size / global_vertex_decl.getStride();
		}

		if (version > FileVersion::BOUNDING_SHAPES_PRECOMPUTED)
		{
			file.read(&m_bounding_radius, sizeof(m_bounding_radius));
			file.read(&m_aabb, sizeof(m_aabb));
		}

		e_float bounding_radius_squared = 0;
		float3 min_vertex(0, 0, 0);
		float3 max_vertex(0, 0, 0);

		e_int32 vertex_size = global_vertex_decl.getStride();
		e_int32 position_attribute_offset = global_vertex_decl.getOffset(bgfx::Attrib::Position);
		e_int32 uv_attribute_offset = global_vertex_decl.getOffset(bgfx::Attrib::TexCoord0);
		e_int32 weights_attribute_offset = global_vertex_decl.getOffset(bgfx::Attrib::Weight);
		e_int32 bone_indices_attribute_offset = global_vertex_decl.getOffset(bgfx::Attrib::Indices);
		e_bool keep_skin = m_loading_flags & (e_uint32)LoadingFlags::KEEP_SKIN;
		keep_skin = keep_skin && global_vertex_decl.has(bgfx::Attrib::Weight) && global_vertex_decl.has(bgfx::Attrib::Indices);
		for (e_int32 i = 0; i < m_meshes.size(); ++i)
		{
			Offsets& offsets = mesh_offsets[i];
			Mesh& mesh = m_meshes[i];
			mesh.indices_count = offsets.mesh_tri_count * 3;
			mesh.indices.resize(mesh.indices_count * index_size);
			StringUnitl::copyMemory(&mesh.indices[0], &indices[offsets.indices_offset * index_size], mesh.indices_count * index_size);

			e_int32 mesh_vertex_count = offsets.attribute_array_size / global_vertex_decl.getStride();
			e_int32 mesh_attributes_array_offset = offsets.attribute_array_offset;
			mesh.vertices.resize(mesh_vertex_count);
			mesh.uvs.resize(mesh_vertex_count);
			if (keep_skin) mesh.skin.resize(mesh_vertex_count);
			for (e_int32 j = 0; j < mesh_vertex_count; ++j)
			{
				e_int32 offset = mesh_attributes_array_offset + j * vertex_size;
				if (keep_skin)
				{
					mesh.skin[j].weights = *(const float4*)&vertices[offset + weights_attribute_offset];
					StringUnitl::copyMemory(mesh.skin[j].indices,
						&vertices[offset + bone_indices_attribute_offset],
						sizeof(mesh.skin[j].indices));
				}
				mesh.vertices[j] = *(const float3*)&vertices[offset + position_attribute_offset];
				mesh.uvs[j] = *(const float2*)&vertices[offset + uv_attribute_offset];
				e_float sq_len = mesh.vertices[j].squaredLength();
				bounding_radius_squared = Math::maximum(bounding_radius_squared, sq_len > 0 ? sq_len : 0);
				min_vertex.x = Math::minimum(min_vertex.x, mesh.vertices[j].x);
				min_vertex.y = Math::minimum(min_vertex.y, mesh.vertices[j].y);
				min_vertex.z = Math::minimum(min_vertex.z, mesh.vertices[j].z);
				max_vertex.x = Math::maximum(max_vertex.x, mesh.vertices[j].x);
				max_vertex.y = Math::maximum(max_vertex.y, mesh.vertices[j].y);
				max_vertex.z = Math::maximum(max_vertex.z, mesh.vertices[j].z);
			}
		}

		if (version <= FileVersion::BOUNDING_SHAPES_PRECOMPUTED)
		{
			m_bounding_radius = sqrt(bounding_radius_squared);
			m_aabb = AABB(min_vertex, max_vertex);
		}

		for (e_int32 i = 0; i < m_meshes.size(); ++i)
		{
			Mesh& mesh = m_meshes[i];
			Offsets offsets = mesh_offsets[i];

			ASSERT(!bgfx::isValid(mesh.index_buffer_handle));
			mesh.flags = (m_flags & (e_uint32)Flags::INDICES_16BIT) != 0 ? Mesh::Flags::INDICES_16_BIT : 0;
			e_int32 indices_size = index_size * mesh.indices_count;
			const bgfx::Memory* mem = bgfx::copy(&indices[offsets.indices_offset * index_size], indices_size);
			mesh.index_buffer_handle = bgfx::createIndexBuffer(mem, index_size == 4 ? BGFX_BUFFER_INDEX32 : 0);
			if (!bgfx::isValid(mesh.index_buffer_handle)) return false;

			ASSERT(!bgfx::isValid(mesh.vertex_buffer_handle));
			const bgfx::Memory* vertices_mem = bgfx::copy(&vertices[offsets.attribute_array_offset], offsets.attribute_array_size);
			mesh.vertex_buffer_handle = bgfx::createVertexBuffer(vertices_mem, mesh.vertex_decl);
			if (!bgfx::isValid(mesh.vertex_buffer_handle)) return false;
		}

		return true;
	}


	e_bool Entity::parseLODs(FS::IFile& file)
	{
		e_int32 lod_count;
		file.read(&lod_count, sizeof(lod_count));
		m_lod_count = lod_count;
		if (lod_count <= 0 || lod_count > TlengthOf(m_lods))
		{
			return false;
		}
		for (e_int32 i = 0; i < lod_count; ++i)
		{
			file.read(&m_lods[i].to_mesh, sizeof(m_lods[i].to_mesh));
			file.read(&m_lods[i].distance, sizeof(m_lods[i].distance));
			m_lods[i].from_mesh = i > 0 ? m_lods[i - 1].to_mesh + 1 : 0;
		}
		return true;
	}

	e_bool Entity::load(FS::IFile& file)
	{
		//PROFILE_FUNCTION();
		FileHeader header;
		file.read(&header, sizeof(header));

		if (header.magic != FILE_MAGIC)
		{
			log_waring("Renderer Corrupted Entity %s.", getPath().c_str());
			return false;
		}

		if (header.version > (e_uint32)FileVersion::LATEST)
		{
			log_waring("Renderer Unsupported version of Entity %s.", getPath().c_str());
			return false;
		}

		m_flags = 0;
		if (header.version > (e_uint32)FileVersion::WITH_FLAGS)
		{
			file.read(&m_flags, sizeof(m_flags));
		}

		bgfx::VertexDecl global_vertex_decl;
		if (header.version > (e_uint32)FileVersion::SINGLE_VERTEX_DECL && header.version <= (e_uint32)FileVersion::MULTIPLE_VERTEX_DECLS)
		{
			parseVertexDeclEx(file, &global_vertex_decl);
		}

		if (parseMeshes(global_vertex_decl, file, (FileVersion)header.version)
			&& parseBones(file)
			&& parseLODs(file))
		{
			m_size = file.size();
			return true;
		}

		log_error("Renderer Error loading Entity %s.", getPath().c_str());
		return false;
	}
	
	e_void Entity::unload()
	{
		auto* material_manager = m_resource_manager.getOwner().get(RESOURCE_MATERIAL_TYPE);
		for (e_int32 i = 0; i < m_meshes.size(); ++i)
		{
			removeDependency(*m_meshes[i].material);
			material_manager->unload(*m_meshes[i].material);
		}
		for (Mesh& mesh : m_meshes)
		{
			if (bgfx::isValid(mesh.index_buffer_handle)) bgfx::destroy(mesh.index_buffer_handle);
			if (bgfx::isValid(mesh.vertex_buffer_handle)) bgfx::destroy(mesh.vertex_buffer_handle);
		}
		m_meshes.clear();
		m_bones.clear();
	}
}
