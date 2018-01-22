#ifndef _pipeline_h_
#define _pipeline_h_
#pragma once

#include "common/egal-d.h"
#include "common/resource/resource_define.h"
#include "runtime/EngineFramework/buffer.h"
#include "runtime/EngineFramework/scene_manager.h"

struct lua_State;
namespace egal
{
	struct Draw2D;
	class FrameBuffer;
	struct Font;
	struct IAllocator;
	class Material;
	class ArchivePath;
	struct Pose;
	class Renderer;
	class SceneManager;
	class Entity;
	struct Mesh;
	class Material;
	class Texture;
	struct Frustum;

	struct InstanceData
	{
		const bgfx::InstanceDataBuffer* buffer;

		e_int32						instance_count;

		Mesh*						mesh;
	};

	struct View
	{
		e_uint8		bgfx_id;
		e_uint64	layer_mask;
		e_uint64	render_state;
		e_uint32	stencil;
		e_int32		pass_idx;

		CommandBufferGenerator command_buffer;
	};

	struct TerrainInstance
	{
		e_int32 m_count;
		//const TerrainInfo* m_infos[64];
	};

	struct PointLightShadowmap
	{
		ComponentHandle light;
		FrameBuffer*	framebuffer;
		float4x4		matrices[4];
	};

	struct BaseVertex
	{
		e_float		x, y, z;
		e_uint32	rgba;
		e_float		u;
		e_float		v;
	};


	class Pipeline
	{
	public:
		struct Stats
		{
			e_int32 draw_call_count;
			e_int32 instance_count;
			e_int32 triangle_count;
		};

		struct CustomCommandHandler
		{
			TDelegate<e_void> callback;
			e_char name[30];
			e_uint32 hash;
		};
	public:
		static Pipeline* create(Renderer& renderer, const ArchivePath& path, const e_char* define, IAllocator& allocator);
		static e_void destroy(Pipeline* pipeline);

	public:
		static e_void registerLuaAPI(lua_State* state);

		Pipeline(Renderer& renderer, const ArchivePath& path, const e_char* define, IAllocator& allocator);
		~Pipeline();

		e_void bindTexture(e_int32 uniform_idx, e_int32 texture_idx);
		e_void bindEnvironmentMaps(e_int32 irradiance_uniform_idx, e_int32 radiance_uniform_idx);
		e_void bindRenderbuffer(bgfx::TextureHandle* rb, e_int32 width, e_int32 height, e_int32 uniform_idx);
		e_void load();
		e_void cleanup();
		e_void onFileLoaded(FS::IFile& file, e_bool success);
		e_void createParticleBuffers();
		e_void createUniforms();
		e_void destroyUniforms();
		e_bool frame();
		e_int32 createUniform(const e_char* name);
		e_int32 createVec4ArrayUniform(const e_char* name, e_int32 num);
		e_bool hasScene();
		e_bool cameraExists(const e_char* slot_name);
		e_void enableBlending(const e_char* mode);
		e_void clear(e_uint32 flags, e_uint32 color);
		e_void renderPointLightLitGeometry();
		e_void executeCommandBuffer(const e_uint8* data, Material* material) const;
		e_void renderRigidMeshInstanced(const float4x4& float4x4, Mesh& mesh);
		e_void renderMeshes(const TArrary<EntityInstanceMesh>& meshes);
		e_void renderMeshes(const TArrary<TArrary<EntityInstanceMesh>>& meshes);
		e_void resize(e_int32 width, e_int32 height);

		e_int32 bindFramebufferTexture(const char* framebuffer_name, e_int32 renderbuffer_idx, e_int32 uniform_idx, e_uint32 flags);
		e_int32 addFramebuffer(const e_char* name, e_int32 width, e_int32 height, bool is_screen_size, float2 size_ratio, TArrary<FrameBuffer::RenderBuffer>& buffers);
		e_void clearLayerToViewMap();
		TextureHandle& getRenderbuffer(const e_char* framebuffer_name, e_int32 renderbuffer_idx);

		FrameBuffer* getFramebuffer(const e_char* framebuffer_name);
		e_void setFramebuffer(const e_char* framebuffer_name);
		e_void setScene(SceneManager* scene);
		SceneManager* getScene();

		e_int32 getWidth();
		e_int32 getHeight();

