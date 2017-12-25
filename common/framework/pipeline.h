#ifndef _pipeline_h_
#define _pipeline_h_
#pragma once

#include "common/type.h"
#include "common/math/egal_math.h"
#include "common/stl/delegate.h"
#include "common/struct.h"
#include "common/resource/resource_define.h"

struct lua_State;
namespace egal
{
	struct Draw2D;
	class FrameBuffer;
	struct Font;
	struct IAllocator;
	struct float4x4;
	class Material;
	class ArchivePath;
	struct Pose;
	class Renderer;
	class RenderScene;
	class Entity;

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
		static e_void registerLuaAPI(lua_State* state);

		virtual ~Pipeline() {}

		static Pipeline* create(Renderer& renderer, const ArchivePath& path, const e_char* define, IAllocator& allocator);
		static e_void destroy(Pipeline* pipeline);

		virtual e_void load() = 0;
		virtual e_bool render() = 0;
		virtual e_void resize(e_int32 width, e_int32 height) = 0;
		virtual TextureHandle& getRenderbuffer(const e_char* framebuffer_name, e_int32 renderbuffer_idx) = 0;
		virtual e_void setScene(RenderScene* scene) = 0;
		virtual RenderScene* getScene() = 0;
		virtual e_int32 getWidth() = 0;
		virtual e_int32 getHeight() = 0;
		virtual CustomCommandHandler& addCustomCommandHandler(const e_char* name) = 0;
		virtual e_void setViewProjection(const float4x4& mtx, e_int32 width, e_int32 height) = 0;
		virtual e_void setScissor(e_int32 x, e_int32 y, e_int32 width, e_int32 height) = 0;
		virtual e_bool checkAvailTransientBuffers(e_uint32 num_vertices,
			const bgfx::VertexDecl& decl,
			e_uint32 num_indices) = 0;
		virtual e_void allocTransientBuffers(bgfx::TransientVertexBuffer* tvb,
			e_uint32 num_vertices,
			const bgfx::VertexDecl& decl,
			bgfx::TransientIndexBuffer* tib,
			e_uint32 num_indices) = 0;
		virtual bgfx::TextureHandle createTexture(e_int32 width, e_int32 height, const e_uint32* rgba_data) = 0;
		virtual e_void destroyTexture(bgfx::TextureHandle texture) = 0;
		virtual e_void setTexture(e_int32 slot,
			bgfx::TextureHandle texture,
			bgfx::UniformHandle uniform) = 0;
		virtual e_void destroyUniform(bgfx::UniformHandle uniform) = 0;
		virtual bgfx::UniformHandle createTextureUniform(const e_char* name) = 0;
		virtual e_void render(const bgfx::TransientVertexBuffer& vertex_buffer,
			const bgfx::TransientIndexBuffer& index_buffer,
			const float4x4& mtx,
			e_int32 first_index,
			e_int32 num_indices,
			e_uint64 render_states,
			struct ShaderInstance& shader_instance) = 0;
		virtual e_void renderModel(Entity& entity, Pose* pose, const float4x4& mtx) = 0;
		virtual e_void toggleStats() = 0;
		virtual e_void setWindowHandle(e_void* data) = 0;
		virtual e_bool isReady() const = 0;
		virtual const Stats& getStats() = 0;
		virtual ArchivePath& getPath() = 0;
		virtual e_void callLuaFunction(const e_char* func) = 0;

		virtual ComponentHandle getAppliedCamera() const = 0;
		virtual e_void render(const VertexBufferHandle& vertex_buffer, 
			const IndexBufferHandle& index_buffer, 
			const InstanceDataBuffer& instance_buffer,
			e_int32 count, 
			Material& material) = 0;
	};
}
#endif
