#include "common/resource/entity_manager.h"
#include "common/egal-d.h"

#include "common/resource/resource_manager.h"
#include "common/resource/material_manager.h"

#include "common/lua/lua_manager.h"

namespace egal
{
	static const ResourceType MATERIAL_TYPE("material");
	e_bool Entity::force_keep_skin = false;

	Mesh::Mesh
	(
		Material* mat,
		const bgfx::VertexDecl& vertex_decl,
		const e_char* name,
		IAllocator& allocator
	)	: name(name, allocator)
		, vertex_decl(vertex_decl)
		, material(mat)
		, instance_idx(-1)
		, indices(allocator)
		, vertices(allocator)
		, uvs(allocator)
		, skin(allocator)
	{
	}

	e_void Mesh::set(const Mesh& rhs)
	{
		type = rhs.type;
		indices = rhs.indices;
		vertices = rhs.vertices;
		uvs = rhs.uvs;
		skin = rhs.skin;
		flags = rhs.flags;
		layer_mask = rhs.layer_mask;
		instance_idx = rhs.instance_idx;
		indices_count = rhs.indices_count;
		vertex_decl = rhs.vertex_decl;
		vertex_buffer_handle = rhs.vertex_buffer_handle;
		index_buffer_handle = rhs.index_buffer_handle;
		name = rhs.name;
		// all except material
	}

	e_void Mesh::setMaterial(Material* new_material, Entity& Entity, Renderer& renderer)
	{
		if (material) 
			material->getResourceManager().unload(*material);
		
		material = new_material;
		static const e_int32 transparent_layer = renderer.getLayer("transparent");
		layer_mask = material->getRenderLayerMask();
		if (material->getRenderLayer() == transparent_layer)
		{
			type = Mesh::RIGID;
		}
		else if (material->getLayersCount() > 0)
		{
			if (Entity.getBoneCount() > 0)
			{
				type = Mesh::MULTILAYER_SKINNED;
			}
			else
			{
				type = Mesh::MULTILAYER_RIGID;
			}
		}
		else if (Entity.getBoneCount() > 0)
		{
			type = skin.empty() ? Mesh::RIGID_INSTANCED : Mesh::SKINNED;
		}
		else type = Mesh::RIGID_INSTANCED;
	}


	Entity::Entity(const ArchivePath& path, ResourceManagerBase& resource_manager, Renderer& renderer, IAllocator& allocator)
		: Resource(path, resource_manager, allocator)
		, m_bounding_radius()
		, m_allocator(allocator)
		, m_bone_map(m_allocator)
		, m_meshes(m_allocator)
		, m_bones(m_allocator)
		, m_first_nonroot_bone_index(0)
		, m_flags(0)
		, m_loading_flags(0)
		, m_renderer(renderer)
	{
		if (force_keep_skin) m_loading_flags = (e_uint32)LoadingFlags::KEEP_SKIN;
		m_lods[0] = { 0, -1, FLT_MAX };
		m_lods[1] = { 0, -1, FLT_MAX };
		m_lods[2] = { 0, -1, FLT_MAX };
		m_lods[3] = { 0, -1, FLT_MAX };
	}


	Entity::~Entity()
	{
		ASSERT(isEmpty());
	}


	static float3 evaluateSkin(float3& p, Mesh::Skin s, const float4x4* matrices)
	{
		float4x4 m = matrices[s.indices[0]] * s.weights.x + matrices[s.indices[1]] * s.weights.y +
			matrices[s.indices[2]] * s.weights.z + matrices[s.indices[3]] * s.weights.w;

		return m.transformPoint(p);
	}


	static e_void computeSkinMatrices(const Pose& pose, const Entity& Entity, float4x4* matrices)
	{
		for (e_int32 i = 0; i < pose.count; ++i)
		{
			auto& bone = Entity.getBone(i);
			RigidTransform tmp = { pose.positions[i], pose.rotations[i] };
			matrices[i] = (tmp * bone.inv_bind_transform).toMatrix();
		}
	}


