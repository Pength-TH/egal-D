#ifndef _entity_manager_h_
#define _entity_manager_h_
#pragma once

#include "common/type.h"
#include "common/struct.h"
#include "common/template.h"
#include "common/math/egal_math.h"
#include "common/resource/resource_manager.h"
#include "common/resource/resource_define.h"
#include "common/utils/geometry.h"
#include "common/egal_string.h"

#include "common/filesystem/binary.h"
#include "common/filesystem/os_file.h"
namespace egal
{
	struct Frustum;
	class Material;
	struct Mesh;
	class Entity;
	struct Pose;
	class Renderer;

	namespace FS
	{
		class FileSystem;
		struct IFile;
	}

	struct RayCastEntityHit
	{
		e_bool  m_is_hit;
		e_float m_t;
		float3  m_origin;
		float3  m_dir;
		Mesh*   m_mesh;

		ComponentHandle m_component;
		GameObject		m_game_object;
		ComponentType	m_component_type;
	};

	struct Mesh
	{
		struct Skin
		{
			float4  weights;
			e_int16 indices[4];
		};

		enum Type : e_uint8
		{
			RIGID_INSTANCED,
			SKINNED,
			MULTILAYER_RIGID,
			MULTILAYER_SKINNED,
			RIGID,
		};

		enum Flags : e_uint8
		{
			INDICES_16_BIT = 1 << 0
		};

		Mesh(Material* mat,
			const bgfx::VertexDecl& vertex_decl,
			const e_char* name,
			IAllocator& allocator);

		e_void set(const Mesh& rhs);

		e_void setMaterial(Material* material, Entity& entity, Renderer& renderer);

		e_bool areIndices16() const { return flags & Flags::INDICES_16_BIT; }

		Type				type;
		TArrary<e_uint8>	indices;
		TArrary<float3>		vertices;
		TArrary<float2>		uvs;
		TArrary<Skin>		skin;
		e_uint8				flags;
		e_uint64			layer_mask;
		e_int32				instance_idx;
		e_int32				indices_count;
		VertexDecl			vertex_decl;
		VertexBufferHandle	vertex_buffer_handle = BGFX_INVALID_HANDLE;
		IndexBufferHandle	index_buffer_handle = BGFX_INVALID_HANDLE;
		String				name;
		Material*			material;
	};

	struct LODMeshIndices
	{
		e_int32 from;
		e_int32 to;
	};

	class Entity;
	struct Pose
	{
		explicit Pose(IAllocator& allocator);
		~Pose();

		e_void resize(e_int32 count);
		e_void computeAbsolute(Entity& entity);
		e_void computeRelative(Entity& entity);
		e_void blend(Pose& rhs, e_float weight);

		IAllocator&		allocator;
		e_bool			is_absolute;
		e_int32			count;
		float3*			positions;
		Quaternion*		rotations;

	private:
		Pose(const Pose&);
		e_void operator =(const Pose&);
	};


	class Entity : public Resource
	{
	public:
		typedef THashMap<e_uint32, e_int32> BoneMap;

		enum class Attrs
		{
			Position,
			Normal,
			Tangent,
			Bitangent,
			Color0,
			Color1,
			Indices,
			Weight,
			TexCoord0,
			TexCoord1,
			TexCoord2,
			TexCoord3,
			TexCoord4,
			TexCoord5,
			TexCoord6,
			TexCoord7,
		};

#pragma pack(1)
		struct FileHeader
		{
			e_uint32 magic;
			e_uint32 version;
		};
#pragma pack()

		enum class FileVersion : e_uint32
		{
			FIRST,
			WITH_FLAGS,
			SINGLE_VERTEX_DECL,
			BOUNDING_SHAPES_PRECOMPUTED,
			MULTIPLE_VERTEX_DECLS,

			LATEST // keep this last
		};

		enum class LoadingFlags : e_uint32
		{
			KEEP_SKIN = 1 << 0
		};

		enum class Flags : e_uint32
		{
			INDICES_16BIT = 1 << 0
		};

		struct LOD
		{
			e_int32 from_mesh;
			e_int32 to_mesh;

			e_float distance;
		};

		struct Bone
		{
			enum { MAX_COUNT = 196 };