		e_float getFPS();
		e_void executeCustomCommand(const e_char* name);
		e_int32 newView(const e_char* debug_name, e_uint64 layer_mask);
		e_void saveRenderbuffer(const e_char* framebuffer, e_int32 render_buffer_index, const e_char* out_path);
		e_void copyRenderbuffer(const e_char* src_fb_name, e_int32 src_rb_idx, const e_char* dest_fb_name, e_int32 dest_rb_idx);
		e_void createCubeBuffers();
		e_void finishDeferredPointLightInstances(Material* material, const bgfx::InstanceDataBuffer* instance_buffer, e_int32 instance_count, e_bool is_intersecting, PointLightShadowmap* shadowmap);
		e_void removeFramebuffer(const e_char* framebuffer_name);
		e_void setMaterialDefine(e_int32 material_idx, const e_char* define, e_bool enabled);
		e_void renderLightVolumes(e_int32 material_index);
		e_void renderDecalsVolumes();
		e_void renderSpotLightShadowmap(ComponentHandle light);
		e_void renderOmniLightShadowmap(ComponentHandle light);
		e_void renderLocalLightShadowmaps(ComponentHandle camera, FrameBuffer** fbs, e_int32 framebuffers_count);
		e_void findExtraShadowcasterPlanes(const float3& light_forward, const Frustum& camera_frustum, const float3& camera_position, Frustum* shadow_camera_frustum);
		e_void renderShadowmap(e_int32 split_index);
		e_void renderDebugShapes();
		e_void renderDebugPoints();
		e_void renderDebugLines();
		e_void renderDebugTriangles();
		e_void setPointLightUniforms(ComponentHandle light_cmp);
		e_void setStencilRef(e_uint32 ref);
		e_void setStencilRMask(e_uint32 rmask);
		e_void setStencil(e_uint32 flags);
		e_void setActiveGlobalLightUniforms();
		e_void disableBlending();
		e_void enableDepthWrite();
		e_void disableDepthWrite();
		e_void enableAlphaWrite();
		e_void disableAlphaWrite();
		e_void enableRGBWrite();
		e_void disableRGBWrite();
		e_void renderPointLightInfluencedGeometry(ComponentHandle light);
		e_void renderPointLightInfluencedGeometry(const Frustum& frustum);
		e_uint64 getLayerMask(const e_char* layer);
		e_void drawQuadEx(e_float left, e_float top, e_float w, e_float h, e_float u0, e_float v0, e_float u1, e_float v1, e_int32 material_index);
		e_void drawQuadExMaterial(e_float left, e_float top, e_float w, e_float h, e_float u0, e_float v0, e_float u1, e_float v1, Material* material);
		e_void drawQuad(e_float left, e_float top, e_float w, e_float h, e_int32 material_index);
		e_void renderAll(const Frustum& frustum, e_bool render_grass, ComponentHandle camera, e_uint64 layer_mask);
		CustomCommandHandler& addCustomCommandHandler(const e_char* name);
		e_void setViewProjection(const float4x4& mtx, e_int32 width, e_int32 height);
		e_void setScissor(e_int32 x, e_int32 y, e_int32 width, e_int32 height);
		e_bool checkAvailTransientBuffers(e_uint32 num_vertices,
			const VertexDecl& decl,
			e_uint32 num_indices);
		e_void allocTransientBuffers(TransientVertexBuffer* tvb,
			e_uint32 num_vertices,
			const VertexDecl& decl,
			TransientIndexBuffer* tib,
			e_uint32 num_indices);

		TextureHandle createTexture(e_int32 width, e_int32 height, const e_uint32* rgba_data);
		e_void destroyTexture(TextureHandle texture);
		e_void setTexture(e_int32 slot,
			TextureHandle texture,
			UniformHandle uniform);

		e_void destroyUniform(UniformHandle uniform);
		UniformHandle createTextureUniform(const e_char* name);
		
		e_void frame(const TransientVertexBuffer& vertex_buffer,
			const TransientIndexBuffer& index_buffer,
			const float4x4& mtx,
			e_int32 first_index,
			e_int32 num_indices,
			e_uint64 render_states,
			struct ShaderInstance& shader_instance);

		e_void renderEntity(Entity& entity, Pose* pose, const float4x4& mtx);
		e_void renderSkinnedMesh(const Pose& pose, const Entity& model, const float4x4& matrix, const Mesh& mesh);
		e_void renderMultilayerRigidMesh(const Entity& model, const float4x4& float4x4, const Mesh& mesh);
		e_void renderRigidMesh(const float4x4& float4x4, Mesh& mesh, e_float depth);
		e_void renderMultilayerSkinnedMesh(const Pose& pose, const Entity& model, const float4x4& matrix, const Mesh& mesh);
		e_void toggleStats();
		e_void setWindowHandle(e_void* data);
		e_bool isReady() const;
		const Stats& getStats();
		ArchivePath& getPath();

