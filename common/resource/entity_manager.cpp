#include "common/resource/entity_manager.h"
#include "common/egal-d.h"

#include "common/resource/resource_manager.h"
#include "common/resource/material_manager.h"

#include "common/lua/lua_manager.h"

namespace egal
{
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


	Pose::Pose(IAllocator& allocator)
		: allocator(allocator)
	{
		positions = nullptr;
		rotations = nullptr;
		count = 0;
		is_absolute = false;
	}

	Pose::~Pose()
	{
		allocator.deallocate(positions);
		allocator.deallocate(rotations);
	}

	e_void Pose::blend(Pose& rhs, e_float weight)
	{
		ASSERT(count == rhs.count);
		if (weight <= 0.001f)
			return;
		weight = Math::clamp(weight, 0.0f, 1.0f);
		e_float inv = 1.0f - weight;
		for (e_int32 i = 0, c = count; i < c; ++i)
		{
			positions[i] = positions[i] * inv + rhs.positions[i] * weight;
			nlerp(rotations[i], rhs.rotations[i], &rotations[i], weight);
		}
	}

	e_void Pose::resize(e_int32 count)
	{
		is_absolute = false;
		allocator.deallocate(positions);
		allocator.deallocate(rotations);
		this->count = count;
		if (count)
		{
			positions = static_cast<float3*>(allocator.allocate(sizeof(float3) * count));
			rotations = static_cast<Quaternion*>(allocator.allocate(sizeof(Quaternion) * count));
		}
		else
		{
			positions = nullptr;
			rotations = nullptr;
		}
	}

	e_void Pose::computeAbsolute(Entity& entity)
	{
		//PROFILE_FUNCTION();
		if (is_absolute)
			return;
		for (e_int32 i = entity.getFirstNonrootBoneIndex(); i < count; ++i)
		{
			e_int32 parent = entity.getBone(i).parent_idx;
			positions[i] = rotations[parent].rotate(positions[i]) + positions[parent];
			rotations[i] = rotations[parent] * rotations[i];
		}
		is_absolute = true;
	}

	e_void Pose::computeRelative(Entity& entity)
	{
		//PROFILE_FUNCTION();
		if (!is_absolute) return;
		for (e_int32 i = count - 1; i >= entity.getFirstNonrootBoneIndex(); --i)
		{
			e_int32 parent = entity.getBone(i).parent_idx;
			positions[i] = -rotations[parent].rotate(positions[i] - positions[parent]);
			rotations[i] = rotations[i] * -rotations[parent];
		}
		is_absolute = false;
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
		, m_lod_count(0)
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


	RayCastEntityHit Entity::castRay(const float3& origin, const float3& dir, const Matrix& model_transform, const Pose* pose)
	{
		RayCastEntityHit hit;
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



}

namespace egal
{
	Resource* EntityManager::createResource(const ArchivePath& path)
	{
		return _aligned_new(m_allocator, Entity)(path, *this, m_renderer, m_allocator);
	}

	egal::Resource* EntityManager::createResource()
	{
		return _aligned_new(m_allocator, Entity)(ArchivePath(""), *this, m_renderer, m_allocator);
	}

	e_void EntityManager::destroyResource(Resource& resource)
	{
		_delete(m_allocator, static_cast<Entity*>(&resource));
	}


	static float3 getBonePosition(Entity* Entity, e_int32 bone_index)
	{
		return Entity->getBone(bone_index).transform.pos;
	}


	static e_int32 getBoneParent(Entity* Entity, e_int32 bone_index)
	{
		return Entity->getBone(bone_index).parent_idx;
	}

}