			explicit Bone(IAllocator& allocator)
				: name(allocator)
				, parent(allocator)
			{
			}

			String			name;
			String			parent;
			RigidTransform	transform;
			RigidTransform	relative_transform;
			RigidTransform	inv_bind_transform;
			e_int32			parent_idx;
		};

	public:
		Entity(const ArchivePath& path, ResourceManagerBase& resource_manager, Renderer& renderer, IAllocator& allocator);
		~Entity();

		LODMeshIndices getLODMeshIndices(e_float squared_distance) const
		{
			e_int32 i = 0;
			while (squared_distance >= m_lods[i].distance) ++i;
			return { m_lods[i].from_mesh, m_lods[i].to_mesh };
		}

		Mesh& getMesh(e_int32 index) { return m_meshes[index]; }
		const Mesh& getMesh(e_int32 index) const { return m_meshes[index]; }
		const Mesh* getMeshPtr(e_int32 index) const { return &m_meshes[index]; }
		e_int32 getMeshCount() const { return m_meshes.size(); }
		e_int32 getBoneCount() const { return m_bones.size(); }
		const Bone& getBone(e_int32 i) const { return m_bones[i]; }
		e_int32 getFirstNonrootBoneIndex() const { return m_first_nonroot_bone_index; }
		BoneMap::iterator getBoneIndex(e_uint32 hash) { return m_bone_map.find(hash); }
		e_float getBoundingRadius() const { return m_bounding_radius; }
		const AABB& getAABB() const { return m_aabb; }
		LOD* getLODs() { return m_lods; }
		e_uint32 getFlags() const { return m_flags; }

		RayCastEntityHit castRay(const float3& origin
							   , const float3& dir
							   , const Matrix& model_transform
							   , const Pose* pose);
		e_void getPose(Pose& pose);
		e_void getRelativePose(Pose& pose);
		e_void setKeepSkin();
		e_void onBeforeReady() override;
		
	public:
		static const e_uint32 FILE_MAGIC	= 0x5f4c4d4f;
		static const e_int32  MAX_LOD_COUNT = 4;
		static		   e_bool force_keep_skin;

	private:
		Entity(const Entity&);
		e_void operator=(const Entity&);

		e_int32 getBoneIdx(const e_char* name);

		e_void unload() override;
		e_bool load(FS::IFile& file) override;
	public:
		e_void save(const char * filepath);
		e_bool saveBones(FS::IFile& file);
		e_bool saveMeshes(FS::IFile& file, FileVersion version);
		e_bool saveLODs(FS::IFile& file);

		e_bool parseVertexDecl(FS::IFile& file, bgfx::VertexDecl* vertex_decl);
		e_bool parseVertexDeclEx(FS::IFile& file, bgfx::VertexDecl* vertex_decl);
		e_bool parseBones(FS::IFile& file);
		e_bool parseMeshes(const bgfx::VertexDecl& global_vertex_decl, FS::IFile& file, FileVersion version);
		e_bool parseMeshesOld(bgfx::VertexDecl global_vertex_decl, FS::IFile& file, FileVersion version);
		e_bool parseLODs(FS::IFile& file);
	public:
		IAllocator&		m_allocator;
		Renderer&		m_renderer;
		TArrary<Mesh>	m_meshes;
		TArrary<Bone>	m_bones;
		LOD				m_lods[MAX_LOD_COUNT];
		e_uint32        m_lod_count;
		e_float			m_bounding_radius;
		BoneMap			m_bone_map;
		AABB			m_aabb;
		e_uint32		m_flags;
		e_uint32		m_loading_flags;
		e_int32			m_first_nonroot_bone_index;
	};
}

namespace egal
{
	class Renderer;
	class EntityManager : public ResourceManagerBase
	{
	public:
		EntityManager(Renderer& renderer, IAllocator& allocator)
			: ResourceManagerBase(allocator)
			, m_allocator(allocator)
			, m_renderer(renderer)
		{}

		~EntityManager() {}

	protected:
		virtual Resource* createResource() override;
		Resource* createResource(const ArchivePath& path) override;
		e_void destroyResource(Resource& resource) override;

	private:
		IAllocator& m_allocator;
		Renderer&	m_renderer;
	};
}
#endif