		e_void finishInstances(e_int32 idx);
		e_void finishInstances();
		e_void setPass(const e_char* name);
		e_void applyCamera(const e_char* slot);
		ComponentHandle getAppliedCamera() const;

		e_void frame(const VertexBufferHandle& vertex_buffer, 
			const IndexBufferHandle& index_buffer, 
			const InstanceDataBuffer& instance_buffer,
			e_int32 count, 
			Material& material);
	public:
		TArrary<bgfx::UniformHandle> m_uniforms;
		e_int32						 m_global_textures_count;

		IAllocator&			m_allocator;
		bgfx::VertexDecl	m_deferred_point_light_vertex_decl;
		bgfx::VertexDecl	m_base_vertex_decl;
		TerrainInstance		m_terrain_instances[4];
		e_uint32			m_debug_flags;
		e_int32				m_view_idx;
		e_uint64			m_layer_mask;
		View				m_views[64];
		View*				m_current_view;
		e_int32				m_pass_idx;
		ArchivePath			m_path;
		Renderer&			m_renderer;
		SceneManager*		m_scene;


		FrameBuffer*			m_global_light_shadowmap;
		FrameBuffer*			m_current_framebuffer;
		FrameBuffer*			m_default_framebuffer;
		TArrary<FrameBuffer*>	m_framebuffers;
		
		TArrary<PointLightShadowmap> m_point_light_shadowmaps;
		
		InstanceData			m_instances_data[128];
		e_int32					m_instance_data_idx;
		ComponentHandle			m_applied_camera;
		
		bgfx::VertexBufferHandle	m_cube_vb;
		bgfx::IndexBufferHandle		m_cube_ib;

		bgfx::VertexBufferHandle	m_particle_vertex_buffer;
		bgfx::IndexBufferHandle		m_particle_index_buffer;

		e_bool m_is_current_light_global;
		e_bool m_is_rendering_in_shadowmap;
		e_bool m_is_ready;
		Frustum m_camera_frustum;

		TArrary<TArrary<EntityInstanceMesh>>* m_mesh_buffer;

		float4x4 m_shadow_viewprojection[4];
		e_int32 m_width;
		e_int32 m_height;
		String	m_define;

		lua_State*	m_lua_state;
		e_int32		m_lua_thread_ref;
		e_int32		m_lua_env;
		Stats		m_stats;

		TArrary<CustomCommandHandler> m_custom_commands_handlers;

		bgfx::UniformHandle m_bone_matrices_uniform;
		bgfx::UniformHandle m_layer_uniform;
		bgfx::UniformHandle m_terrain_scale_uniform;
		bgfx::UniformHandle m_rel_camera_pos_uniform;
		bgfx::UniformHandle m_terrain_params_uniform;
		bgfx::UniformHandle m_fog_color_density_uniform;
		bgfx::UniformHandle m_fog_params_uniform;
		bgfx::UniformHandle m_light_pos_radius_uniform;
		bgfx::UniformHandle m_light_color_attenuation_uniform;
		bgfx::UniformHandle m_light_color_indirect_intensity_uniform;
		bgfx::UniformHandle m_light_dir_fov_uniform;
		bgfx::UniformHandle m_shadowmap_matrices_uniform;
		bgfx::UniformHandle m_terrain_matrix_uniform;
		bgfx::UniformHandle m_decal_matrix_uniform;
		bgfx::UniformHandle m_emitter_matrix_uniform;
		bgfx::UniformHandle m_tex_shadowmap_uniform;
		bgfx::UniformHandle m_texture_uniform;
		bgfx::UniformHandle m_cam_view_uniform;
		bgfx::UniformHandle m_cam_proj_uniform;
		bgfx::UniformHandle m_cam_params;
		bgfx::UniformHandle m_cam_inv_view_uniform;
		bgfx::UniformHandle m_cam_inv_proj_uniform;
		bgfx::UniformHandle m_cam_inv_viewproj_uniform;
		bgfx::UniformHandle m_texture_size_uniform;
		bgfx::UniformHandle m_grass_max_dist_uniform;

		e_int32								m_layer_to_view_map[64];

		Material*							m_sky_material;
		Material*							m_debug_line_material;
		Material*							m_draw2d_material;
		Texture*							m_default_cubemap;

		bgfx::DynamicVertexBufferHandle		m_debug_vertex_buffers[32];
		bgfx::DynamicIndexBufferHandle		m_debug_index_buffer;

		e_int32								m_debug_buffer_idx;
		e_int32								m_has_shadowmap_define_idx;
		e_int32								m_instanced_define_idx;



		e_int32 texturehand;
		e_int32 depthBuff;
		e_int32 mulit;

	};
}
#endif