	RayCastModelHit Entity::castRay(const float3& origin, const float3& dir, const Matrix& model_transform, const Pose* pose)
	{
		RayCastModelHit hit;
		hit.m_is_hit = false;
		if (!isReady()) return hit;

		Matrix inv = model_transform;
		inv.inverse();
		float3 local_origin = inv.transformPoint(origin);
		float3 local_dir = static_cast<float3>(inv * float4(dir.x, dir.y, dir.z, 0));

		Matrix matrices[256];
		ASSERT(!pose || pose->count <= TlengthOf(matrices));
		e_bool is_skinned = false;
		for (e_int32 mesh_index = m_lods[0].from_mesh; mesh_index <= m_lods[0].to_mesh; ++mesh_index)
		{
			Mesh& mesh = m_meshes[mesh_index];
			is_skinned = pose && !mesh.skin.empty() && pose->count <= TlengthOf(matrices);
		}
		if (is_skinned)
		{
			computeSkinMatrices(*pose, *this, matrices);
		}

		for (e_int32 mesh_index = m_lods[0].from_mesh; mesh_index <= m_lods[0].to_mesh; ++mesh_index)
		{
			Mesh& mesh = m_meshes[mesh_index];
			e_bool is_mesh_skinned = !mesh.skin.empty();
			e_uint16* indices16 = (e_uint16*)&mesh.indices[0];
			e_uint32* indices32 = (e_uint32*)&mesh.indices[0];
			e_bool is16 = mesh.flags & (e_uint32)Mesh::Flags::INDICES_16_BIT;
			e_int32 index_size = is16 ? 2 : 4;
			for (e_int32 i = 0, c = mesh.indices.size() / index_size; i < c; i += 3)
			{
				float3 p0, p1, p2;
				if (is16)
				{
					p0 = mesh.vertices[indices16[i]];
					p1 = mesh.vertices[indices16[i + 1]];
					p2 = mesh.vertices[indices16[i + 2]];
					if (is_mesh_skinned)
					{
						p0 = evaluateSkin(p0, mesh.skin[indices16[i]], matrices);
						p1 = evaluateSkin(p1, mesh.skin[indices16[i + 1]], matrices);
						p2 = evaluateSkin(p2, mesh.skin[indices16[i + 2]], matrices);
					}
				}
				else
				{
					p0 = mesh.vertices[indices32[i]];
					p1 = mesh.vertices[indices32[i + 1]];
					p2 = mesh.vertices[indices32[i + 2]];
					if (is_mesh_skinned)
					{
						p0 = evaluateSkin(p0, mesh.skin[indices32[i]], matrices);
						p1 = evaluateSkin(p1, mesh.skin[indices32[i + 1]], matrices);
						p2 = evaluateSkin(p2, mesh.skin[indices32[i + 2]], matrices);
					}
				}


				float3 normal = crossProduct(p1 - p0, p2 - p0);
				e_float q = dotProduct(normal, local_dir);
				if (q == 0)	continue;

				e_float d = -dotProduct(normal, p0);
				e_float t = -(dotProduct(normal, local_origin) + d) / q;
				if (t < 0) continue;

				float3 hit_point = local_origin + local_dir * t;

				float3 edge0 = p1 - p0;
				float3 VP0 = hit_point - p0;
				if (dotProduct(normal, crossProduct(edge0, VP0)) < 0) continue;

				float3 edge1 = p2 - p1;
				float3 VP1 = hit_point - p1;
				if (dotProduct(normal, crossProduct(edge1, VP1)) < 0) continue;

				float3 edge2 = p0 - p2;
				float3 VP2 = hit_point - p2;
				if (dotProduct(normal, crossProduct(edge2, VP2)) < 0) continue;

				if (!hit.m_is_hit || hit.m_t > t)
				{
					hit.m_is_hit = true;
					hit.m_t = t;
					hit.m_mesh = &m_meshes[mesh_index];
				}
			}
		}
		hit.m_origin = origin;
		hit.m_dir = dir;
		return hit;
	}


	e_void Entity::getRelativePose(Pose& pose)
	{
		ASSERT(pose.count == getBoneCount());
		float3* pos = pose.positions;
		Quaternion* rot = pose.rotations;
		for (e_int32 i = 0, c = getBoneCount(); i < c; ++i)
		{
			pos[i] = m_bones[i].relative_transform.pos;
			rot[i] = m_bones[i].relative_transform.rot;
		}
		pose.is_absolute = false;
	}


	e_void Entity::getPose(Pose& pose)
	{
		ASSERT(pose.count == getBoneCount());
		float3* pos = pose.positions;
		Quaternion* rot = pose.rotations;
		for (e_int32 i = 0, c = getBoneCount(); i < c; ++i)
		{
			pos[i] = m_bones[i].transform.pos;
			rot[i] = m_bones[i].transform.rot;
		}
		pose.is_absolute = true;
	}


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
			else
			{
				ASSERT(false);
				return false;
			}
		}

		vertex_decl->end();
		return true;
	}


	e_void Entity::onBeforeReady()
	{
		static const e_int32 transparent_layer = m_renderer.getLayer("transparent");
		for (Mesh& mesh : m_meshes)
		{
			mesh.layer_mask = mesh.material->getRenderLayerMask();
			if (mesh.material->getRenderLayer() == transparent_layer)
			{
				mesh.type = Mesh::RIGID;
			}
			else if (mesh.material->getLayersCount() > 0)
			{
				if (getBoneCount() > 0)
				{
					mesh.type = Mesh::MULTILAYER_SKINNED;
				}
				else
				{
					mesh.type = Mesh::MULTILAYER_RIGID;
				}
			}
			else if (getBoneCount() > 0)
			{
				mesh.type = mesh.skin.empty() ? Mesh::RIGID_INSTANCED : Mesh::SKINNED;
			}
			else mesh.type = Mesh::RIGID_INSTANCED;
		}
	}


	e_void Entity::setKeepSkin()
	{
		if (m_loading_flags & (e_uint32)LoadingFlags::KEEP_SKIN) return;
		m_loading_flags = m_loading_flags | (e_uint32)LoadingFlags::KEEP_SKIN;
		if (isReady()) m_resource_manager.reload(*this);
	}


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

	e_int32 Entity::getBoneIdx(const e_char* name)
	{
		for (e_int32 i = 0, c = m_bones.size(); i < c; ++i)
		{
			if (m_bones[i].name == name)
			{
				return i;
			}
		}
		return -1;
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
			file.read(material_name, str_size);
			if (str_size >= MAX_PATH_LENGTH) return false;

			material_name[str_size] = 0;

			e_char material_path[MAX_PATH_LENGTH];
			StringUnitl::copyString(material_path, model_dir);
			StringUnitl::catString(material_path, material_name);
			StringUnitl::catString(material_path, ".mat");

			auto* material_manager = m_resource_manager.getOwner().get(MATERIAL_TYPE);
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
		TVector<Offsets> mesh_offsets(m_allocator);
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

			auto* material_manager = m_resource_manager.getOwner().get(MATERIAL_TYPE);
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
		TVector<e_uint8> indices(m_allocator);
		indices.resize(indices_count * index_size);
		file.read(&indices[0], indices.size());

		e_int32 vertices_size = 0;
		file.read(&vertices_size, sizeof(vertices_size));
		if (vertices_size <= 0) return false;

		TVector<e_uint8> vertices(m_allocator);
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


	static float3 getBonePosition(Entity* Entity, e_int32 bone_index)
	{
		return Entity->getBone(bone_index).transform.pos;
	}


	static e_int32 getBoneParent(Entity* Entity, e_int32 bone_index)
	{
		return Entity->getBone(bone_index).parent_idx;
	}


	e_void Entity::registerLuaAPI(lua_State* L)
	{
#define REGISTER_FUNCTION(F)\
		do { \
			auto f = &lua_manager::wrapMethod<Entity, decltype(&Entity::F), &Entity::F>; \
			lua_manager::createSystemFunction(L, "Entity", #F, f); \
		} while(false) \

		REGISTER_FUNCTION(getBoneCount);

#undef REGISTER_FUNCTION


#define REGISTER_FUNCTION(F)\
		do { \
			auto f = &lua_manager::wrap<decltype(&F), &F>; \
			lua_manager::createSystemFunction(L, "Entity", #F, f); \
		} while(false) \

		REGISTER_FUNCTION(getBonePosition);
		REGISTER_FUNCTION(getBoneParent);

#undef REGISTER_FUNCTION
	}


	e_void Entity::unload()
	{
		auto* material_manager = m_resource_manager.getOwner().get(MATERIAL_TYPE);
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

namespace egal
{
	Resource* EntityManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Entity)(path, *this, m_renderer, m_allocator);
	}

	e_void EntityManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Entity*>(&resource));
	}
}
